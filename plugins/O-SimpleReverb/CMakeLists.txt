cmake_minimum_required(VERSION 3.15)

# Include Ouaricon Module System
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-SimpleReverb
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OuSr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-SimpleReverb"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(O-SimpleReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-SimpleReverb
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-SimpleReverb
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
juce_generate_juce_header(O-SimpleReverb)

# Add Ouaricon Preset Manager module
ouaricon_add_module(O-SimpleReverb preset-manager)

# WebView UI Resources
juce_add_binary_data(O-SimpleReverb_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/preset-manager.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/flora.png
)

target_link_libraries(O-SimpleReverb
    PRIVATE
        O-SimpleReverb_UIResources
)

# Compile definitions
target_compile_definitions(O-SimpleReverb
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
