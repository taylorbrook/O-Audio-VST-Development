#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OFormantEditor : public juce::AudioProcessorEditor
{
public:
    explicit OFormantEditor (OFormantAudioProcessor&);
    ~OFormantEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OFormantAudioProcessor& processorRef;

    // 1. RELAYS (destroyed LAST)
    std::unique_ptr<juce::WebSliderRelay> vowelXRelay;
    std::unique_ptr<juce::WebSliderRelay> vowelYRelay;
    std::unique_ptr<juce::WebSliderRelay> vowelFocusRelay;
    std::unique_ptr<juce::WebSliderRelay> glottalRdRelay;
    std::unique_ptr<juce::WebSliderRelay> breathinessRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> vibratoDelayRelay;
    std::unique_ptr<juce::WebSliderRelay> jitterRelay;
    std::unique_ptr<juce::WebSliderRelay> shimmerRelay;
    std::unique_ptr<juce::WebSliderRelay> rdModDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> consonantLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> consonantToneRelay;
    std::unique_ptr<juce::WebSliderRelay> sibilanceRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoConsonantRelay;
    std::unique_ptr<juce::WebSliderRelay> attackRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> sustainRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseRelay;
    std::unique_ptr<juce::WebSliderRelay> formantShiftRelay;
    std::unique_ptr<juce::WebSliderRelay> formantSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchGlideRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebSliderRelay> stereoWidthRelay;

    // 2. WEBVIEW (destroyed SECOND)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed FIRST)
    std::unique_ptr<juce::WebSliderParameterAttachment> vowelXAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vowelYAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vowelFocusAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> glottalRdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> breathinessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vibratoDelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> jitterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> shimmerAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rdModDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> consonantLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> consonantToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sibilanceAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoConsonantAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantShiftAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchGlideAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stereoWidthAttachment;

    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OFormantEditor)
};
