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

    O-Tapestop - CaptureBuffer

    Stereo circular capture ring with absolute-index bookkeeping. Trimmed port
    of O-ReverseDelay's CaptureBuffer.h (Stage-2 PLAN Task 2 / RESEARCH §2.1):

      KEPT:    prepare() (the only allocation), alloc-free clear(), pushSample,
               readAbs() with the double-mod negative-tolerant wrap,
               getTotalWritten / getBufferSize.
      DROPPED: pushLooped / pushCrossfaded (O-ReverseDelay freeze machinery —
               O-Tapestop v1.0 has no freeze-loop write path) and monoSum
               (this is FULL-STEREO varispeed; ARCHITECTURE forbids
               mono-summing — the stereo image must survive).

    The ring is written every sample, every block, in EVERY state (including
    Bypassed) so an engage gesture always has history behind it. Pre-history
    reads return the cleared buffer's zeros, which makes the debt clamp's
    worst case (playing the oldest valid material) safe even in the first
    kCaptureSeconds after load.

    clear() is alloc-free (never setSize outside prepare).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CaptureBuffer
{
public:
    void prepare (double sampleRate, double maxSeconds)
    {
        bufferSize = static_cast<int> (sampleRate * maxSeconds) + 1;
        buffer.setSize (2, bufferSize);
        buffer.clear();
        writePosition = 0;
        totalWritten = 0;
    }

    // Alloc-free zeroing for reset paths — must NOT call setSize (that would
    // reallocate on the audio thread).
    void clear() noexcept
    {
        buffer.clear();
        writePosition = 0;
        totalWritten = 0;
    }

    void pushSample (float L, float R) noexcept
    {
        buffer.setSample (0, writePosition, L);
        buffer.setSample (1, writePosition, R);
        writePosition = (writePosition + 1) % bufferSize;
        ++totalWritten;
    }

    // Integer read at an absolute (monotonic) sample index. Double-mod handles
    // negative indices (pre-history reads return the cleared buffer's zeros).
    float readAbs (int ch, juce::int64 absIndex) const noexcept
    {
        const int idx = static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
        return buffer.getSample (ch, idx);
    }

    juce::int64 getTotalWritten() const noexcept { return totalWritten; }
    int getBufferSize() const noexcept           { return bufferSize; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    int writePosition = 0;
    juce::int64 totalWritten = 0;
};
