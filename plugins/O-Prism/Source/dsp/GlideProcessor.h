/*
  ==============================================================================

    GlideProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio (Header-only)

  ==============================================================================
*/

#pragma once
#include <cmath>

class GlideProcessor
{
public:
    GlideProcessor() = default;

    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
        updateCoefficient();
    }

    void reset()
    {
        currentFreq = targetFreq;
    }

    void setTarget (double freq)
    {
        targetFreq = freq;
        if (mode == 0 || !wasActive) // Off or no prior note
            currentFreq = targetFreq;
    }

    void setMode (int m) { mode = m; }

    void setTime (double seconds)
    {
        glideTime = seconds;
        updateCoefficient();
    }

    void setWasActive (bool active) { wasActive = active; }

    double getNextFrequency()
    {
        if (mode == 0 || std::abs (currentFreq - targetFreq) < targetFreq * 0.00001)
        {
            currentFreq = targetFreq;
            return currentFreq;
        }

        currentFreq = currentFreq * glideCoeff + targetFreq * (1.0 - glideCoeff);
        return currentFreq;
    }

    double getCurrentFrequency() const { return currentFreq; }

private:
    void updateCoefficient()
    {
        if (glideTime > 0.0 && currentSampleRate > 0.0)
            glideCoeff = std::exp (-1.0 / (glideTime * currentSampleRate));
        else
            glideCoeff = 0.0;
    }

    double currentSampleRate = 44100.0;
    double currentFreq = 440.0;
    double targetFreq = 440.0;
    double glideTime = 0.1;
    double glideCoeff = 0.0;
    int mode = 0;     // 0=Off, 1=Legato, 2=Always
    bool wasActive = false;
};
