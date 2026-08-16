cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OuariconTapestop
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OTsp
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Tapestop${OUARICON_DEV_SUFFIX}"
    # JUCE reads VERSION (never PLUGIN_VERSION — that keyword is silently
    # ignored and the bundle ships as 1.0.0).
    VERSION 1.0.0
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# PluginEditor.cpp belongs to the PLUGIN target only — the Stage-2 render
# harness compiles PluginProcessor.cpp with JUCE_WEB_BROWSER=0 and must never
# see WebView types (pattern_render_harness_breaks_on_webview_editor).
target_sources(OuariconTapestop
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(OuariconTapestop
    PRIVATE
        Source
)

target_link_libraries(OuariconTapestop
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

juce_generate_juce_header(OuariconTapestop)

# Shared preset persistence (Stage 4) — include-path wiring, never a vendored
# copy (O-AnalogEQ reference). Also configure-copies the module JS to
# Source/ui/public/modules/, which is this plugin's real UI root — the copy is
# the file juce_add_binary_data embeds below. Header-only module compiled into
# the processor TU, so the render harness (which borrows this target's include
# dirs) sees it too.
ouaricon_add_module(OuariconTapestop preset-manager)

# WebView UI resources (Stage 3). Distinct NAMESPACE/HEADER_NAME so a Stage-4
# asset target defaulting to `BinaryData` can never duplicate-symbol
# (critical_dual_binary_data_namespace_collision). NOTE: juce_add_binary_data
# STRIPS hyphens from filenames — the envelope module is deliberately named
# envelope_editor.js with an underscore so its symbol is envelope_editor_js
# (critical_binary_data_strips_hyphens). Linked PRIVATE to the plugin target
# only — the render harness never sees it.
juce_add_binary_data(OuariconTapestop_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/envelope_editor.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        # Module JS (configure-copied by ouaricon_add_module above). Hyphen in
        # the filename is the module's canonical name; juce_add_binary_data
        # STRIPS it, so the symbol is UIBinaryData::presetmanager_js
        # (critical_binary_data_strips_hyphens).
        Source/ui/public/modules/preset-manager.js
        Source/ui/public/img/shell.png
)

target_link_libraries(OuariconTapestop
    PRIVATE
        OuariconTapestop_UIResources
)

target_compile_definitions(OuariconTapestop
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

# Stage-2 offline render-test harness (DSP correctness gate). Off by default;
# enable with -DOUARICON_BUILD_TESTS=ON. The harness directory lands in
# Stage 2 — the EXISTS guard keeps a global -DOUARICON_BUILD_TESTS=ON configure
# from failing on this plugin until then.
option(OUARICON_BUILD_TESTS "Build O-Tapestop render-test harness" OFF)
if(OUARICON_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/CMakeLists.txt)
    add_subdirectory(tests/render-harness)
endif()
