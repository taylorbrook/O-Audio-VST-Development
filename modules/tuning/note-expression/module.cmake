# ==============================================================================
# note-expression module CMake hook
# Verifies the local JUCE fork has the JUCE-NE-PATCH markers applied.
# Fails loud + fails fast at configure time (D-15).
#
# Cross-platform note: CMake-native file/READ + string/FIND is chosen over
# execute_process(grep) because Windows hosts lack `grep`. This check is
# scoped to plugins that consume the note-expression module (only fires
# when ouaricon_add_module(<plugin> note-expression) is called).
# ==============================================================================

# Locate JUCE tree the same way the root CMakeLists.txt does.
if(DEFINED ENV{JUCE_DIR})
    set(_NE_JUCE_ROOT "$ENV{JUCE_DIR}")
elseif(WIN32)
    set(_NE_JUCE_ROOT "C:/JUCE")
else()
    set(_NE_JUCE_ROOT "/Users/taylorbrook/JUCE")
endif()

set(_NE_MARKER "JUCE-NE-PATCH")
set(_NE_FILE1 "${_NE_JUCE_ROOT}/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h")
set(_NE_FILE2 "${_NE_JUCE_ROOT}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp")

foreach(_ne_f ${_NE_FILE1} ${_NE_FILE2})
    if(NOT EXISTS "${_ne_f}")
        message(FATAL_ERROR
            "[note-expression] Expected JUCE source not found: ${_ne_f}\n"
            "Ensure JUCE 8.0.4 is installed and run scripts/apply-juce-patches.sh")
    endif()
    file(READ "${_ne_f}" _ne_contents)
    string(FIND "${_ne_contents}" "${_NE_MARKER}" _ne_idx)
    if(_ne_idx EQUAL -1)
        message(FATAL_ERROR
            "[note-expression] JUCE patch marker '${_NE_MARKER}' not found in:\n"
            "  ${_ne_f}\n"
            "Run: ./scripts/apply-juce-patches.sh")
    endif()
endforeach()

message(STATUS "[note-expression] JUCE-NE-PATCH markers verified in ${_NE_JUCE_ROOT}")

# ==============================================================================
# Phase 25 (D-04, D-05, D-06): Canonical Dorico expression map install rules.
# Fires per-consumer when ouaricon_add_module(<Plugin> note-expression) is
# called. Each plugin's installer (PKG on macOS, EXE on Windows) inherits these
# install() rules and dual-writes the .doricoexpmap to (a) Ouaricon shared
# resources path and (b) Dorico's user expression-maps scan path.
#
# The canonical asset is owned by the module — no per-plugin copy of the file
# exists in any plugin's source tree. Phase 25 INST-01..04.
# ==============================================================================

set(_NE_RESOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/resources")
set(_NE_DORICOEXPMAP "${_NE_RESOURCES_DIR}/Ouaricon-VST3-NoteExpression.doricoexpmap")
set(_NE_DORICOEXPMAP_README "${_NE_RESOURCES_DIR}/README-doricoexpmap.txt")
set(_NE_DORICOEXPMAP_NAME "Ouaricon-VST3-NoteExpression.doricoexpmap")

if(NOT EXISTS "${_NE_DORICOEXPMAP}")
    message(FATAL_ERROR
        "[note-expression] Canonical Dorico expression map not found:\n"
        "  ${_NE_DORICOEXPMAP}\n"
        "Module is at version 1.1.0 which requires the resources/ asset. "
        "Re-run from a clean checkout or restore the file from git.")
endif()

# Ouaricon shared resources path — always written, regardless of Dorico
# presence. This is the editable canonical user copy (D-05).
if(APPLE)
    install(FILES
        "${_NE_DORICOEXPMAP}"
        "${_NE_DORICOEXPMAP_README}"
        DESTINATION "$ENV{HOME}/Library/Application Support/Ouaricon/Expression Maps"
        COMPONENT note-expression-resources
    )
elseif(WIN32)
    install(FILES
        "${_NE_DORICOEXPMAP}"
        "${_NE_DORICOEXPMAP_README}"
        DESTINATION "$ENV{APPDATA}/Ouaricon/Expression Maps"
        COMPONENT note-expression-resources
    )
else()
    message(STATUS "[note-expression] Non-macOS, non-Windows host — Dorico expression map install skipped (Dorico is not supported on Linux)")
endif()

# Dorico version targeting (D-07): probe for installed Dorico major versions
# at install time, prefer the latest detected. configure_file() expands
# @-wrapped placeholders at configure time (per consumer plugin), producing a
# concrete script. install(SCRIPT) runs it at install time, where $ENV{HOME}
# and $ENV{APPDATA} resolve cleanly without escape-string ambiguity.
# (Per-consumer file naming prevents collisions when multiple consumers
# configure into the same build tree.)
if(APPLE OR WIN32)
    set(NE_DORICOEXPMAP "${_NE_DORICOEXPMAP}")
    set(NE_DORICOEXPMAP_NAME "${_NE_DORICOEXPMAP_NAME}")
    set(NE_TARGET_NAME "${TARGET_NAME}")
    set(_NE_GENERATED_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-doricoexpmap-${TARGET_NAME}.cmake")
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/install-doricoexpmap.cmake.in"
        "${_NE_GENERATED_SCRIPT}"
        @ONLY
    )
    install(SCRIPT "${_NE_GENERATED_SCRIPT}" COMPONENT note-expression-resources)
endif()

message(STATUS "[note-expression] Canonical .doricoexpmap install rules registered for ${TARGET_NAME} (module v1.1.0, INST-01..04)")
