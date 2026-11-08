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

// ============================================================================
// Scan playheads + loop region (ARCHITECTURE §8, CONTEXT-2.1 Scan, PLAN-2.1 T8).
//
// Region [lo, hi) = sort (loop_start, loop_end) · F, minimum width 10 ms of
// tape time, computed in the DSP (no parameter writes; row S2).
//
// The scan offset σ is kept in TAPE SECONDS, so an SR rebuild (tape rate
// change) needs no rebase: σ_frames = σ · tapeRate. It advances by
// scan_rate / fs tape-seconds per output sample (scan 100 % = 1 tape-second
// per second; 0 holds). σ is an ANCHORED line, σ (t) = sec + (t − t0)·rate,
// re-anchored (and wrapped) at every absolute grid tick and restarted at a
// note-on, so its value at a sample never depends on the host block split.
//
//   Retrigger  one σ per voice, 0 at note-on;       c = lo + wrap (pos·W + σ, W)
//   Free-run   one σ for the stream, shared;        c = lo + wrap (pos·W + σ, W)
//              runs continuously, reset in prepareToPlay
//   Ping-pong  one σ per voice, 0 at note-on;       c = lo + tri (pos·W + σ, W)
//              (reflects at the region edges)
// ============================================================================
namespace ogi
{
    enum ScanMode : int { kScanFreeRun = 0, kScanRetrigger = 1, kScanPingPong = 2 };

    struct LoopRegion
    {
        double lo = 0.0, hi = 0.0;
        double width() const noexcept { return hi - lo; }
    };

    /** sort (a, b) · F with a minimum width of minFrames, kept inside [0, F]. */
    inline LoopRegion loopRegion (double loopStart01, double loopEnd01, int frames, double minFrames) noexcept
    {
        LoopRegion r;
        const double F = static_cast<double> (std::max (0, frames));
        const double a = std::clamp (std::min (loopStart01, loopEnd01), 0.0, 1.0);
        const double b = std::clamp (std::max (loopStart01, loopEnd01), 0.0, 1.0);
        r.lo = a * F;
        r.hi = b * F;
        const double minW = std::min (std::max (1.0, minFrames), F);
        if (r.hi - r.lo < minW)
        {
            r.hi = r.lo + minW;
            if (r.hi > F)
            {
                r.hi = F;
                r.lo = std::max (0.0, F - minW);
            }
        }
        return r;
    }

    /** x mod w in [0, w) (w > 0). */
    inline double wrapPos (double x, double w) noexcept
    {
        if (! (w > 0.0))
            return 0.0;
        double m = x - w * std::floor (x / w);
        if (m >= w) m -= w;
        if (m < 0.0) m = 0.0;
        return m;
    }

    /** Triangle reflection of x into [0, w]. */
    inline double reflectPos (double x, double w) noexcept
    {
        if (! (w > 0.0))
            return 0.0;
        const double m = wrapPos (x, 2.0 * w);
        return m <= w ? m : 2.0 * w - m;
    }

    /** Reflects x into [a, b] (b <= a → a). */
    inline double reflectInto (double x, double a, double b) noexcept
    {
        if (! (b > a))
            return a;
        return a + reflectPos (x - a, b - a);
    }

    /** One anchored scan line. */
    struct ScanAnchor
    {
        double       sec  = 0.0;   // σ at t0, tape seconds
        std::int64_t t0   = 0;     // absolute output sample
        double       rate = 0.0;   // tape seconds per output sample

        double at (std::int64_t t) const noexcept { return sec + static_cast<double> (t - t0) * rate; }

        void restart (std::int64_t t, double newRate) noexcept
        {
            sec = 0.0;
            t0 = t;
            rate = newRate;
        }

        /** Grid tick: σ (t) wrapped into [0, period) (period <= 0: unwrapped). */
        void reanchor (std::int64_t t, double newRate, double periodSec) noexcept
        {
            double s = at (t);
            if (periodSec > 0.0)
                s = wrapPos (s, periodSec);
            sec = s;
            t0 = t;
            rate = newRate;
        }
    };

    /** Read centre in tape frames for a region, position 0..1 and σ in frames. */
    inline double scanCentre (const LoopRegion& r, double position01, double sigmaFrames, int mode) noexcept
    {
        const double w = r.width();
        const double u = position01 * w + sigmaFrames;
        return r.lo + (mode == kScanPingPong ? reflectPos (u, w) : wrapPos (u, w));
    }

    /** Wrap period of σ for a mode (tape seconds): W, or 2W for Ping-pong. */
    inline double scanPeriodSec (const LoopRegion& r, double tapeRate, int mode) noexcept
    {
        if (! (tapeRate > 0.0))
            return 0.0;
        const double w = r.width() / tapeRate;
        return mode == kScanPingPong ? 2.0 * w : w;
    }
} // namespace ogi
