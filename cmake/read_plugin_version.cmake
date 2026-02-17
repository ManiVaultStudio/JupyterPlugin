# -----------------------------------------------------------------------------
# Read plugin version from plugin meta data
# -----------------------------------------------------------------------------
# Populates the three variables handed to the function

# Usage:
#  read_plugin_version(out_major out_minor out_patch)

function(read_plugin_version plugin_config_file out_major out_minor out_patch)
    
    # Read config file
    if(NOT EXISTS "${plugin_config_file}")
        message(FATAL_ERROR "JSON file not found: ${plugin_config_file}")
    endif()

    file(READ ${plugin_config_file} PLUGIN_INFO_JSON)

    # Check if config file contains certain entries
    set(HAS_PLUGIN_TYPE 0)

    # Get the number of top-level keys
    string(JSON TOP_LEVEL_COUNT LENGTH "${PLUGIN_INFO_JSON}")

    math(EXPR LAST_INDEX "${TOP_LEVEL_COUNT} - 1")
    foreach(I RANGE 0 ${LAST_INDEX})
        string(JSON KEY_NAME MEMBER "${PLUGIN_INFO_JSON}" ${I})
        if("${KEY_NAME}" STREQUAL "type")
            set(HAS_PLUGIN_TYPE 1)
        endif()
    endforeach()

    # Extract plugin version
    string(JSON PLUGIN_VERSION GET "${PLUGIN_INFO_JSON}" version plugin)
    set(${output_var} "${result}" PARENT_SCOPE)
    
    string(REPLACE "." ";" parts "${PLUGIN_VERSION}")

    list(GET parts 0 major)
    list(GET parts 1 minor)
    list(GET parts 2 patch)

    set(${out_major} "${major}" PARENT_SCOPE)
    set(${out_minor} "${minor}" PARENT_SCOPE)
    set(${out_patch} "${patch}" PARENT_SCOPE)

endfunction()