#pragma once

#include <JuceHeader.h>
#include "LagrangeInterpolation.h"

class DelayBuffer
{
public:
    void prepare (double sampleRate, float maxDelaySec = 2.0f)
    {
        bufferSize = static_cast<int> (sampleRate * maxDelaySec) + 1;
        buffer.setSize (2, bufferSize);
        buffer.clear();
        writePosition = 0;
    }

    // Alloc-free zeroing for reset() — must NOT call setSize (that reallocates on the
    // audio thread). Mirrors the intent of the old prepare()-in-reset() without the churn.
    void clear()
    {
        buffer.clear();
        writePosition = 0;
    }

    void pushSample (float L, float R)
    {
        buffer.setSample (0, writePosition, L);
        buffer.setSample (1, writePosition, R);
        writePosition = (writePosition + 1) % bufferSize;
    }

    float readSample (int channel, float delaySamples) const
    {
        float readPos = static_cast<float> (writePosition) - delaySamples;
        while (readPos < 0.0f)
            readPos += static_cast<float> (bufferSize);

        int i0 = static_cast<int> (readPos);
        float frac = readPos - static_cast<float> (i0);

        // Lagrange 3rd-order interpolation (4 points)
        int im1 = ((i0 - 1) % bufferSize + bufferSize) % bufferSize;
        i0      = i0 % bufferSize;
        int i1  = (i0 + 1) % bufferSize;
        int i2  = (i0 + 2) % bufferSize;

        auto* data = buffer.getReadPointer (channel);

        return lagrangeInterpolate (data[im1], data[i0], data[i1], data[i2], frac);
    }

    void copyRegion (juce::AudioBuffer<float>& dest, int startOffset, int length) const
    {
        if (length <= 0 || bufferSize <= 0)
            return;

        length = juce::jmin (length, bufferSize, dest.getNumSamples());
        int readPos = ((writePosition - startOffset) % bufferSize + bufferSize) % bufferSize;

        // WR-03: two contiguous memcpys (via AudioBuffer::copyFrom) to span the ring
        // wrap, instead of ~176k per-element getSample/setSample calls on the RT thread.
        int firstRun = juce::jmin (length, bufferSize - readPos);
        for (int ch = 0; ch < 2; ++ch)
        {
            dest.copyFrom (ch, 0, buffer, ch, readPos, firstRun);
            if (length > firstRun)
                dest.copyFrom (ch, firstRun, buffer, ch, 0, length - firstRun);
        }
    }

    int getWritePosition() const { return writePosition; }
    int getBufferSize() const { return bufferSize; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    int writePosition = 0;
};
