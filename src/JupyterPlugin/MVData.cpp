#include "MVData.h"

#include "BindingUtils.h"

#include <Application.h>
#include <ClusterData/Cluster.h>
#include <ClusterData/ClusterData.h>
#include <Dataset.h>
#include <ImageData/ImageData.h>
#include <ImageData/Images.h>
#include <LinkedData.h>
#include <PointData/PointData.h>
#include <Set.h>

#undef slots
#include <pybind11/buffer_info.h>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>       // a.o. for vector conversions
#define slots Q_SLOTS

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
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace py = pybind11;

// =============================================================================
// ManiVault core and data interaction 
// =============================================================================

py::object get_top_level_item_names()
{
    py::list result;
    for (const auto topLevelDataHierarchyItem : Application::core()->getDataHierarchyManager().getTopLevelItems()) {
        result.append(py::str(topLevelDataHierarchyItem->getDataset()->getGuiName().toUtf8().constData()));
    }
    return result;

}

// only works on top level items as test
// Get the point data associated with the names item and return it as a numpy array to python
py::array get_data_for_item(const std::string& datasetGuid)
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
py::array get_selection_for_item(const std::string& datasetGuid)
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
void set_selection_for_item(const std::string& datasetGuid, const std::vector<uint32_t>& selectionIDs)
{
    DataHierarchyItem* item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));

    if (!item)
        return;

    auto data = item->getDataset();

    // Send selection to the core
    data->setSelectionIndices(selectionIDs);
    mv::events().notifyDatasetDataSelectionChanged(data);
}

/**
 * Add new point data in the root of the hierarchy or below dataSetParentID.
 * If successful returns a guid for the new point data
 * If unsuccessful return a empty string
 */
std::string add_new_point_data(const py::array& data, const std::string& dataSetName, const std::string& dataSetParentID, const std::vector<std::string>& dimensionNames)
{
    auto generateNewPoints = [dataSetName, dataSetParentID]() -> mv::Dataset<Points> {
        const Dataset<DatasetImpl> parentData = 
            /* if */   dataSetParentID.empty() ?
            /* then */ Dataset<DatasetImpl>() :
            /* else */ mv::data().getDataset(QString::fromStdString(dataSetParentID));

        return mv::data().createDataset<Points>("Points", dataSetName.c_str(), parentData);
        };


    return add_point_data(data, dimensionNames, generateNewPoints);
}

/**
 * Add new derived point data.
 * If successful returns a guid for the new point data
 * If unsuccessful return a empty string
 */
std::string add_derived_point_data(const py::array& data, const std::string& dataSetName, const std::string& dataSetSourceID, const std::vector<std::string>& dimensionNames)
{
    if (!mv::data().getDataset(QString::fromStdString(dataSetSourceID)).isValid()) {
        qDebug() << "add_new_derived_point_data: source data is not valid";
        return "";
    }

    auto generateDerivedPoints = [dataSetName, dataSetSourceID]() -> mv::Dataset<Points> {
        const Dataset<DatasetImpl> sourceData = mv::data().getDataset(QString::fromStdString(dataSetSourceID));
        return mv::Dataset<Points>(mv::data().createDerivedDataset(dataSetName.c_str(), sourceData, sourceData));
        };

    return add_point_data(data, dimensionNames, generateDerivedPoints);
}

// The MV data model is Band Interlaced by Pixel.
// This means that an image stack (which might be hyperspectral or simply RGB/RGBA)
// is counted as having size(stack) bands called dimensions in the MV Image.
// This function is meant to deal only with the 
// single image case however multiple RGB or RGBA bands may be present
// as given by the number of components
std::string add_new_image_data(const py::array& data, const std::string& dataSetName, const std::vector<std::string>& dimensionNames)
{
    std::string guid                = "";
    const py::dtype dtype           = data.dtype();
    const py::buffer_info buf_info  = createBuffer(data);
    const void* py_data_storage_ptr = buf_info.ptr;

    if (!(data.flags() & py::array::c_style)) {
        qDebug() << "add_point_data: Numpy array must by c-contiguous, use np.ascontiguousarray(data)";
        return guid;
    }

    if (!py_data_storage_ptr) {
        qDebug() << "add_new_point_data: python data transfer failed";
        return guid;
    }

    if (buf_info.shape.size() != 2 && buf_info.shape.size() != 3) {
        qDebug() << "add_new_image_data: numpy image must be an image, i.e. of shape (x, y, dims). Instead got: " << buf_info.shape;
        return guid;
    }

    const size_t num_bands = 
        /* if: Grey-scale image */  buf_info.shape.size() == 2 ?
        /* then: one dim */         1 :
        /* else: multiple dims  */  buf_info.shape[2];

    const size_t height = static_cast<size_t>(buf_info.shape[0]);
    const size_t width = static_cast<size_t>(buf_info.shape[1]);

    const std::array<size_t, 3> shape   = { height, width, num_bands };

    void (*point_setter)(const void* data_in, const std::array<size_t, 3>& shape, mv::Dataset<Points>& points, bool flip) = nullptr;

    // PointData is limited in its type support - hopefully the commented types wil be added soon
    if (dtype.is(pybind11::dtype::of<std::uint8_t>()))
        point_setter = conv_img_points_from_numpy_array<float, std::uint8_t>;
    else if (dtype.is(pybind11::dtype::of<std::int8_t>()))
        point_setter = conv_img_points_from_numpy_array<float, std::int8_t>;
    else if (dtype.is(pybind11::dtype::of<std::uint16_t>()))
        point_setter = conv_img_points_from_numpy_array<float, std::uint16_t>;
    else if (dtype.is(pybind11::dtype::of<std::int16_t>()))
        point_setter = conv_img_points_from_numpy_array<float, std::int16_t>;
    else if (dtype.is(pybind11::dtype::of<std::uint32_t>()))
        point_setter = conv_img_points_from_numpy_array<float, std::uint32_t>;
    else if (dtype.is(pybind11::dtype::of<std::int32_t>()))
        point_setter = conv_img_points_from_numpy_array<float, std::int32_t>;
    //Unsupported <std::uint64_t> :
    //Unsupported <std::int64_t> :
    else if (dtype.is(pybind11::dtype::of<float>()))
        point_setter = set_img_points_from_numpy_array<float>;
    else if (dtype.is(pybind11::dtype::of<double>()))
        point_setter = conv_img_points_from_numpy_array<float, double>;
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
py::array get_mv_image(const std::string& imageGuid)
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

std::string add_new_cluster_data(const std::string& parentPointDatasetGuid, const std::vector<py::array>& clusterIndices, const std::vector<std::string>& clusterNames, const std::vector<py::array>& clusterColors, const std::string& datasetName)
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

        cluster.setIndices(py_array_to_vector<std::uint32_t>(clusterIndices[i]));

        if (hasNames)
            cluster.setName(QString::fromStdString(clusterNames[i]));
        else
            cluster.setName(QString::fromStdString(std::to_string(i)));

        if (hasColors) {
            std::vector<float> clusterColorsVals = py_array_to_vector<float>(clusterColors[i]);
            QColor clusterColor = QColor::fromRgbF(clusterColorsVals[0], clusterColorsVals[1], clusterColorsVals[2], 1);
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
py::list get_top_level_guids()
{
    py::list results;
    for (const auto topLevelDataHierarchyItem : mv::dataHierarchy().getTopLevelItems()) {
        auto dhiGuid    = topLevelDataHierarchyItem->getId().toStdString();
        auto dsGuid     = topLevelDataHierarchyItem->getDataset()->getId().toStdString();
        results.append(py::make_tuple(dhiGuid, dsGuid));
    }
    return results;
}

std::uint64_t get_item_numdimensions(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    return item->getDataset<Points>()->getNumDimensions();
}

std::uint64_t get_item_numpoints(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    return item->getDataset<Points>()->getNumPoints();
}

std::string get_item_name(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto name = item->getDataset()->getGuiName();
    return name.toStdString();
}

std::string get_item_type(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto type = item->getDataset()->getDataType();
    return type.getTypeString().toStdString();
}

std::string get_item_rawname(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto name = item->getDataset()->getRawDataName();
    return name.toStdString();
}

std::uint64_t get_item_rawsize(const std::string& datasetGuid)
{
    auto item = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    return item->getDataset()->getRawDataSize();
}

std::vector<std::string> get_item_properties(const std::string& datasetGuid)
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

py::object get_item_property(const std::string& datasetGuid, const std::string& propertyName)
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
py::list get_item_children(const std::string& datasetGuid)
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

mvstudio_core::DataItemType get_data_type(const std::string& datasetGuid)
{
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
std::string find_image_dataset(const std::string& datasetGuid)
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

py::tuple get_image_dimensions(const std::string& datasetGuid)
{
    auto item       = mv::dataHierarchy().getItem(QString(datasetGuid.c_str()));
    auto images     = item->getDataset<Images>();
    auto numImages  = images->getNumberOfImages();
    auto size       = images->getImageSize();

    return py::make_tuple(size.width(), size.height(), numImages);
}

py::tuple get_cluster(const std::string& datasetGuid)
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

bool set_linked_data(const std::string& sourceDataGuid, const std::string& targetDataGuid, const std::vector<std::vector<int64_t>>& selectionFromAToB)
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

// =============================================================================
// ManiVault embedding module 
// =============================================================================

namespace mvstudio_core {
    void register_mv_core_module(pybind11::module_& m)
    {
        m.def("get_top_level_item_names", get_top_level_item_names);
        m.def("get_top_level_guids", get_top_level_guids);
        m.def("get_data_for_item", get_data_for_item, py::arg("datasetGuid") = std::string());
        m.def("get_selection_for_item", get_selection_for_item, py::arg("datasetGuid") = std::string());
        m.def("set_selection_for_item", set_selection_for_item, py::arg("datasetGuid") = std::string(), py::arg("selectionIDs") = std::vector<uint32_t>());
        m.def("get_image_item", get_mv_image, py::arg("datasetGuid") = std::string());
        m.def("get_item_name", get_item_name, py::arg("datasetGuid") = std::string());
        m.def("get_item_rawsize", get_item_rawsize, py::arg("datasetGuid") = std::string());
        m.def("get_item_type", get_item_type, py::arg("datasetGuid") = std::string());
        m.def("get_item_rawname", get_item_rawname, py::arg("datasetGuid") = std::string());
        m.def("get_item_numdimensions", get_item_numdimensions, py::arg("datasetGuid") = std::string());
        m.def("get_item_numpoints", get_item_numpoints, py::arg("datasetGuid") = std::string());
        m.def("get_item_children", get_item_children, py::arg("datasetGuid") = std::string());
        m.def("get_item_properties", get_item_properties, py::arg("datasetGuid") = std::string());
        m.def("get_item_property", get_item_property, py::arg("datasetGuid") = std::string(), py::arg("propertyName") = std::string());
        m.def("get_data_type", get_data_type, py::arg("datasetGuid") = std::string());
        m.def("find_image_dataset", find_image_dataset, py::arg("datasetGuid") = std::string());
        m.def("get_image_dimensions", get_image_dimensions, py::arg("datasetGuid") = std::string());
        m.def("get_cluster", get_cluster, py::arg("datasetGuid") = std::string());
        m.def("set_linked_data",
            set_linked_data,
            py::arg("sourceDataGuid") = std::string(),
            py::arg("targetDataGuid") = std::string(),
            py::arg("selectionFromAToB") = std::vector<std::vector<int64_t>>()
            );
        m.def("add_new_points",
            add_new_point_data,
            py::arg("data"),    // do NOT = py::array() as this breaks loading the module in subinterpreters
            py::arg("dataSetName") = std::string(),
            py::arg("dataSetParentID") = std::string(),
            py::arg("dimensionNames") = std::vector<std::string>()
        );
        m.def( "add_derived_points",
            add_derived_point_data,
            py::arg("data"),    // do NOT = py::array() as this breaks loading the module in subinterpreters
            py::arg("dataSetName") = std::string(),
            py::arg("dataSetSourceID") = std::string(),
            py::arg("dimensionNames") = std::vector<std::string>()
        );
        m.def("add_new_image",
            add_new_image_data,
            py::arg("data"),    // do NOT = py::array() as this breaks loading the module in subinterpreters
            py::arg("dataSetName") = std::string(),
            py::arg("dimensionNames") = std::vector<std::string>()
        );
        // The cluster tuple contains the following lists: names, indexes (clusters), colors, ids (ignored)
        m.def("add_new_clusters",
            add_new_cluster_data,
            py::arg("parentPointDatasetGuid") = std::string(),
            py::arg("clusterIndices") = std::vector<py::array>(),
            py::arg("clusterNames") = std::vector<std::string>(),
            py::arg("clusterColors") = std::vector<py::array>(),
            py::arg("datasetName") = std::string()
        );

    }

} // namespace mvstudio_core
