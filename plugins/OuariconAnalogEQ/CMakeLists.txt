cmake_minimum_required(VERSION 3.15)
project(OuariconAnalogEQ VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(/Users/taylorbrook/JUCE ${CMAKE_BINARY_DIR}/JUCE)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconAnalogEQ
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE OuAu
    PLUGIN_CODE OuAE
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ouaricon Analog EQ"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OuariconAnalogEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(OuariconAnalogEQ
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconAnalogEQ
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
juce_generate_juce_header(OuariconAnalogEQ)

# WebView UI Resources
juce_add_binary_data(OuariconAnalogEQ_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/images/paper1.jpg
        Source/ui/public/images/flower_ferdinandibauer00baue_0021.png
)

target_link_libraries(OuariconAnalogEQ
    PRIVATE
        OuariconAnalogEQ_UIResources
)

# Compile definitions
target_compile_definitions(OuariconAnalogEQ
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
