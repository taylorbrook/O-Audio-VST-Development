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
# Microtonal Suite — Dorico Playback Template + .doricolib distribution
# Phase 25 v2 (Playback Template pivot — supersedes Plan 25-01 v1).
#
# Per D-08: this module owns BOTH distributable resources. Any consumer of
# ouaricon_add_module(<plugin> note-expression) automatically inherits
# dual-resource installation via install(SCRIPT ...) below.
#
# Build flow:
#   1. ouaricon_extract_vst3_cids -> reads each built VST3's moduleinfo.json
#   2. configure_file @ONLY -> substitutes @<NAME>_PLUGINID@ in endpointconfig.xml.in
#   3. cmake -E tar cf --format=zip -> packs Ouaricon-Microtonal-Suite.dorico_pt
#
# Install flow (per-consumer, per-platform):
#   install-microtonal-suite-<TARGET>.cmake -> cmake -E tar xf into
#   ~/Library/Application Support/Steinberg/Dorico [N]/ + Default Library Additions/
#
# Idempotency note: all 8 cohort plugins fire this block at configure time,
# but the staging directory + dorico_pt artifact are CMAKE_BINARY_DIR-rooted
# (project-global), so the configure_file outputs and add_custom_target are
# defined repeatedly. CMake's add_custom_target() guards against duplicate
# target names; we bracket with an `if(NOT TARGET ...)` to be explicit.
# ==============================================================================

set(_OUARICON_SUITE_NAME "Ouaricon-Microtonal-Suite")
set(_OUARICON_SUITE_PT_FILE "${CMAKE_BINARY_DIR}/${_OUARICON_SUITE_NAME}.dorico_pt")
set(DORICO_PT_STAGE "${CMAKE_BINARY_DIR}/_microtonal-suite/${_OUARICON_SUITE_NAME}")

if(NOT TARGET ouaricon_microtonal_suite_pt)
    file(MAKE_DIRECTORY "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite")
    file(MAKE_DIRECTORY "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite")

    # Read each built VST3's CID into <NAME>_PLUGINID variables in this scope.
    # The helper FATAL_ERRORs if any plugin's moduleinfo.json is missing,
    # forcing the natural ordering: build all 8 _VST3 targets before this fires.
    ouaricon_extract_vst3_cids(
        OUTPUT_VAR _OUARICON_PLUGIN_CIDS
        PLUGINS OLyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant
    )

    # Render the 3 templated XML files into the staging tree.
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml.in"
        "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml"
        @ONLY
    )
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml.in"
        "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
        @ONLY
    )
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/resources/playback-template/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib.in"
        "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib"
        @ONLY
    )

    # Pack the .dorico_pt zip. WORKING_DIRECTORY = stage dir so the zip's
    # first entries are PlaybackTemplateSpecs/... and EndpointConfigs/...
    # NOT a wrapping parent dir (Pitfall 5 mitigation).
    add_custom_command(
        OUTPUT "${_OUARICON_SUITE_PT_FILE}"
        COMMAND "${CMAKE_COMMAND}" -E tar cf
                "${_OUARICON_SUITE_PT_FILE}"
                --format=zip
                "PlaybackTemplateSpecs"
                "EndpointConfigs"
        WORKING_DIRECTORY "${DORICO_PT_STAGE}"
        DEPENDS
            "${DORICO_PT_STAGE}/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml"
            "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"
            "${DORICO_PT_STAGE}/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib"
        COMMENT "Packing Ouaricon-Microtonal-Suite.dorico_pt"
        VERBATIM
    )
    add_custom_target(ouaricon_microtonal_suite_pt ALL
        DEPENDS "${_OUARICON_SUITE_PT_FILE}")
endif()

# Per-consumer install hook: render install-microtonal-suite-<TARGET>.cmake
# from the .in template and register it via install(SCRIPT). Each consumer
# fires its own per-target script output so the install pulls both
# resources into user systems when `cmake --install` runs for that target.
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install-microtonal-suite.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
    @ONLY
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install-microtonal-suite-${TARGET_NAME}.cmake"
        COMPONENT "ouaricon_note_expression_${TARGET_NAME}")
