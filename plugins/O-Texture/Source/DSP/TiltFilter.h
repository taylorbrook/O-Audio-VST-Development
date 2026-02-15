/*
  ==============================================================================

    TiltFilter.h
    O-Texture - 1-pole tilt EQ for BRIGHTNESS parameter
    Ouaricon Audio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class TiltFilter
{
public:
    TiltFilter() = default;

    void prepare(double sampleRate, int numChannels = 2)
    {
        sr = sampleRate;
        lpState.resize(static_cast<size_t>(numChannels), 0.0f);

        smoothedLGain.reset(sampleRate, 0.05);
        smoothedHGain.reset(sampleRate, 0.05);

        updateCoefficients();

        smoothedLGain.setCurrentAndTargetValue(0.0f);
        smoothedHGain.setCurrentAndTargetValue(0.0f);
    }

    void reset()
    {
        std::fill(lpState.begin(), lpState.end(), 0.0f);
        smoothedLGain.setCurrentAndTargetValue(smoothedLGain.getTargetValue());
        smoothedHGain.setCurrentAndTargetValue(smoothedHGain.getTargetValue());
    }

    void setBrightness(float brightness)
    {
        brightness = juce::jlimit(-1.0f, 1.0f, brightness);

        if (juce::approximatelyEqual(brightness, currentBrightness))
            return;

        currentBrightness = brightness;

        const float gainDB = brightness * maxGainDB;
        float g1, g2;

        if (gainDB > 0.0f)
        {
            g1 = -gfactor * gainDB;
            g2 = gainDB;
        }
        else
        {
            g1 = -gainDB;
            g2 = gfactor * gainDB;
        }

        constexpr float amp = 6.0f / 0.693147180559945f;

        smoothedLGain.setTargetValue(std::exp(g1 / amp) - 1.0f);
        smoothedHGain.setTargetValue(std::exp(g2 / amp) - 1.0f);
    }

    void setCenterFrequency(float freqHz)
    {
        centerFreq = juce::jlimit(20.0f, 20000.0f, freqHz);
        updateCoefficients();
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        if (juce::approximatelyEqual(currentBrightness, 0.0f)
            && !smoothedLGain.isSmoothing()
            && !smoothedHGain.isSmoothing())
            return;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float lg = smoothedLGain.getNextValue();
            const float hg = smoothedHGain.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* channelData = buffer.getWritePointer(ch);
                const float input = channelData[sample];

                float& state = lpState[static_cast<size_t>(ch)];
                const float lpOut = a0 * input + b1 * state;
                state = lpOut;

                const float hpOut = input - lpOut;
                channelData[sample] = input + lg * lpOut + hg * hpOut;
            }
        }
    }

private:
    void updateCoefficients()
    {
        if (sr <= 0.0)
            return;

        const float omega = juce::MathConstants<float>::twoPi * centerFreq;
        const float n = 1.0f / (3.0f * static_cast<float>(sr) + omega);
        a0 = 2.0f * omega * n;
        b1 = (3.0f * static_cast<float>(sr) - omega) * n;
    }

    float a0 = 0.0f;
    float b1 = 0.0f;
    std::vector<float> lpState;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedLGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedHGain;

    double sr = 48000.0;
    float centerFreq = 800.0f;
    float currentBrightness = 0.0f;
    float maxGainDB = 6.0f;
    float gfactor = 4.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TiltFilter)
};
