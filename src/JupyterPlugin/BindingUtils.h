#pragma once

#include <CoreInterface.h>
#include <Dataset.h>
#include <PointData/PointData.h>

#undef slots
#include <pybind11/buffer_info.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>       // a.o. for vector conversions
#define slots Q_SLOTS

#include <QString>
#include <QDebug>

#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

std::vector<QString> toQStringVec(const std::vector<std::string>& vec_str);

pybind11::buffer_info createBuffer(const pybind11::array& data);

template<class T>
pybind11::array populate_pyarray(mv::Dataset<Points>& inputPoints, unsigned int numPoints, unsigned int numDimensions)
{
    std::vector<unsigned int> dim_indices(numDimensions);
    std::iota(dim_indices.begin(), dim_indices.end(), 0);

    std::vector<T> data;
    size_t size = static_cast<size_t>(numPoints) * numDimensions;
    data.resize(size);

    inputPoints->populateDataForDimensions<std::vector<T>, std::vector<unsigned int>>(data, dim_indices);

    auto result = pybind11::array_t<T>({ numPoints, numDimensions });
    pybind11::buffer_info result_info = result.request();
    T* output = static_cast<T*>(result_info.ptr);
    for (size_t i = 0; i < size; i++) {
        output[i] = data[i];
    }
    return result;
}

template<typename T>
PointData::ElementTypeSpecifier getTypeSpecifier() {
    PointData::ElementTypeSpecifier res = PointData::ElementTypeSpecifier::float32;

    if (std::is_same_v <T, float>) {
        res = PointData::ElementTypeSpecifier::float32;
    }
    else if (std::is_same_v <T, biovault::bfloat16_t>) {
        res = PointData::ElementTypeSpecifier::bfloat16;
    }
    else if (std::is_same_v <T, std::int32_t>) {
        res = PointData::ElementTypeSpecifier::int32;
    }
    else if (std::is_same_v <T, std::uint32_t>) {
        res = PointData::ElementTypeSpecifier::uint32;
    }
    else if (std::is_same_v <T, std::int16_t>) {
        res = PointData::ElementTypeSpecifier::int16;
    }
    else if (std::is_same_v <T, std::uint16_t>) {
        res = PointData::ElementTypeSpecifier::uint16;
    }
    else if (std::is_same_v <T, std::int8_t>) {
        res = PointData::ElementTypeSpecifier::int8;
    }
    else if (std::is_same_v <T, std::uint8_t>) {
        res = PointData::ElementTypeSpecifier::uint8;
    }
    else
        qWarning() << "getTypeSpecifier: data type not implemented";

    return res;
}

// numpy default for 3d (2D+RGB) images in C format is BIP 
// so we don't do anything 
// Assumes input data is arranged contiguously in memory
// Each 2D band is complete before the next begins
// The data_out is Band Interleaved by Pixel
template<typename T, typename U>
void orient_multiband_imagedata_as_bip(const U* data_in, const std::array<size_t, 3>& shape, std::vector<T>& data_out, bool flip)
{
    if (flip) {
        // C order with flip 
        const size_t row_size = shape[1] * shape[2];
        const size_t num_rows = shape[0];

        // Copy starting at the last row of the data_in
        // to the first row of the data_out
        // and so flip up/down
        for (size_t i = 1; i <= num_rows; ++i) {
            auto source_offset = (num_rows - i) * row_size;
            for (size_t j = 0; j < row_size; ++j) {
                data_out[((i - 1) * row_size) + j] = static_cast<T>(data_in[source_offset + j]);
            }
        }
    }
    else {
        // Copy as is, numpy image is BIP 
        const size_t total = shape[0] * shape[1] * shape[2];
        // C order 
        for (size_t i = 0; i < total; ++i) {
            data_out[i] = static_cast<T>(data_in[i]);
        }
    }
}

// when conversion is needed
template<typename T, typename U>
void conv_img_points_from_numpy_array(const void* data_in, const std::array<size_t, 3>& shape, mv::Dataset<Points>& points, bool flip = false)
{
    const size_t band_size = shape[0] * shape[1];
    const size_t num_bands = shape[2];

    auto warnings = pybind11::module::import("warnings");
    auto builtins = pybind11::module::import("builtins");
    warnings.attr("warn")(
        "This numpy dtype was converted to float to match the ManiVault data model.",
        builtins.attr("UserWarning"));

    auto data_in_U = static_cast<const U*>(data_in);
    std::vector<T> data_out = {};
    data_out.resize(band_size * num_bands);

    if (num_bands == 1 && !flip)
    {
        for (size_t i = 0; i < band_size; ++i)
            data_out[i] = static_cast<const T>(data_in_U[i]);
    }
    else {
        orient_multiband_imagedata_as_bip<T, U>(data_in_U, shape, data_out, flip);
    }

    points->setData(std::move(data_out), num_bands);
}

// when types are the same image
template<class T>
void set_img_points_from_numpy_array(const void* data_in, const std::array<size_t, 3>& shape, mv::Dataset<Points>& points, bool flip = false)
{
    const size_t band_size = shape[0] * shape[1];
    const size_t num_bands = shape[2];

    std::vector<T> data_out = {};
    data_out.resize(band_size * num_bands);

    if (num_bands == 1 && !flip)
        std::memcpy(data_out.data(), static_cast<const T*>(data_in), band_size * sizeof(T));
    else
        orient_multiband_imagedata_as_bip<T, T>(static_cast<const T*>(data_in), shape, data_out, flip);

    points->setData(std::move(data_out), num_bands);
}

// when types are different, setting points
template<typename T, typename U>
void set_points_from_numpy_array_diff_type(const void* data_in, const std::array<size_t, 2>& shape, mv::Dataset<Points>& points, bool flip)
{
    auto warnings = pybind11::module::import("warnings");
    auto builtins = pybind11::module::import("builtins");
    warnings.attr("warn")(
        "This numpy dtype was converted to float to match the ManiVault data model.",
        builtins.attr("UserWarning"));

    const size_t num_values = shape[0] * shape[1];  // num_points * num_dims
    const U* data_in_U = static_cast<const U*>(data_in);

    std::vector<T> data_out = {};
    data_out.resize(num_values);

    for (size_t i = 0; i < num_values; ++i)
        data_out[i] = static_cast<const T>(data_in_U[i]);

    points->setData(std::move(data_out), shape[1]);
}

// when types are the same, setting points
template<typename T>
void set_points_from_numpy_array_same_type(const void* data_in, const std::array<size_t, 2>& shape, mv::Dataset<Points>& points, bool flip)
{
    const size_t num_values = shape[0] * shape[1];  // num_points * num_dims
    std::vector<T> data_out = {};
    data_out.resize(num_values);
    std::memcpy(data_out.data(), static_cast<const T*>(data_in), num_values * sizeof(T));

    points->setData(std::move(data_out), shape[1]);
}

template<typename T, typename U>
void set_points_from_numpy_array(const void* data_in, const std::array<size_t, 2>& shape, mv::Dataset<Points>& points, bool flip = false)
{
    if constexpr (std::is_same_v<T, U>)
        set_points_from_numpy_array_same_type<T>(data_in, shape, points, flip);
    else
        set_points_from_numpy_array_diff_type<T, U>(data_in, shape, points, flip);
}

template <typename T>
std::vector<T> py_array_to_vector(const pybind11::array_t<T>& array) {
    // Request a buffer (no copying) and ensure the array is contiguous
    pybind11::buffer_info buf = array.request();
    if (buf.ndim != 1) {
        throw std::runtime_error("Only 1D numpy arrays can be converted to std::vector");
    }

    // Convert the data pointer to a vector
    T* data_ptr = static_cast<T*>(buf.ptr);
    return std::vector<T>(data_ptr, data_ptr + buf.shape[0]);
}

/* Creates new or derived data
*  The Lambda GeneratePointsFunc controls the manner of creating the new point data
*  This function mainly converts the python data
*/
template <typename GeneratePointsFunc>
std::string add_point_data(const pybind11::array& data, const std::vector<std::string>& dimensionNames, GeneratePointsFunc generatePointsData)
{
    std::string guid = "";
    const pybind11::dtype dtype = data.dtype();
    const pybind11::buffer_info buf_info = createBuffer(data);
    const void* py_data_storage_ptr = buf_info.ptr;

    if (!(data.flags() & pybind11::array::c_style)) {
        qDebug() << "add_point_data: Numpy array must by c-contiguous, use np.ascontiguousarray(data)";
        return guid;
    }

    if (!py_data_storage_ptr) {
        qDebug() << "add_point_data: python data transfer failed";
        return guid;
    }

    if (buf_info.shape.size() != 2) {
        qDebug() << "add_point_data: require numpy data of shape (num_points, num_dims). Instead got: " << buf_info.shape;
        return guid;
    }

    const std::array<size_t, 2> shape = { static_cast<size_t>(buf_info.shape.front()), static_cast<size_t>(buf_info.shape.back()) };

    void (*point_setter)(const void* data_in, const std::array<size_t, 2>&shape, mv::Dataset<Points>&points, bool flip) = nullptr;

    // PointData is limited in its type support - hopefully the commented types wil be added soon
    if (dtype.is(pybind11::dtype::of<std::uint8_t>()))
        point_setter = set_points_from_numpy_array<std::uint8_t, std::uint8_t>;
    else if (dtype.is(pybind11::dtype::of<std::int8_t>()))
        point_setter = set_points_from_numpy_array<std::int8_t, std::int8_t>;
    else if (dtype.is(pybind11::dtype::of<std::uint16_t>()))
        point_setter = set_points_from_numpy_array<std::uint16_t, std::uint16_t>;
    else if (dtype.is(pybind11::dtype::of<std::int16_t>()))
        point_setter = set_points_from_numpy_array<std::int16_t, std::int16_t>;
    // 32 int are cast to float
    else if (dtype.is(pybind11::dtype::of<std::uint32_t>()))
        point_setter = set_points_from_numpy_array<float, std::uint32_t>;
    else if (dtype.is(pybind11::dtype::of<std::int32_t>()))
        point_setter = set_points_from_numpy_array<float, std::int32_t>;
    //Unsupported <std::uint64_t> :
    //Unsupported <std::int64_t> :
    else if (dtype.is(pybind11::dtype::of<float>()))
        point_setter = set_points_from_numpy_array<float, float>;
    else if (dtype.is(pybind11::dtype::of<double>()))
        point_setter = set_points_from_numpy_array<float, double>;
    else
    {
        qDebug() << "add_point_data: type not supported (e.g. uint64_t or int64_t): " << QString(dtype.kind());
        return guid;
    }

    if (point_setter != nullptr)
    {
        mv::Dataset<Points> points = generatePointsData();

        point_setter(py_data_storage_ptr, shape, points, false);

        if (dimensionNames.size() == shape[1])
            points->setDimensionNames(toQStringVec(dimensionNames));

        mv::events().notifyDatasetDataChanged(points);

        guid = points.getDatasetId().toStdString();

        qDebug() << "add_point_data: " << QString(guid.c_str());
    }
    else
    {
        auto warnings = pybind11::module::import("warnings");
        auto builtins = pybind11::module::import("builtins");
        warnings.attr("warn")(
            "This numpy dtype could not be converted to a ManiVaultStudio supported type.",
            builtins.attr("UserWarning"));
    }

    return guid;
}
