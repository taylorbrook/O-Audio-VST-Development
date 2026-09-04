cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconGrainScatter
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuGS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-GrainScatter${OUARICON_DEV_SUFFIX}"
    VERSION 2.7.0
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

# WebView UI Resources
juce_add_binary_data(OuariconGrainScatter_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js
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

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-GrainScatter test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(OuariconGrainScatter ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
