/*
  ==============================================================================

    O-MultiBandCompressor - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 5.1: WebView layout with spectrum analyzer placeholder

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OMultiBandCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor&);
    ~OMultiBandCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    OMultiBandCompressorAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (prevents crashes in release builds)
    // Pattern from troubleshooting/patterns/stage-3-patterns.md

    // Phase 5.1: WebView only (no parameter bindings yet)
    // Parameter relays will be added in Phase 5.2

    // WebView component
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Navigation state
    bool hasNavigated = false;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMultiBandCompressorAudioProcessorEditor)
};
