# ==============================================================================
#   O-Granulator — plugins/O-Granulator/CMakeLists.txt
#
#   Landed in Stage 1 from .planning/mockups/v2-CMakeLists.txt (v2, ui-finalization).
#   Stage 1 ships the GenericAudioProcessorEditor; Source/PluginEditor.cpp is a
#   placeholder TU so Stage 3 swaps file contents, not CMake. Stage 3 verifies
#   the WebView block. It is a COMPLETE plugin CMakeLists rather than a
#   snippet because the WebView requirements reach into the plugin target call
#   itself (NEEDS_WEB_BROWSER / NEEDS_WEBVIEW2) and the compile definitions.
#
#   Suite requirements carried here:
#     - NEEDS_WEB_BROWSER TRUE  (VST3 does not appear in hosts without it)
#     - NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
#       (Windows: without static linking the WebView silently renders blank)
#     - juce_generate_juce_header()   (PluginEditor.h includes <JuceHeader.h>)
#     - C++20 (grain-engine requires it)
#     - modules via OuariconModules.cmake: grain-engine, reverb-unit,
#       preset-manager, eb-garamond, pitch-collections
#     - ONE juce_add_binary_data target with a DISTINCT NAMESPACE + HEADER_NAME
#       (critical_dual_binary_data_namespace_collision); every UI file the
#       resource provider serves is in it, js/i18n.js included (check-i18n [8]).
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconGranulator
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuGn                       # verified unused across plugins/*/CMakeLists.txt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Granulator${OUARICON_DEV_SUFFIX}"
    VERSION 0.1.0                          # JUCE reads VERSION (PLUGIN_VERSION is ignored)
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_compile_features(OuariconGranulator PRIVATE cxx_std_20)

# PluginEditor.cpp belongs to the PLUGIN target only — the offline render
# harness compiles PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and must never
# see WebView types.
target_sources(OuariconGranulator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(OuariconGranulator
    PRIVATE
        Source
)

# ── Shared modules ──────────────────────────────────────────────────────────
# grain-engine 1.1.0: MipRing<2>/BandlimitedReader, GrainPool, GrainScheduler,
# CounterRng, WindowLut (header-only, C++20).
ouaricon_add_module(OuariconGranulator grain-engine)
# reverb-unit 1.1.0: the output reverb (ARCHITECTURE gap G4 — raw-atomic bind).
ouaricon_add_module(OuariconGranulator reverb-unit)
# preset-manager 1.1.x: OuariconPresetManager.h onto the include path. Its JS is
# ALSO copied to Source/ui/public/modules/ by this helper — that copy is a
# gitignored build artefact and is NOT what ships: the binary-data target below
# embeds the module file directly from ${CMAKE_SOURCE_DIR}/modules.
ouaricon_add_module(OuariconGranulator preset-manager)
# eb-garamond 1.0.0: no C++ — registered for module tracking; the stylesheet and
# the three woff2 faces are embedded directly below (the module's documented
# "direct embed" integration) and served under /css/ and /fonts/.
ouaricon_add_module(OuariconGranulator eb-garamond)
# pitch-collections 1.0.0 (modules/tuning/pitch-collections): extracted from
# O-GrainScatter 3.5.1 in Stage 1. Choice index -> Id mapping lives in
# GranulatorParamIds.h (scaleChoiceToId) — do NOT fork a copy into Source/.
ouaricon_add_module(OuariconGranulator pitch-collections)

target_link_libraries(OuariconGranulator
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
        juce::juce_gui_extra              # WebBrowserComponent
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(OuariconGranulator)

# ── Embedded WebView UI resources ───────────────────────────────────────────
# Symbols: hyphens are STRIPPED, not converted (critical_binary_data_strips_hyphens):
#   index_html, i18n_js, index_js (js/juce/index.js), check_native_interop_js,
#   presetmanager_js, ebgaramond_css, EBGaramondRegular_woff2,
#   EBGaramondItalic_woff2, EBGaramondBold_woff2.
# The page is ONE file (inline <style> + inline module controller); i18n.js is
# the only separate authored UI file. Every SOURCES entry has a getResource()
# branch in PluginEditor.cpp and vice versa.
juce_add_binary_data(OuariconGranulator_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
)

target_link_libraries(OuariconGranulator
    PRIVATE
        OuariconGranulator_UIResources
)

target_compile_definitions(OuariconGranulator
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=1
        # Set together with NEEDS_WEBVIEW2 — without static linking the Windows
        # WebView silently renders blank in DAW hosts (critical_webview2_static_linking).
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Offline render-test harness + gates. Off by default.
#
# ouaricon_add_processor_console() does NOT propagate the plugin target's
# cxx_std_20 (consoles build at gnu++17 otherwise; grain-engine needs 20), so
# every console gets the feature explicitly.
option(OUARICON_BUILD_TESTS "Build O-Granulator test targets" OFF)
if(OUARICON_BUILD_TESTS)
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(OuariconGranulator ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    target_compile_features(O-Granulator-param-dump PRIVATE cxx_std_20)

    # Stage 1 gate (G1-G6 params, D1-D4 dest table, B1-B3 buses / pass-through /
    # latency, S1-S4 state, P1 scale map). Goldens are spec-derived:
    #   python3 plugins/O-Granulator/tests/golden/extract_spec.py
    #   O-Granulator-render-harness            (exit 0 = all rows PASS)
    ouaricon_add_processor_console(OuariconGranulator ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-harness)
    target_compile_features(O-Granulator-render-harness PRIVATE cxx_std_20)
    target_compile_definitions(O-Granulator-render-harness PRIVATE
        GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden")
endif()
