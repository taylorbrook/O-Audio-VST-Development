/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
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

    O-simpleFM - Plugin Editor

    Stage 3 (GUI): single-page Ouaricon-Naturalist WebView UI. Binds all 17
    APVTS parameters two-way via Web*Relay / Web*ParameterAttachment, and pushes
    the live spectrum (256 dB bins) + oscilloscope (128 pts) to the page on the
    30 Hz message-thread Timer (FmVizAnalyzer runs here, off the audio thread).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleFMAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit OSimpleFMAudioProcessorEditor (OSimpleFMAudioProcessor&);
    ~OSimpleFMAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleFMAudioProcessor& processorRef;

    // Message-thread FFT + scope (already exercised in Stage 2).
    FmVizAnalyzer vizAnalyzer;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS
    std::vector<std::unique_ptr<juce::WebSliderRelay>>        sliderRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>  toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    // Preset save/load native file dialogs (held while the async chooser is open).
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Last sample rate pushed to the page (0 = not yet pushed); the timer emits
    // `sampleRateUpdate` on change so the JS spectrum axis tracks the live rate.
    double lastPushedSampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleFMAudioProcessorEditor)
};
