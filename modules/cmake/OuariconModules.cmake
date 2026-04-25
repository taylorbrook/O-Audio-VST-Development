# ==============================================================================
#
#   OuariconModules.cmake
#   CMake integration for the Ouaricon Module System
#
#   Usage in plugin CMakeLists.txt:
#     include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
#     ouaricon_add_module(MyPlugin preset-manager CONFIG preset-config.yaml)
#
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

# Module system paths
set(OUARICON_MODULES_DIR "${CMAKE_SOURCE_DIR}/modules" CACHE PATH "Root directory for Ouaricon modules")

# ==============================================================================
# ouaricon_add_module
#
# Adds a module to a plugin target. Handles:
# - Adding C++ source files to target
# - Copying JS files to ui/public/modules/
# - Processing module configuration
#
# Arguments:
#   TARGET_NAME - The JUCE plugin target name
#   MODULE_NAME - The module name (e.g., "preset-manager")
#   CONFIG      - Optional path to module config YAML
# ==============================================================================
function(ouaricon_add_module TARGET_NAME MODULE_NAME)
    cmake_parse_arguments(ARG "" "CONFIG" "" ${ARGN})

    # Find module directory by searching categories
    set(MODULE_CATEGORIES core persistence metering tuning modulation synthesis effects ui)
    set(MODULE_DIR "")

    foreach(CATEGORY ${MODULE_CATEGORIES})
        set(CANDIDATE_DIR "${OUARICON_MODULES_DIR}/${CATEGORY}/${MODULE_NAME}")
        if(EXISTS "${CANDIDATE_DIR}/module.yaml")
            set(MODULE_DIR "${CANDIDATE_DIR}")
            break()
        endif()
    endforeach()

    if("${MODULE_DIR}" STREQUAL "")
        message(FATAL_ERROR "Module '${MODULE_NAME}' not found in any category. "
                           "Searched: ${MODULE_CATEGORIES}")
    endif()

    message(STATUS "[Ouaricon] Adding module '${MODULE_NAME}' from ${MODULE_DIR}")

    # Add C++ sources to target
    if(EXISTS "${MODULE_DIR}/cpp")
        file(GLOB_RECURSE MODULE_CPP_SOURCES
            "${MODULE_DIR}/cpp/*.cpp"
            "${MODULE_DIR}/cpp/*.h"
        )

        if(MODULE_CPP_SOURCES)
            target_sources(${TARGET_NAME} PRIVATE ${MODULE_CPP_SOURCES})
            target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")
            message(STATUS "[Ouaricon]   Added ${CMAKE_CURRENT_LIST_DIR} C++ sources")
        endif()
    endif()

    # Copy JS files to ui/public/modules/
    if(EXISTS "${MODULE_DIR}/js")
        file(GLOB MODULE_JS_FILES "${MODULE_DIR}/js/*.js")

        if(MODULE_JS_FILES)
            set(UI_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/modules")
            file(MAKE_DIRECTORY "${UI_MODULES_DIR}")

            foreach(JS_FILE ${MODULE_JS_FILES})
                get_filename_component(JS_FILENAME ${JS_FILE} NAME)
                configure_file(${JS_FILE} "${UI_MODULES_DIR}/${JS_FILENAME}" COPYONLY)
                message(STATUS "[Ouaricon]   Copied ${JS_FILENAME} to ui/public/modules/")
            endforeach()
        endif()
    endif()

    # Module-supplied CMake hook (optional, backward-compatible)
    if(EXISTS "${MODULE_DIR}/module.cmake")
        message(STATUS "[Ouaricon]   Including ${MODULE_NAME}/module.cmake")
        include("${MODULE_DIR}/module.cmake")
    endif()

    # Process configuration if provided
    if(ARG_CONFIG)
        if(NOT EXISTS "${ARG_CONFIG}")
            # Try relative to current source dir
            set(ARG_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_CONFIG}")
        endif()

        if(EXISTS "${ARG_CONFIG}")
            message(STATUS "[Ouaricon]   Using config: ${ARG_CONFIG}")
            # Config processing would generate header here
            # For now, we just note it
        else()
            message(WARNING "[Ouaricon] Config file not found: ${ARG_CONFIG}")
        endif()
    endif()

    # Set a variable indicating this module is included
    set(OUARICON_MODULE_${MODULE_NAME}_INCLUDED TRUE PARENT_SCOPE)

endfunction()

# ==============================================================================
# ouaricon_list_modules
#
# Lists all available modules (useful for debugging)
# ==============================================================================
function(ouaricon_list_modules)
    message(STATUS "[Ouaricon] Available modules:")

    set(MODULE_CATEGORIES core persistence metering tuning modulation synthesis effects ui)

    foreach(CATEGORY ${MODULE_CATEGORIES})
        set(CATEGORY_DIR "${OUARICON_MODULES_DIR}/${CATEGORY}")
        if(EXISTS "${CATEGORY_DIR}")
            file(GLOB MODULE_DIRS "${CATEGORY_DIR}/*")
            foreach(MODULE_DIR ${MODULE_DIRS})
                if(EXISTS "${MODULE_DIR}/module.yaml")
                    get_filename_component(MODULE_NAME ${MODULE_DIR} NAME)
                    message(STATUS "  - ${CATEGORY}/${MODULE_NAME}")
                endif()
            endforeach()
        endif()
    endforeach()
endfunction()

# ==============================================================================
# ouaricon_check_module_updates
#
# Checks if any modules have updates available
# Compares modules.lock.yaml in plugin dir with registry versions
# ==============================================================================
function(ouaricon_check_module_updates PLUGIN_DIR)
    set(LOCK_FILE "${PLUGIN_DIR}/modules.lock.yaml")

    if(NOT EXISTS "${LOCK_FILE}")
        message(STATUS "[Ouaricon] No modules.lock.yaml found - no updates to check")
        return()
    endif()

    message(STATUS "[Ouaricon] Checking for module updates...")
    # Would parse YAML and compare versions here
    # For now, just note the functionality
endfunction()
