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
#include "BandlimitedReader.h"   // grain-engine 1.1.0 (modules/synthesis/grain-engine/cpp)

// v3.3.0: the 12 s ring is a grain-engine MipRing. Every pushed sample also runs a
// half-band cascade that keeps the ring at 1/2, 1/4 and 1/8 rate (levels 1..3), and
// grains read through a 16-tap polyphase sinc instead of the v3.2.0 4-point polynomial. A grain latches
// its octave level at spawn from its rate (+12 st and up read a decimated level) and
// the sinc set from the residual rate on that level, so pitched-up grains no longer
// alias (v3.2.0: aliases at full level from +12 st up).
//
// Delays are level-0 samples behind the write head (delay 1 = the newest sample), as
// before. A level-l read needs margin(l) = 9 / 80 / 222 / 506 samples of settled
// history; the reader clamps every read to it.
class DelayBuffer
{
public:
    using Ring = ouaricon::grain::MipRing<3>;
    static constexpr int kMaxLevel = 3;

    static constexpr int margin (int level) { return Ring::margin (level); }

    void prepare (double sampleRate, float maxDelaySec = 2.0f)
    {
        bufferSize = static_cast<int> (sampleRate * maxDelaySec) + 1;
        ring.prepare (bufferSize);
    }

    // Alloc-free zeroing for reset() (CR-02): MipRing::clear never resizes.
    void clear() { ring.clear(); }

    void pushSample (float L, float R) { ring.push (L, R); }

    // v3.1.0: the read position is a double; v3.3.0: the integer and fractional parts
    // are split against an absolute 64-bit write counter, so precision depends on the
    // delay only, never on how far the head has run.
    float read (int level, int channel, double delaySamples, int sincSet) const
    {
        return ring.read (level, channel, delaySamples, sincSet);
    }

    void readStereo (int level, double delaySamples, int sincSet, float& outL, float& outR) const
    {
        ring.readStereo (level, delaySamples, sincSet, outL, outR);
    }

    // (L + R) / 2 through one convolution.
    float readMono (int level, double delaySamples, int sincSet) const
    {
        return ring.readMono (level, delaySamples, sincSet);
    }

    const Ring& getRing() const { return ring; }
    int getBufferSize() const { return bufferSize; }
    juce::int64 getWriteCount() const { return ring.getWriteCount(); }
    // Samples pushed since prepare()/clear(), capped at the ring size.
    int getSamplesWritten() const
    {
        return static_cast<int> (juce::jmin<juce::int64> (ring.getWriteCount(), bufferSize));
    }

private:
    Ring ring;
    int bufferSize = 0;
};
