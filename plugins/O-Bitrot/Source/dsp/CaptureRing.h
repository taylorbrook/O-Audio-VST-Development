/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - CaptureRing (Stage 2, Phase 2.1)

    Stereo circular capture ring, continuously written at rate 1.0 by the
    plugin input. Absolute-index scheme borrowed from O-ReverseDelay's
    CaptureBuffer.h (write side: writePosition mod size + monotonic
    totalWritten; read side: double-mod wrap tolerant of negative indices —
    pre-history reads hit the cleared buffer and return zeros).

    NEW versus CaptureBuffer: a fractional read for the variable-rate read
    heads — 4-point Catmull-Rom as of v1.3.0 (was a 2-point lerp). The
    neighbours that move TOWARD the write head (i0+1, i0+2) are clamped to
    the last-written sample (guard band below the write position).
    Exact-integer positions take a fast path that touches only i0 — this is
    what keeps the all-off passthrough bit-transparent (FUNC-02).

    prepare() is the ONLY allocation; clear() is alloc-free (audio-thread
    safe reset).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CaptureRing
{
public:
    // Ring span budget (FUNC-01 acceptance). v1.3.0: 2.5 s -> 10 s.
    //
    // The old span capped every sustained loop. A locked groove re-jumps only
    // while lag + revolution <= maxLag - 50 ms (VinylTransport), and at 2.5 s
    // that inequality demands a NEGATIVE starting lag for a SECOND pass — so
    // the headline "locked groove loops with a pop per pass" self-released
    // after exactly one re-pass, and a sustained CD loop grew its lag by one
    // segment per pass until the ReadHead overflow clamp teleported the head.
    // 10 s buys >= kMinLockedGroovePasses re-passes at 33 1/3 RPM and ~99 CD
    // passes at the default 100 ms segment. Cost is one prepare()-time
    // allocation: stereo float, 3.8 MB @ 48 kHz (15.4 MB @ 192 kHz).
    static constexpr double kRingSeconds           = 10.0;
    static constexpr double kMaxRevolutionSeconds  = 1.8;   // 33 1/3 RPM
    static constexpr double kMaxTapeRampSeconds    = 0.5;   // TAPE_RAMP max
    static constexpr double kSafetySeconds         = 0.1;
    static constexpr int    kMinLockedGroovePasses = 4;

    // The assert must CONSTRAIN kRingSeconds, not merely be satisfied by it.
    // The pre-1.3.0 form (>= one revolution + ramp + safety) is equally true
    // of 2.5 s and of 10 s, so it could never have caught the one-pass
    // ceiling above. Stated instead as the budget a locked groove actually
    // spends: every re-pass ages the head by one full revolution, and the
    // head must still clear the tape ramp and the safety margin at the far
    // end of the ring.
    static_assert (kRingSeconds - kSafetySeconds
                       >= kMinLockedGroovePasses * kMaxRevolutionSeconds + kMaxTapeRampSeconds,
                   "Capture ring must span kMinLockedGroovePasses vinyl revolutions "
                   "+ max tape ramp + safety margin");

    // The only allocation. Call from prepareToPlay.
    void prepare (double sampleRate)
    {
        ringSize = static_cast<int> (sampleRate * kRingSeconds) + 1;
        buffer.setSize (2, ringSize);
        buffer.clear();
        writePosition = 0;
        totalWritten  = 0;
    }

    // Alloc-free zeroing — never calls setSize (audio-thread safe).
    void clear() noexcept
    {
        buffer.clear();
        writePosition = 0;
        totalWritten  = 0;
    }

    void push (float left, float right) noexcept
    {
        buffer.setSample (0, writePosition, left);
        buffer.setSample (1, writePosition, right);
        writePosition = (writePosition + 1) % ringSize;
        ++totalWritten;
    }

    // Integer read at an absolute (monotonic) sample index. Double-mod handles
    // negative indices — pre-history reads return the cleared buffer's zeros.
    float readAbs (int ch, juce::int64 absIndex) const noexcept
    {
        const int idx = static_cast<int> (((absIndex % ringSize) + ringSize) % ringSize);
        return buffer.getSample (ch, idx);
    }

    // Fractional read at an absolute position. 4-point Catmull-Rom as of
    // v1.3.0; this was a 2-point lerp, whose mid-sample response is
    // |cos(pi f / fs)| — about -16 dB at 0.9x Nyquist — so the tape family's
    // own bend table (rates 0.5-2.0) ran the whole melodic voice through it,
    // and the 2.0x bend aliased everything above fs/4 unfiltered.
    //
    // The two neighbours that move TOWARD the write head are clamped to the
    // last-written sample — the same guard band the lerp used, extended from
    // i0+1 to i0+2. The trailing neighbour i0-1 needs no clamp: every
    // position change routes through ReadHead's choke point, which holds the
    // head at least kSafetySeconds inside the oldest ring sample, and before
    // the ring has filled a pre-history index simply reads the cleared
    // buffer's zeros (see readAbs).
    //
    // Exact-integer positions bypass the interpolator entirely — unchanged,
    // and load-bearing: it is what keeps the all-off passthrough bit-exact
    // (FUNC-02).
    float readFrac (int ch, double posAbs) const noexcept
    {
        const double f    = std::floor (posAbs);
        const auto   i0   = static_cast<juce::int64> (f);
        const double frac = posAbs - f;

        const float a = readAbs (ch, i0);

        if (frac <= 0.0)
            return a;                                       // exact-integer fast path

        const juce::int64 lastWritten = totalWritten - 1;

        const float ym1 = readAbs (ch, i0 - 1);
        const float y1  = readAbs (ch, juce::jmin (i0 + 1, lastWritten));
        const float y2  = readAbs (ch, juce::jmin (i0 + 2, lastWritten));

        // Catmull-Rom in Horner form: exactly `a` at t = 0 and exactly `y1`
        // at t = 1, so the interpolated path stays continuous across every
        // integer boundary the head crosses.
        const auto  t  = static_cast<float> (frac);
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * a + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (a - y1);

        return ((c3 * t + c2) * t + c1) * t + a;
    }

    juce::int64 getTotalWritten() const noexcept { return totalWritten; }
    int         getSize() const noexcept         { return ringSize; }

private:
    juce::AudioBuffer<float> buffer;
    int         ringSize      = 0;
    int         writePosition = 0;
    juce::int64 totalWritten  = 0;
};
