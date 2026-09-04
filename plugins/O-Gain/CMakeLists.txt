cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Gain
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OGan
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Gain${OUARICON_DEV_SUFFIX}"
    VERSION 1.3.3
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-Gain
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-Gain
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Gain
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

# Generate JuceHeader.h (JUCE 8 requirement - CRITICAL: must be after target_link_libraries)
juce_generate_juce_header(O-Gain)

# WebView UI Resources
# NO NAMESPACE ARGUMENT — this target takes the default BinaryData namespace
# and works only because it is the only juce_add_binary_data target in this
# plugin. v1.3.0 added js/app.js and js/i18n.js to this EXISTING list; a second
# target would collide on the BinaryData namespace and fail in a way that reads
# like something else entirely (critical_dual_binary_data_namespace_collision).
#
# Filenames carry no hyphen: juce_add_binary_data STRIPS hyphens rather than
# converting them to underscores (critical_binary_data_strips_hyphens).
juce_add_binary_data(O-Gain_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/images/paper1.jpg
        Source/ui/public/images/shell_decoration.png
)

target_link_libraries(O-Gain
    PRIVATE
        O-Gain_UIResources
)

# Compile definitions
target_compile_definitions(O-Gain
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
