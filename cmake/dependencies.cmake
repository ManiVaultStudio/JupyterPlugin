# Download dependencies with CPM
# Wrapper around fetch content
include(get_cpm)

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

# Hack: ManiVault downloads nlohmann_json but only downloads it
#       so we want to add the package again but also set it up
list(FIND CPM_PACKAGES "nlohmann_json" index_cpm_nlohmann_json)
if(NOT index_cpm_nlohmann_json EQUAL -1)
    list(REMOVE_ITEM CPM_PACKAGES "nlohmann_json")
endif()

CPMAddPackage(
  NAME              nlohmann_json
  GITHUB_REPOSITORY	"nlohmann/json"
  GIT_TAG           ${nlohmann_json_VERSION}
  DOWNLOAD_ONLY     NO
  EXCLUDE_FROM_ALL  YES
  OPTIONS
      "JSON_BuildTests=OFF"            # skip upstream tests
      "JSON_MultipleHeaders=ON"        # shave compile time (optional)
)

if(XEUS_BUILD_STATIC_DEPENDENCIES)
    message(STATUS "Linking against UUID statically...")
endif()

CPMAddPackage(
    NAME                xeus
    GITHUB_REPOSITORY   "jupyter-xeus/xeus"
    GIT_TAG             ${xeus_VERSION}
    EXCLUDE_FROM_ALL    YES
    CPM_USE_LOCAL_PACKAGES ON
    OPTIONS 
        "BUILD_EXAMPLES OFF"
        "XEUS_BUILD_SHARED_LIBS OFF"
        "XEUS_BUILD_STATIC_LIBS ON"
        "XEUS_STATIC_DEPENDENCIES ${XEUS_BUILD_STATIC_DEPENDENCIES}"
)

# produces libzmq and libzmq-static depending on settings
CPMAddPackage(
    NAME                libzmq
    GITHUB_REPOSITORY   "zeromq/libzmq"
    GIT_TAG             ${libzmq_VERSION}
    EXCLUDE_FROM_ALL    YES
    OPTIONS 
        "WITH_PERF_TOOL OFF"
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
    NAME                cppzmq
    GITHUB_REPOSITORY   "zeromq/cppzmq"
    GIT_TAG             ${cppzmq_VERSION}
    EXCLUDE_FROM_ALL    YES
    OPTIONS 
        "CPPZMQ_BUILD_TESTS OFF"
)

# XEUS_ZMQ_STATIC_DEPENDENCIES introduces libsodium dependency on linux
CPMAddPackage(
    NAME                xeus-zmq
    GITHUB_REPOSITORY   "jupyter-xeus/xeus-zmq"
    GIT_TAG             ${xeus-zmq_VERSION}
    EXCLUDE_FROM_ALL    YES
    CPM_USE_LOCAL_PACKAGES ON
    OPTIONS 
        "XEUS_ZMQ_BUILD_TESTS OFF"
        "XEUS_ZMQ_BUILD_SHARED_LIBS OFF"
        "XEUS_ZMQ_BUILD_STATIC_LIBS ON"
        "XEUS_ZMQ_STATIC_DEPENDENCIES ON"
)

install(TARGETS nlohmann_json EXPORT xeus-targets)

# preempt some complaints from pybind
set(PYTHON_EXECUTABLE "${Python_EXECUTABLE}")

CPMAddPackage(
    NAME                pybind
    GITHUB_REPOSITORY   pybind/pybind11
    GIT_TAG             ${pybind11_VERSION}
    EXCLUDE_FROM_ALL    YES
    OPTIONS 
        "PYBIND11_FINDPYTHON ON"
)

include("${pybind_SOURCE_DIR}/tools/pybind11Common.cmake")

CPMAddPackage(
    NAME                pybind11_json
    GITHUB_REPOSITORY   pybind/pybind11_json
    GIT_TAG             ${pybind11_json_VERSION_TAG}
    VERSION             ${pybind11_json_VERSION}
    EXCLUDE_FROM_ALL    YES
)

CPMAddPackage(
    NAME                xeus-python
    GITHUB_REPOSITORY   jupyter-xeus/xeus-python
    GIT_TAG             ${xeus-python_VERSION}
    EXCLUDE_FROM_ALL    YES
    OPTIONS 
        "XPYT_BUILD_TESTS OFF"
        "XPYT_BUILD_SHARED OFF"
        "XPYT_BUILD_STATIC ON"
        "XPYT_BUILD_XPYTHON_EXECUTABLE OFF"
        "XPYT_USE_SHARED_XEUS_PYTHON OFF"
        "XPYT_USE_SHARED_XEUS OFF"
        "EMSCRIPTEN OFF"
)

if(UNIX)
    set_target_properties(xeus-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(xeus-zmq-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(xeus-python-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(libzmq-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(cppzmq-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

CPMAddPackage(
    NAME                md4qt
    GIT_REPOSITORY      https://invent.kde.org/libraries/md4qt.git
    GIT_TAG             a777b56c14679b94d48261ec465cf0e97650daad
    VERSION             4.2.1
    DOWNLOAD_ONLY       YES
    EXCLUDE_FROM_ALL    YES
)
