#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class AngelGrainAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AngelGrainAudioProcessorEditor(AngelGrainAudioProcessor&);
    ~AngelGrainAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AngelGrainAudioProcessor& processorRef;

    // CRITICAL: Member declaration order prevents release build crashes
    // Destruction happens in REVERSE order of declaration
    // Order: Relays -> WebView -> Attachments

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> delayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> chaosRelay;
    std::unique_ptr<juce::WebSliderRelay> characterRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> tempoSyncRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on both relays and parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> delayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> chaosAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> tempoSyncAttachment;

    // Helper for resource serving
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AngelGrainAudioProcessorEditor)
};
