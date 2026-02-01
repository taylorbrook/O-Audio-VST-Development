/*
  ==============================================================================

    WavetableOscillator.h
    Phase-driven wavetable oscillator with mipmap anti-aliasing

    Stage 4.2: Selects band-limited mipmap based on frequency to prevent aliasing

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "WavetableData.h"
#include <algorithm>

class WavetableOscillator
{
public:
    WavetableOscillator() = default;

    void setFrequency(float freq, double sampleRate)
    {
        frequency = freq;
        phaseIncrement = static_cast<float>(freq / sampleRate);

        // Select appropriate mipmap level based on frequency
        currentMipmapLevel = WavetableData::getMipmapLevel(static_cast<double>(freq));
    }

    void setWavetablePosition(float pos)
    {
        wavetablePosition = juce::jlimit(0.0f, 1.0f, pos);
    }

    void reset()
    {
        phase = 0.0f;
    }

    float getNextSample()
    {
        // Calculate frame indices and interpolation fraction
        float framePosition = wavetablePosition * static_cast<float>(WavetableData::NUM_FRAMES - 1);
        int lowerFrame = static_cast<int>(framePosition);
        int upperFrame = juce::jmin(lowerFrame + 1, WavetableData::NUM_FRAMES - 1);
        float frameFrac = framePosition - static_cast<float>(lowerFrame);

        // Calculate sample index within frame
        int sampleIndex = static_cast<int>(phase * static_cast<float>(WavetableData::SAMPLES_PER_FRAME))
                          % WavetableData::SAMPLES_PER_FRAME;

        // Fetch samples from the appropriate mipmap level
        const auto& mipmap = WavetableData::mipmapWavetable[static_cast<size_t>(currentMipmapLevel)];

        float lowerSample = mipmap[static_cast<size_t>(lowerFrame)][static_cast<size_t>(sampleIndex)];
        float upperSample = mipmap[static_cast<size_t>(upperFrame)][static_cast<size_t>(sampleIndex)];

        // Linear interpolation between frames
        float output = lowerSample + frameFrac * (upperSample - lowerSample);

        // Advance phase
        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return output;
    }

private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float frequency = 440.0f;
    float wavetablePosition = 0.5f;
    int currentMipmapLevel = 0;
};
