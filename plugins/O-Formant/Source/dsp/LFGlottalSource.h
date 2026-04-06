/*
  ==============================================================================

    LFGlottalSource.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice wavetable oscillator for LF glottal pulse.
    Bilinear interpolation between Rd steps and mipmap levels.
    Phase accumulator [0, 1), reads from shared GlottalWavetable.

  ==============================================================================
*/

#pragma once
#include "GlottalWavetable.h"
#include <cmath>
#include <algorithm>

class LFGlottalSource
{
public:
    void setWavetable (const GlottalWavetable* wt) noexcept
    {
        wavetable = wt;
    }

    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        phase = 0.0;
        phaseIncrement = 0.0;
    }

    void setFrequency (float f0) noexcept
    {
        frequency = f0;
        phaseIncrement = static_cast<double> (f0) / sampleRate;
    }

    void setRd (float rd) noexcept
    {
        currentRd = rd;
    }

    // Inline hot path -- bilinear interpolation (2 Rd x 2 mipmap levels)
    inline float getNextSample() noexcept
    {
        if (wavetable == nullptr)
            return 0.0f;

        // Advance phase accumulator
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        // Mipmap level from frequency
        float baseFreq = static_cast<float> (sampleRate) / static_cast<float> (GlottalWavetable::kTableSize);
        float levelFloat = std::log2 (std::max (frequency, baseFreq) / baseFreq);
        levelFloat = std::max (0.0f, std::min (levelFloat, static_cast<float> (GlottalWavetable::kNumMipmapLevels - 1)));
        int level0 = static_cast<int> (levelFloat);
        int level1 = std::min (level0 + 1, GlottalWavetable::kNumMipmapLevels - 1);
        float levelFrac = levelFloat - static_cast<float> (level0);

        // Rd index in log space
        // Pre-computed constants: log(0.3) = -1.2040, log(2.7) = 0.9933
        // Range = log(2.7) - log(0.3) = 2.1972
        static constexpr float kLogRdMin = -1.2039728f;  // std::log(0.3f)
        static constexpr float kLogRdRange = 2.1972246f;  // std::log(2.7f) - std::log(0.3f)

        float clampedRd = std::max (0.3f, std::min (currentRd, 2.7f));
        float rdNorm = (std::log (clampedRd) - kLogRdMin) / kLogRdRange;
        rdNorm = std::max (0.0f, std::min (rdNorm, 1.0f));
        float rdIndexF = rdNorm * static_cast<float> (GlottalWavetable::kNumRdSteps - 1);
        int rd0 = static_cast<int> (rdIndexF);
        int rd1 = std::min (rd0 + 1, GlottalWavetable::kNumRdSteps - 1);
        float rdFrac = rdIndexF - static_cast<float> (rd0);

        // Sample position within table
        float samplePos = static_cast<float> (phase) * static_cast<float> (GlottalWavetable::kTableSize);
        int idx0 = static_cast<int> (samplePos);
        float frac = samplePos - static_cast<float> (idx0);

        // 4 table lookups (2 Rd x 2 mipmap levels), each with linear sample interpolation
        // Guard samples handle wrap-around, so idx0+1 is always valid
        float s00 = lerp (wavetable->getSample (level0, rd0, idx0),
                          wavetable->getSample (level0, rd0, idx0 + 1), frac);
        float s01 = lerp (wavetable->getSample (level0, rd1, idx0),
                          wavetable->getSample (level0, rd1, idx0 + 1), frac);
        float v0 = lerp (s00, s01, rdFrac);

        float s10 = lerp (wavetable->getSample (level1, rd0, idx0),
                          wavetable->getSample (level1, rd0, idx0 + 1), frac);
        float s11 = lerp (wavetable->getSample (level1, rd1, idx0),
                          wavetable->getSample (level1, rd1, idx0 + 1), frac);
        float v1 = lerp (s10, s11, rdFrac);

        return lerp (v0, v1, levelFrac);
    }

    void reset() noexcept
    {
        phase = 0.0;
    }

private:
    static inline float lerp (float a, float b, float t) noexcept
    {
        return a + t * (b - a);
    }

    const GlottalWavetable* wavetable = nullptr;
    double sampleRate = 44100.0;
    float frequency = 220.0f;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float currentRd = 1.0f;
};
