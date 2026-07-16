/*
  ==============================================================================

    O-simplePhysicalModelSynth - OnePoleLPF

    Bilinear-transform one-pole low-pass, PORT VERBATIM from O-Lyrica
    (WaveguideString.h:191-216). Trivially-copyable POD: the plain state layout
    lets a live filter snapshot into a shadow copy with a simple assignment
    (coeffs + state) for click-free crossfading on cutoff changes.

    Shared by the KS loop damping (StringResonator), the Pluck brightness filter,
    and the Strike hardness filter — one definition, no drift.

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

struct OnePoleLPF
{
    float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f, state = 0.0f;

    inline float processSample (float x) noexcept
    {
        const float y = b0 * x + state;
        state = b1 * x - a1 * y;
        return y;
    }

    void reset() noexcept { state = 0.0f; }

    void setCutoff (float cutoffHz, double sampleRate) noexcept
    {
        const double n     = std::tan (juce::MathConstants<double>::pi * (static_cast<double> (cutoffHz) / sampleRate));
        const double a0    = n + 1.0;
        const double invA0 = 1.0 / a0;
        b0 = static_cast<float> (n * invA0);
        b1 = static_cast<float> (n * invA0);
        a1 = static_cast<float> ((n - 1.0) * invA0);
    }
};
