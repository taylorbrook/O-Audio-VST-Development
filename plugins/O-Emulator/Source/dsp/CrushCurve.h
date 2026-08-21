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

    // ── NES row (Phase 2.3) ──────────────────────────────────────────────────
    // Crush walks DOWN the 16-entry NTSC DPCM timer table: index 15
    // (33.1 kHz) at 0 % to index 0 (~4.2 kHz) at 100 % (ARCHITECTURE "Crush
    // Mapping"). Integer steps by design; micro-fades land in Phase 2.4.

    static float nesDriveDb (float crushPct) noexcept { return snesDriveDb (crushPct); }

    static int nesRateIndex (float crushPct) noexcept
    {
        const float c = juce::jlimit (0.0f, 100.0f, crushPct);
        return 15 - (int) std::lround ((double) c * 0.15);
    }

    // ── Game Boy row (Phase 2.3) ─────────────────────────────────────────────
    // Crush reduces the wave-channel level count 16 -> 8 -> 4 at the top of
    // the range (ARCHITECTURE: "GB: crush reduces effective wave steps").

    static float gbDriveDb (float crushPct) noexcept { return snesDriveDb (crushPct); }

    static int gbLevels (float crushPct) noexcept
    {
        const float c = juce::jlimit (0.0f, 100.0f, crushPct);
        return c < 70.0f ? 16 : (c < 90.0f ? 8 : 4);
    }

    // ── Genesis row (Phase 2.3) ──────────────────────────────────────────────
    // Crush lowers the effective DAC update rate 26.3 kHz -> 8 kHz linearly
    // (ARCHITECTURE: mirrors real Z80-driven playback rates).

    static float genesisDriveDb (float crushPct) noexcept { return snesDriveDb (crushPct); }

    static double genesisUpdateRateHz (float crushPct) noexcept
    {
        const double c = (double) juce::jlimit (0.0f, 100.0f, crushPct);
        return 26320.0 - c * 183.20;   // 100 % -> 8000 Hz
    }

    // ── Console dispatch (indices per the ConsoleSpec table order) ───────────
    static float driveDbFor (int consoleIndex, float crushPct) noexcept
    {
        switch (consoleIndex)
        {
            case 1:  return ps1DriveDb (crushPct);
            case 2:  return nesDriveDb (crushPct);
            case 3:  return gbDriveDb (crushPct);
            case 4:  return genesisDriveDb (crushPct);
            default: return snesDriveDb (crushPct);
        }
    }
};

} // namespace oemu
