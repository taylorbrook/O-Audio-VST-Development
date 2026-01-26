/*
  ==============================================================================

    OBass - WebView Editor
    Ouaricon Audio
    Developer: Taylor Brook

    WebView-based editor with botanical aesthetic and JUCE 8 parameter binding.

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OBassAudioProcessorEditor(OBassAudioProcessor&);
    ~OBassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OBassAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11 from RESEARCH.md)
    // 1. Relays FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> frequencyRelay;
    std::unique_ptr<juce::WebSliderRelay> enhanceRelay;
    std::unique_ptr<juce::WebSliderRelay> outputRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> modeRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state (per-instance, not static)
    bool hasNavigated = false;

    // 3. Attachments LAST (depend on both relays and parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> frequencyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> enhanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modeAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for preset load/save dialogs
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassAudioProcessorEditor)
};
