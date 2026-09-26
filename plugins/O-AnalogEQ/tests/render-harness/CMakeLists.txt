# O-AnalogEQ DSP render harness (v1.5.1).
#
# Console app. Instantiates the processor directly and gates the three DSP fixes
# from the v1.5.0 code review on RENDERED AUDIO rather than on flags — see
# pattern_pointer_moved_is_not_audio_changed:
#
#   G1 (CR-02) isBusesLayoutSupported refuses asymmetric negotiation. Driving
#              only (2,2) would be vacuous, so (2,1) and (1,2) are driven too and
#              required to be REFUSED, and the symmetric pair required accepted.
#   G2 (WR-05) output_gain ramps instead of stepping. Measured on a DC bed with
#              every band off, so the gain stage is the ONLY thing that can
#              produce a first difference and the threshold is derived from the
#              ramp rather than copied.
#   G3 (WR-06) band on/off crossfades, and a bypassed band is still fed — no
#              click on the edge, no stale-state burst on re-enable.
#
# Off by default; enable with -DOUARICON_BUILD_TESTS=ON.

juce_add_console_app(O-AnalogEQ-dsp-test
    PRODUCT_NAME "O-AnalogEQ-dsp-test"
)

target_sources(O-AnalogEQ-dsp-test PRIVATE
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/PluginProcessor.cpp
)

target_include_directories(O-AnalogEQ-dsp-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source
    ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
)

add_dependencies(O-AnalogEQ-dsp-test OuariconAnalogEQ)
target_include_directories(O-AnalogEQ-dsp-test PRIVATE
    $<TARGET_PROPERTY:OuariconAnalogEQ,INCLUDE_DIRECTORIES>
)

# JUCE_WEB_BROWSER=0: the editor TU is deliberately NOT compiled in. The
# processor's createEditor() falls back to GenericAudioProcessorEditor under
# that define (see the #if in PluginProcessor.cpp), which keeps this target
# linkable without dragging the WebView surface into a console app —
# pattern_render_harness_breaks_on_webview_editor.
target_compile_definitions(O-AnalogEQ-dsp-test PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JucePlugin_Build_Standalone=1

    JucePlugin_Name="O-AnalogEQ"
    JucePlugin_Desc="O-AnalogEQ"
    JucePlugin_Manufacturer="Ouaricon"
    JucePlugin_ManufacturerCode=0x4f756172
    JucePlugin_PluginCode=0x4f754145
    # Inherited from the parent CMakeLists, never a second literal: the processor
    # ctor runs initializeFactoryPresets(), whose .factory-version sentinel
    # compares against JucePlugin_VersionString, and a stale value here would
    # restamp the user's real factory-preset directory with the wrong version.
    JucePlugin_VersionString="${OAEQ_VERSION}"

    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0
)

target_link_libraries(O-AnalogEQ-dsp-test
    PRIVATE
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
