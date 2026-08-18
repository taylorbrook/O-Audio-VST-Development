cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OBitrot
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OBrt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Bitrot${OUARICON_DEV_SUFFIX}"
    VERSION 1.7.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(OBitrot
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include paths
target_include_directories(OBitrot
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OBitrot
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

# Vendored libgsm (GSM 06.10) — own C static lib with relaxed warnings so the
# 1992 C sources never touch the plugin target's flags. License: TU-Berlin
# permissive, recorded in NOTES.md + third_party/libgsm/COPYRIGHT.
# SASR: signed arithmetic shift right (required, clang and MSVC both provide
# it); NDEBUG stubs debug.c; NeedFunctionPrototypes=1 expands the P() macro
# to ANSI prototypes (what makes it compile cleanly as C17, incl. MSVC).
# Never WAV49 (MS-GSM packing) / FAST / USE_FLOAT_MUL / LTP_CUT.
add_library(OBitrot_gsm STATIC
    third_party/libgsm/src/add.c
    third_party/libgsm/src/code.c
    third_party/libgsm/src/debug.c
    third_party/libgsm/src/decode.c
    third_party/libgsm/src/long_term.c
    third_party/libgsm/src/lpc.c
    third_party/libgsm/src/preprocess.c
    third_party/libgsm/src/rpe.c
    third_party/libgsm/src/gsm_create.c
    third_party/libgsm/src/gsm_decode.c
    third_party/libgsm/src/gsm_destroy.c
    third_party/libgsm/src/gsm_encode.c
    third_party/libgsm/src/gsm_explode.c
    third_party/libgsm/src/gsm_implode.c
    third_party/libgsm/src/gsm_option.c
    third_party/libgsm/src/gsm_print.c
    third_party/libgsm/src/short_term.c
    third_party/libgsm/src/table.c
)
target_include_directories(OBitrot_gsm PUBLIC third_party/libgsm/inc)
target_compile_definitions(OBitrot_gsm PRIVATE SASR NDEBUG NeedFunctionPrototypes=1)
if(MSVC)
    target_compile_options(OBitrot_gsm PRIVATE /w)
else()
    target_compile_options(OBitrot_gsm PRIVATE -w)
endif()
set_target_properties(OBitrot_gsm PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(OBitrot PRIVATE OBitrot_gsm)

# Generate JuceHeader.h (JUCE 8 requirement - MUST be after target_link_libraries)
juce_generate_juce_header(OBitrot)

# Shared modules — adds the module cpp/ include dir and configure-copies the
# JS to Source/ui/public/modules/ (that IS this plugin's UI root; never
# relocate the copy — it is re-created on every configure).
ouaricon_add_module(OBitrot preset-manager)

# WebView UI Resources (no hyphenated filenames — BinaryData strips hyphens;
# preset-manager.js becomes BinaryData::presetmanager_js)
juce_add_binary_data(OBitrot_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/specimen.webp
        Source/ui/public/modules/preset-manager.js
)

target_link_libraries(OBitrot PRIVATE OBitrot_UIResources)

# Compile definitions
target_compile_definitions(OBitrot
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets (render harness) — off by default, enable with
# -DOUARICON_BUILD_TESTS=ON. Suite-wide cache option (same name as the other
# plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Bitrot test targets" OFF)

if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
