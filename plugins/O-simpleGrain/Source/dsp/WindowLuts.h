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
    v2.0.0: the Blackman-Harris table (below) is internal to Pulsar mode and not
    a user shape, so the contract covers it only if the inset ever draws a
    pulsaret window (kBhCoeffs would then be mirrored in app.js too).

    v2.0.0 — a SIXTH, internal table: 4-term Blackman-Harris (kShapeBH), read
    only by Pulsar-mode pulsarets through readBH(). It is not in the user's
    Window choice: read() still clamps the shape to kNumShapes - 1, and the five
    user tables are built exactly as before, so every Cloud read is unchanged.
    Moments (continuous): μ₁ = a₀ = 0.35875, μ₂ = a₀² + (a₁² + a₂² + a₃²)/2
    = 0.25796. powerGainVsHann() / bhPowerGain() give √(P_Hann / P_w), the
    per-pulsaret level match that keeps the Hann -> BH crossfade free of a step.
    P is the power the window gives its CONTENT: μ₂ for broadband (sample)
    content, but for the one-cycle sine pulsaret it is mean(w² · sin²(2πφ)),
    which weights the window's shoulders (φ = 1/4, 3/4) — μ₂ alone would leave
    BH 1.7 dB under Hann there (measured). So each window carries both moments.

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
    static constexpr int kShapeBH    = 6;    // v2.0.0 — internal (Pulsar), NOT a user shape

    // 4-term Blackman-Harris (-92 dB sidelobes): a0 - a1 cos + a2 cos2 - a3 cos3.
    static constexpr double kBhCoeffs[4] = { 0.35875, 0.48829, 0.14128, 0.01168 };

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
        buildPulsarTables();   // v2.0.0 — after build(): the five user tables are untouched
    }

    // v2.0.0 — the Blackman-Harris table at phase ∈ [0,1], linear-interp (Pulsar
    // pulsarets only; a separate entry point so read()'s shape clamp stays put).
    float readBH (float phase) const noexcept
    {
        const float p    = juce::jlimit (0.0f, 1.0f, phase);
        const float fpos = p * (float) (size - 1);
        const int   i0   = (int) fpos;
        const int   i1   = juce::jmin (i0 + 1, size - 1);
        const float frac = fpos - (float) i0;
        return bhTable[(size_t) i0] + frac * (bhTable[(size_t) i1] - bhTable[(size_t) i0]);
    }

    // v2.0.0 — √(μ₂_Hann / μ₂_w) for a user shape at this taper: the gain that
    // gives a pulsaret of that window the power of a Hann one. The table shapes
    // use their cached μ₂; rect / Tukey are a flat top with Hann-half edges of
    // total length 2·te, so μ₂ = (1 - 2te) + 2te·μ₂_Hann (closed form — the
    // pulsar spawn runs at audio rate and cannot afford meanSquare()).
    // sineContent: the sine-weighted moment instead of μ₂ (see the header);
    // rect / Tukey then read a 65-point table over te ∈ [0, 0.5].
    float powerGainVsHann (int shape, float taperEnd, bool sineContent = false) const noexcept
    {
        const int s = juce::jlimit (0, kNumShapes - 1, shape);
        const float ref = sineContent ? hannSinMs : hannMs;
        float ms = ref;
        if (s == kShapeRect || s == kShapeTukey)
        {
            const float te = juce::jlimit (1.0e-4f, 0.5f, taperEnd);
            if (sineContent)
            {
                const float x  = te / 0.5f * (float) (kTaperSteps - 1);
                const int   i0 = juce::jlimit (0, kTaperSteps - 2, (int) x);
                const float fr = x - (float) i0;
                ms = taperSinMs[(size_t) i0] + fr * (taperSinMs[(size_t) i0 + 1] - taperSinMs[(size_t) i0]);
            }
            else
            {
                ms = (1.0f - 2.0f * te) + 2.0f * te * hannMs;
            }
        }
        else
        {
            ms = sineContent ? tableSinMs[(size_t) s] : tableMs[(size_t) s];
        }
        return std::sqrt (ref / juce::jmax (1.0e-6f, ms));
    }

    // v2.0.0 — √(P_Hann / P_BH) (cached): 1.206 broadband, 1.469 sine content.
    float bhPowerGain (bool sineContent = false) const noexcept { return sineContent ? bhSinGain : bhGain; }

    // v2.0.0 — mean / mean-square of the BH table by the midpoint rule (tests).
    float bhMean() const noexcept       { return bhMoment (1); }
    float bhMeanSquare() const noexcept { return bhMoment (2); }

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

    // Mean of w² over phase [0, phaseEnd] (phaseEnd = 1 → the power moment μ₂),
    // by midpoint rule over the same read() the voices use (v1.5.5 density
    // compensation). phaseEnd < 1 covers grains stolen part-way through their
    // window. Off the hot path: the processor caches the full-window value.
    float meanSquare (int shape, float taperEnd = 0.5f, float phaseEnd = 1.0f) const noexcept
    {
        constexpr int kSteps = 512;
        double acc = 0.0;
        for (int i = 0; i < kSteps; ++i)
        {
            const float w = read (shape, phaseEnd * ((float) i + 0.5f) / (float) kSteps, taperEnd);
            acc += (double) w * (double) w;
        }
        return (float) (acc / kSteps);
    }

    //==========================================================================
    // v1.8.0 — grain-shape TILT (research 2026 update §5 item 6, Tempera 2.3).
    // Tilt moves the window's peak along the grain: −1 = plucked (the peak at
    // the very start, a fast attack and a long fall), +1 = reverse swell (a long
    // rise, cut off just before the end), 0 = the symmetric window, untouched.
    //
    // The table shapes (tri / Welch / Gauss / Hann) are PHASE-WARPED: the rise
    // half of the window is played over [0, peak], the fall half over
    // [peak, 1]. Each half is a linear time-scale of the original, so the window
    // keeps its peak (1.0), its end values and — for these symmetric windows —
    // exactly its power μ₂: the density normalization needs no new term.
    // The remapped shapes (rect / Tukey) split their fade budget instead: the
    // attack taper is 2·peak·te and the release 2·(1−peak)·te, which is the
    // same warp for Tukey α = 1 (= Hann) and keeps the total fade length.
    //
    // CLICK FLOOR: the steep side never gets shorter than kTiltMinEdgeMs = 3 ms
    // (matches O-GrainScatter 3.4.0), whatever the grain size; the peak is
    // clamped to [guard, 1 − guard] in phase units, so a tilted extreme is never
    // steeper than the same shape's symmetric 6 ms grain (3 ms halves). Grains
    // of 6 ms or less therefore cannot tilt (guard = 0.5) on the table shapes.
    // Rect / Tukey: the floor is min(3 ms, the untilted taper), so tilt never
    // makes an edge steeper than the untilted window had it and never softens
    // a short edge either — Rectangular keeps its 1 ms guard and barely moves:
    // it has no slope to tilt.
    //
    // CONTRACT (IN-05): mirrored in app.js tiltFor() / windowValueTilted().
    static constexpr float kTiltMinEdgeMs   = 3.0f;    // v1.8.0: was the 1 ms rect guard pre-release
    static constexpr float kTiltMaxShift    = 0.48f;   // peak 0.02 .. 0.98 at ±100 % (before the floor)

    struct Tilt
    {
        float peak = 0.5f;   // phase of the window maximum (table shapes)
        float teA  = 0.5f;   // attack taper end, phase units (rect / Tukey)
        float teR  = 0.5f;   // release taper length, phase units (rect / Tukey)
    };

    // tilt ∈ [−1, 1]; taperEnd from taperEndFor(); lenSamp the grain length.
    static Tilt tiltFor (int shape, float tilt, float taperEnd, float lenSamp, double sampleRate) noexcept
    {
        const float guard = juce::jlimit (1.0e-4f, 0.5f,
            (kTiltMinEdgeMs * 0.001f * (float) sampleRate) / juce::jmax (1.0f, lenSamp));
        const float c = 0.5f + kTiltMaxShift * juce::jlimit (-1.0f, 1.0f, tilt);

        Tilt t;
        t.peak = juce::jlimit (guard, 1.0f - guard, c);
        const int s = juce::jlimit (0, kNumShapes - 1, shape);
        if (s == kShapeRect || s == kShapeTukey)
        {
            const float te    = juce::jlimit (1.0e-4f, 0.5f, taperEnd);
            const float floor = juce::jmin (guard, te);   // never softer than untilted
            t.teA = juce::jmax (floor, 2.0f * c * te);
            t.teR = juce::jmax (floor, 2.0f * (1.0f - c) * te);
            // The two tapers sum to 2·te <= 1 unless the floor lifted one of
            // them (short grains). Then the other gives way, so the window still
            // reaches 1 (no plateau-less, quieter short grain): the floored side
            // keeps its floor, the other takes the rest (floor <= 0.5 keeps both
            // >= floor).
            if (t.teA + t.teR > 1.0f)
            {
                if (t.teA <= floor) t.teR = 1.0f - t.teA;
                else                t.teA = 1.0f - t.teR;
            }
        }
        return t;
    }

    // read() with a tilt applied (only called for tilted grains — an untilted
    // grain keeps read(), so tilt 0 renders bit-identically to v1.7.0).
    float readTilted (int shape, float phase, float peak, float teA, float teR) const noexcept
    {
        int s = juce::jlimit (0, kNumShapes - 1, shape);
        float p = juce::jlimit (0.0f, 1.0f, phase);

        if (s == kShapeRect || s == kShapeTukey)
        {
            const float r = juce::jmin (p / juce::jmax (1.0e-4f, teA),
                                        (1.0f - p) / juce::jmax (1.0e-4f, teR), 1.0f);
            p = r * 0.5f;
            s = kShapeHann;
        }
        else
        {
            const float c = juce::jlimit (1.0e-4f, 1.0f - 1.0e-4f, peak);
            p = (p < c) ? 0.5f * p / c
                        : 1.0f - 0.5f * (1.0f - p) / (1.0f - c);
        }

        const float fpos = p * (float) (size - 1);
        const int   i0   = juce::jlimit (0, size - 1, (int) fpos);
        const int   i1   = juce::jmin (i0 + 1, size - 1);
        const float frac = fpos - (float) i0;

        const auto& t = tables[(size_t) s];
        return t[(size_t) i0] + frac * (t[(size_t) i1] - t[(size_t) i0]);
    }

    // meanSquare() for a tilted window (v1.8.0 density compensation).
    float meanSquareTilted (int shape, const Tilt& tilt, float phaseEnd = 1.0f) const noexcept
    {
        constexpr int kSteps = 512;
        double acc = 0.0;
        for (int i = 0; i < kSteps; ++i)
        {
            const float w = readTilted (shape, phaseEnd * ((float) i + 0.5f) / (float) kSteps,
                                        tilt.peak, tilt.teA, tilt.teR);
            acc += (double) w * (double) w;
        }
        return (float) (acc / kSteps);
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

    // v2.0.0 — Pulsar-only state, built after the user tables.
    void buildPulsarTables()
    {
        bhTable.assign ((size_t) size, 0.0f);
        const double twoPi = juce::MathConstants<double>::twoPi;
        for (int i = 0; i < size; ++i)
        {
            const double phi = (double) i / (double) (size - 1);
            bhTable[(size_t) i] = (float) (kBhCoeffs[0] - kBhCoeffs[1] * std::cos (twoPi * phi)
                                         + kBhCoeffs[2] * std::cos (2.0 * twoPi * phi)
                                         - kBhCoeffs[3] * std::cos (3.0 * twoPi * phi));
        }
        for (int s = 0; s < kNumTables; ++s)
        {
            tableMs[(size_t) s]    = meanSquare (s);
            tableSinMs[(size_t) s] = sineMoment ([&] (float p) { return read (s, p); });
        }
        for (int i = 0; i < kTaperSteps; ++i)
        {
            const float te = juce::jmax (1.0e-4f, 0.5f * (float) i / (float) (kTaperSteps - 1));
            taperSinMs[(size_t) i] = sineMoment ([&] (float p) { return read (kShapeTukey, p, te); });
        }
        hannMs    = tableMs[(size_t) kShapeHann];
        hannSinMs = tableSinMs[(size_t) kShapeHann];
        bhGain    = std::sqrt (hannMs / bhMoment (2));
        bhSinGain = std::sqrt (hannSinMs / sineMoment ([this] (float p) { return readBH (p); }));
    }

    // mean of w(φ)² · sin²(2πφ) by the midpoint rule (the one-cycle sine
    // pulsaret's power through window w).
    template <typename Fn>
    static float sineMoment (Fn&& w)
    {
        constexpr int kSteps = 512;
        double acc = 0.0;
        for (int i = 0; i < kSteps; ++i)
        {
            const float  p = ((float) i + 0.5f) / (float) kSteps;
            const double v = (double) w (p) * std::sin (juce::MathConstants<double>::twoPi * (double) p);
            acc += v * v;
        }
        return (float) (acc / kSteps);
    }

    float bhMoment (int power) const noexcept
    {
        constexpr int kSteps = 512;
        double acc = 0.0;
        for (int i = 0; i < kSteps; ++i)
        {
            const double w = (double) readBH (((float) i + 0.5f) / (float) kSteps);
            acc += (power == 1) ? w : w * w;
        }
        return (float) (acc / kSteps);
    }

    int size;
    std::array<std::vector<float>, kNumTables> tables {};
    std::vector<float> bhTable;                       // v2.0.0 — internal BH (kShapeBH)
    static constexpr int kTaperSteps = 65;
    std::array<float, kNumTables> tableMs {};         // v2.0.0 — μ₂ of each user table
    std::array<float, kNumTables> tableSinMs {};      // v2.0.0 — sine-weighted moment of each
    std::array<float, kTaperSteps> taperSinMs {};     // v2.0.0 — rect / Tukey, sine-weighted, by te
    float hannMs    = 0.375f;
    float hannSinMs = 0.15625f;
    float bhGain    = 1.206f;
    float bhSinGain = 1.469f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowLuts)
};
