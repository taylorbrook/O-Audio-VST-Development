cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleSampler - Pedagogical Keyboard Sampler
# Stage 1 (Foundation): silent 16-voice synth shell with the full 20-param APVTS
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
    VERSION "1.4.3"
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

# Stage 3 (GUI): WebView UI resources + shared drag-drop streaming JS.
#
# webview-drop-streaming module: the C++ drop handlers are HAND-ROLLED in
# PluginProcessor.cpp (this plugin does NOT use the module's SessionManager), so
# ouaricon_add_module here just (a) copies webview-drop-streaming.js into
# Source/ui/public/modules/ at configure time and (b) adds the header-only module
# C++ to the include path (harmless — never #included). The JS is then embedded
# below and served at /js/modules/webview-drop-streaming.js by getResource. MUST
# come BEFORE the juce_add_binary_data that lists it as a SOURCE.
ouaricon_add_module(O-simpleSampler webview-drop-streaming)

# WebView UI resources (embedded as BinaryData). CRITICAL: this target MUST use a
# distinct NAMESPACE *and* HEADER_NAME (UIBinaryData) so it does NOT collide with
# the O-simpleSampler_Samples target below (NAMESPACE BinaryData / BinaryData.h for
# the embedded piano.wav). A distinct HEADER_NAME alone is insufficient — both
# targets otherwise redefine namedResourceListSize / getNamedResource inside the
# same namespace (O-simpleGrain Stage-3.1 duplicate-symbol link lesson, MEMORY.md).
# The editor uses UIBinaryData::*; the processor uses BinaryData::* (the .wav).
juce_add_binary_data(O-simpleSampler_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/webview-drop-streaming.js
)

# Required JUCE modules
target_link_libraries(O-simpleSampler
    PRIVATE
        O-simpleSampler_UIResources
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

# Stage 2.1 — embed the built-in source(s) as binary data. MUST come AFTER
# juce_generate_juce_header. Yields BinaryData::piano_wav / BinaryData::piano_wavSize
# (filename `piano.wav` -> identifier `piano_wav`). The processor decodes via
# createReaderFor(MemoryInputStream(...)) + resamples to the engine rate off the
# audio thread, then atomic-publishes. NAMESPACE BinaryData / HEADER_NAME
# BinaryData.h — the Stage-3 UI-resources target MUST use a DISTINCT namespace
# (UIBinaryData) to avoid the duplicate getNamedResource symbol collision.
# v1.1.0 — a single embedded source ships: piano (recorded root C3/48). The other
# v1.0 built-ins were withdrawn; see CHANGELOG.md and Source/samples/LICENSE.md.
# Filenames are kept simple so JUCE mangles them predictably (piano.wav->piano_wav).
# Append SOURCES only; never touch NAMESPACE/HEADER_NAME.
juce_add_binary_data(O-simpleSampler_Samples
    NAMESPACE BinaryData
    HEADER_NAME BinaryData.h
    SOURCES
        Source/samples/piano.wav)

target_link_libraries(O-simpleSampler PRIVATE O-simpleSampler_Samples)

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
# pitch/time independence, loop-seam click absence, region-end declick, Vintage
# clean-at-zero, anti-alias budget) without a DAW. Off by default; enable with
# -DOUARICON_BUILD_TESTS=ON. Added in Stage 2.3, mirroring O-simpleGrain.
option(OUARICON_BUILD_TESTS "Build O-simpleSampler render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
