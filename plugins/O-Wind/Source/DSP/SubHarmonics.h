/*
  ==============================================================================

    SubHarmonics.h
    O-Wind - Sub-Octave Generator via Asymmetric Clipping
    Ouaricon Audio
    Developer: Taylor Brook

    Frequency-halving via asymmetric soft-clipping in the bore feedback path.
    Positive half-cycle passes through; negative half-cycle is reduced.
    Creates period-doubling effect for sub-octave content.

    NOTE: The sub-harmonics processing is integrated directly into
    BoreWaveguide::processSample() for optimal signal flow. This header
    documents the algorithm and provides a standalone utility if needed.

  ==============================================================================
*/

#pragma once

namespace SubHarmonics
{
    // Apply asymmetric soft-clipping for sub-octave generation.
    // amount: 0 = bypass, 1 = full sub-octave
    inline float processSample (float input, float amount)
    {
        if (amount <= 0.0f)
            return input;

        // Asymmetric: positive half passes through, negative half reduced
        float asymmetric = (input >= 0.0f) ? input : input * 0.5f;

        // Blend with original
        return input + amount * (asymmetric - input);
    }
}
