/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - shared table read (Stage 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

    THE single reference read. The voice calls readSample() per sample; the
    Stage 3 visualizer and the 2.4 frozen-cycle capture call the same
    function, so "bars = heard cycle" is provable (dsp-check G-READ).

    Every clamp is NaN-safe: jlimit passes NaN through and (int) NaN is UB,
    so float clamps are written as !(x >= 0) -> 0. Frame indices are clamped
    to numFrames - 1 (a latched frame from a 256-frame bank must not overrun
    a 32-frame bank), the in-cycle index to 2047 (guard makes i0 + 1 safe).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

#include "WavetableBank.h"

namespace wt
{
    // D-A (user sign-off 2026-10-05): band-limited level floor = 1. At 96 kHz
    // notes <= MIDI 30 would otherwise select L0 (1023 harmonics, only 2x
    // table oversampling) and alias at -68 dB. Band-limit Off stays L = 0.
    constexpr int kMinBandLimitedLevel = 1;

    // Strict level: no harmonic of level L reaches fs/2. bandlimit Off -> 0
    // (raw, intended aliasing). L = clamp(ceil(log2(f * 2048 / fs)), 1, 10).
    inline int selectLevel (double hz, double fs, bool bandlimit) noexcept
    {
        if (! bandlimit)
            return 0;
        if (! (fs > 0.0))
            return WavetableBank::kLevels - 1;

        const double x = hz * (double) WavetableBank::kTableSize / fs;   // kmax_L * hz < fs/2  <=>  x < 2^L
        if (! (x > 0.0))
            return kMinBandLimitedLevel;
        if (! (x < 1024.0))
            return WavetableBank::kLevels - 1;

        const int level = (int) std::ceil (std::log2 (x));
        return juce::jlimit (kMinBandLimitedLevel, WavetableBank::kLevels - 1, level);
    }

    inline float clamp01 (float x) noexcept
    {
        if (! (x >= 0.0f)) return 0.0f;   // NaN -> 0
        if (x > 1.0f)      return 1.0f;
        return x;
    }

    // Guard sample makes fr[i0 + 1] safe for i0 <= 2047.
    inline float lerpCycle (const float* fr, int i0, float t) noexcept
    {
        const float a = fr[i0];
        return a + t * (fr[i0 + 1] - a);
    }

    // Frame index latched at note-on and at each phase wrap (Interp Off).
    inline int latchFrame (float pos01, int numFrames) noexcept
    {
        if (numFrames <= 1)
            return 0;
        const float p = clamp01 (pos01);
        const long f = std::lround (p * (float) (numFrames - 1));
        return f < 0 ? 0 : (f > (long) (numFrames - 1) ? numFrames - 1 : (int) f);
    }

    // Exactly the voice's per-sample read (Interp On: frame lerp; Off: latched frame).
    // phase in [0, 1). Returns 0 for an empty bank.
    inline float readSample (const WavetableBank& b, int level, double phase, bool interp,
                             float pos01, int latchedFrame) noexcept
    {
        const int nF = b.numFrames;
        if (nF <= 0)
            return 0.0f;

        level = level < 0 ? 0 : (level >= WavetableBank::kLevels ? WavetableBank::kLevels - 1 : level);

        double ph = phase;
        if (! (ph >= 0.0) || ! (ph < 1.0))
            ph = 0.0;

        const double idx = ph * (double) WavetableBank::kTableSize;
        int i0 = (int) idx;
        i0 = i0 < WavetableBank::kTableSize - 1 ? i0 : WavetableBank::kTableSize - 1;   // Prism IN-08
        const float t = (float) (idx - (double) i0);

        if (! interp)
        {
            const int f = latchedFrame < 0 ? 0 : (latchedFrame > nF - 1 ? nF - 1 : latchedFrame);
            return lerpCycle (b.frame (level, f), i0, t);
        }

        const float p  = clamp01 (pos01);
        const float fp = p * (float) (nF - 1);
        int f0 = (int) fp;
        f0 = f0 < nF - 1 ? f0 : nF - 1;
        const int f1 = f0 + 1 < nF ? f0 + 1 : nF - 1;
        const float a = lerpCycle (b.frame (level, f0), i0, t);
        const float c = lerpCycle (b.frame (level, f1), i0, t);
        return a + (fp - (float) f0) * (c - a);
    }
}
