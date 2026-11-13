cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Single source of truth for the version. The test consoles derive
# JucePlugin_VersionString from the plugin target, so nothing mirrors it.
set(OPEQ_VERSION 0.1.0)

# Plugin formats: VST3, AU, Standalone. Effect, no MIDI.
# NOTE: no comment in this file may spell the plugin-declaration command name
# followed by an opening parenthesis: scripts/resolve-target.sh greps for that
# text, comments included, and takes the first hit as the target.
juce_add_plugin(OuariconParametricEQ
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuPQ
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-ParametricEQ${OUARICON_DEV_SUFFIX}"
    VERSION ${OPEQ_VERSION}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files. Stage 1 has no editor translation unit: createEditor() returns
# the generic editor until Phase 3.1.
target_sources(OuariconParametricEQ
    PRIVATE
        Source/PluginProcessor.cpp
)

# Include paths
target_include_directories(OuariconParametricEQ
    PRIVATE
        Source
)

# Required JUCE modules (juce_dsp: Stage 2 analyzer FFT; juce_gui_extra: Stage 3 WebView)
target_link_libraries(OuariconParametricEQ
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

# Generate JuceHeader.h (the processor and the test consoles include it)
juce_generate_juce_header(OuariconParametricEQ)

# Shared modules.
#   preset-manager 1.1.1: wired now, first used in Stage 4. The processor does
#                         not own a manager yet.
#   vu-meter 1.0.0:       header compiled from PluginProcessor.h as a proof;
#                         the meter is mounted in Stage 3.
#   eb-garamond 1.0.0:    has no helper. Its stylesheet and fonts are a direct
#                         embed into the UI binary-data target in Phase 3.1.
ouaricon_add_module(OuariconParametricEQ preset-manager)
ouaricon_add_module(OuariconParametricEQ vu-meter)

# The WebView UI binary-data target lands in Phase 3.1 (its sources do not
# exist yet). The snippet is .planning/mockups/v1-CMakeLists-SNIPPET.txt.

# Compile definitions (WebView plumbing wired now, so Stage 3 is an editor swap)
target_compile_definitions(OuariconParametricEQ
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets: off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-ParametricEQ test targets" OFF)

if(OUARICON_BUILD_TESTS)
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)

    # Suite runtime parameter inventory: O-ParametricEQ-param-dump (later UI gates).
    ouaricon_add_param_dump(OuariconParametricEQ ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    # Gate G11: O-ParametricEQ-render-harness. Layout golden (--dump-params),
    # state codec, bus layouts and pass-through rows. The console is named
    # after this folder, not after the plugin target.
    ouaricon_add_processor_console(OuariconParametricEQ ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-harness)

    target_compile_definitions(O-ParametricEQ-render-harness PRIVATE
        GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden")

    # Gate G1 (offline): O-ParametricEQ-designer-harness. A plain C++17 console
    # over the std-only headers in Source/dsp: no plugin target, no framework
    # modules, and no floating-point options of its own, so the designer is
    # compiled with the same arithmetic as the plugin. Threads is the standard
    # library's std::thread dependency (row B1), nothing else.
    find_package(Threads REQUIRED)

    add_executable(O-ParametricEQ-designer-harness EXCLUDE_FROM_ALL
        tests/designer-harness/main.cpp)

    set_target_properties(O-ParametricEQ-designer-harness PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    target_include_directories(O-ParametricEQ-designer-harness PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    target_compile_definitions(O-ParametricEQ-designer-harness PRIVATE
        GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden")

    target_link_libraries(O-ParametricEQ-designer-harness PRIVATE Threads::Threads)

    # Release optimisation whatever the build type, and the warning set the
    # build log is checked against. (MSVC keeps the build type's own flags.)
    if(NOT MSVC)
        target_compile_options(O-ParametricEQ-designer-harness PRIVATE
            -O3 -Wall -Wextra -Wconversion -Wshadow -Wsign-conversion -Wfloat-equal)
    endif()
endif()
