#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * OuariconMarimba WebView-based Plugin Editor
 *
 * CRITICAL: Member order prevents release build crashes.
 * Order: Relays → WebView → Attachments
 *
 * Destruction order (reverse of declaration):
 * 1. Attachments destroyed FIRST (stop using relays and WebView)
 * 2. WebView destroyed SECOND (safe, attachments are gone)
 * 3. Relays destroyed LAST (safe, nothing using them)
 */

class OuariconMarimbaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OuariconMarimbaAudioProcessorEditor(OuariconMarimbaAudioProcessor& p);
    ~OuariconMarimbaAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    /**
     * Resource provider for embedded UI files
     * Maps URLs to binary data from juce_add_binary_data
     */
    std::optional<juce::WebBrowserComponent::Resource> getResource(
        const juce::String& url
    );

    // Reference to audio processor
    OuariconMarimbaAudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order (attachments first, relays last)
    // ========================================================================

    // ------------------------------------------------------------------------
    // 1️⃣ RELAYS FIRST (created first, destroyed last)
    // ------------------------------------------------------------------------
    std::unique_ptr<juce::WebSliderRelay> malletHardnessRelay;
    std::unique_ptr<juce::WebSliderRelay> barMaterialRelay;
    std::unique_ptr<juce::WebSliderRelay> resonanceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> tuningModeRelay;
    std::unique_ptr<juce::WebSliderRelay> referencePitchRelay;
    std::unique_ptr<juce::WebSliderRelay> velCurveRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // ------------------------------------------------------------------------
    // 2️⃣ WEBVIEW SECOND (created after relays, destroyed before relays)
    // ------------------------------------------------------------------------
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ------------------------------------------------------------------------
    // 3️⃣ PARAMETER ATTACHMENTS LAST (created last, destroyed first)
    // ------------------------------------------------------------------------
    std::unique_ptr<juce::WebSliderParameterAttachment> malletHardnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> barMaterialAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> tuningModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> referencePitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> velCurveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconMarimbaAudioProcessorEditor)
};
