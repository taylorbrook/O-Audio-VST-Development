/*
  ==============================================================================

    O-simpleGrain - Window LUTs

    Five precomputed grain-envelope tables (rectangular / triangular / Welch /
    Gaussian / Hann), each LUT_SIZE points, built once at construction and read
    with linear interpolation by grain phase 0..1. No per-sample transcendental
    in the grain loop (the O-simpleAdditive table rationale — RESEARCH §2.4).

    Shape index meaning (matches the windowShape AudioParameterChoice order in
    PluginProcessor.cpp: "Rectangular","Triangular","Welch","Gaussian","Hann"):
        0 = rectangular  (flat 1.0 — clicks at hard edges, the teaching artifact)
        1 = triangular   1 - |2φ-1|
        2 = Welch        1 - (2φ-1)^2          (parabolic)
        3 = Gaussian     exp(-0.5((φ-0.5)/σ)^2), σ≈0.18, normalized to 1.0 centre
        4 = Hann         0.5(1 - cos(2πφ))     (default)

    LUTs are sample-rate-independent (indexed by phase). Build once; never on the
    audio thread.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

//==============================================================================
class WindowLuts
{
public:
    static constexpr int kNumShapes = 5;

    // Construct with the LUT length (PluginProcessor::kWindowLutSize = 2048).
    explicit WindowLuts (int lutSize = 2048)
        : size (juce::jmax (2, lutSize))
    {
        build();
    }

    // Read the envelope value for shape ∈ [0,4] at phase ∈ [0,1], linear-interp.
    // Hot-path: clamp + one table lookup + one lerp. No transcendental.
    float read (int shape, float phase) const noexcept
    {
        const int s = juce::jlimit (0, kNumShapes - 1, shape);
        const float p = juce::jlimit (0.0f, 1.0f, phase);

        const float fpos = p * (float) (size - 1);
        const int   i0   = (int) fpos;
        const int   i1   = juce::jmin (i0 + 1, size - 1);
        const float frac = fpos - (float) i0;

        const auto& t = tables[(size_t) s];
        return t[(size_t) i0] + frac * (t[(size_t) i1] - t[(size_t) i0]);
    }

    int getSize() const noexcept { return size; }

private:
    void build()
    {
        for (auto& t : tables)
            t.assign ((size_t) size, 0.0f);

        const float twoPi = juce::MathConstants<float>::twoPi;
        constexpr float sigma = 0.18f;
        // Gaussian is normalized so its centre (φ=0.5) reads 1.0; the un-normalized
        // value at the centre is exp(0) = 1 already, but the edges taper toward
        // exp(-0.5(0.5/σ)^2) — we keep the raw shape (centre == 1.0).
        for (int i = 0; i < size; ++i)
        {
            const float phi = (float) i / (float) (size - 1);   // 0..1 inclusive

            tables[0][(size_t) i] = 1.0f;                                          // rectangular
            tables[1][(size_t) i] = 1.0f - std::abs (2.0f * phi - 1.0f);           // triangular
            {
                const float u = 2.0f * phi - 1.0f;
                tables[2][(size_t) i] = 1.0f - u * u;                              // Welch
            }
            {
                const float d = (phi - 0.5f) / sigma;
                tables[3][(size_t) i] = std::exp (-0.5f * d * d);                  // Gaussian (centre=1)
            }
            tables[4][(size_t) i] = 0.5f * (1.0f - std::cos (twoPi * phi));        // Hann
        }
    }

    int size;
    std::array<std::vector<float>, kNumShapes> tables {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowLuts)
};
