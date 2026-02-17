cmake_minimum_required(VERSION 3.31)

option(XEUS_BUILD_STATIC_DEPENDENCIES "Xeus depends on libuuid. If that package is installed in your build env you can link statically to it" OFF)
option(MV_JUPYTER_USE_VCPKG "Use vcpkg for openssl dependency" OFF)

# vcpkg library settings
if(${MV_JUPYTER_USE_VCPKG} AND WIN32)
    message(STATUS "JupyterPlugins: Using vcpkg to install dependencies")
    set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "")
endif()

# -----------------------------------------------------------------------------
# Jupyter Plugins
# -----------------------------------------------------------------------------
PROJECT("JupyterPlugins"
    VERSION 0.6.0
    DESCRIPTION "Jupyter plugin for ManiVault"
    LANGUAGES CXX)

# the version number corresponds to the pyproject.toml files in mvstudio_data and mvstudio_kernel,
# as well as the PluginInfo.json in src/JupyterLauncher and src/JupyterPlugin

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_AUTOMOC ON)

# Ensures that all non-inline and inline symbols are hidden unless explicitly marked for export
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif()

include(CMakePackageConfigHelpers)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
# xeus depends on: nlohmann_json
# xeus-zmq depends on: xeus, ZeroMQ (libzmq), cppzmq, OpenSSL, and nlohmann_json
# xeus-python depends on: xeus-zmq, pybind11, pybind11_json and nlohmann_json
set(xeus_VERSION "5.2.6")
set(nlohmann_json_VERSION "v3.12.0")
set(libzmq_VERSION "v4.3.5")
set(cppzmq_VERSION "v4.11.0")
set(xeus-zmq_VERSION "3.1.1")
set(pybind11_VERSION "v3.0.2")
set(pybind11_json_VERSION "0.2.15")
set(pybind11_json_VERSION_TAG "d0bf434be9d287d73a963ff28745542daf02c08f") # includes support for cmake 4
set(xeus-python_VERSION "0.17.6")

find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
find_package(ManiVault COMPONENTS Core PointData ClusterData ImageData CONFIG QUIET)
find_package(Python COMPONENTS Development Interpreter REQUIRED)

message(STATUS "*** Python version: ${Python_VERSION}")
message(STATUS "*** Python interpreter: ${Python_EXECUTABLE}")
message(STATUS "*** Python libraries: ${Python_LIBRARIES}")
message(STATUS "*** Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "*** Python library dirs: ${Python_LIBRARY_DIRS}")

include(dependencies)

# -----------------------------------------------------------------------------
# Plugins
# -----------------------------------------------------------------------------

add_subdirectory(src/JupyterLauncher)
add_subdirectory(src/JupyterPlugin)

# we'd like to always build the python bridge plugin when building the launcher plugin
add_dependencies(JupyterLauncher ${JupyterPluginTarget})
