cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconSimpleReverb
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE OuAu
    PLUGIN_CODE OuSr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ouaricon Simple Reverb"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OuariconSimpleReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(OuariconSimpleReverb
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconSimpleReverb
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
juce_generate_juce_header(OuariconSimpleReverb)

# WebView UI Resources
juce_add_binary_data(OuariconSimpleReverb_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/flora.png
)

target_link_libraries(OuariconSimpleReverb
    PRIVATE
        OuariconSimpleReverb_UIResources
)

# Compile definitions
target_compile_definitions(OuariconSimpleReverb
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
