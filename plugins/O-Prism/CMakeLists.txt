cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Prism
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuPr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Prism${OUARICON_DEV_SUFFIX}"
    VERSION 1.17.2
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-Prism
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/FactoryPresets.cpp
        Source/PrismVoice.cpp
        Source/TuningEngine.cpp
        Source/ScaleGenerator.cpp
        Source/EmbeddedTunings.cpp
        Source/TuningExporter.cpp
        Source/dsp/WavetableGenerator.cpp
        Source/dsp/WavetableFactory.cpp
        Source/dsp/WavetableOscillator.cpp
        Source/dsp/WavetableImporter.cpp
        Source/dsp/UserWavetableManager.cpp
        Source/dsp/SubOscillator.cpp
        Source/dsp/NoiseGenerator.cpp
        Source/dsp/SVFFilter.cpp
        Source/dsp/DistortionProcessor.cpp
        Source/dsp/DelayProcessor.cpp
        Source/dsp/ReverbProcessor.cpp
        Source/dsp/EQProcessor.cpp
        Source/dsp/EnsembleChorus.cpp
        Source/dsp/LFO.cpp
        Source/dsp/ModulationMatrix.cpp
        Source/dsp/WavetableEditor.cpp
)

# Include paths
target_include_directories(O-Prism
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Prism
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

# Generate JuceHeader.h (JUCE 8 requirement — after target_link_libraries)
juce_generate_juce_header(O-Prism)

# WebView relay manager module
ouaricon_add_module(O-Prism webview-relay-manager)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Prism note-expression)

# WebView UI Resources
juce_add_binary_data(O-Prism_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/wavetable-editor.js
        Source/ui/public/css/wavetable-editor.css
)

target_link_libraries(O-Prism
    PRIVATE
        O-Prism_UIResources
)

# Compile definitions
target_compile_definitions(O-Prism
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
