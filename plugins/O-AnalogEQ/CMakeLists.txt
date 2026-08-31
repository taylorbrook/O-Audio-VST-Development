cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OuariconAnalogEQ
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuAE
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-AnalogEQ${OUARICON_DEV_SUFFIX}"
    VERSION 1.3.1
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(OuariconAnalogEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(OuariconAnalogEQ
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OuariconAnalogEQ
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

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(OuariconAnalogEQ)

# Preset manager module
ouaricon_add_module(OuariconAnalogEQ preset-manager)

# WebView UI Resources
juce_add_binary_data(OuariconAnalogEQ_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/images/paper1.jpg
        Source/ui/public/images/flower_ferdinandibauer00baue_0021.png
        Source/ui/public/modules/preset-manager.js
)

target_link_libraries(OuariconAnalogEQ
    PRIVATE
        OuariconAnalogEQ_UIResources
)

# Compile definitions
target_compile_definitions(OuariconAnalogEQ
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-AnalogEQ test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(OuariconAnalogEQ ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
