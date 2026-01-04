#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Scatter WebView-based Plugin Editor
 *
 * This template demonstrates the CORRECT member declaration order for WebView plugins.
 * CRITICAL: Member order prevents 90% of release build crashes.
 *
 * Destruction order (reverse of declaration):
 * 1. Attachments destroyed FIRST (stop using relays and WebView)
 * 2. WebView destroyed SECOND (safe, attachments are gone)
 * 3. Relays destroyed LAST (safe, nothing using them)
 *
 * WRONG order (attachments before WebView) causes:
 * - Destructor tries to call evaluateJavascript() on destroyed WebView
 * - Undefined behavior in release builds (optimization breaks assumptions)
 * - CRASH only in release builds (debug builds hide the bug)
 */

class ScatterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    /**
     * Constructor
     * @param p Reference to the audio processor
     */
    ScatterAudioProcessorEditor(ScatterAudioProcessor& p);

    /**
     * Destructor
     * Members destroyed in reverse order of declaration.
     * This is why member order matters!
     */
    ~ScatterAudioProcessorEditor() override;

    // AudioProcessorEditor overrides
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    /**
     * Resource provider (JUCE 8 required pattern)
     * Maps URLs to embedded binary data.
     *
     * @param url Requested resource URL (e.g., "/", "/js/juce/index.js")
     * @return Resource data and MIME type, or std::nullopt for 404
     */
    std::optional<juce::WebBrowserComponent::Resource> getResource(
        const juce::String& url
    );

    // Reference to audio processor
    ScatterAudioProcessor& audioProcessor;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    //
    // Order: Relays → WebView → Attachments
    //
    // Why: Members are destroyed in REVERSE order of declaration.
    // - Attachments must be destroyed BEFORE WebView (they call evaluateJavascript)
    // - WebView must be destroyed BEFORE Relays (it holds references via Options)
    //
    // DO NOT REORDER without understanding destructor sequence!
    // ========================================================================

    // ------------------------------------------------------------------------
    // 1️⃣ RELAYS FIRST (created first, destroyed last)
    // ------------------------------------------------------------------------
    //
    // Relays bridge C++ parameters to JavaScript state objects.
    // They have no dependencies, so they're declared first.
    //
    // Total: 9 parameters (7 sliders + 2 combo boxes)
    //
    std::unique_ptr<juce::WebSliderRelay> delayTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> densityRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchRandomRelay;
    std::unique_ptr<juce::WebComboBoxRelay> scaleRelay;
    std::unique_ptr<juce::WebComboBoxRelay> rootNoteRelay;
    std::unique_ptr<juce::WebSliderRelay> panRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;

    // ------------------------------------------------------------------------
    // 2️⃣ WEBVIEW SECOND (created after relays, destroyed before relays)
    // ------------------------------------------------------------------------
    //
    // WebBrowserComponent is the HTML rendering engine.
    // It depends on relays (registered via withOptionsFrom).
    //
    // Must be destroyed AFTER attachments (they call evaluateJavascript).
    // Must be destroyed BEFORE relays (holds references to them).
    //
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ------------------------------------------------------------------------
    // 3️⃣ PARAMETER ATTACHMENTS LAST (created last, destroyed first)
    // ------------------------------------------------------------------------
    //
    // Attachments synchronize APVTS parameters with relay state.
    // They depend on BOTH the relay AND the WebView.
    //
    // MUST be declared AFTER WebView to ensure correct destruction order.
    //
    std::unique_ptr<juce::WebSliderParameterAttachment> delayTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> densityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchRandomAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> scaleAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> rootNoteAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> panRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScatterAudioProcessorEditor)
};
