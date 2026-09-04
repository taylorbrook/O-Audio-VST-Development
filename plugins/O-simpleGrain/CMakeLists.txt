cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Single source of truth for the plugin version (WR-04): consumed by
# juce_add_plugin below AND by tests/render-harness/CMakeLists.txt (whose
# hand-rolled JucePlugin_VersionString/Code previously drifted).
# VERSION_CODE = 0x00MMmmpp (major/minor/patch bytes) — keep in sync.
set(OSIMPLEGRAIN_VERSION "1.4.3")
set(OSIMPLEGRAIN_VERSION_CODE 0x010403)

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
    VERSION "${OSIMPLEGRAIN_VERSION}"
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
        # Stage 2.3 — viz taps (lock-free SPSC + ring/FFT + cloud frame POD)
        Source/dsp/TripleBuffer.h
        Source/dsp/GrainCloudFrame.h
        Source/VizAnalyzer.h
)

# Include paths
target_include_directories(O-simpleGrain
    PRIVATE
        Source
)

# Stage 3 (GUI): WebView UI resources + shared drag-drop streaming JS.
#
# webview-drop-streaming module: the C++ drop handlers are HAND-ROLLED in
# PluginProcessor.cpp (this plugin does NOT use the module's SessionManager), so
# ouaricon_add_module here just (a) copies webview-drop-streaming.js into
# Source/ui/public/modules/ and (b) adds the header-only module C++ to the include
# path (harmless — never #included). The JS is then embedded below and served at
# /js/modules/webview-drop-streaming.js by PluginEditor::getResource.
ouaricon_add_module(O-simpleGrain webview-drop-streaming)

# WebView UI resources (embedded as BinaryData). CRITICAL: this target MUST use a
# distinct NAMESPACE *and* HEADER_NAME (UIBinaryData) so it does NOT collide with
# the existing O-simpleGrain_Samples target (which uses NAMESPACE BinaryData /
# BinaryData.h for the embedded built-in .wavs). A distinct HEADER_NAME alone is
# insufficient — both targets share the `namedResourceListSize` / `getNamedResource`
# symbols inside the same namespace, which redefines when one TU includes both.
# The editor uses UIBinaryData::*; the processor uses BinaryData::* (the .wavs).
juce_add_binary_data(O-simpleGrain_UIResources
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
        Source/ui/public/img/insects.png
)

# Required JUCE modules
target_link_libraries(O-simpleGrain
    PRIVATE
        O-simpleGrain_UIResources
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

# Stage 2.3 — embed the four built-in sources (fire/voice/water/piano) as binary
# data. MUST come AFTER juce_generate_juce_header. Yields BinaryData::fire_wav /
# BinaryData::fire_wavSize (filename `fire.wav` -> identifier `fire_wav`), etc.
# The processor decodes each via createReaderFor(MemoryInputStream(...)) +
# resamples to the engine rate off the audio thread, then atomic-publishes.
juce_add_binary_data(O-simpleGrain_Samples
    NAMESPACE BinaryData
    HEADER_NAME BinaryData.h
    SOURCES
        Source/samples/fire.wav
        Source/samples/voice.wav
        Source/samples/water.wav
        Source/samples/piano.wav)

target_link_libraries(O-simpleGrain PRIVATE O-simpleGrain_Samples)

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

# Stage 2 DSP correctness gate — offline render harness (console app). Verifies
# the granular engine's audible acceptance criteria without a DAW. Off by
# default; enable with -DOUARICON_BUILD_TESTS=ON.
option(OUARICON_BUILD_TESTS "Build O-simpleGrain render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
