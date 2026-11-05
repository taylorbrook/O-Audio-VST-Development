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

#include <cstdint>
#include "CounterRng.h"        // grain-engine 1.1.0
#include "GrainScheduler.h"    // grain-engine 1.1.0
#include "EngineConstants.h"

// ============================================================================
// Stream scheduler (ARCHITECTURE §4, RESEARCH-2.1 §7 R4 and §8, PLAN-2.1 T4).
//
// A wrapper over grain-engine's GrainScheduler per stream (Free timing only in
// 2.1 — Tempo arrives in 2.5).
//
// Telescoping jitter (§8 pin):
//     interval (k) = I · (1 + J · (u_{k+1} − u_k))
//     u_k = Draw (seed, kSchedStream + s, epoch + k).nextFloat (Slot::Jitter),  u_0 := ½
// Onset k sits at its grid slot displaced by J·(u_k − ½)·I, so the mean rate is
// exact at every jitter and nothing random-walks. u_0 := ½ puts the first onset
// after (re-)enable exactly on the grid (it fires immediately and cannot move
// backwards). J = 0 draws nothing. GrainScheduler::jittered is NOT used.
//
// R4 — onset counter survives a re-enable: GrainScheduler::reset() restarts its
// index at 0, which would replay the same random grains on every toggle. A
// per-stream `epoch` (zeroed only in prepare()) absorbs the onset count at each
// rising enable edge, and every draw is keyed on epoch + index.
// ============================================================================
namespace ogran
{
    /** One onset inside the current piece. */
    struct StreamOnset
    {
        int           offset      = 0;     // sample within the piece [0, len)
        double        frac        = 0.0;   // [0, 1): samples since the exact onset instant
        std::uint64_t globalIndex = 0;     // epoch + scheduler index — the CounterRng counter
        int           stream      = 0;
    };

    class StreamScheduler
    {
    public:
        /** Message thread (Engine::prepare). */
        void prepare() noexcept
        {
            sched.reset();
            epoch   = 0;
            enabled = false;
        }

        /** Grid tick. Rising edge: fold the onset count into the epoch and restart
            the clock, so the first onset fires on the next processed sample (R4). */
        void setEnabled (bool on) noexcept
        {
            if (on && ! enabled)
            {
                epoch += sched.getOnsetCount();
                sched.reset();
            }
            enabled = on;
        }

        bool isEnabled() const noexcept            { return enabled; }
        std::uint64_t getEpoch() const noexcept    { return epoch; }

        /** Onsets over the next n (<= kCell) samples into out[0 .. kCell).
            intervalSamples (I) and jitter (J, 0..1) are the grid values for this
            piece. Run only while enabled (a disabled stream draws nothing). */
        int process (int n, int streamIndex, std::uint64_t seed, double intervalSamples, float jitter,
                     StreamOnset* out) noexcept
        {
            const std::uint64_t ep     = epoch;
            const std::uint64_t key    = kSchedStream + static_cast<std::uint64_t> (streamIndex);
            const double        jit    = static_cast<double> (jitter);

            auto u = [seed, key, ep] (std::uint64_t k) noexcept -> double
            {
                if (k == 0)
                    return 0.5;
                return static_cast<double> (ouaricon::grain::CounterRng::Draw (seed, key, ep + k).nextFloat (Slot::Jitter));
            };

            auto nextInterval = [&u, intervalSamples, jit] (std::uint64_t k) noexcept -> double
            {
                if (! (jit > 0.0))
                    return intervalSamples;                  // J = 0: no draws
                return intervalSamples * (1.0 + jit * (u (k + 1) - u (k)));
            };

            // At most one onset per sample (kMinInterval = 1) and n <= kCell, so
            // the capacity can never drop an onset.
            const auto r = sched.process (n, raw, kCell, nextInterval);

            for (int i = 0; i < r.count; ++i)
                out[i] = { raw[i].offset, raw[i].frac, ep + raw[i].index, streamIndex };
            return r.count;
        }

    private:
        ouaricon::grain::GrainScheduler sched;
        ouaricon::grain::Onset raw[kCell] {};
        std::uint64_t epoch = 0;
        bool enabled = false;
    };

    /** Merge per-stream onset lists (each sorted by offset) into one list ordered
        by (offset, stream index) — the explicit tie-break (memory: sort ties change
        RNG assignment). Bounded: kMaxStreams × kCell entries. */
    inline int mergeOnsets (const StreamOnset (&lists)[kMaxStreams][kCell], const int (&counts)[kMaxStreams],
                            StreamOnset* out) noexcept
    {
        int head[kMaxStreams] {};
        int total = 0;
        for (int s = 0; s < kMaxStreams; ++s)
            total += counts[s];

        for (int k = 0; k < total; ++k)
        {
            int best = -1;
            for (int s = 0; s < kMaxStreams; ++s)
                if (head[s] < counts[s]
                    && (best < 0 || lists[s][head[s]].offset < lists[best][head[best]].offset))
                    best = s;
            out[k] = lists[best][head[best]++];
        }
        return total;
    }
} // namespace ogran
