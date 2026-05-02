/*
  ==============================================================================

    MasterSaturator.h
    O-Contrabass — Master Polynomial Saturator (Phase 2.6a R39a)
    Ouaricon Audio
    Developer: Taylor Brook

    Wet/dry polynomial saturator per RESEARCH §22.2.4 + PLAN rev-13 R39a
    locked design contract. Memoryless cubic waveshaper with Option B
    wet/dry mix:

        out = (1 - a) · in + a · (xClamp − xClamp³ / 3)
        xClamp = jlimit(-1.5, 1.5, in)

    True bypass at amount=0; full ARCHITECTURE-spec saturator at amount=1.
    Default amount = 0.5 (50% wet/dry). 30 ms zipper-free SmoothedValue
    ramp on amount.

    Public API:
      - void prepare(double sampleRate)
      - void reset()
      - void setAmount(float amount)              [0, 1]
      - float processSample(float in) noexcept
      - void processBlock(juce::AudioBuffer<float>& buffer)

    Determinism: deterministic SmoothedValue ramp; no juce::Random; no
    cross-block memory beyond the smoother target.
    RT-safety: no allocations, no locks, no I/O.
    Latency:   0 (memoryless polynomial waveshaper).

    Block-API loops getNextValue() once per sample (NOT per channel) so
    L and R stay phase-locked per sample (matches O-Bowed precedent).

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MasterSaturator
{
public:
    void prepare (double sampleRate)
    {
        amountSmoothed.reset (sampleRate, 0.030);  // 30 ms ramp
    }

    void reset()
    {
        amountSmoothed.reset (0);
    }

    void setAmount (float amount) noexcept
    {
        amountSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, amount));
    }

    // Phase 2.6a-bis Risk #22 — seed both current and target so the smoother
    // does NOT ramp at first processBlock. Used by OCBS_DISABLE_DECORRELATOR
    // bit-equivalence build path; safe to call anytime (no allocation).
    void setAmountImmediate (float amount) noexcept
    {
        amountSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, amount));
    }

    float processSample (float in) noexcept
    {
        const float a      = amountSmoothed.getNextValue();
        const float xClamp = juce::jlimit (-1.5f, 1.5f, in);
        const float wet    = xClamp - xClamp * xClamp * xClamp / 3.0f;
        return (1.0f - a) * in + a * wet;
    }

    // Block API — advance the amount smoother ONCE per sample (not per channel)
    // so L and R stay synchronized per-sample. Matches O-Bowed precedent.
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float a = amountSmoothed.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data        = buffer.getWritePointer (ch);
                const float in     = data[i];
                const float xClamp = juce::jlimit (-1.5f, 1.5f, in);
                const float wet    = xClamp - xClamp * xClamp * xClamp / 3.0f;
                data[i]            = (1.0f - a) * in + a * wet;
            }
        }
    }

private:
    juce::SmoothedValue<float> amountSmoothed { 0.0f };
};
