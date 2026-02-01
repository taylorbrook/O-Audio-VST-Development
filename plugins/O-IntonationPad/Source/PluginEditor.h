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

class OIntonationPadAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor&);
    ~OIntonationPadAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OIntonationPadAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11)
    // 1. RELAYS FIRST (no dependencies) - 15 parameters
    std::unique_ptr<juce::WebSliderRelay> voiceCountRelay;
    std::unique_ptr<juce::WebSliderRelay> complexityRelay;
    std::unique_ptr<juce::WebSliderRelay> keyRootRelay;
    std::unique_ptr<juce::WebSliderRelay> keyScaleRelay;
    std::unique_ptr<juce::WebSliderRelay> inversionRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> tuningSystemRelay;
    std::unique_ptr<juce::WebSliderRelay> wavetablePosRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoRateRelay;
    std::unique_ptr<juce::WebSliderRelay> lfoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay> timingRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> detuneRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> filterCutoffRelay;
    std::unique_ptr<juce::WebSliderRelay> masterVolumeRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state
    bool hasNavigated = false;

    // 3. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> voiceCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> complexityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> keyRootAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> keyScaleAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> inversionRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tuningSystemAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wavetablePosAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> timingRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> detuneRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> masterVolumeAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OIntonationPadAudioProcessorEditor)
};
