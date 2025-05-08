#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QtConcurrent>

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

    //auto runScript = [&scriptPath, args]() -> void {
    //    // Insert manivault communication module into sys.modules
    //    py::module sys = py::module::import("sys");
    //    py::dict modules = sys.attr("modules");

    //    if (!modules.contains("mvstudio_core")) {
    //        py::module MVData_module = *(JupyterPlugin::mv_communication_module.get());

    //        sys.attr("modules")["mvstudio_core"] = MVData_module;
    //    }

    //    // Set sys.argv
    //    py::list py_args = py::list();
    //    py_args.append(py::cast(QFileInfo(scriptPath).fileName().toStdString()));   // add file name
    //    for (const auto& arg : args) {
    //        py_args.append(py::cast(arg.toStdString()));                            // add arguments
    //    }
    //    sys.attr("argv") = py_args;

    //    // Get the __main__ module's dictionary for THIS sub-interpreter
    //    // This will be the global scope for eval_file.
    //    py::module_ sub_main_module = py::module_::import("__main__");
    //    py::object sub_main_dict = sub_main_module.attr("__dict__");

    //    // Execute the script in __main__'s context
    //    py::eval_file(scriptPath.toStdString(), sub_main_dict);

    //    };

    //auto runScriptInPythonSubprocess = [runScript, &scriptPath, &args]() -> void {
    //    try {
    //        if (Py_IsInitialized()) {
    //            // --- Scenario 1: Interpreter Already Exists ---
    //            PyThreadState* sub_tstate = nullptr; // Pointer for the sub-interpreter's state

    //            // Acquire GIL to use C API for sub-interpreters
    //            py::gil_scoped_acquire acquire_gil;

    //            // Create a sub-interpreter
    //            sub_tstate = Py_NewInterpreter();

    //            if (!sub_tstate) {
    //                qDebug() << "C++: Failed to create sub-interpreter!";
    //                if (PyErr_Occurred()) PyErr_Print(); // Use C API error printing here
    //                // No need to release GIL manually, gil_scoped_acquire handles it on exit
    //                return;
    //            }
    //            qDebug() << "C++: Created sub-interpreter with thread state: " << sub_tstate;

    //            // Execute code in the sub-interpreter
    //            qDebug() << "C++: Switching to sub-interpreter context...";
    //            PyThreadState* original_tstate = PyThreadState_Swap(sub_tstate); // Swap TO sub-interpreter
    //            qDebug() << "C++: Now running in context: " << PyThreadState_Get();
    //            qDebug() << "C++: Previous original: " << original_tstate;

    //            runScript(scriptPath, args);

    //            qDebug() << "C++: Switching back to original context (" << original_tstate << ")...";
    //            PyThreadState_Swap(original_tstate); // Swap BACK to original state
    //            qDebug() << "C++: Now running in context: " << PyThreadState_Get();

    //            if (sub_tstate) {
    //                qDebug() << "C++: Explicitly ending sub-interpreter: " << sub_tstate;
    //                Py_EndInterpreter(sub_tstate);
    //                sub_tstate = nullptr; // Mark as ended
    //            }
    //        }
    //        else {
    //            // --- Scenario 2: Interpreter Does Not Exist ---
    //            py::scoped_interpreter guard{};
    //            runScript(scriptPath, args);
    //        }

    //    }
    //    catch (const std::exception& e) {
    //        qWarning() << "Python error: " << e.what();
    //    }
    //    catch (...) {
    //        qWarning() << "Python error: unkown error occurred";
    //    }
    //    };

    //JupyterPlugin::init_mv_communication_module();

    //auto future = QtConcurrent::run(runScriptInPythonSubprocess);


    //try {

    //    JupyterPlugin::init_mv_communication_module();

    //    if (Py_IsInitialized()) {
    //        // --- Scenario 1: Interpreter Already Exists ---
    //        PyThreadState* main_tstate_before_sub = PyThreadState_Get();
    //        PyThreadState* sub_tstate = nullptr; // Pointer for the sub-interpreter's state
    //        PyObject* sub_interpreter_main_dict = nullptr; // To store the __main__.__dict__ of the sub-interpreter

    //        // Acquire GIL to use C API for sub-interpreters
    //        py::gil_scoped_acquire acquire_gil;

    //        // Create a sub-interpreter
    //        sub_tstate = Py_NewInterpreter();

    //        if (!sub_tstate) {
    //            qDebug() << "C++: Failed to create sub-interpreter!";
    //            if (PyErr_Occurred()) PyErr_Print(); // Use C API error printing here
    //            // No need to release GIL manually, gil_scoped_acquire handles it on exit
    //            return;
    //        }
    //        qDebug() << "C++: Created sub-interpreter with thread state: " << sub_tstate;
    //        qDebug() << "C++: Previous original: " << main_tstate_before_sub;

    //        // Execute code in the sub-interpreter
    //        qDebug() << "C++: Switching to sub-interpreter context...";
    //        PyThreadState* original_tstate = PyThreadState_Swap(sub_tstate); // Swap TO sub-interpreter
    //        qDebug() << "C++: Now running in context: " << PyThreadState_Get();

    //        runScript();

    //        // Swap back to the original thread state
    //        qDebug() << "C++: Switching back to original context (" << original_tstate << ")...";
    //        if (PyThreadState_Swap(original_tstate) != sub_tstate) {
    //            qWarning() << "C++: WARNING - Unexpected thread state when swapping back!";
    //        }
    //        qDebug() << "C++: Now running in context: " << PyThreadState_Get();

    //        if (sub_tstate) {
    //            qDebug() << "C++: Explicitly ending sub-interpreter: " << sub_tstate;
    //            Py_EndInterpreter(sub_tstate);
    //            sub_tstate = nullptr; // Mark as ended
    //        }
    //    }
    //    else {
    //        // --- Scenario 2: Interpreter Does Not Exist ---
    //        py::scoped_interpreter guard{};
    //        runScript();
    //    }

    //}
    //catch (const py::error_already_set& e) {
    //    qWarning() << "Python error (probably form script): " << e.what();
    //}
    //catch (const std::runtime_error& e) {
    //    qWarning() << "std::runtime_error: " << e.what();
    //}
    //catch (const std::exception& e) {
    //    qWarning() << "::exception: " << e.what();
    //}
    //catch (...) {
    //    qWarning() << "Python error: unkown";
    //}

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
