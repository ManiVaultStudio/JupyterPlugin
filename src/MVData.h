#pragma once

#include "pybind11/pybind11.h"
#include "pybind11/functional.h"
#include <exception>
// TBD should be generated from PROJECT_VERSION cmake/config.py.in
#define JUPYTERPLUGIN_VERSION "0.1.0"

namespace py = pybind11;

py::module get_MVData_module();

namespace mvstudio_core {
    enum DataItemType {
        Image,
        Points,
        Cluster
    };
}