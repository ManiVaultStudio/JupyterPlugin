cmake_minimum_required(VERSION 3.22)

set(JUPYTERLAUNCHER "JupyterLauncher")
message(STATUS "Plugin: ${JUPYTERLAUNCHER}")

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

qt6_add_resources(JUPYTERLAUNCHER_RESOURCE_FILES res/jupyter_launcher_resources.qrc)

set(JUPYTERLAUNCHER_SOURCES
    JupyterLauncher.h
    JupyterLauncher.cpp
    PluginInfo.json
    GlobalSettingsAction.h
    GlobalSettingsAction.cpp
    UserDialogActions.h
    UserDialogActions.cpp
    ScriptDialogAction.h
    ScriptDialogAction.cpp
    Utils.h
    Utils.cpp
)

set(JUPYTERLAUNCHER_AUX
    PluginInfo.json
)

set(JUPYTERLAUNCHER_RESOURCES
    res/jupyter_launcher_resources.qrc
)

source_group( Plugin FILES ${JUPYTERLAUNCHER_SOURCES} ${JUPYTERLAUNCHER_AUX})
source_group( ResourceFiles FILES ${JUPYTERLAUNCHER_RESOURCES})
source_group( ResourceFiles\\generated FILES ${CMAKE_CURRENT_BINARY_DIR}/qrc_jupyter_launcher_resources.cpp)

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_library(${JUPYTERLAUNCHER} SHARED ${JUPYTERLAUNCHER_SOURCES} ${JUPYTERLAUNCHER_AUX} ${JUPYTERLAUNCHER_RESOURCES} ${JUPYTERLAUNCHER_RESOURCE_FILES})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${JUPYTERLAUNCHER} PRIVATE "${ManiVault_INCLUDE_DIR}")

target_include_directories(${JUPYTERLAUNCHER} PRIVATE ${md4qt_SOURCE_DIR})

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${JUPYTERLAUNCHER} PRIVATE cxx_std_20)

set_target_properties(${JUPYTERLAUNCHER} 
    PROPERTIES 
    FOLDER AnalysisPlugins
)

if(UNIX)
    set_target_properties(${JUPYTERLAUNCHER} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE Qt6::Widgets)
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE Qt6::WebEngineWidgets)

target_link_libraries(${JUPYTERLAUNCHER} PRIVATE ManiVault::Core)

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
install(TARGETS ${JUPYTERLAUNCHER}
    RUNTIME DESTINATION Plugins COMPONENT PLUGIN # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGIN # Linux/Mac .so
)

add_custom_command(TARGET ${JUPYTERLAUNCHER} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --component PLUGIN
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

mv_handle_plugin_config(${JUPYTERLAUNCHER} 0)

install(FILES 
    ${PROJECT_SOURCE_DIR}/tutorials/test.json
    ${PROJECT_SOURCE_DIR}/tutorials/test.md
    DESTINATION tutorials/JupyterLauncher 
    COMPONENT PYTHONTUTORIALS
)

add_custom_command(TARGET ${JUPYTERLAUNCHER} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --component PYTHONTUTORIALS
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET ${JUPYTERLAUNCHER} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/Debug,$<IF:$<CONFIG:RELWITHDEBINFO>,${ManiVault_INSTALL_DIR}/RelWithDebInfo,${ManiVault_INSTALL_DIR}/Release>>)
    set_property(TARGET ${JUPYTERLAUNCHER} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/Debug/ManiVault Studio.exe",$<IF:$<CONFIG:RELWITHDEBINFO>,"${ManiVault_INSTALL_DIR}/RelWithDebInfo/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/Release/ManiVault Studio.exe">>)
endif()
