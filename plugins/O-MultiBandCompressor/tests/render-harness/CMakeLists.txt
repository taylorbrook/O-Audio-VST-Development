# O-MultiBandCompressor preset-verification harness (v1.5.0).
#
# Console app. Instantiates the processor directly, loads each factory preset via
# the preset manager, renders pink-ish noise, and reports the gain reduction each
# band actually achieves — the check that the per-band threshold staircase engages
# rather than sitting silently under the band's real level.
# Off by default; enable with -DOUARICON_BUILD_TESTS=ON.

juce_add_console_app(O-MultiBandCompressor-preset-test
    PRODUCT_NAME "O-MultiBandCompressor-preset-test"
)

target_sources(O-MultiBandCompressor-preset-test PRIVATE
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/PluginEditor.cpp
)

target_include_directories(O-MultiBandCompressor-preset-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source
    ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
)

add_dependencies(O-MultiBandCompressor-preset-test O-MultiBandCompressor)
target_include_directories(O-MultiBandCompressor-preset-test PRIVATE
    $<TARGET_PROPERTY:O-MultiBandCompressor,INCLUDE_DIRECTORIES>
)

target_compile_definitions(O-MultiBandCompressor-preset-test PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=1
    JucePlugin_Build_Standalone=1

    JucePlugin_Name="O-MultiBandCompressor"
    JucePlugin_Desc="O-MultiBandCompressor"
    JucePlugin_Manufacturer="Ouaricon"
    JucePlugin_ManufacturerCode=0x4f756172
    JucePlugin_PluginCode=0x4f4d6263
    # Inherited from the parent CMakeLists rather than repeated. The processor ctor
    # runs initializeFactoryPresets(), whose .factory-version sentinel compares
    # against JucePlugin_VersionString — a stale literal here would restamp the
    # user's real factory-preset directory with the wrong version.
    JucePlugin_VersionString="${OMBC_VERSION}"

    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0
)

target_link_libraries(O-MultiBandCompressor-preset-test
    PRIVATE
        O-MultiBandCompressor_UIResources
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
        juce::juce_recommended_warning_flags
)
