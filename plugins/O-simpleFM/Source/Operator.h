/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
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

    O-simpleFM - Operator (band-clean sine lookup)

    fastSine() is the single sine primitive for both the carrier and modulator
    operators (v1.0 is sine-only). It is backed by a 1024-point
    juce::dsp::LookupTableTransform (linear interp, ~97 dB SNR — cubic is
    inaudible at this depth).

    CRITICAL (ARCHITECTURE.md §"Sine Operator", risk #1): the phase MUST be
    floor-modulo wrapped into [0, 2*pi) BEFORE the lookup. The phase-modulation
    argument (carPhase + I*modOut) swings to many multiples of 2*pi at high
    index, and LookupTableTransform CLAMPS inputs outside its initialised range
    rather than wrapping — without the wrap, high-index tones flat-line.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace OSimpleFM
{
    // Band-clean sine via a shared 1024-point lookup table. The table is a
    // function-local static (C++11 thread-safe init), built once; std::sin is
    // used ONLY for the one-time fill.
    inline float fastSine (float phase) noexcept
    {
        struct SineTable
        {
            juce::dsp::LookupTableTransform<float> t;
            SineTable()
            {
                t.initialise ([] (float x) { return std::sin (x); },
                              0.0f, juce::MathConstants<float>::twoPi, 1024);
            }
        };
        static const SineTable table;

        if (! std::isfinite (phase)) return 0.0f;       // self-defensive: floor(NaN)=NaN -> LUT UB
        constexpr float twoPi = juce::MathConstants<float>::twoPi;
        phase -= twoPi * std::floor (phase / twoPi);   // MANDATORY wrap (LUT clamps, not wraps)
        return table.t (phase);
    }
}
