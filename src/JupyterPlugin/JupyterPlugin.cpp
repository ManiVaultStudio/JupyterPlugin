#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include <Application.h>

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
#include <pybind11/pybind11.h>
#include <pybind11/subinterpreter.h>
#define slots Q_SLOTS

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

namespace py = pybind11;

std::unique_ptr<py::module> JupyterPlugin::mvCommunicationModule = {};

void JupyterPlugin::initMvCommunicationModule() {
    if (mvCommunicationModule) {
        return;
    }

    mvCommunicationModule = std::make_unique<py::module>(get_MVData_module());
    mvCommunicationModule->doc() = "Provides access to low level ManiVaultStudio core functions";
}

JupyterPlugin::JupyterPlugin(const mv::plugin::PluginFactory* factory) :
    mv::plugin::ViewPlugin(factory),
    _xeusKernel(std::make_unique<XeusKernel>())
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
    _mainPyInterpreter = std::make_unique<py::scoped_interpreter>();
}

void JupyterPlugin::startJupyterNotebook() const
{
    _xeusKernel->startKernel(_connectionFilePath, QString::fromStdString(getVersion().getVersionString()));
}

void JupyterPlugin::runScriptWithArgs(const QString& scriptPath, const QStringList& args)
{
    if (!_mainPyInterpreter) {
        init();
    }

    if (!Py_IsInitialized()) {
        qWarning() << "JupyterPlugin::runScriptWithArgs: Script not executed - interpreter is not initialized";
        return;
    }

    // TODO: remove and exchange with below
    py::gil_scoped_acquire acquire;
    //py::subinterpreter sub = py::subinterpreter::create();
    //py::subinterpreter_scoped_activate guard(sub);

    // Load the script from file
    std::ifstream file(scriptPath.toStdString());
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string script_code = buffer.str();

    try {

        // Insert manivault communication module into sys.modules
        py::module sys = py::module::import("sys");
        py::dict modules = sys.attr("modules");

        if (!modules.contains("mvstudio_core")) {
            JupyterPlugin::initMvCommunicationModule();
            modules["mvstudio_core"] = *JupyterPlugin::mvCommunicationModule;
        }

        if (_baseModules.empty()) {
            for (auto& [key, item] : modules) {
                std::string name = py::str(key);
                _baseModules.insert(name);
            }
        }

        // Set sys.argv
        py::list py_args = py::list();
        py_args.append(py::cast(QFileInfo(scriptPath).fileName().toStdString()));      // add file name
        for (const auto& arg : args) {
            py_args.append(py::cast(arg.toStdString()));                                // add arguments
        }
        sys.attr("argv") = py_args;

        // Execute the script in __main__'s context
        const py::module_ mainModule   = py::module_::import("__main__");
        const py::object mainNamespace = mainModule.attr("__dict__");

        py::exec(script_code, mainNamespace);

        // Run garbage collection
        py::module gc = py::module::import("gc");
        const auto collected = gc.attr("collect")();

        // Clean up modules for fresh start
        for (auto& [key, item] : modules) {
            std::string name = py::str(key);
            if (!_baseModules.contains(name)) {
                modules.attr("pop")(name, py::none());
            }
        }

    }
    catch (const py::error_already_set& e) {
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
