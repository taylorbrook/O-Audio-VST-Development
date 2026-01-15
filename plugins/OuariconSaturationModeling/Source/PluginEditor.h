/*
  ==============================================================================

    OuariconSaturationModeling - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class OuariconSaturationModelingAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                       private juce::Timer
{
public:
    explicit OuariconSaturationModelingAudioProcessorEditor(OuariconSaturationModelingAudioProcessor&);
    ~OuariconSaturationModelingAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    OuariconSaturationModelingAudioProcessor& processorRef;

    // Member declaration order matters for destruction safety:
    // Relays first, then WebView, then Attachments (destroyed in reverse)
    std::unique_ptr<juce::WebSliderRelay> intensityRelay;
    std::unique_ptr<juce::WebSliderRelay> modelRelay;
    std::unique_ptr<juce::WebSliderRelay> qualityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autogainRelay;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> intensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> qualityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autogainAttachment;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessorEditor)
};
