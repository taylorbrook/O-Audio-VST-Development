cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleAmbisonics - Pedagogical Ambisonic Spatializer (mono-source → binaural / stereo)
# Stage 1 (Foundation): stereo EFFECT shell with the full 12-parameter APVTS,
# mono/stereo-in → stereo-out buses, state persistence, and the FINAL latency
# contract already in place: setLatencySamples(kPartition = 64) with a
# pass-through chain (input conditioner → 64-sample delay → seeded output gain).
# Stage 2 swaps the delay for SpatialEngine without changing host-visible timing.
# WebView UI: Stage 3. WebView2 flags are set NOW so the GUI stage inherits a
# correct cross-platform config; no WebView/binary-data is constructed at
# foundation.
#
# HRTF data (Stage 2.3) will be a generated constexpr header
# (Source/dsp/BinauralFilterData.h) — NEVER a second juce_add_binary_data target
# (critical_dual_binary_data_namespace_collision). The Stage-3 UI target will be
# the only binary-data target this plugin ever has.
juce_add_plugin(O-simpleAmbisonics
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSAm
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleAmbisonics${OUARICON_DEV_SUFFIX}"
    VERSION "1.0.0"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files — no PluginEditor.* at Stage 1 (createEditor() returns the
# Generic editor inline in PluginProcessor.cpp; Stage 3 adds the editor TU
# behind a #if JUCE_WEB_BROWSER guard so the render harness keeps building).
target_sources(O-simpleAmbisonics
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/dsp/SpatialEngine.h
        Source/dsp/FixedDelay.h
        Source/dsp/Smoothers.h
        Source/dsp/ShEncoder.h
        Source/dsp/DistanceModel.h
        Source/dsp/OrbitLfo.h
        Source/dsp/StereoDecoder.h
        Source/dsp/DecoderMixer.h
        Source/dsp/MeterFrame.h
        Source/dsp/TripleBuffer.h
)

# Include paths
target_include_directories(O-simpleAmbisonics
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-simpleAmbisonics
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

# Generate JuceHeader.h (JUCE 8 requirement - MUST come after target_link_libraries)
juce_generate_juce_header(O-simpleAmbisonics)

# Compile definitions
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct cross-platform
# config; the WebView itself (binary data + editor) is added in Stage 3.
target_compile_definitions(O-simpleAmbisonics
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Offline render harness (Stage-1 pass-through / latency probes P0–P5; becomes
# the Stage-2 DSP correctness gate). Off by default; enable with
# -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-simpleAmbisonics render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
