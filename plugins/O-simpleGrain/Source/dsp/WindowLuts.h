/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-simpleGrain - Window LUTs

    Five precomputed grain-envelope tables (rectangular / triangular / Welch /
    Gaussian / Hann), each LUT_SIZE points, built once at construction and read
    with linear interpolation by grain phase 0..1. No per-sample transcendental
    in the grain loop (the O-simpleAdditive table rationale — RESEARCH §2.4).

    Shape index meaning (matches the windowShape AudioParameterChoice order in
    PluginProcessor.cpp: "Rectangular","Triangular","Welch","Gaussian","Hann","Tukey"):
        0 = rectangular  flat 1.0 with a fixed kRectGuardMs fade at each edge
                         (v1.4.0 — the bare step clicked; read via the Tukey remap)
        1 = triangular   1 - |2φ-1|
        2 = Welch        1 - (2φ-1)^2          (parabolic)
        3 = Gaussian     exp(-0.5((φ-0.5)/σ)^2), σ≈0.18, normalized to 1.0 centre
        4 = Hann         0.5(1 - cos(2πφ))     (default)
        5 = Tukey        flat top, Hann-shaped taper of α/2 at each edge (v1.4.0)

    Tukey is NOT a sixth table. Its taper IS a Hann half, so with taperEnd = α/2:
        u = min(φ, 1-φ)                      distance to the nearest edge
        r = min(u / taperEnd, 1) * 0.5       [0, 0.5] — flat top at exactly 0.5
        w = Hann(r)                          Hann(0.5) == 1.0 → true unity plateau
    One phase remap into the existing Hann table serves every α (and the rect
    guard, which is the same remap with taperEnd = guardSamples / grainLength).

    LUTs are sample-rate-independent (indexed by phase). Build once; never on the
    audio thread.

    CONTRACT (IN-05): the closed-form window formulas, the Gaussian σ, the Tukey
    remap, kRectGuardMs and taperEndFor() are re-implemented in the UI's
    window-envelope inset — app.js windowValue() / GAUSS_SIGMA / RECT_GUARD_MS /
    taperEndFor() (an accepted design decision: the inset recomputes JS-side
    rather than pushing tables). Any change here MUST be mirrored there.

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
    static constexpr int kNumTables = 5;     // rect/tri/Welch/Gauss/Hann tables
    static constexpr int kNumShapes = 6;     // + Tukey (remapped, no table)
    static constexpr int kShapeRect  = 0;
    static constexpr int kShapeHann  = 4;
    static constexpr int kShapeTukey = 5;

    // Rectangular guard fade, milliseconds per edge (v1.4.0). Short enough to
    // leave the "hard-edged" character (the pedagogical contrast with Hann is
    // still >1.5x top-octave energy in the render harness) while removing the
    // full-scale step that clicked.
    static constexpr float kRectGuardMs = 1.0f;

    // Taper end in phase units [0, 0.5] for a grain of lenSamp samples. Only
    // rect (fixed guard) and Tukey (α/2, floored at the guard so α=0 does not
    // reintroduce the click) use it; the other shapes ignore it.
    static float taperEndFor (int shape, float alpha, float lenSamp, double sampleRate) noexcept
    {
        const float guard = (kRectGuardMs * 0.001f * (float) sampleRate) / juce::jmax (1.0f, lenSamp);
        float te = guard;
        if (shape == kShapeTukey)
            te = juce::jmax (guard, 0.5f * juce::jlimit (0.0f, 1.0f, alpha));
        return juce::jlimit (1.0e-4f, 0.5f, te);
    }

    // Construct with the LUT length (PluginProcessor::kWindowLutSize = 2048).
    explicit WindowLuts (int lutSize = 2048)
        : size (juce::jmax (2, lutSize))
    {
        build();
    }

    // Read the envelope value for shape ∈ [0,5] at phase ∈ [0,1], linear-interp.
    // Hot-path: clamp + one table lookup + one lerp. No transcendental. Rect and
    // Tukey remap the phase into the Hann table (taperEnd from taperEndFor()).
    float read (int shape, float phase, float taperEnd = 0.5f) const noexcept
    {
        int s = juce::jlimit (0, kNumShapes - 1, shape);
        float p = juce::jlimit (0.0f, 1.0f, phase);

        if (s == kShapeRect || s == kShapeTukey)
        {
            const float u  = juce::jmin (p, 1.0f - p);
            const float te = juce::jmax (1.0e-4f, taperEnd);
            p = juce::jmin (u / te, 1.0f) * 0.5f;
            s = kShapeHann;
        }

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
    std::array<std::vector<float>, kNumTables> tables {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowLuts)
};
