#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include <exception>
#include <format>
#include <fstream>
#include <set>
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
#include "PythonBuildVersion.h"
#include "XeusKernel.h"

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(mvstudio_core, m, py::multiple_interpreters::per_interpreter_gil()) {
    m.doc() = "Provides access to low level ManiVaultStudio core functions";
    m.attr("__version__") = std::format("{}.{}.{}", pythonBridgeVersionMajor, pythonBridgeVersionMinor, pythonBridgeVersionPatch);
    mvstudio_core::register_mv_data_items(m);
    mvstudio_core::register_mv_core_module(m);
}

namespace
{
    void importMvModule()
    {
        if (const py::dict modules = py::module::import("sys").attr("modules");
            !modules.contains("mvstudio_core")) {
            auto pyModMv = py::module::import("mvstudio_core");
        }
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
    // start the interpreter and keep it alive
    _mainPyInterpreter = std::make_unique<py::scoped_interpreter>();
    importMvModule();

    // save global base state
    for (const py::dict loadedModules = py::module::import("sys").attr("modules");
        auto& [key, _] : loadedModules) {
        _baseModules.insert(py::str(key));
    }
}

void JupyterPlugin::startJupyterNotebook()
{
    if (!Py_IsInitialized()) {
        qWarning() << "JupyterPlugin::startJupyterNotebook: could not start notebook - interpreter is not initialized";
        return;
    }

    _xeusKernel = std::make_unique<XeusKernel>();
    _xeusKernel->startKernel(_connectionFilePath, QString::fromStdString(getVersion().getVersionString()));
}

void JupyterPlugin::cleanGlobalNamespace() const
{
    // Clear user-defined globals (keep builtins)
    py::dict pythonGlobals = py::module_::import("__main__").attr("__dict__");

    // Remove everything except builtins and special attributes
    std::set<std::string> toRemove;
    for (auto& [key, _] : pythonGlobals) {

        if (std::string globalName = py::str(key);
            !globalName.empty() && globalName[0] != '_') {  // keep __name__, __builtins__, etc.
            toRemove.insert(globalName);
        }
    }

    for (const auto& key : toRemove) {
        pythonGlobals.attr("pop")(key);
    }

    // Clean up modules for fresh start
    for (const py::dict loadedModules = py::module::import("sys").attr("modules");
        auto& [key, _] : loadedModules) {

        if (std::string moduleName = py::str(key);
            !_baseModules.contains(moduleName)) {
            loadedModules.attr("pop")(moduleName, py::none());
        }
    }

    // Run garbage collection
    const py::module gc = py::module::import("gc");
    const auto gcResult = gc.attr("collect")();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
// Cannot be static since we want to apply Q_INVOKABLE 
void JupyterPlugin::runScriptWithArgs(const QString& scriptPath, const QStringList& args)
{
    if (!Py_IsInitialized()) {
        qWarning() << "JupyterPlugin::runScriptWithArgs: Script not executed - interpreter is not initialized";
        return;
    }

    py::gil_scoped_acquire acquire;
    importMvModule();

    // Load the script from file
    std::ifstream file(scriptPath.toStdString());
    const std::string scriptCode(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>{}
    );

    try {
        // Set sys.argv
        auto pyArgs = py::list();
        pyArgs.append(py::cast(QFileInfo(scriptPath).fileName().toStdString()));      // add file name
        for (const auto& arg : args) {
            pyArgs.append(py::cast(arg.toStdString()));                               // add arguments
        }
        py::module_::import("sys").attr("argv") = pyArgs;

        // Execute the script in __main__'s context
        const py::module_ mainModule   = py::module_::import("__main__");
        const py::object mainNamespace = mainModule.attr("__dict__");

        py::exec(scriptCode, mainNamespace);

        cleanGlobalNamespace();
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
