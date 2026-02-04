cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-FreqPulse
    VERSION 1.0.0
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OFPu
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-FreqPulse"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(O-FreqPulse
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-FreqPulse
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-FreqPulse
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
        O-FreqPulse_UIResources
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(O-FreqPulse)

# WebView UI Resources (Stage 3)
juce_add_binary_data(O-FreqPulse_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/css/styles.css
        Resources/ui/js/app.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
)

# Compile definitions
target_compile_definitions(O-FreqPulse
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
