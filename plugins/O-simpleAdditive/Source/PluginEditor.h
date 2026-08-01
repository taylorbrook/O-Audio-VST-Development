/*
   This file is part of O-simpleAdditive, an Ouaricon Audio plugin.
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

    O-simpleAdditive - Plugin Editor

    Stage 3 (GUI): single-page Ouaricon "Additive Field Guide" WebView UI (sibling
    of O-simpleFM). Binds all 33 APVTS parameters two-way via Web*Relay /
    Web*ParameterAttachment (31 sliders + 2 combo boxes), and on the 30 Hz
    message-thread Timer pushes:
      - the oscilloscope (128 pts, from AdditiveVizAnalyzer over the VizRing), and
      - the live drawbar spectrum (16 morphed+decayed amplitudes from the lock-free
        active-spectrum snapshot, plus a "sounding" flag).
    The 16 drawbars double as the live spectrum display (the headline lesson).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleAdditiveAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit OSimpleAdditiveAudioProcessorEditor (OSimpleAdditiveAudioProcessor&);
    ~OSimpleAdditiveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleAdditiveAudioProcessor& processorRef;

    // Message-thread FFT + scope downsampler (audio thread is copy-only into VizRing).
    AdditiveVizAnalyzer vizAnalyzer;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS
    std::vector<std::unique_ptr<juce::WebSliderRelay>>   sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>> comboRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAdditiveAudioProcessorEditor)
};
