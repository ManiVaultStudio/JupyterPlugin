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
#define slots Q_SLOTS

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

using namespace mv;
namespace py = pybind11;

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
    auto runScript = [](const QString& scriptPath, const QStringList& args) {
        // Insert manivault communication module into sys.modules
        py::module MVData_module = get_MVData_module();
        MVData_module.doc() = "Provides access to low level ManiVaultStudio core functions";

        py::module sys = py::module::import("sys");
        sys.attr("modules")["mvstudio_core"] = MVData_module;

        // Set sys.argv
        py::list py_args = py::list();
        for (const auto& arg : args) {
            py_args.append(py::cast(arg));
        }
        sys.attr("argv") = py_args;

        // Execute the script in __main__'s context
        py::object scope = py::module_::import("__main__").attr("__dict__");
        py::eval_file(scriptPath.toStdString(), scope);

        };

    try {

        if (Py_IsInitialized()) {
            // --- Scenario 1: Interpreter Already Exists ---
            PyThreadState* sub_tstate = nullptr; // Pointer for the sub-interpreter's state

            // Acquire GIL to use C API for sub-interpreters
            py::gil_scoped_acquire acquire_gil;

            // Create a sub-interpreter
            sub_tstate = Py_NewInterpreter();

            if (!sub_tstate) {
                qDebug() << "C++: Failed to create sub-interpreter!";
                if (PyErr_Occurred()) PyErr_Print(); // Use C API error printing here
                // No need to release GIL manually, gil_scoped_acquire handles it on exit
                return;
            }
            qDebug() << "C++: Created sub-interpreter with thread state: " << sub_tstate;

            // Execute code in the sub-interpreter
            qDebug() << "C++: Switching to sub-interpreter context...";
            PyThreadState* original_tstate = PyThreadState_Swap(sub_tstate); // Swap TO sub-interpreter
            qDebug() << "C++: Now running in context: " << PyThreadState_Get();
            qDebug() << "C++: Previous original: " << original_tstate;

            runScript(scriptPath, args);

            qDebug() << "C++: Switching back to original context (" << original_tstate << ")...";
            PyThreadState_Swap(original_tstate); // Swap BACK to original state
            qDebug() << "C++: Now running in context: " << PyThreadState_Get();

            if (sub_tstate) {
                qDebug() << "C++: Explicitly ending sub-interpreter: " << sub_tstate;
                Py_EndInterpreter(sub_tstate);
                sub_tstate = nullptr; // Mark as ended
            }
        }
        else {
            // --- Scenario 2: Interpreter Does Not Exist ---
            py::scoped_interpreter guard{};
            runScript(scriptPath, args);
        }

    }
    catch (const std::exception& e) {
        qWarning() << "Python error: " << e.what();
    }

    // Interpreter is automatically finalized when 'guard' goes out of scope
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
