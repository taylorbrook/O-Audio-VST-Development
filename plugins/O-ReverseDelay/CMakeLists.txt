cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconReverseDelay
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE ORvD
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-ReverseDelay${OUARICON_DEV_SUFFIX}"
    # JUCE reads VERSION (never PLUGIN_VERSION — that keyword is silently ignored
    # and the bundle ships as 1.0.0). Bumping it also invalidates the factory
    # preset .factory-version sentinel, which is what makes preset table edits
    # actually reach disk instead of being a silent no-op — v1.1.0 adds the four
    # randomisation keys to all eight presets, so this bump is load-bearing.
    VERSION 1.10.0
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# PluginEditor.cpp belongs to the PLUGIN target only — the Stage-2 render harness
# compiles PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and must never see WebView
# types (pattern_render_harness_breaks_on_webview_editor).
target_sources(OuariconReverseDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(OuariconReverseDelay
    PRIVATE
        Source
        # OuariconPresetManager is header-only (all impl inline) — included by
        # path, never vendored into Source/. The render harness picks this up for
        # free via $<TARGET_PROPERTY:OuariconReverseDelay,INCLUDE_DIRECTORIES>,
        # so probe N audits through the SHIPPING manager, not a re-typed table.
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
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

# ── Embedded WebView UI resources ───────────────────────────────────────────
# NAMESPACE *and* HEADER_NAME are both distinct from the default BinaryData /
# BinaryData.h. No second binary-data target exists today, but a Stage-4 preset
# or sample target defaulting to BinaryData would duplicate-symbol at link time —
# a distinct HEADER_NAME alone does NOT prevent it
# (critical_dual_binary_data_namespace_collision).
juce_add_binary_data(OuariconReverseDelay_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        # v1.9.0 — hover-help copy, English + French. NO HYPHEN in the filename:
        # juce_add_binary_data STRIPS hyphens rather than converting them to
        # underscores (critical_binary_data_strips_hyphens), so one combined
        # file for both languages sidesteps the question a second i18n-fr.js
        # would have raised. Symbol: UIBinaryData::i18n_js.
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/birds.png
        # Stage 4: the shared preset-manager JS module, embedded into the EXISTING
        # UIResources target (C3 — a second juce_add_binary_data target defaulting
        # to NAMESPACE BinaryData duplicate-symbols against UIBinaryData).
        # Symbol: UIBinaryData::presetmanager_js — juce_add_binary_data STRIPS the
        # hyphen, it does NOT convert it to an underscore
        # (critical_binary_data_strips_hyphens). Served at /js/preset-manager.js.
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js
)

target_link_libraries(OuariconReverseDelay
    PRIVATE
        OuariconReverseDelay_UIResources
)

target_compile_definitions(OuariconReverseDelay
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=1
        # Set together with NEEDS_WEBVIEW2 — without static linking the Windows
        # WebView silently renders blank in DAW hosts
        # (critical_webview2_static_linking). Wired now, before the first
        # Windows CI build, rather than retrofitted.
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Stage-2 offline render-test harness (DSP correctness gate — probes A..E in
# Phase 2.1). Off by default; enable with -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-ReverseDelay render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
