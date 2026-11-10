# ==============================================================================
#   O-GranulatorInstrument — plugins/O-GranulatorInstrument/CMakeLists.txt
#   (v1 mockup, ui-finalization)
#
#   Stage 1 (foundation-agent) lands this whole file; Stage 3 (gui-agent) only
#   verifies the WebView block. It is a COMPLETE plugin CMakeLists rather than a
#   snippet because the WebView requirements reach into the plugin target call
#   itself (NEEDS_WEB_BROWSER / NEEDS_WEBVIEW2) and the compile definitions.
#   (Comments here never spell the JUCE plugin-target call with its opening
#   parenthesis: resolve-target.sh greps for it and a comment match yields a
#   garbage target — pattern_resolve_target_matches_juce_add_plugin_in_comment.)
#
#   Suite requirements carried here:
#     - IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE (instrument; MPE + note expression)
#     - NEEDS_WEB_BROWSER TRUE  (VST3 does not appear in hosts without it)
#     - NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
#       (Windows: without static linking the WebView silently renders blank)
#     - juce_generate_juce_header()   (PluginEditor.h includes <JuceHeader.h>)
#     - C++20 (grain-engine requires it)
#     - modules via OuariconModules.cmake: grain-engine, pitch-collections,
#       reverb-unit, preset-manager, eb-garamond, note-expression,
#       webview-drop-streaming (ROADMAP Stage 1)
#     - ONE UI juce_add_binary_data target with a DISTINCT NAMESPACE + HEADER_NAME,
#       and a SECOND target for the factory sources with its own namespace
#       (critical_dual_binary_data_namespace_collision); every UI file the
#       resource provider serves is in the UI target, js/i18n.js included
#       (check-i18n [8]).
#
#   Buses (declared in the processor, not here): stereo output + ONE optional
#   stereo "Sidechain" input for Record-in, typed kAux on VST3 via
#   getPluginHasMainInput() == false, active by default (Logic only offers its
#   Side Chain menu for an active bus). Fallback if a host rejects the input
#   bus on an instrument: a format-conditional BusesProperties (ARCHITECTURE
#   Risks, "Instrument + sidechain bus").
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconGranulatorInstrument
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuGI                       # verified unused across plugins/*/CMakeLists.txt (2026-11-07)
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-GranulatorInstrument${OUARICON_DEV_SUFFIX}"
    VERSION 0.1.0                          # JUCE reads VERSION (PLUGIN_VERSION is ignored)
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_compile_features(OuariconGranulatorInstrument PRIVATE cxx_std_20)

# PluginEditor.cpp belongs to the PLUGIN target only — an offline render
# harness compiles PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and must never
# see WebView types.
target_sources(OuariconGranulatorInstrument
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/SlotLoader.cpp
        Source/dsp/Engine.cpp
        # Phase 2.1 dev loader (D13). The name matches "PluginEditor", so the
        # consoles (harness, param-dump) drop it (ParamDump.cmake editor filter).
        Source/PluginEditorDevLoader.cpp
)

target_include_directories(OuariconGranulatorInstrument
    PRIVATE
        Source
        Source/dsp
        Source/dsp/forked
)

# ── Shared modules ──────────────────────────────────────────────────────────
# grain-engine 1.1.0: BandlimitedReader / MipRing, GrainPool, GrainScheduler,
# CounterRng, WindowLut (header-only, C++20).
ouaricon_add_module(OuariconGranulatorInstrument grain-engine)
# pitch-collections 1.0.0: the 32-entry sN_scale tables, the shared custom
# scale parser (edo N, badEdo), toScala. Never fork a copy into Source/.
ouaricon_add_module(OuariconGranulatorInstrument pitch-collections)
# reverb-unit 1.2.0: the output reverb (atomic bind, as the parent).
ouaricon_add_module(OuariconGranulatorInstrument reverb-unit)
# preset-manager: OuariconPresetManager.h onto the include path. Its JS is ALSO
# copied to Source/ui/public/modules/ by this helper — a gitignored build
# artefact that does NOT ship: the UI binary-data target (lands in Phase 3.1)
# embeds the module file directly from ${CMAKE_SOURCE_DIR}/modules.
ouaricon_add_module(OuariconGranulatorInstrument preset-manager)
# eb-garamond 1.0.0: no C++ — registered for module tracking; the stylesheet
# and the three woff2 faces are embedded by the UI binary-data target in
# Phase 3.1 and served under /css/ and /fonts/.
ouaricon_add_module(OuariconGranulatorInstrument eb-garamond)
# note-expression 1.1.2: Dorico microtonal note expression (D5). The
# plugin-local GiVst3Extensions (Source/PluginProcessor.h), a subclass of the
# module's VST3Extensions, returns getPluginHasMainInput() = false, which types
# the sidechain as kAux (D9). Follow the module README for the per-format VST3
# routing.
ouaricon_add_module(OuariconGranulatorInstrument note-expression)
# webview-drop-streaming 1.0.0: lane drag-drop (WKWebView strips paths). Adds
# WebViewDropStreaming.h + DropSessionGuard.h (editor-only); its JS is embedded
# from modules/ by the UI binary-data target in Phase 3.1 and served at
# /js/webview-drop-streaming.js.
ouaricon_add_module(OuariconGranulatorInstrument webview-drop-streaming)

target_link_libraries(OuariconGranulatorInstrument
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats          # file slots, FLAC embed, Record-in WAV
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra              # WebBrowserComponent
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(OuariconGranulatorInstrument)

# ── Embedded WebView UI resources ───────────────────────────────────────────
# Phase 3.1: the UI binary-data target and its link line are DEFERRED (D8).
# Stage 1 ships the GenericAudioProcessorEditor and no file exists under
# Source/ui/public/ yet, so these SOURCES cannot resolve; Phase 3.1 uncomments
# both blocks together with the WebView editor and the UI files.
#
# Symbols: hyphens are STRIPPED, not converted (critical_binary_data_strips_hyphens):
#   index_html, i18n_js, index_js (js/juce/index.js), check_native_interop_js,
#   presetmanager_js, webviewdropstreaming_js, ebgaramond_css,
#   EBGaramondRegular_woff2, EBGaramondItalic_woff2, EBGaramondBold_woff2.
# The page is ONE file (inline <style> + inline module controller); i18n.js is
# the only separate authored UI file. Every SOURCES entry has a getResource()
# branch in PluginEditor.cpp and vice versa.
# juce_add_binary_data(OuariconGranulatorInstrument_UIResources
#     NAMESPACE UIBinaryData
#     HEADER_NAME UIBinaryData.h
#     SOURCES
#         Source/ui/public/index.html
#         Source/ui/public/js/i18n.js
#         Source/ui/public/js/juce/index.js
#         Source/ui/public/js/juce/check_native_interop.js
#         ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js
#         ${CMAKE_SOURCE_DIR}/modules/core/webview-drop-streaming/js/webview-drop-streaming.js
#         ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
#         ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
#         ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
#         ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
# )
#
# target_link_libraries(OuariconGranulatorInstrument
#     PRIVATE
#         OuariconGranulatorInstrument_UIResources
# )

# ── Factory sources (Design Sync S10; CONTEXT-2.6 D33, PLAN-2.6 T10) ─────────
# Embedded BinaryData FLAC, addressed as factory:<id>, budget ≤ 8 MB in total.
# A SECOND binary-data target with its OWN namespace and header — sharing the
# default BinaryData namespace with the UI target is a link-time collision.
# Resource names keep [A-Za-z0-9_] (hyphens are STRIPPED): ids use
# underscores. Linked BEFORE the console block so the harness / param-dump
# inherit it (ParamDump.cmake carries the plugin's non-JUCE link targets).
# 2.6: two generated stubs (render harness --gen-factory; pins in
# Source/dsp/GiFactory.h + tests/golden/factory-stubs.sha256). Stage 4
# replaces them by id with the curated 6–8 licence-clean sources.
juce_add_binary_data(OuariconGranulatorInstrument_FactorySources
    NAMESPACE FactorySourceData
    HEADER_NAME FactorySourceData.h
    SOURCES
        Resources/factory/stub_tone.flac
        Resources/factory/stub_noise.flac
)
target_link_libraries(OuariconGranulatorInstrument PRIVATE OuariconGranulatorInstrument_FactorySources)

target_compile_definitions(OuariconGranulatorInstrument
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=1
        # Set together with NEEDS_WEBVIEW2 — without static linking the Windows
        # WebView silently renders blank in DAW hosts (critical_webview2_static_linking).
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Gate-only switch (Stage 1 PLAN P7): builds the plugin with the Sidechain input
# INACTIVE by default so pluginval / auval exercise the disabled-bus arm. The
# shipping build leaves it OFF; the processor defaults OGI_SC_DEFAULT_ACTIVE to 1
# and the consoles never inherit this definition.
option(OGI_GATE_SC_DEFAULT_OFF "Gate-only: Sidechain inactive by default. Never ship." OFF)
if(OGI_GATE_SC_DEFAULT_OFF)
    target_compile_definitions(OuariconGranulatorInstrument PRIVATE OGI_SC_DEFAULT_ACTIVE=0)
    message(WARNING "[O-GranulatorInstrument] OGI_GATE_SC_DEFAULT_OFF=ON: gate variant, do not ship")
endif()

# Phase 2.1 dev-loader editor (D13): file buttons over the generic editor until
# the WebView lands. PLUGIN TARGET ONLY — consoles never see the define.
# Removed in Phase 3.1 together with Source/PluginEditorDevLoader.* and the
# createEditor() branch.
option(OGI_DEV_LOADER "Dev loader editor until Phase 3.1" ON)
if(OGI_DEV_LOADER)
    target_compile_definitions(OuariconGranulatorInstrument PRIVATE OGI_DEV_LOADER=1)
endif()

# Offline render-test harness + gates (Stage 1/2). Off by default.
# ouaricon_add_processor_console() does NOT propagate cxx_std_20 — set it on
# every console target explicitly (O-Granulator precedent).
option(OUARICON_BUILD_TESTS "Build O-GranulatorInstrument test targets" OFF)
if(OUARICON_BUILD_TESTS)
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(OuariconGranulatorInstrument ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_compile_features(O-GranulatorInstrument-param-dump PRIVATE cxx_std_20)

    # Stage 1 gate (G1-G6 params, D1-D5 dest table, SRC1 sources, B1/B3 buses +
    # latency, M1 MIDI script, R1 sidechain-zero render, S1-S5 state, H1/H2
    # hooks + kAux). Goldens are spec-derived:
    #   python3 plugins/O-GranulatorInstrument/tests/golden/extract_spec.py
    #   O-GranulatorInstrument-render-harness   (exit 0 = all rows PASS)
    ouaricon_add_processor_console(OuariconGranulatorInstrument ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-harness)
    target_compile_features(O-GranulatorInstrument-render-harness PRIVATE cxx_std_20)
    # OGI_TEST_HOOKS=1 on THIS target only (loadSlotSync stubs); the plugin and
    # the param-dump console compile every hook out.
    target_compile_definitions(O-GranulatorInstrument-render-harness PRIVATE
        GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden"
        OGI_TEST_HOOKS=1)
endif()
