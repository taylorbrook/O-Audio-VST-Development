/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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
/*
  ==============================================================================

    O-Emulator — CrushCurve (Stage 2, Task 6 — SNES row only)

    Per-console macro mapping for the `crush` parameter (ARCHITECTURE "Crush
    Mapping"). Phase 2.1 ships the SNES row:

        drive:       0 % -> 0 dB, 100 % -> +12 dB into the codec domain
        shift floor: 0 for crush <= 50 %, rising to 8 at 100 % (coarser
                     quantisation — the encoder search is restricted upward)

    At crush = 0 the signal STILL passes the full BRR round trip (subtle
    color, never bypass). Later phases add the PS1/NES/GB/Genesis rows and
    the crush >= 80 % AA-open behaviour (precomputed coefficient sets),
    plus the 5 ms integer-step micro-fades (Phase 2.4, plan decision #4).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace oemu
{

struct CrushCurve
{
    /** SNES: linear 0..+12 dB drive into the encoder. */
    static float snesDriveDb (float crushPct) noexcept
    {
        return 0.12f * juce::jlimit (0.0f, 100.0f, crushPct);
    }

    /** SNES: shift floor 0 below 50 %, then 0 -> 8 across 50..100 %.
        Integer steps by design — control-trajectory smoothing (5 ms
        micro-fades) lands in Phase 2.4. */
    static int snesShiftFloor (float crushPct) noexcept
    {
        const float c = juce::jlimit (0.0f, 100.0f, crushPct);

        if (c <= 50.0f)
            return 0;

        return (int) std::lround ((double) (c - 50.0f) * 0.16);
    }

    // ── PS1 row (Phase 2.2) ──────────────────────────────────────────────────
    // ARCHITECTURE gives SNES and PS1 the SAME crush semantics ("SNES/PS1:
    // ... shift-floor that rises with crush" + the shared 0..+12 dB drive), so
    // the PS1 row delegates to the ADPCM curve above. Kept as named rows so
    // Phase 2.4's per-console tuning can split them without touching callers.

    static float ps1DriveDb (float crushPct) noexcept    { return snesDriveDb (crushPct); }
    static int   ps1ShiftFloor (float crushPct) noexcept { return snesShiftFloor (crushPct); }
};

} // namespace oemu
