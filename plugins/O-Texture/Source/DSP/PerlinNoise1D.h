/*
  ==============================================================================

    PerlinNoise1D.h
    O-Texture - 1D Value Noise with Quintic Interpolation
    Ouaricon Audio

  ==============================================================================
*/

#pragma once

#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>

template <int NumChannels = 28>
class PerlinNoise1D
{
public:
    PerlinNoise1D()
    {
        setSeed(0);
        reset();
    }

    void setSeed(uint32_t seed)
    {
        currentSeed = seed;
        buildPermutationTable(seed);

        for (size_t ch = 0; ch < static_cast<size_t>(NumChannels); ++ch)
            channelOffsets[ch] = static_cast<uint8_t>((ch * 37 + 7 + seed) & 0xFF);
    }

    void reset()
    {
        cursors.fill(0.0f);
        cachedValues.fill(0.0f);

        for (size_t ch = 0; ch < static_cast<size_t>(NumChannels); ++ch)
            cachedValues[ch] = evaluateNoise(cursors[ch], channelOffsets[ch]);
    }

    void setSpeed(float evolveParam)
    {
        float clamped = std::clamp(evolveParam, 0.0f, 1.0f);
        stepPerHop = clamped * clamped * maxStepPerHop;
    }

    void setMaxStepPerHop(float maxStep) { maxStepPerHop = maxStep; }

    void advance(bool freeze = false)
    {
        if (freeze || stepPerHop <= 0.0f)
            return;

        for (size_t ch = 0; ch < static_cast<size_t>(NumChannels); ++ch)
        {
            cursors[ch] += stepPerHop;
            cachedValues[ch] = evaluateNoise(cursors[ch], channelOffsets[ch]);
        }
    }

    float getValue(int channel) const
    {
        return cachedValues[static_cast<size_t>(channel)];
    }

    const std::array<float, NumChannels>& getAllValues() const { return cachedValues; }
    const std::array<float, NumChannels>& getCursors() const { return cursors; }

    void setCursors(const std::array<float, NumChannels>& newCursors)
    {
        cursors = newCursors;
        for (size_t ch = 0; ch < static_cast<size_t>(NumChannels); ++ch)
            cachedValues[ch] = evaluateNoise(cursors[ch], channelOffsets[ch]);
    }

    uint32_t getSeed() const { return currentSeed; }
    static constexpr int getNumChannels() { return NumChannels; }

private:
    float evaluateNoise(float x, uint8_t offset) const
    {
        int i0 = fastFloor(x);
        float t = x - static_cast<float>(i0);

        float v0 = hashToFloat(hashAt(i0, offset));
        float v1 = hashToFloat(hashAt(i0 + 1, offset));

        float u = quinticFade(t);
        float result = lerp(v0, v1, u);

        return result * 2.0f - 1.0f;
    }

    static inline int fastFloor(float x)
    {
        int xi = static_cast<int>(x);
        return (x < static_cast<float>(xi)) ? xi - 1 : xi;
    }

    static inline float quinticFade(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static inline float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    static inline float hashToFloat(uint8_t h)
    {
        return static_cast<float>(h) / 255.0f;
    }

    inline uint8_t hashAt(int position, uint8_t offset) const
    {
        uint8_t index = static_cast<uint8_t>(position & 0xFF);
        return perm[static_cast<uint8_t>(index + offset)];
    }

    void buildPermutationTable(uint32_t seed)
    {
        for (size_t i = 0; i < 256; ++i)
            perm[i] = static_cast<uint8_t>(i);

        uint32_t state = seed;
        for (size_t i = 255; i > 0; --i)
        {
            state = state * 1664525u + 1013904223u;
            size_t j = static_cast<size_t>((state >> 16) % static_cast<uint32_t>(i + 1));
            uint8_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }

        for (size_t i = 0; i < 256; ++i)
            perm[i + 256] = perm[i];
    }

    std::array<uint8_t, 512> perm{};
    std::array<float, NumChannels> cursors{};
    std::array<float, NumChannels> cachedValues{};
    std::array<uint8_t, NumChannels> channelOffsets{};

    float stepPerHop = 0.006f;
    float maxStepPerHop = 0.02f;
    uint32_t currentSeed = 0;
};
