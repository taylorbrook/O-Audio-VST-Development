cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OLyrica
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OLyr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Lyrica${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
# v1.3.1: Removed unused files (HarpSynthSound.cpp, DelayLine, StringVoice, BridgeFilter)
# v1.14.0: Added ScaleGenerator for EDO/Harmonic/Rank-2 scale generation
# v1.15.0: Added EmbeddedTunings for factory tuning library
# v1.16.0: Added TuningExporter for HTML export
# v1.17.0: Renamed from OuariconLyrica to O-Lyrica
target_sources(OLyrica
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
        Source/DSP/ScaleGenerator.cpp
        Source/DSP/EmbeddedTunings.cpp
        Source/DSP/TuningExporter.cpp
        Source/DSP/DelayProcessor.cpp
        Source/DSP/EQProcessor.cpp
        Source/DSP/ReverbProcessor.cpp
)

# Include paths
target_include_directories(OLyrica
    PRIVATE
        Source
        ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/persistence/preset-manager/cpp
)

# Required JUCE modules
target_link_libraries(OLyrica
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
juce_generate_juce_header(OLyrica)

# ═══════════════════════════════════════════════════════════════════
# Phase 3.1: WebView UI Resources - Embed as BinaryData
# ═══════════════════════════════════════════════════════════════════
# v1.4.0: Added Ouaricon Naturalist aesthetic images
juce_add_binary_data(OLyricaBinaryData
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
target_link_libraries(OLyrica
    PRIVATE
        OLyricaBinaryData
)

# Compile definitions
target_compile_definitions(OLyrica
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
