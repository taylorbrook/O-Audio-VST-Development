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
    VERSION "0.1.0"
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
)

target_include_directories(O-simpleWavetable PRIVATE Source)

# Copies webview-drop-streaming.js into Source/ui/public/modules/ (gitignored)
# and puts the header-only C++ on the include path. Consumed in Stage 2.4/3.
ouaricon_add_module(O-simpleWavetable webview-drop-streaming)

target_link_libraries(O-simpleWavetable
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
endif()
