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

    O-simpleWavetable - Plugin Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Single-page "Wavetable Field Guide" WebView UI (1120 x 780, fixed).
    Adapted from mockups/v1-PluginEditor.h (Stage 3, PLAN Task 3).

    Binds all 21 APVTS parameters two-way:
      13 WebSliderRelay        position, lfo_rate, lfo_depth, menv_attack,
                               menv_decay, menv_sustain, menv_release,
                               env_amount, amp_attack, amp_decay, amp_sustain,
                               amp_release, output_level
       6 WebComboBoxRelay      bank, bit_depth, lfo_sync, lfo_div, lfo_shape,
                               voice_mode
       2 WebToggleButtonRelay  interp, bandlimit
    and on the 30 Hz message-thread Timer pushes, in this order (P10, D-T):
      bankUpdate    N x 128 level-0 thumbnails; on a bank-generation OR bank
                    index change (P2), content-hash gated, forced by uiReady
      importStatus  idle | busy | done | error, on every importer transition
      presetState   { name, id, factory, modified } (Stage 4, D-AG); on a
                    preset revision change or a modified flip, forced by uiReady
      cycleUpdate   the heard cycle, its harmonics 1..32 and the lead-voice
                    readouts; hash-gated over the quantized payload (D-P), so
                    an idle editor sends nothing
    Payload building lives in VizPayload.h (shared with viz-check).
    20 natives (Stage 4 adds the 10 preset-manager.js resolves, plus
    getPresetCatalog and stepKnobDrag).

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

#include <memory>
#include <optional>

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

    // Resource provider - serves the embedded UI files (bare-path matching).
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // C++ -> page push. Safe to call at any time on the message thread;
    // emitEventIfBrowserIsVisible drops it while the browser is hidden, which
    // is why the page calls the `uiReady` native to ask again.
    void emitImportStatus();
    void emitBankUpdate (bool force);    // skipped when ! force and the content hash is unchanged
    void emitCycleUpdate (bool force);   // skipped when ! force and the payload hash is unchanged
    void emitPresetState (bool force);   // skipped when ! force and neither the revision nor modified changed

    OSimpleWavetableAudioProcessor& processorRef;

    // Viz buffers, reused every tick: building and hashing allocate nothing
    // (bankThumbs.points is reserved for 256 x 128 in the ctor).
    OSimpleWavetableAudioProcessor::CycleView  cycleView;
    OSimpleWavetableAudioProcessor::BankThumbs bankThumbs;

    // Change detection for the timer (message thread only).
    juce::uint32 lastImportVersion  = 0;
    juce::uint32 lastBankGeneration = 0;
    int          lastBankIndex      = -1;
    juce::uint64 lastBankHash       = 0;
    juce::uint64 lastCycleHash      = 0;
    bool         forceCycleEmit     = true;
    juce::uint32 lastPresetRevision = 0;           // D-AG
    bool         lastPresetModified = false;

    // N1 (D-AP): ONE import dialog at a time. The chooser must outlive its
    // async dialog, so it is a member; it is only ever replaced on the NEXT
    // launch (never reset inside its own callback, which would destroy the
    // running std::function). The flag, not the pointer, is the in-flight
    // test (O-AnalogEQ IN-08).
    std::unique_ptr<juce::FileChooser> importChooser;
    bool importDialogInFlight = false;

    // =====================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Order: Relays -> WebView -> Attachments
    // Members are destroyed in REVERSE order:
    //   attachments first (WebView still alive), WebView second, relays last.
    // Wrong order = release-build crash / DAW freeze on plugin reload.
    // =====================================================================

    // 1) RELAYS FIRST (no dependencies) - 21, one per parameter
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

    // 2) WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3) ATTACHMENTS LAST (depend on relays AND webView) - 21, same order
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
