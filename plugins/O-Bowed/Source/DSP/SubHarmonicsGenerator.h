/*
  ==============================================================================

    SubHarmonicsGenerator.h
    O-Bowed - Sub-Harmonics via Asymmetric Waveshaping
    Ouaricon Audio
    Developer: Taylor Brook

    Asymmetric tanh waveshaper that generates sub-octave content
    by applying different saturation to positive and negative peaks.
    No state -- purely combinatorial.

  ==============================================================================
*/

#pragma once
#include <cmath>

class SubHarmonicsGenerator
{
public:
    // Process a single sample with sub-harmonics amount (0-1).
    // No state -- const noexcept.
    float process (float input, float amount) const noexcept
    {
        if (amount < 0.001f)
            return input;

        float depth = amount * 3.0f;

        // Asymmetric waveshaping: different saturation for +/- peaks
        float shaped;
        if (input >= 0.0f)
            shaped = std::tanh (input * (1.0f + depth));        // Harder clip on positive
        else
            shaped = std::tanh (input * (1.0f + depth * 0.3f)); // Softer clip on negative

        // Wet/dry blend
        return input + amount * (shaped - input);
    }
};
