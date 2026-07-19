#pragma once

#include <JuceHeader.h>
#include "DelayBuffer.h"
#include "LagrangeInterpolation.h"

class FreezeManager
{
public:
    void prepare (double newSampleRate, int maxLengthSamples)
    {
        sampleRate = newSampleRate;
        freezeBuffer.setSize (2, maxLengthSamples);
        freezeBuffer.clear();
        crossfadeSamples = static_cast<int> (sampleRate * 0.005);  // ~5ms
        captureLength = 0;
        active = false;
        crossfadeCounter = 0;
    }

    // Alloc-free state reset for reset() — zeroes the (already-sized) buffer and clears
    // freeze/crossfade flags without setSize(). CR-02: keeps reset() off the allocator.
    void clear()
    {
        freezeBuffer.clear();
        captureLength = 0;
        active = false;
        releasing = false;
        crossfadeCounter = 0;
    }

    void engage (const DelayBuffer& delayBuf, int grainSizeSamples)
    {
        int len = juce::jmin (grainSizeSamples * 4, freezeBuffer.getNumSamples());
        delayBuf.copyRegion (freezeBuffer, len, len);
        captureLength = len;
        active = true;
        releasing = false;
        crossfadeCounter = crossfadeSamples;
    }

    void release()
    {
        if (!active) return;
        releasing = true;
        crossfadeCounter = crossfadeSamples;
    }

    bool isActive() const { return active; }
    bool isReleasing() const { return releasing; }
    int getCaptureLength() const { return captureLength; }

    float readSample (int channel, float position) const
    {
        if (captureLength <= 0) return 0.0f;

        // Wrap within capture length
        float wrapped = std::fmod (position, static_cast<float> (captureLength));
        if (wrapped < 0.0f) wrapped += static_cast<float> (captureLength);

        int i0 = static_cast<int> (wrapped);
        float frac = wrapped - static_cast<float> (i0);

        // Lagrange 3rd-order interpolation
        int im1 = ((i0 - 1) % captureLength + captureLength) % captureLength;
        i0      = i0 % captureLength;
        int i1  = (i0 + 1) % captureLength;
        int i2  = (i0 + 2) % captureLength;

        auto* data = freezeBuffer.getReadPointer (channel);

        return lagrangeInterpolate (data[im1], data[i0], data[i1], data[i2], frac);
    }

    void advanceCrossfade()
    {
        if (crossfadeCounter > 0)
        {
            --crossfadeCounter;
            if (crossfadeCounter <= 0 && releasing)
            {
                active = false;
                releasing = false;
            }
        }
    }

private:
    juce::AudioBuffer<float> freezeBuffer;
    double sampleRate = 44100.0;
    int captureLength = 0;
    int crossfadeSamples = 220;
    int crossfadeCounter = 0;
    bool active = false;
    bool releasing = false;
};
