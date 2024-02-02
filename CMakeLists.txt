cmake_minimum_required(VERSION 3.21)

# -----------------------------------------------------------------------------
# Jupyter Plugin
# -----------------------------------------------------------------------------
set(JUPYTERPLUGIN "JupyterPlugin")
PROJECT(${JUPYTERPLUGIN})

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(QTVERSION 6)
add_subdirectory(external/json)
add_subdirectory(external/xtl)
add_subdirectory(external/libzmq)
add_subdirectory(external/cppzmq)
find_package(Python 3.11 COMPONENTS Development Interpreter REQUIRED)
add_subdirectory(external/pybind11)
set(pybind11_ROOT "C:/Users/bvanlew/.conan/data/pybind11/2.11.1/_/_/package/5ab84d6acfe1f23c4fae0ab88f26e3a396351ac9")


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

set(xtl_DIR "${CMAKE_BINARY_DIR}/external/xtl" CACHE PATH "")
set(nlohmann_json_DIR "${CMAKE_BINARY_DIR}/external/json" CACHE PATH "")
set(cppzmq_DIR "${CMAKE_BINARY_DIR}/external/cppzmq" CACHE PATH "")
set(ZeroMQ_DIR ${CMAKE_BINARY_DIR}/external/libzmq CACHE PATH "")

find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
set(pybind11_INCLUDE_DIR "./external/pybind11/install")
set(xeus_ROOT "" CACHE PATH "path to the xeus dependency root")
find_package(xeus REQUIRED)
set(xeus-zmq_ROOT "" CACHE PATH "path to the xeus-zmq dependency root")
find_package(xeus-zmq REQUIRED)

set(xeus-python_ROOT "" CACHE PATH "path to the xeus-python dependency root")
find_package(xeus-python REQUIRED)


macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

print_all_variables()
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
    src/JupyterPlugin.json
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
    "${pybind11_INCLUDE_DIR}"
    "${PYTHON_INCLUDE_DIR}"
)

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
# Request C++17
target_compile_features(${JUPYTERPLUGIN} PRIVATE cxx_std_17)

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
if (MSVC)
    set(zeromq_LIBS_DEBUG "libzmq-v142-mt-4_3_5") # not the libzmq-v142-mt-gd-4_3_5 version
endif()

target_link_libraries(${JUPYTERPLUGIN} PRIVATE "${MV_LINK_LIBRARY}")
target_link_libraries(${JUPYTERPLUGIN} PRIVATE "${POINTDATA_LINK_LIBRARY}")
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-zmq)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-python)
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
