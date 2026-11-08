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

#include <atomic>
#include <cstdint>
#include "GiConstants.h"
#include "MipTape.h"

// ============================================================================
// SlotTape — one slot's working copy at a fixed tape rate (ARCHITECTURE §5–§7).
// Built and destroyed OFF the audio thread (loader worker / message thread).
// The audio thread only reads it through raw pointers taken from
// SlotHandoff::take(), and retires it through the SPSC ring (SlotHandoff.h).
//
// liveCount counts constructed-and-not-destroyed tapes (leak gates H2 / H5).
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

        SlotTape() noexcept  { liveCount.fetch_add (1, std::memory_order_relaxed); }
        ~SlotTape()          { liveCount.fetch_sub (1, std::memory_order_relaxed); }
        SlotTape (const SlotTape&) = delete;
        SlotTape& operator= (const SlotTape&) = delete;

        inline static std::atomic<int> liveCount { 0 };
    };
} // namespace ogi
