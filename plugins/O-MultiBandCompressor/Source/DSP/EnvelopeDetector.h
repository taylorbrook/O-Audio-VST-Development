/*
  ==============================================================================

    EnvelopeDetector.h
    Peak/RMS envelope detection with blend parameter
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class EnvelopeDetector
{
public:
    EnvelopeDetector() = default;
    ~EnvelopeDetector() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;

        // RMS window size: 10ms (good balance for most material)
        rmsWindowSize = static_cast<int>(0.01 * sampleRate);
        rmsBuffer.resize(rmsWindowSize, 0.0f);
        rmsWritePosition = 0;
        rmsSum = 0.0f;
    }

    void reset()
    {
        std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
        rmsWritePosition = 0;
        rmsSum = 0.0f;
    }

    // Process single sample and return detected level (linear, not dB)
    float processSample(float input, float peakRmsBlend)
    {
        // Peak detection: absolute value
        float currentPeak = std::abs(input);

        // RMS detection: circular buffer with sliding window
        float oldSample = rmsBuffer[rmsWritePosition];
        float newSample = input;

        // Update RMS sum (remove old, add new)
        rmsSum -= oldSample * oldSample;
        rmsSum += newSample * newSample;

        // Store new sample
        rmsBuffer[rmsWritePosition] = newSample;
        rmsWritePosition = (rmsWritePosition + 1) % rmsWindowSize;

        // Calculate RMS
        float currentRms = std::sqrt(std::max(0.0f, rmsSum / static_cast<float>(rmsWindowSize)));

        // Blend: 0.0 = peak, 1.0 = RMS
        float blend = peakRmsBlend / 100.0f;  // Convert 0-100% to 0-1
        float detectorLevel = (1.0f - blend) * currentPeak + blend * currentRms;

        return detectorLevel;
    }

private:
    double currentSampleRate = 44100.0;

    // RMS circular buffer
    std::vector<float> rmsBuffer;
    int rmsWindowSize = 441;  // ~10ms @ 44.1kHz
    int rmsWritePosition = 0;
    float rmsSum = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeDetector)
};
