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

    O-Emulator — ConsoleEngine (Stage 2, Tasks 6/9/11 — SNES + PS1 + reverb)

    The shared processing skeleton (ARCHITECTURE "One shared engine
    skeleton"), run inside the fixed-chunk walk:

        AA -> downsample -> drive/clip -> codec round-trip
           -> [reverb send tap] -> upsample -> [+ reverb return]
           -> output stage (LP + clip) -> DC blocker

    Phase 2.2 state: SNES (BRR) and PS1 (SPU-ADPCM) pipelines are both
    pre-allocated and prepared; the SPU reverb is wired in every mode.
    NES/GB/Genesis + the 30 ms crossfader land in Phase 2.3 — until then a
    console change is an interim HARD switch at a chunk boundary (incoming
    pipeline reset from silence), and console indices 2-4 map to SNES.

    ── Reverb routing (ARCHITECTURE Processing Order 4/5 + plan L119) ────────
    The send tap is POST-CODEC in the CONSOLE domain, linear-resampled into
    the fixed 22050 Hz reverb domain; the return is linear-resampled to host
    rate through its own PRIMED alignment ring so the round trip joins the
    direct path time-aligned BEFORE the pre-output-stage sum — you cannot
    blend first and delay after when structural delays differ.

    ── The reverb-inactive gate (digest anchor preservation, L110) ───────────
    While `reverb` has NEVER been > 0 (sticky, evaluated at chunk
    boundaries), the send/tick/return code does not run and the return sum is
    skipped entirely. This is an EXACT-0.0 rail: renders with reverb at its
    default 0 are bit-identical to Phase 2.1 (a `x += 0.0f` sum would flip
    -0.0 samples to +0.0 and silently move the recorded anchor). Once active,
    it stays active so the tail keeps rendering after the send closes.

    Control updates happen ONCE PER FIXED CHUNK (chunk-rate SmoothedValues) —
    trajectories are pure functions of the absolute chunk index, block-size
    invariant by construction (O-Octagon GainStage control-grid model).

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "BrrCodec.h"
#include "ConsoleResampler.h"
#include "CrushCurve.h"
#include "FixedChunkFeeder.h"
#include "OutputStage.h"
#include "SpuAdpcmCodec.h"
#include "SpuReverb.h"

#include <cmath>

namespace oemu
{

/** Per-console pipeline configuration (ARCHITECTURE Pipeline Manager table).
    Rows 0 (SNES) and 1 (PS1) are wired; rows 2-4 are data awaiting Phase 2.3
    (their codecBlockLen is 1 — streaming quantizers, no block buffering). */
struct ConsoleSpec
{
    double rate;                  // fixed console-domain rate, Hz
    float outputLpHz;             // output-stage DAC LP corner at host rate
    int codecBlockLen;            // codec structural delay, console samples
    OutputStage::ClipMode clip;   // per-console clip character
};

//==============================================================================
/** Console-domain -> 22050 Hz linear decimator feeding the reverb (the SPU
    reverb input was already crude — linear is the authentic choice,
    ARCHITECTURE "SPU Reverb" notes). Push-driven: each console sample may
    emit 0..n reverb ticks through the templated callback (no std::function
    on the audio thread). `rel` is a bounded running countdown in input-sample
    units — deterministic per console sample, block-size invariant. */
struct ReverbSend
{
    void prepare (double consoleRate)
    {
        step = consoleRate / SpuReverb::kRate;   // input samples per reverb tick
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
            const float frac = (float) (rel + 1.0);   // position in (0, 1] between prev and cur
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
/** 22050 Hz -> host-rate linear upsampler with a PRIMED alignment ring: the
    priming count (computed by the pipeline) places the reverb return
    time-aligned with the direct path at the sum point (L119). Pull-driven:
    exactly one host sample per pull(); the phase is a running double, never
    reset per block. */
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
            return;   // defensive: drop rather than overwrite unread output

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
                jassertfalse;   // priming makes this unreachable
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
    /** The single latency figure — plan decision #2, EXACT formula:

            aaGroupDelayBudget (16 host samples, fixed)
          + kFeederChunk (32)
          + ceil(28 · hostRate / 22050)     worst-case codec block (PS1)
          + ceil( 3 · hostRate / 22050)     Gaussian history, worst console rate

        Constant across console modes (worst case by construction) so mode
        switching never renegotiates PDC. ~116 samples @ 48 kHz, 319 @ 192 kHz
        — under kMaxWetLatencySamples = 1024 with margin. Each pipeline aligns
        its own (shorter) structural delay onto this figure via its
        upsample-ring priming. */
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

    /** Crush target, percent 0–100, latched once per processBlock; consumed
        (smoothed) at fixed-chunk boundaries. Audio thread only. */
    void setCrushPercent (float pct) noexcept { crushTargetPct = pct; }

    /** Reverb send target, percent 0–100, latched once per processBlock. */
    void setReverbSendPercent (float pct) noexcept { reverbTargetPct = pct; }

    /** Console choice (0 SNES, 1 PS1; 2-4 map to SNES until Phase 2.3),
        latched once per block, applied at the next chunk boundary. */
    void setConsoleIndex (int index) noexcept
    {
        pendingPipeline = (index == 1) ? 1 : 0;
    }

    /** In-place wet processing of channels 0/1. */
    void process (juce::AudioBuffer<float>& buffer) noexcept;

#if OUARICON_RENDER_HARNESS
    int getChunkPhaseForTest() const noexcept { return feeder.getChunkPhaseForTest(); }
    std::uint64_t getAbsoluteSamplesForTest() const noexcept { return feeder.getAbsoluteSamplesForTest(); }
    int getUpsampleFillForTest() const noexcept { return pipelines[activePipeline].resampler.getUpFillForTest(); }
    int getActivePipelineForTest() const noexcept { return activePipeline; }
    bool getReverbEverActiveForTest() const noexcept { return reverbEverActive; }
#endif

private:
    //==========================================================================
    /** One complete console pipeline — resampler, codecs, output stage,
        reverb send/return glue — pre-allocated per console (ARCHITECTURE:
        console switch never allocates; this is also the unit Phase 2.3's
        crossfader renders two of). */
    struct Pipeline
    {
        void prepare (const ConsoleSpec& spec, int consoleIdx,
                      double hostRate, int reportedLatencySamples,
                      float initialDriveGain);

        /** RT-safe (no allocation): fixed-array memsets, primed rings, state
            zeroes on already-sized filters. Used by the interim hard switch. */
        void reset (float initialDriveGain) noexcept;

        void processChunk (float* inL, float* inR, float* outL, float* outR,
                           float driveTargetGain, int shiftFloor,
                           float sendGain, bool reverbActive,
                           SpuReverb& reverb) noexcept;

        int consoleIndex = 0;
        ConsoleResampler resampler;
        BrrCodec brr[2];
        SpuAdpcmCodec spu[2];
        OutputStage outputStage;
        ReverbSend send;
        ReverbReturn ret;

        /** Chunk-rate drive smoother (~20 ms), one step per fixed chunk. */
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveGain;

        float consoleBuf[2][128] {};
        static constexpr int kConsoleCap = 128;
    };

    void processChunk (float* inL, float* inR, float* outL, float* outR) noexcept;

    float currentDriveTargetGain (int pipelineIndex) const noexcept;

    // ── Latency-alignment estimates (documented ±few samples; the harness
    //    xcorr probes budget ±15 on top — L120) ──────────────────────────────
    /** Butterworth-8 low-frequency group delay: GD(0) = (Σ 2·cos θ_i)/ωc with
        θ_i the pole angles (11.25°, 33.75°, 56.25°, 78.75°) → Σ = 5.126.
        Host-sample GD = 5.126 · hostRate / (2π · cutoffHz). Gives the same
        prime (33) as Phase 2.1's fixed 3.0 estimate at 44.1/48 kHz for SNES —
        the anchored rates — while tracking PS1's lower cutoff correctly. */
    static constexpr double kButterworthGdSum = 5.126;

    static constexpr double kDownHoldbackEstHost    = 5.0;  // FIFO residual (−3 margin) + Lagrange base latency
    static constexpr double kGaussHistoryEstConsole = 2.0;  // interp point sits ~2 samples behind newest

    static constexpr ConsoleSpec kConsoleSpecs[5] = {
        { 32000.0, 10000.0f, BrrCodec::kBlockLen,      OutputStage::ClipMode::soft },  // SNES — ACTIVE
        { 22050.0, 12000.0f, SpuAdpcmCodec::kBlockLen, OutputStage::ClipMode::hard },  // PS1  — ACTIVE (2.2)
        { 33144.0, 14000.0f, 1,                        OutputStage::ClipMode::hard },  // NES      — Phase 2.3
        { 16384.0,  8000.0f, 1,                        OutputStage::ClipMode::hard },  // Game Boy — Phase 2.3
        { 26320.0, 12000.0f, 1,                        OutputStage::ClipMode::hard },  // Genesis  — Phase 2.3
    };

    FixedChunkFeeder feeder;
    Pipeline pipelines[2];        // [0] SNES, [1] PS1
    int activePipeline = 0;
    int pendingPipeline = 0;

    SpuReverb reverb;

    /** Chunk-rate send-level smoother (~20 ms). Advancing it never touches
        audio while the reverb-inactive gate holds. */
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sendGainSmoothed;

    /** Sticky activation gate — see the header comment. */
    bool reverbEverActive = false;

    float crushTargetPct = 50.0f;    // parameter-spec defaults
    float reverbTargetPct = 0.0f;
};

} // namespace oemu
