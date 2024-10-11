cmake_minimum_required(VERSION 3.21)

# -----------------------------------------------------------------------------
# BinExporter Plugin
# -----------------------------------------------------------------------------
set(JUPYTERLAUNCHER "JupyterLauncher")
PROJECT(${JUPYTERLAUNCHER})

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

qt6_add_resources(RESOURCE_FILES res/jupyter_launcher_resources.qrc)

# uses MVSTUDIO_VERSION
set(MVSTUDIO_VERSIO_IN src/mvstudio_version.h.in)
set(MVSTUDIO_VERSIO_H src/mvstudio_version.h.in)
configure_file(${MVSTUDIO_VERSIO_IN} ${MVSTUDIO_VERSIO_H} @ONLY)

set(SOURCES
    src/JupyterLauncher.h
    src/JupyterLauncher.cpp
    src/JupyterLauncher.json
    src/GlobalSettingsAction.h
    src/GlobalSettingsAction.cpp
    src/SettingsAction.h
    src/SettingsAction.cpp
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
target_include_directories(${JUPYTERLAUNCHER} PUBLIC "${MVSTUDIO_VERSIO_H}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${JUPYTERLAUNCHER} PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(${JUPYTERLAUNCHER})

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE Qt6::Widgets)
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE ManiVault::Core)
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE ManiVault::PointData)

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
install(TARGETS ${JUPYTERLAUNCHER}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
    ARCHIVE DESTINATION lib     COMPONENT LINKLIB # Windows .lib
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