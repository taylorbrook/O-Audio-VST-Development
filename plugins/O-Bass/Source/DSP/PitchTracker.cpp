/*
  ==============================================================================

    PitchTracker.cpp
    O-Bass - YIN-based Monophonic Pitch Detection

    Reference: de Cheveigne & Kawahara 2002

  ==============================================================================
*/

#include "PitchTracker.h"
#include <algorithm>

void PitchTracker::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = maxBlockSize;  // Store for runtime validation

    // Analysis window: 2 periods minimum at 30Hz (lowest expected frequency)
    // At 44.1kHz: 44100 / 30 * 2 = ~2940 samples
    windowSize = static_cast<int>(sampleRate / 30.0) * 2;

    // Cap at 4096 for performance (sufficient for bass range)
    windowSize = juce::jmin(windowSize, 4096);

    // Allocate buffers
    buffer.resize(static_cast<size_t>(windowSize), 0.0f);
    yinBuffer.resize(static_cast<size_t>(windowSize / 2), 0.0f);

    reset();
}

void PitchTracker::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    std::fill(yinBuffer.begin(), yinBuffer.end(), 0.0f);
    writeIndex = 0;
    lastPitch = 0.0f;
}

void PitchTracker::setThreshold(float threshold)
{
    yinThreshold = juce::jlimit(0.01f, 0.5f, threshold);
}

float PitchTracker::detectPitch(const float* input, int numSamples)
{
    // Accumulate samples into ring buffer
    for (int i = 0; i < numSamples; ++i)
    {
        buffer[static_cast<size_t>(writeIndex)] = input[i];
        writeIndex = (writeIndex + 1) % windowSize;
    }

    // YIN Step 1 & 2: Compute difference function d(tau) and
    // cumulative mean normalized difference d'(tau)
    const int halfWindow = windowSize / 2;
    float runningSum = 0.0f;

    yinBuffer[0] = 1.0f;  // d'(0) = 1 by definition

    for (int tau = 1; tau < halfWindow; ++tau)
    {
        // Difference function: sum of squared differences
        float delta = 0.0f;
        for (int j = 0; j < halfWindow; ++j)
        {
            // Handle ring buffer wrap-around
            int idx1 = (writeIndex + j) % windowSize;
            int idx2 = (writeIndex + j + tau) % windowSize;
            float diff = buffer[static_cast<size_t>(idx1)] - buffer[static_cast<size_t>(idx2)];
            delta += diff * diff;
        }

        runningSum += delta;

        // Cumulative mean normalized difference
        // d'(tau) = d(tau) / ((1/tau) * sum(d(1)...d(tau)))
        //         = d(tau) * tau / sum
        if (runningSum > 0.0f)
            yinBuffer[static_cast<size_t>(tau)] = delta * static_cast<float>(tau) / runningSum;
        else
            yinBuffer[static_cast<size_t>(tau)] = 1.0f;
    }

    // YIN Step 3: Find first minimum below threshold
    int tauEstimate = -1;
    for (int tau = 2; tau < halfWindow; ++tau)
    {
        if (yinBuffer[static_cast<size_t>(tau)] < yinThreshold)
        {
            // Walk down to find the local minimum
            while (tau + 1 < halfWindow &&
                   yinBuffer[static_cast<size_t>(tau + 1)] < yinBuffer[static_cast<size_t>(tau)])
            {
                ++tau;
            }
            tauEstimate = tau;
            break;
        }
    }

    // No pitch detected
    if (tauEstimate < 0)
    {
        lastPitch = 0.0f;
        return 0.0f;
    }

    // YIN Step 4: Parabolic interpolation for sub-sample accuracy
    float betterTau = parabolicInterpolation(tauEstimate);

    // Convert period to frequency
    lastPitch = static_cast<float>(currentSampleRate / static_cast<double>(betterTau));
    return lastPitch;
}

float PitchTracker::parabolicInterpolation(int tau) const
{
    // Can't interpolate at boundaries
    if (tau < 1 || tau >= static_cast<int>(yinBuffer.size()) - 1)
        return static_cast<float>(tau);

    float s0 = yinBuffer[static_cast<size_t>(tau - 1)];
    float s1 = yinBuffer[static_cast<size_t>(tau)];
    float s2 = yinBuffer[static_cast<size_t>(tau + 1)];

    // Parabolic interpolation formula
    // adjustment = (s2 - s0) / (2 * (2*s1 - s2 - s0))
    float denominator = 2.0f * (2.0f * s1 - s2 - s0);

    // Avoid division by zero or very small denominator
    if (std::abs(denominator) < 1e-10f)
        return static_cast<float>(tau);

    float adjustment = (s2 - s0) / denominator;

    return static_cast<float>(tau) + adjustment;
}
