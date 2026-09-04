cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleSubtractive - Pedagogical Subtractive Synthesizer
# Stage 1 (Foundation): silent synth shell with the full 20-parameter APVTS +
# state persistence. First audio: Stage 2 (DSP). WebView UI: Stage 3.
# WebView2 flags are set NOW so the GUI stage inherits a correct cross-platform
# config; no WebView/binary-data is constructed at foundation.
juce_add_plugin(O-simpleSubtractive
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSiS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleSubtractive${OUARICON_DEV_SUFFIX}"
    VERSION "1.4.1"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-simpleSubtractive
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/OscillatorBank.h
        Source/SvfZDF.h
        Source/SubVoice.h
        Source/SubVizAnalyzer.h
)

# Include paths
target_include_directories(O-simpleSubtractive
    PRIVATE
        Source
)

# WebView UI resources (embedded as BinaryData) — Stage 3 GUI.
# Single binary-data target → the default BinaryData namespace is fine (only a
# 2nd target would need a distinct NAMESPACE — O-simpleGrain Stage 3.1 lesson).
juce_add_binary_data(O-simpleSubtractive_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# Required JUCE modules
target_link_libraries(O-simpleSubtractive
    PRIVATE
        O-simpleSubtractive_UIResources
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
juce_generate_juce_header(O-simpleSubtractive)

# Compile definitions
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct cross-platform
# config; the WebView itself (binary data + editor) is added in Stage 3.
target_compile_definitions(O-simpleSubtractive
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Stage 2 DSP correctness gate (offline render harness). Off by default;
# enable with -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-simpleSubtractive render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
