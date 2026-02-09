#pragma once

#include <JuceHeader.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OuariconPolystutterAudioProcessor;

//==============================================================================
/**
 * OuariconPolystutter WebView-based UI
 *
 * Dimensions: 1000×750px (fixed)
 * Total Parameters: 70 (56 lane params + 6 tape params + 4 global params + 64 pattern steps)
 * Aesthetic: Ouaricon Audio Naturalist (paper texture, botanical overlay)
 */
class OuariconPolystutterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OuariconPolystutterAudioProcessorEditor(OuariconPolystutterAudioProcessor&);
    ~OuariconPolystutterAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    OuariconPolystutterAudioProcessor& audioProcessor;

    //==============================================================================
    // CRITICAL MEMBER DECLARATION ORDER (see juce8-critical-patterns.md #11)
    // Order: Relays → WebView → Attachments
    // Destroyed in REVERSE order (attachments first, relays last)
    //==============================================================================

    // ========================================================================
    // 1️⃣ RELAYS FIRST (no dependencies)
    // ========================================================================

    // Lane 1 relays (14 parameters)
    std::unique_ptr<juce::WebToggleButtonRelay> lane1EnabledRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lane1SubdivisionRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane1SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1ManualRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane1FreezeRelay;

    // Lane 2 relays (14 parameters)
    std::unique_ptr<juce::WebToggleButtonRelay> lane2EnabledRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lane2SubdivisionRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane2SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2ManualRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane2FreezeRelay;

    // Lane 3 relays (14 parameters)
    std::unique_ptr<juce::WebToggleButtonRelay> lane3EnabledRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lane3SubdivisionRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane3SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3ManualRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane3FreezeRelay;

    // Lane 4 relays (14 parameters)
    std::unique_ptr<juce::WebToggleButtonRelay> lane4EnabledRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lane4SubdivisionRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4RepeatsRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4DecayRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4PitchRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4FilterRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4ProbabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4VolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4PanRelay;
    std::unique_ptr<juce::WebSliderRelay> lane4SwingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4PingpongRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4ReverseRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4ManualRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lane4FreezeRelay;

    // Tape degradation relays (6 parameters)
    std::unique_ptr<juce::WebSliderRelay> tapeSaturationRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeWowRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeFlutterRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeHissRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeRolloffRelay;
    std::unique_ptr<juce::WebSliderRelay> tapeDropoutRelay;

    // Global relays (4 parameters)
    std::unique_ptr<juce::WebToggleButtonRelay> envelopeEnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> sidechainEnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> midiEnabledRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> manualTriggerRelay;

    // Pattern sequencer relays (64 step buttons = 4 lanes × 16 steps)
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

    // ========================================================================
    // 2️⃣ WEBVIEW SECOND (depends on relays)
    // ========================================================================
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ========================================================================
    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    // ========================================================================

    // Lane 1 attachments (14 parameters)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1EnabledAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane1SubdivisionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1ManualAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane1FreezeAttachment;

    // Lane 2 attachments (14 parameters)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2EnabledAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane2SubdivisionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane2SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2ManualAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane2FreezeAttachment;

    // Lane 3 attachments (14 parameters)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3EnabledAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane3SubdivisionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane3SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3ManualAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane3FreezeAttachment;

    // Lane 4 attachments (14 parameters)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4EnabledAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lane4SubdivisionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4RepeatsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4DecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4PitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4FilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4ProbabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4VolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4PanAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lane4SwingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4PingpongAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4ReverseAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4ManualAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lane4FreezeAttachment;

    // Tape degradation attachments (6 parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeSaturationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeWowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeFlutterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeHissAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeRolloffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapeDropoutAttachment;

    // Global attachments (4 parameters)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> envelopeEnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> sidechainEnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> midiEnabledAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> manualTriggerAttachment;

    // Pattern sequencer attachments (64 step buttons)
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

    //==============================================================================
    // Helper methods
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconPolystutterAudioProcessorEditor)
};
