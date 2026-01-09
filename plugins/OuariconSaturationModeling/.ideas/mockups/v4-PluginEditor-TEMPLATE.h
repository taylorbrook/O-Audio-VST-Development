/*
  ==============================================================================

    OuariconSaturationModelingAudioProcessorEditor.h

    TEMPLATE FILE - Generated from v4 UI mockup finalization

    This is a REFERENCE TEMPLATE for gui-agent to adapt during Stage 3 (GUI).
    DO NOT copy-paste directly - gui-agent will integrate into actual plugin structure.

    Generated: 2026-01-09
    Plugin: OuariconSaturationModeling
    Version: v4

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Forward declaration
class OuariconSaturationModelingAudioProcessor;

/**
 * WebView-based plugin editor for OuariconSaturationModeling
 *
 * CRITICAL PATTERN: Member declaration order (JUCE 8 critical pattern #11)
 * Order: Relays → WebView → Attachments
 *
 * Why: Members destroyed in REVERSE order. Attachments call evaluateJavascript()
 * during destruction, so they MUST be destroyed BEFORE WebView.
 */
class OuariconSaturationModelingAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                        private juce::Timer
{
public:
    OuariconSaturationModelingAudioProcessorEditor(OuariconSaturationModelingAudioProcessor&);
    ~OuariconSaturationModelingAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Timer callback for VU meter updates (30-60 Hz recommended)
    void timerCallback() override;

    //==============================================================================
    // WebView resource provider (JUCE 8 critical pattern #8)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    //==============================================================================
    // VU meter level calculations
    float calculateInputLevel();   // Returns dB value for input meter
    float calculateOutputLevel();  // Returns dB value for output meter

    //==============================================================================
    // Reference to processor
    OuariconSaturationModelingAudioProcessor& audioProcessor;

    //==============================================================================
    // CRITICAL MEMBER DECLARATION ORDER (prevents release build crashes)
    // Order: Relays → WebView → Attachments
    // Members destroyed in REVERSE order of declaration
    //==============================================================================

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> intensityRelay;
    std::unique_ptr<juce::WebSliderRelay> modelRelay;
    std::unique_ptr<juce::WebSliderRelay> qualityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autogainRelay;

    // 2. WEBVIEW SECOND (depends on relays for options)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on relays AND webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> intensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> qualityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autogainAttachment;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessorEditor)
};
