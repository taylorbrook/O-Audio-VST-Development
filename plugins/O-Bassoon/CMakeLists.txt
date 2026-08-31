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
    VERSION "1.2.0"
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
        Source/BassoonSynthesiser.h     # Phase 2.4 — voice manager subclass with active-cap (FUNC-02)
        Source/BassoonSynthesiser.cpp   # Header-inline override; .cpp pair per Phase 2.3 convention
        Source/ModeBank.h
        Source/ModeBank.cpp
        Source/Exciter.h         # Phase 2.4 retention (D6-rev-3) — Phase 2.3 voice does not call,
        Source/Exciter.cpp       # but member declared for attack-character morph re-introduction.
        Source/Vibrato.h         # Phase 2.3 — per-voice sine LFO + onset envelope
        Source/Vibrato.cpp
        Source/NoiseExciter.h    # Phase 2.3 — per-voice continuous filtered-noise excitation
        Source/NoiseExciter.cpp
        Source/FactoryPresets.h  # Stage 4 — 4 ROADMAP factory presets
        Source/FactoryPresets.cpp
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
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
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

# WebView UI Resources (Stage 3 — Pattern A: tuning-panel.{js,css} referenced from
# shared module path, NOT copied per-plugin, so upstream fixes propagate without re-copy)
juce_add_binary_data(O-Bassoon_UIResources
    SOURCES
        Resources/ui/index.html
        # v1.1.0 — the label table. EMBEDDED here AND served from a
        # getResource() branch in PluginEditor.cpp, in the same commit. A file
        # embedded but not served, or served but not embedded, is a 404 that
        # presents as a page stuck in English and nothing else; check-i18n
        # assertion 8 exists for exactly this pair.
        Resources/ui/js/i18n.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/fern.png
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Bassoon
    PRIVATE
        O-Bassoon_UIResources
)

# Compile definitions
target_compile_definitions(O-Bassoon
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Bassoon test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-Bassoon ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
