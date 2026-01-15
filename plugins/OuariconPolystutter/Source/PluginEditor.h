/*
  ==============================================================================

    Ouaricon Polystutter - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <optional>

class OuariconPolystutterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconPolystutterAudioProcessorEditor(OuariconPolystutterAudioProcessor&);
    ~OuariconPolystutterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconPolystutterAudioProcessor& processorRef;

    // Phase 3.1: WebView for static layout rendering (no parameter bindings yet)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Resource provider for serving HTML/CSS/JS from BinaryData
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconPolystutterAudioProcessorEditor)
};
