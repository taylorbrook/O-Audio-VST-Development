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
# v1.3.1: Removed unused files (HarpSynthSound.cpp, DelayLine, StringVoice, BridgeFilter)
target_sources(OuariconLyrica
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/HarpSynthVoice.cpp
        Source/DSP/WaveguideString.cpp
        Source/DSP/PluckExciter.cpp
        Source/DSP/StiffnessFilter.cpp
        Source/DSP/StringMaterial.cpp
        Source/DSP/BodyResonance.cpp
        Source/DSP/SympatheticResonance.cpp
        Source/DSP/TuningEngine.cpp
        Source/DSP/GlissandoController.cpp
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

# ═══════════════════════════════════════════════════════════════════
# Phase 3.1: WebView UI Resources - Embed as BinaryData
# ═══════════════════════════════════════════════════════════════════
# v1.4.0: Added Ouaricon Naturalist aesthetic images
juce_add_binary_data(OuariconLyricaBinaryData
    SOURCES
        Resources/ui/index.html
        Resources/ui/css/styles.css
        Resources/ui/js/app.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/images/paper1.jpg
        Resources/ui/images/fern_naturalistsmisc1Geor_0089.png
)

# Link binary data to plugin
target_link_libraries(OuariconLyrica
    PRIVATE
        OuariconLyricaBinaryData
)

# Compile definitions
target_compile_definitions(OuariconLyrica
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
