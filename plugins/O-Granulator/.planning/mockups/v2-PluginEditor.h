/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

    O-Granulator — Plugin Editor (v2 mockup, ui-finalization TEMPLATE)

    gui-agent reference for Stage 3, not a drop-in: adapt class names and the
    processor accessors (listed in v2-integration-checklist.md section 2) to the
    Stage 1/2 processor. Generated from v2-ui.yaml / parameter-spec.md.

    220 APVTS parameters bound two-way through 140 WebSliderRelay + 59
    WebComboBoxRelay + 21 WebToggleButtonRelay. With 220 relays the members are
    VECTORS (O-ReverseDelay's shape), filled in the ID order of the k*Ids tables
    in PluginEditor.cpp. modules/core/webview-relay-manager is NOT used: it has
    no WebComboBoxRelay support (59 of the 220 are choices) and no Windows
    WebView2 handling.

    Native-function surface — exactly 21; keep it in sync with the page
    (grep getNativeFunction in index.html + preset-manager.js):
      getParameterDefaults, getUiLanguage, setUiLanguage,
      getTelemetry, getWindowCurve,
      getScaleState, setDegreeMask, setCustomScale, parseScaleText, loadScala, saveScala,
      10 preset fns (preset-manager.js contract).
    An unregistered fn is a silently dead control that passes build, auval and
    pluginval (pattern_webview_native_fn_bridge_gap).

    No juce::Timer: live data is a PULL (getTelemetry at ~30 Hz from the page),
    as O-ReverseDelay's meter. The scale-state revision rides in that payload,
    so a preset load or session restore re-pulls getScaleState
    (pattern_webview_one_shot_state_push_stale_on_preset_load).

    Include this header from PluginProcessor.cpp only inside #if JUCE_WEB_BROWSER
    (the offline render harness builds the processor with JUCE_WEB_BROWSER=0).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GranulatorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GranulatorEditor (GranulatorProcessor&);
    ~GranulatorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    juce::var buildScaleState() const;     // { revision, tables[32], custom{}, masks[4][32] }
    juce::var buildTelemetry();            // see getTelemetry in the .cpp

    GranulatorProcessor& processorRef;

    // ========================================================================
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members are destroyed in REVERSE order: attachments (which call
    // evaluateJavascript while they die) go first, while the WebView is alive;
    // the relays the WebView's options reference go last.
    // ========================================================================

    // 1️⃣ RELAYS FIRST (no dependencies) — one per parameter, 220 in total
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;   // 140, kSliderIds order
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;    //  59, kComboIds order
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;   //  21, kToggleIds order

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

    // Native file choosers (presets, .scl). One dialog at a time.
    std::shared_ptr<juce::FileChooser> fileChooser;
    bool fileDialogOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorEditor)
};
