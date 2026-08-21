cmake_minimum_required(VERSION 3.31)

set(JUPYTERPLUGINTEST "JupyterPluginTest")
message(STATUS "Tests: ${JUPYTERPLUGINTEST}")

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

set(JUPYTERPLUGIN_TEST_SOURCES
    JupyterPluginTest.cpp
)

set(JUPYTERPLUGIN_TEST_AUX
    ${PROJECT_SOURCE_DIR}/src/JupyterLauncher/PythonUtils.cpp
)

source_group(Tests FILES ${JUPYTERPLUGIN_TEST_SOURCES})
source_group(Aux FILES ${JUPYTERPLUGIN_TEST_AUX})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_executable(${JUPYTERPLUGINTEST} EXCLUDE_FROM_ALL ${JUPYTERPLUGIN_TEST_SOURCES} ${JUPYTERPLUGIN_TEST_AUX})

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
# Request C++20
target_compile_features(${JUPYTERPLUGINTEST} PRIVATE cxx_std_20)

if(UNIX)
    set_target_properties(${JUPYTERPLUGINTEST} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${JUPYTERPLUGINTEST} PRIVATE "${PROJECT_SOURCE_DIR}/src/JupyterLauncher")

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${JUPYTERPLUGINTEST} PRIVATE Qt6::Core)

target_link_libraries(${JUPYTERPLUGINTEST} PRIVATE pybind11::pybind11)
target_link_libraries(${JUPYTERPLUGINTEST} PRIVATE Python::Python)

if(UNIX)
    target_link_libraries(${JUPYTERPLUGINTEST} PRIVATE pthread dl util m)     # see https://docs.python.org/3/extending/embedding.html
endif()
