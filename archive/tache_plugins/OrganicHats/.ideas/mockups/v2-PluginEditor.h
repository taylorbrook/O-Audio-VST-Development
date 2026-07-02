#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class OrganicHatsAudioProcessor;

class OrganicHatsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OrganicHatsAudioProcessorEditor(OrganicHatsAudioProcessor&);
    ~OrganicHatsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OrganicHatsAudioProcessor& audioProcessor;

    // WebView parameter relays (MUST be declared BEFORE webView)
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay closedToneRelay;
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay closedDecayRelay;
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay closedNoiseColorRelay;
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay openToneRelay;
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay openReleaseRelay;
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay openNoiseColorRelay;

    // WebView component
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Parameter attachments (MUST be declared AFTER webView)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> closedToneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> closedDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> closedNoiseColorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> openToneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> openReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> openNoiseColorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganicHatsAudioProcessorEditor)
};
