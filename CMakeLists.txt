cmake_minimum_required(VERSION 3.21)

# -----------------------------------------------------------------------------
# Jupyter Plugin
# -----------------------------------------------------------------------------
set(JUPYTERPLUGIN "JupyterPlugin")
PROJECT(${JUPYTERPLUGIN})

add_subdirectory(src/JupyterLauncher)

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(QTVERSION 6)
set(CPPZMQ_BUILD_TESTS OFF) # cppzmq

cmake_policy(SET CMP0074 NEW)
# gcc visibility setting
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

# -----------------------------------------------------------------------------
# Set install directory
# -----------------------------------------------------------------------------
# Check if the directory to the ManiVault installation has been provided
if(NOT DEFINED MV_INSTALL_DIR)
    set(MV_INSTALL_DIR "" CACHE PATH "Directory where ManiVault is installed")
    message(FATAL_ERROR "Please set MV_INSTALL_DIR to the directory where ManiVault is installed")
endif()
file(TO_CMAKE_PATH ${MV_INSTALL_DIR} MV_INSTALL_DIR)
include(dependencies.cmake)

set(xtl_DIR "${xtl_SOURCE_DIR}" CACHE PATH "")
set(PYBIND11_DIR "${pybind_SOURCE_DIR}")
set(PYBIND11_INCLUDE_DIR "${pybind_SOURCE_DIR}/include" CACHE PATH "Path to root of pybind11 include")
set(xeus_ROOT "" CACHE PATH "path to the xeus dependency root")
set(xeus-zmq_ROOT "" CACHE PATH "path to the xeus-zmq dependency root")
set(xeus-python_ROOT "" CACHE PATH "path to the xeus-python dependency root")
# These header only libraries are packages with xeus-zmq for convenience
set(nlohmann_json_INCLUDE_DIR ${nlohmann_SOURCE_DIR}/include)
set(cppzmq_INCLUDE_DIR ${xeus-zmq_ROOT}/include)


find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
#find_package(xeus REQUIRED)
#find_package(xeus-zmq REQUIRED)
#find_package(xeus-python REQUIRED)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
# Define the plugin sources
set(JUPYTERPLUGIN_SOURCES
    src/JupyterPlugin.h
    src/JupyterPlugin.cpp
    src/XeusKernel.h
    src/XeusKernel.cpp
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
)

source_group( Plugin FILES ${JUPYTERPLUGIN_SOURCES})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
# Create dynamic library for the plugin
add_library(${JUPYTERPLUGIN} SHARED ${JUPYTERPLUGIN_SOURCES})

add_dependencies(${JUPYTERPLUGIN} xeus-python JupyterLauncher)

qt_wrap_cpp(JUPYTERPLUGIN_MOC ${PLUGIN_MOC_HEADERS} TARGET ${JUPYTERPLUGIN})
target_sources(${JUPYTERPLUGIN} PRIVATE ${JUPYTERPLUGIN_MOC})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
# Include HDPS headers, including system data plugins
target_include_directories(${JUPYTERPLUGIN} PRIVATE 
    "${MV_INSTALL_DIR}/$<CONFIGURATION>/include/" 
    "${xeus_INCLUDE_DIR}"
    "${xeus-zmq_INCLUDE_DIR}" 
    "${xeus-python_INCLUDE_DIR}" 
    "${xtl_INCLUDE_DIRS}"
    "${ZeroMQ_INCLUDE_DIR}" 
    "${cppzmq_INCLUDE_DIR}" 
    "${nlohmann_json_INCLUDE_DIR}"
    "${PYBIND11_INCLUDE_DIR}"
    "${PYTHON_INCLUDE_DIR}"
)

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
target_link_libraries(${JUPYTERPLUGIN} PRIVATE Qt6::Widgets)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE Qt6::WebEngineWidgets)

# Link to HDPS and data plugins
# The link path in this repo assumes that the ManiVault core was built locally
# in contrast to having been installed with an installer. In the latter case you'll have
# to adapt the MV_LINK_PATH and PLUGIN_LINK_PATH to your install folder
set(MV_LINK_PATH "${MV_INSTALL_DIR}/$<CONFIGURATION>/lib")
set(PLUGIN_LINK_PATH "${MV_INSTALL_DIR}/$<CONFIGURATION>/$<IF:$<CXX_COMPILER_ID:MSVC>,lib,Plugins>")
set(MV_LINK_SUFFIX $<IF:$<CXX_COMPILER_ID:MSVC>,${CMAKE_LINK_LIBRARY_SUFFIX},${CMAKE_SHARED_LIBRARY_SUFFIX}>)

set(MV_LINK_LIBRARY "${MV_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}MV_Public${MV_LINK_SUFFIX}")
set(POINTDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}PointData${MV_LINK_SUFFIX}") 
set(IMAGEDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ImageData${MV_LINK_SUFFIX}") 
if (MSVC)
    set(zeromq_LIBS_DEBUG "libzmq-v142-mt-4_3_5") # not the libzmq-v142-mt-gd-4_3_5 version
endif()

target_link_libraries(${JUPYTERPLUGIN} PRIVATE "${MV_LINK_LIBRARY}")
target_link_libraries(${JUPYTERPLUGIN} PRIVATE "${POINTDATA_LINK_LIBRARY}")
target_link_libraries(${JUPYTERPLUGIN} PRIVATE "${IMAGEDATA_LINK_LIBRARY}")
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-zmq-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-python-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE "${PYTHON_LIBRARY}")
target_link_libraries(${JUPYTERPLUGIN} PRIVATE pybind11::embed)

if (MSVC)
    target_compile_options(${JUPYTERPLUGIN} PUBLIC /wd4251 /wd4141)
    target_compile_options(${JUPYTERPLUGIN} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
endif()
target_compile_features(${JUPYTERPLUGIN} PRIVATE cxx_std_14)

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
# Install the shared plugin libary to the "Plugins" folder in the ManiVault install directory
install(TARGETS ${JUPYTERPLUGIN}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
    ARCHIVE DESTINATION lib     COMPONENT LINKLIB # Windows .lib
)

add_custom_command(TARGET ${JUPYTERPLUGIN} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix ${MV_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${JUPYTERPLUGIN} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${MV_INSTALL_DIR}/debug,${MV_INSTALL_DIR}/release>)
    set_property(TARGET ${JUPYTERPLUGIN} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${MV_INSTALL_DIR}/debug/ManiVault Studio.exe","${MV_INSTALL_DIR}/release/ManiVault Studio.exe">)
endif()

