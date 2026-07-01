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
#include <cstdint>
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
        jitterEma = 0.0f;
        shimmerEma = 0.0f;
        cyclePhaseIncrementMod = 1.0;
        cycleGainMod = 1.0f;
    }

    void setFrequency (float f0) noexcept
    {
        frequency = f0;
        // WR-04: clamp the per-sample phase increment to < 0.5 (the wavetable's
        // Nyquist). A runaway f0 (≥ sampleRate, reachable at low SR with high
        // notes) would otherwise advance phase past the guard sample in a single
        // step and over-read the frame. Combined with the while-loop wrap below,
        // phase is guaranteed to stay in [0, 1) and samplePos within the table.
        double inc = static_cast<double> (f0) / sampleRate;
        phaseIncrement = std::max (0.0, std::min (inc, 0.5));
    }

    void setRd (float rd) noexcept
    {
        currentRd = rd;
    }

    // Set jitter/shimmer amounts and note velocity for inverse scaling
    // jitterAmt: 0-1 maps to 0-2% relative f0 perturbation
    // shimmerAmt: 0-1 maps to 0-5% relative amplitude perturbation
    void setJitterShimmer (float jitterAmt, float shimmerAmt, float velocity) noexcept
    {
        jitterAmount = jitterAmt * 0.02f;
        shimmerAmount = shimmerAmt * 0.05f;
        noteVelocity = velocity;
    }

    // Seed RNG per-voice for uncorrelated perturbation patterns
    void setSeed (uint32_t seed) noexcept
    {
        rngState = seed != 0 ? seed : 1;
    }

    // Inline hot path -- bilinear interpolation (2 Rd x 2 mipmap levels)
    inline float getNextSample() noexcept
    {
        if (wavetable == nullptr)
            return 0.0f;

        // Advance phase accumulator with jitter-modified increment
        phase += phaseIncrement * cyclePhaseIncrementMod;

        // Detect cycle boundary (phase wrap) — update perturbations once per cycle
        if (phase >= 1.0)
        {
            // WR-04: while-loop wrap. A single if only corrects [1.0, 2.0); with
            // a large increment (or jitter modulation) phase could land ≥ 2.0 and
            // stay ≥ 1.0 after one subtraction, driving samplePos past the frame.
            // Loop until phase is back in [0, 1) so the table read is always safe.
            while (phase >= 1.0)
                phase -= 1.0;
            updateCyclePerturbations();
        }

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

        // Apply shimmer (per-cycle amplitude modulation)
        return lerp (v0, v1, levelFrac) * cycleGainMod;
    }

    float getPhase() const noexcept { return static_cast<float> (phase); }

    void reset() noexcept
    {
        phase = 0.0;
        jitterEma = 0.0f;
        shimmerEma = 0.0f;
        cyclePhaseIncrementMod = 1.0;
        cycleGainMod = 1.0f;
    }

private:
    static inline float lerp (float a, float b, float t) noexcept
    {
        return a + t * (b - a);
    }

    // Called once per glottal cycle at phase wrap
    void updateCyclePerturbations() noexcept
    {
        // 1/f approximation: EMA-filtered white noise (~50ms time constant)
        float cyclePeriod = 1.0f / std::max (frequency, 20.0f);
        float alpha = 1.0f - std::exp (-cyclePeriod / 0.050f);

        float randJ = nextRandom();
        float randS = nextRandom();

        jitterEma += alpha * (randJ - jitterEma);
        shimmerEma += alpha * (randS - shimmerEma);

        // Inverse pitch scaling: more perturbation at low f0 (ref 200 Hz)
        float pitchScale = std::min (200.0f / std::max (frequency, 20.0f), 2.0f);
        // Inverse velocity scaling: high velocity = more controlled
        float velScale = 1.0f - noteVelocity * 0.7f;
        float scale = pitchScale * velScale;

        cyclePhaseIncrementMod = 1.0 + static_cast<double> (jitterAmount * jitterEma * scale);
        cycleGainMod = 1.0f + shimmerAmount * shimmerEma * scale;
    }

    // Xorshift32 PRNG — fast, sufficient quality for noise perturbation
    float nextRandom() noexcept
    {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return static_cast<float> (static_cast<int32_t> (rngState)) * (1.0f / 2147483648.0f);
    }

    const GlottalWavetable* wavetable = nullptr;
    double sampleRate = 44100.0;
    float frequency = 220.0f;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float currentRd = 1.0f;

    // Jitter/shimmer state
    float jitterAmount = 0.0f;
    float shimmerAmount = 0.0f;
    float noteVelocity = 0.0f;
    float jitterEma = 0.0f;
    float shimmerEma = 0.0f;
    double cyclePhaseIncrementMod = 1.0;
    float cycleGainMod = 1.0f;
    uint32_t rngState = 2463534242u;
};
