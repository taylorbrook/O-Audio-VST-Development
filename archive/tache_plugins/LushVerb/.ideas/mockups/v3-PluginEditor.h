#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * LushVerb Plugin Editor with WebView UI
 *
 * UI Mockup: v3 (500×300px industrial rack unit)
 * Parameters: SIZE, DAMPING, SHIMMER, MIX (4 total)
 */
class LushVerbAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    LushVerbAudioProcessorEditor(LushVerbAudioProcessor&);
    ~LushVerbAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    LushVerbAudioProcessor& audioProcessor;

    // ⚠️ CRITICAL: Member order matters for release builds
    // Order: relays → webView → attachments

    // WebView relays (created BEFORE webView)
    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> dampingRelay;
    std::unique_ptr<juce::WebSliderRelay> shimmerRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;

    // WebView component
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // Parameter attachments (created AFTER webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> shimmerAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LushVerbAudioProcessorEditor)
};
