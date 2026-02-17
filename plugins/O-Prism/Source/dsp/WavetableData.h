/*
  ==============================================================================

    WavetableData.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <vector>
#include <cstdint>

struct WavetableData
{
    static constexpr int kTableSize = 2048;
    static constexpr int kGuardSamples = 1;
    static constexpr int kFrameSize = kTableSize + kGuardSamples; // 2049
    static constexpr int kMaxFrames = 256;
    static constexpr int kNumMipmapLevels = 10;

    int numFrames = 0;
    std::vector<float> data; // Flat: [level][frame][sample+guard]

    void allocate (int frames)
    {
        numFrames = frames;
        data.resize (static_cast<size_t> (kNumMipmapLevels) * static_cast<size_t> (numFrames) * kFrameSize, 0.0f);
    }

    float getSample (int level, int frame, int sampleIndex) const
    {
        return data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize + sampleIndex)];
    }

    void setSample (int level, int frame, int sampleIndex, float value)
    {
        data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize + sampleIndex)] = value;
    }

    float* getFrameData (int level, int frame)
    {
        return &data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize)];
    }

    const float* getFrameData (int level, int frame) const
    {
        return &data[static_cast<size_t> ((level * numFrames + frame) * kFrameSize)];
    }

    void setGuardSamples()
    {
        for (int level = 0; level < kNumMipmapLevels; ++level)
        {
            for (int frame = 0; frame < numFrames; ++frame)
            {
                // Guard sample = copy of first sample (wraps for interpolation)
                setSample (level, frame, kTableSize, getSample (level, frame, 0));
            }
        }
    }
};
