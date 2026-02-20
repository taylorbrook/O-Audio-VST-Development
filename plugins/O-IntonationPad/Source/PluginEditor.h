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

class OIntonationPadAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor&);
    ~OIntonationPadAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;

    OIntonationPadAudioProcessor& processorRef;

    // Member declaration order matters: relays → webView → attachments
    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> voiceCountRelay;
    std::unique_ptr<juce::WebSliderRelay> complexityRelay;
    std::unique_ptr<juce::WebSliderRelay> keyRootRelay;
    std::unique_ptr<juce::WebSliderRelay> spacingRelay;
    std::unique_ptr<juce::WebSliderRelay> inversionRelay;
    std::unique_ptr<juce::WebSliderRelay> wavetablePosRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> timingRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> detuneRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> filterCutoffRelay;
    std::unique_ptr<juce::WebSliderRelay> masterVolumeRelay;

    // v1.6.0: Wavetable bank relay
    std::unique_ptr<juce::WebComboBoxRelay> wavetableBankRelay;

    // v1.3.0: Tuning module relays
    std::unique_ptr<juce::WebSliderRelay> tuningMasterTuneRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningOctaveStretchRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningPitchBendRangeRelay;
    std::unique_ptr<juce::WebComboBoxRelay> tuningTemperamentPresetRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state
    bool hasNavigated = false;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> voiceCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> complexityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> keyRootAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spacingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> inversionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wavetablePosAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> timingRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> masterVolumeAttachment;

    // v1.6.0: Wavetable bank attachment
    std::unique_ptr<juce::WebComboBoxParameterAttachment> wavetableBankAttachment;

    // v1.3.0: Tuning module attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningMasterTuneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningOctaveStretchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningPitchBendRangeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningTemperamentPresetAttachment;

    // v1.3.0: File chooser for tuning file dialogs
    std::shared_ptr<juce::FileChooser> tuningFileChooser;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OIntonationPadAudioProcessorEditor)
};
