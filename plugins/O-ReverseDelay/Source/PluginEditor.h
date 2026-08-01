/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-ReverseDelay — Plugin Editor

    Stage 3 (GUI): Ouaricon Naturalist WebView UI. Framed group panels in
    signal-flow order binding all 25 APVTS parameters two-way through
    Web*Relay / Web*ParameterAttachment.

    Stage 4 (Polish) added the preset bar and the OuariconPresetManager v1.0.5
    bridge. The window is 940 × 768 as of v1.7.1, which is the size that fits a
    1080p display; PluginEditor.cpp:530 is the single setSize call and
    styles.css (html/body and .frame) must agree with it.

    No drag-drop, no withInitialisationData. Decision D10 ("no visualization, no
    Timer, no C++→JS polling bridge") was REVERSED at v1.3.0 by the COUNT
    panel's live grain readout: getGrainMeter is polled from JS at 15 Hz (see
    app.js METER_POLL_MS). It is a PULL, so there is still no juce::Timer in
    this class and no emitEvent plumbing — see PluginEditor.cpp:24-38 for why
    that distinction is what keeps the ui-stub able to render the page.

    The native-function surface is exactly THIRTEEN:
      - getParameterDefaults  (dblclick-reset)
      - getWindowCurve        (the v1.4.0 envelope display)
      - getGrainMeter         (the v1.3.0 COUNT readout, polled)
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

    // 1. RELAYS — 20 sliders + 4 combos + 1 toggle = 25, one per APVTS
    //    parameter. The authoritative lists are kSliderIds / kComboIds /
    //    kToggleIds in PluginEditor.cpp:50-99; ui_frontend_check.js diffs all
    //    three against the APVTS in both directions, which is the only thing
    //    that catches an id landing in the wrong list.
    //    syncMode, noteDivision, grainShape and sourceMode are all
    //    AudioParameterChoice, so all four are combo relays. v1.6.0's `freeze`
    //    is the plugin's first and only AudioParameterBool and therefore its
    //    first WebToggleButtonRelay —
    //    the relay TYPE has to match the parameter type, and a bool bound through
    //    a slider relay attaches without error and produces a control whose state
    //    never updates (pattern_webview_native_fn_bridge_gap, same failure class
    //    reached by a different route).
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    // Preset save/load dialogs. fileDialogOpen guards re-entry: a second click
    // while a chooser is up must complete immediately with {false, ""} rather
    // than replace the live FileChooser out from under its own callback.
    std::shared_ptr<juce::FileChooser> fileChooser;
    bool fileDialogOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseDelayEditor)
};
