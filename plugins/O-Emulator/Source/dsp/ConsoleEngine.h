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
    - Dulling: output LP corner × (1.0 -> 0.45) over age, per chunk, gated on
      the controlling value (OutputStage::setLpCutoffHz).
    - Drift: bounded random-walk (own RNG stream, advanced unconditionally
      per chunk) of ±15 cents × age on the ACTIVE pipeline's decimation read
      rate. The accumulated time offset is clamped to the pipeline's OWN rail
      (what its upsample-ring priming absorbs — a BUFFER-SAFETY bound; note
      pattern_clamped_float_ramp_rails_are_rate_dependent: the offset clamp
      is in host samples by design, the ±15-cent rate rail is dimensionless
      and exact at every rate). Latency is NOT re-reported on drift wobble
      (nominal figure stands, wobble << 1 ms).

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
        drift wobble deliberately NOT reflected here). */
    static int computeLatencySamples (double hostRate) noexcept
    {
        constexpr int aaGroupDelayBudget = 16;

        const int codecWorst = (int) std::ceil (28.0 * hostRate / 22050.0);
        const int gaussHist  = (int) std::ceil (3.0 * hostRate / 22050.0);

        return aaGroupDelayBudget + FixedChunkFeeder::getLatencySamples()
             + codecWorst + gaussHist;
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
            units, clamped [2, 64]). Pitch wobble IS time storage — a slow
            ±15-cent walk needs tens of samples of excursion, so consoles
            with deep priming (SNES/NES/GB/GEN, 40-65 host samples) wobble
            slowly at full depth while PS1 (shallow priming to stay on the
            reported latency) rails sooner and flutters faster. */
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

    // ── Drift state (Task 17): own RNG stream, advanced unconditionally per
    //    chunk from this phase's introduction ────────────────────────────────
    std::uint32_t driftRng = 0x1234ABCDu;
    double driftWalk = 0.0;      // raw bounded random walk, [−1, 1]
    double driftSlow = 0.0;      // ~0.3 Hz smoothed walk (the audible wobble)
    double driftSmoothCoeff = 0.00126;
    double driftOffsetHost = 0.0;

    bool reverbEverActive = false;

    float crushTargetPct = 50.0f;    // parameter-spec defaults
    float ageTargetPct = 20.0f;
    float reverbTargetPct = 0.0f;
};

} // namespace oemu
