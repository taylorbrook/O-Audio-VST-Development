cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simplePhysicalModelSynth - Pedagogical Physical Modeling Synthesizer
# Stage 1 (Foundation): silent synth shell with full 17-param APVTS + state persistence.
# 16-voice juce::Synthesiser + header-only silent PhysicalModelVoice. First audio:
# Stage 2 / Phase 2.1. WebView UI: Stage 3 (GenericAudioProcessorEditor placeholder for now).
#
# Diverges from the O-simpleFM template in four deliberate ways (RESEARCH §1):
#   D1: no juce_add_binary_data / WebView editor / ui/ tree (Generic editor placeholder).
#   D2: no preset-manager module — plain APVTS XML state round-trip.
#   D3: percent params stored 0–100 (NOT O-simpleFM's 0–1 unitRange) — zero-drift contract.
#   D4: no dsp::Oversampling; setLatencySamples(0).
juce_add_plugin(O-simplePhysicalModelSynth
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OsPM
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simplePhysicalModelSynth${OUARICON_DEV_SUFFIX}"
    VERSION "1.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files. Stage 3 adds the WebView editor (PluginEditor.{h,cpp}); createEditor is
# declared in PluginProcessor.h and defined in PluginProcessor.cpp behind the
# #if JUCE_WEB_BROWSER seam (the guarded #include "PluginEditor.h" keeps the
# render-harness TU, compiled at JUCE_WEB_BROWSER=0, free of WebView types).
# PhysicalModelVoice.h is header-only (no extra .cpp).
target_sources(O-simplePhysicalModelSynth
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/FactoryPresets.cpp
        Source/FactoryPresets.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PhysicalModelVoice.h
        # Stage 2 DSP (header-only): exciters, resonators, shared helpers, viz tap.
        Source/OnePoleLPF.h
        Source/PositionComb.h
        Source/StringResonator.h
        Source/ModalResonator.h
        Source/PluckExciter.h
        Source/StrikeExciter.h
        Source/BowExciter.h
        Source/BowNoiseGenerator.h
        Source/VizTap.h
)

# Include paths
target_include_directories(O-simplePhysicalModelSynth
    PRIVATE
        Source
)

# Stage 2 (DSP): shared bow-friction module — HyperbolicFriction (memoryless STK
# Stribeck curve) drives the Bow exciter's KS-loop friction injection (RESEARCH F2/R2).
# Pure value-class C++, zero deps, cpp_standard 20; already consumed by O-Bowed +
# O-Contrabass. Adds modules/synthesis/bow-friction/cpp to the plugin's PRIVATE
# include dirs — which the render-harness inherits via INCLUDE_DIRECTORIES, so the
# harness resolves "HyperbolicFriction.h" too (no extra harness wiring needed).
ouaricon_add_module(O-simplePhysicalModelSynth bow-friction)

# Stage 3 (GUI) / D4: preset-manager module. Adds OuariconPresetManager.h to the include
# path and copies preset-manager.js into Source/ui/public/modules/. MUST precede
# juce_add_binary_data below (which embeds the copied JS).
ouaricon_add_module(O-simplePhysicalModelSynth preset-manager)

# WebView UI resources (embedded as BinaryData) — Stage 3 GUI. Single binary-data target,
# so the default `BinaryData` namespace is fine (no audio samples embedded here).
juce_add_binary_data(O-simplePhysicalModelSynth_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/preset-manager.js
)

# Required JUCE modules. juce_dsp is linked NOW even though the DSP (DelayLine<Thiran>,
# modal biquad bank, dsp::FFT) lands in Stage 2 — the module dependency belongs at the
# foundation. O-simplePhysicalModelSynth_UIResources is linked FIRST (Stage 3 WebView UI).
target_link_libraries(O-simplePhysicalModelSynth
    PRIVATE
        O-simplePhysicalModelSynth_UIResources
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
juce_generate_juce_header(O-simplePhysicalModelSynth)

# Compile definitions.
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct cross-platform
# config; the WebView itself (binary data + editor) is added in Stage 3.
# JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 is mandatory alongside NEEDS_WEBVIEW2 TRUE
# (COMPAT-02 — without it Windows silently shows a blank WebView).
target_compile_definitions(O-simplePhysicalModelSynth
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Stage 2 DSP correctness gate (offline render harness). Off by default;
# enable with -DOUARICON_BUILD_TESTS=ON. At Stage 1 the harness is a build/link
# smoke-stub (no audio yet) that locks the JUCE_WEB_BROWSER=0 / no-editor seam early.
option(OUARICON_BUILD_TESTS "Build O-simplePhysicalModelSynth render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
