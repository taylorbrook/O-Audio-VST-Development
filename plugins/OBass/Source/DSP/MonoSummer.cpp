#include "MonoSummer.h"
#include <algorithm>

void MonoSummer::prepare(int maxBlockSize)
{
    // Pre-allocate balance storage for worst-case block size
    balanceRatios.resize(static_cast<size_t>(maxBlockSize), 0.5f);
    balanceCaptured = false;
}

void MonoSummer::reset()
{
    std::fill(balanceRatios.begin(), balanceRatios.end(), 0.5f);
    balanceCaptured = false;
}

void MonoSummer::captureBalance(const juce::AudioBuffer<float>& stereoInput)
{
    jassert(stereoInput.getNumChannels() >= 2);

    const int numSamples = stereoInput.getNumSamples();
    const float* left = stereoInput.getReadPointer(0);
    const float* right = stereoInput.getReadPointer(1);

    // Ensure we have enough storage
    if (balanceRatios.size() < static_cast<size_t>(numSamples))
        balanceRatios.resize(static_cast<size_t>(numSamples));

    for (int i = 0; i < numSamples; ++i)
    {
        float absLeft = std::abs(left[i]);
        float absRight = std::abs(right[i]);
        float sum = absLeft + absRight;

        // Avoid division by zero; default to center
        if (sum > 1e-10f)
            balanceRatios[static_cast<size_t>(i)] = absLeft / sum;
        else
            balanceRatios[static_cast<size_t>(i)] = 0.5f;
    }

    balanceCaptured = true;
}

void MonoSummer::sumToMono(const juce::AudioBuffer<float>& stereoInput,
                           juce::AudioBuffer<float>& monoOutput)
{
    jassert(stereoInput.getNumChannels() >= 2);
    jassert(monoOutput.getNumChannels() >= 1);
    jassert(stereoInput.getNumSamples() == monoOutput.getNumSamples());

    const int numSamples = stereoInput.getNumSamples();
    const float* left = stereoInput.getReadPointer(0);
    const float* right = stereoInput.getReadPointer(1);
    float* mono = monoOutput.getWritePointer(0);

    // Simple (L+R)/2 sum
    for (int i = 0; i < numSamples; ++i)
    {
        mono[i] = (left[i] + right[i]) * 0.5f;
    }
}

void MonoSummer::expandToStereo(const juce::AudioBuffer<float>& monoInput,
                                 juce::AudioBuffer<float>& stereoOutput)
{
    jassert(monoInput.getNumChannels() >= 1);
    jassert(stereoOutput.getNumChannels() >= 2);
    jassert(monoInput.getNumSamples() == stereoOutput.getNumSamples());

    const int numSamples = monoInput.getNumSamples();
    const float* mono = monoInput.getReadPointer(0);
    float* left = stereoOutput.getWritePointer(0);
    float* right = stereoOutput.getWritePointer(1);

    // Simple mono to stereo - same signal to both channels
    // Avoids balance-matching complexity that could cause artifacts
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = mono[i];
        right[i] = mono[i];
    }
}
