# Download dependencies with CPM
# Wrapper around fetch content
include(cmake/get_cpm.cmake)

if (WIN32)
    if(${MV_JUPYTER_USE_VCPKG})
        find_package (OpenSSL REQUIRED)
    else()
        find_package (OpenSSL)
        if (NOT OpenSSL_FOUND)
            message(STATUS "Download OpenSSL 1.1.1w static binary form artifactory")
            set(ARTIFACTORY-DOWNLOAD-TOKEN "cmVmdGtuOjAxOjAwMDAwMDAwMDA6OGV4QnVZcmR0S2piU0RSWTJTbjRRQTMxYkRh")
            file(DOWNLOAD https://lkeb-artifactory.lumc.nl:443/artifactory/miscbinaries/openssl111w_windows_static.zip ${CMAKE_CURRENT_BINARY_DIR}/openssl111w_windows_static.zip HTTPHEADER "X-JFrog-Art-Api:${ARTIFACTORY-DOWNLOAD-TOKEN}") 
            file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/openssl111w_windows_static.zip DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
            set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl111w CACHE PATH "OpenSSL root side loaded")
        endif()
    endif()
    
endif()

CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY "nlohmann/json"
    GIT_TAG ${nlohmann_json_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS
    "JSON_BuildTests OFF"
)

CPMAddPackage(
    NAME xtl
    GITHUB_REPOSITORY "xtensor-stack/xtl"
    GIT_TAG ${xtl_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS "BUILD_TESTS OFF"
)

# produces xeus and xeus-static
CPMAddPackage(
    NAME xeus
    GITHUB_REPOSITORY "jupyter-xeus/xeus"
    GIT_TAG ${xeus_VERSION}
    EXCLUDE_FROM_ALL YES
    CPM_USE_LOCAL_PACKAGES ON
    OPTIONS "BUILD_EXAMPLES OFF"
            "XEUS_BUILD_SHARED_LIBS OFF"
            "XEUS_BUILD_STATIC_LIBS ON"
            "XEUS_STATIC_DEPENDENCIES ON"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
            "XEUS_DISABLE_ARCH_NATIVE ON"
            "XEUS_USE_DYNAMIC_UUID ON"
            "XEUS_ZMQ_BUILD_WITHOUT_LIBSODIUM ON"
)


# produces libzmq and libzmq-static depending on settings
CPMAddPackage(
    NAME libzmq
    GITHUB_REPOSITORY "zeromq/libzmq"
    GIT_TAG ${libzmq_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS "WITH_PERF_TOOL OFF"
            "BUILD_TESTS OFF"
            "ENABLE_CPACK OFF"
            "BUILD_SHARED OFF"
            "BUILD_STATIC ON"
            "WITH_LIBSODIUM OFF"
            "ENABLE_CURVE OFF"
            "WITH_TLS OFF"
            "WITH_DOC OFF"
)

# produces cppzmq and cppzmq-static
CPMAddPackage(
    NAME cppzmq
    GITHUB_REPOSITORY "zeromq/cppzmq"
    GIT_TAG ${cppzmq_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS "CPPZMQ_BUILD_TESTS OFF"
)

CPMAddPackage(
    NAME xeus-zmq
    GITHUB_REPOSITORY "jupyter-xeus/xeus-zmq"
    GIT_TAG ${xeus-zmq_VERSION}
    EXCLUDE_FROM_ALL YES
    CPM_USE_LOCAL_PACKAGES ON
    OPTIONS "XEUS_ZMQ_BUILD_TESTS OFF"
            "XEUS_ZMQ_BUILD_SHARED_LIBS OFF"
            "XEUS_ZMQ_BUILD_STATIC_LIBS ON"
            "XEUS_ZMQ_STATIC_DEPENDENCIES ON"
            "XEUS_ZMQ_WITH_LIBSODIUM OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

install(TARGETS nlohmann_json EXPORT xeus-targets)

CPMAddPackage(
    NAME pybind
    GITHUB_REPOSITORY pybind/pybind11
    GIT_TAG ${pybind11_VERSION}
    EXCLUDE_FROM_ALL YES
    OPTIONS "PYBIND11_FINDPYTHON ON"
)

include("${pybind_SOURCE_DIR}/tools/pybind11Common.cmake")

CPMAddPackage(
    NAME pybind11_json
    GITHUB_REPOSITORY pybind/pybind11_json
    GIT_TAG ${pybind11_json_VERSION}
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME xeus-python
    GITHUB_REPOSITORY "jupyter-xeus/xeus-python"
    GIT_TAG ${xeus-python_VERSION}
    EXCLUDE_FROM_ALL YES
    CPM_USE_LOCAL_PACKAGES ON
    OPTIONS "XPYT_BUILD_TESTS OFF"
            "XPYT_BUILD_SHARED OFF"
            "XPYT_BUILD_STATIC ON"
            "XPYT_BUILD_XPYTHON_EXECUTABLE OFF"
            "XPYT_USE_SHARED_XEUS_PYTHON OFF"
            "XPYT_USE_SHARED_XEUS OFF"
            "EMSCRIPTEN OFF"
)

