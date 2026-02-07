cmake_minimum_required(VERSION 3.15)

# Include Ouaricon Module System
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconTremolo
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuTr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Tremolo${OUARICON_DEV_SUFFIX}"
    VERSION 1.4.0
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Add preset-manager module
ouaricon_add_module(OuariconTremolo preset-manager)

# Source files
target_sources(OuariconTremolo
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(OuariconTremolo
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconTremolo
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# CRITICAL: Generate JuceHeader.h (JUCE 8 requirement)
# Must come AFTER target_link_libraries, BEFORE target_compile_definitions
juce_generate_juce_header(OuariconTremolo)

# Licensing module (compile-flag gated, OFF for local dev)
if(OUARICON_LICENSING)
    ouaricon_add_module(OuariconTremolo licensing)
    target_compile_definitions(OuariconTremolo PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(OuariconTremolo PRIVATE juce::juce_cryptography)
endif()

# Compile definitions
target_compile_definitions(OuariconTremolo
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# WebView UI Resources
juce_add_binary_data(OuariconTremolo_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/carrot.png
        Source/ui/public/modules/preset-manager.js
)

target_link_libraries(OuariconTremolo
    PRIVATE
        OuariconTremolo_UIResources
)
