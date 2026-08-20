# O-Emulator — retro console audio emulation (stereo effect).
# Target is OEmulator (no hyphen) inside folder O-Emulator/ —
# build-and-install.sh resolves the juce_add_plugin target from this file,
# not the folder name (memory pattern build_script_target_name_vs_folder).
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(OEmulator
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OEmu
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Emulator${OUARICON_DEV_SUFFIX}"
    VERSION 1.0.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(OEmulator
    PRIVATE
        Source/PluginProcessor.cpp
)

# Include paths
target_include_directories(OEmulator
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(OEmulator
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

# Generate JuceHeader.h (JUCE 8 requirement - MUST be after target_link_libraries)
juce_generate_juce_header(OEmulator)

# Stage 3 will add here: ouaricon_add_module(OEmulator preset-manager) and the
# single juce_add_binary_data UI-resources target (exactly one — two binary-data
# targets collide on the BinaryData namespace; no hyphens in resource filenames).

# Compile definitions — WebView flags set now so Stage 3 never touches plugin
# identity/format config.
target_compile_definitions(OEmulator
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets (render harness) — off by default, enable with
# -DOUARICON_BUILD_TESTS=ON. Suite-wide cache option (same name as the other
# plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Emulator test targets" OFF)

if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
