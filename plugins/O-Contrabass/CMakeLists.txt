cmake_minimum_required(VERSION 3.15)

# O-Contrabass — Specialized 4-String Contrabass Physical Model
# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Contrabass
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OCbs
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Contrabass${OUARICON_DEV_SUFFIX}"

    # Synthesizer configuration (juce8-critical-patterns #22)
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # WebView UI configuration (Stage 3 will populate; flags must be present from Stage 1)
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-Contrabass PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp

    # Phase 2.1a — single-string E1 voice + bass-tuned bow-friction + waveguide.
    # HyperbolicFriction.h, BowModel.h, WaveguideString.h, BowedContrabassVoice.h,
    # OContrabassMPESynthesiser.h are header-only / picked up via include path.
    Source/BowedContrabassVoice.cpp
    Source/DSP/BowModel.cpp
    Source/DSP/WaveguideString.cpp

    # scala-tuning-engine — Pattern B (explicit file refs, sibling-plugin convention)
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

# Include paths
target_include_directories(O-Contrabass PRIVATE
    Source
    Source/DSP
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)

# Required JUCE modules (canonical list — matches O-Bells / O-Bowed)
target_link_libraries(O-Contrabass
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

# Generate JuceHeader.h (JUCE 8 requirement — MUST come after target_link_libraries)
juce_generate_juce_header(O-Contrabass)

# Compile definitions
target_compile_definitions(O-Contrabass
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1   # MEMORY: required when NEEDS_WEBVIEW2 TRUE
        JUCE_USE_CURL=0
)

# Module system integration
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# note-expression — Pattern A (per-format VST3 routing required)
ouaricon_add_module(O-Contrabass note-expression)

# ----------------------------------------------------------------------------
# Phase 2.1a render-harness binary (Phase 2.1 stability gate, RESEARCH §Q1).
# Off by default; enable with -DOUARICON_BUILD_TESTS=ON.
# ----------------------------------------------------------------------------
option(OUARICON_BUILD_TESTS "Build test harness binaries (e.g. O-Contrabass-render-test)" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
