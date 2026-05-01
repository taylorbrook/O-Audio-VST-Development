/*
  ==============================================================================

    StereoWidth.h
    O-Contrabass — Allpass Decorrelator + Mid-Side Width (Phase 2.6a R39c)
    Ouaricon Audio
    Developer: Taylor Brook

    Direct port of O-Wind StereoWidth pattern (also shared with O-Bowed)
    per RESEARCH §22.4.3 + PLAN rev-13 R39c locked design contract.
    Creates stereo from mono voice output via allpass decorrelation on
    R channel + mid-side width control. Applied post-saturator and
    post-limiter in the master output chain.

    Voice writes mono L=R clone. Decorrelator coefficients:
        makeAllPass(sampleRate, 800 Hz, Q = 0.7)
    M/S width range [0, 2], default 1.0, 20 ms SmoothedValue ramp.

    Public API:
      - void prepare(double sampleRate, int maxBlockSize)
      - void reset()
      - void setWidth(float w)                    [0, 2]
      - void processBlock(juce::AudioBuffer<float>& buffer)

    Determinism: juce::dsp::IIR::Filter is deterministic (verified at
                 Phase 2.5 R37d 3-trial bit-stability for body resonator).
    RT-safety:   no allocations, no locks, no I/O. IIR processSample +
                 scalar math only.
    Latency:     0 algorithmic (allpass IIR has frequency-dependent
                 group delay, NOT algorithmic latency reported via
                 setLatencySamples()) — matches O-Wind precedent.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class StereoWidth
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (maxBlockSize),
            1
        };
        decorrelator.prepare (spec);
        decorrelator.coefficients =
            juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 800.0f, 0.7f);
        widthSmoothed.reset (sampleRate, 0.020);  // 20 ms ramp (matches O-Wind)
    }

    void reset()
    {
        decorrelator.reset();
        widthSmoothed.reset (1.0f);
    }

    void setWidth (float w) noexcept
    {
        widthSmoothed.setTargetValue (juce::jlimit (0.0f, 2.0f, w));
    }

    // Process stereo buffer in-place.
    // Input:  channel 0 has mono voice output, channel 1 = copy of channel 0.
    // Output: decorrelated stereo with M/S width applied.
    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 2)
            return;

        auto* leftData  = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);

        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            const float mono  = leftData[i];

            // Stereo via allpass decorrelation on R channel.
            const float left  = mono;
            const float right = decorrelator.processSample (mono);

            // Mid-side width.
            const float w     = widthSmoothed.getNextValue();
            const float mid   = (left + right) * 0.5f;
            const float side  = (left - right) * 0.5f * w;

            leftData[i]  = mid + side;
            rightData[i] = mid - side;
        }
    }

private:
    juce::dsp::IIR::Filter<float>  decorrelator;
    juce::SmoothedValue<float>     widthSmoothed { 1.0f };
};
