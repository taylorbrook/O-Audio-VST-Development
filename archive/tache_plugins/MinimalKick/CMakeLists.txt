cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(MinimalKick
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ylxc
    PLUGIN_CODE Mnkk
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "MinimalKick"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(MinimalKick
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# WebView UI Resources
juce_add_binary_data(MinimalKick_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# Include paths
target_include_directories(MinimalKick
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(MinimalKick
    PRIVATE
        MinimalKick_UIResources
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
        juce::juce_gui_extra  # Required for WebBrowserComponent
        juce::juce_dsp        # Required for DSP components
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h (JUCE 8 requirement - Pattern #1)
juce_generate_juce_header(MinimalKick)

# Compile definitions
target_compile_definitions(MinimalKick
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
