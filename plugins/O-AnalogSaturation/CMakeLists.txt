cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-AnalogSaturation
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OaSa
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-AnalogSaturation"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(O-AnalogSaturation
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-AnalogSaturation
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-AnalogSaturation
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

# Generate JuceHeader.h (JUCE 8 requirement - CRITICAL: must come after target_link_libraries)
juce_generate_juce_header(O-AnalogSaturation)

# WebView UI Resources (Pattern 3: Binary data for embedded HTML/CSS/JS/images)
juce_add_binary_data(O-AnalogSaturation_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper1.jpg
        Source/ui/public/img/snake_mobot31753000317195_0068.png
        Source/ui/public/img/snake_mobot31753000317195_0070.png
        Source/ui/public/img/snake_NA_0145.png
        Source/ui/public/img/snake_snakesaustralia00kref_0145.png
)

target_link_libraries(O-AnalogSaturation
    PRIVATE
        O-AnalogSaturation_UIResources
)

# Compile definitions
target_compile_definitions(O-AnalogSaturation
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1  # Pattern 3: Enable WebView
        JUCE_USE_CURL=0     # Pattern 3: Disable CURL (not needed for local resources)
)
