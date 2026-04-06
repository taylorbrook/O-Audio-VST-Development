/*
  ==============================================================================

    GlottalWavetable.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Mipmapped wavetable storage for LF glottal pulse waveforms.
    Flat vector layout: [level][rdStep][sample+guard]
    Adapted from O-Prism WavetableData pattern.

  ==============================================================================
*/

#pragma once
#include <vector>
#include <cstddef>

struct GlottalWavetable
{
    static constexpr int kTableSize = 2048;
    static constexpr int kGuardSamples = 1;
    static constexpr int kFrameSize = kTableSize + kGuardSamples; // 2049
    static constexpr int kNumRdSteps = 128;
    static constexpr int kNumMipmapLevels = 10;

    std::vector<float> data; // Flat: [level][rdStep][sample+guard]

    void allocate()
    {
        data.resize (static_cast<size_t> (kNumMipmapLevels)
                     * static_cast<size_t> (kNumRdSteps)
                     * static_cast<size_t> (kFrameSize), 0.0f);
    }

    float getSample (int level, int rdStep, int sampleIndex) const noexcept
    {
        return data[static_cast<size_t> ((level * kNumRdSteps + rdStep) * kFrameSize + sampleIndex)];
    }

    void setSample (int level, int rdStep, int sampleIndex, float value) noexcept
    {
        data[static_cast<size_t> ((level * kNumRdSteps + rdStep) * kFrameSize + sampleIndex)] = value;
    }

    float* getFrameData (int level, int rdStep) noexcept
    {
        return &data[static_cast<size_t> ((level * kNumRdSteps + rdStep) * kFrameSize)];
    }

    const float* getFrameData (int level, int rdStep) const noexcept
    {
        return &data[static_cast<size_t> ((level * kNumRdSteps + rdStep) * kFrameSize)];
    }

    void setGuardSamples() noexcept
    {
        for (int level = 0; level < kNumMipmapLevels; ++level)
        {
            for (int rdStep = 0; rdStep < kNumRdSteps; ++rdStep)
            {
                // Guard sample = copy of first sample (wraps for interpolation)
                setSample (level, rdStep, kTableSize, getSample (level, rdStep, 0));
            }
        }
    }
};
