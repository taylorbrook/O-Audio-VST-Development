/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Plugin Editor  (TEMPLATE — mockup v1 finalization)

    gui-agent reference for Stage 3, NOT a copy-paste file: adapt names to what
    Stage 1/2 actually built (processor class, accessors, display structs).

    Single-page "Wavetable Field Guide" WebView UI (1120 x 780, fixed — Rule 4).
    Binds all 21 APVTS parameters two-way:
      13 WebSliderRelay        position, lfo_rate, lfo_depth, menv_attack,
                               menv_decay, menv_sustain, menv_release,
                               env_amount, amp_attack, amp_decay, amp_sustain,
                               amp_release, output_level
       6 WebComboBoxRelay      bank, bit_depth, lfo_sync, lfo_div, lfo_shape,
                               voice_mode
       2 WebToggleButtonRelay  interp, bandlimit
    and on the 30 Hz message-thread Timer pushes:
      cycleUpdate   the exact read cycle + harmonics 1-32 (hash-gated)
      bankUpdate    N x 128 level-0 thumbnails, on bank change / import / uiReady
      importStatus  idle | busy | done | error, on every importer transition

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OSimpleWavetableAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit OSimpleWavetableAudioProcessorEditor (OSimpleWavetableAudioProcessor&);
    ~OSimpleWavetableAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Resource provider — serves embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // C++ → page pushes. Each is safe to call at any time on the message thread;
    // emitEventIfBrowserIsVisible drops them while the browser is hidden, which
    // is why the page calls the `uiReady` native to ask again.
    void emitBankUpdate();
    void emitImportStatus();
    void emitCycleUpdate (bool force);

    OSimpleWavetableAudioProcessor& processorRef;

    // Change detection for the timer (message thread only).
    juce::uint32 lastBankGeneration   = 0;
    juce::uint32 lastImportVersion    = 0;
    juce::uint64 lastCycleHash        = 0;
    bool         forceCycleEmit       = true;

    // ═══════════════════════════════════════════════════════════════════
    // ⚠️ CRITICAL MEMBER DECLARATION ORDER ⚠️
    // Order: Relays → WebView → Attachments
    // Members are destroyed in REVERSE order:
    //   attachments first (WebView still alive), WebView second, relays last.
    // Wrong order = release-build crash / DAW freeze on plugin reload.
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ RELAYS FIRST (no dependencies) — 21, one per parameter
    std::unique_ptr<juce::WebComboBoxRelay>     bankRelay;
    std::unique_ptr<juce::WebSliderRelay>       positionRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> interpRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> bandlimitRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     bitDepthRelay;
    std::unique_ptr<juce::WebSliderRelay>       lfoRateRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     lfoSyncRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     lfoDivRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     lfoShapeRelay;
    std::unique_ptr<juce::WebSliderRelay>       lfoDepthRelay;
    std::unique_ptr<juce::WebSliderRelay>       menvAttackRelay;
    std::unique_ptr<juce::WebSliderRelay>       menvDecayRelay;
    std::unique_ptr<juce::WebSliderRelay>       menvSustainRelay;
    std::unique_ptr<juce::WebSliderRelay>       menvReleaseRelay;
    std::unique_ptr<juce::WebSliderRelay>       envAmountRelay;
    std::unique_ptr<juce::WebSliderRelay>       ampAttackRelay;
    std::unique_ptr<juce::WebSliderRelay>       ampDecayRelay;
    std::unique_ptr<juce::WebSliderRelay>       ampSustainRelay;
    std::unique_ptr<juce::WebSliderRelay>       ampReleaseRelay;
    std::unique_ptr<juce::WebComboBoxRelay>     voiceModeRelay;
    std::unique_ptr<juce::WebSliderRelay>       outputLevelRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on relays AND webView) — 21, same order
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     bankAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       positionAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> interpAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bandlimitAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     bitDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       lfoRateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     lfoSyncAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     lfoDivAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     lfoShapeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       lfoDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       menvAttackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       menvDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       menvSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       menvReleaseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       envAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       ampAttackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       ampDecayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       ampSustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       ampReleaseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>     voiceModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       outputLevelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleWavetableAudioProcessorEditor)
};
