#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * FlutterVerb WebView-based Plugin Editor
 *
 * UI Mockup: v6 (600×640px, TapeAge-inspired design)
 * Parameters: 7 total (SIZE, DECAY, MIX, AGE, DRIVE, TONE, MOD_MODE)
 *
 * CRITICAL: Member declaration order prevents release build crashes.
 * Order: Relays → WebView → Attachments
 *
 * Destruction order (reverse of declaration):
 * 1. Attachments destroyed FIRST (stop using relays and WebView)
 * 2. WebView destroyed SECOND (safe, attachments are gone)
 * 3. Relays destroyed LAST (safe, nothing using them)
 */

class FlutterVerbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    FlutterVerbAudioProcessorEditor(FlutterVerbAudioProcessor& p);
    ~FlutterVerbAudioProcessorEditor() override;

    // AudioProcessorEditor overrides
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for VU meter updates
    void timerCallback() override;

    /**
     * Resource provider (JUCE 8 required pattern)
     * Maps URLs to embedded binary data from Source/ui/public/
     *
     * @param url Requested resource URL (e.g., "/", "/index.html", "/js/juce/index.js")
     * @return Resource data and MIME type, or std::nullopt for 404
     */
    std::optional<juce::WebBrowserComponent::Resource> getResource(
        const juce::String& url
    );

    // Reference to audio processor
    FlutterVerbAudioProcessor& audioProcessor;

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
    // FlutterVerb has 6 continuous parameters + 1 toggle:
    // - SIZE, DECAY, MIX, AGE, DRIVE, TONE (sliders/knobs)
    // - MOD_MODE (toggle: Wet Only / Wet+Dry)
    //
    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> ageRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> toneRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> modModeRelay;

    // ------------------------------------------------------------------------
    // 2️⃣ WEBVIEW SECOND (created after relays, destroyed before relays)
    // ------------------------------------------------------------------------
    //
    // WebBrowserComponent renders Source/ui/public/index.html
    // Registered with all relays via .withOptionsFrom(*relay)
    //
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ------------------------------------------------------------------------
    // 3️⃣ PARAMETER ATTACHMENTS LAST (created last, destroyed first)
    // ------------------------------------------------------------------------
    //
    // Synchronize APVTS parameters with WebView relay state
    //
    // 6 slider attachments (continuous parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ageAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneAttachment;

    // 1 toggle attachment (boolean parameter)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlutterVerbAudioProcessorEditor)
};
