cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleWavetable - Pedagogical 16-voice Wavetable Synthesizer
# Stage 1 (Foundation): silent synth shell, full 21-param APVTS, state + IMPORTED_BANK stub.
# WebView2 flags are set here so Stage 3 (GUI) needs no CMake rework.
juce_add_plugin(O-simpleWavetable
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSiW
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleWavetable${OUARICON_DEV_SUFFIX}"
    VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(O-simpleWavetable
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PresetRecipes.h
        Source/TestHooks.h
        Source/WtSynthesiser.h
        Source/WtVoice.h
        Source/WavetableBank.h
        Source/MipmapBuilder.cpp
        Source/MipmapBuilder.h
        Source/BankFactory.cpp
        Source/BankFactory.h
        Source/BuiltInBanks.cpp
        Source/BuiltInBanks.h
        Source/WtRead.h
        Source/BitQuantizer.h
        Source/MonoStack.h
        Source/PositionLfo.h
        Source/PositionSmoother.h
        Source/WavetableImporter.cpp
        Source/WavetableImporter.h
        Source/CycleView.cpp
        Source/CycleView.h
        Source/VizPayload.h
)

target_include_directories(O-simpleWavetable PRIVATE Source)

# Copies webview-drop-streaming.js into Source/ui/public/modules/ (gitignored)
# and puts the header-only C++ on the include path. Consumed in Stage 2.4/3.
ouaricon_add_module(O-simpleWavetable webview-drop-streaming)

# Stage 4 (FUNC-08): preset-manager. Copies preset-manager.js into
# Source/ui/public/modules/ (gitignored) BEFORE juce_add_binary_data below
# lists it, and puts OuariconPresetManager.h on the include path. Only its
# folders, user save / delete and the factory sentinel are used (D-AA).
ouaricon_add_module(O-simpleWavetable preset-manager)

# WebView UI resources (embedded as BinaryData). Every file listed here is also
# served by a getResource() branch in PluginEditor.cpp. Default BinaryData
# namespace: there is no second binary-data target. BinaryData.h is included
# ONLY by PluginEditor.cpp (console targets compile the other .cpp files
# without this library).
juce_add_binary_data(O-simpleWavetable_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/webview-drop-streaming.js
        Source/ui/public/modules/preset-manager.js
        Source/ui/public/img/insects.png
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
)

target_link_libraries(O-simpleWavetable
    PRIVATE
        O-simpleWavetable_UIResources
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

# JUCE 8: MUST come after target_link_libraries
juce_generate_juce_header(O-simpleWavetable)

# JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 is mandatory with NEEDS_WEBVIEW2 TRUE
# (else Windows shows a blank UI).
target_compile_definitions(O-simpleWavetable
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets: off by default. Suite-wide option name, deliberately.
option(OUARICON_BUILD_TESTS "Build O-simpleWavetable test targets" OFF)
if(OUARICON_BUILD_TESTS AND APPLE)
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/state-check/main.cpp state-check)

    # Stage 2 Part 1 gates. OSIW_TEST_HOOKS compiles the test-only members;
    # it is defined ONLY on these console targets, never on the plugin.
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/bank-check/main.cpp bank-check)
    target_compile_definitions(O-simpleWavetable-bank-check PRIVATE OSIW_TEST_HOOKS=1)

    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/dsp-check/main.cpp dsp-check)
    target_compile_definitions(O-simpleWavetable-dsp-check PRIVATE OSIW_TEST_HOOKS=1)

    # Stage 2.3 gates (DSP-05, FUNC-05, FUNC-06, S&H determinism, clicks).
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mod-check/main.cpp mod-check)
    target_compile_definitions(O-simpleWavetable-mod-check PRIVATE OSIW_TEST_HOOKS=1)

    # Stage 2.4 gates (QUAL-03, DSP-06, reaper REG-01 + D-C, FUNC-03,
    # COMPAT-03, FUNC-04).
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/import-check/main.cpp import-check)
    target_compile_definitions(O-simpleWavetable-import-check PRIVATE OSIW_TEST_HOOKS=1)

    # Stage 3 gates (G-LESSON, G-VIZ-*; later G-DROP / G-IMPORT-ERR).
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/viz-check/main.cpp viz-check)
    target_compile_definitions(O-simpleWavetable-viz-check PRIVATE OSIW_TEST_HOOKS=1)

    # Stage 4 PERF-02 harness. Deliberately WITHOUT OSIW_TEST_HOOKS: the hooks
    # add branches in the voice loop, so the timed code must equal the plugin's.
    # Build it in a Release tree.
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/perf-check/main.cpp perf-check)
endif()
