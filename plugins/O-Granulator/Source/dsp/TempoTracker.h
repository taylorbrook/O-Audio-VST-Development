/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
#include "EngineConstants.h"

// ============================================================================
// Host transport → engine (Stage 2 / Phase 2.5a, PLAN-2.5a P48; RESEARCH-2.5a
// R39, R42, R43, R44).
//
// The processor reads getPlayHead()->getPosition() ONCE per host block (by
// value — no allocation) into a POD TransportSnapshot and hands it to
// Engine::process, which calls TempoTracker::update before the first piece.
// Nothing else in the plugin touches the playhead.
//
// R39 — HELD ANCHOR. Every ppq the engine uses is
//     ppq (abs) = anchor.ppq + (abs − anchor.abs) · anchor.pps
// a pure function of the absolute sample counter. The host position is only
// compared with a prediction and adopted when it disagrees, so a constant
// tempo never re-anchors and grid ppq values are bitwise identical at every
// host block size (a per-block host origin differs at 30–52 % of grid points).
//
// R42 — per-block rules (evaluated on the host's report at the block start):
//   invalid (no ppq / no bpm / bpm <= 0, WR-09)  → 120 bpm, not playing
//   not playing        → IGNORE host ppq: the virtual ppq runs on at host bpm
//                        (re-anchor on a bpm / time-signature change only)
//   isPlaying rising   → re-anchor to the host + RE-LOCK
//   playing, the host ppq misses BOTH predictions from the previous report by
//                        more than 1 ms → discontinuity (loop, locate): re-anchor
//                        + RE-LOCK. Predictions: step (constant tempo through the
//                        previous block) and trapezoid (linear ramp, R42) — a
//                        tempo step OR a tempo ramp is not a discontinuity.
//   playing, |host − anchor| > 0.5 sample, or bpm / time signature / the host's
//                        bar frame changed → re-anchor only (the grid re-times,
//                        LFO direct continues)
//   otherwise          → keep the anchor
// R44 — bars from the host time signature: barLen = 4·num/den quarters (4/4
// if absent), barOrigin = floorMod (ppqPositionOfLastBarStart, barLen), latched
// at each anchor (0 when the host gives no bar start).
// All grid maths in double with floor / ceil; never fmod or an int cast
// (negative ppq in pre-roll).
// ============================================================================
namespace ogran
{
    /** One host block's transport, as reported at its first sample. */
    struct TransportSnapshot
    {
        bool   valid      = false;   // a ppq position AND a bpm were reported
        bool   playing    = false;
        bool   hasLastBar = false;   // ppqPositionOfLastBarStart reported
        double bpm        = 0.0;
        double ppq        = 0.0;
        double lastBarPpq = 0.0;
        int    num = 4, den = 4;     // 4/4 when absent
    };

    /** x − floor (x / m)·m: the floor-based modulo (m > 0), exact sign for negative x. */
    inline double floorMod (double x, double m) noexcept
    {
        return x - std::floor (x / m) * m;
    }

    /** Exact equality without -Wfloat-equal (NaN compares unequal). */
    inline bool sameValue (double a, double b) noexcept { return ! (a < b) && ! (a > b) && a == a && b == b; }

    /** frac (x) = x − floor (x), in [0, 1) for every finite x (also negative). */
    inline double fracPart (double x) noexcept
    {
        const double f = x - std::floor (x);
        return f < 1.0 ? f : 0.0;
    }

    struct TempoAnchor
    {
        std::int64_t abs   = 0;      // absolute sample of the anchor
        double ppq         = 0.0;    // ppq at abs
        double pps         = 0.0;    // quarters per sample (bpm / 60 / fs)
        double bpm         = kTempoFallbackBpm;
        double barOrigin   = 0.0;    // floorMod (last bar start, barLen)
        double barLen      = 4.0;    // quarters per bar
        int    num = 4, den = 4;
    };

    class TempoTracker
    {
    public:
        /** Message thread (Engine::prepare): virtual 120 bpm at ppq 0; the first
            update adopts the host (counted neither as re-anchor nor re-lock). */
        void prepare (double sampleRate) noexcept
        {
            fs = sampleRate > 0.0 ? sampleRate : 48000.0;
            a = {};
            a.pps = kTempoFallbackBpm / 60.0 / fs;
            fresh = true;
            wasPlaying = false;
            prevValid = false;
            ++relockEpoch;
            reanchors = relocks = 0;
        }

        /** Audio thread, once per host block, before the first piece. */
        void update (const TransportSnapshot& s, std::int64_t absBlockStart) noexcept
        {
            const bool   valid   = s.valid && s.bpm > 0.0 && std::isfinite (s.bpm) && std::isfinite (s.ppq);
            const bool   playing = valid && s.playing;
            const double bpm     = valid ? s.bpm : kTempoFallbackBpm;
            const double pps     = bpm / 60.0 / fs;
            const int    num     = s.num > 0 ? s.num : 4;
            const int    den     = s.den > 0 ? s.den : 4;
            const double barLen  = 4.0 * static_cast<double> (num) / static_cast<double> (den);
            const bool   sigChanged = num != a.num || den != a.den;
            const bool   bpmChanged = ! sameValue (bpm, a.bpm);   // exact: a host re-reporting the same bpm keeps the anchor
            // The host moved its bar frame without a signature change (a pickup bar): re-anchor.
            bool barMoved = false;
            if (s.hasLastBar && std::isfinite (s.lastBarPpq) && ! sigChanged)
            {
                const double d = std::abs (floorMod (s.lastBarPpq, barLen) - a.barOrigin);
                barMoved = std::min (d, barLen - d) > 1.0e-9;
            }

            auto adoptHost = [&] (bool relock) noexcept
            {
                a.abs = absBlockStart;
                a.ppq = s.ppq;
                a.pps = pps;
                a.bpm = bpm;
                a.num = num; a.den = den; a.barLen = barLen;
                a.barOrigin = s.hasLastBar && std::isfinite (s.lastBarPpq) ? floorMod (s.lastBarPpq, barLen) : 0.0;
                if (! fresh)
                {
                    ++reanchors;
                    if (relock) ++relocks;
                }
                if (relock) ++relockEpoch;
            };
            auto reanchorVirtual = [&] () noexcept
            {
                const double q = ppqAt (absBlockStart);
                const double oldLen = a.barLen;
                a.abs = absBlockStart;
                a.ppq = q;
                a.pps = pps;
                a.bpm = bpm;
                if (! sameValue (barLen, oldLen))
                    a.barOrigin = floorMod (a.barOrigin, barLen);
                a.num = num; a.den = den; a.barLen = barLen;
                if (! fresh)
                    ++reanchors;
            };

            if (fresh)
            {
                if (valid) adoptHost (true);
                else       reanchorVirtual();
                ++relockEpoch;   // prepare-time lock
            }
            else if (! playing)
            {
                // Stopped / invalid: ignore host ppq; the virtual ppq runs on.
                if (bpmChanged || sigChanged)
                    reanchorVirtual();
            }
            else if (! wasPlaying)
            {
                adoptHost (true);   // transport start: re-anchor + re-lock
            }
            else if (perBlockOrigin)
            {
                adoptHost (false);  // TT1n / N8n: the GrainScatter per-block origin (R39 control)
            }
            else
            {
                // Discontinuity: the host misses both the step and the trapezoid
                // prediction from its previous report by more than 1 ms.
                bool relock = true;
                if (prevValid)
                {
                    const double n         = static_cast<double> (absBlockStart - prevAbs);
                    const double stepPred  = prevPpq + n * prevPps;
                    const double trapPred  = prevPpq + n * 0.5 * (prevPps + pps);
                    const double err       = std::min (std::abs (s.ppq - stepPred), std::abs (s.ppq - trapPred)) / pps;
                    relock = ! (err <= kRelockSeconds * fs);
                }
                if (relock)
                    adoptHost (true);
                else if (bpmChanged || sigChanged || barMoved || std::abs (s.ppq - ppqAt (absBlockStart)) / pps > kRetimeSamples)
                    adoptHost (false);
            }

            fresh = false;
            wasPlaying = playing;
            prevValid  = playing;
            prevAbs    = absBlockStart;
            prevPpq    = s.ppq;
            prevPps    = pps;
        }

        double ppqAt (std::int64_t abs) const noexcept
        {
            return a.ppq + static_cast<double> (abs - a.abs) * a.pps;
        }

        const TempoAnchor& anchor() const noexcept   { return a; }
        bool isPlaying() const noexcept              { return wasPlaying; }
        std::uint64_t getRelockEpoch() const noexcept { return relockEpoch; }
        std::int64_t getReanchors() const noexcept   { return reanchors; }
        std::int64_t getRelocks() const noexcept     { return relocks; }
        double getSampleRate() const noexcept        { return fs; }

        bool perBlockOrigin = false;   // test hook (Engine copies TestHooks::perBlockOrigin)

    private:
        double fs = 48000.0;
        TempoAnchor a;
        bool fresh = true, wasPlaying = false, prevValid = false;
        std::int64_t prevAbs = 0;
        double prevPpq = 0.0, prevPps = 0.0;
        std::uint64_t relockEpoch = 0;
        std::int64_t reanchors = 0, relocks = 0;
    };
} // namespace ogran
