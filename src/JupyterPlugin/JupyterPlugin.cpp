#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "MVData.h"
#include "XeusKernel.h"

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#undef slots
#include <pybind11/cast.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
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
    // start the interpreter and keep it alive
    if (!_init_guard) {
        _init_guard = std::make_unique<pybind11::scoped_interpreter>();
    }

    if (!Py_IsInitialized()) {
        qWarning() << "Script not executed";
        return;
    }

    // Load the script from file
    std::ifstream file(scriptPath.toStdString());
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string script_code = buffer.str();

    // Acquire GIL
    py::gil_scoped_acquire acquire;

    try {

        // Insert manivault communication module into sys.modules
        py::module sys = py::module::import("sys");
        py::dict modules = sys.attr("modules");

        if (!modules.contains("mvstudio_core")) {
            JupyterPlugin::init_mv_communication_module();
            sys.attr("modules")["mvstudio_core"] = *(JupyterPlugin::mv_communication_module.get());
        }

        // Set sys.argv
        py::list py_args = py::list();
        py_args.append(py::cast(QFileInfo(scriptPath).fileName().toStdString()));      // add file name
        for (const auto& arg : args) {
            py_args.append(py::cast(arg.toStdString()));                                // add arguments
        }
        sys.attr("argv") = py_args;

        // Execute the script in __main__'s context
        py::module_ main_module = py::module_::import("__main__");
        py::object main_namespace = main_module.attr("__dict__");

        py::exec(script_code, main_namespace);
    }
    catch (const py::error_already_set& e) {
        QString err = QStringLiteral("Python error (probably form script): ") + e.what();
        qWarning() << err;
    }
    catch (const std::runtime_error& e) {
        QString err = QStringLiteral("std::runtime_error: ") + e.what();
        qWarning() << err;
    }
    catch (const std::exception& e) {
        QString err = QStringLiteral("std::exception: ") + e.what();
        qWarning() << err;
    }
    catch (...) {
        QString err = QStringLiteral("Python error: unkown");
        qWarning() << err;
    }

    // GIL is released when 'guard' goes out of scope
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
