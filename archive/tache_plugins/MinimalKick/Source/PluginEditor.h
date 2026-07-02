#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>  // Required for WebBrowserComponent
#include "PluginProcessor.h"

class MinimalKickAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MinimalKickAudioProcessorEditor(MinimalKickAudioProcessor&);
    ~MinimalKickAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MinimalKickAudioProcessor& processorRef;

    // Pattern #11: Member order is critical - Relays → WebView → Attachments
    // (Destroyed in reverse order to prevent use-after-free)

    // 1. Relays (created first, destroyed last)
    std::unique_ptr<juce::WebSliderRelay> sweepRelay;
    std::unique_ptr<juce::WebSliderRelay> timeRelay;
    std::unique_ptr<juce::WebSliderRelay> attackRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;

    // 2. WebView (created second, uses relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments (created last, destroyed first - depend on relays)
    std::unique_ptr<juce::WebSliderParameterAttachment> sweepAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> timeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;

    // Resource provider for serving HTML/JS files
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MinimalKickAudioProcessorEditor)
};
