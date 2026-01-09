/*
  ==============================================================================

    OuariconSaturationModeling - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class OuariconSaturationModelingAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OuariconSaturationModelingAudioProcessorEditor(OuariconSaturationModelingAudioProcessor&);
    ~OuariconSaturationModelingAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OuariconSaturationModelingAudioProcessor& processorRef;

    // ⚠️ CRITICAL: MEMBER DECLARATION ORDER (Pattern 11)
    // Members destroyed in REVERSE order of declaration
    // Order: Relays → WebView → Attachments (prevents 90% of release build crashes)

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> intensityRelay;
    std::unique_ptr<juce::WebSliderRelay> modelRelay;
    std::unique_ptr<juce::WebSliderRelay> qualityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autogainRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> intensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> qualityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autogainAttachment;

    // Helper for resource serving (Pattern 8: Explicit URL mapping)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessorEditor)
};
