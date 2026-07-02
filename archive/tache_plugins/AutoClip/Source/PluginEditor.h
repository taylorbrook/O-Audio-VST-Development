#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class AutoClipAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit AutoClipAudioProcessorEditor(AutoClipAudioProcessor&);
    ~AutoClipAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AutoClipAudioProcessor& processorRef;

    // Pattern #11: Member declaration order (CRITICAL)
    // Order: Relays → WebView → Attachments
    // Destroyed in REVERSE order (prevents 90% of crashes)

    // 1. Relays (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> clipThresholdRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> soloClippedRelay;

    // 2. WebView (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> clipThresholdAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> soloClippedAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // Phase 5.3: Metering (send meter data to UI)
    void timerCallback() override;
    float smoothedInputPeak = 0.0f;
    float smoothedOutputPeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoClipAudioProcessorEditor)
};
