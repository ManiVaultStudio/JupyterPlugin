cmake_minimum_required(VERSION 3.21)

set(JUPYTERPLUGIN "JupyterPlugin${Python_VERSION_MAJOR}${Python_VERSION_MINOR}")
set(JupyterPluginTarget "${JUPYTERPLUGIN}" PARENT_SCOPE)
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

set_target_properties(${JUPYTERPLUGIN} PROPERTIES FOLDER JupyterPlugin)

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
execute_process(
    COMMAND ${Python_EXECUTABLE} -m pip show poetry
    RESULT_VARIABLE EXIT_CODE
    OUTPUT_QUIET)

if (NOT ${EXIT_CODE} EQUAL 0)
    message(STATUS "Cannot find 'poetry' in python environment. 'poetry' is required for building the JupyterPlugin support package")
    message(STATUS "Attempting to to install python poetry package...")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m pip install -r requirements.txt
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE POETRY_INSTALL_RESULT
        OUTPUT_QUIET)
    if (POETRY_INSTALL_RESULT)
        message(FATAL_ERROR "Install poetry failed. Try 'pip install poetry'.")
    else()
        message(STATUS "'poetry' succesfully installed")
    endif()
endif()

message(STATUS "Building the mvstudio_kernel mvstudio_data python packages")
execute_process(
    COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/mvstudio_kernel
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(
    COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/mvstudio_data
    COMMAND_ERROR_IS_FATAL ANY)

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
    res/jupyter_server_config.py 
    ${PROJECT_SOURCE_DIR}/src/mvstudio_data/dist/mvstudio_data-${PROJECT_VERSION}-py3-none-any.whl 
    ${PROJECT_SOURCE_DIR}/src/mvstudio_kernel/dist/mvstudio_kernel-${PROJECT_VERSION}-py3-none-any.whl 
    DESTINATION PluginDependencies/JupyterLauncher/py 
    COMPONENT PLUGINS
)

install(FILES 
    ${PROJECT_SOURCE_DIR}/examples/demo_data_loading.ipynb
    DESTINATION examples/JupyterPlugin 
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
