#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * DriveVerb WebView-based Plugin Editor
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

class DriveVerbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    /**
     * Constructor
     * @param p Reference to the audio processor
     */
    DriveVerbAudioProcessorEditor(DriveVerbAudioProcessor& p);

    /**
     * Destructor
     * Members destroyed in reverse order of declaration.
     * This is why member order matters!
     */
    ~DriveVerbAudioProcessorEditor() override;

    // AudioProcessorEditor overrides
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for VU meter updates
    void timerCallback() override;

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
    DriveVerbAudioProcessor& audioProcessor;

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
    // Parameter mapping:
    // - size          → WebSliderRelay (0-100%, room size)
    // - decay         → WebSliderRelay (0.5s-10s, tail length)
    // - dryWet        → WebSliderRelay (0-100%, mix)
    // - drive         → WebSliderRelay (0-24dB, saturation)
    // - filter        → WebSliderRelay (-100% to +100%, DJ-style filter)
    // - filterPosition → WebToggleButtonRelay (PRE=0, POST=1)

    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> dryWetRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> filterRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> filterPositionRelay;

    // ------------------------------------------------------------------------
    // 2️⃣ WEBVIEW SECOND (created after relays, destroyed before relays)
    // ------------------------------------------------------------------------
    //
    // WebBrowserComponent is the HTML rendering engine.
    // It depends on relays (registered via withOptionsFrom).
    //
    // Must be destroyed AFTER attachments (they call evaluateJavascript).
    // Must be destroyed BEFORE relays (holds references to them).

    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ------------------------------------------------------------------------
    // 3️⃣ PARAMETER ATTACHMENTS LAST (created last, destroyed first)
    // ------------------------------------------------------------------------
    //
    // Attachments synchronize APVTS parameters with relay state.
    // They depend on BOTH the relay AND the WebView.
    //
    // MUST be declared AFTER WebView to ensure correct destruction order.

    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryWetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> filterPositionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriveVerbAudioProcessorEditor)
};
