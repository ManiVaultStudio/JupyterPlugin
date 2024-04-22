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
set(ARTIFACTORY-DOWNLOAD-TOKEN "cmVmdGtuOjAxOjAwMDAwMDAwMDA6OGV4QnVZcmR0S2piU0RSWTJTbjRRQTMxYkRh")

if (WIN32)
    find_package (OpenSSL)
    if (NOT OpenSSL_FOUND)
        message("Unpacking OpenSSL 1.1.1w static binary")
        file(DOWNLOAD https://lkeb-artifactory.lumc.nl:443/artifactory/miscbinaries/openssl111w_windows_static.zip ${CMAKE_CURRENT_BINARY_DIR}/openssl111w_windows_static.zip HTTPHEADER "X-JFrog-Art-Api:${ARTIFACTORY-DOWNLOAD-TOKEN}") 
        file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/openssl111w_windows_static.zip DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl111w CACHE PATH "OpenSSL root side loaded")
    endif()
endif()

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

set(CPM_DEPS_DIR ${CMAKE_BINARY_DIR}/_deps)
message("xeus src : ${CPM_DEPS_DIR}/xeus-src")
# Delete the previous _deps to allow the patch to work (TBD find more efficient approach)
if(EXISTS ${CPM_DEPS_DIR}/xeus-src)
    exec_program(
        ${CMAKE_COMMAND} ARGS "-E remove_directory ${CPM_DEPS_DIR}/xeus-src"
    )
endif()
if(EXISTS ${CPM_DEPS_DIR}/xeus-subbuild)
    exec_program(
        ${CMAKE_COMMAND} ARGS "-E remove_directory ${CPM_DEPS_DIR}/xeus-subbuild"
    )
endif()
if(EXISTS ${CPM_DEPS_DIR}/xeus-build)
    exec_program(
        ${CMAKE_COMMAND} ARGS "-E remove_directory ${CPM_DEPS_DIR}/xeus-build"
    )
endif()

set(CPM_DOWNLOAD_xeus YES)
# produces xeus and xeus-static
# Check if the patch (used in codon) is still needed
# appears to be releated to libuuid
CPMAddPackage(
    NAME xeus
    GITHUB_REPOSITORY "jupyter-xeus/xeus"
    VERSION ${xeus_VERSION}
    GIT_TAG ${xeus_VERSION}
    EXCLUDE_FROM_ALL YES
    PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace --whitespace=nowarn ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uuidopt.patch
    OPTIONS "BUILD_EXAMPLES OFF"
            "XEUS_BUILD_SHARED_LIBS OFF"
            "XEUS_BUILD_STATIC_LIBS ON"
            "XEUS_STATIC_DEPENDENCIES ON"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "XEUS_DISABLE_ARCH_NATIVE ON"
            "XEUS_USE_DYNAMIC_UUID ON")

if (xeus_ADDED)
    install(TARGETS nlohmann_json EXPORT xeus-targets)
    include("${xeus_BINARY_DIR}/xeusConfig.cmake")
endif()

# produces libzmq and libzmq-static depending on settings
CPMAddPackage(
  NAME libzmq
  VERSION ${libzmq_VERSION}
  URL ${libzmq_PATH}
  EXCLUDE_FROM_ALL YES
  OPTIONS "WITH_PERF_TOOL OFF"
          "BUILD_TESTS OFF"
          "ENABLE_CPACK OFF"
          "BUILD_SHARED OFF"
          "BUILD_STATIC ON"
          "WITH_LIBSODIUM OFF"
          "ENABLE_CURVE OFF"
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

find_package (Python ${PythonVERSION} COMPONENTS Development Interpreter REQUIRED)


message("Python root dir: ${Python_ROOT_DIR}")
message("*** Python libraries : ${Python_LIBRARIES}")
message("*** Python include dirs ${Python_INCLUDE_DIRS}")
set(PYTHON_LIBRARIES ${Python_LIBRARIES})  # for xeus-python linking

# Delete an existing xeus-zmq to allow the patch to work (TBD find more efficient solution)
if(EXISTS ${CPM_DEPS_DIR}/xeus-zmq-src)
    exec_program(
        ${CMAKE_COMMAND} ARGS "-E remove_directory ${CPM_DEPS_DIR}/xeus-zmq-src"
    )
endif()
if(EXISTS ${CPM_DEPS_DIR}/xeus-zmq-subbuild)
    exec_program(
        ${CMAKE_COMMAND} ARGS "-E remove_directory ${CPM_DEPS_DIR}/xeus-zmq-subbuild"
    )
endif()
if(EXISTS ${CPM_DEPS_DIR}/xeus-zmq-build)
    exec_program(
        ${CMAKE_COMMAND} ARGS "-E remove_directory ${CPM_DEPS_DIR}/xeus-zmq-build"
    )
endif()
set(CPM_DOWNLOAD_xeus-zmq YES)
CPMAddPackage(
    NAME xeus-zmq
    GITHUB_REPOSITORY "jupyter-xeus/xeus-zmq"
    VERSION ${xeus-zmq_VERSION}
    GIT_TAG ${xeus-zmq_VERSION}
    EXCLUDE_FROM_ALL YES
    PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace --whitespace=nowarn ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libsodiumopt.patch
    OPTIONS "XEUS_ZMQ_BUILD_TESTS OFF"
            "XEUS_ZMQ_BUILD_SHARED_LIBS OFF"
            "XEUS_ZMQ_BUILD_STATIC_LIBS ON"
            "XEUS_ZMQ_STATIC_DEPENDENCIES ON"
            "XEUS_ZMQ_WITH_LIBSODIUM OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON")

if (xeus-zmq_ADDED)
    add_dependencies(xeus-zmq-static libzmq-static xeus-static)
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
    OPTIONS "XPYT_BUILD_TESTS OFF"
            "XPYT_BUILD_SHARED OFF"
            "XPYT_BUILD_STATIC ON"
            "XPYT_BUILD_XPYTHON_EXECUTABLE OFF"
            "XPYT_USE_SHARED_XEUS_PYTHON OFF"
            "XPYT_USE_SHARED_XEUS OFF"
            "EMSCRIPTEN OFF")

add_dependencies(xeus-python-static xeus-zmq-static xeus-static)

