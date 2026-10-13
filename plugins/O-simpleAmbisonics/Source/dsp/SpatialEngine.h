/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    SpatialEngine — the Stage-2 DSP composition. Phase 2.1: the audible chain
    minus rotation and HRTF. Phase 2.2: scene rotation in the SH domain.
    Phase 2.3: the real binaural leg (SADIE II MagLS banks, UPOLS).

      mono (conditioned by the processor)
        → DistanceModel   g(d) · x → TPT air LP with the D7 wet mix
        → ShEncoder       × c_eff[16] = Y_acn(az_eff, el_eff) · orderGain[band]
        → ShRotator       bus → tap = R_SH(ψ, θ, φ) · bus, 83 elements lerped
                          per sample between the previous and current control
                          step (W implicit)
        → 16-channel bus  (busTap[16] = the ROTATED bus of the last sample;
                          shPeak scan POST-rotation — D5)
        → BinauralConvolver  on the ROTATED bus: grid-synchronous UPOLS with
                          the per-order MagLS filter bank (256 taps at 48 k,
                          resampled + DC-calibrated elsewhere), dual-bank
                          20 ms order crossfade, queued latest-wins requests
        → StereoDecoder   ±60° cardioid pair on W / Y / X of the rotated bus
        → DecoderMixer    FixedDelay<kPartition> × 2 on the stereo leg (time-
                          aligns it with the convolver's intrinsic 64), 20 ms
                          mix ramp; when both mix endpoints are exactly 1 the
                          convolver output is COPIED (D3), else the A3 blend
      outL / outR         (the processor applies outputLevel per sample)

    THE BINAURAL LEG (Phase 2.3, CONTEXT-2.3 D1 / D3 / D4, PLAN-2.3 C6).
    D1: the convolver is GRID-SYNCHRONOUS — stepped from controlStep() at
    gridPos == 0 on the 16 rotated-bus samples push()ed during the grid that
    just ended; the output it writes is read during the grid that now begins,
    so the latency is exactly kPartition = 64 by construction, with no FIFO
    and no host-sized buffer (maxBlock is the processor's scratch size — the
    convolver is grid-sized). Inside controlStep the order is MANDATORY:
    conv.arrive() → conv.requestSet (params.order) → conv.step (kRampStep)
    (R10: a request sampled on the arrival step must see fading == false),
    then binauralOnly = mixer.bothEndpointsAtOne(), then fillFrameState().
    D3: one bool per grid; renderSample copies the convolver output when it
    is set (Binaural mode bit-exact to the convolver), else blends. D4: the
    16 input FFTs always run, the accumulate loop covers (order + 1)²
    channels; an order change is a dual-bank equal-gain fade with the 2.1
    LinearRamp shape, requests during a fade are queued (latest wins) and
    applied on arrival. The decoder level match is BAKED INTO THE FILTER
    SCALE by the generator (kBinauralLevelMatchDb, +0.53 dB), so the Stereo
    leg's exact literals and digest-2.1 / digest-2.2 are untouched (D2).

    ROTATION (Phase 2.2, CONTEXT-2.2 D1 / D2 / RESEARCH-2.2). The BUS is
    rotated, not the coefficient vector: the same block-diagonal matrix would
    rotate any number of encoded sources, and Phase 2.3's binaural decoder
    consumes the rotated bus — that is the lesson. Per control step the three
    wrap-smoothed angles (τ = 20 ms, shortest path, D4) build R₃ =
    Rz(ψ)·Ry′(θ)·Rx(φ), the order-1 block is its ACN permutation and the
    5 × 5 / 7 × 7 blocks come from the Ivanic–Ruedenberg recursion
    (ShRotator.h). The 83 nonzero elements are lerped per sample with the
    same t as the coefficient vector (A5, A3 form) and all three blocks are
    always applied. The identity is EXACT in float, so at zero rotation the
    tap equals the bus to the bit and the Phase 2.1 render digest
    (9189d911a1fe98d5) is this phase's regression verdict (D3).

    Meter frame: shGain = R_SH · c_eff (post-rotation); azEff / elEff stay
    PRE-rotation (the puck); azRot / elRot = dir (R₃ · d (azEff, elEff)) —
    with an identity FAST PATH when all three smoothed angles are exactly 0
    (azRot = azEff, elRot = elEff). The fast path exists because atan2f does
    not round-trip the encoder's own trig (37° → 36.999992) and the UI wants
    the compass exactly on the puck at zero rotation; the matrix itself is
    still built and applied. The wrap smoothers' snap rule delivers exact 0
    after any motion, so the fast path is reachable again (F2).

    Step response (RESEARCH-2.2 R2): a 180° yaw STEP through the one-pole
    produces grid-boundary kinks (piecewise-linear interpolation of an
    exponential) that read 12 / 25 / 44× on bands 1 / 2 / 3 against a STATIC
    control, but 1.000 against the same-motion reference — identical to the
    2.1 azimuth path's response to a 180° azimuth step. Continuous motion
    (360°/s) reads 1.13. v1.1 candidates if the ear objects: a longer τ for
    jumps only, or quadratic (prev / cur / next) element interpolation.

    THE CONTROL GRID (PLAN-2.1 A1 — the D5 amendment). Every control quantity
    moves ONCE PER 64 SAMPLES on a grid that PERSISTS across process() calls
    (gridPos ∈ [0, 64)). Inside a grid the per-sample path lerps between the
    previous and the current endpoints with t = (j + 1)/64 (A5), every blend
    in the form b + t·(a − b) (A3).

    Why not "one step per process() call" (CONTEXT-2.1 D5 as first written):
    at 4096-sample host blocks a 2 Hz orbit moves 61° per call, and the lerp
    across that chord dips the band-3 coefficient norm by −5.56 dB mid-block
    (band 1: −1.31 dB, band 2: −3.75 dB — RESEARCH-2.1 R3). On the 64-sample
    grid the chord error is ≤ 0.002 dB at the fastest orbit, AND the render
    becomes bit-identical across any host slicing with parameters moving and
    orbit On, because the same operations run in the same order at the same
    absolute sample positions (harness P2b / P3b pin this). A host block
    shorter than 64 may contain no control step — correct: the lerp just
    continues toward the endpoints set at the last step.

    Smoother constants come from the grid, never from the slice length:
    kOnePole = 1 − exp(−64/(τ·fs)), kRampStep = 64/(τ·fs), τ = 20 ms (A1).

    Allocation: NONE in process(). The chain is strictly per-sample feed-
    forward, so the bus for one sample lives in busTap[16]; there is no
    16 × maxBlock scratch. The convolver's tables are std::vectors assigned
    in prepare() only (K and L depend on fs); maxBlock is the processor's
    scratch size (accessor) — the convolver is grid-sized (D1).

    prepare() snaps every smoother / ramp / orbit phase from the Params the
    processor copied in BEFORE calling it (A7), so block 0 is at steady state
    (no fade-in — pattern_gain_ramp_without_seeding_fades_in_from_silence) and
    every value is finite by construction (the engine's own smoothers replace
    juce::SmoothedValue, whose reset() does not clear a NaN).

    Meters (A8): the engine owns peak accumulation across process() calls
    (resetPeaks() / frame()); the processor gates the reset on
    meterFrames.hasNewData() and publishes once per host block.

    Latency: kPartition = 64 in both decoder modes at every sample rate
    (ARCHITECTURE §Latency). setLatencySamples is the processor's ONE call
    site and is never touched by Stage 2.

    Headless: <algorithm>, <cmath>, <cstddef> + the Source/dsp headers. JUCE
    reaches this header only through BinauralConvolver.h (juce_dsp FFT +
    WindowedSinc — the ONE JUCE-including DSP header, CONTEXT-2.3 D6).

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "FixedDelay.h"
#include "Smoothers.h"
#include "ShEncoder.h"
#include "ShRotator.h"
#include "DistanceModel.h"
#include "OrbitLfo.h"
#include "StereoDecoder.h"
#include "DecoderMixer.h"
#include "BinauralConvolver.h"
#include "MeterFrame.h"

namespace OSimpleAmbisonics::dsp
{

// The ONE latency constant. Processor (setLatencySamples), harness (P0/P4),
// and Stage 2's UPOLS partition size all read this. 64 samples, fixed, at
// every sample rate and in both decoder modes.
inline constexpr int kPartition = 64;

// The 2.3 UPOLS step must land on the control grid.
static_assert (kControlGrid == kPartition, "control grid and UPOLS partition must coincide");

// The stereo-decoder path delay that time-aligns it with the binaural UPOLS
// path. Owned by the DecoderMixer from Phase 2.1.
using DecoderDelay = FixedDelay<kPartition>;

struct Params
{
    float azimuthDeg   = 0.0f;   // 0 front, +90 left (AmbiX), ±180 behind
    float elevationDeg = 0.0f;   // +90 overhead
    float distanceM    = 1.0f;   // 0.2 … 20 m
    int   order        = 1;      // 1 / 2 / 3
    float yawDeg       = 0.0f;   // scene rotation, + = CCW from above
    float pitchDeg     = 0.0f;   // + = front rises
    float rollDeg      = 0.0f;   // + = left rises
    int   decoder      = 0;      // 0 Binaural, 1 Stereo
    bool  orbit        = false;
    float orbitRateHz  = 0.1f;
};

class SpatialEngine
{
public:
    SpatialEngine() = default;

    // Call setParams() BEFORE prepare() (A7): prepare snaps everything to the
    // stored Params. Allocates (the convolver's tables for this fs) — the
    // only place in the engine that does.
    void prepare (double sampleRate, int maxBlockSize)
    {
        sampleRateHz = sampleRate;
        maxBlock     = maxBlockSize;

        kOnePole  = (float) (1.0 - std::exp (-(double) kControlGrid / (kSmoothingTauSeconds * sampleRateHz)));
        kRampStep = (float) ((double) kControlGrid / (kSmoothingTauSeconds * sampleRateHz));

        dist.prepare (sampleRateHz);
        mixer.prepare();
        rot.prepare();          // u / v / w tables — before the first build()
        conv.prepare (sampleRateHz);   // tables / spectra / FDL for this fs (C3 / C14) — before snapToParams
        snapToParams();
    }

    // Delays + filter state zeroed, grid restarted, everything re-snapped.
    // No allocation.
    void reset() noexcept
    {
        mixer.reset();
        dist.reset();
        snapToParams();
    }

    // Copied once per host block by the processor; SAMPLED at the next
    // control step.
    void setParams (const Params& p) noexcept { params = p; }

    // The control-step spec (PLAN-2.1 §The control step), op-for-op with the
    // harness ReferenceModel.
    void process (const float* monoIn, float* outL, float* outR, int numSamples) noexcept
    {
        int i = 0;

        while (i < numSamples)
        {
            if (gridPos == 0)
                controlStep();

            const int run = std::min (numSamples - i, kControlGrid - gridPos);

            for (int j = 0; j < run; ++j)
            {
                const float t = (float) (gridPos + j + 1) * kInvGrid;
                renderSample (monoIn[i + j], t, gridPos + j, outL[i + j], outR[i + j]);
            }

            i += run;
            gridPos += run;

            if (gridPos == kControlGrid)
                gridPos = 0;
        }
    }

    static constexpr int latencySamples() noexcept { return kPartition; }

    // Peak accumulation restarts here; the processor calls this at the top of
    // a host block only when the UI has consumed the last published frame.
    void resetPeaks() noexcept
    {
        std::fill (std::begin (meterFrame.shPeak), std::end (meterFrame.shPeak), 0.0f);
    }

    // State fields are refreshed at every control step (and by prepare/reset);
    // shPeak accumulates per sample (post-rotation). shGain is R_SH · c_eff.
    // outPeakL/R are the processor's.
    const MeterFrame& frame() const noexcept { return meterFrame; }

    // The 16-channel bus of the LAST processed sample — POST-rotation (D5),
    // post order gating, pre-decode. Harness tap for the order-fade and the
    // 2.2 rotation click gates; zero extra cost because the peak scan reads
    // it anyway.
    const float* busTap() const noexcept { return tap; }

    int maxBlockSize() const noexcept { return maxBlock; }

    // The convolver, read-only — the harness reads the resampled tables
    // (filterTaps / tapsAtRate / resamplerDcGain) and the bank state
    // (activeSet / isFading / bankMixCur) for its gates.
    const BinauralConvolver& convolver() const noexcept { return conv; }

private:
    static constexpr float kInvGrid = 1.0f / (float) kControlGrid;

    void snapToParams() noexcept
    {
        az.snapTo (params.azimuthDeg);
        el.snapTo (params.elevationDeg);
        dist.snapTo (params.distanceM);

        for (int b = 0; b < 3; ++b)
            orderGain[b].snapTo (b + 1 <= params.order ? 1.0f : 0.0f);

        mixer.snapTo (params.decoder);
        orbit.reset();

        yaw.snapTo (params.yawDeg);
        pitch.snapTo (params.pitchDeg);
        roll.snapTo (params.rollDeg);

        evaluateCoefficients();
        std::copy (std::begin (curC), std::end (curC), std::begin (prevC));

        rot.build (yaw.cur, pitch.cur, roll.cur, curR, curR3);
        std::copy (std::begin (curR), std::end (curR), std::begin (prevR));

        std::fill (std::begin (bus), std::end (bus), 0.0f);
        std::fill (std::begin (tap), std::end (tap), 0.0f);

        // Phase 2.3: grid buffers + FDL zeroed, bank state snapped to the order
        // (no fade pending), the D3 copy flag from the snapped mixer.
        conv.reset();
        conv.snapToSet (params.order);
        binauralOnly = mixer.bothEndpointsAtOne();

        gridPos = 0;
        resetPeaks();
        fillFrameState();
    }

    // One control step: endpoints for the next kControlGrid samples.
    void controlStep() noexcept
    {
        const float azTarget = orbit.targetAzimuthDeg (params.orbit, params.orbitRateHz,
                                                       params.azimuthDeg, sampleRateHz);
        az.step (azTarget, kOnePole);
        el.step (params.elevationDeg, kOnePole);
        dist.controlStep (params.distanceM, kOnePole);

        for (int b = 0; b < 3; ++b)
            orderGain[b].step (b + 1 <= params.order ? 1.0f : 0.0f, kRampStep);

        mixer.controlStep (params.decoder, kRampStep);

        // Phase 2.2: the three rotation angles, same k, same snap rule (D4).
        yaw.step (params.yawDeg, kOnePole);
        pitch.step (params.pitchDeg, kOnePole);
        roll.step (params.rollDeg, kOnePole);

        std::copy (std::begin (curC), std::end (curC), std::begin (prevC));
        evaluateCoefficients();

        std::copy (std::begin (curR), std::end (curR), std::begin (prevR));
        rot.build (yaw.cur, pitch.cur, roll.cur, curR, curR3);

        // Phase 2.3 (C6 / R10): arrive → requestSet → step — in this order.
        // requestSet runs EVERY step (latest wins by construction).
        conv.arrive();
        conv.requestSet (params.order);
        conv.step (kRampStep);
        binauralOnly = mixer.bothEndpointsAtOne();      // D3: one bool per grid

        fillFrameState();
    }

    // curC = Y(az, el) · orderGain[band] — encodeSH16 takes radians.
    void evaluateCoefficients() noexcept
    {
        float y[kNumSh];
        encodeSH16 (az.cur * kDegToRad, el.cur * kDegToRad, y);

        const float og[4] = { 1.0f, orderGain[0].cur, orderGain[1].cur, orderGain[2].cur };
        applyOrderGains (y, og, curC);
    }

    void fillFrameState() noexcept
    {
        // shGain = R_SH · c_eff (W passes through) — post-rotation (D5).
        ShRotator::apply (curR, curC, meterFrame.shGain);

        // The puck: pre-rotation (UI-02).
        meterFrame.azEffDeg = az.cur;
        meterFrame.elEffDeg = el.cur;

        if (yaw.cur == 0.0f && pitch.cur == 0.0f && roll.cur == 0.0f)
        {
            // Identity fast path (B6 / F1): exact, and the compass sits on the puck.
            meterFrame.azRotDeg = az.cur;
            meterFrame.elRotDeg = el.cur;
        }
        else
        {
            // D8: d = (cos e·cos a, cos e·sin a, sin e); d′ = R₃·d; back to angles.
            const float a = az.cur * kDegToRad, e = el.cur * kDegToRad;
            const float ce = std::cos (e);
            const float d[3] = { ce * std::cos (a), ce * std::sin (a), std::sin (e) };

            const float x = curR3[0][0] * d[0] + curR3[0][1] * d[1] + curR3[0][2] * d[2];
            const float y = curR3[1][0] * d[0] + curR3[1][1] * d[1] + curR3[1][2] * d[2];
            const float z = curR3[2][0] * d[0] + curR3[2][1] * d[1] + curR3[2][2] * d[2];

            // clamp is mandatory: |d′| can exceed 1 by an ulp (R4). azRot is
            // undefined within ≈ 0.01° of the pole (MeterFrame.h).
            meterFrame.azRotDeg = wrapTo180 (std::atan2 (y, x) * kRadToDeg);
            meterFrame.elRotDeg = std::asin (std::clamp (z, -1.0f, 1.0f)) * kRadToDeg;
        }

        meterFrame.orderActive    = (params.order + 1) * (params.order + 1);
        meterFrame.decoderMix     = mixer.currentMix();
        meterFrame.latencySamples = kPartition;
    }

    // pos = gridPos + j ∈ [0, 64): the convolver's slot for this sample.
    void renderSample (float x, float t, int pos, float& outL, float& outR) noexcept
    {
        const float y = dist.processSample (x, t);

        // Encode: the pre-rotation bus (lerped coefficient vector, A3 / A5).
        for (int acn = 0; acn < kNumSh; ++acn)
            bus[acn] = y * (prevC[acn] + t * (curC[acn] - prevC[acn]));

        // Rotate (Phase 2.2, B5): tap = lerp (prevR, curR, t) · bus, W implicit.
        ShRotator::applyLerped (prevR, curR, t, bus, tap);

        // Peak scan POST-rotation (D5).
        for (int acn = 0; acn < kNumSh; ++acn)
            meterFrame.shPeak[acn] = std::max (meterFrame.shPeak[acn], std::abs (tap[acn]));

        // Binaural leg (Phase 2.3, D1): this grid's rotated bus goes in (consumed
        // by the NEXT control step); what comes out was convolved at this step
        // from the previous grid — 64 samples of structural latency. During a
        // bank fade outputAt blends the two banks in the A3 form.
        conv.push (tap, pos);
        const float binL = conv.outputAt (0, pos, t);
        const float binR = conv.outputAt (1, pos, t);

        float stL, stR;
        StereoDecoder::decode (tap, stL, stR);

        float dL, dR;
        mixer.delayStereo (stL, stR, dL, dR);      // the stereo leg's delay ALWAYS runs

        if (binauralOnly)
        {
            // D3: both mix endpoints exactly 1 → bit-exact copy of the convolver.
            outL = binL;
            outR = binR;
        }
        else
        {
            mixer.blend (dL, dR, binL, binR, t, outL, outR);
        }
    }

    // ── state ──────────────────────────────────────────────────────────────
    double sampleRateHz = 48000.0;
    int    maxBlock     = 0;
    float  kOnePole     = 0.0f;
    float  kRampStep    = 1.0f;
    int    gridPos      = 0;

    Params params;

    WrapSmoother            az;
    OnePoleSmoother         el;
    WrapSmoother            yaw, pitch, roll;
    DistanceModel           dist;
    OrbitLfo                orbit;
    LinearRamp              orderGain[3];
    DecoderMixer<kPartition> mixer;
    ShRotator               rot;
    BinauralConvolver       conv;
    bool                    binauralOnly = false;   // D3: set once per grid

    float prevC[kNumSh] {};
    float curC[kNumSh] {};
    float prevR[kNumRotElems] {};
    float curR[kNumRotElems] {};
    float curR3[3][3] {};
    float bus[kNumSh] {};     // pre-rotation, local to the sample
    float tap[kNumSh] {};     // post-rotation — busTap()

    MeterFrame meterFrame;
};

} // namespace OSimpleAmbisonics::dsp
