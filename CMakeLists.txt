cmake_minimum_required(VERSION 3.21)

# vcpkg library settings
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg" AND WIN32)
    set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "")
endif()

# -----------------------------------------------------------------------------
# Jupyter Plugin
# -----------------------------------------------------------------------------
set(JUPYTERPLUGIN "JupyterPlugin")
PROJECT(${JUPYTERPLUGIN}
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

set(MV_JUPYTER_USE_VCPKG OFF)
if(DEFINED VCPKG_TARGET_TRIPLET)
    set(MV_JUPYTER_USE_VCPKG ON)
endif()

# Cmake internal functions for generating a summary of enabled/disabled features.
include(FeatureSummary)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
# xeus-python depends on: xeus-zmq, pybind11, pybind11_json and nlohmann_json
# xeus-zmq depends on: xeus, ZeroMQ (libzmq), cppzmq, OpenSSL, and nlohmann_json
# xeus depends on: nlohmann json
set(nlohmann_json_VERSION "v3.11.3")
set(xeus_VERSION "5.1.1")
set(libzmq_VERSION "v4.3.5")
set(cppzmq_VERSION "v4.10.0")
set(xeus-zmq_VERSION "3.1.0")
set(pybind11_VERSION "v2.13.6")
set(pybind11_json_VERSION "0.2.14")
set(xeus-python_VERSION "0.17.1")
set(Python_VERSION "3.11")

find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
find_package(ManiVault COMPONENTS Core PointData ClusterData ImageData CONFIG QUIET)
find_package(Python ${Python_VERSION} COMPONENTS Development Interpreter REQUIRED)

message(STATUS "Python root dir: ${Python_ROOT_DIR}")
message(STATUS "*** Python libraries : ${Python_LIBRARIES}")
message(STATUS "*** Python include dirs ${Python_INCLUDE_DIRS}")

include(cmake/dependencies.cmake)

add_subdirectory(src/JupyterLauncher)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
# Define the plugin sources
set(JUPYTERPLUGIN_SOURCES
    src/JupyterPlugin.h
    src/JupyterPlugin.cpp
    src/XeusKernel.h
    src/XeusKernel.cpp
    src/XeusPoller.h
    src/XeusPoller.cpp
    src/XeusServer.h
    src/XeusServer.cpp
    src/XeusInterpreter.h
    src/XeusInterpreter.cpp
    src/SettingsAction.h
    src/SettingsAction.cpp
    src/MVData.cpp
    src/MVData.h
    src/JupyterPlugin.json
    ${CMAKE_CURRENT_BINARY_DIR}/jupyterplugin_export.h
)

set(PLUGIN_MOC_HEADERS
    src/JupyterPlugin.h
    src/XeusKernel.h
    src/XeusPoller.h
    src/XeusServer.h
)

source_group(Plugin FILES ${JUPYTERPLUGIN_SOURCES})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
# Create dynamic library for the plugin
add_library(${JUPYTERPLUGIN} SHARED ${JUPYTERPLUGIN_SOURCES})

add_dependencies(${JUPYTERPLUGIN} JupyterLauncher)

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${JUPYTERPLUGIN} PRIVATE  "${ManiVault_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
# Request C++17
target_compile_features(${JUPYTERPLUGIN} PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(${JUPYTERPLUGIN})

# Retrieve the file name of the generated export header.
file(GLOB EXPORT_HEADER_FILE_NAME 
    RELATIVE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/*_export.h)
list(APPEND IMAGE_DATA_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_HEADER_FILE_NAME})

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
# Link to Qt libraries
target_link_libraries(${JUPYTERPLUGIN} PUBLIC Qt6::Widgets)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC Qt6::WebEngineWidgets)

target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::Core)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::PointData)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::ImageData)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::ClusterData)

target_link_libraries(${JUPYTERPLUGIN} PUBLIC xeus-static)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC xeus-zmq-static)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC xeus-python-static)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC libzmq-static cppzmq-static)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC pybind11::embed)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC nlohmann_json::nlohmann_json)

if (MSVC)
    target_compile_options(${JUPYTERPLUGIN} PUBLIC /wd4251 /wd4141)
    target_compile_options(${JUPYTERPLUGIN} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
endif()

# -----------------------------------------------------------------------------
# Python MVJupyterPluginManager package
# -----------------------------------------------------------------------------
option(MVSTUDIO_ENABLE_PYTHON_BUILD
       "Build JupyterPlugin support package (requires poetry)" ON)
add_feature_info("MVSTUDIO_ENABLE_PYTHON_BUILD" MVSTUDIO_ENABLE_PYTHON_BUILD
    "build Python bindings (requires poetry).")
if (MVSTUDIO_ENABLE_PYTHON_BUILD)
    execute_process(
            COMMAND ${Python_EXECUTABLE} -m pip show poetry
            RESULT_VARIABLE EXIT_CODE
            OUTPUT_QUIET)
    if (NOT ${EXIT_CODE} EQUAL 0)        
        message(WARNING
            "Cannot find 'poetry' in.
            'poetry' is required for building the JupyterPlugin support package")
        # If not a vritual environment install only for user
        if (DEFINED ENV{VIRTUL_ENV} OR DEFINED ENV{CONDA_PREFIX})
            set(_pip_args)
        else()
            set(_pip_args "--user")
        endif()
        message(STATUS "Attempting to to install python poetry package...")
        execute_process(
            COMMAND ${Python_EXECUTABLE} -m pip install ${_pip_args} --no-warn-script-location poetry 
            RESULT_VARIABLE POETRY_INSTALL_RESULT
            OUTPUT_QUIET)
        if (POETRY_INSTALL_RESULT)
            message(
            FATAL_ERROR
                "Install poetry failed 
                Either 'pip install poetry' or 'MVSTUDIO_ENABLE_PYTHON_BUILD' to 'OFF'"
            )
        else()
            message(STATUS "'poetry' succesfully installed")
        endif()
    endif()
    message(STATUS "Building the mvstudio_data python package in ${CMAKE_CURRENT_SOURCE_DIR}")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry version ${CMAKE_PROJECT_VERSION}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/mvstudio_data
        COMMAND_ERROR_IS_FATAL ANY)    
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/mvstudio_data
        COMMAND_ERROR_IS_FATAL ANY)
    message(STATUS "Building the mvstudio_data python package in ${CMAKE_CURRENT_SOURCE_DIR}")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry version ${CMAKE_PROJECT_VERSION}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/mvstudio_kernel
        COMMAND_ERROR_IS_FATAL ANY)  
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/mvstudio_kernel
        COMMAND_ERROR_IS_FATAL ANY)
endif ()


# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
# Install the shared plugin libary to the "Plugins" folder in the ManiVault install directory
install(TARGETS ${JUPYTERPLUGIN}
    RUNTIME DESTINATION Plugins/JupyterPlugin COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins/JupyterPlugin COMPONENT PLUGINS # Linux/Mac .so
    ARCHIVE DESTINATION lib     COMPONENT LINKLIB # Windows .lib 
)

install(FILES jupyter_server_config.py python/mvstudio_data/dist/mvstudio_data-${CMAKE_PROJECT_VERSION}-py3-none-any.whl
DESTINATION Plugins/JupyterPlugin/wheelhouse COMPONENT PLUGINS)

install(FILES jupyter_server_config.py python/mvstudio_kernel/dist/mvstudio_kernel-${CMAKE_PROJECT_VERSION}-py3-none-any.whl
DESTINATION Plugins/JupyterPlugin/wheelhouse COMPONENT PLUGINS)

add_custom_command(TARGET ${JUPYTERPLUGIN} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>/
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${JUPYTERPLUGIN} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/debug,${ManiVault_INSTALL_DIR}/release>)
    set_property(TARGET ${JUPYTERPLUGIN} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/debug/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/release/ManiVault Studio.exe">)
endif()

