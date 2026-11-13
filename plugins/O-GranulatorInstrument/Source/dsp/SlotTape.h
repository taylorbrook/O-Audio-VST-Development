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
#include <atomic>
#include <cstdint>
#include <vector>
#include "GiConstants.h"
#include "MipTape.h"

// ============================================================================
// SlotTape — one slot's working copy at a fixed tape rate (ARCHITECTURE §5–§7).
// Built and destroyed OFF the audio thread (loader worker / message thread).
// The audio thread only reads it through raw pointers taken from
// SlotHandoff::take(), and retires it through the SPSC ring (SlotHandoff.h).
//
// liveCount counts constructed-and-not-destroyed tapes (leak gates H2 / H5).
//
// PLAN-R2 PD5: the tape also carries the level reference of its PRISTINE
// content: the cumulative sum of L² + R² per kRefBin level-0 frames, built once
// in SlotLoader::buildTape (worker thread) and never written again. The record
// head reads the mean square of its latched loop from it in O(1); the working
// copy is never measured (anything read from the lane itself drifts).
// ============================================================================
namespace ogi
{
    struct SlotTape
    {
        MipTape<kTapeLevels> tape;
        double        rate       = 0.0;   // tape frames per second (== the session rate it was built for)
        int           frames     = 0;     // F (level-0 frames)
        int           slot       = 0;     // 0..3
        std::uint64_t generation = 0;     // loader generation of the SOURCE (a rebuild keeps it)
        bool          rebuild    = false; // same source re-rated (SR change): dual-read crossfade on take

        // PLAN-R2 PD5: pristine energy, refCum[b] = Σ (L² + R²) over frames [0, b·kRefBin).
        static constexpr int kRefBin = 256;
        std::vector<double> refCum;       // bins + 1 entries; empty = no reference
        int                 refFrames = 0;

        /** Off the audio thread (allocates). l / r: the pristine level-0 PCM. */
        void buildRef (const float* l, const float* r, int n)
        {
            const int bins = (std::max (n, 0) + kRefBin - 1) / kRefBin;
            refCum.assign (static_cast<std::size_t> (bins) + 1, 0.0);
            refFrames = std::max (n, 0);
            double acc = 0.0;
            for (int b = 0; b < bins; ++b)
            {
                const int e = std::min (n, (b + 1) * kRefBin);
                for (int i = b * kRefBin; i < e; ++i)
                    acc += static_cast<double> (l[i]) * l[i] + static_cast<double> (r[i]) * r[i];
                refCum[static_cast<std::size_t> (b) + 1] = acc;
            }
        }

        /** Mean square per sample and channel of the pristine over the bins
            that cover [lo, hi). O(1); 0 for an empty table. */
        double refMeanSq (std::int64_t lo, std::int64_t hi) const noexcept
        {
            const std::int64_t bins = static_cast<std::int64_t> (refCum.size()) - 1;
            if (bins < 1)
                return 0.0;
            const std::int64_t b0 = std::clamp<std::int64_t> (lo / kRefBin, 0, bins - 1);
            const std::int64_t b1 = std::clamp<std::int64_t> ((hi + kRefBin - 1) / kRefBin, b0 + 1, bins);
            const double n = static_cast<double> (std::min<std::int64_t> (b1 * kRefBin, refFrames) - b0 * kRefBin);
            return n > 0.0 ? (refCum[static_cast<std::size_t> (b1)] - refCum[static_cast<std::size_t> (b0)]) / (2.0 * n) : 0.0;
        }

        SlotTape() noexcept  { liveCount.fetch_add (1, std::memory_order_relaxed); }
        ~SlotTape()          { liveCount.fetch_sub (1, std::memory_order_relaxed); }
        SlotTape (const SlotTape&) = delete;
        SlotTape& operator= (const SlotTape&) = delete;

        inline static std::atomic<int> liveCount { 0 };
    };
} // namespace ogi
