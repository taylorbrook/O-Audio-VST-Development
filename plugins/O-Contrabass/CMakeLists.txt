cmake_minimum_required(VERSION 3.15)

# O-Contrabass — Specialized 4-String Contrabass Physical Model
# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-Contrabass
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OCbs
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Contrabass${OUARICON_DEV_SUFFIX}"

    # VERSION is the keyword JUCE actually reads (NOT PLUGIN_VERSION, which JUCE
    # silently ignores). Without it the bundle Info.plist reports JUCE's 1.0.0
    # default, so v1.0.0 was correct only by coincidence and any future bump
    # would never have reached the built artefact.
    VERSION 1.7.1

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
    # WaveguideString.h, BowedContrabassVoice.h, OContrabassMPESynthesiser.h
    # are header-only / picked up via include path.
    # Phase 2.1b — HyperbolicFriction.h + BowModel.{h,cpp} now come from the
    # shared bow-friction module via ouaricon_add_module below.
    Source/BowedContrabassVoice.cpp
    Source/DSP/WaveguideString.cpp
    Source/DSP/BodyResonator.cpp                 # Phase 2.5 NEW

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
    # Stage 3 Task 8 (D6) — preset-manager v1.0.4, canonical module include
    # (O-Wind pattern; header-only OuariconPresetManager.h — stays current,
    # no vendored-copy drift).
    ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
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

# -----------------------------------------------------------------------------
# WebView UI resources (Stage 3) — embedded as binary data, served by the
# editor's resource provider (bare-path lookups: "/", "/index.html",
# "/js/juce/*.js").
#
# GOTCHA (MEMORY: critical_dual_binary_data_namespace_collision): if a SECOND
# juce_add_binary_data target is ever added, give one a distinct NAMESPACE —
# two targets defaulting to `BinaryData` collide at link time. O-Contrabass
# has only this one, so the default BinaryData namespace is fine and matches
# the `#include "BinaryData.h"` in PluginEditor.cpp.
# -----------------------------------------------------------------------------
juce_add_binary_data(OContrabass_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        # Stage 3 Tasks 8/9 — canonical shared-module JS/CSS embedded directly
        # from the module tree (O-Wind pattern: no vendored copy → no drift).
        # Served by the resource provider as /js/preset-manager.js,
        # /js/tuning-panel.js, /css/tuning-panel.css.
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Contrabass
    PRIVATE
        OContrabass_UIResources
)

# Module system integration
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# note-expression — Pattern A (per-format VST3 routing required)
ouaricon_add_module(O-Contrabass note-expression)

# Phase 2.1b — extracted shared friction module (bass setters in prepareToPlay).
ouaricon_add_module(O-Contrabass bow-friction)

# ----------------------------------------------------------------------------
# Phase 2.1a render-harness binary (Phase 2.1 stability gate, RESEARCH §Q1).
# Off by default; enable with -DOUARICON_BUILD_TESTS=ON.
# ----------------------------------------------------------------------------
option(OUARICON_BUILD_TESTS "Build test harness binaries (e.g. O-Contrabass-render-test)" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()

# ----------------------------------------------------------------------------
# Stage 4 (Task 5) — ship the Dorico Playback Template bundle in the package.
# Without this rule the EndpointConfig / PlaybackTemplateSpec / .doricolib +
# install docs never leave the source tree, so `cmake --install` (and any
# packaging step) would omit the Dorico integration entirely.
#
# Installs the whole Resources/dorico/ subtree (no trailing slash on the source
# → the `dorico` folder itself is preserved under the destination):
#   <prefix>/share/O-Contrabass/dorico/EndpointConfigs/O-Contrabass/...
#   <prefix>/share/O-Contrabass/dorico/PlaybackTemplateSpecs/O-Contrabass/...
#   <prefix>/share/O-Contrabass/dorico/INSTALL-DORICO.md, SMOKE-TEST.md
# End users then follow INSTALL-DORICO.md to copy the three files into their
# Dorico user library. Additive only — does not touch the plugin target,
# compile definitions, or WebView config.
# ----------------------------------------------------------------------------
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/dorico"
    DESTINATION "share/O-Contrabass"
    COMPONENT Dorico)
