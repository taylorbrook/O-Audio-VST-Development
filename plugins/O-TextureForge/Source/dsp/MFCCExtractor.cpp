/*
  ==============================================================================

    MFCCExtractor.cpp
    MFCC extraction implementation

  ==============================================================================
*/

#include "MFCCExtractor.h"
#include <cmath>

MFCCExtractor::MFCCExtractor()
    : fft(FFT_ORDER),
      window(FFT_SIZE, juce::dsp::WindowingFunction<float>::hann, false)  // normalise=false
{
    fftData.fill(0.0f);
    magnitudes.resize(FFT_SIZE / 2 + 1, 0.0f);

    buildMelFilterbank(44100.0);  // Default, will be rebuilt if needed
    buildDCTMatrix();
}

std::array<float, 13> MFCCExtractor::extractMFCCs(const float* frame, int frameSize)
{
    // Clamp analysis length to FFT window size — grain frames at higher sample
    // rates (e.g. 50ms @ 48kHz = 2400 samples) may exceed FFT_SIZE (2048).
    // Standard MFCC practice: analyze first FFT_SIZE samples, zero-pad if shorter.
    const int analysisSize = std::min(frameSize, FFT_SIZE);

    // Pre-emphasis: y[n] = x[n] - 0.97 * x[n-1]
    static constexpr float alpha = 0.97f;
    std::array<float, FFT_SIZE> emphasized;
    emphasized.fill(0.0f);

    emphasized[0] = frame[0];
    for (int i = 1; i < analysisSize; ++i)
        emphasized[i] = frame[i] - alpha * frame[i - 1];

    // Copy to FFT buffer (real part only)
    for (int i = 0; i < analysisSize; ++i)
        fftData[i] = emphasized[i];
    for (int i = analysisSize; i < FFT_SIZE; ++i)
        fftData[i] = 0.0f;

    // Apply Hann window
    window.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);

    // Perform FFT (frequency-only forward transform)
    fft.performFrequencyOnlyForwardTransform(fftData.data(), true);  // true = magnitudes

    // Extract magnitudes (already computed by performFrequencyOnlyForwardTransform)
    for (int i = 0; i < FFT_SIZE / 2 + 1; ++i)
        magnitudes[i] = fftData[i];

    // Apply Mel filterbank
    std::array<float, NUM_MEL_FILTERS> melEnergies;
    for (int i = 0; i < NUM_MEL_FILTERS; ++i)
    {
        float energy = 0.0f;
        for (int j = 0; j < FFT_SIZE / 2 + 1; ++j)
            energy += magnitudes[j] * melFilterbank[i][j];
        melEnergies[i] = std::max(energy, 1e-10f);  // Floor to prevent log(0)
    }

    // Log Mel energies
    for (int i = 0; i < NUM_MEL_FILTERS; ++i)
        melEnergies[i] = std::log(melEnergies[i]);

    // Apply DCT
    std::array<float, NUM_MFCCS> mfccs;
    for (int i = 0; i < NUM_MFCCS; ++i)
    {
        float sum = 0.0f;
        for (int j = 0; j < NUM_MEL_FILTERS; ++j)
            sum += dctMatrix[i][j] * melEnergies[j];
        mfccs[i] = sum;
    }

    return mfccs;
}

void MFCCExtractor::buildMelFilterbank(double sampleRate)
{
    const float nyquist = static_cast<float>(sampleRate) / 2.0f;
    const float minHz = 20.0f;
    const float maxHz = nyquist;

    const float minMel = hzToMel(minHz);
    const float maxMel = hzToMel(maxHz);
    const float melStep = (maxMel - minMel) / (NUM_MEL_FILTERS + 1);

    std::array<float, NUM_MEL_FILTERS + 2> filterFreqs;
    for (int i = 0; i < NUM_MEL_FILTERS + 2; ++i)
        filterFreqs[i] = melToHz(minMel + i * melStep);

    // Build triangular filters
    for (int i = 0; i < NUM_MEL_FILTERS; ++i)
    {
        const float leftHz = filterFreqs[i];
        const float centerHz = filterFreqs[i + 1];
        const float rightHz = filterFreqs[i + 2];

        for (int bin = 0; bin < FFT_SIZE / 2 + 1; ++bin)
        {
            const float binHz = (bin * static_cast<float>(sampleRate)) / FFT_SIZE;

            float weight = 0.0f;
            if (binHz >= leftHz && binHz <= centerHz)
            {
                weight = (binHz - leftHz) / (centerHz - leftHz);
            }
            else if (binHz > centerHz && binHz <= rightHz)
            {
                weight = (rightHz - binHz) / (rightHz - centerHz);
            }

            melFilterbank[i][bin] = weight;
        }
    }
}

void MFCCExtractor::buildDCTMatrix()
{
    for (int i = 0; i < NUM_MFCCS; ++i)
    {
        for (int j = 0; j < NUM_MEL_FILTERS; ++j)
        {
            dctMatrix[i][j] = std::cos(juce::MathConstants<float>::pi * i * (j + 0.5f) / NUM_MEL_FILTERS);
        }
    }
}

float MFCCExtractor::hzToMel(float hz) const
{
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

float MFCCExtractor::melToHz(float mel) const
{
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}
