#include "MVData.h"

#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "xeus-python/xinterpreter.hpp"
#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>
#include <QString>


namespace py = pybind11;

py::object get_info()
{
    return py::str("ManiVault is cool");
}

py::object get_top_level_items()
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
    std::vector<T> data;
    std::vector<unsigned int> indices;
    auto size = numPoints * numDimensions;
    data.resize(size);
    for (int i = 0; i < numDimensions; i++) {
        indices.push_back(i);
    }
    inputPoints->populateDataForDimensions<std::vector<T>, std::vector<unsigned int>>(data, indices);
    auto result = py::array_t<T>(size);
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
py::array get_data_for_item(const std::string& itemName)
{
    PointData::ElementTypeSpecifier dataSpec;
    unsigned int numDimensions;
    unsigned int numPoints;
    for (const auto topLevelDataHierarchyItem : Application::core()->getDataHierarchyManager().getTopLevelItems()) {
        if (topLevelDataHierarchyItem->getDataset()->getGuiName() == QString(itemName.c_str())) {
            auto dataType = topLevelDataHierarchyItem->getDataType();
            auto inputPoints = topLevelDataHierarchyItem->getDataset<Points>();
            numDimensions = inputPoints->getNumDimensions();
            numPoints = inputPoints->isFull() ? inputPoints->getNumPoints() : inputPoints->indices.size();
            auto size = numPoints * numDimensions;

            // extract the source type 
            inputPoints->visitSourceData([&dataSpec](auto pointData) {
                for (auto pointView : pointData) {
                    for (auto value : pointView) {
                        qInfo() << "checking point data";
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
        }
    }
    return py::array_t<float>(0);
}

template<class T>
void set_points_from_numpy_array(const void* data_in, size_t size, size_t dims, mv::Dataset<Points> points)
{
    auto data_out = std::vector<T>();
    data_out.resize(size);
    std::memcpy(data_out.data(), static_cast<const T*>(data_in), size);
    points->setData(data_out.data(), size, dims);
}

bool add_new_mvdata(const py::array& data, std::string dataSetName)
{
    size_t ndim = data.ndim();
    mv::Dataset<Points> points = mv::data().createDataset<Points>("Points", dataSetName.c_str(), nullptr);
    auto dtype = data.dtype();
    // PointData is limited in its type support - hopefully the commented types wil be added soon
    if (auto point_setter = (
        (dtype.is(pybind11::dtype::of<std::uint8_t>())) ? set_points_from_numpy_array<std::uint8_t> :
        (dtype.is(pybind11::dtype::of<std::int8_t>())) ? set_points_from_numpy_array<std::int8_t> :
        (dtype.is(pybind11::dtype::of<std::uint16_t>())) ? set_points_from_numpy_array<std::uint16_t> :
        (dtype.is(pybind11::dtype::of<std::int16_t>())) ? set_points_from_numpy_array<std::int16_t> :
        // 32 int are cast to float
        (dtype.is(pybind11::dtype::of<std::uint32_t>())) ? set_points_from_numpy_array<float> :
        (dtype.is(pybind11::dtype::of<std::int32_t>())) ? set_points_from_numpy_array<float> :
        //(dtype.is(pybind11::dtype::of<std::uint64_t>())) ? set_points_from_numpy_array<std::uint64_t> :
        //(dtype.is(pybind11::dtype::of<std::int64_t>())) ? set_points_from_numpy_array<std::int64_t> :
        (dtype.is(pybind11::dtype::of<float>())) ? set_points_from_numpy_array<float> :
        //(dtype.is(pybind11::dtype::of<double>())) ? set_points_from_numpy_array<double> :
        nullptr)
    ) {
        point_setter(data.data(), data.size(), data.ndim(), points);
        return true;
    }
    return false;
}

py::module get_MVData_module()
{
    py::module MVData_module = py::module_::create_extension_module("MVData", nullptr, new py::module_::module_def);
    MVData_module.def("get_info", get_info);
    MVData_module.def("get_top_level_items", get_top_level_items);
    MVData_module.def("get_data_for_item", get_data_for_item, py::arg("itemName") = py::str());
    MVData_module.def(
        "add_new_data",
        add_new_mvdata,
        py::arg("data") = py::array(),
        py::arg("dataSetName") = py::str()
    );

    return MVData_module;
}