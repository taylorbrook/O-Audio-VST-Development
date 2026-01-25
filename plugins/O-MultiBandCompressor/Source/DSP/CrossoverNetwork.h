/*
  ==============================================================================

    CrossoverNetwork.h
    Linkwitz-Riley 4th order crossover (24 dB/octave)
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class CrossoverNetwork
{
public:
    CrossoverNetwork() = default;
    ~CrossoverNetwork() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        currentSampleRate = sampleRate;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = static_cast<juce::uint32>(numChannels);

        // Prepare all filters (3 crossover points × 2 filters (LP/HP) × 2 cascades = 12 filters per channel)
        for (int i = 0; i < 3; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                // Linkwitz-Riley = 2 cascaded Butterworth 2nd order
                lowpass1[i][ch].prepare(spec);
                lowpass2[i][ch].prepare(spec);
                highpass1[i][ch].prepare(spec);
                highpass2[i][ch].prepare(spec);
            }
        }

        // Initialize crossover frequencies
        updateCoefficients(200.0f, 2000.0f, 8000.0f);
    }

    void reset()
    {
        for (int i = 0; i < 3; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                lowpass1[i][ch].reset();
                lowpass2[i][ch].reset();
                highpass1[i][ch].reset();
                highpass2[i][ch].reset();
            }
        }
    }

    // Update crossover frequencies
    void updateCoefficients(float xover1Hz, float xover2Hz, float xover3Hz)
    {
        // Validate frequency ordering: xover1 < xover2 < xover3
        xover1Hz = juce::jlimit(20.0f, 500.0f, xover1Hz);
        xover2Hz = juce::jlimit(std::max(xover1Hz + 100.0f, 200.0f), 5000.0f, xover2Hz);
        xover3Hz = juce::jlimit(std::max(xover2Hz + 100.0f, 2000.0f), 16000.0f, xover3Hz);

        // Crossover 1: Low/Low-Mid split
        auto lp1Coeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            xover1Hz, currentSampleRate, 2);
        auto hp1Coeffs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
            xover1Hz, currentSampleRate, 2);

        // Crossover 2: Low-Mid/High-Mid split
        auto lp2Coeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            xover2Hz, currentSampleRate, 2);
        auto hp2Coeffs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
            xover2Hz, currentSampleRate, 2);

        // Crossover 3: High-Mid/High split
        auto lp3Coeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            xover3Hz, currentSampleRate, 2);
        auto hp3Coeffs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
            xover3Hz, currentSampleRate, 2);

        // Apply coefficients to all channels
        // Note: designIIRLowpassHighOrderButterworthMethod returns ReferenceCountedArray,
        // need to access [0] for single 2nd order section
        for (int ch = 0; ch < 2; ++ch)
        {
            // Crossover 1 (both cascades use same coefficients)
            *lowpass1[0][ch].coefficients = *lp1Coeffs[0];
            *lowpass2[0][ch].coefficients = *lp1Coeffs[0];
            *highpass1[0][ch].coefficients = *hp1Coeffs[0];
            *highpass2[0][ch].coefficients = *hp1Coeffs[0];

            // Crossover 2
            *lowpass1[1][ch].coefficients = *lp2Coeffs[0];
            *lowpass2[1][ch].coefficients = *lp2Coeffs[0];
            *highpass1[1][ch].coefficients = *hp2Coeffs[0];
            *highpass2[1][ch].coefficients = *hp2Coeffs[0];

            // Crossover 3
            *lowpass1[2][ch].coefficients = *lp3Coeffs[0];
            *lowpass2[2][ch].coefficients = *lp3Coeffs[0];
            *highpass1[2][ch].coefficients = *hp3Coeffs[0];
            *highpass2[2][ch].coefficients = *hp3Coeffs[0];
        }
    }

    // Process buffer and split into 4 bands
    // bandBuffers[0] = LOW, bandBuffers[1] = LOMID, bandBuffers[2] = HIMID, bandBuffers[3] = HIGH
    void processSplit(const juce::AudioBuffer<float>& input,
                     juce::AudioBuffer<float>* bandBuffers)
    {
        const int numSamples = input.getNumSamples();
        const int numChannels = input.getNumChannels();

        // Clear all band buffers
        for (int band = 0; band < 4; ++band)
        {
            bandBuffers[band].clear();
        }

        // Process each channel independently
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* inputData = input.getReadPointer(ch);

            // Temporary storage for crossover outputs
            float xover1_low, xover1_high;
            float xover2_low, xover2_high;
            float xover3_low, xover3_high;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                float inputSample = inputData[sample];

                // ===== Crossover 1: Split input into LOW and REST =====
                // Low band (final): LP through both cascades
                xover1_low = lowpass1[0][ch].processSample(inputSample);
                xover1_low = lowpass2[0][ch].processSample(xover1_low);

                // High band (to next crossover): HP through both cascades
                xover1_high = highpass1[0][ch].processSample(inputSample);
                xover1_high = highpass2[0][ch].processSample(xover1_high);

                // ===== Crossover 2: Split xover1_high into LOMID and REST =====
                // Low-Mid band (final): LP through both cascades
                xover2_low = lowpass1[1][ch].processSample(xover1_high);
                xover2_low = lowpass2[1][ch].processSample(xover2_low);

                // High band (to next crossover): HP through both cascades
                xover2_high = highpass1[1][ch].processSample(xover1_high);
                xover2_high = highpass2[1][ch].processSample(xover2_high);

                // ===== Crossover 3: Split xover2_high into HIMID and HIGH =====
                // High-Mid band (final): LP through both cascades
                xover3_low = lowpass1[2][ch].processSample(xover2_high);
                xover3_low = lowpass2[2][ch].processSample(xover3_low);

                // High band (final): HP through both cascades
                xover3_high = highpass1[2][ch].processSample(xover2_high);
                xover3_high = highpass2[2][ch].processSample(xover3_high);

                // Write to band buffers
                bandBuffers[0].setSample(ch, sample, xover1_low);   // LOW
                bandBuffers[1].setSample(ch, sample, xover2_low);   // LOMID
                bandBuffers[2].setSample(ch, sample, xover3_low);   // HIMID
                bandBuffers[3].setSample(ch, sample, xover3_high);  // HIGH
            }
        }
    }

private:
    double currentSampleRate = 44100.0;
    juce::dsp::ProcessSpec spec;

    // Linkwitz-Riley 4th order = 2 cascaded 2nd order Butterworth filters
    // [crossover_index][channel]
    juce::dsp::IIR::Filter<float> lowpass1[3][2];   // First cascade, LP
    juce::dsp::IIR::Filter<float> lowpass2[3][2];   // Second cascade, LP
    juce::dsp::IIR::Filter<float> highpass1[3][2];  // First cascade, HP
    juce::dsp::IIR::Filter<float> highpass2[3][2];  // Second cascade, HP

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossoverNetwork)
};
