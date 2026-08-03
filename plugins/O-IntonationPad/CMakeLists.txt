cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-IntonationPad
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuIP
    VERSION "2.8.4"
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-IntonationPad${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-IntonationPad
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PresetManager.cpp
        Source/DSP/WavetableVoice.cpp
        Source/DSP/ChordGenerator.cpp
        Source/DSP/TuningEngine.cpp
        Source/DSP/ScaleGenerator.cpp
        Source/DSP/TuningExporter.cpp
        Source/DSP/EmbeddedTunings.cpp
        Source/DSP/DelayProcessor.cpp
        Source/DSP/EQProcessor.cpp
        Source/DSP/ReverbProcessor.cpp
)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-IntonationPad note-expression)

# Include paths
target_include_directories(O-IntonationPad
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-IntonationPad
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices  # Phase 24: required for Standalone format (matches sibling plugins)
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils     # Phase 24: required for Standalone format (matches sibling plugins)
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(O-IntonationPad)

# WebView UI Resources
juce_add_binary_data(O-IntonationPad_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/constants.js
        Source/ui/public/js/tuning-panel.js
        Source/ui/public/css/tuning-panel.css
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/shell.png
)

target_link_libraries(O-IntonationPad
    PRIVATE
        O-IntonationPad_UIResources
)

# Compile definitions
target_compile_definitions(O-IntonationPad
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
