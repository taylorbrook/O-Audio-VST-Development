/*
  ==============================================================================

    O-IntonationPad - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OIntonationPadAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                         , private OuariconLicense::Listener
#endif
{
public:
    explicit OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor&);
    ~OIntonationPadAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;
    void createRelays();
    juce::WebBrowserComponent::Options buildWebViewOptions();
    void createAttachments();

    OIntonationPadAudioProcessor& processorRef;

    // Member declaration order matters: relays → webView → attachments
    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> voiceCountRelay;
    std::unique_ptr<juce::WebSliderRelay> complexityRelay;
    std::unique_ptr<juce::WebSliderRelay> keyRootRelay;
    std::unique_ptr<juce::WebSliderRelay> stereoSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> spacingRelay;
    std::unique_ptr<juce::WebSliderRelay> inversionRelay;
    std::unique_ptr<juce::WebSliderRelay> wavetablePosRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> timingRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> detuneRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> decayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> sustainLevelRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> filterCutoffRelay;
    std::unique_ptr<juce::WebSliderRelay> filterLfoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> velocityToFilterRelay;
    std::unique_ptr<juce::WebSliderRelay> masterVolumeRelay;

    // v1.6.0: Wavetable bank relay
    std::unique_ptr<juce::WebComboBoxRelay> wavetableBankRelay;

    // v1.10.0: Dual oscillator relays (independent gains + rates)
    std::unique_ptr<juce::WebSliderRelay> wavetablePos2Relay;
    std::unique_ptr<juce::WebSliderRelay> gainARelay;
    std::unique_ptr<juce::WebSliderRelay> gainBRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoRate2Relay;
    std::unique_ptr<juce::WebSliderRelay> lfoDepth2Relay;
    std::unique_ptr<juce::WebComboBoxRelay> wavetableBank2Relay;

    // v1.3.0: Tuning module relays
    std::unique_ptr<juce::WebSliderRelay> tuningMasterTuneRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningOctaveStretchRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningPitchBendRangeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> tuningTemperamentPresetRelay;

    // v1.11.0: Effects relays (Chorus, Delay, EQ, Reverb)
    std::unique_ptr<juce::WebSliderRelay> chorusRateRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusMixRelay;
    std::unique_ptr<juce::WebSliderRelay> delayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> delayFeedbackRelay;
    std::unique_ptr<juce::WebComboBoxRelay> delayModeRelay;
    std::unique_ptr<juce::WebSliderRelay> delayMixRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLowGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHighGainRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbDampRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbPredelayRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbMixRelay;
    // Bypass toggles
    std::unique_ptr<juce::WebToggleButtonRelay> chorusBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> delayBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> reverbBypassRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state
    bool hasNavigated = false;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> voiceCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> complexityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> keyRootAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> stereoSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spacingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> inversionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wavetablePosAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> timingRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainLevelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterLfoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> velocityToFilterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> masterVolumeAttachment;

    // v1.6.0: Wavetable bank attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> wavetableBankAttachment;

    // v1.10.0: Dual oscillator attachments (independent gains + rates)
    std::unique_ptr<juce::WebSliderParameterAttachment> wavetablePos2Attachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> gainAAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> gainBAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoRate2Attachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoDepth2Attachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> wavetableBank2Attachment;

    // v1.3.0: Tuning module attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningMasterTuneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningOctaveStretchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningPitchBendRangeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningTemperamentPresetAttachment;

    // v1.11.0: Effects attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> delayModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLowGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHighGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbDampAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbPredelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbMixAttachment;
    // Bypass toggle attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> chorusBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delayBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverbBypassAttachment;

    // v1.3.0: File chooser for tuning file dialogs
    std::shared_ptr<juce::FileChooser> tuningFileChooser;

    // Helpers
    bool parseAndApplyIntervals(const juce::String& jsonStr, const juce::String& name);
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OIntonationPadAudioProcessorEditor)
};
