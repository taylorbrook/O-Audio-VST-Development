#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

class DistanceModel
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 512, 1 };
        lpf.prepare (spec);
        lpf.reset();
        currentGain = 1.0f;
    }

    void updateDistance (float dist, float airAbs, int curveType)
    {
        float gain = 1.0f;

        switch (curveType)
        {
            case 0: // Linear
                gain = std::clamp (1.0f - (dist - 0.1f) / 29.9f, 0.0f, 1.0f);
                break;
            case 1: // Inverse
                gain = 1.0f / std::max (dist, 0.1f);
                break;
            case 2: // Inverse Square
                gain = 1.0f / std::max (dist * dist, 0.01f);
                break;
        }

        currentGain = std::clamp (gain, 0.0f, 1.0f);

        float cutoff = 20000.0f / (1.0f + airAbs * 0.01f * (dist / 10.0f));
        cutoff = std::clamp (cutoff, 100.0f, 20000.0f);

        *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, cutoff);
    }

    float processSample (float sample)
    {
        return currentGain * lpf.processSample (sample);
    }

    void reset()
    {
        lpf.reset();
        currentGain = 1.0f;
    }

private:
    double sampleRate = 48000.0;
    juce::dsp::IIR::Filter<float> lpf;
    float currentGain = 1.0f;
};
