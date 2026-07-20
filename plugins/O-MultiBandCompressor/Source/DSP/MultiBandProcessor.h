/*
  ==============================================================================

    MultiBandProcessor.h
    Orchestrates crossover network and 4-band compression
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "CrossoverNetwork.h"
#include "Compressor.h"

class MultiBandProcessor
{
public:
    MultiBandProcessor() = default;
    ~MultiBandProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        currentSampleRate = sampleRate;

        // Prepare crossover network
        crossover.prepare(sampleRate, maxBlockSize, numChannels);

        // Prepare all 4 compressors
        for (int band = 0; band < 4; ++band)
        {
            compressors[band].prepare(sampleRate, maxBlockSize);
        }

        // Preallocate band buffers
        for (int band = 0; band < 4; ++band)
        {
            bandBuffers[band].setSize(numChannels, maxBlockSize);
            bandBuffers[band].clear();
        }
    }

    void reset()
    {
        crossover.reset();

        for (int band = 0; band < 4; ++band)
        {
            compressors[band].reset();
            bandBuffers[band].clear();
        }
    }

    // Update crossover frequencies
    void updateCrossoverFrequencies(float xover1, float xover2, float xover3)
    {
        crossover.updateCoefficients(xover1, xover2, xover3);
    }

    // Process multiband compression
    void processMultiband(juce::AudioBuffer<float>& buffer,
                         const float* thresholds,
                         const float* ratios,
                         const float* knees,
                         const float* makeups,
                         const float* peakRmsBlends,
                         const bool* bypasses,
                         const bool* solos,
                         const float* scHPFs,
                         const float* scLPFs,
                         const bool* scListens,
                         bool autoMakeupEnabled,
                         std::atomic<float>* const* gainReductionMeters)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples == 0 || numChannels == 0)
            return;

        // Check if any band is soloed
        bool anySolo = false;
        for (int band = 0; band < 4; ++band)
        {
            if (solos[band])
            {
                anySolo = true;
                break;
            }
        }

        // Split input into 4 frequency bands
        crossover.processSplit(buffer, bandBuffers);

        // Process each band with its compressor
        for (int band = 0; band < 4; ++band)
        {
            // Determine if this band should be muted (solo logic)
            bool shouldMute = false;
            if (anySolo && !solos[band])
            {
                // Another band is soloed and this one isn't
                shouldMute = true;
            }

            if (shouldMute)
            {
                // Mute this band
                bandBuffers[band].clear();
                gainReductionMeters[band]->store(0.0f, std::memory_order_relaxed);
            }
            else
            {
                // Process compressor on this band with sidechain filtering.
                // WR-02: pass the *input* channel count — in mono M/S modes the stereo
                // band buffers only carry signal on channel 0.
                compressors[band].processStereo(
                    bandBuffers[band],
                    numChannels,
                    thresholds[band],
                    ratios[band],
                    knees[band],
                    makeups[band],
                    peakRmsBlends[band],
                    bypasses[band],
                    scHPFs[band],
                    scLPFs[band],
                    scListens[band],
                    autoMakeupEnabled,
                    *gainReductionMeters[band]
                );
            }
        }

        // Sum all bands back together (Linkwitz-Riley guarantees flat magnitude)
        buffer.clear();
        for (int band = 0; band < 4; ++band)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                buffer.addFrom(ch, 0, bandBuffers[band], ch, 0, numSamples);
            }
        }
    }

    // Set attack/release times for all bands
    void setAttackTimes(const float* attacks)
    {
        for (int band = 0; band < 4; ++band)
        {
            compressors[band].setAttackTime(attacks[band]);
        }
    }

    void setReleaseTimes(const float* releases)
    {
        for (int band = 0; band < 4; ++band)
        {
            compressors[band].setReleaseTime(releases[band]);
        }
    }

private:
    double currentSampleRate = 44100.0;

    // DSP components
    CrossoverNetwork crossover;
    Compressor compressors[4];  // LOW, LOMID, HIMID, HIGH

    // Band buffers (preallocated in prepare())
    juce::AudioBuffer<float> bandBuffers[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiBandProcessor)
};
