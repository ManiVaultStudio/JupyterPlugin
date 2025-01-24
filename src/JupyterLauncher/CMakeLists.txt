cmake_minimum_required(VERSION 3.21)

set(JUPYTERLAUNCHER "JupyterLauncher")
message(STATUS "Plugin: ${JUPYTERLAUNCHER}")

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

qt6_add_resources(RESOURCE_FILES res/jupyter_launcher_resources.qrc)

set(SOURCES
    JupyterLauncher.h
    JupyterLauncher.cpp
    JupyterLauncher.json
    GlobalSettingsAction.h
    GlobalSettingsAction.cpp
    UserDialogActions.h
    UserDialogActions.cpp
)

set (RESOURCES
    res/jupyter_launcher_resources.qrc
)

source_group( Plugin FILES ${SOURCES})
source_group( ResourceFiles FILES ${RESOURCES})
source_group( ResourceFiles\\generated FILES ${CMAKE_CURRENT_BINARY_DIR}/qrc_jupyter_launcher_resources.cpp)

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_library(${JUPYTERLAUNCHER} SHARED ${SOURCES} ${RESOURCES} ${RESOURCE_FILES})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${JUPYTERLAUNCHER} PRIVATE "${ManiVault_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${JUPYTERLAUNCHER} PRIVATE cxx_std_17)

set_target_properties(${JUPYTERLAUNCHER} PROPERTIES FOLDER JupyterPlugin)

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
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
)

add_custom_command(TARGET ${JUPYTERLAUNCHER} POST_BUILD
    DEPENDS JupyterLauncher
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${JUPYTERLAUNCHER} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY  $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/debug,${ManiVault_INSTALL_DIR}/release>)
    set_property(TARGET ${JUPYTERLAUNCHER} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/debug/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/release/ManiVault Studio.exe">)
endif()
