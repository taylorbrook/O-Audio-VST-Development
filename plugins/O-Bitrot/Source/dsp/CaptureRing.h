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

    NEW versus CaptureBuffer: a fractional read (floor/lerp) for the
    variable-rate read heads. The i0+1 interpolation neighbour moves TOWARD
    the write head, so it is clamped to the last-written sample (>= 1-sample
    guard band below the write position). Exact-integer positions take a
    fast path that touches only i0 — this is what keeps the all-off
    passthrough bit-transparent (FUNC-02).

    prepare() is the ONLY allocation; clear() is alloc-free (audio-thread
    safe reset).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CaptureRing
{
public:
    // Ring span budget (FUNC-01 acceptance): max vinyl revolution (1.8 s @
    // 33 1/3) + max tape ramp (TAPE_RAMP max 500 ms) + safety margin.
    static constexpr double kRingSeconds          = 2.5;
    static constexpr double kMaxRevolutionSeconds = 1.8;
    static constexpr double kMaxTapeRampSeconds   = 0.5;
    static constexpr double kSafetySeconds        = 0.1;
    static_assert (kRingSeconds >= kMaxRevolutionSeconds + kMaxTapeRampSeconds + kSafetySeconds,
                   "Capture ring must span max vinyl revolution + max tape ramp + safety margin");

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

    // Fractional read at an absolute position. Linear interpolation with the
    // neighbour clamped to the last-written sample (guard band below the
    // write head). Exact-integer positions bypass the lerp entirely so the
    // rate-1.0 passthrough path stays bit-exact.
    float readFrac (int ch, double posAbs) const noexcept
    {
        const double f    = std::floor (posAbs);
        const auto   i0   = static_cast<juce::int64> (f);
        const double frac = posAbs - f;

        const float a = readAbs (ch, i0);

        if (frac <= 0.0)
            return a;                                       // exact-integer fast path

        const juce::int64 lastWritten = totalWritten - 1;
        const juce::int64 i1          = juce::jmin (i0 + 1, lastWritten);
        const float       b           = readAbs (ch, i1);

        return a + static_cast<float> (frac) * (b - a);
    }

    juce::int64 getTotalWritten() const noexcept { return totalWritten; }
    int         getSize() const noexcept         { return ringSize; }

private:
    juce::AudioBuffer<float> buffer;
    int         ringSize      = 0;
    int         writePosition = 0;
    juce::int64 totalWritten  = 0;
};
