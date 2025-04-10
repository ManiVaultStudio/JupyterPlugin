cmake_minimum_required(VERSION 3.22)

set(JUPYTERPLUGIN "JupyterPlugin${Python_VERSION_MAJOR}${Python_VERSION_MINOR}")
set(JupyterPluginTarget "${JUPYTERPLUGIN}" PARENT_SCOPE) # for add_dependencies in parent scope
message(STATUS "Plugin: ${JUPYTERPLUGIN}")

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

# create a file that defines variables for the python version that this plugin was built against
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/PythonBuildVersion.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/PythonBuildVersion.h" @ONLY)

set(JUPYTERPLUGIN_SOURCES
    JupyterPlugin.h
    JupyterPlugin.cpp
    XeusKernel.h
    XeusKernel.cpp
    XeusServer.h
    XeusServer.cpp
    XeusInterpreter.h
    XeusInterpreter.cpp
    MVData.cpp
    MVData.h
    PythonBuildVersion.h
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
target_include_directories(${JUPYTERPLUGIN} PRIVATE "${ManiVault_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
# Request C++20
target_compile_features(${JUPYTERPLUGIN} PRIVATE cxx_std_20)

set_target_properties(${JUPYTERPLUGIN} 
    PROPERTIES 
    FOLDER AnalysisPlugins
)

if(UNIX)
    set_target_properties(${JUPYTERPLUGIN} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
# Link to Qt libraries
target_link_libraries(${JUPYTERPLUGIN} PRIVATE Qt6::Widgets)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE Qt6::WebEngineWidgets)

target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::Core)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::PointData)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::ImageData)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE ManiVault::ClusterData)

target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-zmq-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE xeus-python-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE libzmq-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE cppzmq-static)
target_link_libraries(${JUPYTERPLUGIN} PRIVATE pybind11::pybind11)    # Python headers + pybind11::headers, see https://pybind11.readthedocs.io/en/stable/compiling.html#advanced-interface-library-targets
target_link_libraries(${JUPYTERPLUGIN} PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(${JUPYTERPLUGIN} PRIVATE Python::Python)

if(UNIX)
    target_link_libraries(${JUPYTERPLUGIN} PRIVATE pthread dl util m)     # see https://docs.python.org/3/extending/embedding.html
endif()

# -----------------------------------------------------------------------------
# Python MVJupyterPluginManager packages
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
        message(STATUS "'poetry' successfully installed")
    endif()
endif()

add_custom_target(mvstudio_kernel 
    COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/mvstudio_kernel
)

target_sources(mvstudio_kernel PRIVATE
    ../mvstudio_kernel/mvstudio/kernel/__init__.py
    ../mvstudio_kernel/mvstudio/kernel/manager.py
    ../mvstudio_kernel/mvstudio/kernel/provisioning.py
)

add_custom_target(mvstudio_data 
    COMMAND ${Python_EXECUTABLE} -m poetry build --no-interaction --format=wheel
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/mvstudio_data
)

target_sources(mvstudio_data PRIVATE
    ../mvstudio_data/mvstudio/data/__init__.py
    ../mvstudio_data/mvstudio/data/cluster.py
    ../mvstudio_data/mvstudio/data/clusterbase.py
    ../mvstudio_data/mvstudio/data/factory.py
    ../mvstudio_data/mvstudio/data/hierarchy.py
    ../mvstudio_data/mvstudio/data/image.py
    ../mvstudio_data/mvstudio/data/item.py
)

add_dependencies(${JUPYTERPLUGIN} mvstudio_kernel mvstudio_data)

set_target_properties(mvstudio_kernel mvstudio_data xeus-static xeus-zmq-static xeus-python-static libzmq-static
    PROPERTIES 
    FOLDER AnalysisPlugins/Dependencies/JupyterPluginDependencies
)

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
# Install the shared libary to the "PluginDependencies" folder in the ManiVault install directory
install(TARGETS ${JUPYTERPLUGIN}
    RUNTIME DESTINATION PluginDependencies/JupyterLauncher/bin COMPONENT PLUGIN # Windows .dll
    LIBRARY DESTINATION PluginDependencies/JupyterLauncher/bin COMPONENT PLUGIN # Linux/Mac .so
)

install(FILES 
    res/jupyter_server_config.py 
    ${PROJECT_SOURCE_DIR}/src/mvstudio_data/dist/mvstudio_data-${PROJECT_VERSION}-py3-none-any.whl 
    ${PROJECT_SOURCE_DIR}/src/mvstudio_kernel/dist/mvstudio_kernel-${PROJECT_VERSION}-py3-none-any.whl 
    DESTINATION PluginDependencies/JupyterLauncher/py 
    COMPONENT PYTHONWHEELS
)

install(FILES 
    ${PROJECT_SOURCE_DIR}/examples/demo_data_loading.ipynb
    ${PROJECT_SOURCE_DIR}/examples/save_load_clusters.ipynb
    ${PROJECT_SOURCE_DIR}/examples/count_cells_per_cluster.ipynb
    ${PROJECT_SOURCE_DIR}/examples/requirements.txt
    DESTINATION examples/JupyterPlugin 
    COMPONENT PLUGIN
)

add_custom_command(TARGET ${JUPYTERPLUGIN} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
    --install ${CMAKE_CURRENT_BINARY_DIR}
    --config $<CONFIGURATION>
    --component PLUGIN
    --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

add_custom_command(TARGET ${JUPYTERPLUGIN} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
    --install ${CMAKE_CURRENT_BINARY_DIR}
    --config $<CONFIGURATION>
    --component PYTHONWHEELS
    --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET ${JUPYTERPLUGIN} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/Debug,$<IF:$<CONFIG:RELWITHDEBINFO>,${ManiVault_INSTALL_DIR}/RelWithDebInfo,${ManiVault_INSTALL_DIR}/Release>>)
    set_property(TARGET ${JUPYTERPLUGIN} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/Debug/ManiVault Studio.exe",$<IF:$<CONFIG:RELWITHDEBINFO>,"${ManiVault_INSTALL_DIR}/RelWithDebInfo/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/Release/ManiVault Studio.exe">>)
endif()
