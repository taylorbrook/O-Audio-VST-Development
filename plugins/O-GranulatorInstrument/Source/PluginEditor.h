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

    O-GranulatorInstrument - Plugin Editor (Stage 3 / Phase 3.1, PLAN-3.1 PD13)

    The v1 page (Source/ui/public/index.html) in a WebView at a fixed
    1200 x 800. Adapted from .planning/mockups/v1-PluginEditor.h; the deltas
    against that template are listed at the top of PluginEditor.cpp.

    261 APVTS parameters bound two-way through 176 WebSliderRelay + 70
    WebComboBoxRelay + 15 WebToggleButtonRelay. With 261 relays the members are
    VECTORS, filled in the ID order of the k*Ids tables in PluginEditor.cpp.

    Native-function surface: exactly 34, each registered by its literal name in
    PluginEditor.cpp (keep it in sync with the page: index.html +
    preset-manager.js + webview-drop-streaming.js):
      getParameterDefaults, getUiLanguage, setUiLanguage,
      getTelemetry, getWindowCurve,
      getScaleState, setDegreeMask, setCustomScale, parseScaleText, loadScala, saveScala,
      getSlotState, getSlotOverview, loadSlot, relinkSlot, clearSlot, resetSlot,
      resetAll, setSlotEmbed, saveRecording            (slot ACTIONS, never parameters),
      dropSessionStart / AddFile / CommitFolder / CommitFile (webview-drop-streaming),
      10 preset fns (preset-manager.js contract).
    Record ARM is the rec_arm PARAMETER, not a native function.
    An unregistered fn is a silently dead control that passes build, auval and
    pluginval; a fn that never completes hangs its promise (the page has no
    timeout), so every fn completes on every path.

    No timer and no pushed events: live data is a PULL (the page calls
    getTelemetry at about 30 Hz). The scale, slot and per-lane overview
    revisions ride in that payload, so a preset load, session restore, file
    load or reset makes the page re-pull.

    The editor reads and acts only through the processor's message-thread API
    and GiUiJson.h; it never reaches the tape pool, the voices or the tapes.

    Include this header from PluginProcessor.cpp only inside #if JUCE_WEB_BROWSER
    (the consoles build the processor with JUCE_WEB_BROWSER=0).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <optional>
#include <vector>
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

    // loadSlot / relinkSlot share one chooser. `slot0` is 0...3. Completes on
    // every path except a dead editor (bare return).
    void launchSlotChooser (int slot0, bool relink, juce::WebBrowserComponent::NativeFunctionCompletion complete);

    GranulatorInstrumentProcessor& processorRef;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Order: Relays -> drop sessions -> WebView -> Attachments
    // Members are destroyed in REVERSE order: attachments (which call
    // evaluateJavascript while they die) go first, while the WebView is alive;
    // the relays and the drop-session manager the WebView's options reference
    // go last.
    // ========================================================================

    // 1. RELAYS FIRST (no dependencies): one per parameter, 261 in total
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;   // 176, kSliderIds order
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;    //  70, kComboIds order
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;   //  15, kToggleIds order

    // Drag-drop session manager (webview-drop-streaming): its native functions
    // capture it, so it is constructed BEFORE the WebView and dies after it.
    std::unique_ptr<Ouaricon::WebViewDropStreaming::SessionManager> dropSessions;

    // 2. WEBVIEW SECOND (depends on relays). Null on Windows when WebView2
    //    cannot start: every use outside the constructor is null-guarded.
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on relays and webView)
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
