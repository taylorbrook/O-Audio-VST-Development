cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(AutoClip
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE AuCl
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "AutoClip"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(AutoClip
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(AutoClip
    PRIVATE
        Source
)

# WebView UI Resources (embed HTML/CSS/JS into binary)
juce_add_binary_data(AutoClip_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# Required JUCE modules
target_link_libraries(AutoClip
    PRIVATE
        AutoClip_UIResources  # Link UI resources
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

# Generate JuceHeader.h (JUCE 8 requirement - MUST be after target_link_libraries)
juce_generate_juce_header(AutoClip)

# Compile definitions
target_compile_definitions(AutoClip
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
