/*
  ==============================================================================

    O-Polystutter - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <optional>
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OPolystutterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                              , private OuariconLicense::Listener
#endif
{
public:
    explicit OPolystutterAudioProcessorEditor(OPolystutterAudioProcessor&);
    ~OPolystutterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // v1.5.0: Timer callback for lane progress updates (~30Hz)
    void timerCallback() override;

private:
    OPolystutterAudioProcessor& processorRef;

    // ⚠️ CRITICAL: Member declaration order prevents 90% of release build crashes
    // Members destroyed in REVERSE order of declaration
    // Order: Relays (no deps) → WebView (depends on relays) → Attachments (depend on both)

    // ========== RELAYS (DECLARE FIRST) ==========

    // Lane 1 Relays (8 sliders + 4 toggles + 1 combo)
    std::unique_ptr<juce::WebSliderRelay> lane1RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1EnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1ManualRelay;

    std::unique_ptr<juce::WebComboBoxRelay> lane1SubdivisionRelay;
    // v1.7.0: Lane 1 Pitch Randomization Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane1PitchRandEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1PitchRandMinRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1PitchRandMaxRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1PitchRandQuantizeRelay;
    // v1.9.0: Lane 1 Euclidean Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane1EuclideanEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1EuclideanPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1EuclideanStepsRelay;

    // Lane 2 Relays (8 sliders + 4 toggles + 1 combo)
    std::unique_ptr<juce::WebSliderRelay> lane2RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2EnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2ManualRelay;

    std::unique_ptr<juce::WebComboBoxRelay> lane2SubdivisionRelay;
    // v1.7.0: Lane 2 Pitch Randomization Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane2PitchRandEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2PitchRandMinRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2PitchRandMaxRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2PitchRandQuantizeRelay;
    // v1.9.0: Lane 2 Euclidean Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane2EuclideanEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2EuclideanPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2EuclideanStepsRelay;

    // Lane 3 Relays (8 sliders + 4 toggles + 1 combo)
    std::unique_ptr<juce::WebSliderRelay> lane3RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3EnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3ManualRelay;

    std::unique_ptr<juce::WebComboBoxRelay> lane3SubdivisionRelay;
    // v1.7.0: Lane 3 Pitch Randomization Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane3PitchRandEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3PitchRandMinRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3PitchRandMaxRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3PitchRandQuantizeRelay;
    // v1.9.0: Lane 3 Euclidean Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane3EuclideanEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3EuclideanPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3EuclideanStepsRelay;

    // Lane 4 Relays (8 sliders + 4 toggles + 1 combo)
    std::unique_ptr<juce::WebSliderRelay> lane4RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4EnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4ManualRelay;

    std::unique_ptr<juce::WebComboBoxRelay> lane4SubdivisionRelay;
    // v1.7.0: Lane 4 Pitch Randomization Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane4PitchRandEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4PitchRandMinRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4PitchRandMaxRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4PitchRandQuantizeRelay;
    // v1.9.0: Lane 4 Euclidean Relays
    std::unique_ptr<juce::WebToggleButtonRelay> lane4EuclideanEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4EuclideanPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4EuclideanStepsRelay;

    // Tape Degradation Relays (6 sliders + 1 toggle)
    std::unique_ptr<juce::WebSliderRelay> tapeSaturationRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeWowRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeFlutterRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeHissRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeRolloffRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeDropoutRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> tapeBypassRelay;  // v1.6.5

    // v1.1.0: Wet/Dry Mix Relays
    std::unique_ptr<juce::WebSliderRelay> mixDryRelay;
    std::unique_ptr<juce::WebSliderRelay> mixWetRelay;

    // Global Controls (2 toggles + 1 sequencer enable) - v1.3.1: Removed ENV and SC
    std::unique_ptr<juce::WebToggleButtonRelay> midiEnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> manualTriggerRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> sequencerEnabledRelay;  // v1.0.2

    // Pattern Sequencer Step Relays (64 toggles: 4 lanes × 16 steps)
    // Lane 1 pattern steps (16 toggles)
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step1Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step2Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step3Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step4Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step5Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step6Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step7Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step8Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step9Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step10Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step11Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step12Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step13Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step14Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step15Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane1Step16Relay;

    // Lane 2 pattern steps (16 toggles)
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step1Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step2Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step3Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step4Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step5Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step6Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step7Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step8Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step9Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step10Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step11Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step12Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step13Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step14Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step15Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane2Step16Relay;

    // Lane 3 pattern steps (16 toggles)
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step1Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step2Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step3Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step4Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step5Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step6Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step7Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step8Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step9Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step10Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step11Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step12Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step13Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step14Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step15Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane3Step16Relay;

    // Lane 4 pattern steps (16 toggles)
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step1Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step2Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step3Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step4Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step5Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step6Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step7Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step8Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step9Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step10Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step11Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step12Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step13Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step14Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step15Relay;
    std::unique_ptr<juce::WebToggleButtonRelay> patternLane4Step16Relay;

    // ========== WEBVIEW (DECLARE SECOND, depends on relays via withOptionsFrom) ==========
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ========== ATTACHMENTS (DECLARE LAST, depend on both relays and WebView) ==========

    // Lane 1 Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1EnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1ManualAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane1SubdivisionAttachment;
    // v1.7.0: Lane 1 Pitch Randomization Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1PitchRandEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1PitchRandMinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1PitchRandMaxAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1PitchRandQuantizeAttachment;
    // v1.9.0: Lane 1 Euclidean Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1EuclideanEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1EuclideanPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1EuclideanStepsAttachment;

    // Lane 2 Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2EnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2ManualAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane2SubdivisionAttachment;
    // v1.7.0: Lane 2 Pitch Randomization Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2PitchRandEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2PitchRandMinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2PitchRandMaxAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2PitchRandQuantizeAttachment;
    // v1.9.0: Lane 2 Euclidean Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2EuclideanEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2EuclideanPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2EuclideanStepsAttachment;

    // Lane 3 Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3EnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3ManualAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane3SubdivisionAttachment;
    // v1.7.0: Lane 3 Pitch Randomization Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3PitchRandEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3PitchRandMinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3PitchRandMaxAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3PitchRandQuantizeAttachment;
    // v1.9.0: Lane 3 Euclidean Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3EuclideanEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3EuclideanPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3EuclideanStepsAttachment;

    // Lane 4 Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4EnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4ManualAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane4SubdivisionAttachment;
    // v1.7.0: Lane 4 Pitch Randomization Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4PitchRandEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4PitchRandMinAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4PitchRandMaxAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4PitchRandQuantizeAttachment;
    // v1.9.0: Lane 4 Euclidean Attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4EuclideanEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4EuclideanPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4EuclideanStepsAttachment;

    // Tape Degradation Attachments (6 sliders + 1 toggle)
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeSaturationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeWowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeFlutterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeHissAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeRolloffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeDropoutAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> tapeBypassAttachment;  // v1.6.5

    // v1.1.0: Wet/Dry Mix Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> mixDryAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixWetAttachment;

    // Global Controls Attachments - v1.3.1: Removed ENV and SC
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> midiEnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> manualTriggerAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> sequencerEnabledAttachment;  // v1.0.2

    // Pattern Sequencer Step Attachments (64 toggles: 4 lanes × 16 steps)
    // Lane 1 pattern steps (16 attachments)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step1Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step2Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step3Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step4Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step5Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step6Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step7Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step8Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step9Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step10Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step11Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step12Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step13Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step14Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step15Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane1Step16Attachment;

    // Lane 2 pattern steps (16 attachments)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step1Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step2Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step3Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step4Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step5Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step6Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step7Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step8Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step9Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step10Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step11Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step12Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step13Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step14Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step15Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane2Step16Attachment;

    // Lane 3 pattern steps (16 attachments)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step1Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step2Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step3Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step4Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step5Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step6Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step7Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step8Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step9Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step10Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step11Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step12Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step13Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step14Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step15Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane3Step16Attachment;

    // Lane 4 pattern steps (16 attachments)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step1Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step2Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step3Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step4Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step5Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step6Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step7Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step8Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step9Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step10Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step11Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step12Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step13Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step14Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step15Attachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> patternLane4Step16Attachment;

    // Resource provider for serving HTML/CSS/JS from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OPolystutterAudioProcessorEditor)
};
