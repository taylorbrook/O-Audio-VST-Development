cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OBass
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE OuAu
    PLUGIN_CODE OuBa
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "OBass"
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OBass
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/CrossoverFilter.cpp
        Source/DSP/MonoSummer.cpp
        Source/DSP/EnvelopeFollower.cpp
        Source/DSP/PitchTracker.cpp
        Source/DSP/HarmonicGenerator.cpp
)

# Include paths
target_include_directories(OBass
    PRIVATE
        Source
        Source/DSP
)

# Required JUCE modules
target_link_libraries(OBass
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

# Generate JuceHeader.h (JUCE 8 requirement - CRITICAL)
juce_generate_juce_header(OBass)

# Compile definitions
target_compile_definitions(OBass
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
