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

#include <cmath>
#include <cstdint>
#include <cstring>

namespace oo::motion
{

/**
    v1.8.0 — MOTION PHASE AS A PURE FUNCTION OF ABSOLUTE POSITION (RESEARCH Q1 / Q7).

    ── The trap this file exists to avoid ───────────────────────────────────────────────────────
    O-Orbit's MotionEngine::advance(numSamples) integrates `phase += 2π·f·numSamples/sr` once per
    host block, so both the sample point AND the increment depend on the buffer size
    (pattern_block_rate_envelope_breaks_blocksize_invariance). Here there is NO ACCUMULATOR.
    cyclesAt() is evaluated at each boundary of GainStage's absolute 64-sample control grid and
    derives the phase from the absolute sample counter (Free mode) or from the host's PPQ
    extrapolated to that boundary (Sync mode). A 1024-sample block reaches the same boundaries as
    sixteen 64-sample blocks, so the answer is the same — probes DE/DF hold that with ragged sizes,
    and negative control NC2 (an accumulator in this function's place) is what proves they can fail.

    ── The one piece of state, and why it is not an accumulator ─────────────────────────────────
    A position-derived phase JUMPS when the rate changes: rate·t is discontinuous in rate. At the
    grid boundary where a new rate is first observed, `phaseBase += (oldRate − newRate)·t` — the
    observation point is a grid boundary, which is absolute, so the re-base is deterministic and
    block-size invariant. `phaseBase` is a constant of integration, not an integral.

    ── The four situations (Q7) ─────────────────────────────────────────────────────────────────
      Free                          cycles = rate·t + phaseBase, t = absoluteSample / sr
      Synced, host rolling with PPQ cycles = (ppqBlockStart + (Δsamples/sr)·bpm/60) · mult
      Synced, transport stopped     HOLD: the host's stopped PPQ · mult (constant), or the last
                                    rolling value when the host stops supplying PPQ
      Synced, no PPQ at all         free-run from the absolute counter at bpm/60 · mult (bpm 120
                                    when the host gives none — Standalone)

    No JUCE. <cstdint> only, so the geometry unit target compiles it.
*/

/// What the processor reads from getPlayHead() once per block and hands to GainStage (P24).
struct HostClock
{
    double bpm      { 120.0 };
    double ppq      { 0.0 };     // PPQ at the START of the current host block
    bool   ppqValid { false };   // the host supplied a PPQ position at all
    bool   playing  { false };   // the transport is rolling
};

/// Not an accumulator. See the header note.
struct MotionClockState
{
    double phaseBase   { 0.0 };
    float  lastRate    { -1.0f };   // < 0: no rate observed yet, so the first call does not re-base
    double heldCycles  { 0.0 };
    bool   haveHeld    { false };
};

/// Sync choices, in MENU ORDER: index 0 is Free; 1..14 are the fourteen note divisions O-Orbit
/// also offers, in CYCLES PER BEAT (Hz = bpm/60 · mult). A division names the DURATION of one
/// cycle: "1/4" is one cycle per quarter note — one cycle per BEAT — and "1 Bar" is one cycle
/// per four beats. The bar entries assume 4/4; the plugin reads no time signature.
///
/// v1.10.0 (WR-01): the v1.8.0 table was 4× slower than every label ("1/4" = 0.25, "1 Bar" =
/// 0.0625 = one cycle per SIXTEEN beats) and used 4/3 for triplets, so "1/16D" and "1/8T" were
/// bit-identical. A triplet is 3/2 the rate of its parent; a dotted note is 2/3.
inline constexpr int kNumSyncChoices = 15;

inline constexpr double kSyncMultipliers[kNumSyncChoices] = {
    0.0,          // Free (unused — the Free branch never reads it)
    6.0,          // 1/16T
    4.0,          // 1/16
    8.0 / 3.0,    // 1/16D
    3.0,          // 1/8T
    2.0,          // 1/8
    4.0 / 3.0,    // 1/8D
    1.5,          // 1/4T
    1.0,          // 1/4
    2.0 / 3.0,    // 1/4D
    0.5,          // 1/2
    1.0 / 3.0,    // 1/2D
    0.25,         // 1 Bar
    0.125,        // 2 Bars
    0.0625        // 4 Bars
};

/** The free-running branch, shared by Free mode and the no-PPQ fallback. `rate` in Hz. */
inline double freeRunCycles (std::uint64_t absoluteSample, double sr, float rate,
                             MotionClockState& st) noexcept
{
    const double t = static_cast<double> (absoluteSample) / sr;

    // Bitwise, not ==: -Wfloat-equal, and a rate that is the same bits is the same rate.
    if (st.lastRate >= 0.0f && std::memcmp (&rate, &st.lastRate, sizeof (float)) != 0)
        st.phaseBase += (static_cast<double> (st.lastRate) - static_cast<double> (rate)) * t;

    st.lastRate = rate;

    return static_cast<double> (rate) * t + st.phaseBase;
}

/** Cycles at `absoluteSample`.

    @param samplesSinceBlockStart  how far the grid boundary sits past the host block's first
                                   sample — the PPQ extrapolation distance. 0 at a block start.
    @param clock                   may be null (every harness call site that predates v1.8.0):
                                   treated as "no PPQ, bpm 120".
*/
inline double cyclesAt (std::uint64_t absoluteSample, double sr,
                        std::uint64_t samplesSinceBlockStart,
                        const HostClock* clock, int syncIndex, float rateHz,
                        MotionClockState& st) noexcept
{
    if (syncIndex <= 0 || syncIndex >= kNumSyncChoices)
        return freeRunCycles (absoluteSample, sr, rateHz, st);

    const double mult = kSyncMultipliers[syncIndex];
    // isfinite here as well as at the processor's copy (IN-01): this header has no JUCE and can
    // be fed by any harness; `> 0.0` alone admits +inf.
    const double bpm  = (clock != nullptr && std::isfinite (clock->bpm) && clock->bpm > 0.0)
                          ? clock->bpm : 120.0;

    if (clock != nullptr && clock->ppqValid)
    {
        if (clock->playing)
        {
            const double beats = clock->ppq
                               + (static_cast<double> (samplesSinceBlockStart) / sr) * (bpm / 60.0);
            st.heldCycles = beats * mult;
            st.haveHeld   = true;
            return st.heldCycles;
        }

        // Stopped, PPQ supplied: the host's own (constant) position — where playback resumes.
        st.heldCycles = clock->ppq * mult;
        st.haveHeld   = true;
        return st.heldCycles;
    }

    // No PPQ. If the transport has stopped feeding us one after rolling, hold; otherwise free-run
    // at the tempo-derived rate (Standalone).
    if (clock != nullptr && ! clock->playing && st.haveHeld)
        return st.heldCycles;

    return freeRunCycles (absoluteSample, sr, static_cast<float> ((bpm / 60.0) * mult), st);
}

} // namespace oo::motion
