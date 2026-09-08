cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Strata
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuSt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Strata${OUARICON_DEV_SUFFIX}"
    VERSION 1.0.0
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-Strata
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/FactoryPresets.cpp
        Source/StrataVoice.cpp
        Source/TuningEngine.cpp
        Source/ScaleGenerator.cpp
        Source/EmbeddedTunings.cpp
        Source/TuningExporter.cpp
        Source/dsp/WavetableGenerator.cpp
        Source/dsp/WavetableOscillator.cpp
        Source/dsp/SubOscillator.cpp
        Source/dsp/NoiseGenerator.cpp
        Source/dsp/SVFFilter.cpp
        Source/dsp/DistortionProcessor.cpp
        Source/dsp/DelayProcessor.cpp
        Source/dsp/ReverbProcessor.cpp
        Source/dsp/EQProcessor.cpp
        Source/dsp/EnsembleChorus.cpp
        Source/dsp/LFO.cpp
        Source/dsp/ModulationMatrix.cpp
)

# Include paths
target_include_directories(O-Strata
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-Strata
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_cryptography           # SHA-256 for FUNC-08 path-linked imports (Stage 4.1)
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

# Generate JuceHeader.h (JUCE 8 requirement — after target_link_libraries)
juce_generate_juce_header(O-Strata)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Strata note-expression)

# WebView UI Resources — exactly ONE binary-data target (two collide on the
# BinaryData namespace). Stage 3 adds js/geometry-view.js + the shell PNG HERE.
juce_add_binary_data(O-Strata_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_link_libraries(O-Strata
    PRIVATE
        O-Strata_UIResources
)

# Compile definitions
target_compile_definitions(O-Strata
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Strata test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory. Most of O-Strata's 219 parameter IDs are
    # string-concatenated (createOscParameters / mod-matrix loops) and appear
    # nowhere in the source as literals — the runtime walk is the only way to
    # enumerate them. Expect 219 rows; diff vs O-Prism's params.tsv = -2 +48.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-Strata ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
