cmake_minimum_required(VERSION 3.15)

# O-Bells - Physical Modeling Bell Synthesizer
# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Bells
    COMPANY_NAME "Ouaricon Development"
    PLUGIN_MANUFACTURER_CODE OuDv
    PLUGIN_CODE OBls
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bells"
    PLUGIN_VERSION "3.1.1"

    # Synthesizer configuration
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # WebView UI configuration
    NEEDS_WEB_BROWSER TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-Bells
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BellSound.h
        Source/BellVoice.h
        Source/BellVoice.cpp
        Source/TuningEngine.cpp
        Source/ScaleGenerator.cpp
        Source/TuningExporter.cpp
        Source/EmbeddedTunings.cpp
)

# Include paths
target_include_directories(O-Bells
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Bells
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
juce_generate_juce_header(O-Bells)

# Compile definitions
target_compile_definitions(O-Bells
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)

# Module system integration
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# WebView UI Resources
juce_add_binary_data(O-Bells_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/snail.png
        Resources/ui/modules/instrument-footer-panel.js
        Resources/ui/css/instrument-footer-panel.css
        Resources/ui/js/tuning-panel.js
        Resources/ui/css/tuning-panel.css
)

target_link_libraries(O-Bells
    PRIVATE
        O-Bells_UIResources
)
