cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Freeze
    VERSION 1.2.0
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OFCR
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Freeze${OUARICON_DEV_SUFFIX}"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(O-Freeze
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-Freeze
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Freeze
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
juce_generate_juce_header(O-Freeze)

# WebView UI Resources
juce_add_binary_data(O-Freeze_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/assets/paper1.jpg
        Source/ui/public/assets/muscles.png
)

target_link_libraries(O-Freeze
    PRIVATE
        O-Freeze_UIResources
)

# Compile definitions
target_compile_definitions(O-Freeze
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
