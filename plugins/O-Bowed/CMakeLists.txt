cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-Bowed - Physical Modeling Bowed String Synthesizer
juce_add_plugin(O-Bowed
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OBwd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bowed${OUARICON_DEV_SUFFIX}"
    VERSION "1.5.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-Bowed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BowedMPESynthesiser.h
        Source/BowedStringVoice.h
        Source/BowedStringVoice.cpp
        Source/DSP/WaveguideString.h
        Source/DSP/WaveguideString.cpp
        Source/DSP/BodyResonator.h
        Source/DSP/BodyResonator.cpp
        Source/DSP/SympatheticStringEngine.h
        Source/DSP/SympatheticStringEngine.cpp
        Source/DSP/StereoWidthProcessor.h
        Source/DSP/ElastoPlasticFriction.h
        Source/DSP/ThermalFriction.h
        Source/DSP/SubHarmonicsGenerator.h
        Source/DSP/BowNoiseGenerator.h
        Source/DSP/HumanizeEngine.h
        # Tuning module files (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Bowed note-expression)

# Phase 2.1b: shared bow-friction module (HyperbolicFriction + BowModel).
# Treble defaults preserved verbatim; no setter calls required in O-Bowed.
ouaricon_add_module(O-Bowed bow-friction)

# Include paths
target_include_directories(O-Bowed
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
)

# Required JUCE modules
target_link_libraries(O-Bowed
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
juce_generate_juce_header(O-Bowed)

# WebView UI Resources
juce_add_binary_data(O-Bowed_UIResources
    SOURCES
        Resources/ui/index.html
        # v1.5.0: THE FOUR-PLACE RULE. js/i18n.js must appear HERE **and** get a
        # getResource() branch in PluginEditor.cpp, in the same commit. Embedded
        # but not served, or served but not embedded, is a 404 that presents as
        # a missing language selector and nothing else — check-i18n assertion 8
        # exists for exactly this pair.
        Resources/ui/js/i18n.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/botanical.png
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Bowed
    PRIVATE
        O-Bowed_UIResources
)

# Compile definitions
target_compile_definitions(O-Bowed
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# ----------------------------------------------------------------------------
# Phase 2.1b render-harness binary (module-extraction regression baseline).
# Off by default; enable with -DOUARICON_BUILD_TESTS=ON.
# ----------------------------------------------------------------------------
option(OUARICON_BUILD_TESTS "Build test harness binaries (e.g. O-Bowed-render-test)" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
