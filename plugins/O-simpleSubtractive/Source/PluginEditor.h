/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive - Plugin Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI): single-page Ouaricon-Naturalist WebView UI. Binds all 20
    APVTS parameters two-way (16 sliders + 4 combos) via Web*Relay /
    Web*ParameterAttachment, and pushes the live filter-curve (256 dB bins),
    spectrum (256), oscilloscope (128), and dual-envelope levels to the page on
    the 30 Hz message-thread Timer (SubVizAnalyzer runs here, off the audio
    thread — PERF-01).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class OSimpleSubtractiveAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit OSimpleSubtractiveAudioProcessorEditor (OSimpleSubtractiveAudioProcessor&);
    ~OSimpleSubtractiveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleSubtractiveAudioProcessor& processorRef;

    // Message-thread FFT + scope + closed-form filter curve (off the audio thread).
    SubVizAnalyzer vizAnalyzer;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS (16 sliders + 4 combos; NO toggles)
    std::vector<std::unique_ptr<juce::WebSliderRelay>>   sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSubtractiveAudioProcessorEditor)
};
