/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

#include <atomic>
#include <cmath>
#include <cstdint>

#include "Vec.h"

//==============================================================================
/** Compile-time instrumentation switch.

    Defined to 1 by the two TEST targets only (PLAN-2.2 P20). The plugin target never defines it, so
    in the shipping binary the counter objects DO NOT EXIST and countedPow() collapses to std::pow at
    the call site.

    Defaulted here rather than relying on `#if UNDEFINED_MACRO` evaluating to 0, so the header is
    clean under -Wundef.
*/
#ifndef OOCTAGON_INSTRUMENT
 #define OOCTAGON_INSTRUMENT 0
#endif

namespace oo
{

//==============================================================================
/**
    Six counters that turn PERF acceptance criteria — and Phase 2.3's bit-transparency claim — from
    prose into numbers.

    The counter OBJECTS exist only under OOCTAGON_INSTRUMENT — in the shipping binary they are not
    merely unread, they are not there. The count*() helpers are always declared so the DSP code needs
    no `#if` at its call sites; they compile to nothing when the flag is off.

    `inline` variables, so every TU in a test target shares one definition without a .cpp. Relaxed
    ordering: they are read after the render finishes and never used to synchronise anything.
*/
namespace instr
{
#if OOCTAGON_INSTRUMENT
    /// PERF-02/3 — std::pow calls. Expected EXACTLY 16 per control block (8 per sub-point, via the
    /// t = pow(d,-a) reuse). The bound of 32 alone would still pass if the reuse were dropped.
    inline std::atomic<std::uint64_t> powCalls { 0 };

    /// PERF-02/1,2 — completed solves. Must NOT increment across blocks with nothing changed.
    inline std::atomic<std::uint64_t> solveRuns { 0 };

    /// PERF-01/3 — hull projections. "Only when outside" becomes a number rather than an argument
    /// about where the call sits.
    inline std::atomic<std::uint64_t> hullProjections { 0 };

    /// PERF-02/4 — inner-loop iterations. `sampleAdvances == totalSamplesRendered` in BOTH modes is
    /// the executable form of "every getNextValue() exactly once per sample, unconditionally".
    /// Without this the criterion is unmeasurable.
    inline std::atomic<std::uint64_t> sampleAdvances { 0 };

    /// PLAN-2.3 P32 / RESEARCH-2.3 Q2 — setCutoffFrequency() calls. THIS ASSERTS PLACEMENT, NOT
    /// COST: `airCutoffUpdates == solveRuns * 2` is the executable form of "the cutoff is set at the
    /// control boundary and never per sample". The number that can silently regress is where the
    /// call sits, and this is what makes that a number rather than an argument.
    inline std::atomic<std::uint64_t> airCutoffUpdates { 0 };

    /// DSP-07/2,/5,/6 — samples that actually went THROUGH a filter. This is what makes P33's
    /// structural bit-transparency proof measurable: 0 when the stage is defeated, and
    /// `samplesRendered * 2` when it is active. A branch counted as never taken is a stronger claim
    /// than a rendered comparison against a "filter absent" build that does not exist.
    inline std::atomic<std::uint64_t> airSamplesFiltered { 0 };

    inline void resetCounters() noexcept
    {
        powCalls.store           (0, std::memory_order_relaxed);
        solveRuns.store          (0, std::memory_order_relaxed);
        hullProjections.store    (0, std::memory_order_relaxed);
        sampleAdvances.store     (0, std::memory_order_relaxed);
        airCutoffUpdates.store   (0, std::memory_order_relaxed);
        airSamplesFiltered.store (0, std::memory_order_relaxed);
    }

    inline std::uint64_t get (const std::atomic<std::uint64_t>& c) noexcept
    {
        return c.load (std::memory_order_relaxed);
    }
#endif

    //==========================================================================
    // Call sites use these unconditionally. Nothing is emitted when the flag is off.

    inline void countSolveRun() noexcept
    {
       #if OOCTAGON_INSTRUMENT
        solveRuns.fetch_add (1, std::memory_order_relaxed);
       #endif
    }

    inline void countHullProjection() noexcept
    {
       #if OOCTAGON_INSTRUMENT
        hullProjections.fetch_add (1, std::memory_order_relaxed);
       #endif
    }

    inline void countSampleAdvance() noexcept
    {
       #if OOCTAGON_INSTRUMENT
        sampleAdvances.fetch_add (1, std::memory_order_relaxed);
       #endif
    }

    inline void countAirCutoffUpdate() noexcept
    {
       #if OOCTAGON_INSTRUMENT
        airCutoffUpdates.fetch_add (1, std::memory_order_relaxed);
       #endif
    }

    inline void countAirSampleFiltered() noexcept
    {
       #if OOCTAGON_INSTRUMENT
        airSamplesFiltered.fetch_add (1, std::memory_order_relaxed);
       #endif
    }
}

//==============================================================================
/**
    Distance-Based Amplitude Panning — ARCHITECTURE §3.3, implemented against the **2011-04-14
    revised** equations.

    THE ORIGINAL PAPER'S EQUATIONS 3-6 AND 9-10 ARE WRONG (`pattern_dbap_not_vbap_for_irregular_arrays`).
    Do not "correct" this against the first edition.

    ── Raw inputs, deliberately (PLAN-2.2 P19) ───────────────────────────────────────────────────
    No VenueModel, no VenueSnapshot, no JUCE. DbapSolver.cpp includes only <cmath>, <atomic> and
    Vec.h. That is what lets the DBAP probes live in the fast unit target against a self-contained
    fixture, with zero coupling to VenueModel's defaults (CONTEXT-2.2 D3) — a stronger form of D3's
    intent than constructing a VenueModel from the fixture would be.
*/
namespace dbap
{
    inline constexpr int kNumSpeakers = 8;

    //==========================================================================
    // §3.3.1 constants. ASSERTED, NOT ASSUMED — an invariant that lives only in prose is an
    // invariant that is not enforced (pattern_ring_invariant_needs_static_assert).

    inline constexpr float kInvTwentyLog10Two = 1.0f / 6.020599913f;  ///< 1 / (20·log10 2)
    inline constexpr float kMinDistance       = 0.05f;                ///< metres, hard d_i floor
    inline constexpr float kMaxBlurMetres     = 8.0f;                 ///< absolute r_s ceiling
    inline constexpr float kBlurScale         = 0.5f;                 ///< blur=1 → r_s = 0.5·rigScale
    inline constexpr float kDenomEpsilon      = 1e-20f;               ///< all-zero-weight detector

    static_assert (kMinDistance > 0.0f,
                   "d_i floor must be strictly positive: DBAP divides by d_i^a");
    static_assert (kMaxBlurMetres > 0.0f,
                   "the absolute blur ceiling must be positive or a mistyped coordinate can produce "
                   "an r_s that swamps the array");
    static_assert (kBlurScale > 0.0f,
                   "blur must map to a positive radius");
    static_assert (kDenomEpsilon > 0.0f,
                   "the all-zero-weight guard must have a strictly positive threshold: at 0 the "
                   "comparison never fires and denom=0 yields k=inf, v=inf*0=NaN, which then latches "
                   "permanently in the SmoothedValue targets");

    //==========================================================================
    /** std::pow, counted under instrumentation and nothing else in the shipping build. */
    inline float countedPow (float base, float exponent) noexcept
    {
       #if OOCTAGON_INSTRUMENT
        instr::powCalls.fetch_add (1, std::memory_order_relaxed);
       #endif

        return std::pow (base, exponent);
    }

    //==========================================================================
    /** §3.3.2 — blur → spatial-blur radius in metres.

        `r_s = min (blur · kBlurScale · rigScale, kMaxBlurMetres)`

        `rigScale` is the RMS speaker radius from the centroid, which is the paper's §3.1
        "covariance of speaker distances from rig centre" made dimensional. Scaling by it is what
        makes `blur` room-size independent (DSP-08) — and the SCALING INVARIANT, not the constant, is
        what probe AG asserts (pattern_test_fixture_mirrors_drift_silently).
    */
    float blurToRadius (float blur, float rigScale) noexcept;

    /** §3.3.1 / eq 4 — rolloff in dB per distance doubling → the DBAP exponent `a`. */
    float rolloffToAlpha (float rolloff) noexcept;

    //==========================================================================
    /** The solve. Writes 8 gains satisfying `Σ v_i² = 1` (DSP-02), or 8 exact zeros (DSP-05/3).

        @param spk      speaker positions in metres, 3D
        @param w        per-speaker weights, 0..1
        @param src      solve position in metres, 3D — already hull-projected by the caller if outside
        @param a        rolloff exponent from rolloffToAlpha()
        @param rs       blur radius in metres from blurToRadius()
        @param outV     the 8 gains
        @param outInvK  optional. `1/k = √denom` — the UN-NORMALISED field. nullptr on the audio path.

        Exactly 8 std::pow calls. `t = pow(d, −a)` is computed once and reused as `t` and `t²`,
        halving §3.3.5's naive 16-per-sub-point budget — probe AE asserts the exact figure.

        ── WHY THE OUT-PARAM EXISTS, AND WHY IT IS THIS QUANTITY (PLAN-3.3 P69 / N10) ─────────────
        Phase 3.3's UI-04 backdrop draws the DBAP level field, and ROADMAP originally named
        `max_i v_i²` as the quantity. THAT FORMULA IS DISQUALIFIED BY MEASUREMENT. DBAP normalises
        to `Σ v_i² = 1`, so `max_i v_i²` measures CONCENTRATION, not level: it is IDENTICALLY
        1.0000 at every point in the room whenever exactly one weight is non-zero, and spans only
        3.2-5.4 dB otherwise. The picture would go blank precisely when the spatial situation is
        most extreme.

        `1/k = √denom` is the field BEFORE normalisation. Measured over the default envelope it
        spans 1.3-10.4 dB with correct radial structure and never degenerates.

        ── A DEFAULTED OUT-PARAM, following P54's `MapDiagnosis* whyNot = nullptr` ────────────────
        Every existing call site compiles unchanged and NOT ONE IS TOUCHED. No new `std::pow` is
        introduced — `denom` is already computed here — so `powCalls == 16` is untouched and probe
        AE keeps its exact figure. Probe CA asserts BOTH halves: that `outInvK` equals `√denom`,
        and that the eight gains are BIT-IDENTICAL to the nullptr path.

        REJECTED: recomputing `denom` inside the field sampler. That would be a mirrored fixture
        over the highest-risk arithmetic in the plugin
        (`pattern_test_fixture_mirrors_drift_silently`), and it would turn UI-04 criterion 1's
        "compared against a direct solve" into a comparison of two copies of the same mistake.

        On the all-zero-weight early return this writes `0.0f`, which is the correct field value
        for a rig with no active speaker — not a sentinel, and not left uninitialised.
    */
    void solve (const Vec3 spk[kNumSpeakers], const float w[kNumSpeakers], Vec3 src,
                float a, float rs, float outV[kNumSpeakers],
                float* outInvK = nullptr) noexcept;
}

} // namespace oo
