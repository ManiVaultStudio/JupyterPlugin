#pragma once

#include "pybind11/pybind11.h"
#include "pybind11/functional.h"
#include <exception>

namespace py = pybind11;

py::module get_MVData_module();

namespace mvstudio_core {
    enum DataItemType {
        Image,
        Points,
        Cluster
    };
}