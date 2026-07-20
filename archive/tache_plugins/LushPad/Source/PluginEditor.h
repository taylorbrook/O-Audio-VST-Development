#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class LushPadAudioProcessor;

class LushPadAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    LushPadAudioProcessorEditor(LushPadAudioProcessor&);
    ~LushPadAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    LushPadAudioProcessor& processorRef;

    // Order: Relays → WebView → Attachments (Pattern #11)
    std::unique_ptr<juce::WebSliderRelay> timbreRelay;
    std::unique_ptr<juce::WebSliderRelay> filterCutoffRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbAmountRelay;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> timbreAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbAmountAttachment;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LushPadAudioProcessorEditor)
};
