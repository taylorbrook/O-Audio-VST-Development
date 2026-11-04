/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#include "CounterRng.h"   // grain-engine 1.1.0 (modules/synthesis/grain-engine/cpp)

// v3.1.0: counter-based RNG. Every draw is a pure function of (seed, stream, event
// counter, slot), so the value a grain or a trigger gets can't drift with block size,
// and a conditional draw (Size Random at 0, spatial off, not frozen) can't shift the
// values any other draw gets. The seed is saved with the session.
//
// v3.5.1: the draw functions are the grain-engine module's (ouaricon::grain::CounterRng),
// which this plugin's copy was the origin of; the core is the same code, so every draw
// is bit-identical. Only the plugin's streams and slots stay here. The namespace keeps
// the name CounterRng so the call sites, and the test drivers that also compile against
// older trees, are unchanged.
namespace CounterRng
{
    using ouaricon::grain::CounterRng::mix;
    using ouaricon::grain::CounterRng::hash;
    using ouaricon::grain::CounterRng::uniform;
    using ouaricon::grain::CounterRng::uniformInt;
    using ouaricon::grain::CounterRng::Draw;

    // Streams keep the scheduler's and the grains' draws independent of each other.
    enum Stream : uint64_t
    {
        SchedulerFree = 0x5C4ED0000001ull,
        SchedulerSync = 0x5C4ED0000002ull,
        Grain         = 0x6EA1A0000003ull
    };

    // Fixed per-quantity slots for a grain's draws.
    enum GrainSlot : uint32_t
    {
        SlotSize = 1, SlotAmp, SlotSpread, SlotPitch, SlotPan, SlotReverse,
        SlotAzimuth, SlotElevation, SlotDetune,
        SlotSkip    // v3.4.0: drawn only while Grain Skip > 0 (appended: no other slot moves)
    };
}
