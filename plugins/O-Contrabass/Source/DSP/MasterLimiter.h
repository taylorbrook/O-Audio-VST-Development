/*
  ==============================================================================

    MasterLimiter.h
    O-Contrabass — Zero-Latency Feedforward Stereo-Linked Limiter
    Ouaricon Audio                            (Phase 2.6a R39b)
    Developer: Taylor Brook

    Hand-written feedforward limiter per RESEARCH §22.3.3 (Option C) +
    PLAN rev-13 R39b locked design contract. Implements ARCHITECTURE §527
    verbatim algorithm:

        env  = coeff · env + (1 - coeff) · |x|
        gain = (env > threshold) ? threshold / env : 1.0

    Stereo-linked envelope (`max(|L|, |R|)`) — apply common gain to both
    channels so stereo imaging is preserved at clip-down events.

    Coefficients per CONTEXT rev-11 Q4 LOCKED (NOT ARCHITECTURE §177–179):
      - attack  = 3 ms   →  attackCoeff  = exp(-1 / (0.003 · sr))
      - release = 50 ms  →  releaseCoeff = exp(-1 / (0.050 · sr))
      - ceiling = -0.3 dBFS default (range [-6, 0])

    Public API:
      - void prepare(double sampleRate)
      - void reset()
      - void setCeilingDb(float dB)               [-6, 0] dB
      - void processBlock(juce::AudioBuffer<float>& buffer)

    Determinism: std::exp + scalar math; no juce::Random.
    RT-safety:   no allocations, no locks, no I/O.
    Latency:     0 algorithmic (no look-ahead, no delay line) —
                 PERF-03 invariant preserved.

  ==============================================================================
*/

#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MasterLimiter
{
public:
    void prepare (double sampleRate)
    {
        const double sr = (sampleRate > 0.0) ? sampleRate : 48000.0;

        attackCoeff  = static_cast<float> (std::exp (-1.0 / (0.003 * sr)));
        releaseCoeff = static_cast<float> (std::exp (-1.0 / (0.050 * sr)));

        env = 0.0f;

        ceilingSmoothed.reset (sr, 0.030);  // 30 ms ramp on ceiling
        ceilingSmoothed.setCurrentAndTargetValue (ceilingLinear);
    }

    void reset()
    {
        env = 0.0f;
        ceilingSmoothed.setCurrentAndTargetValue (ceilingLinear);
    }

    void setCeilingDb (float dB) noexcept
    {
        const float clamped = juce::jlimit (-6.0f, 0.0f, dB);
        ceilingLinear       = juce::Decibels::decibelsToGain (clamped);
        ceilingSmoothed.setTargetValue (ceilingLinear);
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numChannels == 0 || numSamples == 0)
            return;

        if (numChannels == 1)
        {
            float* L = buffer.getWritePointer (0);

            for (int i = 0; i < numSamples; ++i)
            {
                const float threshold = ceilingSmoothed.getNextValue();
                const float absMax    = std::abs (L[i]);
                const float coeff     = (absMax > env) ? attackCoeff : releaseCoeff;

                env = coeff * env + (1.0f - coeff) * absMax;

                const float gain = (env > threshold) ? (threshold / env) : 1.0f;
                L[i] *= gain;
            }
        }
        else
        {
            float* L = buffer.getWritePointer (0);
            float* R = buffer.getWritePointer (1);

            for (int i = 0; i < numSamples; ++i)
            {
                const float threshold = ceilingSmoothed.getNextValue();
                const float absMax    = juce::jmax (std::abs (L[i]), std::abs (R[i]));
                const float coeff     = (absMax > env) ? attackCoeff : releaseCoeff;

                env = coeff * env + (1.0f - coeff) * absMax;

                const float gain = (env > threshold) ? (threshold / env) : 1.0f;
                L[i] *= gain;
                R[i] *= gain;
            }
        }
    }

private:
    // Stereo-linked envelope (single scalar). `envR` reserved for future
    // per-channel mode (deferred to v1.1).
    float env { 0.0f };

    // Default -0.3 dBFS per CONTEXT rev-11 Q4 LOCKED.
    float ceilingLinear { juce::Decibels::decibelsToGain (-0.3f) };

    juce::SmoothedValue<float> ceilingSmoothed { juce::Decibels::decibelsToGain (-0.3f) };

    // Filled in prepare() — recomputed on sample-rate change.
    float attackCoeff  { 0.0f };
    float releaseCoeff { 0.0f };
};
