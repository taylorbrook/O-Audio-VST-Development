/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop — Plugin Editor

    Stage 3 (GUI): Ouaricon Naturalist WebView UI, fixed 860 × 580 (mirrored by
    styles.css html/body and .frame — keep in sync, never resize).

    Binds all 19 APVTS parameters two-way: 11 WebSliderRelay knobs + 7
    WebComboBoxRelay controls (MODE / SYNC_MODE / CHARACTER as segment
    groups, four SYNC_DIV divisions as selects) + 1 WebToggleButtonRelay
    (ENGAGE — the large latching performance control, UI-02).

    The native-function surface is exactly THIRTEEN:
      - getParameterDefaults  (dblclick-reset, engineering units)
      - commitEnvelope        (envelope commit; completes SYNCHRONOUSLY with
                               the C++-sanitized echo)
      - requestEnvelope       (page init: current envelope JSON)
      - the 10 preset fns modules/preset-manager.js requests (Stage 4):
        savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
        getPresetList, getCurrentPreset, selectNextPreset,
        selectPreviousPreset, deletePreset, isFactoryPreset
    Keep that count in sync with js/app.js + modules/preset-manager.js — an
    unregistered fn is a silently dead control that passes build, auval AND
    pluginval (pattern_webview_native_fn_bridge_gap). Synchronous completions
    capture `this` (never outlive the call — safe); the two DIALOG fns defer
    their completion into a FileChooser callback and therefore use a HOISTED
    SafePointer and plain `return` on a dead editor — even complete(false)
    there would be a UAF
    (pattern_webview_launchasync_safepointer_no_complete).

    Live readback is a PUSH: a 30 Hz juce::Timer reads three relaxed processor
    atomics and emits ONE "transportFrame" event {state, ratio, phase} via
    emitEventIfBrowserIsVisible; the same tick compares uiEnvGeneration and
    emits "envelopeState" (sanitized JSON) on change. No polling native fn for
    live data, so no promise can hang while the view is hidden
    (critical_webview_completion_gated_on_isvisible).

    NOTE: this header is included by PluginProcessor.cpp only from inside an
    #if JUCE_WEB_BROWSER guard — the Stage-2 render harness builds the
    processor with JUCE_WEB_BROWSER=0 and no editor sources
    (pattern_render_harness_breaks_on_webview_editor).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TapestopEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit TapestopEditor (TapestopProcessor&);
    ~TapestopEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Serves the embedded UI files. The callback receives a BARE PATH.
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    TapestopProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload, because an
    // attachment would outlive the WebView and call into a freed component.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS — 11 sliders + 7 combos + 1 toggle = 19, one per APVTS
    //    parameter. Authoritative lists: kSliderIds / kComboIds / kToggleIds
    //    in PluginEditor.cpp. The relay TYPE has to match the parameter type —
    //    a bool bound through a slider relay attaches without error and
    //    produces a control whose state never updates.
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    // Envelope generation last pushed to the page (message thread only — the
    // timer and the commit native fn both run there, so no atomics needed on
    // this side; the processor counter itself is atomic).
    std::uint32_t lastEnvGeneration = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapestopEditor)
};
