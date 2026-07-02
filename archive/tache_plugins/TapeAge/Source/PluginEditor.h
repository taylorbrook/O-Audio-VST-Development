#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class TapeAgeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::Timer
{
public:
    explicit TapeAgeAudioProcessorEditor(TapeAgeAudioProcessor&);
    ~TapeAgeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    TapeAgeAudioProcessor& processorRef;

    // ⚠️ CRITICAL: Member declaration order prevents release build crashes
    // Destruction happens in REVERSE order of declaration
    // Order: Relays → WebView → Attachments

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> inputRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> ageRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> outputRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> inputAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ageAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputAttachment;

    // Helper for resource serving
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeAgeAudioProcessorEditor)
};
