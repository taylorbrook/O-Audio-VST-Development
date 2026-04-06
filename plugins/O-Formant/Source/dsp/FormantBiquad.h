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

        return output * gain;
    }

    void setCoefficients (const std::array<float, 6>& coeffs) noexcept
    {
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
