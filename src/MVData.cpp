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
    for (const auto topLevelDataHierarchyItem :  Application::core()->getDataHierarchyManager().getTopLevelItems()) {
        result.append(py::str(topLevelDataHierarchyItem->getDataset()->getGuiName().toUtf8().constData()));
    }
    return result;

}


// only works on top level items as test
// Get the point data associated with the names item and return it as a numpy array to python
py::array_t<float, py::array::c_style> get_data_for_item(const std::string& itemName)
{
    std::vector<float> data;
    std::vector<unsigned int> indices;
    unsigned int numDimensions;
    unsigned int numPoints;
    for (const auto topLevelDataHierarchyItem : Application::core()->getDataHierarchyManager().getTopLevelItems()) {
        if (topLevelDataHierarchyItem->getDataset()->getGuiName() == QString(itemName.c_str())) {
            auto inputPoints = topLevelDataHierarchyItem->getDataset<Points>();
            numDimensions = inputPoints->getNumDimensions();
            numPoints = inputPoints->isFull() ? inputPoints->getNumPoints() : inputPoints->indices.size();
            data.resize( numPoints * numDimensions);
            for (int i = 0; i < numDimensions; i++) {
                indices.push_back(i);
            }
            inputPoints->populateDataForDimensions<std::vector<float>, std::vector<unsigned int>>(data, indices);
        }
    }
    auto size = numPoints * numDimensions;
    auto result = py::array_t<float>(size);
    py::buffer_info result_info = result.request();
    float* output = static_cast<float*>(result_info.ptr);
    for (decltype(size) i = 0; i < size; i++) {
        output[i] = data[i];
    }
    return result;
}

py::module get_MVData_module()
{
    py::module MVData_module = py::module_::create_extension_module("MVData", nullptr, new py::module_::module_def);
    MVData_module.def("get_info", get_info);
    MVData_module.def("get_top_level_items", get_top_level_items);
    MVData_module.def("get_data_for_item", get_data_for_item, py::arg("itemName") = py::str());

    return MVData_module;
}