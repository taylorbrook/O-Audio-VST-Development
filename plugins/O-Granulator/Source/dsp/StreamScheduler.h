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
#include "TempoTracker.h"

// ============================================================================
// Stream scheduler (ARCHITECTURE §4, RESEARCH-2.1 §7 R4 and §8, PLAN-2.1 T4).
//
// A wrapper over grain-engine's GrainScheduler per stream. Free + Tempo
// (2.5a): Free keeps GrainScheduler verbatim; Tempo is a plugin-local
// absolute-time grid clock (RESEARCH-2.5a R40 / R41, PLAN-2.5a P49).
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
//
// Tempo (2.5a, R40 / R41, PLAN-2.5a P49). GrainScheduler cannot re-time before
// its first onset (it fires on the next sample after reset), so Tempo mode is
// a plugin-local ABSOLUTE-TIME grid clock. Onset k of slot s_k:
//     t_k = A.abs + ((A.barOrigin + s_k·div) − A.ppq)/A.pps + J·(u_k − ½)·I,
//     I = div / A.pps,   u_0 := ½,   u_k keyed on epoch + k (as Free)
// emitted when t_k < pieceEnd at sample max (pieceStart, ceil (t_k), last + 1),
// frac = sample − t_k (0 when that is >= 1: a late re-anchor). t_k is recomputed
// every piece from the CURRENT anchor (A), so a tempo change re-times the
// pending onset exactly (no threshold). The slot index advances by 1 per onset
// and is derived from ppq — slot = ceil ((ppq_now − barOrigin)/div − 1e-9) —
// only at a re-lock (transport start, loop, locate), an enable / mode edge or a
// division change: never per tick (R40: the ceil flips on grid points that land
// on cell starts). A plain re-anchor (tempo change) keeps the slot: ppq is
// continuous there. Mode switches fold the onset count into the epoch (R4).
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
            tempo   = false;
            tIndex  = 0;
            needSlot = true;
        }

        /** Grid tick. Rising edge: fold the onset count into the epoch and restart
            the clock, so the first onset fires on the next processed sample (R4).
            Tempo: the first onset is the next grid slot >= now (R41). */
        void setEnabled (bool on) noexcept
        {
            if (on && ! enabled)
                restart();
            enabled = on;
        }

        /** Grid tick, before setEnabled: Free ↔ Tempo. A switch folds the onset
            count into the epoch exactly like the enable edge (draws never replay). */
        void setTempo (bool on) noexcept
        {
            if (on != tempo)
            {
                restart();
                tempo = on;
            }
        }

        bool isEnabled() const noexcept            { return enabled; }
        bool isTempo() const noexcept              { return tempo; }
        std::uint64_t getEpoch() const noexcept    { return epoch; }
        std::int64_t  getSlotNext() const noexcept { return slotNext; }

        /** Tempo grid for one piece (Engine, from the TempoTracker anchor). */
        struct TempoGrid
        {
            const TempoAnchor* anchor = nullptr;
            double        div         = 1.0;   // div_eff, quarters
            std::uint64_t relockEpoch = 0;
            bool          rederive    = false; // test hook ceilPerTick (R40 control): derive the slot now
        };

        /** Tempo-mode onsets over [pieceStart, pieceStart + n) (n <= kCell). */
        int processTempo (int n, std::int64_t pieceStart, int streamIndex, std::uint64_t seed, const TempoGrid& g,
                          float jitter, StreamOnset* out) noexcept
        {
            const TempoAnchor& A = *g.anchor;
            const std::uint64_t ep  = epoch;
            const std::uint64_t key = kSchedStream + static_cast<std::uint64_t> (streamIndex);
            const double jit = static_cast<double> (jitter);

            if (needSlot || g.rederive || g.relockEpoch != lockSeen || ! sameValue (g.div, divSeen))
            {
                const double ppqNow = A.ppq + static_cast<double> (pieceStart - A.abs) * A.pps;
                slotNext = static_cast<std::int64_t> (std::ceil ((ppqNow - A.barOrigin) / g.div - 1.0e-9));
                needSlot = false;
                lockSeen = g.relockEpoch;
                divSeen  = g.div;
            }

            const double pieceEnd = static_cast<double> (pieceStart + n);
            const double I = g.div / A.pps;
            int count = 0;
            while (count < kCell)
            {
                double t = static_cast<double> (A.abs)
                           + ((A.barOrigin + static_cast<double> (slotNext) * g.div) - A.ppq) / A.pps;
                if (jit > 0.0)
                {
                    const double u = tIndex == 0 ? 0.5
                                                 : static_cast<double> (ouaricon::grain::CounterRng::Draw (seed, key, ep + tIndex)
                                                                            .nextFloat (Slot::Jitter));
                    t += jit * (u - 0.5) * I;
                }
                if (! (t < pieceEnd))
                    break;
                std::int64_t smp = std::max (pieceStart, static_cast<std::int64_t> (std::ceil (t)));
                if (tIndex > 0)
                    smp = std::max (smp, lastEmit + 1);   // kMinInterval: at most one onset per sample
                if (smp >= pieceStart + n)
                    break;
                double frac = static_cast<double> (smp) - t;
                if (! (frac >= 0.0 && frac < 1.0))
                    frac = 0.0;
                out[count++] = { static_cast<int> (smp - pieceStart), frac, ep + tIndex, streamIndex };
                lastEmit = smp;
                ++tIndex;
                ++slotNext;
            }
            return count;
        }

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
        void restart() noexcept
        {
            epoch += tempo ? tIndex : sched.getOnsetCount();
            sched.reset();
            tIndex   = 0;
            needSlot = true;
        }

        ouaricon::grain::GrainScheduler sched;
        ouaricon::grain::Onset raw[kCell] {};
        std::uint64_t epoch = 0;
        bool enabled = false;
        // 2.5a Tempo clock
        bool          tempo    = false;
        bool          needSlot = true;
        std::uint64_t tIndex   = 0;       // Tempo onsets since the last edge (the draw index)
        std::int64_t  slotNext = 0;       // pending grid slot
        std::int64_t  lastEmit = 0;
        std::uint64_t lockSeen = 0;
        double        divSeen  = 0.0;
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
