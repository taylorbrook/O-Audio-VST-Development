/*
  ==============================================================================

    O-ReverseDelay — Plugin Editor

    Stage 3 (GUI): Ouaricon Naturalist WebView UI. Four framed group panels in
    signal-flow order (TIME | GRAIN | FEEDBACK | OUTPUT) binding all 10 APVTS
    parameters two-way through Web*Relay / Web*ParameterAttachment.

    Stage 4 (Polish) grows the window to 940 × 484 for a preset bar and adds the
    OuariconPresetManager v1.0.5 bridge.

    No visualization, no Timer, no C++→JS polling bridge, no drag-drop, no
    withInitialisationData (D10). The native-function surface is exactly ELEVEN:
      - getParameterDefaults  (dblclick-reset)
      - 10 preset fns         (the contract js/preset-manager.js fetches)
    Keep that count in sync with app.js + preset-manager.js — an unregistered fn
    is a silently dead control that passes build, auval AND pluginval
    (pattern_webview_native_fn_bridge_gap).

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

    // Preset save/load dialogs. fileDialogOpen guards re-entry: a second click
    // while a chooser is up must complete immediately with {false, ""} rather
    // than replace the live FileChooser out from under its own callback.
    std::shared_ptr<juce::FileChooser> fileChooser;
    bool fileDialogOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseDelayEditor)
};
