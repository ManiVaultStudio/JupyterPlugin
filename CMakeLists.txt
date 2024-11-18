cmake_minimum_required(VERSION 3.21)

# vcpkg library settings
if(DEFINED MV_JUPYTER_USE_VCPKG AND MV_JUPYTER_USE_VCPKG)
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg" AND WIN32)
        set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
        set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "")
    endif()
else()
    set(MV_JUPYTER_USE_VCPKG OFF)
endif()

# -----------------------------------------------------------------------------
# Jupyter Plugins
# -----------------------------------------------------------------------------
PROJECT("JupyterPlugins"
    VERSION 0.1.0
    DESCRIPTION "Jupyter plugin for ManiVault"
    LANGUAGES CXX)

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# Ensures that all non-inline and inline symbols are hidden unless explicitly marked for export
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

if(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Cmake internal functions for generating a summary of enabled/disabled features.
include(FeatureSummary)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
# xeus depends on: nlohmann_json and xtl
# xeus-zmq depends on: xeus, ZeroMQ (libzmq), cppzmq, OpenSSL, and nlohmann_json
# xeus-python depends on: xeus-zmq, pybind11, pybind11_json and nlohmann_json
set(xeus_VERSION "3.2.0") # 5.1.1
set(xtl_VERSION "0.7.7") # not required for xeus > 5
set(nlohmann_json_VERSION "v3.11.3")
set(libzmq_VERSION "v4.3.5")
set(cppzmq_VERSION "v4.10.0")
set(xeus-zmq_VERSION "1.3.0") # 3.1.0
set(pybind11_VERSION "v2.13.6")
set(pybind11_json_VERSION "0.2.14")
set(xeus-python_VERSION "0.16.0") # 0.17.2, remove patch when updating

find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
find_package(ManiVault COMPONENTS Core PointData ClusterData ImageData CONFIG QUIET)
find_package(Python COMPONENTS Development Interpreter REQUIRED)

message(STATUS "*** Python version ${Python_VERSION}")
message(STATUS "*** Python libraries : ${Python_LIBRARIES}")
message(STATUS "*** Python include dirs ${Python_INCLUDE_DIRS}")

include(cmake/dependencies.cmake)

set_target_properties(xeus-static libzmq-static cppzmq xeus-zmq-static 
    PROPERTIES 
    FOLDER JupyterPluginDependencies
)

# -----------------------------------------------------------------------------
# Plugins
# -----------------------------------------------------------------------------

add_subdirectory(src/JupyterPlugin)
add_subdirectory(src/JupyterLauncher)

# we'd like to always build the python bridge plugin when building the launcher plugin
add_dependencies(JupyterLauncher ${JupyterPluginTarget})
