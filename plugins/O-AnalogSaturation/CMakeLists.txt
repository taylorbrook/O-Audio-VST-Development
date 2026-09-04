cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-AnalogSaturation
    VERSION 1.4.0
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OaSa
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-AnalogSaturation${OUARICON_DEV_SUFFIX}"
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-AnalogSaturation
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-AnalogSaturation
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-AnalogSaturation
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

# Generate JuceHeader.h (JUCE 8 requirement - CRITICAL: must come after target_link_libraries)
juce_generate_juce_header(O-AnalogSaturation)

# WebView UI Resources (Pattern 3: Binary data for embedded HTML/CSS/JS/images)
juce_add_binary_data(O-AnalogSaturation_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper1.jpg
        Source/ui/public/img/snake_mobot31753000317195_0068.png
        Source/ui/public/img/snake_mobot31753000317195_0070.png
        Source/ui/public/img/snake_NA_0145.png
        Source/ui/public/img/snake_snakesaustralia00kref_0145.png
)

target_link_libraries(O-AnalogSaturation
    PRIVATE
        O-AnalogSaturation_UIResources
)

# Compile definitions
target_compile_definitions(O-AnalogSaturation
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-AnalogSaturation test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-AnalogSaturation ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
