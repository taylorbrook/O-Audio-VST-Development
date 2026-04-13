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

#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OBellsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                 , private OuariconLicense::Listener
#endif
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
    std::unique_ptr<juce::WebSliderRelay> overtoneBrightnessRelay;   // v2.0.0: renamed from brightnessRelay
    std::unique_ptr<juce::WebSliderRelay> acousticBrightnessRelay;   // v2.0.0: new
    std::unique_ptr<juce::WebSliderRelay> airAbsorptionRelay;       // v2.1.0: new
    std::unique_ptr<juce::WebSliderRelay> airAbsorptionTimeRelay;  // v2.2.0: independent time
    std::unique_ptr<juce::WebSliderRelay> inharmonicityRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomSpeedRelay;   // v1.4.0: Split bloom
    std::unique_ptr<juce::WebSliderRelay> bloomAmountRelay;
    // v1.5.0: Bloom fine controls (per-band)
    std::unique_ptr<juce::WebSliderRelay> bloomFineEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomSpeedLowRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomSpeedMidRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomSpeedHighRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomAmountLowRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomAmountMidRelay;
    std::unique_ptr<juce::WebSliderRelay> bloomAmountHighRelay;
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
    std::unique_ptr<juce::WebSliderRelay> humanizeRelay;  // v2.4.0
    std::unique_ptr<juce::WebSliderRelay> lpFilterEnabledRelay;  // v2.6.0
    std::unique_ptr<juce::WebSliderRelay> lpFilterCutoffRelay;   // v2.6.0
    std::unique_ptr<juce::WebSliderRelay> highFidelityRelay;     // v3.1.2
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // v4.0.0: Effects chain relays
    std::unique_ptr<juce::WebSliderRelay> chorusRateRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> chorusMixRelay;
    std::unique_ptr<juce::WebSliderRelay> fxDelayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> delayFeedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> delayMixRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLowGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidGainRelay;
    std::unique_ptr<juce::WebSliderRelay> eqMidFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqHighGainRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbDampRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbPredelayRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbMixRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbModRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbShimmerRelay;
    std::unique_ptr<juce::WebComboBoxRelay> delayModeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> chorusBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> delayBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> reverbBypassRelay;
    // Multi-stage envelope relays (4 sliders, active when decayShape == 2)
    std::unique_ptr<juce::WebSliderRelay> strikeTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> brillianceRelay;
    std::unique_ptr<juce::WebSliderRelay> bodyTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> humSustainRelay;

    // v3.0.0: Tuning parameter relays
    std::unique_ptr<juce::WebSliderRelay> tuningMasterTuneRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningOctaveStretchRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningPitchBendRangeRelay;

    // Choice parameter relays (3 combo boxes in v1.3.0 - material added)
    std::unique_ptr<juce::WebComboBoxRelay> materialRelay;
    std::unique_ptr<juce::WebComboBoxRelay> strikeNoiseCharRelay;
    std::unique_ptr<juce::WebComboBoxRelay> velocityCurveRelay;
    // v3.0.0: Tuning temperament combo relay
    std::unique_ptr<juce::WebComboBoxRelay> tuningTemperamentPresetRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    // Float/Int parameter attachments (17 sliders in v1.3.0 - material now ComboBox)
    std::unique_ptr<juce::WebSliderParameterAttachment> strikePositionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> malletHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> overtoneBrightnessAttachment;   // v2.0.0
    std::unique_ptr<juce::WebSliderParameterAttachment> acousticBrightnessAttachment;   // v2.0.0
    std::unique_ptr<juce::WebSliderParameterAttachment> airAbsorptionAttachment;       // v2.1.0
    std::unique_ptr<juce::WebSliderParameterAttachment> airAbsorptionTimeAttachment;  // v2.2.0
    std::unique_ptr<juce::WebSliderParameterAttachment> inharmonicityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomSpeedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomAmountAttachment;
    // v1.5.0: Bloom fine controls (per-band)
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomFineEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomSpeedLowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomSpeedMidAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomSpeedHighAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomAmountLowAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomAmountMidAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bloomAmountHighAttachment;
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
    std::unique_ptr<juce::WebSliderParameterAttachment> humanizeAttachment;  // v2.4.0
    std::unique_ptr<juce::WebSliderParameterAttachment> lpFilterEnabledAttachment;  // v2.6.0
    std::unique_ptr<juce::WebSliderParameterAttachment> lpFilterCutoffAttachment;   // v2.6.0
    std::unique_ptr<juce::WebSliderParameterAttachment> highFidelityAttachment;     // v3.1.2
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    // v4.0.0: Effects chain attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chorusMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> fxDelayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> delayMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLowGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqMidFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> eqHighGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbDampAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbPredelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbModAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbShimmerAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> delayModeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> chorusBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delayBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverbBypassAttachment;
    // Multi-stage envelope attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> strikeTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brillianceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> bodyTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> humSustainAttachment;

    // v3.0.0: Tuning parameter attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningMasterTuneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningOctaveStretchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningPitchBendRangeAttachment;

    // Choice parameter attachments (3 combo boxes in v1.3.0 - material added)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> materialAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> strikeNoiseCharAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> velocityCurveAttachment;
    // v3.0.0: Tuning temperament combo attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningTemperamentPresetAttachment;

    // v2.7.0: Previous note state for change detection
    uint64_t prevActiveNotesLow = 0;
    uint64_t prevActiveNotesHigh = 0;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset save/load dialogs
    std::shared_ptr<juce::FileChooser> fileChooser;

    // v3.0.0: File chooser for tuning file dialogs
    std::shared_ptr<juce::FileChooser> tuningFileChooser;

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBellsAudioProcessorEditor)
};
