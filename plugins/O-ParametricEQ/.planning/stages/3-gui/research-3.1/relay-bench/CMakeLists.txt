cmake_minimum_required(VERSION 3.22)
project(RelayBench VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

# Same JUCE tree the repo root CMakeLists pulls in (8.0.15).
add_subdirectory(/Users/taylorbrook/JUCE JUCE)

set(REPO /Users/taylorbrook/Dev/VST-development)
set(PEQ  ${REPO}/plugins/O-ParametricEQ/Source)

juce_add_gui_app(RelayBench PRODUCT_NAME "RelayBench" NEEDS_WEB_BROWSER TRUE)
juce_generate_juce_header(RelayBench)

target_sources(RelayBench PRIVATE
    src/Main.cpp
    src/BenchEditor.cpp
    # The real processor TUs, by absolute path (never copied, never edited)
    ${PEQ}/PluginProcessor.cpp
    ${PEQ}/analysis/SpectrumAnalyzer.cpp
    ${PEQ}/analysis/DisplayModel.cpp)

target_include_directories(RelayBench PRIVATE
    src
    ${PEQ}
    ${REPO}/modules/metering/vu-meter/cpp)

juce_add_binary_data(RelayBench_UI
    NAMESPACE UIBinaryData
    HEADER_NAME UIBinaryData.h
    SOURCES
        ui/index.html
        ui/js/i18n.js
        ui/js/juce/index.js
        ui/js/juce/check_native_interop.js
        ${REPO}/modules/persistence/preset-manager/js/preset-manager.js
        ${REPO}/modules/metering/vu-meter/js/vu-meter.js
        ${REPO}/modules/ui/eb-garamond/css/eb-garamond.css
        ${REPO}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${REPO}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${REPO}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2)

# Identity macros as the plugin target derives them (OPEQ 0.6.0, OuPQ).
target_compile_definitions(RelayBench PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="RelayBench"
    JUCE_APPLICATION_VERSION_STRING="0.6.0"
    JucePlugin_Name="O-ParametricEQ-dev"
    JucePlugin_Desc="O-ParametricEQ-dev"
    JucePlugin_Manufacturer="Ouaricon"
    JucePlugin_ManufacturerCode=0x4f756172
    JucePlugin_PluginCode=0x4f755051
    JucePlugin_VersionString="0.6.0"
    JucePlugin_VersionCode=0x600
    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0)

target_link_libraries(RelayBench PRIVATE
    RelayBench_UI
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
