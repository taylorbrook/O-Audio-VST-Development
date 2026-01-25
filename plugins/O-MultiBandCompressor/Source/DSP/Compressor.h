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

        // Prepare sidechain filters
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1; // Mono sidechain

        scHPF.prepare(spec);
        scLPF.prepare(spec);
        scHPF.reset();
        scLPF.reset();

        // Initialize auto-makeup
        averageGainReduction = 0.0f;
        autoMakeupSmoothingCoeff = 1.0f - std::exp(-1.0f / (0.5f * static_cast<float>(sampleRate))); // 500ms smoothing
    }

    void reset()
    {
        envelopeDetector.reset();
        smoothedGainReductionDB = 0.0f;
        scHPF.reset();
        scLPF.reset();
        averageGainReduction = 0.0f;
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

    // Process stereo buffer with sidechain filtering and auto-makeup
    void processStereo(juce::AudioBuffer<float>& buffer,
                      float thresholdDB,
                      float ratio,
                      float kneeDB,
                      float makeupDB,
                      float peakRmsBlend,
                      bool isBypassed,
                      float scHPFFreq,
                      float scLPFFreq,
                      bool scListen,
                      bool autoMakeupEnabled,
                      std::atomic<float>& gainReductionMeter)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples == 0 || numChannels == 0)
            return;

        // Update sidechain filter coefficients
        updateSidechainFilters(scHPFFreq, scLPFFreq);

        if (isBypassed)
        {
            gainReductionMeter.store(0.0f, std::memory_order_relaxed);
            return;
        }

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

            // Apply sidechain filtering to detector signal (not audio path)
            float filteredSidechain = applySidechainFilters(detectorInput);

            // Sidechain listen mode: replace audio output with filtered sidechain
            if (scListen)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    buffer.setSample(channel, sample, filteredSidechain);
                }
                continue; // Skip compression processing in listen mode
            }

            // Envelope detection (peak/RMS blend) on filtered sidechain
            float envelopeLevel = envelopeDetector.processSample(filteredSidechain, peakRmsBlend);

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

            // Update auto-makeup calculation (running average of GR)
            averageGainReduction += (smoothedGainReductionDB - averageGainReduction) * autoMakeupSmoothingCoeff;

            // Calculate total makeup gain
            float autoMakeupDB = autoMakeupEnabled ? (-averageGainReduction * 0.8f) : 0.0f;
            float totalMakeupDB = makeupDB + autoMakeupDB;

            // Convert to linear gain
            float gainReductionLinear = std::pow(10.0f, smoothedGainReductionDB / 20.0f);
            float makeupLinear = std::pow(10.0f, totalMakeupDB / 20.0f);
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

    void updateSidechainFilters(float hpfFreq, float lpfFreq)
    {
        // HPF: 0 means off, otherwise 20-2000 Hz
        if (hpfFreq > 0.0f && hpfFreq != currentSCHPFFreq)
        {
            auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
                currentSampleRate, hpfFreq, 0.707f); // Q = 0.707 (Butterworth)
            *scHPF.coefficients = *hpfCoeffs;
            currentSCHPFFreq = hpfFreq;
            scHPFEnabled = true;
        }
        else if (hpfFreq == 0.0f)
        {
            scHPFEnabled = false;
        }

        // LPF: 0 means off, otherwise 500-20000 Hz
        if (lpfFreq > 0.0f && lpfFreq < 20000.0f && lpfFreq != currentSCLPFFreq)
        {
            auto lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
                currentSampleRate, lpfFreq, 0.707f); // Q = 0.707 (Butterworth)
            *scLPF.coefficients = *lpfCoeffs;
            currentSCLPFFreq = lpfFreq;
            scLPFEnabled = true;
        }
        else if (lpfFreq == 0.0f || lpfFreq >= 20000.0f)
        {
            scLPFEnabled = false;
        }
    }

    float applySidechainFilters(float input)
    {
        float filtered = input;

        // Apply HPF if enabled
        if (scHPFEnabled)
        {
            filtered = scHPF.processSample(filtered);
        }

        // Apply LPF if enabled
        if (scLPFEnabled)
        {
            filtered = scLPF.processSample(filtered);
        }

        return filtered;
    }

    double currentSampleRate = 44100.0;

    // DSP components
    EnvelopeDetector envelopeDetector;
    GainComputer gainComputer;

    // Sidechain filters (2nd order Butterworth)
    juce::dsp::IIR::Filter<float> scHPF;
    juce::dsp::IIR::Filter<float> scLPF;
    bool scHPFEnabled = false;
    bool scLPFEnabled = false;
    float currentSCHPFFreq = 0.0f;
    float currentSCLPFFreq = 0.0f;

    // Auto-makeup gain
    float averageGainReduction = 0.0f;
    float autoMakeupSmoothingCoeff = 0.0f;

    // Attack/release parameters
    float currentAttackMs = 10.0f;
    float currentReleaseMs = 100.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Smoothed gain reduction state
    float smoothedGainReductionDB = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compressor)
};
