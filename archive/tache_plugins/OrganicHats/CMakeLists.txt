cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OrganicHats
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ycmp
    PLUGIN_CODE Orgh
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "OrganicHats"
    NEEDS_WEB_BROWSER TRUE
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_EMAIL "support@yourcompany.com"
    PLUGIN_MANUFACTURER_WEBSITE "https://yourcompany.com"
)

# Source files
target_sources(OrganicHats
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/HiHatVoice.cpp
)

# Include paths
target_include_directories(OrganicHats
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OrganicHats
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
juce_generate_juce_header(OrganicHats)

# Compile definitions
target_compile_definitions(OrganicHats
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)

# WebView UI Resources (Pattern #13: check_native_interop.js is REQUIRED)
juce_add_binary_data(OrganicHats_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_link_libraries(OrganicHats
    PRIVATE
        OrganicHats_UIResources
)
