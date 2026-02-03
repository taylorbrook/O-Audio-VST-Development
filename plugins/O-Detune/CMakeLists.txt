cmake_minimum_required(VERSION 3.15)

# O-Detune - Colorful lo-fi detuning plugin
# Combines tape wobble with unison thickness

# Plugin configuration
juce_add_plugin(O-Detune
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OuDt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Detune"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(O-Detune
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-Detune
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Detune
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

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(O-Detune)

# Compile definitions
target_compile_definitions(O-Detune
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)

# WebView UI Resources
juce_add_binary_data(O-Detune_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/assets/paper1.jpg
        Source/ui/public/assets/slug.png
)

target_link_libraries(O-Detune
    PRIVATE
        O-Detune_UIResources
)
