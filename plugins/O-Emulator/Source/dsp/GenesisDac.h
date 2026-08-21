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

    O-Emulator — GenesisDac (Stage 2, Task 12)

    YM2612 DAC channel: 8-bit mid-tread quantization + the LADDER EFFECT per
    the Nuked-OPN2-validated model (ARCHITECTURE "Ladder Effect"): after
    quantization, `q >= 0 ? q + 1 : q` — the voltage gap between −1 and 0 is
    twice the linear step. The distortion is level-dependent: inaudible loud,
    gritty on quiet material and fades.

    Crush lowers the effective DAC update rate (26.3 kHz -> ~8 kHz) via a
    fractional sample-hold accumulator inside the fixed 26320 Hz domain,
    mirroring real Z80-driven playback rates.

    GPL hygiene: the ladder model is implemented from its published
    DESCRIPTION (jsgroth "Emulating the YM2612 Part 5" + the Nuked-OPN2
    finding as documented there); no Nuked-OPN2 code consulted or ported.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace oemu
{

class GenesisDac
{
public:
    void prepare (double consoleRate)
    {
        domainRate = consoleRate;
        setUpdateRateHz (consoleRate);
        reset();
    }

    void reset() noexcept
    {
        acc = 0.0;
        held = 0.0f;
    }

    /** Crush's rate-hold: effective DAC update rate, clamped to the domain. */
    void setUpdateRateHz (double hz) noexcept
    {
        step = juce::jlimit (0.01, 1.0, hz / domainRate);
    }

    float processSample (float x) noexcept
    {
        acc += step;   // step <= 1: at most one update per domain sample
        if (acc >= 1.0)
        {
            acc -= 1.0;

            int q = juce::jlimit (-128, 127, (int) std::lrint (x * 127.5f));
            q = q >= 0 ? q + 1 : q;   // ladder: −1→0 gap doubled

            // May reach 128/127.5 ≈ 1.004 — the output stage clips the rails.
            held = (float) q / 127.5f;
        }

        return held;   // zero-order hold between updates (the rate-crush grit)
    }

private:
    double domainRate = 26320.0;
    double step = 1.0;
    double acc = 0.0;
    float held = 0.0f;
};

} // namespace oemu
