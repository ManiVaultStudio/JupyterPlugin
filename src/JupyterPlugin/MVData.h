#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

// =============================================================================
// ManiVault embedding module 
// =============================================================================

namespace mvstudio_core {
    enum DataItemType {
        NOT_IMPLEMENTED = -1,
        Image = 0,
        Points = 1,
        Cluster = 2
    };

    inline void register_mv_data_items(pybind11::module_& m) {
        pybind11::enum_<DataItemType>(m, "DataItemType")
            .value("Image", DataItemType::Image)
            .value("Points", DataItemType::Points)
            .value("Cluster", DataItemType::Cluster)
            .export_values();
    }

    void register_mv_core_module(pybind11::module_& m);
}

// =============================================================================
// ManiVault core and data interaction 
// =============================================================================

pybind11::object get_top_level_item_names();
pybind11::array get_data_for_item(const std::string& datasetGuid);
pybind11::list get_top_level_guids();
std::uint64_t get_item_numdimensions(const std::string& datasetGuid);
std::uint64_t get_item_numpoints(const std::string& datasetGuid);
std::string get_item_name(const std::string& datasetGuid);
std::string get_item_type(const std::string& datasetGuid);
std::string get_item_rawname(const std::string& datasetGuid);
pybind11::array get_selection_for_item(const std::string& datasetGuid);
void set_selection_for_item(const std::string& datasetGuid, const pybind11::array_t<uint32_t>& selectionIDs);
std::uint64_t get_item_rawsize(const std::string& datasetGuid);
std::vector<std::string> get_item_properties(const std::string& datasetGuid);
pybind11::object get_item_property(const std::string& datasetGuid, const std::string& propertyName);
pybind11::list get_item_children(const std::string& datasetGuid);
mvstudio_core::DataItemType get_data_type(const std::string& datasetGuid);
std::string find_image_dataset(const std::string& datasetGuid);
pybind11::tuple get_image_dimensions(const std::string& datasetGuid);
pybind11::tuple get_cluster(const std::string& datasetGuid);

std::string add_new_point_data(const pybind11::array& data, const std::string& dataSetName, const std::string& dataSetParentID, const std::vector<std::string>& dimensionNames);
std::string add_derived_point_data(const pybind11::array& data, const std::string& dataSetName, const std::string& dataSetSourceID, const std::vector<std::string>& dimensionNames);
std::string add_new_image_data(const pybind11::array& data, const std::string& dataSetName, const std::vector<std::string>& dimensionNames);
pybind11::array get_mv_image(const std::string& imageGuid);
std::string add_new_cluster_data(const std::string& parentPointDatasetGuid, const std::vector<pybind11::array>& clusterIndices, const std::vector<std::string>& clusterNames, const std::vector<pybind11::array>& clusterColors, const std::string& datasetName);

bool set_linked_data(const std::string& sourceDataGuid, const std::string& targetDataGuid, const std::vector<std::vector<int64_t>>& selectionFromAToB);
