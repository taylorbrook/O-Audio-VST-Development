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
#include <climits>
#include <cmath>
#include <cstdint>
#include "forked/EngineConstants.h"
#include "forked/TempoTracker.h"

// ============================================================================
// GiTempoClock — Phase 2.5a (PLAN-2.5a T12, PD1 / PD12; RESEARCH-2.5a §7).
//
// One per (voice, stream): a PORT of the parent StreamScheduler::processTempo
// (82406fce plugins/O-Granulator/Source/dsp/StreamScheduler.h:139-188).
//   grid point t = A.abs + (barOrigin + slot·div − A.ppq) / A.pps (absolute
//   samples, on the TempoTracker's held anchor), emitted at ceil (t) with
//   frac = smp − t: |onset − grid| < 1 sample by construction.
//   The slot is re-derived only at a restart, a re-lock, or a div change
//   (P12; hook ceilPerTick re-derives at every tick — the TS1n control).
//   Jitter: t += J·(u − ½)·I, u from the engine's telescoping draw keyed on
//   (serial << 24) | (epoch + tIndex) on kSchedStream + s (u_0 = ½).
// Instrument additions:
//   - PD1 (USER): after a note-on the grid starts strictly AFTER the note-on
//     sample (the burst onset owns that sample; floorSample = note-on + 1).
//   - div = the thinned division divBeats·2^(−e)·2^j (PD12), so thinning
//     drops only power-of-two onsets and every onset stays on the bar grid.
// ============================================================================
namespace ogi
{
    struct GiTempoOnset { int offset = 0; double frac = 0.0; std::uint64_t index = 0; };

    class GiTempoClock
    {
    public:
        /** Note-on / enable edge / Free ↔ Tempo: the next grid slot is derived
            at the next process(). floorSample >= 0: no grid onset at or before
            floorSample − 1 (the burst took the note-on sample). */
        void restart (std::int64_t floorSampleIn = -1) noexcept
        {
            needSlot = true;
            tIndex = 0;
            lastEmit = LLONG_MIN;
            floorSample = floorSampleIn;
        }

        std::uint64_t getOnsetCount() const noexcept { return tIndex; }
        std::int64_t  getSlotNext() const noexcept   { return slotNext; }

        /** Onsets over [pieceStart, pieceStart + n) (n <= kCell) into out
            (capacity >= kCell). u (k) = the jitter draw of onset k. */
        template <typename JitterU>
        int process (int n, std::int64_t pieceStart, const TempoAnchor& A, double div, std::uint64_t relockEpoch,
                     bool rederive, double jitter, JitterU&& u, GiTempoOnset* out) noexcept
        {
            if (! (div > 0.0) || ! (A.pps > 0.0))
                return 0;
            if (needSlot || rederive || relockEpoch != lockSeen || ! sameValue (div, divSeen))
            {
                const std::int64_t from = std::max (pieceStart, floorSample);
                const double ppqNow = A.ppq + static_cast<double> (from - A.abs) * A.pps;
                slotNext = static_cast<std::int64_t> (std::ceil ((ppqNow - A.barOrigin) / div - 1.0e-9));
                needSlot = false;
                lockSeen = relockEpoch;
                divSeen  = div;
            }

            const double pieceEnd = static_cast<double> (pieceStart + n);
            const double I = div / A.pps;
            int count = 0;
            while (count < kCell)
            {
                double t = static_cast<double> (A.abs)
                           + ((A.barOrigin + static_cast<double> (slotNext) * div) - A.ppq) / A.pps;
                if (jitter > 0.0)
                    t += jitter * (u (tIndex) - 0.5) * I;
                if (! (t < pieceEnd))
                    break;
                std::int64_t smp = std::max (pieceStart, static_cast<std::int64_t> (std::ceil (t)));
                if (tIndex > 0 && lastEmit != LLONG_MIN)
                    smp = std::max (smp, lastEmit + 1);   // at most one onset per sample
                if (floorSample >= 0)
                    smp = std::max (smp, floorSample);
                if (smp >= pieceStart + n)
                    break;
                double frac = static_cast<double> (smp) - t;
                if (! (frac >= 0.0 && frac < 1.0))
                    frac = 0.0;
                out[count++] = { static_cast<int> (smp - pieceStart), frac, tIndex };
                lastEmit = smp;
                ++tIndex;
                ++slotNext;
            }
            return count;
        }

    private:
        std::int64_t  slotNext = 0;
        std::uint64_t tIndex = 0;
        std::int64_t  lastEmit = LLONG_MIN;
        std::int64_t  floorSample = -1;
        bool          needSlot = true;
        std::uint64_t lockSeen = ~std::uint64_t (0);
        double        divSeen = -1.0;
    };
} // namespace ogi
