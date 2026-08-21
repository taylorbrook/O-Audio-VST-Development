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

    O-Emulator — DpcmCodec (Stage 2, Task 12)

    NES 2A03 DPCM: a 1-bit delta stream driving a 7-bit counter. The encoder
    IS the decoder (ARCHITECTURE "DPCM Codec"): emit bit = (target > counter),
    counter ±2, clamp 0–126 — the counter output is the round trip itself.
    Max slope 2 steps per timer tick IS the sound: bass tracks, bright
    material slew-distorts, rails flat-top.

    Crush walks the 16-entry NTSC timer-rate table (33.1 kHz down to
    ~4.2 kHz) via a fractional step accumulator INSIDE the fixed 33144 Hz
    console domain — the counter only steps at the effective timer rate and
    holds between steps (the hold is the authentic sample-and-hold grit).

    Counter starts at midpoint 64 (documented deviation: hardware $4011
    resets to 0, but a −1.0 onset ramp on every engine reset would thump
    through the console crossfade; the midpoint start is silent-in/silent-out
    while preserving every in-signal DPCM behavior). Signal-dependent DC from
    slew asymmetry is real and removed structurally by the output stage's
    10 Hz blocker (verified at the mixer boundary by probe M4).

    GPL hygiene: rate table + state machine entered from the NESdev wiki
    (2A03 DPCM) — hardware constants, no GPL code.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace oemu
{

class DpcmCodec
{
public:
    /** NTSC DPCM timer rates, Hz — NESdev 2A03 table, ascending (index 15 is
        the $0 fastest rate the console domain runs at). */
    static constexpr double kNtscRatesHz[16] = {
        4181.7,  4709.9,  5264.0,  5593.0,  6257.9,  7046.4,  7919.3,  8363.4,
        9419.9, 11186.1, 12604.3, 13982.6, 16884.7, 21306.8, 26843.9, 33143.9
    };

    void prepare (double consoleRate)
    {
        domainRate = consoleRate;
        setRateIndex (15);
        reset();
    }

    void reset() noexcept
    {
        counter = 64;   // midpoint start — see header
        acc = 0.0;
    }

    /** Crush's table walk: 15 (full rate) down to 0 (~4.2 kHz). */
    void setRateIndex (int idx) noexcept
    {
        step = kNtscRatesHz[(size_t) juce::jlimit (0, 15, idx)] / domainRate;
    }

    float processSample (float x) noexcept
    {
        acc += step;   // step <= 1 by construction: at most one tick per sample
        if (acc >= 1.0)
        {
            acc -= 1.0;

            // ±1.0 float -> 0..127 unipolar target (the NES's DAC domain).
            const int target = juce::jlimit (0, 127, (int) std::lrint ((x + 1.0f) * 64.0f));

            counter += (target > counter) ? 2 : -2;
            counter = juce::jlimit (0, 126, counter);
        }

        // Held between ticks; unipolar -> bipolar (spec: counter/64 − 1); the
        // residual DC is the output stage DC blocker's job.
        return (float) counter * (1.0f / 64.0f) - 1.0f;
    }

private:
    double domainRate = 33144.0;
    double step = 1.0;
    double acc = 0.0;
    int counter = 64;
};

} // namespace oemu
