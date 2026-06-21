/*
  ==============================================================================

    O-simpleFM - Plugin Editor

    Stage 3 (GUI): single-page Ouaricon-Naturalist WebView UI. Binds all 17
    APVTS parameters two-way via Web*Relay / Web*ParameterAttachment, and pushes
    the live spectrum (256 dB bins) + oscilloscope (128 pts) to the page on the
    30 Hz message-thread Timer (FmVizAnalyzer runs here, off the audio thread).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleFMAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit OSimpleFMAudioProcessorEditor (OSimpleFMAudioProcessor&);
    ~OSimpleFMAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleFMAudioProcessor& processorRef;

    // Message-thread FFT + scope (already exercised in Stage 2).
    FmVizAnalyzer vizAnalyzer;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS
    std::vector<std::unique_ptr<juce::WebSliderRelay>>        sliderRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>  toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleFMAudioProcessorEditor)
};
