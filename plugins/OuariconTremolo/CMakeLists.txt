cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconTremolo
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ycom
    PLUGIN_CODE Outr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ouaricon Tremolo"
    VERSION 1.1.4
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OuariconTremolo
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(OuariconTremolo
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconTremolo
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# CRITICAL: Generate JuceHeader.h (JUCE 8 requirement)
# Must come AFTER target_link_libraries, BEFORE target_compile_definitions
juce_generate_juce_header(OuariconTremolo)

# Compile definitions
target_compile_definitions(OuariconTremolo
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)

# WebView UI Resources
juce_add_binary_data(OuariconTremolo_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/carrot.png
)

target_link_libraries(OuariconTremolo
    PRIVATE
        OuariconTremolo_UIResources
)
