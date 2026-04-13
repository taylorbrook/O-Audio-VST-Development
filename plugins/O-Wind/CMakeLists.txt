cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-Wind - Physical Modeling Flute Synthesizer
juce_add_plugin(O-Wind
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OWnd
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Wind${OUARICON_DEV_SUFFIX}"
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
        Source/DSP/SubHarmonics.h
        Source/DSP/InstrumentPresets.h
        Source/DSP/StereoWidth.h
        Source/DSP/ToneHoleSystem.h
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

# Licensing module (compile-flag gated, OFF for local dev)
if(OUARICON_LICENSING)
    ouaricon_add_module(O-Wind licensing)
    target_compile_definitions(O-Wind PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-Wind PRIVATE juce::juce_cryptography)
endif()

# WebView UI Resources
juce_add_binary_data(O-Wind_UIResources
    SOURCES
        Resources/ui/index.html
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
