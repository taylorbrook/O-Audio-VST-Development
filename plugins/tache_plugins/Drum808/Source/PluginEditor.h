#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class Drum808AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Drum808AudioProcessorEditor(Drum808AudioProcessor&);
    ~Drum808AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    Drum808AudioProcessor& processorRef;

    // CRITICAL: Member declaration order prevents 90% of release build crashes
    // Members destroyed in REVERSE order: attachments → webView → relays

    // 1. Relays FIRST (no dependencies) - 24 parameters
    std::unique_ptr<juce::WebSliderRelay> kickLevelRelay, kickToneRelay, kickDecayRelay, kickTuningRelay;
    std::unique_ptr<juce::WebSliderRelay> lowtomLevelRelay, lowtomToneRelay, lowtomDecayRelay, lowtomTuningRelay;
    std::unique_ptr<juce::WebSliderRelay> midtomLevelRelay, midtomToneRelay, midtomDecayRelay, midtomTuningRelay;
    std::unique_ptr<juce::WebSliderRelay> clapLevelRelay, clapToneRelay, clapSnapRelay, clapTuningRelay;
    std::unique_ptr<juce::WebSliderRelay> closedhatLevelRelay, closedhatToneRelay, closedhatDecayRelay, closedhatTuningRelay;
    std::unique_ptr<juce::WebSliderRelay> openhatLevelRelay, openhatToneRelay, openhatDecayRelay, openhatTuningRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments LAST (depend on both relays and webView) - 24 parameters
    std::unique_ptr<juce::WebSliderParameterAttachment> kickLevelAttachment, kickToneAttachment, kickDecayAttachment, kickTuningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lowtomLevelAttachment, lowtomToneAttachment, lowtomDecayAttachment, lowtomTuningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> midtomLevelAttachment, midtomToneAttachment, midtomDecayAttachment, midtomTuningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> clapLevelAttachment, clapToneAttachment, clapSnapAttachment, clapTuningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> closedhatLevelAttachment, closedhatToneAttachment, closedhatDecayAttachment, closedhatTuningAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> openhatLevelAttachment, openhatToneAttachment, openhatDecayAttachment, openhatTuningAttachment;

    // Resource provider
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Drum808AudioProcessorEditor)
};
