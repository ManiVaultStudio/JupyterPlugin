cmake_minimum_required(VERSION 3.21)
# Building xeus
set(xtl_VERSION "0.7.7")
set(nlohmann_json_VERSION "v3.11.3" )
set(nlohmann_json_SHA "a22461d13119ac5c78f205d3df1db13403e58ce1bb1794edc9313677313f4a9d")
set(nlohmann_json_PATH "https://github.com/nlohmann/json/releases/download/${nlohmann_json_VERSION}/include.zip")
set(xeus_VERSION "3.1.4")

# Building xeus-zmq
set(libzmq_VERSION "v4.3.5")
set(libzmq_PATH "https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz")
set(cppzmq_VERSION "v4.10.0")
set(cppzmq_PATH "https://github.com/zeromq/cppzmq/archive/refs/tags/v4.10.0.tar.gz")
set(xeus-zmq_VERSION "1.1.1")

# Building xeus-python
set(pybind11_VERSION "2.11.1")
set(pybind11_json_VERSION "0.2.11")
set(xeus-python_VERSION "0.15.12")

include(CPM)

CPMAddPackage(
    NAME xtl
    GITHUB_REPOSITORY "xtensor-stack/xtl"
    VERSION ${xtl_VERSION}
    GIT_TAG ${xtl_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS "BUILD_TESTS OFF")

# nlohmann json is rather large: just use the release/include rather than the whole repo.
# Check https://github.com/nlohmann/json/releases for details
CPMAddPackage(
    NAME nlohmann_json
    URL "${nlohmann_json_PATH}"
    URL_HASH SHA256=${nlohmann_json_SHA}
    DOWNLOAD_ONLY YES
)

add_library(nlohmann_json INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)

target_include_directories(nlohmann_json INTERFACE
    "$<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}>/include"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# produces xeus and xeus-static
# Check if the patch (used in codon) is still needed
# appears to be releated to libuuid
CPMAddPackage(
    NAME xeus
    GITHUB_REPOSITORY "jupyter-xeus/xeus"
    VERSION ${xeus_VERSION}
    GIT_TAG ${xeus_VERSION}
    EXCLUDE_FROM_ALL YES
#    PATCH_COMMAND git apply --reject --whitespace=fix ${CMAKE_SOURCE_DIR}/xeus.patch
    OPTIONS "BUILD_EXAMPLES OFF"
            "XEUS_BUILD_SHARED_LIBS ON"
            "XEUS_BUILD_STATIC_LIBS ON"
            "XEUS_STATIC_DEPENDENCIES ON"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "XEUS_DISABLE_ARCH_NATIVE ON"
            "XEUS_USE_DYNAMIC_UUID ${XEUS_USE_DYNAMIC_UUID}")

if (xeus_ADDED)
    install(TARGETS nlohmann_json EXPORT xeus-targets)
    include("${xeus_BINARY_DIR}/xeusConfig.cmake")
endif()

# produces libzmq and libzmq-static
CPMAddPackage(
  NAME libzmq
  VERSION ${libzmq_VERSION}
  URL ${libzmq_PATH}
  EXCLUDE_FROM_ALL YES
  OPTIONS "WITH_PERF_TOOL OFF"
          "ZMQ_BUILD_TESTS OFF"
          "ENABLE_CPACK OFF"
          "BUILD_SHARED ON"
          "BUILD_STATIC ON"
          "WITH_LIBSODIUM OFF"
          "WITH_TLS OFF"
          "WITH_DOC OFF")

# produces cppzmq and cppzmq-static
CPMAddPackage(
    NAME cppzmq
    URL ${cppzmq_PATH}
    VERSION ${cppzmq_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS "CPPZMQ_BUILD_TESTS OFF")

target_include_directories(cppzmq INTERFACE
    "$<BUILD_INTERFACE:${cppzmq_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set(PythonVERSION 3.11)

if(${CMAKE_VERSION} VERSION_LESS "3.12.0") 
	include(FindPythonInterp)
    message("CMAKE VERSION less than 3.12.0")
	# Set variables as they would be set by module FindPython,
	# which is available from CMake 3.12.
	set(Python_Interpreter_FOUND ${PYTHONINTERP_FOUND})
	set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
else()
    find_package (Python COMPONENTS Development Interpreter REQUIRED)
endif()

message("Python root dir: ${Python_ROOT_DIR}")
message("*** Python libraries : ${Python_LIBRARIES}")
message("*** Python include dirs ${Python_INCLUDE_DIRS}")
set(PYTHON_LIBRARIES ${Python_LIBRARIES})  # for xeus-python linking

CPMAddPackage(
    NAME xeus-zmq
    GITHUB_REPOSITORY "jupyter-xeus/xeus-zmq"
    VERSION ${xeus-zmq_VERSION}
    GIT_TAG ${xeus-zmq_VERSION}
    EXCLUDE_FROM_ALL YES
#    PATCH_COMMAND patch -N -u CMakeLists.txt --ignore-whitespace -b ${CMAKE_SOURCE_DIR}/xeus.patch || true
    OPTIONS "XEUS_ZMQ_BUILD_TESTS OFF"
            "XEUS_ZMQ_BUILD_SHARED_LIBS ON"
            "XEUS_ZMQ_BUILD_STATIC_LIBS ON"
            "XEUS_ZMQ_STATIC_DEPENDENCIES ON"
            "CMAKE_POSITION_INDEPENDENT_CODE ON")

if (xeus-zmq_ADDED)
    add_dependencies(xeus-zmq xeus)
    # install(TARGETS cppzmq EXPORT xeus-zmq-targets)
endif()

CPMAddPackage(
  NAME pybind
  VERSION ${pybind11_VERSION}
  EXCLUDE_FROM_ALL YES
  GITHUB_REPOSITORY pybind/pybind11)

if(pybind_ADDED)
  message("pybind11 added: ${pybind_SOURCE_DIR}/tools/pybind11Common.cmake")
  include("${pybind_SOURCE_DIR}/tools/pybind11Common.cmake")
  message("pybind include dirs ${pybind11_INCLUDE_DIRS}")
endif()

CPMAddPackage(
    NAME GTest
    GITHUB_REPOSITORY google/googletest
    VERSION 1.14.0
    OPTIONS  "BUILD_GMOCK OFF" "INSTALL_GTEST OFF"
)

CPMAddPackage("gh:pybind/pybind11_json#${pybind11_json_VERSION}")


CPMAddPackage(
    NAME xeus-python
    GITHUB_REPOSITORY "jupyter-xeus/xeus-python"
    GIT_TAG ${xeus-python_VERSION}
    VERSION ${xeus-python_VERSION}
    EXCLUDE_FROM_ALL YES
#    PATCH_COMMAND git apply --reject --whitespace=fix ${CMAKE_SOURCE_DIR}/xeus.patch
    OPTIONS "XPYT_BUILD_TESTS OFF"
            "BUILD_SHARED_LIBS ON"
            "XPYT_BUILD_XPYTHON_EXECUTABLE OFF"
            "EMSCRIPTEN OFF")

target_link_libraries(xeus-python
    PUBLIC 
    cppzmq
    nlohmann_json
    xeus)
add_dependencies(xeus-python xeus-zmq)

