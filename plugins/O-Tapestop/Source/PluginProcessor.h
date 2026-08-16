/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>

#include <atomic>

#include "dsp/CaptureBuffer.h"
#include "dsp/VarispeedVoice.h"
#include "dsp/TapestopTransport.h"

// O-Tapestop — varispeed tapestop/tapestart + drawable-envelope scratch mode.
//
// Stage 2 / Phase 2.2: 26 s capture ring + 2-voice varispeed pool + Stop-mode
// transport with Signalsmith resync (fall-behind → 1.25× catchup → 50 ms
// crossfade-skip, bitwise post-resync null) + tempo sync (edge-latched
// divisions, O-Polystutter BPM fallbacks). The Bypassed state is a TRUE hard
// pass-through — out = in with NO arithmetic, bitwise dry regardless of
// MIX/OUTPUT_GAIN (Stage-0 decision #6; the DSP-03 null probes depend on
// it — and the 0 dB "unity" trim is NOT exactly 1.0f after the APVTS
// normalized round-trip, so it must never ride the bypass path). MIX and
// OUTPUT_GAIN act on the engaged chain only; the trim releases to unity
// across the resync fade (transport engaged-trim blend), so a non-default
// trim cannot step at the Bypassed handoff. Scratch + toneTrack land in
// Phase 2.3.
//
// NOTE: this file (and PluginProcessor.cpp) must stay free of editor-only
// includes — the Stage-2 render harness compiles the processor with
// JUCE_WEB_BROWSER=0 and no editor sources.
class TapestopProcessor : public juce::AudioProcessor
{
public:
    TapestopProcessor();
    ~TapestopProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // ── Ring sizing (research/ARCHITECTURE.md "Ring Sizing", BINDING) ────────
    // Worst-case debt growth is Scratch full-reverse: d(debt)/dt = 1 − r with
    // r = −2 ⇒ 3 s of debt per second of pass. Over the longest gesture
    // (STOP/START/ENV_FREE_MS max = kMaxGestureSeconds = 8 s) that is 24 s of
    // debt; + 2 s margin = 26 s. The Stop-mode bound is smaller (spin-down 8 s
    // ≤ 8 s debt; a full stop+start cycle ≤ ~16 s). Pre-history reads return
    // the cleared ring's zeros, so the debt clamp's worst case (playing the
    // oldest valid material) is safe even in the first 26 s after load.
    // Memory cost: 26 s stereo float = 9.2 MB @ 44.1 kHz, 40 MB @ 192 kHz.
    static constexpr double kMaxGestureSeconds = 8.0;   // freeMsRange() max derives from this
    static constexpr double kCaptureSeconds    = (1.0 - (-2.0)) * kMaxGestureSeconds + 2.0;

    static_assert(kCaptureSeconds >= 3.0 * kMaxGestureSeconds + 2.0,
                  "capture ring must cover worst-case scratch full-reverse debt "
                  "(3 s of debt per second over the longest gesture) plus margin");

#if OUARICON_RENDER_HARNESS
    // Harness-only debt inspection (probe P4, Phase 2.3; API stable from 2.1).
    // Debt d = live head − CARRIER voice position, in samples.
    double getDebtSamplesForTest() const noexcept
    {
        return (double) (capture.getTotalWritten() - 1)
             - voices[transport.getCarrierIndex()].readAbsFrac;
    }

    // Harness-only skip-splice law selector (Task 8/11 A/B — both laws
    // compiled; the DAW build ships the default set in TapestopTransport).
    void setSpliceLawForTest (bool linear) noexcept
    {
        transport.setSpliceLaw (linear ? TapestopTransport::SpliceLaw::Linear
                                       : TapestopTransport::SpliceLaw::EqualPower);
    }
#endif

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Gesture duration in samples, latched at the ENGAGE edge (Task 10).
    // SYNC_MODE gates which of the SYNC_DIV/FREE_MS pair is read; the BPM was
    // read into currentBpm at the block header (O-Polystutter fallback+clamp).
    double gestureDurationSamples (bool isStopGesture) const noexcept;

    // Cached raw parameter atomics — resolved once in the constructor
    // (getRawParameterValue), read per block on the audio thread
    // (research/ARCHITECTURE.md line 348, suite convention). Unused in the
    // Stage-1 pass-through shell; Stage 2 wires them.
    std::atomic<float>* pEngage       = nullptr;
    std::atomic<float>* pMode         = nullptr;
    std::atomic<float>* pSyncMode     = nullptr;
    std::atomic<float>* pStopSyncDiv  = nullptr;
    std::atomic<float>* pStopFreeMs   = nullptr;
    std::atomic<float>* pStopCurve    = nullptr;
    std::atomic<float>* pStartSyncDiv = nullptr;
    std::atomic<float>* pStartFreeMs  = nullptr;
    std::atomic<float>* pStartCurve   = nullptr;
    std::atomic<float>* pEnvSyncDiv   = nullptr;
    std::atomic<float>* pEnvFreeMs    = nullptr;
    std::atomic<float>* pToneTrack    = nullptr;
    std::atomic<float>* pMix          = nullptr;
    std::atomic<float>* pOutputGain   = nullptr;

    // ── Stage-2 DSP state ───────────────────────────────────────────────────
    // The ring is written every sample in EVERY state (including Bypassed) so
    // an engage gesture always has history. Fixed 2-voice pool: the transport
    // drives the CARRIER; the other slot exists only during skip crossfades
    // (resync, catchup-retrigger).
    CaptureBuffer     capture;
    VarispeedVoice    voices[2];
    TapestopTransport transport;

    juce::SmoothedValue<float> mixSmoothed;    // MIX/100, 20 ms
    juce::SmoothedValue<float> gainSmoothed;   // OUTPUT_GAIN dB → linear, 20 ms

    double currentFs  = 48000.0;
    double currentBpm = 120.0;   // per-block host read; consumed at edges only
    bool   lastEngage = false;   // block-header ENGAGE edge detector

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapestopProcessor)
};
