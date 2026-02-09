#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
/**
 * OuariconComp Audio Processor Editor - WebView UI
 *
 * This is a TEMPLATE file for gui-agent reference during Stage 3 (GUI).
 * Do NOT copy directly - adapt to actual plugin structure.
 *
 * Generated from: v8-ui.yaml mockup finalization
 * Parameters: 7 total (6 sliders + 1 toggle)
 */
class OuariconCompAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OuariconCompAudioProcessorEditor(OuariconCompAudioProcessor&);
    ~OuariconCompAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Reference to processor
    OuariconCompAudioProcessor& audioProcessor;

    //==============================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order
    // Attachments call evaluateJavascript() during destruction
    // Therefore attachments MUST be destroyed BEFORE webView
    //==============================================================================

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> thresholdRelay;
    std::unique_ptr<juce::WebSliderRelay> ratioRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> kneeRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autoGainRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ratioAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> kneeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autoGainAttachment;

    //==============================================================================
    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconCompAudioProcessorEditor)
};
