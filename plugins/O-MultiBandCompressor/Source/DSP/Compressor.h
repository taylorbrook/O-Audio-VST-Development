/*
  ==============================================================================

    Compressor.h
    Single-band feed-forward compressor with soft knee
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "EnvelopeDetector.h"
#include "GainComputer.h"

class Compressor
{
public:
    Compressor() = default;
    ~Compressor() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;

        // Prepare envelope detector
        envelopeDetector.prepare(sampleRate, maxBlockSize);

        // Initialize smoothed gain reduction
        smoothedGainReductionDB = 0.0f;

        // Update coefficients
        updateAttackReleaseCoefficients();
    }

    void reset()
    {
        envelopeDetector.reset();
        smoothedGainReductionDB = 0.0f;
    }

    void setAttackTime(float attackMs)
    {
        currentAttackMs = attackMs;
        updateAttackReleaseCoefficients();
    }

    void setReleaseTime(float releaseMs)
    {
        currentReleaseMs = releaseMs;
        updateAttackReleaseCoefficients();
    }

    // Process stereo buffer
    void processStereo(juce::AudioBuffer<float>& buffer,
                      float thresholdDB,
                      float ratio,
                      float kneeDB,
                      float makeupDB,
                      float peakRmsBlend,
                      bool isBypassed,
                      std::atomic<float>& gainReductionMeter)
    {
        if (isBypassed)
        {
            gainReductionMeter.store(0.0f, std::memory_order_relaxed);
            return;
        }

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples == 0 || numChannels == 0)
            return;

        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Use stereo link: average both channels for detection
            float detectorInput = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
            {
                detectorInput += buffer.getSample(channel, sample);
            }
            detectorInput /= static_cast<float>(numChannels);

            // Envelope detection (peak/RMS blend)
            float envelopeLevel = envelopeDetector.processSample(detectorInput, peakRmsBlend);

            // Gain computer (soft knee)
            float targetGainReductionDB = gainComputer.calculateGainReduction(
                envelopeLevel, thresholdDB, ratio, kneeDB);

            // Envelope smoothing (attack/release ballistics)
            if (targetGainReductionDB < smoothedGainReductionDB)
            {
                // Rising gain reduction (more compression) - use attack
                smoothedGainReductionDB += (targetGainReductionDB - smoothedGainReductionDB) * (1.0f - attackCoeff);
            }
            else
            {
                // Falling gain reduction (less compression) - use release
                smoothedGainReductionDB += (targetGainReductionDB - smoothedGainReductionDB) * (1.0f - releaseCoeff);
            }

            // Convert to linear gain
            float gainReductionLinear = std::pow(10.0f, smoothedGainReductionDB / 20.0f);
            float makeupLinear = std::pow(10.0f, makeupDB / 20.0f);
            float totalGain = gainReductionLinear * makeupLinear;

            // Apply gain to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float sample_value = buffer.getSample(channel, sample);
                buffer.setSample(channel, sample, sample_value * totalGain);
            }
        }

        // Update gain reduction meter (atomic for UI thread)
        gainReductionMeter.store(smoothedGainReductionDB, std::memory_order_relaxed);
    }

private:
    void updateAttackReleaseCoefficients()
    {
        // Convert ms to seconds
        float attackSec = currentAttackMs * 0.001f;
        float releaseSec = currentReleaseMs * 0.001f;

        // Calculate coefficients: coeff = exp(-1.0 / (time * sampleRate))
        attackCoeff = std::exp(-1.0f / (attackSec * static_cast<float>(currentSampleRate)));
        releaseCoeff = std::exp(-1.0f / (releaseSec * static_cast<float>(currentSampleRate)));
    }

    double currentSampleRate = 44100.0;

    // DSP components
    EnvelopeDetector envelopeDetector;
    GainComputer gainComputer;

    // Attack/release parameters
    float currentAttackMs = 10.0f;
    float currentReleaseMs = 100.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Smoothed gain reduction state
    float smoothedGainReductionDB = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compressor)
};
