cmake_minimum_required(VERSION 3.21)

set(JUPYTERPLUGIN "JupyterPlugin")
message(STATUS "Plugin: ${JUPYTERPLUGIN}")

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

set(JUPYTERPLUGIN_SOURCES
    JupyterPlugin.h
    JupyterPlugin.cpp
    XeusKernel.h
    XeusKernel.cpp
    XeusPoller.h
    XeusPoller.cpp
    XeusServer.h
    XeusServer.cpp
    XeusInterpreter.h
    XeusInterpreter.cpp
    MVData.cpp
    MVData.h
    JupyterPlugin.json
    ${CMAKE_CURRENT_BINARY_DIR}/jupyterplugin_export.h
)

source_group(Plugin FILES ${JUPYTERPLUGIN_SOURCES})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
# Create dynamic library for the plugin
add_library(${JUPYTERPLUGIN} SHARED ${JUPYTERPLUGIN_SOURCES})

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
target_link_libraries(${JUPYTERPLUGIN} PUBLIC xtl)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC libzmq-static cppzmq-static)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC pybind11::embed)
target_link_libraries(${JUPYTERPLUGIN} PUBLIC nlohmann_json::nlohmann_json)

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
    message(STATUS "Building the mvstudio_data python package in ${PROJECT_SOURCE_DIR}")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry version ${CMAKE_PROJECT_VERSION}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/python/mvstudio_data
        COMMAND_ERROR_IS_FATAL ANY)    
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/python/mvstudio_data
        COMMAND_ERROR_IS_FATAL ANY)
    message(STATUS "Building the mvstudio_data python package in ${PROJECT_SOURCE_DIR}")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry version ${CMAKE_PROJECT_VERSION}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/python/mvstudio_kernel
        COMMAND_ERROR_IS_FATAL ANY)  
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/python/mvstudio_kernel
        COMMAND_ERROR_IS_FATAL ANY)
endif ()


# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
# Install the shared plugin libary to the "Plugins" folder in the ManiVault install directory
install(TARGETS ${JUPYTERPLUGIN}
    RUNTIME DESTINATION PluginDependencies/JupyterLauncher/bin COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION PluginDependencies/JupyterLauncher/bin COMPONENT PLUGINS # Linux/Mac .so
    ARCHIVE DESTINATION lib                                    COMPONENT LINKLIB # Windows .lib 
)

install(FILES 
    ${PROJECT_SOURCE_DIR}/jupyter_server_config.py 
    ${PROJECT_SOURCE_DIR}/python/mvstudio_data/dist/mvstudio_data-${CMAKE_PROJECT_VERSION}-py3-none-any.whl
    ${PROJECT_SOURCE_DIR}/python/mvstudio_kernel/dist/mvstudio_kernel-${CMAKE_PROJECT_VERSION}-py3-none-any.whl
    DESTINATION PluginDependencies/JupyterLauncher/py 
    COMPONENT PLUGINS
)

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

