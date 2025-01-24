#pragma once

#include "pybind11/pybind11.h"

namespace py = pybind11;

py::module get_MVData_module();

namespace mvstudio_core {
    enum DataItemType {
        NOT_IMPLEMENTED = -1,
        Image = 0,
        Points = 1,
        Cluster = 2
    };
}