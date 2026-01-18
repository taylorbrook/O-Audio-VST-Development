cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconPolystutter
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE OuAu
    PLUGIN_CODE OuPs
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ouaricon Polystutter"
    VERSION 1.4.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
)

# Source files
target_sources(OuariconPolystutter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/RepeatLane.cpp
        Source/DSP/TriggerRouter.cpp
        Source/DSP/TapeDegrader.cpp
)

# Include paths
target_include_directories(OuariconPolystutter
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconPolystutter
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
juce_generate_juce_header(OuariconPolystutter)

# Phase 3.2: WebView UI Resources (BinaryData)
# Embed HTML/CSS/JS/images into plugin binary
juce_add_binary_data(OuariconPolystutter_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/parameter-bindings.js
        Source/ui/public/img/paper-background.jpg
        Source/ui/public/img/botanical-bug.png
)

# Link UIResources to plugin
target_link_libraries(OuariconPolystutter
    PRIVATE
        OuariconPolystutter_UIResources
)

# Compile definitions
target_compile_definitions(OuariconPolystutter
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1  # Enable WebView support
        JUCE_USE_CURL=0     # Disable CURL (not needed for local HTML)
)
