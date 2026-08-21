cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Single source of truth for the version. The test harness needs the same value in
# JucePlugin_VersionString: the processor ctor runs initializeFactoryPresets(), whose
# .factory-version sentinel compares against it, so a harness left on a stale literal
# would restamp the user's real factory-preset directory with the wrong version.
set(OMBC_VERSION 1.8.0)

# Plugin Configuration
juce_add_plugin(O-MultiBandCompressor
    VERSION ${OMBC_VERSION}
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OMbc
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-MultiBandCompressor${OUARICON_DEV_SUFFIX}"
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Preset manager module (v1.5.0).
# MUST run before juce_add_binary_data below: ouaricon_add_module copies
# preset-manager.js into Source/ui/public/modules/ at configure time, and the
# binary-data target needs that file to already exist when it is declared.
ouaricon_add_module(O-MultiBandCompressor preset-manager)

# WebView UI Resources (Phase 5.1)
juce_add_binary_data(O-MultiBandCompressor_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/preset-manager.js
)

# Source files
target_sources(O-MultiBandCompressor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-MultiBandCompressor
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-MultiBandCompressor
    PRIVATE
        O-MultiBandCompressor_UIResources
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
juce_generate_juce_header(O-MultiBandCompressor)

# v1.5.0 factory-preset verification harness — off by default,
# enable with -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-MultiBandCompressor preset-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()

# Compile definitions
target_compile_definitions(O-MultiBandCompressor
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
