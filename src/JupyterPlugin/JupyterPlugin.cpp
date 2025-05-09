#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "MVData.h"
#include "XeusKernel.h"

#include <exception>

#undef slots
#include <pybind11/cast.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <stdexcept>
#define slots Q_SLOTS

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

using namespace mv;
namespace py = pybind11;
namespace py = pybind11;

std::unique_ptr<pybind11::module> JupyterPlugin::mv_communication_module = {};

void JupyterPlugin::init_mv_communication_module() {
    if (mv_communication_module) {
        return;
    }

    mv_communication_module = std::make_unique<pybind11::module>(get_MVData_module());
    mv_communication_module->doc() = "Provides access to low level ManiVaultStudio core functions";
}

JupyterPlugin::JupyterPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _pKernel(std::make_unique<XeusKernel>()),
    _connectionFilePath(this, "Connection file", QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json"))
{
}

JupyterPlugin::~JupyterPlugin()
{
    _pKernel->stopKernel();
}

// https://pybind11.readthedocs.io/en/stable/reference.html#_CPPv422initialize_interpreterbiPPCKcb
void JupyterPlugin::init()
{  
    QString jupyter_configFilepath = _connectionFilePath.getFilePath();
    QString pluginVersion = QString::fromStdString(getVersion().getVersionString());

    _init_guard = std::make_unique<pybind11::scoped_interpreter>(); // start the interpreter and keep it alive

    _pKernel->startKernel(jupyter_configFilepath, pluginVersion);
}

void JupyterPlugin::runScriptWithArgs(const QString& scriptPath, const QStringList& args)
{
    PythonWorker* workerThread = new PythonWorker(this, scriptPath, args);
    connect(workerThread, &PythonWorker::resultReady, this, [](const QString& s) { qDebug() << s; });
    connect(workerThread, &PythonWorker::errorMessage, this, [](const QString& s) { qWarning() << s; });
    connect(workerThread, &PythonWorker::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
}

// =============================================================================
// Plugin Factory
// =============================================================================

ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { };
}

// =============================================================================
// PythonWorker
// =============================================================================

PythonWorker::PythonWorker(QObject* parent, const QString& scriptPath, const QStringList& args) :
    QThread(parent),
    _scriptPath(scriptPath),
    _args(args)
{
}

void PythonWorker::run() {

    if (Py_IsInitialized()) {
        emit errorMessage("Script not executed");
        return;
    }

    try {
        // Acquire GIL in this thread
        pybind11::scoped_interpreter guard{};

        // Insert manivault communication module into sys.modules
        py::module sys = py::module::import("sys");
        py::dict modules = sys.attr("modules");

        if (!modules.contains("mvstudio_core")) {
            JupyterPlugin::init_mv_communication_module();
            sys.attr("modules")["mvstudio_core"] = *(JupyterPlugin::mv_communication_module.get());
        }

        // Set sys.argv
        py::list py_args = py::list();
        py_args.append(py::cast(QFileInfo(_scriptPath).fileName().toStdString()));      // add file name
        for (const auto& arg : _args) {
            py_args.append(py::cast(arg.toStdString()));                                // add arguments
        }
        sys.attr("argv") = py_args;

        // Get the __main__ module's dictionary for THIS sub-interpreter
        // This will be the global scope for eval_file.
        py::module_ sub_main_module = py::module_::import("__main__");
        py::object sub_main_dict = sub_main_module.attr("__dict__");

        // Execute the script in __main__'s context
        py::eval_file(_scriptPath.toStdString(), sub_main_dict);

        emit resultReady("Script executed");
    }
    catch (const py::error_already_set& e) {
        QString err = QStringLiteral("Python error (probably form script): ") + e.what();
        emit errorMessage(err);
    }
    catch (const std::runtime_error& e) {
        QString err = QStringLiteral("std::runtime_error: ") + e.what();
        emit errorMessage(err);
    }
    catch (const std::exception& e) {
        QString err = QStringLiteral("std::exception: ") + e.what();
        emit errorMessage(err);
    }
    catch (...) {
        QString err = QStringLiteral("Python error: unkown");
        emit errorMessage(err);
    }

    // GIL is released when 'guard' goes out of scope
}
