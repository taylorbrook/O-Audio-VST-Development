/*
  ==============================================================================

    O-simpleSampler - Plugin Editor

    Stage 3 (GUI): single-page Ouaricon "Keyboard Sampler Field Guide" WebView UI.
    Binds all 21 APVTS parameters two-way via Web*Relay / Web*ParameterAttachment
    (17 sliders + 3 combo boxes + 1 toggle), serves the embedded field-guide page
    through a bare-path resource provider, and registers the source drag-drop /
    file-picker / on-screen-keyboard native functions (decode is C++-side on the
    processor).

    Phase 3.1 wires the layout + 21-param binding + load-your-own source + the
    on-screen keyboard. The 30 Hz message-thread Timer is declared here but its
    body is filled in Phase 3.2 (the interactive waveform editor, filter curve,
    amp-ADSR, and output scope).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit OSimpleSamplerAudioProcessorEditor (OSimpleSamplerAudioProcessor&);
    ~OSimpleSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleSamplerAudioProcessor& processorRef;

    // Message-thread FFT + scope downsampler (audio thread is copy-only into the
    // VizRing). Consumed by the Timer in Phase 3.2 (output scope / spectrum).
    SamplerVizAnalyzer vizAnalyzer;

    // Reserved: held alive across a UI-initiated async dialog. The working file
    // picker lives on the PROCESSOR (loadSourceFromFileChooser owns its own
    // chooser); this mirrors the sibling editor layout.
    std::unique_ptr<juce::FileChooser> fileChooser;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload (an attachment would
    // outlive the WebView and call into a freed component).
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS — 17 sliders + 3 combos + 1 toggle
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSamplerAudioProcessorEditor)
};
