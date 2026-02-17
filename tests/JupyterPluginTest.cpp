#include <iostream>

#include <QString>

#undef slots
#include <pybind11/embed.h>
#include <pybind11/subinterpreter.h>
#define slots Q_SLOTS

#include "PythonUtils.h"

namespace py = pybind11;

// Define a test module
PYBIND11_EMBEDDED_MODULE(my_module, m, py::multiple_interpreters::per_interpreter_gil()) {
    m.def("hello", []() {
        return "Hello from interpreter: " + std::to_string(py::subinterpreter::current().id());
        });

    m.attr("value") = 42;
}

namespace Test {
    void runPython();
}

// Run with e.g. ./JupyterPluginTest.exe C:/Users/default/AppData/Local/miniconda3/envs/mv_13_run/python.exe
int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Please provide a string argument\n";
        return 1;
    }

    const auto pythonInterpreterPath = QString(argv[1]);
    const auto pythonVersion = getPythonVersion(pythonInterpreterPath);
    std::cout << "pythonInterpreterPath: " << pythonInterpreterPath.toStdString() << "\n";
    std::cout << "pythonVersion: " << pythonVersion.toStdString() << "\n";

    setPythonEnv(pythonInterpreterPath, pythonVersion, "");

    std::cout << " --- START::runPython --- " << std::endl;
    Test::runPython();
    std::cout << " --- FINISHED::runPython --- " << std::endl;

    return 0;
}

namespace Test
{
    void runPython()
    {
        py::scoped_interpreter main_interpreter{};

        // --- Main interpreter ---
        {
            const auto mod = py::module_::import("my_module");
            py::print("Main interpreter:", mod.attr("hello")());
            py::print("Main value:", mod.attr("value"));

            // Mutate something to prove isolation later
            mod.attr("value") = 100;
            py::print("Main value after mutation:", mod.attr("value"));
        }

        // --- Subinterpreter ---
        {
            py::subinterpreter sub = py::subinterpreter::create();
            py::subinterpreter_scoped_activate guard(sub);

            // Import runs the PYBIND11_EMBEDDED_MODULE init fresh
            const auto mod = py::module_::import("my_module");
            py::print("Sub interpreter:", mod.attr("hello")());

            // value is back to 42 - completely separate from main
            py::print("Sub value:", mod.attr("value"));
        } 

        // --- Back in main interpreter ---
        {
            const auto mod = py::module_::import("my_module");
            // Still 100 - main interpreter was unaffected by the sub
            py::print("Main value after sub destroyed:", mod.attr("value"));
        }
    }

} // namespace TEST
