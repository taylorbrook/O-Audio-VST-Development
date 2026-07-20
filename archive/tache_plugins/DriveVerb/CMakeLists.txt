cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(DriveVerb
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Drvb
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DriveVerb"
    NEEDS_WEB_BROWSER TRUE
)

# Source files (minimal for Stage 2)
target_sources(DriveVerb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(DriveVerb
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(DriveVerb
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
juce_generate_juce_header(DriveVerb)

# WebView UI Resources
juce_add_binary_data(DriveVerb_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# Link UI resources
target_link_libraries(DriveVerb
    PRIVATE
        DriveVerb_UIResources
)

# Compile definitions
target_compile_definitions(DriveVerb
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
