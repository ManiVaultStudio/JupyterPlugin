#pragma once

#include "pybind11/pybind11.h"

namespace mvstudio_core {
    enum DataItemType {
        NOT_IMPLEMENTED = -1,
        Image = 0,
        Points = 1,
        Cluster = 2
    };

    void init_binding(pybind11::module_& m);
}
