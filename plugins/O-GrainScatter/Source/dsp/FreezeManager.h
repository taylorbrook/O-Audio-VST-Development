#pragma once

#include <JuceHeader.h>
#include "DelayBuffer.h"

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

    void engage (const DelayBuffer& delayBuf, int grainSizeSamples)
    {
        int len = juce::jmin (grainSizeSamples * 4, freezeBuffer.getNumSamples());
        delayBuf.copyRegion (freezeBuffer, len, len);
        captureLength = len;
        active = true;
        crossfadeCounter = crossfadeSamples;
    }

    void release()
    {
        active = false;
        crossfadeCounter = 0;
    }

    bool isActive() const { return active; }
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
        float ym1 = data[im1];
        float y0  = data[i0];
        float y1  = data[i1];
        float y2  = data[i2];

        float c0 = y0;
        float c1 = y1 - (1.0f / 3.0f) * ym1 - 0.5f * y0 - (1.0f / 6.0f) * y2;
        float c2 = 0.5f * (ym1 + y1) - y0;
        float c3 = (1.0f / 6.0f) * (y2 - ym1) + 0.5f * (y0 - y1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    // Returns crossfade gain (0→1) during engage transition
    float getCrossfadeGain() const
    {
        if (crossfadeCounter <= 0) return 1.0f;
        return 1.0f - static_cast<float> (crossfadeCounter) / static_cast<float> (crossfadeSamples);
    }

    void advanceCrossfade()
    {
        if (crossfadeCounter > 0) --crossfadeCounter;
    }

private:
    juce::AudioBuffer<float> freezeBuffer;
    double sampleRate = 44100.0;
    int captureLength = 0;
    int crossfadeSamples = 220;
    int crossfadeCounter = 0;
    bool active = false;
};
