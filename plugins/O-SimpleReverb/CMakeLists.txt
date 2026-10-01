cmake_minimum_required(VERSION 3.15)

# Include Ouaricon Module System
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-SimpleReverb
    VERSION 2.0.0
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuSr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-SimpleReverb${OUARICON_DEV_SUFFIX}"
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-SimpleReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(O-SimpleReverb
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-SimpleReverb
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
juce_generate_juce_header(O-SimpleReverb)

# Add Ouaricon Preset Manager module
ouaricon_add_module(O-SimpleReverb preset-manager)

# WebView UI Resources
juce_add_binary_data(O-SimpleReverb_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/flora.png
        # v1.10.0 - bundled EB Garamond, direct embed from the shared
        # module; served by four getResource branches in PluginEditor.cpp.
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
)

target_link_libraries(O-SimpleReverb
    PRIVATE
        O-SimpleReverb_UIResources
)

# Compile definitions
target_compile_definitions(O-SimpleReverb
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-SimpleReverb test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-SimpleReverb ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    # Render check: factory TYPE recall and bank levels, click-free CHARACTER,
    # LOW CUT, TYPE, SIZE and DECAY moves, per-type level match and the level
    # law, VU peak hold, and (v2.0.0) the reverb engines - decay time against
    # target, early reflections, stereo, ring-out, no allocation.
    # See tests/render-check/main.cpp.
    ouaricon_add_processor_console(O-SimpleReverb ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-check/main.cpp render-check)
    # Mutants and the NaN injection hook exist in this target only (see
    # OSR_MUTANT in Source/PluginProcessor.h); the plugin is built without them.
    target_compile_definitions(O-SimpleReverb-render-check PRIVATE OSIMPLEREVERB_TEST_HOOKS=1)
endif()
