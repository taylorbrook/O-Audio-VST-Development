cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconReverseDelay
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE ORvD
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-ReverseDelay${OUARICON_DEV_SUFFIX}"
    VERSION 1.0.0
)

target_sources(OuariconReverseDelay
    PRIVATE
        Source/PluginProcessor.cpp
)

target_include_directories(OuariconReverseDelay
    PRIVATE
        Source
)

target_link_libraries(OuariconReverseDelay
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

juce_generate_juce_header(OuariconReverseDelay)

target_compile_definitions(OuariconReverseDelay
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
)

# Stage-2 offline render-test harness (DSP correctness gate — probes A..E in
# Phase 2.1). Off by default; enable with -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-ReverseDelay render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
