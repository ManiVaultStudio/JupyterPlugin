cmake_minimum_required(VERSION 3.17)

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

set(SOURCES
    src/JupyterLauncher.h
    src/JupyterLauncher.cpp
    src/JupyterLauncher.json
    src/GlobalSettingsAction.h
    src/GlobalSettingsAction.cpp
    src/SettingsAction.h
    src/SettingsAction.cpp
)

set(PLUGIN_MOC_HEADERS
    src/JupyterLauncher.h
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

qt_wrap_cpp(JUPYTERLAUNCHER_MOC ${PLUGIN_MOC_HEADERS} TARGET ${JUPYTERLAUNCHER})
target_sources(${JUPYTERLAUNCHER} PRIVATE ${JUPYTERLAUNCHER_MOC})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${JUPYTERLAUNCHER} PRIVATE 
    "${MV_INSTALL_DIR}/$<CONFIGURATION>/include/"
)

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${JUPYTERLAUNCHER} PRIVATE cxx_std_17)


include(GenerateExportHeader)
generate_export_header(${JUPYTERLAUNCHER})

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
set(MV_LINK_PATH "${MV_INSTALL_DIR}/$<CONFIGURATION>/lib")
set(PLUGIN_LINK_PATH "${MV_INSTALL_DIR}/$<CONFIGURATION>/$<IF:$<CXX_COMPILER_ID:MSVC>,lib,Plugins>")
set(MV_LINK_SUFFIX $<IF:$<CXX_COMPILER_ID:MSVC>,${CMAKE_LINK_LIBRARY_SUFFIX},${CMAKE_SHARED_LIBRARY_SUFFIX}>)

set(MV_LINK_LIBRARY "${MV_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}MV_Public${MV_LINK_SUFFIX}")
set(POINTDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}PointData${MV_LINK_SUFFIX}") 

target_link_libraries(${JUPYTERLAUNCHER} PRIVATE Qt6::Widgets)
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE "${MV_LINK_LIBRARY}")
target_link_libraries(${JUPYTERLAUNCHER} PRIVATE "${POINTDATA_LINK_LIBRARY}")

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
        --prefix ${MV_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${JUPYTERLAUNCHER} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY  $<IF:$<CONFIG:DEBUG>,${MV_INSTALL_DIR}/debug,${MV_INSTALL_DIR}/release>)
    set_property(TARGET ${JUPYTERLAUNCHER} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${MV_INSTALL_DIR}/debug/ManiVault Studio.exe","${MV_INSTALL_DIR}/release/ManiVault Studio.exe">)
endif()