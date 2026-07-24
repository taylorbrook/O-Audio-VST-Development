/*
  ==============================================================================

    O-ReverseDelay — Plugin Editor

    Stage 3 (GUI): Ouaricon Naturalist WebView UI, fixed 940 × 440. Four framed
    group panels in signal-flow order (TIME | GRAIN | FEEDBACK | OUTPUT) binding
    all 10 APVTS parameters two-way through Web*Relay / Web*ParameterAttachment.

    This is the simplest WebView editor in the suite by design (D10): no
    visualization, no Timer, no C++→JS polling bridge, no drag-drop, no
    FileChooser, no withInitialisationData. Exactly ONE native function
    (getParameterDefaults) backs dblclick-reset.

    NOTE: this header is included by PluginProcessor.cpp only from inside an
    #if JUCE_WEB_BROWSER guard — the Stage-2 render harness builds the processor
    with JUCE_WEB_BROWSER=0 and no editor sources.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ReverseDelayEditor : public juce::AudioProcessorEditor
{
public:
    explicit ReverseDelayEditor (ReverseDelayProcessor&);
    ~ReverseDelayEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Serves the embedded UI files. The callback receives a BARE PATH.
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    ReverseDelayProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView still alive)
    // Wrong order = release-build crash on plugin reload, because an attachment
    // would outlive the WebView and call into a freed component.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS — 8 sliders + 2 combos.
    //    syncMode AND noteDivision are both AudioParameterChoice, so BOTH are
    //    combo relays; WebToggleButtonRelay is for bool params only and is
    //    deliberately unused here.
    std::vector<std::unique_ptr<juce::WebSliderRelay>>   sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseDelayEditor)
};
