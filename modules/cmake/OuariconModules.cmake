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

    # Add C++ sources to target — SharedCode glob (top-level cpp/ ONLY).
    # Per-format subdirectories (cpp/vst3/, cpp/au/, etc.) are handled in the
    # per-format routing block below and MUST NOT be swept into SharedCode,
    # otherwise format-specific symbol references leak into the shared static
    # library and break non-format link lines (Plan 23-05 D-23-04-A).
    if(EXISTS "${MODULE_DIR}/cpp")
        file(GLOB MODULE_CPP_SOURCES
            "${MODULE_DIR}/cpp/*.cpp"
            "${MODULE_DIR}/cpp/*.h"
        )

        if(MODULE_CPP_SOURCES)
            target_sources(${TARGET_NAME} PRIVATE ${MODULE_CPP_SOURCES})
            target_include_directories(${TARGET_NAME} PRIVATE "${MODULE_DIR}/cpp")
            message(STATUS "[Ouaricon]   Added ${MODULE_NAME} SharedCode sources to ${TARGET_NAME}")
        endif()

        # Per-format routing (Plan 23-05 D-24..D-28): files placed under
        # cpp/<format>/ are added to ${TARGET_NAME}_<FormatSuffix> only, with
        # PRIVATE include directories so format-specific headers cannot be pulled
        # in by SharedCode or other-format translation units. Silently no-ops
        # when ${TARGET_NAME}_<FormatSuffix> does not exist (e.g. plugin excludes
        # that format from its FORMATS list).
        #
        # NOTE: JUCE uses mixed-case target suffixes for several formats
        # (Standalone, AAX, LV2, AUv3, Unity) — see
        # JUCE/extras/Build/CMake/JUCEUtils.cmake. We keep lowercase directory
        # names as the on-disk convention but map to JUCE's exact target suffix
        # via parallel lists. Using string(TOUPPER ...) here would produce
        # nonexistent targets like ${TARGET_NAME}_STANDALONE and silently no-op.
        set(_OUA_FMT_DIRS    vst3 au standalone vst2 aax lv2 auv3 unity)
        set(_OUA_FMT_TARGETS VST3 AU Standalone VST2 AAX LV2 AUv3 Unity)
        list(LENGTH _OUA_FMT_DIRS _OUA_FMT_COUNT)
        math(EXPR _OUA_FMT_LAST "${_OUA_FMT_COUNT} - 1")
        foreach(_idx RANGE 0 ${_OUA_FMT_LAST})
            list(GET _OUA_FMT_DIRS    ${_idx} _FMT_DIR_NAME)
            list(GET _OUA_FMT_TARGETS ${_idx} _FMT_TARGET_SUFFIX)
            set(_FMT_DIR "${MODULE_DIR}/cpp/${_FMT_DIR_NAME}")
            if(EXISTS "${_FMT_DIR}" AND TARGET "${TARGET_NAME}_${_FMT_TARGET_SUFFIX}")
                file(GLOB_RECURSE _FMT_SOURCES
                    "${_FMT_DIR}/*.cpp"
                    "${_FMT_DIR}/*.h"
                )
                if(_FMT_SOURCES)
                    target_sources(${TARGET_NAME}_${_FMT_TARGET_SUFFIX} PRIVATE ${_FMT_SOURCES})
                    target_include_directories(${TARGET_NAME}_${_FMT_TARGET_SUFFIX} PRIVATE "${_FMT_DIR}")
                    message(STATUS "[Ouaricon]   Added ${MODULE_NAME}/cpp/${_FMT_DIR_NAME} sources to ${TARGET_NAME}_${_FMT_TARGET_SUFFIX}")
                endif()
            endif()
        endforeach()
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

# ==============================================================================
# ouaricon_extract_vst3_cids(OUTPUT_VAR <var> PLUGINS <list>)
#
# Reads each built VST3 bundle's Contents/Resources/moduleinfo.json (emitted by
# JUCE 8) and extracts the canonical 32-hex Audio Module Class CID. Sets
# per-plugin <NAME>_PLUGINID variables in PARENT_SCOPE for use with
# configure_file @ONLY substitution in module.cmake.
#
# Honors ${OUARICON_DEV_SUFFIX} so dev installers ship dev CIDs and prod
# installers ship prod CIDs (mitigates Pitfall 2 — see Phase 25 RESEARCH.md).
#
# Fails loud + fails fast at configure time if any plugin's VST3 bundle is
# not yet built (mirrors ouaricon_add_module's FATAL_ERROR pattern).
#
# Variable suffix derivation: target name is upper-cased and "-" stripped, e.g.
# "O-Lyrica" -> "OLYRICA_PLUGINID", "O-IntonationPad" -> "OINTONATIONPAD_PLUGINID".
# This matches the @<NAME>_PLUGINID@ tokens in endpointconfig.xml.in.
# ==============================================================================
function(ouaricon_extract_vst3_cids)
    set(options)
    set(oneValueArgs OUTPUT_VAR)
    set(multiValueArgs PLUGINS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(plugin_target IN LISTS ARG_PLUGINS)
        # Honor OUARICON_DEV_SUFFIX (S-3) — dev builds emit "-dev" suffixed
        # bundles whose moduleinfo.json carries dev CIDs; prod builds emit
        # un-suffixed bundles with prod CIDs.
        set(moduleinfo "${CMAKE_BINARY_DIR}/plugins/${plugin_target}/${plugin_target}_artefacts/Release/VST3/${plugin_target}${OUARICON_DEV_SUFFIX}.vst3/Contents/Resources/moduleinfo.json")

        if(NOT EXISTS "${moduleinfo}")
            message(FATAL_ERROR
                "[Ouaricon] CID extraction: ${plugin_target} VST3 not built yet — "
                "build all _VST3 targets before packaging the Microtonal Suite.\n"
                "Expected: ${moduleinfo}")
        endif()

        # JUCE 8 emits moduleinfo.json with trailing commas — strip them via
        # python3 regex, then parse and pull the Audio Module Class CID.
        execute_process(
            COMMAND python3 -c "
import json, sys, re
with open(sys.argv[1]) as f:
    raw = f.read()
raw = re.sub(r',(\\s*[}\\]])', r'\\1', raw)
data = json.loads(raw)
for cls in data['Classes']:
    if cls['Category'] == 'Audio Module Class':
        print(cls['CID'])
        sys.exit(0)
sys.exit(1)
" "${moduleinfo}"
            OUTPUT_VARIABLE cid
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )

        # Variable suffix: "O-IntonationPad" -> "OINTONATIONPAD_PLUGINID"
        string(TOUPPER "${plugin_target}" var_name)
        string(REPLACE "-" "" var_name "${var_name}")
        set("${var_name}_PLUGINID" "${cid}" PARENT_SCOPE)
        message(STATUS "[Ouaricon] ${plugin_target} pluginID = ${cid}")
    endforeach()

    if(ARG_OUTPUT_VAR)
        set("${ARG_OUTPUT_VAR}" "extracted" PARENT_SCOPE)
    endif()
endfunction()
