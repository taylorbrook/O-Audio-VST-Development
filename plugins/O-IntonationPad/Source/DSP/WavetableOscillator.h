/*
  ==============================================================================

    WavetableOscillator.h
    Phase-driven wavetable oscillator with mipmap anti-aliasing and bank selection

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
        currentMipmapLevel = WavetableData::getMipmapLevel(static_cast<double>(freq));
    }

    void setWavetablePosition(float pos)
    {
        wavetablePosition = juce::jlimit(0.0f, 1.0f, pos);
    }

    void setWavetableBank(const WavetableData::MipmapTable* bank)
    {
        activeBank = bank;
    }

    void reset()
    {
        phase = 0.0f;
    }

    void advancePhase(int numSamples)
    {
        phase += phaseIncrement * static_cast<float>(numSamples);
        phase -= std::floor(phase);
    }

    void processBlockStereo(float* destL, float* destR, int numSamples, float gainL, float gainR)
    {
        // Hoist frame calculations out of the loop (constant for the block)
        float framePosition = wavetablePosition * static_cast<float>(WavetableData::NUM_FRAMES - 1);
        int lowerFrame = static_cast<int>(framePosition);
        int upperFrame = juce::jmin(lowerFrame + 1, WavetableData::NUM_FRAMES - 1);
        float frameFrac = framePosition - static_cast<float>(lowerFrame);

        const auto& mipmap = (*activeBank)[static_cast<size_t>(currentMipmapLevel)];
        const auto& lowerData = mipmap[static_cast<size_t>(lowerFrame)];
        const auto& upperData = mipmap[static_cast<size_t>(upperFrame)];

        const float spf = static_cast<float>(WavetableData::SAMPLES_PER_FRAME);

        for (int i = 0; i < numSamples; ++i)
        {
            float samplePos = phase * spf;
            int sampleIndex0 = static_cast<int>(samplePos) % WavetableData::SAMPLES_PER_FRAME;
            int sampleIndex1 = (sampleIndex0 + 1) % WavetableData::SAMPLES_PER_FRAME;
            float sampleFrac = samplePos - std::floor(samplePos);

            float s0 = lowerData[static_cast<size_t>(sampleIndex0)];
            float s1 = lowerData[static_cast<size_t>(sampleIndex1)];
            float lowerSample = s0 + sampleFrac * (s1 - s0);

            float s2 = upperData[static_cast<size_t>(sampleIndex0)];
            float s3 = upperData[static_cast<size_t>(sampleIndex1)];
            float upperSample = s2 + sampleFrac * (s3 - s2);

            float output = lowerSample + frameFrac * (upperSample - lowerSample);

            destL[i] += output * gainL;
            destR[i] += output * gainR;

            phase += phaseIncrement;
            if (phase >= 1.0f)
                phase -= 1.0f;
        }
    }

private:
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float frequency = 440.0f;
    float wavetablePosition = 0.5f;
    int currentMipmapLevel = 0;
    const WavetableData::MipmapTable* activeBank = &WavetableData::BankCache::getBank(0);
};
