/*
  ==============================================================================

    WavetableGenerator.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "WavetableGenerator.h"
#include "MathConstants.h"
#include <JuceHeader.h>
#include <cmath>

void WavetableGenerator::generateSaw (float* buffer, int size)
{
    // Additive synthesis: sum(sin(n*2pi*i/size) / n) for n=1..size/2
    std::fill (buffer, buffer + size, 0.0f);
    int numHarmonics = size / 2;

    for (int n = 1; n <= numHarmonics; ++n)
    {
        double amplitude = 1.0 / n;
        for (int i = 0; i < size; ++i)
            buffer[i] += static_cast<float> (amplitude * std::sin (n * kTwoPi * i / size));
    }

    // Normalize
    float maxVal = 0.0f;
    for (int i = 0; i < size; ++i)
        maxVal = std::max (maxVal, std::abs (buffer[i]));
    if (maxVal > 0.0f)
        for (int i = 0; i < size; ++i)
            buffer[i] /= maxVal;
}

void WavetableGenerator::generateSquare (float* buffer, int size)
{
    // Additive: odd harmonics only
    std::fill (buffer, buffer + size, 0.0f);
    int numHarmonics = size / 2;

    for (int n = 1; n <= numHarmonics; n += 2)
    {
        double amplitude = 1.0 / n;
        for (int i = 0; i < size; ++i)
            buffer[i] += static_cast<float> (amplitude * std::sin (n * kTwoPi * i / size));
    }

    float maxVal = 0.0f;
    for (int i = 0; i < size; ++i)
        maxVal = std::max (maxVal, std::abs (buffer[i]));
    if (maxVal > 0.0f)
        for (int i = 0; i < size; ++i)
            buffer[i] /= maxVal;
}

void WavetableGenerator::generateTriangle (float* buffer, int size)
{
    // Additive: odd harmonics, alternating sign, 1/n^2
    std::fill (buffer, buffer + size, 0.0f);
    int numHarmonics = size / 2;
    double sign = 1.0;

    for (int n = 1; n <= numHarmonics; n += 2)
    {
        double amplitude = sign / (static_cast<double> (n) * n);
        for (int i = 0; i < size; ++i)
            buffer[i] += static_cast<float> (amplitude * std::sin (n * kTwoPi * i / size));
        sign = -sign;
    }

    float maxVal = 0.0f;
    for (int i = 0; i < size; ++i)
        maxVal = std::max (maxVal, std::abs (buffer[i]));
    if (maxVal > 0.0f)
        for (int i = 0; i < size; ++i)
            buffer[i] /= maxVal;
}

void WavetableGenerator::generateSine (float* buffer, int size)
{
    for (int i = 0; i < size; ++i)
        buffer[i] = static_cast<float> (std::sin (kTwoPi * i / size));
}

std::unique_ptr<WavetableData> WavetableGenerator::generateProceduralTable (WaveShape shape)
{
    auto table = std::make_unique<WavetableData>();
    table->allocate (1); // Single frame for procedural tables

    // Generate base waveform into level 0, frame 0
    float* frameData = table->getFrameData (0, 0);

    switch (shape)
    {
        case WaveShape::Saw:      generateSaw (frameData, WavetableData::kTableSize); break;
        case WaveShape::Square:   generateSquare (frameData, WavetableData::kTableSize); break;
        case WaveShape::Triangle: generateTriangle (frameData, WavetableData::kTableSize); break;
        case WaveShape::Sine:     generateSine (frameData, WavetableData::kTableSize); break;
    }

    // Generate mipmaps from level 0
    generateMipmaps (*table);

    return table;
}

void WavetableGenerator::generateMipmaps (WavetableData& table)
{
    static constexpr int fftOrder = 11; // log2(2048)
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft (fftOrder);
    std::vector<float> fftBuffer (fftSize * 2, 0.0f);
    std::vector<float> workBuffer (fftSize * 2, 0.0f);

    for (int frame = 0; frame < table.numFrames; ++frame)
    {
        // Copy level 0 frame data into FFT buffer
        const float* srcFrame = table.getFrameData (0, frame);
        std::copy (srcFrame, srcFrame + fftSize, fftBuffer.begin());
        std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

        // Forward FFT (false = full spectrum, needed for IFFT)
        fft.performRealOnlyForwardTransform (fftBuffer.data(), false);

        // Generate each mipmap level
        for (int level = 0; level < WavetableData::kNumMipmapLevels; ++level)
        {
            int maxHarmonic = (fftSize / 2) >> level;

            // Copy spectral data
            std::copy (fftBuffer.begin(), fftBuffer.end(), workBuffer.begin());

            // Zero DC bin
            workBuffer[0] = 0.0f;
            workBuffer[1] = 0.0f;

            // Zero bins above maxHarmonic
            for (int bin = maxHarmonic + 1; bin <= fftSize / 2; ++bin)
            {
                workBuffer[bin * 2] = 0.0f;
                workBuffer[bin * 2 + 1] = 0.0f;
            }

            // Zero negative frequency bins that correspond to zeroed positive bins
            // In JUCE's real-only FFT layout, negative freqs are at bins fftSize-k
            for (int bin = 1; bin < fftSize / 2; ++bin)
            {
                if (bin > maxHarmonic)
                {
                    int negBin = fftSize - bin;
                    if (negBin < fftSize)
                    {
                        workBuffer[negBin * 2] = 0.0f;
                        workBuffer[negBin * 2 + 1] = 0.0f;
                    }
                }
            }

            // Inverse FFT
            fft.performRealOnlyInverseTransform (workBuffer.data());

            // Store result (JUCE IFFT divides by N internally)
            float* destFrame = table.getFrameData (level, frame);
            std::copy (workBuffer.begin(), workBuffer.begin() + fftSize, destFrame);
        }
    }

    // Set guard samples for all levels and frames
    table.setGuardSamples();
}

void WavetableGenerator::generateMipmapsForFrame (WavetableData& table, int frameIndex)
{
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft (fftOrder);
    std::vector<float> fftBuffer (fftSize * 2, 0.0f);
    std::vector<float> workBuffer (fftSize * 2, 0.0f);

    // Copy level 0 frame data into FFT buffer
    const float* srcFrame = table.getFrameData (0, frameIndex);
    std::copy (srcFrame, srcFrame + fftSize, fftBuffer.begin());
    std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

    fft.performRealOnlyForwardTransform (fftBuffer.data(), false);

    for (int level = 0; level < WavetableData::kNumMipmapLevels; ++level)
    {
        int maxHarmonic = (fftSize / 2) >> level;

        std::copy (fftBuffer.begin(), fftBuffer.end(), workBuffer.begin());

        workBuffer[0] = 0.0f;
        workBuffer[1] = 0.0f;

        for (int bin = maxHarmonic + 1; bin <= fftSize / 2; ++bin)
        {
            workBuffer[bin * 2] = 0.0f;
            workBuffer[bin * 2 + 1] = 0.0f;
        }

        for (int bin = 1; bin < fftSize / 2; ++bin)
        {
            if (bin > maxHarmonic)
            {
                int negBin = fftSize - bin;
                if (negBin < fftSize)
                {
                    workBuffer[negBin * 2] = 0.0f;
                    workBuffer[negBin * 2 + 1] = 0.0f;
                }
            }
        }

        fft.performRealOnlyInverseTransform (workBuffer.data());

        float* destFrame = table.getFrameData (level, frameIndex);
        std::copy (workBuffer.begin(), workBuffer.begin() + fftSize, destFrame);

        // Set guard sample for this frame at this level
        table.setSample (level, frameIndex, WavetableData::kTableSize,
                         table.getSample (level, frameIndex, 0));
    }
}
