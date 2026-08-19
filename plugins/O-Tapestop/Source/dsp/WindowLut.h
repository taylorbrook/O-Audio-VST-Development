/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop - WindowLut (Hann-only)

    ~40-line reduction of O-ReverseDelay's WindowLut.h (Stage-2 PLAN Task 3 /
    RESEARCH §2.2): the Hann table build + readAt (clamp, lookup, lerp) and
    nothing else. The shapes / tilt / Tukey-taper / normalisation machinery in
    the parent file serves overlapping GRAIN populations — none of it applies
    to a deterministic 2-voice crossfade.

    Crossfade gains per ARCHITECTURE (consumed from Phase 2.2's ResyncXfade):

        fadeOut = hann(0.5 + phi/2) = cos^2(pi*phi/2)   // monotonic falling half
        fadeIn  = hann(phi/2)       = sin^2(pi*phi/2)   // monotonic rising half

    This is a raised-cosine EQUAL-GAIN law, not equal-power — `SpliceLaw::
    EqualPower` is a misnomer kept for compatibility. sin^2 + cos^2 = 1 holds
    over the GAINS themselves, so what is constant across the fade is the
    AMPLITUDE sum, not the power sum:

        correlated:   (fadeOut + fadeIn)^2       = 1                  -> 0 dB, flat
        decorrelated: fadeOut^2 + fadeIn^2       = (1 + cos^2(pi*phi))/2
                                                 -> 0.5 at phi = 0.5, a 3.01 dB dip

    So this law is exact on CORRELATED material and dips on decorrelated
    material. True equal-power is the square root of these gains —
    sin(pi*phi/2) / cos(pi*phi/2) — which flattens the decorrelated case and
    instead over-sums correlated material by +3.01 dB at the midpoint. Neither
    law is flat for both; the choice is which material the splice actually
    sees. O-Tapestop's resync crossfade splices the live-head rider against a
    voice seconds behind it, so the decorrelated case is the one that applies.

    The 3.01 dB above is the law's analytic floor and assumes two EQUAL-power
    decorrelated sources. The real splice is worse, because the fading voice is
    a varispeed read of different material at a different level: harness probe
    `AB-splice-equal-power` measures dip = -6.21 dB (and bump = -0.48 dB, i.e.
    no correlated over-sum, as predicted). `AB-splice-linear` measures -6.99 dB
    — a deeper dip, consistent with the linear law's power sum falling off
    faster away from the midpoint (0.625 vs 0.750 at phi = 0.25) even though
    both laws share the same 0.5 floor exactly at phi = 0.5. Do not quote 3 dB
    as the plugin's resync dip; quote the measured figure.
    See improvements/2026-08-16-audit-queue.md item 4 (B2b).

    No normalisation constants: exactly two voices, deterministic gains, no
    population statistics.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <cmath>

class WindowLut
{
public:
    static constexpr int kSize = 2048;

    WindowLut() noexcept
    {
        // Bit-for-bit the O-ReverseDelay v1.0.0 Hann expression.
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < kSize; ++i)
        {
            const float phi = static_cast<float> (i) / static_cast<float> (kSize - 1);
            table[(size_t) i] = 0.5f * (1.0f - std::cos (twoPi * phi));
        }
    }

    /** One clamp + one table lookup + one lerp. No transcendental. */
    float readAt (float phase) const noexcept
    {
        const float p    = juce::jlimit (0.0f, 1.0f, phase);
        const float fpos = p * static_cast<float> (kSize - 1);
        const int   i0   = static_cast<int> (fpos);
        const int   i1   = juce::jmin (i0 + 1, kSize - 1);
        const float frac = fpos - static_cast<float> (i0);

        return table[(size_t) i0] + frac * (table[(size_t) i1] - table[(size_t) i0]);
    }

private:
    std::array<float, kSize> table {};
};
