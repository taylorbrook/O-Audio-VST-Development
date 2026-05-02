/*
  ==============================================================================

    Exciter.cpp
    Modal Synthesis Bassoon - dual-shape attack-character morph exciter
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "Exciter.h"

void Exciter::prepare (double sampleRate)
{
    // Phase 2.1 softShape — half-sine × exp envelope.
    // rev-5 (verify-phase, DSP-05 audibility): extended to 30 ms + LP-filtered to give a
    // woody pad onset audibly distinct from tonguedShape's sharp 7.5 ms noise burst.
    const int softN = std::min (static_cast<int> (MAX_ONSET_SAMPLES),
                                static_cast<int> (sampleRate * SOFT_DURATION_MS * 0.001));

    for (int i = 0; i < softN; ++i)
    {
        const float t      = static_cast<float> (i) / static_cast<float> (sampleRate);
        const float window = std::sin (juce::MathConstants<float>::pi
                                       * static_cast<float> (i) / static_cast<float> (softN));
        const float decay  = std::exp (-t / (SOFT_TAU_MS * 0.001f));
        softShape[static_cast<size_t> (i)] = window * decay;
    }

    // rev-5: 1-pole LP @ 600 Hz on softShape (BEFORE peak-normalise so gain compensates LP attenuation).
    // Coefficient: a = 1 - exp(-2π·fc/fs); state-variable: y[n] = y[n-1] + a·(x[n] - y[n-1]).
    {
        const float lpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi
                                                * SOFT_LP_FREQ_HZ
                                                / static_cast<float> (sampleRate));
        float lpState = 0.0f;
        for (int i = 0; i < softN; ++i)
        {
            lpState += lpCoeff * (softShape[static_cast<size_t> (i)] - lpState);
            softShape[static_cast<size_t> (i)] = lpState;
        }
    }

    // Peak-normalise softShape
    float softPeak = 0.0f;
    for (int i = 0; i < softN; ++i)
        softPeak = std::max (softPeak, std::abs (softShape[static_cast<size_t> (i)]));
    if (softPeak > 1e-6f)
        for (int i = 0; i < softN; ++i)
            softShape[static_cast<size_t> (i)] /= softPeak;

    // Phase 2.4 tonguedShape — 7.5 ms exp-decay × white noise (NEW).
    // Deterministic seed (OQ#3-rev-4) — same shape every prepare() across runs.
    const int tonguedN = std::min (static_cast<int> (MAX_ONSET_SAMPLES),
                                   static_cast<int> (sampleRate * TONGUED_DURATION_MS * 0.001));

    juce::Random rng (12345);
    for (int i = 0; i < tonguedN; ++i)
    {
        const float noise = rng.nextFloat() * 2.0f - 1.0f;     // [-1, 1]
        const float decay = std::exp (-static_cast<float> (i)
                                      / static_cast<float> (tonguedN) * 4.0f);  // 4 time-constants
        tonguedShape[static_cast<size_t> (i)] = noise * decay;
    }

    // Peak-normalise tonguedShape
    float tonguedPeak = 0.0f;
    for (int i = 0; i < tonguedN; ++i)
        tonguedPeak = std::max (tonguedPeak, std::abs (tonguedShape[static_cast<size_t> (i)]));
    if (tonguedPeak > 1e-6f)
        for (int i = 0; i < tonguedN; ++i)
            tonguedShape[static_cast<size_t> (i)] /= tonguedPeak;

    // Onset window covers the longer of the two shapes; std::array zero-init
    // pads softShape beyond softN automatically (D2-rev-4).
    onsetSamples = std::max (softN, tonguedN);

    reset();
}
