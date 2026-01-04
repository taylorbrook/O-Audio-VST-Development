#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class ScatterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit ScatterAudioProcessorEditor(ScatterAudioProcessor&);
    ~ScatterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Phase 4.2: Timer callback for grain visualization updates (30Hz)
    void timerCallback() override;

private:
    ScatterAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11)
    // Relays → WebView → Attachments (destroyed in reverse order)

    // 1. Relays (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> delayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> densityRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchRandomRelay;
    std::unique_ptr<juce::WebComboBoxRelay> scaleRelay;
    std::unique_ptr<juce::WebComboBoxRelay> rootNoteRelay;
    std::unique_ptr<juce::WebSliderRelay> panRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;

    // 2. WebView (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> delayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> densityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchRandomAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> scaleAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> rootNoteAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> panRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;

    // Helper for resource serving
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScatterAudioProcessorEditor)
};
