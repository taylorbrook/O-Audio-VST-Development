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
    std::unique_ptr<juce::WebSliderRelay> spectralTiltRelay;
    std::unique_ptr<juce::WebSliderRelay> consonantLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> consonantToneRelay;
    std::unique_ptr<juce::WebSliderRelay> sibilanceRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoConsonantRelay;
    std::unique_ptr<juce::WebSliderRelay> attackRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> sustainRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseRelay;
    std::unique_ptr<juce::WebComboBoxRelay> formantTopologyRelay;
    std::unique_ptr<juce::WebSliderRelay> formantShiftRelay;
    std::unique_ptr<juce::WebSliderRelay> formantSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchGlideRelay;
    std::unique_ptr<juce::WebSliderRelay> transitionTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> singersFormantRelay;
    std::unique_ptr<juce::WebSliderRelay> nasalCouplingRelay;
    std::unique_ptr<juce::WebSliderRelay> nasalPlaceRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebSliderRelay> stereoWidthRelay;

    // Effects relays
    std::unique_ptr<juce::WebToggleButtonRelay> chorusBypassRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusRateRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusMixRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> delayBypassRelay;
    std::unique_ptr<juce::WebSliderRelay> delayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> delayFeedbackRelay;
    std::unique_ptr<juce::WebComboBoxRelay> delayModeRelay;
    std::unique_ptr<juce::WebSliderRelay> delayMixRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> reverbBypassRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbDampRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbPredelayRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbMixRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbModRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbShimmerRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqBypassRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLowGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHighGainRelay;

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
    std::unique_ptr<juce::WebSliderParameterAttachment> spectralTiltAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> consonantLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> consonantToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sibilanceAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoConsonantAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> formantTopologyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantShiftAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> formantSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchGlideAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> transitionTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> singersFormantAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nasalCouplingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nasalPlaceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stereoWidthAttachment;

    // Effects attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> chorusBypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusMixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delayBypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> delayModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayMixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverbBypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbDampAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbPredelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbModAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbShimmerAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqBypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLowGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHighGainAttachment;

    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OFormantEditor)
};
