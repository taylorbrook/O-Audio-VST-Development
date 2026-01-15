cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconMarimba
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE OuAu
    PLUGIN_CODE OuMa
    PLUGIN_VERSION 1.9.9
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ouaricon Marimba"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OuariconMarimba
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MarimbaVoice.cpp
        Source/TuningEngine.cpp
        Source/BodyResonance.cpp
        Source/PresetManager.cpp
)

# Include paths
target_include_directories(OuariconMarimba
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/effects/analog-eq-unit/cpp
        ${CMAKE_SOURCE_DIR}/modules/effects/compressor-unit/cpp
)

# Required JUCE modules
target_link_libraries(OuariconMarimba
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

# Generate JuceHeader.h (JUCE 8 requirement - MUST be after target_link_libraries)
juce_generate_juce_header(OuariconMarimba)

# Compile definitions
target_compile_definitions(OuariconMarimba
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)

# WebView UI Resources
juce_add_binary_data(OuariconMarimba_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/images/paper1.jpg
        Source/ui/public/images/tree_expositionmtho00lamo_0207.png
        ${CMAKE_SOURCE_DIR}/modules/effects/analog-eq-unit/js/analog-eq-unit.js
        ${CMAKE_SOURCE_DIR}/modules/effects/compressor-unit/js/compressor-unit.js
)

target_link_libraries(OuariconMarimba
    PRIVATE
        OuariconMarimba_UIResources
)
