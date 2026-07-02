#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
/**
 * AutoClip Plugin Editor with WebView UI
 *
 * CRITICAL: Member order matters for initialization
 * Order MUST be: relays → webView → attachments
 *
 * This prevents crashes in release builds where incorrect initialization
 * order can cause accessing uninitialized members.
 */
class AutoClipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AutoClipAudioProcessorEditor (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AutoClipAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Reference to the processor's APVTS
    juce::AudioProcessorValueTreeState& apvts;

    //==============================================================================
    // CRITICAL: Member declaration order MUST match initialization order
    // Order: relays → webView → attachments

    // 1. Parameter relays (declared first, initialized first)
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay clipThresholdRelay;
    juce::AudioProcessorValueTreeState::ButtonAttachment::Relay soloClippedRelay;

    // 2. WebView component (declared second, initialized second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Parameter attachments (declared third, initialized third)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipThresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloClippedAttachment;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoClipAudioProcessorEditor)
};
