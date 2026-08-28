cmake_minimum_required(VERSION 3.15)

# Kept for branding-var parity with the suite (OuariconModules helpers). No
# module is added at Foundation — preset-manager is a Stage 4 concern.
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleAdditive - Pedagogical 16-voice Additive / Wavetable-scan Synthesizer
# Stage 1 (Foundation): silent synth shell with full 33-param APVTS + state persistence.
# Port of O-simpleFM minus oversampling / viz / preset machinery.
# First audio: Stage 2. WebView UI: Stage 3 (CMake flags set now so Stage 3 inherits).
juce_add_plugin(O-simpleAdditive
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSiA
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleAdditive${OUARICON_DEV_SUFFIX}"
    VERSION "1.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files. Stage 2 (DSP) added the additive voice + viz analyzer headers;
# Stage 3 (GUI) adds the WebView editor (already listed) + the binary-data target below.
target_sources(O-simpleAdditive
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/AdditiveVoice.h
        Source/AdditiveVizAnalyzer.h
)

# Include paths
target_include_directories(O-simpleAdditive
    PRIVATE
        Source
)

# WebView UI resources (embedded as BinaryData) — Stage 3 GUI. The persistent
# preset-manager module is a Stage 4 concern (lesson presets ship as C++ snapshots).
juce_add_binary_data(O-simpleAdditive_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/insects.png
)

# Required JUCE modules
target_link_libraries(O-simpleAdditive
    PRIVATE
        O-simpleAdditive_UIResources
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
juce_generate_juce_header(O-simpleAdditive)

# Compile definitions
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct cross-platform
# config; the WebView itself (binary data + editor) is added in Stage 3.
# JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 is mandatory alongside NEEDS_WEBVIEW2 TRUE,
# else Windows silently shows a blank UI.
target_compile_definitions(O-simpleAdditive
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Stage 2 DSP correctness gate + per-voice refill profile (offline render
# harness). Off by default; enable with -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-simpleAdditive render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
