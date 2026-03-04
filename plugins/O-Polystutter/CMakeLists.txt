cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OPolystutter
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuPs
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Polystutter${OUARICON_DEV_SUFFIX}"
    VERSION 1.12.1
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(OPolystutter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/RepeatLane.cpp
        Source/DSP/TriggerRouter.cpp
        Source/DSP/TapeDegrader.cpp
)

# Include paths
target_include_directories(OPolystutter
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OPolystutter
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
juce_generate_juce_header(OPolystutter)

# Phase 3.2: WebView UI Resources (BinaryData)
# Embed HTML/CSS/JS/images into plugin binary
juce_add_binary_data(OPolystutter_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/parameter-bindings.js
        Source/ui/public/modules/preset-manager.js
        Source/ui/public/img/paper-background.jpg
        Source/ui/public/img/botanical-bug.png
)

# Link UIResources to plugin
target_link_libraries(OPolystutter
    PRIVATE
        OPolystutter_UIResources
)

# Compile definitions
target_compile_definitions(OPolystutter
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
