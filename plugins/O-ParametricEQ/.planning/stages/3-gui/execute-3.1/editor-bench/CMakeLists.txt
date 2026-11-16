cmake_minimum_required(VERSION 3.22)
project(EditorBench VERSION 0.7.0)

# PLAN-3.1 T11 / P10: the research relay bench re-pointed at the REAL editor.
# Source/PluginEditor.cpp and the real page are compiled / embedded by absolute
# path (never copied, never edited). OPEQ_EDITOR_BENCH is defined on THIS target
# only; the plugin build never defines it.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

# Same JUCE tree the repo root CMakeLists pulls in (8.0.15).
add_subdirectory(/Users/taylorbrook/JUCE JUCE)

set(REPO /Users/taylorbrook/Dev/VST-development)
set(PEQ  ${REPO}/plugins/O-ParametricEQ/Source)

juce_add_gui_app(EditorBench PRODUCT_NAME "EditorBench" NEEDS_WEB_BROWSER TRUE)
juce_generate_juce_header(EditorBench)

target_sources(EditorBench PRIVATE
    src/Main.cpp
    # The real plugin TUs, by absolute path (same list as the plugin's target_sources)
    ${PEQ}/PluginProcessor.cpp
    ${PEQ}/PluginEditor.cpp
    ${PEQ}/analysis/SpectrumAnalyzer.cpp
    ${PEQ}/analysis/DisplayModel.cpp)

# Source + the two shared-module include paths ouaricon_add_module adds
target_include_directories(EditorBench PRIVATE
    src
    ${PEQ}
    ${REPO}/modules/persistence/preset-manager/cpp
    ${REPO}/modules/metering/vu-meter/cpp)

# The plugin's OuariconParametricEQ_UIResources file list, verbatim
# (plugins/O-ParametricEQ/CMakeLists.txt), same NAMESPACE / HEADER_NAME.
juce_add_binary_data(EditorBench_UIResources
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        ${PEQ}/ui/public/index.html
        ${PEQ}/ui/public/js/i18n.js
        ${PEQ}/ui/public/js/juce/index.js
        ${PEQ}/ui/public/js/juce/check_native_interop.js
        ${REPO}/modules/persistence/preset-manager/js/preset-manager.js
        ${REPO}/modules/metering/vu-meter/js/vu-meter.js
        ${REPO}/modules/ui/eb-garamond/css/eb-garamond.css
        ${REPO}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${REPO}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${REPO}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2)

# Identity macros as the plugin target derives them (OPEQ 0.7.0, OuPQ).
target_compile_definitions(EditorBench PRIVATE
    OPEQ_EDITOR_BENCH=1
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="EditorBench"
    JUCE_APPLICATION_VERSION_STRING="0.7.0"
    JucePlugin_Name="O-ParametricEQ-dev"
    JucePlugin_Desc="O-ParametricEQ-dev"
    JucePlugin_Manufacturer="Ouaricon"
    JucePlugin_ManufacturerCode=0x4f756172
    JucePlugin_PluginCode=0x4f755051
    JucePlugin_VersionString="0.7.0"
    JucePlugin_VersionCode=0x700
    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0)

target_link_libraries(EditorBench PRIVATE
    EditorBench_UIResources
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
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
    juce::juce_recommended_lto_flags)
