/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include "DelayBuffer.h"
#include "LagrangeInterpolation.h"
#include <array>
#include <cmath>

// v3.0.4 freeze. A held snapshot of the delay line, played by grains as if the
// write head were still running. Each slot keeps a virtual write head that keeps
// advancing while freeze is held; a grain latches base = head - delay at spawn and
// reads base + readPosition, wrapped round the captured loop. Frozen grains then
// stream through the held audio exactly as live grains stream through the delay
// line, so Scan and Spread work and the output is the captured loop repeating.
// (v3.0.3 started every frozen grain at snapshot index 0, so each grain replayed
// the same samples: a buzz at the spawn rate.)
//
// Two slots: a grain latches its slot at spawn and finishes on it after release,
// so release needs no timer and nothing switches source mid-grain. The grain
// envelopes are the crossfade in both directions.
class FreezeManager
{
public:
    static constexpr int kNumSlots = 2;

    void prepare (double newSampleRate, int maxLengthSamples)
    {
        for (auto& s : slots)
        {
            s.buffer.setSize (2, maxLengthSamples);
            s.buffer.clear();
            s.length = s.loopLength = s.xfade = 0;
            s.head = 0.0;
        }
        maxXfade = juce::jmax (1, static_cast<int> (newSampleRate * 0.010));  // 10 ms seam crossfade
        active = false;
        currentSlot = 0;
    }

    // Alloc-free state reset for reset() — zeroes the (already-sized) buffers and clears
    // freeze state without setSize(). CR-02: keeps reset() off the allocator.
    void clear()
    {
        for (auto& s : slots)
        {
            s.buffer.clear();
            s.length = s.loopLength = s.xfade = 0;
            s.head = 0.0;
        }
        active = false;
        currentSlot = 0;
    }

    // Capture the most recent audio into `slot`. The loop length is latched here to
    // what the delay line has actually been written with, capped at the slot size, so
    // a freeze pressed right after load doesn't loop through never-written silence.
    void engage (const DelayBuffer& delayBuf, int slot)
    {
        auto& s = slots[static_cast<size_t> (slot)];
        const int len = juce::jmin (s.buffer.getNumSamples(), delayBuf.getSamplesWritten());
        delayBuf.copyRegion (s.buffer, len, len);
        s.length     = len;
        s.xfade      = juce::jmin (maxXfade, len / 4);
        s.loopLength = len - s.xfade;
        s.head       = static_cast<double> (len);   // one past the newest captured sample
        currentSlot  = slot;
        active       = true;
    }

    // New grains go back to the live delay line. Grains already on the snapshot keep it.
    void release() { active = false; }

    bool isActive() const { return active; }
    int getCurrentSlot() const { return currentSlot; }

    // Per sample, after the delay-line push: the held slot's virtual head moves on.
    // Kept bounded by stepping back one loop length (reads wrap modulo the loop anyway).
    void advance()
    {
        if (! active) return;
        auto& s = slots[static_cast<size_t> (currentSlot)];
        s.head += 1.0;
        if (s.loopLength > 0 && s.head >= static_cast<double> (s.length + s.loopLength))
            s.head -= static_cast<double> (s.loopLength);
    }

    // Snapshot position a grain spawned now at `delaySamples` starts from.
    double getSpawnBase (double delaySamples) const
    {
        return slots[static_cast<size_t> (currentSlot)].head - delaySamples;
    }

    // The captured audio is a loop over [xfade, length). Its last xfade samples are
    // blended equal-power into the xfade samples before the loop start, so the seam
    // where the newest captured sample meets the oldest is not a splice.
    float readSample (int slot, int channel, double position) const
    {
        const auto& s = slots[static_cast<size_t> (slot)];
        if (s.loopLength <= 0) return 0.0f;

        const double x  = static_cast<double> (s.xfade);
        const double lp = static_cast<double> (s.loopLength);
        double m = std::fmod (position - x, lp);
        if (m < 0.0) m += lp;
        const double b = x + m;   // [xfade, length)

        auto* data = s.buffer.getReadPointer (channel);
        const double seamStart = static_cast<double> (s.length) - x;

        if (s.xfade > 0 && b >= seamStart)
        {
            const float fade = static_cast<float> ((b - seamStart) / x);
            const float gOut = std::cos (fade * juce::MathConstants<float>::halfPi);
            const float gIn  = std::sin (fade * juce::MathConstants<float>::halfPi);
            return gOut * readAt (data, s.length, b) + gIn * readAt (data, s.length, b - lp);
        }

        return readAt (data, s.length, b);
    }

private:
    struct Slot
    {
        juce::AudioBuffer<float> buffer;
        int length = 0;       // captured samples
        int loopLength = 0;   // length - xfade
        int xfade = 0;        // seam crossfade samples
        double head = 0.0;    // virtual write head (snapshot coordinates)
    };

    // Lagrange 3rd-order read with the kernel clamped to the captured span.
    static float readAt (const float* data, int length, double pos)
    {
        int i0 = static_cast<int> (pos);
        const float frac = static_cast<float> (pos - static_cast<double> (i0));
        i0 = juce::jlimit (0, length - 1, i0);
        const int im1 = juce::jmax (0, i0 - 1);
        const int i1  = juce::jmin (length - 1, i0 + 1);
        const int i2  = juce::jmin (length - 1, i0 + 2);
        return lagrangeInterpolate (data[im1], data[i0], data[i1], data[i2], frac);
    }

    std::array<Slot, kNumSlots> slots;
    int maxXfade = 441;
    int currentSlot = 0;
    bool active = false;
};
