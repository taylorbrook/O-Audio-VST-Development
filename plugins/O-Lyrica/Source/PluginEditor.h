/*
  ==============================================================================

    O-Lyrica - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>

class OLyricaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit OLyricaAudioProcessorEditor(OLyricaAudioProcessor&);
    ~OLyricaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // v1.7.9: Timer callback for MIDI event polling (tuning circle visualization)
    void timerCallback() override;

    OLyricaAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL ORDER: Relays → WebView → Attachments
    // Members destroyed in REVERSE order. Attachments call evaluateJavascript()
    // during destruction, so WebView must still exist when attachments die.
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> masterVolumeRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    std::unique_ptr<juce::WebSliderRelay> timbreRelay;        // v1.1.0: renamed from sustain
    std::unique_ptr<juce::WebSliderRelay> decayTimeRelay;     // v1.1.0: new parameter
    std::unique_ptr<juce::WebSliderRelay> bodySizeRelay;
    std::unique_ptr<juce::WebSliderRelay> bodyResonanceRelay;
    std::unique_ptr<juce::WebSliderRelay> sympatheticAmountRelay;
    std::unique_ptr<juce::WebSliderRelay> pluckPositionRelay;
    std::unique_ptr<juce::WebSliderRelay> fingerHardnessRelay;
    std::unique_ptr<juce::WebSliderRelay> stringTensionRelay;
    std::unique_ptr<juce::WebSliderRelay> stringGaugeRelay;
    std::unique_ptr<juce::WebSliderRelay> stringLengthRelay;
    std::unique_ptr<juce::WebSliderRelay> stringStiffnessRelay;
    std::unique_ptr<juce::WebSliderRelay> masterTuneRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchBendRangeRelay;
    // v1.9.0: Octave stretch
    std::unique_ptr<juce::WebSliderRelay> octaveStretchRelay;
    // v1.4.0: New parameters from v1.3.0
    std::unique_ptr<juce::WebSliderRelay> attackNoiseRelay;
    std::unique_ptr<juce::WebSliderRelay> sympatheticQRelay;
    std::unique_ptr<juce::WebSliderRelay> bodyModeSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> bridgeBrightnessRelay;
    // v1.19.0: Humanize (per-note randomization)
    std::unique_ptr<juce::WebSliderRelay> humanizeRelay;

    std::unique_ptr<juce::WebComboBoxRelay> stringMaterialRelay;
    std::unique_ptr<juce::WebComboBoxRelay> woodTypeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> techniqueRelay;
    std::unique_ptr<juce::WebComboBoxRelay> glissandoModeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> glissandoScaleRelay;
    // v1.6.0: Tuning mode relay
    std::unique_ptr<juce::WebComboBoxRelay> tuningModeRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> masterVolumeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> timbreAttachment;      // v1.1.0: renamed from sustain
    std::unique_ptr<juce::WebSliderParameterAttachment> decayTimeAttachment;   // v1.1.0: new parameter
    std::unique_ptr<juce::WebSliderParameterAttachment> bodySizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyResonanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pluckPositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> fingerHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stringTensionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stringGaugeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stringLengthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stringStiffnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> masterTuneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchBendRangeAttachment;
    // v1.9.0: Octave stretch attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> octaveStretchAttachment;
    // v1.4.0: New parameters from v1.3.0
    std::unique_ptr<juce::WebSliderParameterAttachment> attackNoiseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sympatheticQAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyModeSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bridgeBrightnessAttachment;
    // v1.19.0: Humanize attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizeAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> stringMaterialAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> woodTypeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> techniqueAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoModeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoScaleAttachment;
    // v1.6.0: Tuning mode attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningModeAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // v1.7.10: File chooser for preset save/load dialogs
    // Uses shared_ptr so async callbacks keep FileChooser alive even if user
    // triggers another dialog before the first one completes
    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OLyricaAudioProcessorEditor)
};
