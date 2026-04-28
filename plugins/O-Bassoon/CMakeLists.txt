cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-Bassoon - Modal Synthesis Bassoon (Microtonal Long-Tone Synthesizer)
# Stage 1 (Foundation): silent shell with APVTS, headless TuningEngine + NE drain.
# First audio: Phase 2.1.
juce_add_plugin(O-Bassoon
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OBsn
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bassoon${OUARICON_DEV_SUFFIX}"
    PLUGIN_VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-Bassoon
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BassoonSound.h
        Source/BassoonVoice.h
        Source/BassoonVoice.cpp
        # Tuning module files (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

# VST3 Note Expression microtonal support (Dorico) — D1: ouaricon_add_module ONLY
ouaricon_add_module(O-Bassoon note-expression)

# Include paths
target_include_directories(O-Bassoon
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)

# Required JUCE modules
target_link_libraries(O-Bassoon
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

# Generate JuceHeader.h (JUCE 8 requirement - MUST come after target_link_libraries)
juce_generate_juce_header(O-Bassoon)

# Licensing module (compile-flag gated, OFF for local dev)
if(OUARICON_LICENSING)
    ouaricon_add_module(O-Bassoon licensing)
    target_compile_definitions(O-Bassoon PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-Bassoon PRIVATE juce::juce_cryptography)
endif()

# Compile definitions
target_compile_definitions(O-Bassoon
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
