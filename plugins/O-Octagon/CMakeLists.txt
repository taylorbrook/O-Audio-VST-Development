cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# NOTE: unlike sibling O-Orbit, O-Octagon does NOT use SAF. There is deliberately no
# add_subdirectory(libs/SAF/framework) here — DBAP is implemented in-tree (ARCHITECTURE §3).
# Copying O-Orbit's CMakeLists wholesale would silently import SAF.

juce_add_plugin(OuariconOctagon
    COMPANY_NAME             "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE              OuOc            # verified unused across all 39 existing plugins
    FORMATS                  VST3 AU Standalone
    PRODUCT_NAME             "O-Octagon${OUARICON_DEV_SUFFIX}"
    VERSION                  1.0.0           # NOT PLUGIN_VERSION — that keyword is not a JUCE
                                             # keyword, is silently ignored, and ships 1.0.0 anyway
    NEEDS_WEB_BROWSER        TRUE
    NEEDS_WEBVIEW2           TRUE
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    IS_MIDI_EFFECT           FALSE
    VST3_CATEGORIES          "Fx" "Spatial"  # JUCE hoists Fx to index 0 regardless and emits
                                             # "Fx|Spatial"; writing it first makes the source
                                             # match what actually ships (RESEARCH F6)
    AU_MAIN_TYPE             "kAudioUnitType_Effect"
    # DO NOT set PLUGIN_CHANNEL_CONFIGURATIONS — it understands channel COUNTS only, not channel
    # TYPES, and breaks surround layout detection. It is also redundant: JUCE derives the entire
    # AU channel-config set from isBusesLayoutSupported() (RESEARCH F2).
)

target_sources(OuariconOctagon
    PRIVATE
        Source/PluginProcessor.cpp
        Source/Data/VenueModel.cpp
        Source/DSP/ChannelMap.cpp
        Source/DSP/ConvexHull2D.cpp
        # Source/DSP/Vec.h and Source/Data/VenueSnapshot.h are header-only and deliberately absent.
)

# No juce_add_binary_data target at Stage 1 — the WebView UI lands at Phase 3.1.
# Neither the hyphen-stripping nor the namespace-collision hazard applies yet.

target_include_directories(OuariconOctagon
    PRIVATE
        Source
)

target_link_libraries(OuariconOctagon
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Order is load-bearing: juce_generate_juce_header() AFTER target_link_libraries and
# BEFORE target_compile_definitions (juce8-critical-patterns §1).
juce_generate_juce_header(OuariconOctagon)

target_compile_definitions(OuariconOctagon
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Set now although Stage 1 has no WebView. The flags are inert with a generic editor, and
        # having JUCE_WEB_BROWSER defined from the start is what lets the Phase 2.2 render harness
        # guard createEditor with #if JUCE_WEB_BROWSER BEFORE the Stage 3 swap rather than after it
        # (pattern_render_harness_breaks_on_webview_editor).
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# ── Test targets (Phase 2.1) ──────────────────────────────────────────────────
# One option gates both, matching the repo convention across twelve existing harnesses.
#
#   O-Octagon-geometry-test  — links ONLY the three geometry TUs (no PluginProcessor.cpp)
#   O-Octagon-render-test    — links the processor; bus layouts constructed programmatically
#
# DO NOT add these to .github/workflows/build-and-release.yml: it is tag-triggered and
# secrets-bearing, and carries a standing rule against widening its trigger surface. The residual
# gap — a JUCE bump performed without running the test target ships silently — is logged as a
# repo-level todo instead (PLAN-2.1 P13).
option(OUARICON_BUILD_TESTS "Build O-Octagon test targets" OFF)

if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/unit)
    add_subdirectory(tests/render-harness)
endif()
