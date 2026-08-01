/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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

    O-simplePhysicalModelSynth - Plugin Editor

    Stage 3 (GUI) — Phase 3.1: single-page Ouaricon-Naturalist WebView UI with a
    left→right signal-flow layout (Excitation → Resonator → Material → Amp/Output).
    Binds all 17 APVTS parameters two-way via Web*Relay / Web*ParameterAttachment
    (3 combos + 14 sliders, 0 toggles), wires the resonator-/exciter-aware grey-out,
    the preset-bar shell (10 native fns) and the on-screen keyboard (uiMidi).

    Phase 3.2 adds the live spectrum + oscilloscope: a 30 Hz message-thread Timer
    runs PmVizAnalyzer (FFT off the audio thread) and emits spectrum/scope frames to
    the WebView canvases. The SVG loop/flow diagram + modal stems arrive in 3.3.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PmVizAnalyzer.h"

class OSimplePhysicalModelSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                      private juce::Timer
{
public:
    explicit OSimplePhysicalModelSynthAudioProcessorEditor (OSimplePhysicalModelSynthAudioProcessor&);
    ~OSimplePhysicalModelSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // 30 Hz message-thread tick: runs the analyzer and pushes spectrum + scope.
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimplePhysicalModelSynthAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS
    std::vector<std::unique_ptr<juce::WebSliderRelay>>   sliderRelays;   // 14 sliders
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;    // 3 choice params

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;

    // Message-thread spectrum/scope analyzer (Phase 3.2). Declared AFTER the
    // attachments and BEFORE fileChooser — it has no dependency on the relays or the
    // WebView, so it does not affect the load-bearing relay→WebView→attachment order.
    // The Timer is stopped FIRST in the destructor so this never runs mid-teardown.
    PmVizAnalyzer vizAnalyzer;

    // Preset save/load native file dialogs (held while the async chooser is open).
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimplePhysicalModelSynthAudioProcessorEditor)
};
