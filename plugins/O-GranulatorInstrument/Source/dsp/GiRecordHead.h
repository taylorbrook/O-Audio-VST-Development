/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "GiConstants.h"
#include "MipTape.h"
#include "forked/ControlGrid.h"   // ExactRamp

// ============================================================================
// RecordHead — one slot's working-copy writer (PLAN-2.4 PD2–PD5, PD10;
// ARCHITECTURE D7 / §17, RESEARCH-2.4 §3).
//
// The head moves at exactly 1× through its latched loop [lo, hi) (integer
// tape frames, re-latched from the slot's loop region at every wrap). At each
// grid tick it writes the WHOLE cell [h, h + 32) before any grain of that cell
// reads (PD3: kFbDelay == kCell, so every return r[n − 32] of the cell is
// already in fbRet):
//
//     λ  = kFbKnob · fb.value (i) · run.value (i)     (20 ms ramps, PD4)
//     rw = clamp (g·r, −kAgcCeil, kAgcCeil)           (R2 makeup g, then the PD2 writer clamp, counted)
//     w  = (1 − λ)·wc + λ·rw                          (convex write, D7)
//     w  non-finite → wc (writeScrubs)                (ARCH §17)
//     w  = clamp (w, min (wc, rw), max (wc, rw))      (PD2 convex envelope)
//
// Level-hold makeup (PLAN-R2 PD6, hands-on finding F3). Every stage of the
// return has gain ≤ 1 (the tap law alone is 4 to 20 dB), so the convex write
// used to erase the lane lap by lap. g is a feed-forward gain formed once per
// cell, at the tick, before the write:
//
//   · running one-pole means (τ = 1 s) over the cells already written of
//     A = wc², B = wc·r, C = r² (both channels, r before the makeup) and of the
//     constant 1 (u), so the target T = ref·u is unbiased from the first cell
//     after a park. ref = the mean square of the PRISTINE over the latched
//     loop (SlotTape::refMeanSq), never the working copy;
//   · g solves λ²C·g² + 2λ(1−λ)B·g + (1−λ)²A = T: the written power of the
//     convex blend equals the pristine power (λ at mid-cell);
//   · limits, in this order: cap g ≤ kMkCap·κ, κ = √((2−λ)/λ) (what a blend of
//     two uncorrelated signals at equal level loses); peak g ≤ kAgcCeil / pk
//     (pk = held return peak, 50 ms release); short-term ceiling
//     g ≤ κ·√(kMkCeilX·ref / Cf) (Cf = the return's cell mean square, held
//     50 ms); floor g ≥ 1; a non-finite g becomes 1;
//   · g may FALL to its new value inside one cell and rises at no more than
//     kMkRiseDbPerS; it is applied as a linear ramp across the cell;
//   · a cell with a non-finite return sample updates none of the running
//     values and writes with the previous gain (a NaN in a running mean would
//     stay until the head parks);
//   · a parked head resets the makeup (Engine::writeHeads).
//
// The bound is untouched: g acts before the writer clamp, so
// |w| ≤ max (|wc|, kAgcCeil) for any finite g. shortRun counts the consecutive
// cells whose levelled return sits more than kMkLatchDb under the κ-normalised
// target; Engine::updateHeads turns it into the stop-writing latch (PD9).
// At the tick: arithmetic and sqrt only (every exp / pow is in
// MakeupCoef::prepare).
//
// Level 0 and every mip child go through MipTape::writeRun; a cell that
// crosses hi is split into two runs, the head re-latches at lo and the children
// past the old hi are settled (PD10). park() settles the children past a
// stopped head. Audio thread: no allocation (32-frame scratch).
// ============================================================================
namespace ogi
{
    /** Frames past a wrap / park point whose mip children are re-derived
        (PLAN-2.4 PD10, corrected): a level-l child depends on 127 parent
        frames, so the stale span after the last write is 126·(2^l − 1) level-0
        frames at level l (1890 at l = 4) — more than margin (4) = 1074, which
        bounds the READ reach, not the child dependency. 2048 covers it with
        the index rounding. */
    inline constexpr int kSettleFrames = 128 << kTapeLevels;
    static_assert (kSettleFrames >= 126 * ((1 << kTapeLevels) - 1) + (2 << kTapeLevels),
                   "settle span must cover the half-band dependency chain of every level");

    // PLAN-R2 PD6: the makeup's fixed constants (sign-offs 1–3, 2026-11-13).
    inline constexpr double kMkCap        = 100.0;                // cap on g / κ: +40 dB
    inline constexpr double kMkCeilX      = 7.943282347242816;    // short-term ceiling, energy ratio: +9 dB
    inline constexpr double kMkTauS       = 1.0;                  // running means of A, B, C, u
    inline constexpr double kMkRelS       = 0.05;                 // held return peak and held cell mean square
    inline constexpr double kMkRiseDbPerS = 375.0;                // 0.25 dB per cell at 48 kHz
    inline constexpr double kMkLatchDb    = 6.0;                  // PD9: "short" = more than this under the target …
    inline constexpr double kMkLatchS     = 0.05;                 // … for this long (written cells)
    inline constexpr double kMkLamMin     = 1.0e-4;               // below this λ the cell writes (almost) nothing: g = 1
    inline constexpr double kMkRefMin     = 1.0e-12;              // −120 dBFS: no target below it (silence is never lifted)

    /** Rate-dependent makeup coefficients, formed once in Engine::prepare. */
    struct MakeupCoef
    {
        double mean      = 0.0;    // one-pole coefficient per cell, τ = kMkTauS
        double rel       = 0.0;    // 50 ms release per cell (cell mean square)
        float  pkRel     = 0.0f;   // 50 ms release per cell (return peak)
        float  rise      = 1.0f;   // largest gain ratio from one cell to the next
        double shortRatio = 0.25;  // energy ratio of kMkLatchDb
        int    latchCells = 75;    // cells in kMkLatchS

        void prepare (double fs) noexcept
        {
            const double cellS = static_cast<double> (kCell) / fs;
            mean  = std::exp (-cellS / kMkTauS);
            rel   = std::exp (-cellS / kMkRelS);
            pkRel = static_cast<float> (rel);
            rise  = static_cast<float> (std::pow (10.0, kMkRiseDbPerS * cellS / 20.0));
            shortRatio = std::pow (10.0, -kMkLatchDb / 10.0);
            latchCells = std::max (1, static_cast<int> (std::lround (kMkLatchS / cellS)));
        }
    };

   #if OGI_TEST_HOOKS
    /** Gain statistics for the LH rows (test builds only). */
    struct MakeupStats
    {
        std::uint64_t cells = 0, capCells = 0, pkCells = 0, ceilCells = 0, scrubCells = 0;
        double gLogSum = 0.0;                // Σ log10 g
        float  gMin = 1.0f, gMax = 1.0f;
    };
   #endif

    /** Per-head makeup state (a few scalars; reset whenever the head is parked). */
    struct Makeup
    {
        double a = 0.0, b = 0.0, c = 0.0, u = 0.0;   // running means
        double cf = 0.0;                             // held cell mean square of the return
        float  g = 1.0f, pk = 0.0f;                  // gain at the end of the last cell; held return peak
        int    shortRun = 0;                         // consecutive short cells (PD9)

        void reset() noexcept { a = b = c = u = 0.0; cf = 0.0; g = 1.0f; pk = 0.0f; shortRun = 0; }
    };

    struct RecordHeadFlags
    {
        bool rClampOff    = false;   // FL1s control: no writer clamp
        bool writeScrubOff = false;  // FL6 control: a non-finite w is written
        bool settleOff    = false;   // MS1 control: no settle at wrap / park
        // PLAN-R2 PD8
        bool makeup        = true;   // false = g ≡ 1 and no statistics: the R1 write (makeupOff, chainKnobParity)
        bool mkAttackSlew  = false;  // LH4 control: the gain falls at the rise limit too
        bool mkPeakOff     = false;  // LH10 control: no peak limit
        bool mkCapOff      = false;  // LH7 control: no cap
        bool mkStatScrubOff = false; // LH12 control: a non-finite return cell enters the running values
       #if OGI_TEST_HOOKS
        MakeupStats* stats = nullptr;
       #endif
    };

    struct RecordHeadCounters
    {
        std::uint64_t writes = 0;       // frames written
        std::uint64_t rClamps = 0;      // returns clamped to ±kAgcCeil
        std::uint64_t writeScrubs = 0;  // non-finite w replaced by wc
        std::uint64_t settles = 0;      // settle bursts (wrap + park)
        std::uint64_t parks = 0;
        float maxWritten = 0.0f;        // max |w| written
    };

    struct RecordHead
    {
        std::int64_t h  = 0;            // next frame to write
        std::int64_t lo = 0, hi = 0;    // latched loop (integer frames)
        ExactRamp    run;               // 0 parked … 1 running (20 ms, PD4)
        float        returnPk = 0.0f;   // held per-cell return peak (50 ms release)
        bool         parked = true;     // λ_eff settled at exactly 0 for the cell
        bool         swapWaiting = false;
        Makeup       mk;                // PLAN-R2 PD6

        // Head position for every sample of the cell written at the last tick
        // (the clamp / scan read it): frame of offset o = cellStart + o, or
        // after a wrap at offset wrapOff: wrapLo + (o − wrapOff).
        std::int64_t cellStart = 0, wrapLo = 0;
        int          wrapOff = -1;

        std::int64_t headAt (int o) const noexcept
        {
            return (wrapOff >= 0 && o >= wrapOff) ? wrapLo + (o - wrapOff) : cellStart + o;
        }

        /** Integer loop of a region (ceil lo, floor hi; at least 1 frame). */
        static void latchRegion (double rlo, double rhi, int frames, std::int64_t& lo, std::int64_t& hi) noexcept
        {
            if (frames <= 1)
            {
                lo = 0;
                hi = std::max (1, frames);
                return;
            }
            lo = std::clamp<std::int64_t> (static_cast<std::int64_t> (std::ceil (rlo)), 0, frames - 1);
            hi = std::clamp<std::int64_t> (static_cast<std::int64_t> (std::floor (rhi)), lo + 1, frames);
        }

        /** Places the head at the region start, parked (new tape / SR rebuild). */
        void relatch (double rlo, double rhi, int frames) noexcept
        {
            latchRegion (rlo, rhi, frames, lo, hi);
            h = lo;
            cellStart = h;
            wrapOff = -1;
        }

        /** Settles the children of [pos, pos + kSettleFrames) (no level-0 change). */
        static void settle (MipTape<kTapeLevels>& tape, std::int64_t pos, const RecordHeadFlags& f, RecordHeadCounters& c) noexcept
        {
            if (f.settleOff)
                return;
            tape.settleRun (pos, kSettleFrames);
            ++c.settles;
        }

        /** The head stopped (λ_eff landed on 0): settle the children past it. */
        void park (MipTape<kTapeLevels>& tape, const RecordHeadFlags& f, RecordHeadCounters& c) noexcept
        {
            settle (tape, h, f, c);
            ++c.parks;
        }

        /** Writes the whole cell at the tick. r = the previous cell's returns
            (r[ch][i] = r[n − 32] for frame i of this cell). rlo / rhi: the slot's
            CURRENT loop region (latched at a wrap). rL / rR: kFbDelay samples.
            mc / refMs (PLAN-R2 PD6): the makeup coefficients and the pristine
            mean square of the latched loop (0 = no reference: g = 1). */
        void writeCell (MipTape<kTapeLevels>& tape, const float* rL, const float* rR, const ExactRamp& fb,
                        double rlo, double rhi, int frames, const RecordHeadFlags& f, RecordHeadCounters& c,
                        const MakeupCoef& mc, double refMs) noexcept
        {
            float bufL[kCell], bufR[kCell];
            cellStart = h;
            wrapOff = -1;

            // PLAN-R2 PD6: this cell's makeup gain, from the statistics through
            // the previous cell. gPrev → gNew is the linear ramp across the cell.
            const float gPrev = mk.g;
            float gNew = gPrev;
            bool mkStats = false;   // the cell's sums enter the running means
            if (f.makeup)
            {
                float cellPk = 0.0f;
                double cellMs = 0.0;
                bool finite = true;
                for (int i = 0; i < kCell; ++i)
                {
                    const float l = rL[i], r = rR[i];
                    finite = finite && std::isfinite (l) && std::isfinite (r);
                    cellPk = std::max ({ cellPk, std::abs (l), std::abs (r) });
                    cellMs += static_cast<double> (l) * l + static_cast<double> (r) * r;
                }
                if (finite || f.mkStatScrubOff)
                {
                    mkStats = true;
                    cellMs /= 2.0 * kCell;
                    mk.cf = std::max (cellMs, mk.cf * mc.rel);
                    mk.pk = std::max (cellPk, mk.pk * mc.pkRel);

                    const double lam = static_cast<double> (kFbKnob * fb.value (kCell / 2) * run.value (kCell / 2));
                    const double T = refMs * mk.u;
                    double g = 1.0;
                    bool capped = false, peaked = false, ceiled = false;
                    if (lam > kMkLamMin && T > kMkRefMin)
                    {
                        const double kap2 = (2.0 - lam) / lam;
                        const double kap  = std::sqrt (kap2);
                        const double cap  = f.mkCapOff ? 1.0e30 : kMkCap * kap;
                        const double qa = lam * lam * mk.c, qb = 2.0 * lam * (1.0 - lam) * mk.b, qc = (1.0 - lam) * (1.0 - lam) * mk.a - T;
                        if (qc >= 0.0)
                            g = 1.0;                 // what is left of the old content already carries the target
                        else if (qa <= 1.0e-30)
                            g = cap;                 // no return at all
                        else
                            g = (-qb + std::sqrt (std::max (0.0, qb * qb - 4.0 * qa * qc))) / (2.0 * qa);
                        if (g >= cap) { g = cap; capped = true; }
                        if (! f.mkPeakOff && mk.pk > 0.0f)
                        {
                            const double gp = static_cast<double> (kAgcCeil) / static_cast<double> (mk.pk);
                            if (g > gp) { g = gp; peaked = true; }
                        }
                        if (mk.cf > 1.0e-30)
                        {
                            const double gs = kap * std::sqrt (kMkCeilX * refMs / mk.cf);
                            if (g > gs) { g = gs; ceiled = true; }
                        }
                        if (g < 1.0) g = 1.0;
                    }
                    if (! std::isfinite (g)) g = 1.0;
                    gNew = std::min (static_cast<float> (g), gPrev * mc.rise);   // falls inside the cell, rises at the limit
                    if (f.mkAttackSlew)
                        gNew = std::max (gNew, gPrev / mc.rise);
                    mk.g = gNew;

                    // PD9: the levelled return against the κ-normalised target.
                    const double kap2n = lam > kMkLamMin ? (2.0 - lam) / lam : 1.0;
                    const bool shortNow = refMs > kMkRefMin
                                       && static_cast<double> (gNew) * static_cast<double> (gNew) * mk.cf < mc.shortRatio * kap2n * refMs;
                    mk.shortRun = shortNow ? std::min (mk.shortRun + 1, 1 << 30) : 0;
                   #if OGI_TEST_HOOKS
                    if (f.stats != nullptr)
                    {
                        auto& st = *f.stats;
                        ++st.cells;
                        st.capCells += capped; st.pkCells += peaked; st.ceilCells += ceiled;
                        st.gLogSum += std::log10 (static_cast<double> (std::max (gNew, 1.0e-30f)));
                        st.gMin = std::min (st.gMin, gNew); st.gMax = std::max (st.gMax, gNew);
                    }
                   #else
                    (void) capped; (void) peaked; (void) ceiled;
                   #endif
                }
               #if OGI_TEST_HOOKS
                else if (f.stats != nullptr)
                    ++f.stats->scrubCells;
               #endif
            }
            const float gStep = (gNew - gPrev) * kCellInv;
            double sa = 0.0, sb = 0.0, sc = 0.0;

            if (h > hi || h < lo)
            {
                // The head ran off its loop (cannot happen while latched; a guard).
                // h == hi is the ordinary wrap, handled (with its settle) below.
                latchRegion (rlo, rhi, frames, lo, hi);
                h = lo;
                cellStart = h;
            }

            std::int64_t runStart = h;
            int runLen = 0, runBase = 0;
            for (int i = 0; i < kCell; ++i)
            {
                if (h >= hi)
                {
                    // Wrap: flush the run, settle past the old hi, re-latch.
                    tape.writeRun (runStart, runLen, bufL + runBase, bufR + runBase);
                    const std::int64_t oldHi = hi;
                    latchRegion (rlo, rhi, frames, lo, hi);
                    h = lo;
                    settle (tape, oldHi, f, c);
                    wrapOff = i;
                    wrapLo  = h;
                    runStart = h;
                    runBase  = i;
                    runLen   = 0;
                }

                const float lam = kFbKnob * fb.value (i) * run.value (i);
                float out[2];
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float wc = tape.levelSample (0, ch, h);
                    float rw = (ch == 0 ? rL : rR)[i];
                    if (f.makeup)
                    {
                        sa += static_cast<double> (wc) * wc;
                        sb += static_cast<double> (wc) * rw;
                        sc += static_cast<double> (rw) * rw;
                        rw *= gPrev + gStep * static_cast<float> (i);
                    }
                    if (! f.rClampOff)
                    {
                        if (rw > kAgcCeil || rw < -kAgcCeil)
                            ++c.rClamps;
                        rw = std::clamp (rw, -kAgcCeil, kAgcCeil);   // NaN passes (scrubbed below)
                    }
                    float w = (1.0f - lam) * wc + lam * rw;
                    if (! std::isfinite (w))
                    {
                        if (! f.writeScrubOff)
                        {
                            w = wc;
                            ++c.writeScrubs;
                        }
                    }
                    else
                    {
                        w = std::clamp (w, std::min (wc, rw), std::max (wc, rw));
                    }
                    out[ch] = w;
                    c.maxWritten = std::max (c.maxWritten, std::abs (w));
                }
                bufL[i] = out[0];
                bufR[i] = out[1];
                ++runLen;
                ++h;
            }
            tape.writeRun (runStart, runLen, bufL + runBase, bufR + runBase);
            c.writes += static_cast<std::uint64_t> (kCell);

            if (mkStats)
            {
                const double k = 1.0 - mc.mean, n = 2.0 * kCell;
                mk.a = mc.mean * mk.a + k * (sa / n);
                mk.b = mc.mean * mk.b + k * (sb / n);
                mk.c = mc.mean * mk.c + k * (sc / n);
                mk.u = mc.mean * mk.u + k;
            }
        }
    };
} // namespace ogi
