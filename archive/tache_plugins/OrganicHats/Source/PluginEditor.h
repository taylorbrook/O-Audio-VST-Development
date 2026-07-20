#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

/**
 * OrganicHats Plugin Editor with WebView UI
 *
 * UI Mockup: v2 (600×590px industrial rack unit)
 * Parameters: 6 total (3 closed + 3 open hi-hat controls)
 *
 * CRITICAL: Member order follows Pattern #11
 * Order: relays → webView → attachments
 */
class OrganicHatsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OrganicHatsAudioProcessorEditor(OrganicHatsAudioProcessor&);
    ~OrganicHatsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    OrganicHatsAudioProcessor& processorRef;

    // Helper for resource serving (Pattern #8)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // ⚠️ CRITICAL: Member order matters for release builds
    // Order: relays → webView → attachments

    // WebView relays (created BEFORE webView)
    std::unique_ptr<juce::WebSliderRelay> closedToneRelay;
    std::unique_ptr<juce::WebSliderRelay> closedDecayRelay;
    std::unique_ptr<juce::WebSliderRelay> closedNoiseColorRelay;
    std::unique_ptr<juce::WebSliderRelay> openToneRelay;
    std::unique_ptr<juce::WebSliderRelay> openReleaseRelay;
    std::unique_ptr<juce::WebSliderRelay> openNoiseColorRelay;

    // WebView component
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Parameter attachments (created AFTER webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> closedToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> closedDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> closedNoiseColorAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openToneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openReleaseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openNoiseColorAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganicHatsAudioProcessorEditor)
};
