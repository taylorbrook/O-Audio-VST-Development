/*
  ==============================================================================

    OuariconTremolo - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OuariconTremoloAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconTremoloAudioProcessorEditor(OuariconTremoloAudioProcessor&);
    ~OuariconTremoloAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OuariconTremoloAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11)
    // 1. Relays FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> speedRelay;
    std::unique_ptr<juce::WebSliderRelay> depthRelay;
    std::unique_ptr<juce::WebSliderRelay> waveformRelay;
    std::unique_ptr<juce::WebSliderRelay> smoothingRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> panSyncRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> tempoSyncRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state (per-instance, not static)
    bool hasNavigated = false;

    // 3. Attachments LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> speedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> depthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> waveformAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> smoothingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> panSyncAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> tempoSyncAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTremoloAudioProcessorEditor)
};
