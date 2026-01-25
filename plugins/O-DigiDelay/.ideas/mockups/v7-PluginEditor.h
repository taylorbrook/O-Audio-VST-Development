#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * TEMPLATE PluginEditor for Ouaricon Digital Delay v7
 *
 * This is a REFERENCE TEMPLATE for gui-agent during Stage 3 integration.
 * DO NOT copy-paste directly - gui-agent will adapt to actual plugin structure.
 *
 * Window: 700×196px (ultra-compact rack mount)
 * Parameters: 8 total (6 sliders, 1 toggle, 1 combo)
 * Visual Elements: 14-segment LED output meter (C++ -> JS messaging)
 */
class OuariconDigitalDelayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    OuariconDigitalDelayAudioProcessorEditor(OuariconDigitalDelayAudioProcessor&);
    ~OuariconDigitalDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    OuariconDigitalDelayAudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order (attachments before webView)
    // ========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> timeRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> spreadRelay;
    std::unique_ptr<juce::WebSliderRelay> modRelay;
    std::unique_ptr<juce::WebSliderRelay> wetRelay;
    std::unique_ptr<juce::WebSliderRelay> dryRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> syncRelay;
    std::unique_ptr<juce::WebComboBoxRelay> divisionRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> timeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> syncAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> divisionAttachment;

    // Resource provider for WebView
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconDigitalDelayAudioProcessorEditor)
};
