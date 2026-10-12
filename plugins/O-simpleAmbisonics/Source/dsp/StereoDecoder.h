/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    StereoDecoder — ±60° virtual-cardioid pair on W / Y / X (DSP-04,
    ARCHITECTURE §Stereo Decoder). "Decoding = pointing virtual microphones
    into the soundfield."

      L = k · (0.5·W + 0.5·(cos 60°·X + sin 60°·Y))
      R = k · (0.5·W + 0.5·(cos 60°·X − sin 60°·Y))

    written with EXACT LITERALS 0.5f / 0.25f / 0.4330127f (A4, RESEARCH-2.1
    F3) — never 0.5f·std::cos(π/3), whose float value is 0.49999997 and would
    miss the harness P1 anchor (front / 1 m / order 1 → exactly 0.75·s[n−64])
    by one ulp. 0.5·W and 0.25·X are exact power-of-two scalings, so the
    front-source sum is the correctly rounded 0.75·s.

    Properties (F7): with (X, Y) = cos e·(cos a, sin a) the pattern is
    V = 0.5 + 0.5·cos e·cos(a ∓ 60°) ≥ 0.5·(1 − cos e) ≥ 0 — in-phase for every
    direction. +90° (left): L 0.933 / R 0.067 = 22.9 dB. e = ±90°: L = R = 0.5
    ("stereo has no height"). Front: L = R = 0.75. Z and every order ≥ 2
    channel are ignored — a coincident pair can only carry what two
    microphones hear; switch to Binaural for what orders 2 and 3 add.

    kStereoLevelMatch is a 1.0f PLACEHOLDER in 2.1. Phase 2.3's filter
    generator emits the real value (front source at order 3, equal A-weighted
    level in both decoder modes, ±1 dB) into BinauralFilterData.h, and this
    header then reads it from there.

    Headless: no includes. No JUCE.

  ==============================================================================
*/

#pragma once

namespace OSimpleAmbisonics::dsp
{

// 2.1 placeholder — see the header comment. 2.3 replaces this with the value
// generated into BinauralFilterData.h.
inline constexpr float kStereoLevelMatch = 1.0f;

struct StereoDecoder
{
    // bus16 is the (rotated) ACN/SN3D bus of one sample. Reads W (0), Y (1), X (3).
    static void decode (const float* bus16, float& outL, float& outR) noexcept
    {
        const float w = bus16[0];
        const float y = bus16[1];
        const float x = bus16[3];

        outL = kStereoLevelMatch * (0.5f * w + 0.25f * x + 0.4330127f * y);
        outR = kStereoLevelMatch * (0.5f * w + 0.25f * x - 0.4330127f * y);
    }
};

} // namespace OSimpleAmbisonics::dsp
