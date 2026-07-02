#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * LushPad WebView-based Plugin Editor
 *
 * CRITICAL: Member declaration order prevents 90% of release build crashes.
 *
 * Destruction order (reverse of declaration):
 * 1. Attachments destroyed FIRST (stop using relays and WebView)
 * 2. WebView destroyed SECOND (safe, attachments are gone)
 * 3. Relays destroyed LAST (safe, nothing using them)
 *
 * See: troubleshooting/patterns/juce8-critical-patterns.md Pattern #11
 */

class LushPadAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    LushPadAudioProcessorEditor(LushPadAudioProcessor& p);
    ~LushPadAudioProcessorEditor() override;

    // AudioProcessorEditor overrides
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    /**
     * Resource provider (JUCE 8 required pattern)
     * Maps URLs to embedded binary data.
     *
     * Pattern #8: Explicit URL mapping required
     */
    std::optional<juce::WebBrowserComponent::Resource> getResource(
        const juce::String& url
    );

    // Reference to audio processor
    LushPadAudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    //
    // Order: Relays → WebView → Attachments
    //
    // Members are destroyed in REVERSE order of declaration.
    // - Attachments must be destroyed BEFORE WebView (they call evaluateJavascript)
    // - WebView must be destroyed BEFORE Relays (it holds references via Options)
    //
    // DO NOT REORDER without understanding destructor sequence!
    // ========================================================================

    // ------------------------------------------------------------------------
    // 1️⃣ RELAYS FIRST (created first, destroyed last)
    // ------------------------------------------------------------------------
    //
    // 3 parameters from v2-ui.yaml:
    // - timbre: Float slider (0.0 - 1.0)
    // - filter_cutoff: Float slider (20 - 20000 Hz, skew 0.3)
    // - reverb_amount: Float slider (0.0 - 1.0)
    //
    std::unique_ptr<juce::WebSliderRelay> timbreRelay;
    std::unique_ptr<juce::WebSliderRelay> filterCutoffRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbAmountRelay;

    // ------------------------------------------------------------------------
    // 2️⃣ WEBVIEW SECOND (created after relays, destroyed before relays)
    // ------------------------------------------------------------------------
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ------------------------------------------------------------------------
    // 3️⃣ PARAMETER ATTACHMENTS LAST (created last, destroyed first)
    // ------------------------------------------------------------------------
    //
    // Attachments synchronize APVTS parameters with relay state.
    // MUST be declared AFTER WebView to ensure correct destruction order.
    //
    std::unique_ptr<juce::WebSliderParameterAttachment> timbreAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbAmountAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LushPadAudioProcessorEditor)
};
