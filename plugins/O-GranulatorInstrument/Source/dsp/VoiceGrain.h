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

#include <cstdint>
#include <type_traits>
#include "GrainPool.h"          // grain-engine 1.1.0
#include "GiConstants.h"
#include "forked/Grain.h"

// ============================================================================
// VoiceGrain — the instrument's pool POD (RESEARCH-2.1 §2.3, PLAN-2.1 T8).
// The forked Grain stays VERBATIM; the voice dimension wraps it. GrainPool
// value-initialises a slot on activate and COPIES the whole POD into a fade
// slot on a native steal, so the tape pointers travel with a fading grain —
// which is why retirement scans all 288 slots (P12).
// ============================================================================
namespace ogi
{
    struct SlotTape;

    struct VoiceGrain
    {
        Grain g;                              // reader latch, read position, window, kill fade
        int           voice  = 0;             // voice slot 0..19 (bus index only; NEVER a random key)
        std::uint64_t serial = 0;             // note serial (random key, logs)
        const SlotTape* tape = nullptr;       // latched at spawn
        int           slot   = 0;             // tape slot 0..3 (= stream index in 2.1)

        // Region wrap: a grain whose reach rate·len >= W wraps inside the loop
        // region (ARCHITECTURE §10). wrapW == 0 → no wrap.
        double wrapLo = 0.0, wrapW = 0.0;

        // SR-rebuild dual-read crossfade (ARCHITECTURE §6): while xTape != nullptr
        // the grain also reads the OLD tape at its old coordinates and blends.
        const SlotTape* xTape = nullptr;
        double xReadAbs = 0.0, xStep = 0.0;
        double xWrapLo = 0.0, xWrapW = 0.0;
        int    xLevel = 0, xSet = 0;
    };

    static_assert (std::is_trivially_copyable_v<VoiceGrain>,
                   "GrainPool copies grains into fade slots — VoiceGrain must stay a trivially copyable POD");

    using GiPool = ouaricon::grain::GrainPool<VoiceGrain, kPoolLive, kPoolFade>;
    static_assert (GiPool::kNumSlots == 288, "256 live + 32 fade slots");
} // namespace ogi
