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

namespace Py = pybind11;

std::unique_ptr<Py::module> JupyterPlugin::mvCommunicationModule = {};

void JupyterPlugin::initMvCommunicationModule() {
    if (mvCommunicationModule) {
        return;
    }

    mvCommunicationModule = std::make_unique<Py::module>(get_MVData_module());
    mvCommunicationModule->doc() = "Provides access to low level ManiVaultStudio core functions";
}

JupyterPlugin::JupyterPlugin(const mv::plugin::PluginFactory* factory) :
    mv::plugin::ViewPlugin(factory),
    _xeusKernel(std::make_unique<XeusKernel>()),
    _connectionFilePath(QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json"))
{
}

JupyterPlugin::~JupyterPlugin()
{
    _xeusKernel->stopKernel();
}

// https://pybind11.readthedocs.io/en/stable/reference.html#_CPPv422initialize_interpreterbiPPCKcb
void JupyterPlugin::init()
{  
    // start the interpreter and keep it alive
    _initGuard = std::make_unique<Py::scoped_interpreter>();
}

void JupyterPlugin::startJupyterNotebook() const
{
    _xeusKernel->startKernel(_connectionFilePath, QString::fromStdString(getVersion().getVersionString()));
}

void JupyterPlugin::runScriptWithArgs(const QString& scriptPath, const QStringList& args)
{
    // start the interpreter and keep it alive
    if (!_initGuard) {
        _initGuard = std::make_unique<pybind11::scoped_interpreter>();
    }

    if (!Py_IsInitialized()) {
        qWarning() << "Script not executed";
        return;
    }

    // Load the script from file
    std::ifstream file(scriptPath.toStdString());
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string script_code = buffer.str();

    // Acquire GIL
    Py::gil_scoped_acquire acquire;

    try {

        // Insert manivault communication module into sys.modules
        Py::module sys = Py::module::import("sys");
        Py::dict modules = sys.attr("modules");

        if (!modules.contains("mvstudio_core")) {
            JupyterPlugin::initMvCommunicationModule();
            modules["mvstudio_core"] = *(JupyterPlugin::mvCommunicationModule.get());
        }

        if (_baseModules.empty()) {
            for (auto& [key, item] : modules) {
                std::string name = Py::str(key);
                _baseModules.insert(name);
            }
        }

        // Set sys.argv
        Py::list py_args = Py::list();
        py_args.append(Py::cast(QFileInfo(scriptPath).fileName().toStdString()));      // add file name
        for (const auto& arg : args) {
            py_args.append(Py::cast(arg.toStdString()));                                // add arguments
        }
        sys.attr("argv") = py_args;

        // Execute the script in __main__'s context
        Py::module_ main_module = Py::module_::import("__main__");
        Py::object main_namespace = main_module.attr("__dict__");

        Py::exec(script_code, main_namespace);

        // Run garbage collection
        Py::module gc = Py::module::import("gc");
        gc.attr("collect")();

        // Clean up modules for fresh start
        for (auto& [key, item] : modules) {
            std::string name = Py::str(key);
            if (!_baseModules.contains(name)) {
                modules.attr("pop")(name, Py::none());
            }
        }

    }
    catch (const Py::error_already_set& e) {
        const auto err = QStringLiteral("Python error (probably form script): ") + e.what();
        qWarning() << err;
    }
    catch (const std::runtime_error& e) {
        const auto err = QStringLiteral("std::runtime_error: ") + e.what();
        qWarning() << err;
    }
    catch (const std::exception& e) {
        const auto err = QStringLiteral("std::exception: ") + e.what();
        qWarning() << err;
    }
    catch (...) {
        const auto err = QStringLiteral("Python error: unknown");
        qWarning() << err;
    }

    // GIL is released when 'guard' goes out of scope
}

// =============================================================================
// Plugin Factory
// =============================================================================

mv::plugin::ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { };
}
