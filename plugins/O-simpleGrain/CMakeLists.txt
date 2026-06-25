cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleGrain - Pedagogical Granular Synthesizer
# Stage 1 (Foundation): silent 8-voice synth shell with the full 18-param APVTS
# + state persistence (incl. loaded-source identity as custom ValueTree state).
# First audio: Stage 2 (grain engine / voices / ADSR / window LUTs).
# WebView UI: Stage 3. WebView2 flags are set HERE so Stage 3 inherits a correct
# cross-platform config.
juce_add_plugin(O-simpleGrain
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OsGr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleGrain${OUARICON_DEV_SUFFIX}"
    VERSION "0.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-simpleGrain
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        # Stage 2.1 — grain engine (header-only voice + DSP primitives)
        Source/GrainVoice.h
        Source/GrainSound.h
        Source/dsp/Grain.h
        Source/dsp/WindowLuts.h
        Source/dsp/LagrangeInterpolation.h
)

# Include paths
target_include_directories(O-simpleGrain
    PRIVATE
        Source
)

# Stage 3 (GUI): juce_add_binary_data UI resources + ouaricon_add_module here.
# (No ui/ assets exist yet — adding empty refs would break the build. Deferred.)
#
# TODO(Stage 2.3): embed built-in .wav sources (fire/voice/water/piano) via
# juce_add_binary_data. No .wav blobs exist yet — sourceSample stays a plain
# choice param for now; loaded-source identity round-trips as custom state.

# Required JUCE modules
target_link_libraries(O-simpleGrain
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
juce_generate_juce_header(O-simpleGrain)

# Compile definitions
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct
# cross-platform config; the WebView itself (binary data + editor) is added in
# Stage 3. BOTH Windows WebView2 flags are required or WebView silently blanks.
target_compile_definitions(O-simpleGrain
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
