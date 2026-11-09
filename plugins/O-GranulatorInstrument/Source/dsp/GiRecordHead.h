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
//     rw = clamp (r, −kAgcCeil, kAgcCeil)             (PD2 writer clamp, counted)
//     w  = (1 − λ)·wc + λ·rw                          (convex write, D7)
//     w  non-finite → wc (writeScrubs)                (ARCH §17)
//     w  = clamp (w, min (wc, rw), max (wc, rw))      (PD2 convex envelope)
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

    struct RecordHeadFlags
    {
        bool rClampOff    = false;   // FL1s control: no writer clamp
        bool writeScrubOff = false;  // FL6 control: a non-finite w is written
        bool settleOff    = false;   // MS1 control: no settle at wrap / park
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
            CURRENT loop region (latched at a wrap). rL / rR: kFbDelay samples. */
        void writeCell (MipTape<kTapeLevels>& tape, const float* rL, const float* rR, const ExactRamp& fb,
                        double rlo, double rhi, int frames, const RecordHeadFlags& f, RecordHeadCounters& c) noexcept
        {
            float bufL[kCell], bufR[kCell];
            cellStart = h;
            wrapOff = -1;

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
        }
    };
} // namespace ogi
