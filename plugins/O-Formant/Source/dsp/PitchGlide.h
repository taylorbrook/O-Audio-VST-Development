/*
  ==============================================================================

    PitchGlide.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice exponential one-pole smoother for portamento.
    Adapted from O-Prism GlideProcessor pattern.

  ==============================================================================
*/

#pragma once
#include <cmath>

class PitchGlide
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        coeff = 0.0f;
    }

    void setTarget (float freqHz) noexcept
    {
        targetFreq = freqHz;
    }

    void snapTo (float freqHz) noexcept
    {
        currentFreq = freqHz;
        targetFreq = freqHz;
    }

    void setTime (float timeMs) noexcept
    {
        if (timeMs > 0.0f && sampleRate > 0.0)
            coeff = static_cast<float> (std::exp (-1.0 / (static_cast<double> (timeMs) * 0.001 * sampleRate)));
        else
            coeff = 0.0f;
    }

    inline float getNextFrequency() noexcept
    {
        if (coeff <= 0.0f || std::abs (currentFreq - targetFreq) < targetFreq * 0.00001f)
        {
            currentFreq = targetFreq;
            return currentFreq;
        }

        currentFreq = currentFreq * coeff + targetFreq * (1.0f - coeff);
        return currentFreq;
    }

    void reset() noexcept
    {
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        coeff = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float coeff = 0.0f;
};
