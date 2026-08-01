/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain - Plugin Editor

    Stage 3 (GUI): single-page Ouaricon "Granular Field Guide" WebView UI.
    Binds all 19 APVTS parameters two-way via Web*Relay / Web*ParameterAttachment
    (15 sliders + 2 combo boxes + 2 toggles), serves the embedded field-guide page
    through a bare-path resource provider, and registers the source drag-drop /
    file-picker native functions (decode is C++-side on the processor).

    Phase 3.1 wires the layout + controls + load-your-own source. The 30 Hz
    message-thread Timer is declared here but its body is filled in Phase 3.2
    (the four live visualizations + grain/overlap/CPU readout).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleGrainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit OSimpleGrainAudioProcessorEditor (OSimpleGrainAudioProcessor&);
    ~OSimpleGrainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleGrainAudioProcessor& processorRef;

    // Message-thread FFT + scope downsampler (audio thread is copy-only into the
    // VizRing). Consumed by the Timer in Phase 3.2.
    GrainVizAnalyzer vizAnalyzer;

    // Source-version snapshot for the IN-08 "sourceChanged" event: the timer
    // compares against the processor's counter and emits to the page on change,
    // so the JS thumbnail/status refresh is event-driven, not timer-raced.
    juce::uint32 lastSourceVersion = 0;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload (an attachment would
    // outlive the WebView and call into a freed component).
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS — 15 sliders + 2 combos + 2 toggles
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleGrainAudioProcessorEditor)
};
