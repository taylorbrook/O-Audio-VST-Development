/*
  ==============================================================================

    O-Freeze - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OFreezeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OFreezeAudioProcessorEditor(OFreezeAudioProcessor&);
    ~OFreezeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OFreezeAudioProcessor& processorRef;

    // CRITICAL MEMBER ORDER: relays → webView → attachments
    // Members destroyed in REVERSE order - wrong order causes release build crashes

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> thresholdRelay;
    std::unique_ptr<juce::WebSliderRelay> driftRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> modeRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. NAVIGATION FLAG
    bool hasNavigated = false;

    // 4. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driftAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modeAttachment;

    // Helper for resource serving
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreezeAudioProcessorEditor)
};
