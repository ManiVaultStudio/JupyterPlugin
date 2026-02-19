#include "BindingUtils.h"

namespace py = pybind11;

std::vector<QString> toQStringVec(const std::vector<std::string>& vec_str) {

    const auto n = static_cast<std::int64_t>(vec_str.size());

    std::vector<QString> vec_Qstr(n);

#pragma omp parallel for
    for (std::int64_t i = 0; i < n; ++i) {
        vec_Qstr[i] = QString::fromStdString(vec_str[i]);
    }

    return vec_Qstr;
}

py::buffer_info createBuffer(const py::array& data)
{
    py::buffer_info buf_info;

    // https://numpy.org/doc/stable/user/basics.copies.html#how-to-tell-if-the-array-is-a-view-or-a-copy
    if (data.base().is_none()) {
        buf_info = data.request();
    }
    else {
        py::print("The numpy array is a view. Creating a local copy to handle striding correctly...");

        pybind11::array::ShapeContainer shape(data.shape(), data.shape() + data.ndim());
        pybind11::array::ShapeContainer strides(data.strides(), data.strides() + data.ndim());

        const auto copied_arr = py::array(data.dtype(), shape, strides, data.data());

        buf_info = copied_arr.request();
    }

    return buf_info;
}

