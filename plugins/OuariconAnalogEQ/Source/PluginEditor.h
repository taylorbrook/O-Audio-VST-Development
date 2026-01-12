#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * OuariconAnalogEQ Plugin Editor with WebView UI
 *
 * UI Mockup: v3 (920×220px compact rack-unit style)
 * Parameters: 16 total (4 bands × 3 params + output + analog)
 */
class OuariconAnalogEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    OuariconAnalogEQAudioProcessorEditor(OuariconAnalogEQAudioProcessor&);
    ~OuariconAnalogEQAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

private:
    // Reference to processor
    OuariconAnalogEQAudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order (automatic in C++)
    // ========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies)
    // LF Band
    std::unique_ptr<juce::WebSliderRelay> lfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> lfGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lfOnRelay;

    // LMF Band
    std::unique_ptr<juce::WebSliderRelay> lmfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> lmfGainRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lmfQRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lmfOnRelay;

    // HMF Band
    std::unique_ptr<juce::WebSliderRelay> hmfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> hmfGainRelay;
    std::unique_ptr<juce::WebComboBoxRelay> hmfQRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> hmfOnRelay;

    // HF Band
    std::unique_ptr<juce::WebSliderRelay> hfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> hfGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> hfOnRelay;

    // Global
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> analogRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    // LF Band
    std::unique_ptr<juce::WebSliderParameterAttachment> lfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lfOnAttachment;

    // LMF Band
    std::unique_ptr<juce::WebSliderParameterAttachment> lmfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lmfGainAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lmfQAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lmfOnAttachment;

    // HMF Band
    std::unique_ptr<juce::WebSliderParameterAttachment> hmfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> hmfGainAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> hmfQAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> hmfOnAttachment;

    // HF Band
    std::unique_ptr<juce::WebSliderParameterAttachment> hfFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> hfGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> hfOnAttachment;

    // Global
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> analogAttachment;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconAnalogEQAudioProcessorEditor)
};
