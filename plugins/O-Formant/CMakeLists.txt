cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Formant
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuFm
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Formant${OUARICON_DEV_SUFFIX}"
    VERSION 1.12.2
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-Formant
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/FormantVoice.cpp
        Source/dsp/GlottalTableGenerator.cpp
)

# Include paths
target_include_directories(O-Formant
    PRIVATE
        Source
        Source/dsp
)

# Required JUCE modules
juce_add_binary_data(O-Formant_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/main.js
        Source/ui/public/img/flora.png
)

target_link_libraries(O-Formant
    PRIVATE
        O-Formant_UIResources
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

# Generate JuceHeader.h (JUCE 8 requirement -- after target_link_libraries)
juce_generate_juce_header(O-Formant)

# Compile definitions
target_compile_definitions(O-Formant
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
