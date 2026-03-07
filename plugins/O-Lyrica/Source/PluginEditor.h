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

#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicenseUI.h"
#endif

class OLyricaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
#if OUARICON_LICENSING_ENABLED
                                        , private OuariconLicense::Listener
#endif
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
    // v1.21.0: Glissando speed (notes per second)
    std::unique_ptr<juce::WebSliderRelay> glissandoSpeedRelay;
    // v1.23.0: Glissando custom semitones (for Custom interval)
    std::unique_ptr<juce::WebSliderRelay> glissandoCustomSemitonesRelay;
    // v1.24.0: Glissando humanize (timing jitter)
    std::unique_ptr<juce::WebSliderRelay> glissandoHumanizeRelay;
    // v1.25.0: Glissando time (Free mode ramp duration)
    std::unique_ptr<juce::WebSliderRelay> glissandoTimeRelay;
    // v1.26.0: Glissando excitation softness
    std::unique_ptr<juce::WebSliderRelay> glissandoExcitationRelay;
    // v1.27.0: Glissando velocity profile
    std::unique_ptr<juce::WebSliderRelay> glissandoVelStartRelay;
    std::unique_ptr<juce::WebSliderRelay> glissandoVelEndRelay;

    std::unique_ptr<juce::WebComboBoxRelay> stringMaterialRelay;
    std::unique_ptr<juce::WebComboBoxRelay> woodTypeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> techniqueRelay;
    std::unique_ptr<juce::WebComboBoxRelay> glissandoScaleRelay;
    // v1.22.0: Glissando shape relay
    std::unique_ptr<juce::WebComboBoxRelay> glissandoShapeRelay;
    // v1.30.0: Glissando tonic relay
    std::unique_ptr<juce::WebComboBoxRelay> glissandoTonicRelay;
    // v1.23.0: Glissando interval and direction relays
    std::unique_ptr<juce::WebComboBoxRelay> glissandoIntervalRelay;
    std::unique_ptr<juce::WebComboBoxRelay> glissandoDirectionRelay;
    // v1.6.0: Tuning mode relay
    std::unique_ptr<juce::WebComboBoxRelay> tuningModeRelay;
    // v1.30.0: Glissando toggle relays
    std::unique_ptr<juce::WebToggleButtonRelay> freeToggleRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> scaleToggleRelay;
    // v1.30.0: Free mode parameter relays
    std::unique_ptr<juce::WebComboBoxRelay> freeShapeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> freeIntervalRelay;
    std::unique_ptr<juce::WebComboBoxRelay> freeDirectionRelay;
    std::unique_ptr<juce::WebComboBoxRelay> freeKeyswitchNoteRelay;
    std::unique_ptr<juce::WebComboBoxRelay> scaleKeyswitchNoteRelay;
    std::unique_ptr<juce::WebSliderRelay> freeCustomSemitonesRelay;
    // v1.31.0: Tempo sync relays
    std::unique_ptr<juce::WebComboBoxRelay> freeTempoSyncRelay;
    std::unique_ptr<juce::WebComboBoxRelay> scaleTempoSyncRelay;
    // v1.35.1: Effects chain relays
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
    std::unique_ptr<juce::WebComboBoxRelay> delayModeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> chorusBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> delayBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqBypassRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> reverbBypassRelay;

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
    // v1.21.0: Glissando speed attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoSpeedAttachment;
    // v1.23.0: Glissando custom semitones attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoCustomSemitonesAttachment;
    // v1.24.0: Glissando humanize attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoHumanizeAttachment;
    // v1.25.0: Glissando time attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoTimeAttachment;
    // v1.26.0: Glissando excitation attachment
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoExcitationAttachment;
    // v1.27.0: Glissando velocity profile attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoVelStartAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> glissandoVelEndAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> stringMaterialAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> woodTypeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> techniqueAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoScaleAttachment;
    // v1.22.0: Glissando shape attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoShapeAttachment;
    // v1.30.0: Glissando tonic attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoTonicAttachment;
    // v1.23.0: Glissando interval and direction attachments
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoIntervalAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> glissandoDirectionAttachment;
    // v1.6.0: Tuning mode attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningModeAttachment;
    // v1.30.0: Glissando toggle attachments
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freeToggleAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> scaleToggleAttachment;
    // v1.30.0: Free mode parameter attachments
    std::unique_ptr<juce::WebComboBoxParameterAttachment> freeShapeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> freeIntervalAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> freeDirectionAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> freeKeyswitchNoteAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> scaleKeyswitchNoteAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> freeCustomSemitonesAttachment;
    // v1.31.0: Tempo sync attachments
    std::unique_ptr<juce::WebComboBoxParameterAttachment> freeTempoSyncAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> scaleTempoSyncAttachment;
    // v1.35.1: Effects chain attachments
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
    std::unique_ptr<juce::WebComboBoxParameterAttachment> delayModeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> chorusBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> delayBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> eqBypassAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverbBypassAttachment;

    // Helper for serving UI resources from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // v1.7.10: File chooser for preset save/load dialogs
    // Uses shared_ptr so async callbacks keep FileChooser alive even if user
    // triggers another dialog before the first one completes
    std::shared_ptr<juce::FileChooser> fileChooser;

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicenseOverlay> licenseOverlay;
    void licenseStatusChanged(OuariconLicense&, OuariconLicense::Status) override;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OLyricaAudioProcessorEditor)
};
