cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-MicrotonalSampler - Microtonal Sample Engine (varispeed retune via VST3 NE)
# Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain
# + sample-map shared_ptr surface + SampleLoader skeleton. First audio: Phase 2.1.
juce_add_plugin(O-MicrotonalSampler
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OMtS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-MicrotonalSampler${OUARICON_DEV_SUFFIX}"
    # NOTE: must be `VERSION` (not `PLUGIN_VERSION`). The latter is silently
    # ignored by juce_add_plugin and the build falls back to PROJECT_VERSION
    # (=1.0.0 from the root CMakeLists), so the About tab and bundle plist
    # would always show v1.0.0. Keep this token as `VERSION` to wire bumps
    # through to JucePlugin_VersionString → getPluginVersion → About tab.
    VERSION "1.16.9"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(O-MicrotonalSampler
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MicrotonalSamplerSound.h
        Source/MicrotonalSamplerVoice.h
        Source/MicrotonalSamplerVoice.cpp
        Source/SampleMap.h
        Source/SampleLoader.h
        Source/SampleLoader.cpp
        Source/FilenameParser.h
        Source/FilenameParser.cpp
        # v1.0.3: drag-drop is now handled at the JS layer (sampler-app.js
        # bindWebViewFileDrop) and forwarded to the C++ filesDropped router
        # via the handleWebViewFileDrop native function. The v1.0.1 helper
        # (-unregisterDraggedTypes) and v1.0.2 NSView overlay both failed —
        # see CHANGELOG.md and Source/PluginEditor.cpp top-of-file note.
        # v1.13.0 (ARCH-02): the macOS WKWebView content-streaming pattern
        # (4 dropSession* native fns + DropSessionGuard validators + reaper)
        # has been promoted to the shared modules/core/webview-drop-streaming
        # module. Wired below via ouaricon_add_module().
        # Tuning module files (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

# VST3 Note Expression microtonal support (Dorico) — D-5: ouaricon_add_module ONLY
ouaricon_add_module(O-MicrotonalSampler note-expression)

# v1.13.0 (ARCH-02): WKWebView drag-drop content-streaming pattern.
# Provides Ouaricon::WebViewDropStreaming::SessionManager (header-only) +
# the 4 dropSession* native function handlers + DropSessionGuard
# validators (path-traversal / parent-chain symlink / size caps).
# JS half (webview-drop-streaming.js) is added to the binary data block
# below — OuariconModules.cmake's JS-copy step targets Source/ui/public/
# which this plugin doesn't use.
ouaricon_add_module(O-MicrotonalSampler webview-drop-streaming)

# Include paths
target_include_directories(O-MicrotonalSampler
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)

# Phase 3.1: WebView UI binary resources (carried verbatim from O-Bells +
# new sampler-shell shell). Served by the WebBrowserComponent's resource
# provider via explicit URL→BinaryData equality mapping (memory pattern).
juce_add_binary_data(O-MicrotonalSampler_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/css/sampler-shell.css
        Resources/ui/css/tuning-panel.css
        Resources/ui/js/sampler-app.js
        Resources/ui/js/tuning-panel.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        # v1.13.0: shared webview-drop-streaming module JS, served by
        # PluginEditor::getResource at /js/modules/webview-drop-streaming.js
        # so the import in sampler-app.js (./modules/webview-drop-streaming.js)
        # resolves through the WebView resource provider. Symbol generated
        # by juce_add_binary_data: BinaryData::webviewdropstreaming_js.
        ${CMAKE_SOURCE_DIR}/modules/core/webview-drop-streaming/js/webview-drop-streaming.js
        # v1.10.0: Naturalist aesthetic — antique anatomical engraving overlay
        Resources/ui/images/brains.png
        # v1.11.0: paper textures — page (paper1) and card (paper2) backgrounds
        Resources/ui/images/paper1.jpg
        Resources/ui/images/paper2.jpg
)

# Required JUCE modules
target_link_libraries(O-MicrotonalSampler
    PRIVATE
        O-MicrotonalSampler_UIResources
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
juce_generate_juce_header(O-MicrotonalSampler)

# Licensing module (compile-flag gated, OFF for local dev)
if(OUARICON_LICENSING)
    ouaricon_add_module(O-MicrotonalSampler licensing)
    target_compile_definitions(O-MicrotonalSampler PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-MicrotonalSampler PRIVATE juce::juce_cryptography)
endif()

# Compile definitions
target_compile_definitions(O-MicrotonalSampler
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Phase 2.1 in-memory test fixture toggle.
# ON: PluginProcessor::prepareToPlay builds an 88-note sine-burst SampleMap so
# the voice DSP can be exercised before Phase 2.2 wires the real loader.
# Phase 2.2 flipped this OFF (real folder load via SampleLoader supersedes the
# fixture); the macro-gated code remains in source as a regression safety net.
option(OMTS_PHASE_2_1_TEST_FIXTURE
    "Enable Phase 2.1 in-memory sine-burst SampleMap for early audio testing" OFF)
if(OMTS_PHASE_2_1_TEST_FIXTURE)
    target_compile_definitions(O-MicrotonalSampler PRIVATE
        O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE=1)
endif()

# Phase 2.1 aliasing measurement driver (RQ-1).
# Standalone executable, EXCLUDE_FROM_ALL — manual run by developer to verify
# +50c retune null-test residual is below -60 dBFS above 8 kHz. Threshold
# breach → conditionally insert juce::dsp::IIR::Filter pre-filter.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/aliasing_check.cpp")
    add_executable(O-MicrotonalSampler_AliasingCheck EXCLUDE_FROM_ALL
        Source/tests/aliasing_check.cpp)
    target_link_libraries(O-MicrotonalSampler_AliasingCheck PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(O-MicrotonalSampler_AliasingCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

# v1.9.0 — round-robin merge unit tests (covers applyMergeRrCell helper used
# by LoadMode::MergeRR and the per-cell single-file merge path). Standalone
# executable, EXCLUDE_FROM_ALL. Build with:
#     ninja O-MicrotonalSampler_MergeRrCheck
# Returns exit code = number of failed assertions.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/merge_rr_check.cpp")
    add_executable(O-MicrotonalSampler_MergeRrCheck EXCLUDE_FROM_ALL
        Source/tests/merge_rr_check.cpp)
    target_link_libraries(O-MicrotonalSampler_MergeRrCheck PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(O-MicrotonalSampler_MergeRrCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

# v1.11.2 — drag-drop streaming security regression tests. Covers the
# dropSessionAddFile path-traversal guard (REVIEW CR-02) and the per-file
# (256 MB) / per-session (4 GB) size caps (REVIEW CR-03). Standalone
# executable, EXCLUDE_FROM_ALL. Build with:
#     ninja O-MicrotonalSampler_DropSessionGuardCheck
# Returns exit code = number of failed assertions.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/drop_session_guard_check.cpp")
    add_executable(O-MicrotonalSampler_DropSessionGuardCheck EXCLUDE_FROM_ALL
        Source/tests/drop_session_guard_check.cpp)
    target_link_libraries(O-MicrotonalSampler_DropSessionGuardCheck PRIVATE
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_include_directories(O-MicrotonalSampler_DropSessionGuardCheck PRIVATE
        Source
        # v1.13.0: DropSessionGuard.h moved to the shared module.
        ${CMAKE_SOURCE_DIR}/modules/core/webview-drop-streaming/cpp)
    target_compile_definitions(O-MicrotonalSampler_DropSessionGuardCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

# v1.14.0 — Playing techniques regression tests.
# Three EXCLUDE_FROM_ALL standalone executables that exercise:
#   * FilenameParser technique-token recognition (substring rejection,
#     case-insensitivity, coexistence with note/velocity/RR).
#   * SampleMap::findCell triplet keying + technique=0 fallback.
#   * State-migration contract — v1.13.0 sessions decode cleanly,
#     curated default vocab seeded by the processor ctor survives.
#
# Build all three:
#     ninja O-MicrotonalSampler_TechniqueParseCheck \
#           O-MicrotonalSampler_FindCellTripletCheck \
#           O-MicrotonalSampler_StateMigrationCheck
# Each returns exit code = number of failed assertions.

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/technique_parse_check.cpp")
    add_executable(O-MicrotonalSampler_TechniqueParseCheck EXCLUDE_FROM_ALL
        Source/tests/technique_parse_check.cpp
        Source/FilenameParser.cpp)
    target_link_libraries(O-MicrotonalSampler_TechniqueParseCheck PRIVATE
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_include_directories(O-MicrotonalSampler_TechniqueParseCheck PRIVATE
        Source)
    target_compile_definitions(O-MicrotonalSampler_TechniqueParseCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/find_cell_triplet_check.cpp")
    add_executable(O-MicrotonalSampler_FindCellTripletCheck EXCLUDE_FROM_ALL
        Source/tests/find_cell_triplet_check.cpp)
    target_link_libraries(O-MicrotonalSampler_FindCellTripletCheck PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(O-MicrotonalSampler_FindCellTripletCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/state_migration_check.cpp")
    add_executable(O-MicrotonalSampler_StateMigrationCheck EXCLUDE_FROM_ALL
        Source/tests/state_migration_check.cpp)
    target_link_libraries(O-MicrotonalSampler_StateMigrationCheck PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(O-MicrotonalSampler_StateMigrationCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

# v1.15.0 — CC + PC trigger routing + dynamics-mapping sanity tests.
# Two EXCLUDE_FROM_ALL standalone executables that exercise:
#   * resolveCcTechnique / resolvePcTechnique / resolveTriggerPrecedence
#     (the pure-data layer behind PluginProcessor::processBlock's
#     KS > CC > PC > history routing).
#   * Velocity-to-layer bucketing — pins the formula documented in
#     docs/dynamics-mapping.md against future refactors.
#
# Build all four (v1.14.0 + v1.15.0):
#     ninja O-MicrotonalSampler_TechniqueParseCheck \
#           O-MicrotonalSampler_FindCellTripletCheck \
#           O-MicrotonalSampler_StateMigrationCheck \
#           O-MicrotonalSampler_CcPcTriggerCheck \
#           O-MicrotonalSampler_DynamicsLayerCheck
# Each returns exit code = number of failed assertions.

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/cc_pc_trigger_check.cpp")
    add_executable(O-MicrotonalSampler_CcPcTriggerCheck EXCLUDE_FROM_ALL
        Source/tests/cc_pc_trigger_check.cpp)
    target_link_libraries(O-MicrotonalSampler_CcPcTriggerCheck PRIVATE
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_include_directories(O-MicrotonalSampler_CcPcTriggerCheck PRIVATE
        Source)
    target_compile_definitions(O-MicrotonalSampler_CcPcTriggerCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/tests/dynamics_layer_check.cpp")
    add_executable(O-MicrotonalSampler_DynamicsLayerCheck EXCLUDE_FROM_ALL
        Source/tests/dynamics_layer_check.cpp)
    target_link_libraries(O-MicrotonalSampler_DynamicsLayerCheck PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(O-MicrotonalSampler_DynamicsLayerCheck PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0)
endif()
