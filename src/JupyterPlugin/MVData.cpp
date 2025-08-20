#include "MVData.h"

#include <Application.h>
#include <ClusterData/Cluster.h>
#include <ClusterData/ClusterData.h>
#include <Dataset.h>
#include <ImageData/ImageData.h>
#include <ImageData/Images.h>
#include <LinkedData.h>
#include <PointData/PointData.h>

#include <pybind11/buffer_info.h>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>       // a.o. for vector conversions

#include <QColor>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QDebug>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <map>
#include <numeric>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace py = pybind11;

static std::vector<QString> toQStringVec(const std::vector<std::string>& vec_str) {

    const std::int64_t n = static_cast<std::int64_t>(vec_str.size());

    std::vector<QString> vec_Qstr(n);

#pragma omp parallel for
    for (std::int64_t i = 0; i < n; ++i) {
        vec_Qstr[i] = QString::fromStdString(vec_str[i]);
    }

    return vec_Qstr;
}

static py::object get_info()
{
    return py::str("ManiVault is cool");
}

static py::object get_top_level_item_names()
{
    py::list result;
    for (const auto topLevelDataHierarchyItem : Application::core()->getDataHierarchyManager().getTopLevelItems()) {
        result.append(py::str(topLevelDataHierarchyItem->getDataset()->getGuiName().toUtf8().constData()));
    }
    return result;

}

template<class T>
py::array populate_pyarray(mv::Dataset<Points> &inputPoints, unsigned int numPoints, unsigned int numDimensions)
{
    std::vector<unsigned int> dim_indices(numDimensions);
    std::iota(dim_indices.begin(), dim_indices.end(), 0);

    std::vector<T> data;
    size_t size = static_cast<size_t>(numPoints) * numDimensions;
    data.resize(size);

    inputPoints->populateDataForDimensions<std::vector<T>, std::vector<unsigned int>>(data, dim_indices);

    auto result = py::array_t<T>({numPoints, numDimensions});
    py::buffer_info result_info = result.request();
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

// only works on top level items as test
// Get the point data associated with the names item and return it as a numpy array to python
static py::array get_data_for_item(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));

    // If this is not a point item we need the parent
    auto dataType = item->getDataType();
    if (dataType != PointType) {
        item = item->getParent();
        dataType = item->getDataType();
        if (dataType != PointType) {
            return py::array_t<float>(0);
        }
    }

    auto inputPoints            = item->getDataset<Points>();
    unsigned int numDimensions  = inputPoints->getNumDimensions();
    unsigned int numPoints      = inputPoints->isFull() ? inputPoints->getNumPoints() : inputPoints->indices.size();

    // extract the source type 
    PointData::ElementTypeSpecifier dataSpec{};
    inputPoints->visitSourceData([&dataSpec](auto pointData) {
        for (auto pointView : pointData) {
            for (auto value : pointView) {
                dataSpec = getTypeSpecifier<decltype(value)>();
                break;
            }
            break;
        }  
    });

    qDebug() << "PointData::ElementTypeSpecifier is " << static_cast<int>(dataSpec);
    if (auto populate =
        (dataSpec == PointData::ElementTypeSpecifier::float32) ? populate_pyarray<float> :
        (dataSpec == PointData::ElementTypeSpecifier::uint16) ? populate_pyarray<std::uint16_t> :
        (dataSpec == PointData::ElementTypeSpecifier::int16) ? populate_pyarray<std::int16_t> :
        (dataSpec == PointData::ElementTypeSpecifier::uint8) ? populate_pyarray<std::uint8_t> :
        (dataSpec == PointData::ElementTypeSpecifier::int8) ? populate_pyarray<std::int8_t> :
        nullptr) {
        return populate(inputPoints, numPoints, numDimensions);
    }

    return py::array_t<float>(0);
}

// Get the selected data points for a data set
static py::array get_selection_for_item(const std::string& datasetGuid)
{
    DataHierarchyItem* item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));

    if (!item)
        return {};

    auto data = item->getDataset();

    std::vector<std::uint32_t>& selectionIndices = data->getSelectionIndices();

    return py::array_t<uint32_t>(
        selectionIndices.size(),    // Number of elements
        selectionIndices.data()     // Pointer to data
    );
}

// Set the selected data points for a data set
static void set_selection_for_item(const std::string& datasetGuid, const py::array_t<uint32_t>& selectionIDs)
{
    DataHierarchyItem* item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));

    if (!item)
        return;

    auto data = item->getDataset();

    // Request a buffer info from the array
    py::buffer_info buf = selectionIDs.request();

    if (buf.ndim != 1)
        throw std::runtime_error("Input array must be 1-dimensional");

    // Copy the data into a std::vector
    uint32_t* ptr  = static_cast<uint32_t*>(buf.ptr);
    size_t len     = static_cast<size_t>(buf.shape[0]);
    auto selection = std::vector<uint32_t>(ptr, ptr + len);

    // Send selection to the core
    data->setSelectionIndices(std::move(selection));
    mv::events().notifyDatasetDataSelectionChanged(data);
}

// numpy default for 3d (2D+RGB) images in C format is BIP 
// so we don't do anything 
// Assumes input data is arranged contiguously in memory
// Each 2D band is complete before the next begins
// The data_out is is Band Interleaved by Pixel
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
                data_out[((i-1) * row_size) + j] = static_cast<T>(data_in[source_offset + j]);
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
void conv_points_from_numpy_array(const void* data_in, const std::array<size_t, 3>& shape, mv::Dataset<Points>& points, bool flip = false)
{
    const size_t band_size = shape[0] * shape[1];
    const size_t num_bands = shape[2];

    auto warnings = pybind11::module::import("warnings");
    auto builtins = pybind11::module::import("builtins");
    warnings.attr("warn")(
        "This numpy dtype was converted to float to match the ManiVault data model.",
        builtins.attr("UserWarning"));

    auto data_in_U = static_cast<const U *>(data_in);
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
void set_img_points_from_numpy_array(const void* data_in, const std::array<size_t, 3>& shape, mv::Dataset<Points>& points, bool flip=false)
{
    const size_t band_size = shape[0] * shape[1];
    const size_t num_bands = shape[2];

    std::vector<T> data_out = {};
    data_out.resize(band_size * num_bands);

    if (num_bands == 1 && !flip)
        std::memcpy(data_out.data(), static_cast<const T*>(data_in), band_size * sizeof(T));
    else
        orient_multiband_imagedata_as_bip<T,T>(static_cast<const T*>(data_in), shape, data_out, flip);
    
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
    const U* data_in_U      = static_cast<const U*>(data_in);

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

static py::buffer_info createBuffer(const py::array& data)
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

        py::array copied_arr = py::array(data.dtype(), shape, strides, data.data());

        buf_info = copied_arr.request();
    }

    return buf_info;
}

/**
 * Add new point data in the root of the hierarchy.
 * If successful returns a guid for the new point data
 * If unsuccessful return a empty string
 */
static std::string add_new_point_data(const py::array& data, const std::string& dataSetName, const std::string& dataSetParentID, const std::vector<std::string>& dimensionNames)
{
    std::string guid                    = "";
    const py::dtype dtype               = data.dtype();
    const py::buffer_info buf_info      = createBuffer(data);
    const void* py_data_storage_ptr     = buf_info.ptr;

    if(!py_data_storage_ptr) {
        qDebug() << "add_new_point_data: python data transfer failed";
        return guid;
    }

    if (buf_info.shape.size() != 2) {
        qDebug() << "add_new_point_data: require numpy data of shape (num_points, num_dims). Instead got: " << buf_info.shape;
        return guid;
    }

    const std::array<size_t, 2> shape   = { static_cast<size_t>(buf_info.shape.front()), static_cast<size_t>(buf_info.shape.back()) };

    void (*point_setter)(const void* data_in, const std::array<size_t, 2>&shape, mv::Dataset<Points>& points, bool flip) = nullptr;

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
        qDebug() << "add_new_image_data: type not supported (e.g. uint64_t or int64_t): " << QString(dtype.kind());
        return guid;
    }

    if (point_setter != nullptr)
    {
        Dataset<DatasetImpl> parentData = Dataset<DatasetImpl>();

        if (!dataSetParentID.empty()) {
            parentData = mv::data().getDataset(QString::fromStdString(dataSetParentID));
        }

        mv::Dataset<Points> points = mv::data().createDataset<Points>("Points", dataSetName.c_str(), parentData);
        point_setter(py_data_storage_ptr, shape, points, false);

        if(dimensionNames.size() == shape[1])
            points->setDimensionNames(toQStringVec(dimensionNames));

        events().notifyDatasetDataChanged(points);

        guid = points.getDatasetId().toStdString();

        qDebug() << "add_new_point_data: " << QString(guid.c_str());
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

// The MV data model is Band Interlaced by Pixel.
// This means that an image stack (which might be hyperspectral or simply RGB/RGBA)
// is counted as having size(stack) bands called dimensions in the MV Image.
// This function is meant to deal only with the 
// single image case however multiple RGB or RGBA bands may be present
// as given by the number of components
static std::string add_new_image_data(const py::array& data, const std::string& dataSetName, const std::vector<std::string>& dimensionNames)
{
    std::string guid                = "";
    const py::dtype dtype           = data.dtype();
    const py::buffer_info buf_info  = createBuffer(data);
    const void* py_data_storage_ptr = buf_info.ptr;

    if (!py_data_storage_ptr) {
        qDebug() << "add_new_point_data: python data transfer failed";
        return guid;
    }

    if (buf_info.shape.size() != 2 && buf_info.shape.size() != 3) {
        qDebug() << "add_new_image_data: numpy image must be an image, i.e. of shape (x, y, dims). Instead got: " << buf_info.shape;
        return guid;
    }

    const size_t num_bands = buf_info.shape.size() ? 
        /* if: Grey-scale image */ 1 :
        /* else: multiple dims  */ buf_info.shape[2];

    const size_t height = static_cast<size_t>(buf_info.shape[0]);
    const size_t width = static_cast<size_t>(buf_info.shape[1]);

    const std::array<size_t, 3> shape   = { height, width, num_bands };

    void (*point_setter)(const void* data_in, const std::array<size_t, 3>& shape, mv::Dataset<Points>& points, bool flip) = nullptr;

    // PointData is limited in its type support - hopefully the commented types wil be added soon
    if (dtype.is(pybind11::dtype::of<std::uint8_t>()))
        point_setter = conv_points_from_numpy_array<float, std::uint8_t>;
    else if (dtype.is(pybind11::dtype::of<std::int8_t>()))
        point_setter = conv_points_from_numpy_array<float, std::int8_t>;
    else if (dtype.is(pybind11::dtype::of<std::uint16_t>()))
        point_setter = conv_points_from_numpy_array<float, std::uint16_t>;
    else if (dtype.is(pybind11::dtype::of<std::int16_t>()))
        point_setter = conv_points_from_numpy_array<float, std::int16_t>;
    else if (dtype.is(pybind11::dtype::of<std::uint32_t>()))
        point_setter = conv_points_from_numpy_array<float, std::uint32_t>;
    else if (dtype.is(pybind11::dtype::of<std::int32_t>()))
        point_setter = conv_points_from_numpy_array<float, std::int32_t>;
    //Unsupported <std::uint64_t> :
    //Unsupported <std::int64_t> :
    else if (dtype.is(pybind11::dtype::of<float>()))
        point_setter = set_img_points_from_numpy_array<float>;
    else if (dtype.is(pybind11::dtype::of<double>()))
        point_setter = conv_points_from_numpy_array<float, double>;
    else
    {
        qDebug() << "add_new_image_data: type not supported (e.g. uint64_t or int64_t): " << QString(dtype.kind());
        return guid;
    }

    if (point_setter != nullptr)
    {
        mv::Dataset<Points> points = mv::data().createDataset<Points>("Points", dataSetName.c_str(), nullptr);
        point_setter(py_data_storage_ptr, shape, points, true);

        if (dimensionNames.size() == num_bands)
            points->setDimensionNames(toQStringVec(dimensionNames));

        events().notifyDatasetDataChanged(points);

        auto imageDataset = mv::data().createDataset<Images>("Images", "numpy image", Dataset<DatasetImpl>(*points));

        imageDataset->setText(QString("Images (%2x%3)").arg(QString::number(width), QString::number(height)));
        imageDataset->setType(ImageData::Type::Stack);
        imageDataset->setNumberOfImages(1);
        imageDataset->setImageSize(QSize(static_cast<int>(width), static_cast<int>(height)));
        imageDataset->setNumberOfComponentsPerPixel(static_cast<uint32_t>(num_bands));

        events().notifyDatasetDataChanged(imageDataset);

        guid = points.getDatasetId().toStdString();

        qDebug() << "add_new_image_data: " << QString(guid.c_str());
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

// All images are float in ManiVault 1.x.
// An Image parent may be a Points or Cluster item
static py::array get_mv_image(const std::string& imageGuid)
{
    auto item               = mv::dataHierarchy().getItem(QString(imageGuid.c_str()));
    auto images             = item->getDataset<Images>();
    uint32_t numImages      = images->getNumberOfImages();
    uint32_t numPixels      = images->getNumberOfPixels();
    uint32_t numComponents  = images->getNumberOfComponentsPerPixel();
    QSize imqSize           = images->getImageSize();
    unsigned int width      = static_cast<unsigned int>(imqSize.width());
    unsigned int height     = static_cast<unsigned int>(imqSize.height());
    size_t pixcmpSize       = static_cast<size_t>(numPixels) * numComponents;

    // For further information on why the numpy array shapes are 
    // specified as shown see 
    // https://scikit-image.org/skimage-tutorials/lectures/00_images_are_arrays.html#other-shapes-and-their-meanings

    std::vector<unsigned int> shape{0};

    if (numImages == 1) {
        if (numComponents == 1) {
            // 2D Grayscale
            shape = std::vector<unsigned int>({ height, width });
        }
        else {
            // 2D Multichannel
            shape = std::vector<unsigned int> { height, width, numComponents };
        }
    } else {

        if (numComponents == 1) {
            // 3D Grayscale
            shape = std::vector<unsigned int> { numImages, height, width };
        }
        else {
            // 3D Multichannel
            shape = std::vector<unsigned int> { numImages, height, width, numComponents };
        }
    }

    auto result                 = py::array_t<float>(shape);
    py::buffer_info result_info = result.request();
    float* output               = static_cast<float*>(result_info.ptr);

    for (uint32_t imIdx = 0; imIdx < numImages; ++imIdx) {
        QVector<float> scalarData(static_cast<size_t>(numPixels) * numComponents);
        QPair<float, float> scalarDataRange;
        images->getImageScalarData(imIdx, scalarData, scalarDataRange);

        for (size_t pixcmpIdx = 0; pixcmpIdx < pixcmpSize; ++pixcmpIdx)
            output[(imIdx * pixcmpSize) + pixcmpIdx] = scalarData[pixcmpIdx];
    }

    return result;
}

template <typename T>
std::vector<T> py_array_to_vector(const py::array_t<T>& array) {
    // Request a buffer (no copying) and ensure the array is contiguous
    py::buffer_info buf = array.request();
    if (buf.ndim != 1) {
        throw std::runtime_error("Only 1D numpy arrays can be converted to std::vector");
    }

    // Convert the data pointer to a vector
    T* data_ptr = static_cast<T*>(buf.ptr);
    return std::vector<T>(data_ptr, data_ptr + buf.shape[0]);
}

static std::string add_new_cluster_data(const std::string& parentPointDatasetGuid, const std::vector<py::array>& clusterIndices, const std::vector<std::string>& clusterNames, const std::vector<py::array>& clusterColors, const std::string& datasetName)
{
    auto warnings               = pybind11::module::import("warnings");
    auto builtins               = pybind11::module::import("builtins");
    std::string guid            = "";

    const QString parentGUID          = QString::fromStdString(parentPointDatasetGuid);
    const size_t numClusters          = clusterIndices.size();

    auto checkDatasetExists = [](const QString& guid ) -> bool {
        QVector<Dataset<DatasetImpl>> datasets = mv::data().getAllDatasets(std::vector<DataType>({ PointType }));
        return std::any_of(datasets.begin(), datasets.end(), [&guid](const Dataset<DatasetImpl>& dataset) { return dataset.getDatasetId() == guid; });
        };

    if (!checkDatasetExists(parentGUID))
    {
        qDebug() << "add_new_cluster_data: parent dataset not found. (ensure that is a PointData set)";
        return guid;
    }

    auto parentDataset          = mv::data().getDataset(parentGUID);
    Dataset<Clusters> clusters  = mv::data().createDataset("Cluster", QString::fromStdString(datasetName), parentDataset);
    guid                        = clusters.getDatasetId().toStdString();

    const bool hasNames         = clusterNames.size() == numClusters;
    const bool hasColors        = clusterColors.size() == numClusters;

    for (size_t i = 0; i < numClusters; ++i) {
        Cluster cluster;

        std::vector<std::uint32_t> clusterIndices_v = py_array_to_vector<std::uint32_t>(clusterIndices[i]);
        cluster.setIndices(clusterIndices_v);

        QString clusterName = QString::fromStdString(std::to_string(i));

        if (hasNames)
            clusterName = QString::fromStdString(clusterNames[i]);

        cluster.setName(clusterName);

        if (hasColors) {
            std::vector<float> clusterColors_v = py_array_to_vector<float>(clusterColors[i]);
            QColor clusterColor = QColor::fromRgbF(clusterColors_v[0], clusterColors_v[1], clusterColors_v[2], 1);
            cluster.setColor(clusterColor);
        }

        clusters->addCluster(cluster);
    }

    if(!hasColors)
        Cluster::colorizeClusters(clusters->getClusters());

    events().notifyDatasetDataChanged(clusters);

    return guid;
}

// return Hierarchy Item and Data set guid in tuple
static py::list get_top_level_guids()
{
    py::list results;
    for (const auto topLevelDataHierarchyItem : mv::dataHierarchy().getTopLevelItems()) {
        auto dhiGuid    = topLevelDataHierarchyItem->getId().toStdString();
        auto dsGuid     = topLevelDataHierarchyItem->getDataset()->getId().toStdString();
        results.append(py::make_tuple(dhiGuid, dsGuid));
    }
    return results;
}

static std::uint64_t get_item_numdimensions(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    return item->getDataset<Points>()->getNumDimensions();
}

static std::uint64_t get_item_numpoints(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    return item->getDataset<Points>()->getNumPoints();
}

static std::string get_item_name(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto name = item->getDataset()->getGuiName();
    return name.toStdString();
}

static std::string get_item_type(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto type = item->getDataset()->getDataType();
    return type.getTypeString().toStdString();
}

static std::string get_item_rawname(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto name = item->getDataset()->getRawDataName();
    return name.toStdString();
}

static std::uint64_t get_item_rawsize(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    return item->getDataset()->getRawDataSize();
}

static std::vector<std::string> get_item_properties(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto dataset = item->getDataset<Points>();
    QStringList propertyNamesQt = dataset->propertyNames();
    std::vector<std::string> propertyNames;
    propertyNames.resize(propertyNamesQt.size());

    for (const auto& propertyName : propertyNamesQt) {
        propertyNames.push_back(propertyName.toStdString());
    }

    return propertyNames;
}

static py::object get_item_property(const std::string& datasetGuid, const std::string& propertyName)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto dataset = item->getDataset<Points>();

    auto propertyNameQt = QString(propertyName.c_str());

    const auto property = dataset->getProperty(propertyNameQt);

    if (property.isNull()) {
        qDebug() << "Dataset " << dataset->getGuiName() << " does not have property " << propertyNameQt;
        return py::none();
    }

    auto variant_to_pyobject = [](const QVariant& var) -> py::object {
        switch (var.typeId()) {
        case QMetaType::Int:
            return py::int_(var.toInt());
        case QMetaType::UInt:
            return py::int_(var.toUInt());
        case QMetaType::LongLong:
            return py::int_(var.toLongLong());
        case QMetaType::ULongLong:
            return py::int_(var.toULongLong());
        case QMetaType::Float:
            return py::float_(var.toFloat());
        case QMetaType::Double:
            return py::float_(var.toDouble());
        case QMetaType::Bool:
            return py::bool_(var.toBool());
        case QMetaType::Char:
            return py::str(var.toChar().decomposition().toStdString());
        case QMetaType::QString:
            return py::str(var.toString().toStdString());
        default:
            return py::none();
        }
        };

    py::object res;
    py::list pyList;

    if (property.typeId() == QMetaType::QVariantList) {
        const QVariantList list = property.toList();
        for (const QVariant& item : list) {
            pyList.append(variant_to_pyobject(item));
        }
        res = pyList;
    }
    else {
        res = variant_to_pyobject(property);
    }

    return res;
}

// Get all children
// Returns a list of tuples
// Each tuple contains:
// (Data Hierarchy Item guid, Dataset guid)
static py::list get_item_children(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    qDebug() << "Children for id: " << QString(datasetGuid.c_str());

    auto children = item->getChildren();
    py::list guidTupleList;
    for (auto& child : children) {
        // The child might be a Dataset rather than an Item
        // however datasets hae 0 children and items have > 0 

        auto childId        = child->getId().toStdString();
        auto childDatasetId = child->getDataset()->getId().toStdString();
        auto guidTuple      = py::make_tuple(childId, childDatasetId);
        guidTupleList.append(guidTuple);
    }
    return guidTupleList;
}

static mvstudio_core::DataItemType get_data_type(const std::string& datasetGuid) {
    QString guid = QString(datasetGuid.c_str());

    qDebug() << "Get type for id: " << guid;
    auto dataset    = mv::data().getDataset(guid);
    auto datatype   = dataset->getDataType();

    mvstudio_core::DataItemType res = mvstudio_core::NOT_IMPLEMENTED;

    if (datatype == ImageType)
        res = mvstudio_core::Image;
    else if (datatype == ClusterType)
        res = mvstudio_core::Cluster;
    else if (datatype == PointType)
        res = mvstudio_core::Points;
    else 
        qWarning() << "Datatype is not handled, it must be one of [Points, Image, Cluster].";

    return res;
}

//bool add_mvimage_stack(const py::list& data, std::string dataSetName ) 
//{
//    return true;
//}

// Return the GUID of the image dataset
static std::string find_image_dataset(const std::string& datasetGuid)
{
    std::string guid = "";

    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    for (auto childHierarchyItem : item->getChildren()) {
        if (childHierarchyItem->getDataType() == ImageType) {
            guid = childHierarchyItem->getDataset()->getId().toStdString();
            break;
        }
    }

    if (item->hasParent())
        guid = find_image_dataset(item->getParent()->getDataset()->getId().toStdString());
 
    return guid;
}

static py::tuple get_image_dimensions(const std::string& datasetGuid)
{
    auto item       = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto images     = item->getDataset<Images>();
    auto numImages  = images->getNumberOfImages();
    auto size       = images->getImageSize();

    return py::make_tuple(size.width(), size.height(), numImages);
}

static py::tuple get_cluster(const std::string& datasetGuid)
{
    auto clusterData        = mv::data().getDataset<Clusters>(QString(datasetGuid.c_str()));
    const auto& clusters    = clusterData->getClusters();
    const auto clusterNames = clusterData->getClusterNames();

    py::list names;
    for (const auto& name : clusterNames)
        names.append(name.toStdString());

    py::list ids;
    py::list colors;
    py::list indexes_list;
    for (const auto& cluster: clusters) {
        ids.append(cluster.getId().toStdString());
        auto color = cluster.getColor();
        float r, g, b, a;
        color.getRgbF(&r, &g, &b, &a);
        colors.append(py::make_tuple(r, g, b, a));
        const auto& indices = cluster.getIndices();
        size_t size = indices.size();

        auto indexArray = py::array_t<unsigned int>(size);
        py::buffer_info array_info = indexArray.request();
        unsigned int* output = static_cast<unsigned int*>(array_info.ptr);
        for (size_t i = 0; i < size; i++) {
            output[i] = indices[i];
        }
        indexes_list.append(indexArray);
    }
    return py::make_tuple(indexes_list, names, colors, ids);
}

static bool set_linked_data(const std::string& sourceDataGuid, const std::string& targetDataGuid, const std::vector<std::vector<int64_t>>& selectionFromAToB)
{
    auto sourceData = mv::data().getDataset<Points>(QString::fromStdString(sourceDataGuid));
    auto targetData = mv::data().getDataset<Points>(QString::fromStdString(targetDataGuid));

    if (!sourceData.isValid()) {
        qWarning() << "set_linked_data:: sourceData cannot be found";
        return false;
    }
    if (!targetData.isValid()) {
        qWarning() << "set_linked_data:: targetData cannot be found";
        return false;
    }

    const std::uint32_t numSource = sourceData->getNumPoints();
    const std::uint32_t numTarget = targetData->getNumPoints();

    if (static_cast<size_t>(numSource) != selectionFromAToB.size()) {
        qWarning() << "set_linked_data:: selectionFromAToB must be of same size as sourceData";
        return false;
    }

    mv::SelectionMap selectionMap;
    std::map<std::uint32_t, std::vector<std::uint32_t>>& map = selectionMap.getMap();

    auto convertToVui32 = [](const std::vector<int64_t>& vi64) -> std::vector<std::uint32_t> {
        std::vector<std::uint32_t> vui32;
        vui32.reserve(vi64.size()); // avoid reallocations

        std::transform(vi64.begin(), vi64.end(), std::back_inserter(vui32),
            [](int64_t val) { return static_cast<std::uint32_t>(val); });

        std::sort(vui32.begin(), vui32.end());

        return vui32;
        };

    int64_t maxIndB = 0;
    for (std::uint32_t idA = 0; idA < numSource; idA++) {
        const auto [it, success] = map.insert({ idA, convertToVui32(selectionFromAToB[idA]) });

        if (it->second.back() > maxIndB)
            maxIndB = it->second.back();
    }

    if(maxIndB > numTarget) {
        qWarning() << "set_linked_data:: max mapped index is larger than the number if points in the target data set";
        return false;
    }

    sourceData->removeLinkedDataset(targetData);
    sourceData->addLinkedData(targetData, std::move(selectionMap));

    return true;
}

py::module get_MVData_module()
{
    py::module MVData_module = py::module_::create_extension_module("mvstudio_core", nullptr, new py::module_::module_def);
    
    py::enum_<mvstudio_core::DataItemType>(MVData_module, "DataItemType")
        .value("Image", mvstudio_core::DataItemType::Image)
        .value("Points", mvstudio_core::DataItemType::Points)
        .value("Cluster", mvstudio_core::DataItemType::Cluster);

    MVData_module.def("get_info", get_info);
    MVData_module.def("get_top_level_item_names", get_top_level_item_names);
    MVData_module.def("get_top_level_guids", get_top_level_guids);
    MVData_module.def("get_data_for_item", get_data_for_item, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_selection_for_item", get_selection_for_item, py::arg("datasetGuid") = std::string());
    MVData_module.def("set_selection_for_item", set_selection_for_item, py::arg("datasetGuid") = std::string(), py::arg("selectionIDs") = py::array_t<uint32_t>());
    MVData_module.def("get_image_item", get_mv_image, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_name", get_item_name, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_rawsize", get_item_rawsize, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_type", get_item_type, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_rawname", get_item_rawname, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_numdimensions", get_item_numdimensions, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_numpoints", get_item_numpoints, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_children", get_item_children, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_properties", get_item_properties, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_item_property", get_item_property, py::arg("datasetGuid") = std::string(), py::arg("propertyName") = std::string());
    MVData_module.def("get_data_type", get_data_type, py::arg("datasetGuid") = std::string());
    MVData_module.def("find_image_dataset", find_image_dataset, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_image_dimensions", get_image_dimensions, py::arg("datasetGuid") = std::string());
    MVData_module.def("get_cluster", get_cluster, py::arg("datasetGuid") = std::string());
    MVData_module.def("set_linked_data", 
        set_linked_data, 
        py::arg("sourceDataGuid") = std::string(), 
        py::arg("targetDataGuid") = std::string(), 
        py::arg("selectionFromAToB") = std::vector<std::vector<int64_t>>()
    );
    MVData_module.def(
        "add_new_points",
        add_new_point_data,
        py::arg("data") = py::array(),
        py::arg("dataSetName") = std::string(),
        py::arg("dataSetParentID") = std::string(),
        py::arg("dimensionNames") = std::vector<std::string>()
    );
    MVData_module.def(
        "add_new_image",
        add_new_image_data,
        py::arg("data") = py::array(),
        py::arg("dataSetName") = std::string(),
        py::arg("dimensionNames") = std::vector<std::string>()
    );
    // The cluster tuple contains the following lists
    // names
    // indexes (clusters) 
    // colors 
    // ids (ignored)
    MVData_module.def(
        "add_new_clusters",
        add_new_cluster_data,
        py::arg("parentPointDatasetGuid") = std::string(),
        py::arg("clusterIndices") = std::vector<py::array>(),
        py::arg("clusterNames") = std::vector<std::string>(),
        py::arg("clusterColors") = std::vector<py::array>(),
        py::arg("datasetName") = std::string()
        );
    return MVData_module;
}