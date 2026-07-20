/*
  ==============================================================================

    FormantBiquad.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Cache-friendly biquad filter struct (32 bytes).
    Direct Form II Transposed topology with per-filter gain.

  ==============================================================================
*/

#pragma once
#include <array>
#include <cmath>

struct FormantBiquad
{
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
    float gain = 1.0f;

    // WR-06: belt-and-suspenders denormal offset. Added then subtracted from the
    // feedback state each sample so a subnormal is rounded to exactly 0 while
    // normal-range state is unchanged (the offset is lost in the mantissa).
    static constexpr float kDenormalOffset = 1.0e-20f;

    // Direct Form II Transposed -- hot path, always inlined
    inline float processSample (float input) noexcept
    {
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;

        // Protect against NaN/Inf from unstable coefficients during rapid automation
        if (! std::isfinite (z1) || ! std::isfinite (z2))
        {
            z1 = 0.0f;
            z2 = 0.0f;
            return 0.0f;
        }

        // WR-06: cheap denormal flush on the feedback state. The whole voice
        // render runs inside juce::ScopedNoDenormals (PluginProcessor::processBlock),
        // which is the primary FTZ/DAZ guard; this is defence-in-depth so the
        // long decaying resonator tails (r up to ~0.9999 in the cascade / nasal
        // banks) can't trap the CPU on x86 if this filter is ever driven outside
        // that scope. Denormals are finite, so the isfinite check above misses them.
        z1 += kDenormalOffset; z1 -= kDenormalOffset;
        z2 += kDenormalOffset; z2 -= kDenormalOffset;

        return output * gain;
    }

    void setCoefficients (const std::array<float, 6>& coeffs) noexcept
    {
        // WR-05: validate before committing. If any incoming coefficient is
        // non-finite (e.g. a NaN formant frequency from an upstream overflow),
        // keep the last-known-good set rather than poisoning the filter — a
        // poisoned coefficient makes processSample re-trip the NaN guard and
        // output 0 on every subsequent sample until the next valid block-rate
        // update, turning a one-sample transient into sticky per-formant silence.
        for (int i = 0; i < 6; ++i)
            if (! std::isfinite (coeffs[i]))
                return;

        b0 = coeffs[0];
        b1 = coeffs[1];
        b2 = coeffs[2];
        // coeffs[3] is a0 (always 1.0 from JUCE ArrayCoefficients)
        a1 = coeffs[4];
        a2 = coeffs[5];
    }

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }
};
