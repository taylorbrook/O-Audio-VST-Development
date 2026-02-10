cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconGrainScatter
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuGS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-GrainScatter${OUARICON_DEV_SUFFIX}"
    VERSION 2.0.1
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(OuariconGrainScatter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(OuariconGrainScatter
    PRIVATE
        Source
)

target_link_libraries(OuariconGrainScatter
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

juce_generate_juce_header(OuariconGrainScatter)

# Licensing module
if(OUARICON_LICENSING)
    ouaricon_add_module(OuariconGrainScatter licensing)
    target_compile_definitions(OuariconGrainScatter PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(OuariconGrainScatter PRIVATE juce::juce_cryptography)
endif()

# WebView UI Resources
juce_add_binary_data(OuariconGrainScatter_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_link_libraries(OuariconGrainScatter
    PRIVATE
        OuariconGrainScatter_UIResources
)

target_compile_definitions(OuariconGrainScatter
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
