/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include <OuariconPresetManager.h>

#include "dsp/CaptureRing.h"
#include "dsp/RngBank.h"
#include "dsp/MediaClock.h"
#include "dsp/ReadHead.h"
#include "dsp/TapeTransport.h"
#include "dsp/TapeDropout.h"
#include "dsp/TapeStopGain.h"
#include "dsp/WowFlutter.h"
#include "dsp/ArtifactSynth.h"
#include "dsp/CDSkip.h"
#include "dsp/VinylTransport.h"
#include "dsp/Arbitration.h"
#include "dsp/PacketLossStage.h"
#include "dsp/CrushStage.h"
#include "dsp/QuantStage.h"
#include "dsp/CodecStage.h"

class OBitrotAudioProcessor : public juce::AudioProcessor
{
public:
    OBitrotAudioProcessor();
    ~OBitrotAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Bitrot"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public APVTS member for direct UI access (standard pattern for JUCE plugins)
    juce::AudioProcessorValueTreeState apvts;

    // Preset manager (Stage 4) — declared AFTER apvts (member init order).
    // LITERAL plugin name: dev and release variants must share one preset
    // library (~/Library/Ouaricon Bitrot/Presets/).
    OuariconPresetManager presetManager { apvts, "Ouaricon Bitrot" };

    // UI activity telemetry (event LEDs). Bits 0-3: tape, cd, vinyl, packet.
    // OR-accumulated per sample inside processBlock (short events survive any
    // block size), published once per block; editor timer reads at 30 Hz.
    enum ActivityBits { kTapeActive = 1, kCdActive = 2,
                        kVinylActive = 4, kPacketActive = 8 };
    std::atomic<uint32_t> uiActivityMask { 0 };

private:
    // ------------------------------------------------------------------------
    // Cached parameter pointers (atomic, real-time safe).
    // Unused in the Stage 1 passthrough processBlock; wired now so Stage 2
    // only adds DSP.
    // ------------------------------------------------------------------------

    // Global (6)
    std::atomic<float>* clockModeParam = nullptr;
    std::atomic<float>* clockSyncDivParam = nullptr;
    std::atomic<float>* clockFreeRateParam = nullptr;
    std::atomic<float>* seedParam = nullptr;
    std::atomic<float>* hardEdgesParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    // Tape (6 — TAPE_DROP and TAPE_WOW appended to the LAYOUT in v1.4.0; see
    // createParameterLayout for why they are not inserted into the tape block)
    std::atomic<float>* tapeEnableParam = nullptr;
    std::atomic<float>* tapeProbParam = nullptr;
    std::atomic<float>* tapeStopProbParam = nullptr;
    std::atomic<float>* tapeRampParam = nullptr;
    std::atomic<float>* tapeDropParam = nullptr;
    std::atomic<float>* tapeWowParam = nullptr;

    // CD Skip (4)
    std::atomic<float>* cdEnableParam = nullptr;
    std::atomic<float>* cdProbParam = nullptr;
    std::atomic<float>* cdSeverityParam = nullptr;
    std::atomic<float>* cdSegmentParam = nullptr;

    // Vinyl (4)
    std::atomic<float>* vinylEnableParam = nullptr;
    std::atomic<float>* vinylProbParam = nullptr;
    std::atomic<float>* vinylRpmParam = nullptr;
    std::atomic<float>* vinylPopParam = nullptr;

    // Packet Loss (4)
    std::atomic<float>* packetEnableParam = nullptr;
    std::atomic<float>* packetLossParam = nullptr;
    std::atomic<float>* packetBurstParam = nullptr;
    std::atomic<float>* packetConcealParam = nullptr;

    // Codec (3)
    std::atomic<float>* codecEnableParam = nullptr;
    std::atomic<float>* codecModeParam = nullptr;
    std::atomic<float>* codecMixParam = nullptr;

    // Crush (6)
    std::atomic<float>* crushEnableParam = nullptr;
    std::atomic<float>* crushBitsParam = nullptr;
    std::atomic<float>* crushRateParam = nullptr;
    std::atomic<float>* crushJitterParam = nullptr;
    std::atomic<float>* crushEnvAmtParam = nullptr;
    std::atomic<float>* crushDitherParam = nullptr;

    // ------------------------------------------------------------------------
    // Stage 2 DSP engine (Phase 2.1: engine core + tape).
    //
    // Latency scheme: CodecStage owns ALL latency — a plain integer delay of
    // kCompLatency = ceil(0.020 * fs) samples until the real codec lands in
    // Phase 2.5. Reported once via setLatencySamples() in prepareToPlay()
    // (JUCE 8: the getter is non-virtual — always report via the setter).
    // ------------------------------------------------------------------------
    static constexpr int kMaxWetLatencySamples = 8192;  // covers 0.020*fs up to 400 kHz

    CaptureRing    captureRing;
    ReadHead       readHead;
    MediaClock     mediaClock;
    RngBank        rngBank;
    TapeTransport  tapeTransport;
    TapeDropout    tapeDropout;      // v1.4.0: oxide-shed level+HF dip
    TapeStopGain   tapeStopGain;     // v1.4.0: output dies with tape speed
    WowFlutter     wowFlutter;       // v1.4.0: continuous speed-modulation bed
    CDSkip         cdSkip;           // Phase 2.2
    VinylTransport vinylTransport;   // Phase 2.2
    ArtifactSynth  artifactSynth;    // Phase 2.2: pops / ticks / chirps
    Arbitration     arbitration;
    PacketLossStage packetStage;     // Phase 2.3: GE loss over its own 20 ms grid
    CodecStage      codecStage;
    CrushStage      crushStage;      // Phase 2.4: fractional-hold SRR + jitter
    QuantStage      quantStage;      // Phase 2.4: fractional bits + TPDF + env depth

    juce::dsp::DryWetMixer<float> dryWetMixer { kMaxWetLatencySamples };

    int    compLatencySamples = 0;
    int    lastSeed           = 0;    // per-block seed-change detection
    double lastAppliedRate    = 1.0;  // tape ramps start from the applied rate

    // Post-event recovery threshold (v1.2.1): a tape release landing back on
    // NORMAL with more lag than this takes ONE intentional crossfaded jump to
    // live rather than leaving the +2% trim to claw it back over ~50x as long.
    // 250 ms of lag already costs the trim ~12.5 s of stale playback.
    static constexpr double kStopRecoverySeconds = 0.25;
    double stopRecoveryLagSamples = kStopRecoverySeconds * 48000.0;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBitrotAudioProcessor)
};
