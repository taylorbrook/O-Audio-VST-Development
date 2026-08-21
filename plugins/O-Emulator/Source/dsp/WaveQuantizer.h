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

    O-Emulator — WaveQuantizer (Stage 2, Task 12)

    Game Boy wave-channel quantization: 4-bit mid-tread over ±1.0 with a
    FIXED per-level DAC error table (±0.5 LSB pseudo-random level offsets —
    the non-ideal GB DAC ladder, ARCHITECTURE "Wave Quantizer"). The
    zero-order hold lives in the resampler's ZOH upsample mode, not here.

    Crush reduces the level count 16 -> 8 -> 4 at the top of the range
    (CrushCurve GB row); reduced-level modes index the FIRST `levels` entries
    of the same error table.

    The error values are authored constants shaped to the spec's ±0.5 LSB
    bound (unit GB DACs vary anyway — there is no canonical table to
    transcribe). Stateless — reset() exists for interface symmetry.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

namespace oemu
{

class WaveQuantizer
{
public:
    void reset() noexcept {}

    /** 16 / 8 / 4 (CrushCurve GB row); anything else snaps down. */
    void setLevels (int lv) noexcept
    {
        levels = lv >= 16 ? 16 : (lv >= 8 ? 8 : 4);
        half   = levels / 2;
        scale  = (float) half - 0.5f;
    }

    float processSample (float x) noexcept
    {
        const int q = juce::jlimit (-half, half - 1, (int) std::lrint (x * scale));
        return ((float) q + kDacErrorLsb[(size_t) (q + half)]) / scale;
    }

private:
    /** Fixed per-level DAC error, LSB units, |e| <= 0.5 (spec bound). */
    static constexpr float kDacErrorLsb[16] = {
        +0.21f, -0.34f, +0.08f, +0.42f, -0.17f, +0.29f, -0.45f, +0.03f,
        -0.26f, +0.38f, -0.09f, +0.15f, -0.41f, +0.24f, -0.12f, +0.33f
    };

    int levels = 16;
    int half = 8;
    float scale = 7.5f;
};

} // namespace oemu
