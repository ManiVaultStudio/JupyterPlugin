#include "MVData.h"

#include <Application.h>
#include <ClusterData/Cluster.h>
#include <ClusterData/ClusterData.h>
#include <Dataset.h>
#include <ImageData/ImageData.h>
#include <ImageData/Images.h>
#include <PointData/PointData.h>

#include <pybind11/buffer_info.h>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include <pybind11/pytypes.h>

#include <QColor>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QDebug>

#include <cstdint>
#include <cstring>
#include <numeric>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace py = pybind11;

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
py::array populate_pyarray(
    mv::Dataset<Points> &inputPoints, 
    unsigned int numPoints, 
    unsigned int numDimensions)
{
    std::vector<unsigned int> dim_indices(numDimensions);
    std::iota(dim_indices.begin(), dim_indices.end(), 0);

    std::vector<T> data;
    auto size = numPoints * numDimensions;
    data.resize(size);

    inputPoints->populateDataForDimensions<std::vector<T>, std::vector<unsigned int>>(data, dim_indices);

    auto result = py::array_t<T>({numPoints, numDimensions});
    py::buffer_info result_info = result.request();
    T* output = static_cast<T*>(result_info.ptr);
    for (auto i = 0; i < size; i++) {
        output[i] = data[i];
    }
    return result;
}

template<typename T>
PointData::ElementTypeSpecifier getTypeSpecifier() {
    if (std::is_same <T, float>::value) {
        return PointData::ElementTypeSpecifier::float32;
    } else if (std::is_same <T, std::int16_t>::value) {
        return PointData::ElementTypeSpecifier::int16;
    } else if (std::is_same <T, std::uint16_t>::value) {
        return PointData::ElementTypeSpecifier::uint16;
    } else if (std::is_same <T, std::int8_t>::value) {
        return PointData::ElementTypeSpecifier::int8;
    } else if (std::is_same <T, std::uint8_t>::value) {
        return PointData::ElementTypeSpecifier::uint8;
    }
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
    auto size                   = numPoints * numDimensions;

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

    qInfo() << "PointData::ElementTypeSpecifier is " << static_cast<int>(dataSpec);
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

// numpy default for 3d (2D+RGB) images in C format is BIP 
// so we don't do anything 
// Assumes input data is arranged contiguously in memory
// Each 2D band is complete before the next begins
// The data_out is is Band Interleaved by Pixel
template<typename T, typename U>
void orient_multiband_imagedata_as_bip(const U* data_in, std::vector<size_t> shape, std::vector<T>& data_out, bool flip)
{
    if (flip) {
        // C order with flip 

        auto row_size = shape[1] * shape[2];
        auto num_rows = shape[0];
        auto total = row_size * num_rows;
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
        auto total = shape[0] * shape[1] * shape[2];
        // C order 
        for (auto i = 0; i < total; ++i) {
            data_out[i] = static_cast<T>(data_in[i]);
        }
    }
}

// when conversion is needed
template<typename T, typename U>
void conv_points_from_numpy_array(const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip = false)
{
    auto band_size = shape[0] * shape[1];
    auto num_bands = shape.size() == 3 ? shape[2] : shape[1];
    auto warnings = pybind11::module::import("warnings");
    auto builtins = pybind11::module::import("builtins");
    warnings.attr("warn")(
        "This numpy dtype was converted to float to match the ManiVault data model.",
        builtins.attr("UserWarning"));
    auto data_in_U = static_cast<const U *>(data_in);
    auto data_out = std::vector<T>();
    data_out.resize(band_size * num_bands);
    if (num_bands == shape[1] && !flip) {
        for (auto i = 0; i < band_size; ++i) {
            data_out[i] = static_cast<const T>(data_in_U[i]);
        }
    }
    else {
        orient_multiband_imagedata_as_bip<T,U>(data_in_U, shape, data_out, flip);
    }
    points->setData(std::move(data_out), num_bands);
}

// when types are the same image
template<class T>
void set_img_points_from_numpy_array(const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip=false)
{
    auto band_size = shape[0] * shape[1];
    auto num_bands = shape.size() == 3 ? shape[2] : shape[1];
    auto data_out = std::vector<T>();
    data_out.resize(band_size * num_bands);
    if (num_bands == shape[1] && !flip)
        std::memcpy(data_out.data(), static_cast<const T*>(data_in), band_size * sizeof(T));
    else
        orient_multiband_imagedata_as_bip<T,T>(static_cast<const T*>(data_in), shape, data_out, flip);
    
    points->setData(std::move(data_out), num_bands);
}


// when types are different, setting points
template<typename T, typename U>
void set_points_from_numpy_array_impl(std::false_type, const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip)
{
    if (shape.size() != 2) {
        return;
    }
    auto warnings = pybind11::module::import("warnings");
    auto builtins = pybind11::module::import("builtins");
    warnings.attr("warn")(
        "This numpy dtype was converted to float to match the ManiVault data model.",
        builtins.attr("UserWarning"));
    size_t num_points = shape[0] * shape[1];
    const U* data_in_U = static_cast<const U*>(data_in);
    std::vector<T> data_out = {};
    data_out.resize(num_points);

    for (size_t i = 0; i < num_points; ++i) {
        data_out[i] = static_cast<const T>(data_in_U[i]);
    }

    points->setData(std::move(data_out), shape[1]);
}

// when types are the same, setting points
template<typename T, typename U>
void set_points_from_numpy_array_impl(std::true_type, const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip)
{
    if (shape.size() != 2) {
        return;
    }
    auto num_points = shape[0] * shape[1];
    std::vector<T> data_out = {};
    data_out.resize(num_points);
    std::memcpy(data_out.data(), static_cast<const T*>(data_in), num_points * sizeof(T));
    points->setData(std::move(data_out), shape[1]);
}

template<typename T, typename U>
void set_points_from_numpy_array(const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip = false)
{
    set_points_from_numpy_array_impl<T,U>(std::is_same<T,U>(), data_in, shape, points, flip);
}

static py::buffer_info createBuffer(const py::array& data)
{
    py::buffer_info buf_info;

    if (!data.base().is_none()) {
        py::print("The numpy array is a view. Creating a local copy to handle striding correctly...");

        pybind11::array::ShapeContainer shape(data.shape(), data.shape() + data.ndim());
        pybind11::array::ShapeContainer strides(data.strides(), data.strides() + data.ndim());

        py::array copied_arr = py::array(data.dtype(), shape, strides, data.data());

        buf_info = copied_arr.request();
    }
    else {
        buf_info = data.request();
    }

    return buf_info;
}

/**
 * Add new point data in the root of the hierarchy.
 * If successful returns a guid for the new point data
 * If unsuccessful return a empty string
 */
static std::string add_new_mvdata(const py::array& data, std::string dataSetName)
{
    std::string guid            = "";
    const auto dtype            = data.dtype();
    py::buffer_info buf_info    = createBuffer(data);
    void* ptr                   = buf_info.ptr;
    auto shape                  = std::vector<size_t>(buf_info.shape.begin(), buf_info.shape.end());

    void (*point_setter)(const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip) = nullptr;

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
        qDebug() << "add_mvimage: type not supported (e.g. uint64_t or int64_t): " << QString(dtype.kind());
        return guid;
    }

    if (point_setter != nullptr)
    {
        mv::Dataset<Points> points = mv::data().createDataset<Points>("Points", dataSetName.c_str(), nullptr);
        point_setter(ptr, shape, points, false);
        events().notifyDatasetDataChanged(points);

        guid = points.getDatasetId().toStdString();

        qDebug() << "add_new_mvdata: " << QString(guid.c_str());
    }
    else
    {
        auto warnings = pybind11::module::import("warnings");
        auto builtins = pybind11::module::import("builtins");
        warnings.attr("warn")(
            "This numpy dtype could not be converted to a ManiVaultStudio supported type.",
            builtins.attr("UserWarning"));
    }

    return py::str(guid);
}

// The MV data model is Band Interlaced by Pixel.
// This means that an image stack (which might be hyperspectral or simply RGB/RGBA)
// is counted as having size(stack) bands called dimensions in the MV Image.
// This function is meant to deal only with the 
// single image case however multiple RGB or RGBA bands may be present
// as given by the number of components
static std::string add_mvimage(const py::array& data, std::string dataSetName)
{
    std::string guid            = "";
    const auto dtype            = data.dtype();
    py::buffer_info buf_info    = createBuffer(data);
    void* ptr                   = buf_info.ptr;
    auto shape                  = std::vector<size_t>(buf_info.shape.begin(), buf_info.shape.end());

    // Grey-scale image:
    if (shape.size() == 2)
        shape.push_back(1);

    int num_bands = shape[2];

    if (shape.size() != 3)
    {
        qDebug() << "add_mvimage: numpy image must be an image, i.e. of shape (x, y, dims). Instead got: " << shape;
        return guid;
    }
    
    void (*point_setter)(const void* data_in, std::vector<size_t> shape, mv::Dataset<Points> points, bool flip) = nullptr;

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
        qDebug() << "add_mvimage: type not supported (e.g. uint64_t or int64_t): " << QString(dtype.kind());
        return guid;
    }

    if (point_setter != nullptr)
    {
        mv::Dataset<Points> points = mv::data().createDataset<Points>("Points", dataSetName.c_str(), nullptr);
        point_setter(ptr, shape, points, true);
        events().notifyDatasetDataChanged(points);

        auto imageDataset = mv::data().createDataset<Images>("Images", "numpy image", Dataset<DatasetImpl>(*points));

        int width = shape[1];
        int height = shape[0];

        imageDataset->setText(QString("Images (%2x%3)").arg(QString::number(width), QString::number(height)));
        imageDataset->setType(ImageData::Type::Stack);
        imageDataset->setNumberOfImages(1);
        imageDataset->setImageSize(QSize(width, height));
        imageDataset->setNumberOfComponentsPerPixel(num_bands);

        events().notifyDatasetDataChanged(imageDataset);

        guid = points.getDatasetId().toStdString();

        qDebug() << "add_mvimage: " << QString(guid.c_str());
    }
    else
    {
        auto warnings = pybind11::module::import("warnings");
        auto builtins = pybind11::module::import("builtins");
        warnings.attr("warn")(
            "This numpy dtype could not be converted to a ManiVaultStudio supported type.",
            builtins.attr("UserWarning"));
    }

    return py::str(guid);
}

// All images are float in ManiVault 1.x.
// An Image parent may be a Points or Cluster item
static py::array get_mv_image(const std::string& imageGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(imageGuid.c_str()));
    auto images = item->getDataset<Images>();
    auto numImages = images->getNumberOfImages();
    auto numPixels = images->getNumberOfPixels();
    auto numComponents = images->getNumberOfComponentsPerPixel();
    auto imqSize = images->getImageSize();
    unsigned int width = imqSize.width();
    unsigned int height = imqSize.height();
    auto pixcmpSize = numPixels * numComponents;

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

    auto result = py::array_t<float>(shape);
    py::buffer_info result_info = result.request();
    float* output = static_cast<float*>(result_info.ptr);
    for (auto imIdx = 0; imIdx < numImages; ++imIdx) {
        QVector<float> scalarData;
        scalarData.resize(static_cast<size_t>(numPixels) * numComponents);
        QPair<float, float> scalarDataRange;
        images->getImageScalarData(imIdx, scalarData, scalarDataRange);
        for (auto pixcmpIdx = 0; pixcmpIdx < pixcmpSize; ++pixcmpIdx) {
            output[(imIdx * pixcmpSize) + pixcmpIdx] = scalarData[pixcmpIdx];
        }
    }

    return result;
}

static std::string add_cluster(const py::tuple& tupleOfClusterLists, std::string dataSetName, const std::string& parentPointDatasetGuid)
{
    auto warnings = pybind11::module::import("warnings");
    auto builtins = pybind11::module::import("builtins");
    // validate the incoming tuple
    if (tupleOfClusterLists.size() != 4) {
        warnings.attr("warn")(
            "Names, cluster indexes, ids and colors must be present.",
            builtins.attr("UserWarning"));
        return "";
    }

    auto names_list = tupleOfClusterLists[0].cast<py::list>();
    auto clusters_list = tupleOfClusterLists[1].cast<py::list>();
    auto colors_list = tupleOfClusterLists[2].cast<py::list>();
    auto ids_list = tupleOfClusterLists[3].cast<py::list>();
    if (!(names_list.size() + clusters_list.size() - ids_list.size() - colors_list.size() == 0)) {
        warnings.attr("warn")(
            "The lengths of the lists names, cluster indexes, ids and colors must be present.",
            builtins.attr("UserWarning"));
    }

    auto parent = mv::dataHierarchy().getItem(QString(parentPointDatasetGuid.c_str()));
    Dataset<Clusters> clusters = mv::data().createDataset("Cluster", dataSetName.c_str(), parent->getDataset<Points>());

    for (size_t i = 0; i < names_list.size(); ++i) {
        auto indexes        = clusters_list[i].cast<py::array>();
        std::string name    = names_list[i].cast<py::str>();
        auto colorTup       = colors_list[i].cast<py::tuple>();
        std::string id      = ids_list[i].cast<py::str>();

        Cluster cluster;
        cluster.setName(name.c_str());

        auto clusterIndices = indexes.cast<std::vector<std::uint32_t>>();
        cluster.setIndices(clusterIndices);

        auto alpha = (colorTup.size() == 4) ? colorTup[3].cast<float>() : 1.0;
        auto clusterColor = QColor::fromRgbF(colorTup[0].cast<float>(), colorTup[1].cast<float>(), colorTup[2].cast<float>(), alpha);
        cluster.setColor(clusterColor);

        clusters->addCluster(cluster);
    }

    events().notifyDatasetDataChanged(clusters);

    return py::str(clusters.getDatasetId().toStdString());
}

// return Hierarchy Item and Data set guid in tuple
static py::list get_top_level_guids()
{
    py::list results;
    for (const auto topLevelDataHierarchyItem : mv::dataHierarchy().getTopLevelItems()) {
        auto dhiGuid = topLevelDataHierarchyItem->getId().toStdString();
        auto dsGuid = topLevelDataHierarchyItem->getDataset()->getId().toStdString();
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
    for (auto child : children) {
        // The child might be a Dataset rather than an Item
        // however datasets hae 0 children and items have > 0 

        auto childId = child->getId().toStdString();
        auto childDatasetId = child->getDataset()->getId().toStdString();
        auto guidTuple = py::make_tuple(childId, childDatasetId);
        guidTupleList.append(guidTuple);
    }
    return guidTupleList;
}

static mvstudio_core::DataItemType get_data_type(const std::string& datasetGuid) {
    qDebug() << "Get type for id: " << QString(datasetGuid.c_str());
    auto dataset = mv::data().getDataset(QString(datasetGuid.c_str()));
    auto datatype = dataset->getDataType();
    if (datatype == ImageType) {
        return mvstudio_core::Image;
    }
    if (datatype == ClusterType) {
        return mvstudio_core::Cluster;
    }
    if (datatype == PointType) {
        return mvstudio_core::Points;
    }
}

//bool add_mvimage_stack(const py::list& data, std::string dataSetName ) 
//{
//    return true;
//}

// Return the GUID of the image dataset
static std::string find_image_dataset(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    for (auto childHierarchyItem : item->getChildren()) {
        if (childHierarchyItem->getDataType() == ImageType) {
            return childHierarchyItem->getDataset()->getId().toStdString();
        }
    }

    if (item->hasParent()) {
        return find_image_dataset(item->getParent()->getDataset()->getId().toStdString());
    }
    return "";
}

static py::tuple get_image_dimensions(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto images = item->getDataset<Images>();
    auto numImages = images->getNumberOfImages();
    auto size = images->getImageSize();

    return py::make_tuple(size.width(), size.height(), numImages);
}

static py::tuple get_cluster(const std::string& datasetGuid)
{
    auto clusterData = mv::data().getDataset<Clusters>(QString(datasetGuid.c_str()));
    const auto& clusters = clusterData->getClusters();
    const auto clusterNames = clusterData->getClusterNames();
    py::list names;
    py::list ids;
    for (const auto& name : clusterNames) {
        names.append(name.toStdString());
    }
    py::list colors;
    py::list indexes_list;
    for (const auto& cluster: clusters) {
        ids.append(cluster.getId().toStdString());
        auto color = cluster.getColor();
        float r, g, b, a;
        color.getRgbF(&r, &g, &b, &a);
        colors.append(py::make_tuple(r, g, b, a));
        const auto& indices = cluster.getIndices();
        auto size = indices.size();

        auto indexArray = py::array_t<unsigned int>(size);
        py::buffer_info array_info = indexArray.request();
        unsigned int* output = static_cast<unsigned int*>(array_info.ptr);
        for (auto i = 0; i < size; i++) {
            output[i] = indices[i];
        }
        indexes_list.append(indexArray);
    }
    return py::make_tuple(indexes_list, names, colors, ids);
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
    MVData_module.def("get_data_for_item", get_data_for_item, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_image_item", get_mv_image, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_name", get_item_name, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_rawsize", get_item_rawsize, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_type", get_item_type, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_rawname", get_item_rawname, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_numdimensions", get_item_numdimensions, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_numpoints", get_item_numpoints, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_item_children", get_item_children, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_data_type", get_data_type, py::arg("datasetGuid") = py::str());
    MVData_module.def("find_image_dataset", find_image_dataset, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_image_dimensions", get_image_dimensions, py::arg("datasetGuid") = py::str());
    MVData_module.def("get_cluster", get_cluster, py::arg("datasetGuid") = py::str());
    MVData_module.def(
        "add_new_data",
        add_new_mvdata,
        py::arg("data") = py::array(),
        py::arg("dataSetName") = py::str()
    );
    MVData_module.def(
        "add_new_image",
        add_mvimage,
        py::arg("data") = py::array(),
        py::arg("dataSetName") = py::str()
    );
    // The cluster tuple contains the following lists
    // names
    // indexes (clusters) 
    // colors 
    // ids (ignored)
    MVData_module.def(
        "add_new_cluster",
        add_cluster,
        py::arg("tupleOfClusterLists") = py::tuple(),
        py::arg("clusterName") = py::str(),
        py::arg("parentPointDatasetGuid") = py::str()
    );
    return MVData_module;
}