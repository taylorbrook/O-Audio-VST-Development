cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleBeatmaker - Pedagogical TR-808/909 Step-Sequencer Drum Machine
# Stage 1 (Foundation): silent synth shell with the full 42-parameter APVTS +
# a custom 6x32 step-grid persisted in a "PATTERN" ValueTree child. No DSP yet
# (first audio: Stage 2). WebView UI: Stage 3 — but the WebView2 cross-platform
# flags are set NOW so the GUI stage inherits a correct config. No binary-data
# target this stage (avoids the O-simpleGrain dual-namespace collision until a
# 2nd target genuinely exists in Stage 3).
juce_add_plugin(O-simpleBeatmaker
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSiB
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleBeatmaker${OUARICON_DEV_SUFFIX}"
    VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-simpleBeatmaker
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        # Stage 2 DSP spine (header-only voices / clock / feel / router / viz)
        Source/BeatmakerIDs.h
        Source/fastSine.h
        Source/DrumVoiceEngine.h
        Source/UnifiedTriggerRouter.h
        Source/SequencerClock.h
        Source/TimingFeelEngine.h
        Source/VizAnalyzer.h
)

# Include paths
target_include_directories(O-simpleBeatmaker
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-simpleBeatmaker
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
juce_generate_juce_header(O-simpleBeatmaker)

# Compile definitions
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct
# cross-platform config; the WebView itself (binary data + editor) is added in
# Stage 3.
target_compile_definitions(O-simpleBeatmaker
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Stage 2 DSP correctness gate (offline render harness). Enable with
# -DOUARICON_BUILD_TESTS=ON. The harness instantiates the processor directly,
# injects a synthetic AudioPlayHead, and asserts the 6 timing/voice probes.
option(OUARICON_BUILD_TESTS "Build O-simpleBeatmaker render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
