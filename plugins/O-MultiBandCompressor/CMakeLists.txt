cmake_minimum_required(VERSION 3.15)

# Plugin Configuration
juce_add_plugin(O-MultiBandCompressor
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OMbc
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-MultiBandCompressor"
    NEEDS_WEB_BROWSER TRUE
)

# WebView UI Resources (Phase 5.1)
juce_add_binary_data(O-MultiBandCompressor_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# Source files
target_sources(O-MultiBandCompressor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-MultiBandCompressor
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-MultiBandCompressor
    PRIVATE
        O-MultiBandCompressor_UIResources
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
juce_generate_juce_header(O-MultiBandCompressor)

# Compile definitions
target_compile_definitions(O-MultiBandCompressor
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
