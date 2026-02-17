#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

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

#include <Application.h>

#include "MVData.h"
#include "XeusKernel.h"

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(mvstudio_core, m, py::multiple_interpreters::per_interpreter_gil()) {
    m.doc() = "Provides access to low level ManiVaultStudio core functions";
    //m.attr("__version__") = tba;
    mvstudio_core::register_mv_data_items(m);
    mvstudio_core::register_mv_core_module(m);
}

}

JupyterPlugin::JupyterPlugin(const mv::plugin::PluginFactory* factory) :
    mv::plugin::ViewPlugin(factory)
{
}

JupyterPlugin::~JupyterPlugin()
{
    _xeusKernel->stopKernel();
}

void JupyterPlugin::init()
{  
    _xeusKernel = std::make_unique<XeusKernel>();

    // start the interpreter and keep it alive
    _mainPyInterpreter = std::make_unique<py::scoped_interpreter>();
    auto pyModMv = py::module::import("mvstudio_core");
}

void JupyterPlugin::startJupyterNotebook() const
{
    if (!Py_IsInitialized()) {
        qWarning() << "JupyterPlugin::startJupyterNotebook: could not start notebook - interpreter is not initialized";
        return;
    }
    _xeusKernel->startKernel(_connectionFilePath, QString::fromStdString(getVersion().getVersionString()));
}

void JupyterPlugin::runScriptWithArgs(const QString& scriptPath, const QStringList& args)
{
    if (!Py_IsInitialized()) {
        qWarning() << "JupyterPlugin::runScriptWithArgs: Script not executed - interpreter is not initialized";
        return;
    }

    // Sub-interpreter will go out of scope after script is executed
    py::subinterpreter sub = py::subinterpreter::create();
    py::subinterpreter_scoped_activate guard(sub);

    // Load the script from file
    std::ifstream file(scriptPath.toStdString());
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string scriptCode = buffer.str();

    try {

        // Insert manivault communication module into sys.modules
        auto pyModSys = py::module::import("sys");
        auto pyModMv = py::module::import("mvstudio_core");


        if (_baseModules.empty()) {
            for (auto& [key, item] : modules) {
                _baseModules.insert(py::str(key));
            }
        }

        // Set sys.argv
        auto pyArgs = py::list();
        pyArgs.append(py::cast(QFileInfo(scriptPath).fileName().toStdString()));      // add file name
        for (const auto& arg : args) {
            pyArgs.append(py::cast(arg.toStdString()));                                // add arguments
        }
        pyModSys.attr("argv") = pyArgs;

        // Execute the script in __main__'s context
        const py::module_ mainModule   = py::module_::import("__main__");
        const py::object mainNamespace = mainModule.attr("__dict__");

        py::exec(scriptCode, mainNamespace);

        // Run garbage collection
        py::module gc = py::module::import("gc");
        const auto collected = gc.attr("collect")();

        // Clean up modules for fresh start
        for (auto& [key, item] : modules) {
            
            if (std::string name = py::str(key);
                !_baseModules.contains(name)) {
                modules.attr("pop")(name, py::none());
            }
        }

    }
    catch (const py::error_already_set& e) {
        qWarning() << QStringLiteral("Python error (probably from script):");
        qWarning() << e.what();
    }
    catch (const std::runtime_error& e) {
        qWarning() << QStringLiteral("std::runtime_error:");
        qWarning() << e.what();
    }
    catch (const std::exception& e) {
        qWarning() << QStringLiteral("std::exception:");
        qWarning() << e.what();
    }
    catch (...) {
        qWarning() << QStringLiteral("Python error: unknown");
    }

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
