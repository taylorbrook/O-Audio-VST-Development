/*
  ==============================================================================

    Exciter.cpp
    Modal Synthesis Bassoon - 5 ms half-sine × exp impulse exciter
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "Exciter.h"

void Exciter::prepare (double sampleRate)
{
    const int N = std::min (static_cast<int> (MAX_ONSET_SAMPLES),
                            static_cast<int> (sampleRate * DURATION_MS * 0.001));
    onsetSamples = N;

    for (int i = 0; i < N; ++i)
    {
        const float t      = static_cast<float> (i) / static_cast<float> (sampleRate);
        const float window = std::sin (juce::MathConstants<float>::pi
                                       * static_cast<float> (i) / static_cast<float> (N));
        const float decay  = std::exp (-t / (TAU_MS * 0.001f));
        onsetBuffer[static_cast<size_t> (i)] = window * decay;
    }

    // Normalise peak to 1.0
    float peak = 0.0f;
    for (int i = 0; i < N; ++i)
        peak = std::max (peak, std::abs (onsetBuffer[static_cast<size_t> (i)]));
    if (peak > 1e-6f)
        for (int i = 0; i < N; ++i)
            onsetBuffer[static_cast<size_t> (i)] /= peak;

    reset();
}
