/*
  ==============================================================================

    O-ReverseDelay - WindowLut

    Single precomputed Hann grain-envelope table (2048 points), built once at
    construction and read with linear interpolation by grain phase 0..1. No
    per-sample transcendental in the grain loop.

    Trimmed from O-simpleGrain's WindowLuts.h to Hann-only (Stage-2 PLAN
    Task 2). The 5-shape class carries a UI-mirroring contract (IN-05) that
    deliberately does NOT travel with this copy.

    LUT is sample-rate-independent (indexed by phase). Build once; never on
    the audio thread.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <vector>

class WindowLut
{
public:
    explicit WindowLut (int lutSize = 2048)
        : size (juce::jmax (2, lutSize))
    {
        table.resize (static_cast<size_t> (size));
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < size; ++i)
        {
            const float phi = static_cast<float> (i) / static_cast<float> (size - 1);  // 0..1 inclusive
            table[static_cast<size_t> (i)] = 0.5f * (1.0f - std::cos (twoPi * phi));   // Hann
        }
    }

    // Hot-path: clamp + one table lookup + one lerp. No transcendental.
    float read (float phase) const noexcept
    {
        const float p    = juce::jlimit (0.0f, 1.0f, phase);
        const float fpos = p * static_cast<float> (size - 1);
        const int   i0   = static_cast<int> (fpos);
        const int   i1   = juce::jmin (i0 + 1, size - 1);
        const float frac = fpos - static_cast<float> (i0);

        return table[static_cast<size_t> (i0)]
             + frac * (table[static_cast<size_t> (i1)] - table[static_cast<size_t> (i0)]);
    }

    int getSize() const noexcept { return size; }

private:
    int size;
    std::vector<float> table;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowLut)
};
