#include "MVData.h""
#include "pybind11/pybind11.h"
#include "xeus-python/xinterpreter.hpp"

namespace py = pybind11;

py::object get_info()
{
    return py::str("ManiVault is cool");
}

py::module get_MVData_module()
{
    py::module MVData_module = py::module_::create_extension_module("MVData", nullptr, new py::module_::module_def);
    MVData_module.def("get_info", get_info);

    return MVData_module;
}