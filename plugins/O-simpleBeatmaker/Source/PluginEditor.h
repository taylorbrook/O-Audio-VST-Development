/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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

    O-simpleBeatmaker - Plugin Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI): single-page Ouaricon-Naturalist WebView UI. Binds all 42 APVTS
    params two-way (29 slider relays + 1 combo relay + 12 toggle relays), drives
    the custom 6x32 step grid through native functions, and on a 60 Hz message-
    thread Timer drains the lock-free VizAnalyzer into one "frame" event (playhead
    phase + the hits emitted this frame + transport state) that feeds the grid
    flash, the applied-Δt timing lane, and the live MIDI readout from one stream.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class OSimpleBeatmakerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit OSimpleBeatmakerAudioProcessorEditor (OSimpleBeatmakerAudioProcessor&);
    ~OSimpleBeatmakerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves the embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    OSimpleBeatmakerAudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: member declaration order (C++ destroys in REVERSE)
    //   1. Relays      — declared first  → destroyed last  (safe)
    //   2. WebView     — declared second → destroyed second
    //   3. Attachments — declared last   → destroyed first (WebView alive)
    // Wrong order = release-build crash on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1. RELAYS
    std::vector<std::unique_ptr<juce::WebSliderRelay>>       sliderRelays;
    std::vector<std::unique_ptr<juce::WebComboBoxRelay>>     comboRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> toggleRelays;

    // 2. WEBVIEW
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>>       sliderAttachments;
    std::vector<std::unique_ptr<juce::WebComboBoxParameterAttachment>>     comboAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> toggleAttachments;

    // The param-ID lists (built once in the ctor, reused for relays + attachments).
    juce::StringArray sliderIds, comboIds, toggleIds;

    // Scratch buffer the Timer drains VizAnalyzer into (no per-frame allocation).
    std::array<OSimpleBeatmaker::VizEvent, 256> vizScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleBeatmakerAudioProcessorEditor)
};
