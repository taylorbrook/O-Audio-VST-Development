/*
  ==============================================================================

    OuariconSimpleReverb - Editor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OuariconSimpleReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit OuariconSimpleReverbAudioProcessorEditor(OuariconSimpleReverbAudioProcessor&);
    ~OuariconSimpleReverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    OuariconSimpleReverbAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (Pattern #11)
    // 1. Relays FIRST (no dependencies)
    std::unique_ptr<juce::WebComboBoxRelay> typeRelay;
    std::unique_ptr<juce::WebSliderRelay> characterRelay;
    std::unique_ptr<juce::WebSliderRelay> wetRelay;
    std::unique_ptr<juce::WebSliderRelay> dryRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> lpFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> lpOnRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> typeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lpFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lpOnAttachment;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    // File chooser for save/load dialogs
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSimpleReverbAudioProcessorEditor)
};
