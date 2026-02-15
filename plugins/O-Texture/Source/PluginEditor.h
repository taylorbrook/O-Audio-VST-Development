/*
  ==============================================================================

    O-Texture - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TextureEditor : public juce::AudioProcessorEditor
{
public:
    explicit TextureEditor(TextureProcessor&);
    ~TextureEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    TextureProcessor& processorRef;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE declaration order (C++ standard)
    // Stage 1: No relays or attachments yet (added in Stage 3)
    // ========================================================================

    // 1. RELAYS - (Stage 3: declare here, before webView)

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS - (Stage 3: declare here, after webView)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureEditor)
};
