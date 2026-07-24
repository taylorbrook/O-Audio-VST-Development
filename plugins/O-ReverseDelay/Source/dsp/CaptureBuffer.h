/*
  ==============================================================================

    O-ReverseDelay - CaptureBuffer

    Stereo circular capture ring, written by input + damped feedback return.
    Adapted from O-GrainScatter's DelayBuffer.h with two contract changes
    (Stage-2 PLAN Task 2 / RESEARCH §3.1):

      * Lagrange interpolation DROPPED — v1.0 grain reads are integer-stepped
        (reverse speed is exactly 1.0, no pitch change).
      * Absolute-position bookkeeping ADDED — a monotonically increasing
        int64 `totalWritten` runs alongside the ring. A grain latches
        `readStartAbs = totalWritten − D` at spawn and reads
        `capture[(readStartAbs − n) mod size]`. No per-sample drift math,
        no wrap ambiguity; the D+2n read-offset growth falls out for free
        (write head +1 per sample while readAbs steps −1).

    clear() is alloc-free (never setSize outside prepare) — O-GrainScatter WR fix.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CaptureBuffer
{
public:
    void prepare (double sampleRate, float maxSeconds = 3.5f)
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

    // Mono-sum grain source (Stage-2 CONTEXT decision D4): the grain engine
    // reads 0.5·(L+R); equal-power pan then places the mono grain.
    float monoSum (juce::int64 absIndex) const noexcept
    {
        const int idx = static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
        return 0.5f * (buffer.getSample (0, idx) + buffer.getSample (1, idx));
    }

    juce::int64 getTotalWritten() const noexcept { return totalWritten; }
    int getBufferSize() const noexcept           { return bufferSize; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    int writePosition = 0;
    juce::int64 totalWritten = 0;
};
