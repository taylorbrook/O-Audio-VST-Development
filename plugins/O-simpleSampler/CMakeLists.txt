cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleSampler - Pedagogical Keyboard Sampler
# Stage 1 (Foundation): silent 16-voice synth shell with the full 21-param APVTS
# + state persistence (incl. loaded-source identity as custom ValueTree state).
# Signal path (built Stage 2+): source -> start/end region + loop/reverse -> pitch
# engine (Repitch varispeed / Stretch synchronous-granular) -> Vintage -> resonant
# LP -> amp ADSR/VCA.
# First audio: Stage 2 (sampler voices / Repitch read head / amp ADSR / built-in
# decode). WebView UI: Stage 3. WebView2 flags are set HERE so Stage 3 inherits a
# correct cross-platform config.
juce_add_plugin(O-simpleSampler
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OsSm
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleSampler${OUARICON_DEV_SUFFIX}"
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
target_sources(O-simpleSampler
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

# Include paths
target_include_directories(O-simpleSampler
    PRIVATE
        Source
)

# Stage 3 (GUI): juce_add_binary_data UI resources + ouaricon_add_module here.
# (No ui/ assets exist yet — adding empty refs would break the build. Deferred.)
# CRITICAL when added: the UI-resources target MUST use a distinct NAMESPACE *and*
# HEADER_NAME (UIBinaryData / UIBinaryData.h) so it does NOT collide with the
# embedded-samples target below (NAMESPACE BinaryData / BinaryData.h). A distinct
# HEADER_NAME alone is insufficient — both share the namedResourceListSize /
# getNamedResource symbols inside the same namespace (O-simpleGrain Stage-3.1
# duplicate-symbol link lesson, in MEMORY.md).
#
# TODO(Stage 2.3): embed built-in .wav sources (piano/vocal/flute/vinyl) via a
# SECOND juce_add_binary_data target (NAMESPACE BinaryData / HEADER_NAME
# BinaryData.h), AFTER juce_generate_juce_header. No .wav blobs exist yet —
# sourceSample stays a plain choice param for now; loaded-source identity
# round-trips as custom ValueTree state.

# Required JUCE modules
target_link_libraries(O-simpleSampler
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
juce_generate_juce_header(O-simpleSampler)

# Compile definitions
# WebView2 flags are set at Foundation so Stage 3 (GUI) inherits a correct
# cross-platform config; the WebView itself (binary data + editor) is added in
# Stage 3. BOTH Windows WebView2 flags are required or WebView silently blanks.
target_compile_definitions(O-simpleSampler
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Stage 2 DSP correctness gate — offline render harness (console app). Verifies
# the sampler engine's audible acceptance criteria (Repitch tuning, Stretch
# pitch/time independence, loop-seam click absence, Vintage clean-at-zero,
# anti-alias budget) without a DAW. Off by default; enable with
# -DOUARICON_BUILD_TESTS=ON. Added in Stage 2.3.
# TODO(Stage 2.3): add_subdirectory(tests/render-harness) under an
# OUARICON_BUILD_TESTS option, mirroring O-simpleGrain.
