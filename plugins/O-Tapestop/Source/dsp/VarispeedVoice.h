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

    O-Tapestop - VarispeedVoice

    POD per the ReverseGrain discipline (Stage-2 PLAN Task 4 / RESEARCH §2.3):
    everything the voice needs is latched at spawn; speed `r` lives in the
    TRANSPORT (exactly one voice is driven at a time), not in the voice.
    `readAbsFrac` is DOUBLE — an 8 s ramp at 192 kHz is 1.5 M accumulation
    steps and float drift there is both audible and a bit-identity hazard.

    Read paths, in priority order (RESEARCH §3.1):

      1. INTEGER read when the fractional part is exactly 0 — this is what
         carries the DSP-03 bitwise null (live riding / post-resync,
         d == 0, r == 1). NEVER routed through the interpolator: bitwise-exact
         by construction, and it keeps the i+1/i+2 taps off not-yet-written
         samples.

      2. Hand-rolled Catmull-Rom (Horner form) over readAbs(ch, i-1 .. i+2),
         taken only with debt d >= kInterpGuard = 4, which keeps all four taps
         behind the write head. (juce::Interpolators::CatmullRom is a STREAM
         resampler — wrong shape for random-access varying-rate ring reads.)

      3. Onset zone 0 < d < kInterpGuard: 2-tap linear interpolation between
         floor(pos) and min(floor(pos)+1, live). Both taps are provably
         written (pos <= live), the path is continuous with (2) as the debt
         crosses the guard, and — unlike clamping the debt up to kInterpGuard —
         it does not content-jump the playhead backwards at the engage edge,
         where debt grows from 0 fractionally.

    Release-build debt clamp: the EFFECTIVE read position is clamped to
    [live - (ringSpan - kInterpGuard), live]. The upper-bound (stale-position)
    clamp is SPECIFIED behavior for Stopped-hold resume paths (CONTEXT
    decision 1), so it does not assert; the stored readAbsFrac is additionally
    clamped once at SpinUp entry by the transport.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <cmath>

#include "CaptureBuffer.h"

struct VarispeedVoice
{
    static constexpr int kInterpGuard = 4;

    double readAbsFrac = 0.0;   // absolute fractional read position vs CaptureBuffer::totalWritten

    /** Standard Catmull-Rom kernel in Horner form. At f == 0 this returns
        bitwise y1 (every term multiplies by 0.0f) — but the null path never
        relies on that: it takes the integer branch in read() instead. */
    static float catmullRom (float y0, float y1, float y2, float y3, float f) noexcept
    {
        const float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        const float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float a2 = 0.5f * (y2 - y0);
        return ((a0 * f + a1) * f + a2) * f + y1;
    }

    float read (const CaptureBuffer& ring, int ch) const noexcept
    {
        // `live` is the most recently written absolute index (per-sample
        // write-then-read order guarantees it exists for this sample).
        const juce::int64 live = ring.getTotalWritten() - 1;

        // Release-build debt clamp [kInterpGuard, ringSpan - kInterpGuard] on
        // the EFFECTIVE position. The lower-position (stale) clamp is the
        // specified Stopped-hold-overrun behavior — no assert. The upper
        // clamp (never past the write head) backstops r > 1 at d ~ 0.
        const double maxDebt = (double) ring.getBufferSize()
                             - (double) kInterpGuard;

        double pos = readAbsFrac;
        pos = juce::jmax (pos, (double) live - maxDebt);
        pos = juce::jmin (pos, (double) live);

        const auto   i = (juce::int64) std::floor (pos);
        const double f = pos - (double) i;

        // Integer fast path — carries the DSP-03 bitwise null. f is
        // pos − floor(pos), so f >= 0 always; `<= 0` is the equality test
        // without tripping -Wfloat-equal.
        if (f <= 0.0)
            return ring.readAbs (ch, i);

        const double debt = (double) live - pos;

        if (debt >= (double) kInterpGuard)
        {
            const float y0 = ring.readAbs (ch, i - 1);
            const float y1 = ring.readAbs (ch, i);
            const float y2 = ring.readAbs (ch, i + 1);
            const float y3 = ring.readAbs (ch, i + 2);
            return catmullRom (y0, y1, y2, y3, (float) f);
        }

        // Onset zone (0 < debt < kInterpGuard): 2-tap linear, both taps
        // provably behind the write head.
        const float y1 = ring.readAbs (ch, i);
        const float y2 = ring.readAbs (ch, juce::jmin (i + 1, live));
        return y1 + (float) f * (y2 - y1);
    }
};
