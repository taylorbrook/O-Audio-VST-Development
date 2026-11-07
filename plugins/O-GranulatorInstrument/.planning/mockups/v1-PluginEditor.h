/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

    O-GranulatorInstrument — Plugin Editor (v1 mockup, ui-finalization TEMPLATE)

    gui-agent reference for Stage 3, not a drop-in: adapt class names and the
    processor accessors (listed in v1-integration-checklist.md section 2) to the
    Stage 1/2 processor. Generated from v1-ui.yaml / parameter-spec.md (LOCKED).

    261 APVTS parameters bound two-way through 176 WebSliderRelay + 70
    WebComboBoxRelay + 15 WebToggleButtonRelay. With 261 relays the members are
    VECTORS (O-Granulator v2 / O-ReverseDelay shape), filled in the ID order of
    the k*Ids tables in PluginEditor.cpp. modules/core/webview-relay-manager is
    NOT used: no WebComboBoxRelay support (70 of the 261 are choices) and no
    Windows WebView2 handling.

    Native-function surface — exactly 34; keep it in sync with the page
    (grep getNativeFunction in index.html + preset-manager.js +
    webview-drop-streaming.js):
      getParameterDefaults, getUiLanguage, setUiLanguage,
      getTelemetry, getWindowCurve,
      getScaleState, setDegreeMask, setCustomScale, parseScaleText, loadScala, saveScala,
      getSlotState, getSlotOverview, loadSlot, relinkSlot, clearSlot, resetSlot,
      resetAll, setSlotEmbed, saveRecording            (slot ACTIONS — never parameters),
      dropSessionStart/AddFile/CommitFolder/CommitFile (webview-drop-streaming 1.0.0),
      10 preset fns (preset-manager.js contract).
    Record ARM is the rec_arm PARAMETER, not a native function; the run state
    (Idle / Armed / Recording / Committing / Done / Discarded) rides in getTelemetry.
    An unregistered fn is a silently dead control that passes build, auval and
    pluginval (pattern_webview_native_fn_bridge_gap).

    No juce::Timer: live data is a PULL (getTelemetry at ~30 Hz from the page).
    The scale-state, slot-state and per-lane overview revisions ride in that
    payload, so a preset load, session restore, file load or Record-in commit
    makes the page re-pull (pattern_webview_one_shot_state_push_stale_on_preset_load).

    SIDECHAIN: the processor declares ONE optional stereo "Sidechain" input
    (VST3 kAux via getPluginHasMainInput() == false, active by default) for
    Record-in. The editor never assumes it exists: getTelemetry reports
    rec.bus = 0 (no bus / disabled / 0 ch) → the page shows "No sidechain" and
    the Record-in cluster goes inert; rec.signal = false on a bus that delivers
    silence → "No signal" (ARCHITECTURE D6, critical_au_hands_unrouted_sidechain_silence).

    Include this header from PluginProcessor.cpp only inside #if JUCE_WEB_BROWSER
    (an offline render harness builds the processor with JUCE_WEB_BROWSER=0).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WebViewDropStreaming.h"   // modules/core/webview-drop-streaming (ouaricon_add_module)

class GranulatorInstrumentEditor : public juce::AudioProcessorEditor
{
public:
    explicit GranulatorInstrumentEditor (GranulatorInstrumentProcessor&);
    ~GranulatorInstrumentEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    juce::var buildScaleState() const;     // { revision, tables[32], custom{}, masks[4][32] }
    juce::var buildSlotState() const;      // { revision, slots[4] }
    juce::var buildSlotOverview (int slot) const;   // { rev, peaks[656], dirty[656] }
    juce::var buildTelemetry();            // see getTelemetry in the .cpp

    // Slot file dialogs (load / relink / save recording) share one chooser.
    void launchSlotChooser (int slot, bool relink, std::function<void (juce::var)> complete);

    GranulatorInstrumentProcessor& processorRef;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → drop sessions → WebView → Attachments
    // Members are destroyed in REVERSE order: attachments (which call
    // evaluateJavascript while they die) go first, while the WebView is alive;
    // the relays and the drop-session manager the WebView's options reference
    // go last.
    // ========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies) — one per parameter, 261 in total
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;   // 176, kSliderIds order
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;    //  70, kComboIds order
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;   //  15, kToggleIds order

    // Drag-drop session manager (webview-drop-streaming): its native functions
    // capture it, so it is constructed BEFORE the WebView and dies after it.
    std::unique_ptr<Ouaricon::WebViewDropStreaming::SessionManager> dropSessions;

    // 2️⃣ WEBVIEW SECOND (depends on relays). Null on Windows when WebView2
    //    cannot start — every use outside the constructor is null-guarded.
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays and webView)
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    // Windows only: shown instead of the WebView when WebView2 cannot start
    // (webView stays null). Null on macOS and whenever the WebView exists.
    std::unique_ptr<juce::HyperlinkButton> webView2RuntimeLink;

    // Native file choosers (slots, presets, .scl). One dialog at a time.
    std::shared_ptr<juce::FileChooser> fileChooser;
    bool fileDialogOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorInstrumentEditor)
};
