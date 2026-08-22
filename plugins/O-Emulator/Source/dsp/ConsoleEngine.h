/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator — ConsoleEngine (Stage 2 complete: Tasks 6/9/11/13/14/16/17/18)

    The shared processing skeleton (ARCHITECTURE "One shared engine
    skeleton"), run inside the fixed-chunk walk:

        AA (crush-openable) -> downsample (age-drifted) -> drive/clip
           -> codec round-trip -> [reverb send] -> upsample -> [+ return]
           -> output stage (LP × age dulling + clip) -> DC blocker
           -> [micro-fade dip on integer-step changes]
        ... crossfader ... -> AGE BED (noise + hum, engine-level) -> mixer

    ── Age model (Tasks 16/17) ───────────────────────────────────────────────
    - Bed (AgeModel): injected ONCE at engine level after the pipeline/
      crossfader output — wet path only, rides on top (never dulled by the
      output LP, never doubled during a fade). DEVIATION from ARCHITECTURE's
      literal ordering (bed before the DC blocker): the bed is zero-mean by
      construction, and engine-level injection is what makes "one RNG stream
      per purpose, consumed unconditionally" tractable with five pipelines.
    - Dulling (v1.0.1): output LP corner × 2^(−2·age/100) — linear in
      OCTAVES over age (×1.0 -> ×0.25), per chunk, gated on the controlling
      value (OutputStage::setLpCutoffHz). The v1.0.0 map (×(1 − 0.55·age/100))
      was near-inaudible below age 60.
    - Drift (v1.0.1 redesign): the wobble is generated in the OFFSET domain,
      not the rate domain. A ~1.2 Hz-decorrelated random walk (own RNG
      stream, advanced unconditionally per chunk, σ normalized at prepare so
      depth is rate-invariant) is tanh-bounded into a time-offset TARGET of
      up to 0.85 × the ACTIVE pipeline's rail × age. A 2.5 Hz first-order
      servo tracks it; the per-chunk offset delta IS the read-rate deviation
      (capped at ±15 cents), so the offset is bounded BY CONSTRUCTION — no
      rail bounce, no clamp discontinuities. Audible result: ~4-12 cent
      warble at age 100. The v1.0.0 rate-domain walk needed ~±220 host
      samples of storage for its ±15-cent/0.3 Hz spec against rails of 9-64,
      so it railed within ~100 ms and clipped the wobble to <1 cent —
      inaudible (the v1.0.1 root cause). Factor is EXACTLY 1.0 at age 0
      (target 0, offset 0, delta 0), so drift-free renders keep the
      bit-nominal path. computeLatencySamples() now carries a +24-host-sample
      (48 kHz-scaled) drift-headroom term so every pipeline's priming — PS1's
      shallow one in particular — deepens toward the reported figure and the
      rails grow with it; alignment stays exact because priming targets the
      reported latency.
    - Program envelope (v1.0.1): per-chunk peak of the wet pre-bed output,
      one-pole follower at the chunk rate (5 ms attack; 150 ms hiss release,
      400 ms hum release, all rate-compensated), NaN-guarded
      (pattern_envelope_follower_state_sticky_nan). scale =
      min(1, max(0, 4·env − 0.004)) multiplies the bed gains — full bed at
      peaks >= −12 dBFS, proportional below, hard ZERO under −60 dBFS peaks
      (the gate offset lets the release land on exact silence).

    ── Crush integer steps (Task 18) ─────────────────────────────────────────
    Shift floor / NES rate index / GB levels / AA-open set are INTEGER steps.
    Each pipeline runs a 5 ms equal-gain micro-fade (linear V-dip) on any
    step-set change: fade down over half, apply ALL pending steps at the
    trough (where the gain is ~0), fade back up. Requests arriving mid-fade
    are latched until the trough. Genesis's update rate and the drive gain
    are continuous and stay per-chunk smooth outside the fade machinery.

    ── Console switching (Task 14, unchanged) ────────────────────────────────
    30 ms equal-power crossfade; first-chunk instant switch; reverb persists
    across switches; drift offset resets with the rings on a switch.

    Control updates happen ONCE PER FIXED CHUNK — block-size invariant by
    construction (O-Octagon control-grid model).

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "AgeModel.h"
#include "BrrCodec.h"
#include "ConsoleCrossfader.h"
#include "ConsoleResampler.h"
#include "CrushCurve.h"
#include "DpcmCodec.h"
#include "FixedChunkFeeder.h"
#include "GenesisDac.h"
#include "OutputStage.h"
#include "SpuAdpcmCodec.h"
#include "SpuReverb.h"
#include "WaveQuantizer.h"

#include <cmath>
#include <cstdint>

namespace oemu
{

/** Per-console pipeline configuration (ARCHITECTURE Pipeline Manager table). */
struct ConsoleSpec
{
    double rate;                             // fixed console-domain rate, Hz
    float outputLpHz;                        // output-stage DAC LP corner
    int codecBlockLen;                       // structural codec delay
    OutputStage::ClipMode clip;              // per-console clip character
    ConsoleResampler::UpsampleMode upsample; // Gaussian vs zero-order hold
};

//==============================================================================
/** Console-domain -> 22050 Hz linear decimator feeding the reverb. Push-
    driven, templated tick callback (no std::function on the audio thread).
    GB's 16384 Hz domain makes step < 1, so the while loop (multiple ticks
    per input sample) is load-bearing. */
struct ReverbSend
{
    void prepare (double consoleRate)
    {
        step = consoleRate / SpuReverb::kRate;
        reset();
    }

    void reset() noexcept
    {
        rel = step;
        prevL = prevR = curL = curR = 0.0f;
    }

    template <typename TickFn>
    void push (float l, float r, TickFn&& tick) noexcept
    {
        prevL = curL;  prevR = curR;
        curL = l;      curR = r;

        rel -= 1.0;
        while (rel <= 0.0)
        {
            const float frac = (float) (rel + 1.0);
            tick (prevL + frac * (curL - prevL),
                  prevR + frac * (curR - prevR));
            rel += step;
        }
    }

    double step = 1.0;
    double rel = 1.0;
    float prevL = 0.0f, prevR = 0.0f, curL = 0.0f, curR = 0.0f;
};

//==============================================================================
/** 22050 Hz -> host-rate linear upsampler with a PRIMED alignment ring (the
    reverb return joins the direct path time-aligned — L119). */
struct ReverbReturn
{
    static constexpr int kCap = 256;
    static_assert ((kCap & (kCap - 1)) == 0, "ring walk uses & (kCap - 1)");

    void prepare (double hostRate, int primeCount) noexcept
    {
        inc = SpuReverb::kRate / hostRate;
        prime = juce::jlimit (0, kCap - 16, primeCount);
        reset();
    }

    void reset() noexcept
    {
        for (auto& ch : ring)
            for (auto& v : ch)
                v = 0.0f;

        w = rd = fill = 0;
        phase = 0.0;
        prevL = prevR = curL = curR = 0.0f;

        for (int i = 0; i < prime; ++i)
            push (0.0f, 0.0f);
    }

    void push (float l, float r) noexcept
    {
        jassert (fill < kCap);
        if (fill >= kCap)
            return;

        ring[0][w] = l;
        ring[1][w] = r;
        w = (w + 1) & (kCap - 1);
        ++fill;
    }

    void pull (float& l, float& r) noexcept
    {
        phase += inc;
        while (phase >= 1.0)
        {
            phase -= 1.0;
            prevL = curL;
            prevR = curR;

            if (fill > 0)
            {
                curL = ring[0][rd];
                curR = ring[1][rd];
                rd = (rd + 1) & (kCap - 1);
                --fill;
            }
            else
            {
                jassertfalse;
                curL = curR = 0.0f;
            }
        }

        l = prevL + (float) phase * (curL - prevL);
        r = prevR + (float) phase * (curR - prevR);
    }

    float ring[2][kCap] {};
    int w = 0, rd = 0, fill = 0;
    float prevL = 0.0f, prevR = 0.0f, curL = 0.0f, curR = 0.0f;
    double phase = 0.0;
    double inc = 0.459375;
    int prime = 0;
};

//==============================================================================
class ConsoleEngine
{
public:
    /** Plan decision #2, EXACT worst-case formula (constant across modes;
        drift wobble deliberately NOT reflected here). v1.0.1 adds a constant
        drift-headroom term (24 host samples at 48 kHz, rate-scaled): the
        priming targets this reported figure, so the extra headroom deepens
        every pipeline's upsample priming — growing the drift rails (PS1's
        above all) while keeping wet/dry alignment exact. */
    static int computeLatencySamples (double hostRate) noexcept
    {
        constexpr int aaGroupDelayBudget = 16;

        const int codecWorst = (int) std::ceil (28.0 * hostRate / 22050.0);
        const int gaussHist  = (int) std::ceil (3.0 * hostRate / 22050.0);
        const int driftHeadroom = (int) std::ceil (24.0 * hostRate / 48000.0);

        return aaGroupDelayBudget + FixedChunkFeeder::getLatencySamples()
             + codecWorst + gaussHist + driftHeadroom;
    }

    void prepare (double hostRate, int maxBlockSize, int reportedLatencySamples);

    void reset();

    void setCrushPercent (float pct) noexcept { crushTargetPct = pct; }
    void setAgePercent (float pct) noexcept { ageTargetPct = pct; }
    void setReverbSendPercent (float pct) noexcept { reverbTargetPct = pct; }

    void setConsoleIndex (int index) noexcept
    {
        pendingPipeline = juce::jlimit (0, 4, index);
    }

    /** In-place wet processing of channels 0/1. */
    void process (juce::AudioBuffer<float>& buffer) noexcept;

#if OUARICON_RENDER_HARNESS
    int getChunkPhaseForTest() const noexcept { return feeder.getChunkPhaseForTest(); }
    std::uint64_t getAbsoluteSamplesForTest() const noexcept { return feeder.getAbsoluteSamplesForTest(); }
    int getUpsampleFillForTest() const noexcept { return pipelines[activePipeline].resampler.getUpFillForTest(); }
    int getActivePipelineForTest() const noexcept { return activePipeline; }
    bool getReverbEverActiveForTest() const noexcept { return reverbEverActive; }
    bool getIsFadingForTest() const noexcept { return crossfader.isFading(); }
    float getOutputLpHzForTest (int console) const noexcept
    {
        return pipelines[juce::jlimit (0, 4, console)].outputStage.getLpCutoffForTest();
    }
#endif

private:
    //==========================================================================
    /** The integer control steps that need micro-fading (Task 18). */
    struct StepSet
    {
        int codecStep = 0;   // shift floor / NES rate index / GB levels (per console)
        int aaIndex = 0;     // crush AA-open coefficient set

        bool operator== (const StepSet& o) const noexcept
        {
            return codecStep == o.codecStep && aaIndex == o.aaIndex;
        }
        bool operator!= (const StepSet& o) const noexcept { return ! (*this == o); }
    };

    //==========================================================================
    struct Pipeline
    {
        void prepare (const ConsoleSpec& spec, int consoleIdx,
                      double hostRate, int reportedLatencySamples,
                      float crushPct);

        /** RT-safe (no allocation). Applies the current crush-derived step
            set immediately (no fade — used at prepare and fade starts). */
        void reset (float crushPct) noexcept;

        void processChunk (float* inL, float* inR, float* outL, float* outR,
                           float crushPct, float agePct, double driftFactor,
                           float sendGain, bool reverbActive,
                           SpuReverb& reverb) noexcept;

        StepSet computeSteps (float crushPct) const noexcept;
        void applySteps (const StepSet& s) noexcept;

        int consoleIndex = 0;
        float baseLpHz = 10000.0f;

        /** This pipeline's drift time-offset rail, HOST samples: what its
            upsample-ring priming can actually absorb ((prime − 4) in host
            units, clamped [2, 64]). Pitch wobble IS time storage — the
            v1.0.1 offset-servo drift keeps its target inside 0.85 × this
            rail by construction, so deeper priming = deeper wobble. The
            +24-sample latency headroom (computeLatencySamples) lifts PS1's
            rail from ~9 to ~33 host samples; the others sit at or near the
            64 cap. */
        double maxDriftOffsetHost = 8.0;

        ConsoleResampler resampler;
        BrrCodec brr[2];
        SpuAdpcmCodec spu[2];
        DpcmCodec dpcm[2];
        WaveQuantizer gbq[2];
        GenesisDac gen[2];
        OutputStage outputStage;
        ReverbSend send;
        ReverbReturn ret;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveGain;

        // ── Micro-fade state (Task 18) ──────────────────────────────────────
        StepSet applied, pending;
        int fadePos = -1;        // -1 idle; else chunk index within the fade
        int fadeChunks = 8;      // 5 ms total at prepare's host rate (even)
        int fadeHalfChunks = 4;

        static constexpr int kConsoleCap = 128;
        float consoleBuf[2][kConsoleCap] {};
    };

    void processChunk (float* inL, float* inR, float* outL, float* outR) noexcept;

    // ── Latency-alignment estimates (xcorr probes budget ±15 on top) ────────
    static constexpr double kButterworthGdSum = 5.126;
    static constexpr double kDownHoldbackEstHost    = 5.0;
    static constexpr double kGaussHistoryEstConsole = 2.0;
    static constexpr double kZohHistoryEstConsole   = 0.5;

    /** Minimum drift headroom every pipeline's priming must provide, HOST
        samples (the priming FLOOR); the per-pipeline rail actually used is
        Pipeline::maxDriftOffsetHost (what the priming really absorbs). */
    static constexpr double kMinDriftOffsetHost = 8.0;

    static constexpr ConsoleSpec kConsoleSpecs[5] = {
        { 32000.0, 10000.0f, BrrCodec::kBlockLen,      OutputStage::ClipMode::soft,
          ConsoleResampler::UpsampleMode::gaussian },                                 // SNES
        { 22050.0, 12000.0f, SpuAdpcmCodec::kBlockLen, OutputStage::ClipMode::hard,
          ConsoleResampler::UpsampleMode::gaussian },                                 // PS1
        { 33144.0, 14000.0f, 0,                        OutputStage::ClipMode::hard,
          ConsoleResampler::UpsampleMode::zoh },                                      // NES
        { 16384.0,  8000.0f, 0,                        OutputStage::ClipMode::crunchy,
          ConsoleResampler::UpsampleMode::zoh },                                      // Game Boy
        { 26320.0, 12000.0f, 0,                        OutputStage::ClipMode::hard,
          ConsoleResampler::UpsampleMode::zoh },                                      // Genesis
    };

    FixedChunkFeeder feeder;
    Pipeline pipelines[5];
    int activePipeline = 0;
    int pendingPipeline = 0;

    ConsoleCrossfader crossfader;
    float fadeIn[2][FixedChunkFeeder::kChunk] {};
    float fadeOut[2][FixedChunkFeeder::kChunk] {};
    bool anyChunkProcessed = false;

    SpuReverb reverb;
    AgeModel ageModel;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sendGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ageSmoothed;

    // ── Drift state (Task 17, v1.0.1 offset-servo redesign): own RNG
    //    stream, advanced unconditionally per chunk from this phase's
    //    introduction ─────────────────────────────────────────────────────────
    std::uint32_t driftRng = 0x1234ABCDu;
    double driftWalk = 0.0;      // AR(1) walk, ~1.2 Hz decorrelation, [−1, 1]
    double driftSlow = 0.0;      // 2.5 Hz-smoothed walk -> tanh-bounded target
    double driftWalkCoeff = 0.995;    // AR pole (prepare: exp(−2π·1.2/chunkRate))
    double driftWalkGain = 0.07;      // AR input gain (prepare: σ ≈ 0.4)
    double driftSmoothCoeff = 0.0105; // walk -> slow one-pole (2.5 Hz)
    double driftServoCoeff = 0.0105;  // offset servo one-pole (2.5 Hz)
    double driftOffsetHost = 0.0;

    /** ±15-cent cap on the per-chunk read-rate deviation (fractional). */
    static constexpr double kMaxDriftFrac = 0.0087;

    // ── Program envelope for the age bed (v1.0.1): chunk-rate one-pole
    //    followers over the wet pre-bed chunk peak ────────────────────────────
    float hissEnv = 0.0f;
    float humEnv = 0.0f;
    float envAtkCoeff = 0.35f;        // prepare: 5 ms at the chunk rate
    float envRelHissCoeff = 0.0044f;  // prepare: 150 ms
    float envRelHumCoeff = 0.0017f;   // prepare: 400 ms

    bool reverbEverActive = false;

    float crushTargetPct = 50.0f;    // parameter-spec defaults
    float ageTargetPct = 20.0f;
    float reverbTargetPct = 0.0f;
};

} // namespace oemu
