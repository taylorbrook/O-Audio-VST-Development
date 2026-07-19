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

# The VST3ClientExtensions header moved modules at JUCE 8.0.14
# (juce_audio_processors → juce_audio_processors_headless/utilities/). Resolve
# it in a both-paths-tolerant way so the marker guard passes on BOTH a local
# pre-8.0.14 tree (old path) AND an 8.0.14+ tree (headless path), and survives
# the future JUCE_VERSION bump.
#
# Preference order matters: a pre-8.0.14 tree COMPILES the old-path header, and
# overlaying the 8.0.14 vendored layout onto such a tree leaves a marker-bearing
# headless header as an orphan the build never touches. Checking the old path
# first whenever it exists keeps the guard pointed at the header the build
# actually compiles (8.0.14+ trees have no old path at all).
set(_NE_FILE1_NEW "${_NE_JUCE_ROOT}/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h")
set(_NE_FILE1_OLD "${_NE_JUCE_ROOT}/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h")
if(EXISTS "${_NE_FILE1_OLD}")
    set(_NE_FILE1 "${_NE_FILE1_OLD}")
else()
    set(_NE_FILE1 "${_NE_FILE1_NEW}")
endif()
set(_NE_FILE2 "${_NE_JUCE_ROOT}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp")

foreach(_ne_f IN ITEMS "${_NE_FILE1}" "${_NE_FILE2}")
    if(NOT EXISTS "${_ne_f}")
        message(FATAL_ERROR
            "[note-expression] Expected JUCE source not found: ${_ne_f}\n"
            "Ensure the patched JUCE fork is installed and run scripts/apply-juce-patches.sh")
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
# Microtonal Suite — Dorico .doricolib distribution (Path B, Phase 25 v3)
#
# Per D-04: this module owns the single distributable resource. Any consumer of
# ouaricon_add_module(<plugin> note-expression) automatically inherits a
# per-consumer install(SCRIPT ...) rule that single-writes the .doricolib +
# README to the platform-specific Ouaricon shared path (D-07):
#   macOS:   ~/Library/Application Support/Ouaricon/Microtonal Suite/
#   Windows: %APPDATA%/Ouaricon/Microtonal Suite/
#
# User performs a one-time Library Manager Import per machine (D-01). No
# auto-discovery, no Playback Template, no plugin CID extraction.
# ==============================================================================

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install-microtonal-suite.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
    @ONLY
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
        COMPONENT "ouaricon_note_expression_${TARGET_NAME}")
