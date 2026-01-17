cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconLyrica
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE OuAu
    PLUGIN_CODE OLyr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "OuariconLyrica"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OuariconLyrica
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/HarpSynthSound.cpp
        Source/HarpSynthVoice.cpp
        Source/DSP/DelayLine.cpp
        Source/DSP/StringVoice.cpp
        Source/DSP/WaveguideString.cpp
        Source/DSP/BridgeFilter.cpp
        Source/DSP/PluckExciter.cpp
        Source/DSP/StiffnessFilter.cpp
        Source/DSP/StringMaterial.cpp
        Source/DSP/BodyResonance.cpp
        Source/DSP/SympatheticResonance.cpp
        Source/DSP/TuningEngine.cpp
)

# Include paths
target_include_directories(OuariconLyrica
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconLyrica
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
juce_generate_juce_header(OuariconLyrica)

# Compile definitions
target_compile_definitions(OuariconLyrica
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
