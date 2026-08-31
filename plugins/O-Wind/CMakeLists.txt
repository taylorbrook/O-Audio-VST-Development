cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-Wind - Physical Modeling Flute Synthesizer
juce_add_plugin(O-Wind
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OWnd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Wind${OUARICON_DEV_SUFFIX}"
    VERSION "1.18.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-Wind
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/FluteSynthSound.h
        Source/FluteSynthVoice.h
        Source/FluteSynthVoice.cpp
        Source/DSP/JetExciter.h
        Source/DSP/JetNonlinearity.h
        Source/DSP/DCBlocker.h
        Source/DSP/BoreWaveguide.h
        Source/DSP/InstrumentPresets.h
        Source/DSP/StereoWidth.h
        Source/DSP/DelayProcessor.h
        Source/DSP/DelayProcessor.cpp
        Source/DSP/EQProcessor.h
        Source/DSP/EQProcessor.cpp
        Source/DSP/ReverbProcessor.h
        Source/DSP/ReverbProcessor.cpp
        # Tuning module files (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Wind note-expression)

# Include paths
target_include_directories(O-Wind
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
)

# Required JUCE modules
target_link_libraries(O-Wind
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
juce_generate_juce_header(O-Wind)

# WebView UI Resources
juce_add_binary_data(O-Wind_UIResources
    SOURCES
        Resources/ui/index.html
        # v1.17.0: the label table. It must be in BOTH this SOURCES block and a
        # getResource() branch in PluginEditor.cpp — a file embedded but not
        # served, or served but not embedded, is a 404 that presents as a
        # missing panel and nothing else. check-i18n assertion 8 asserts both.
        Resources/ui/js/i18n.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/fern.png
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Wind
    PRIVATE
        O-Wind_UIResources
)

# Compile definitions
target_compile_definitions(O-Wind
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# ─────────────────────────────────────────────────────────────────────────────
# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Wind test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory. Static parsing of createParameterLayout()
    # provably undercounts in this repo — IDs can be string-concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-Wind ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
