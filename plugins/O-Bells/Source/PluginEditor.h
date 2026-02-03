/*
  ==============================================================================

    O-Bells - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OBellsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit OBellsAudioProcessorEditor(OBellsAudioProcessor&);
    ~OBellsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for meter updates
    void timerCallback() override;


    OBellsAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL ORDER: Relays → WebView → Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ RELAYS FIRST (no dependencies)
    // Float/Int parameter relays (17 sliders in v1.3.0 - material now ComboBox)
    std::unique_ptr<juce::WebSliderRelay> strikePositionRelay;
    std::unique_ptr<juce::WebSliderRelay> malletHardnessRelay;
    std::unique_ptr<juce::WebSliderRelay> dampingRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    std::unique_ptr<juce::WebSliderRelay> inharmonicityRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomRelay;
    std::unique_ptr<juce::WebSliderRelay> shimmerRelay;
    std::unique_ptr<juce::WebSliderRelay> unisonCountRelay;
    std::unique_ptr<juce::WebSliderRelay> unisonDetuneRelay;
    std::unique_ptr<juce::WebSliderRelay> octaveBlendSubRelay;
    std::unique_ptr<juce::WebSliderRelay> octaveBlendOctRelay;
    std::unique_ptr<juce::WebSliderRelay> stereoSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> partialTuningRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchEnvelopeRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchEnvTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> nonlinearEffectsRelay;
    std::unique_ptr<juce::WebSliderRelay> attackLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbMixRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    // Multi-stage envelope relays (4 sliders, active when decayShape == 2)
    std::unique_ptr<juce::WebSliderRelay> strikeTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> brillianceRelay;
    std::unique_ptr<juce::WebSliderRelay> bodyTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> humSustainRelay;

    // Choice parameter relays (3 combo boxes in v1.3.0 - material added)
    std::unique_ptr<juce::WebComboBoxRelay> materialRelay;
    std::unique_ptr<juce::WebComboBoxRelay> strikeNoiseCharRelay;
    std::unique_ptr<juce::WebComboBoxRelay> velocityCurveRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    // Float/Int parameter attachments (17 sliders in v1.3.0 - material now ComboBox)
    std::unique_ptr<juce::WebSliderParameterAttachment> strikePositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> malletHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> inharmonicityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> shimmerAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> unisonCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> unisonDetuneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> octaveBlendSubAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> octaveBlendOctAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stereoSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> partialTuningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchEnvelopeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchEnvTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nonlinearEffectsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    // Multi-stage envelope attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> strikeTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brillianceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humSustainAttachment;

    // Choice parameter attachments (3 combo boxes in v1.3.0 - material added)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> materialAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> strikeNoiseCharAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> velocityCurveAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset save/load dialogs
    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBellsAudioProcessorEditor)
};
