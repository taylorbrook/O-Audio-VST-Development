/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    GlottalTableGenerator.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Offline LF glottal pulse table generation with Fant 1995 regression,
    bracketed bisection solvers for alpha/epsilon, and FFT mipmap generation.

  ==============================================================================
*/

#include "GlottalTableGenerator.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include <vector>

//==============================================================================
GlottalTableGenerator::LFTimingParams GlottalTableGenerator::computeTimingFromRd (float Rd)
{
    // Fant 1995 regression: Rd -> R-parameters
    const double rd = Rd;
    const double Ra = std::max (0.001, (-1.0 + 4.8 * rd) / 100.0);
    const double Rk = (22.4 + 11.8 * rd) / 100.0;

    // WR-01: Rg from Fant's own regression, not an ad-hoc OQ ramp. The old
    // piecewise OQ reached 0.98 at Rd 2.7 (Fant gives ~0.79), which left the
    // return phase shorter than Ta, so the epsilon equation had no root.
    const double Rg = 0.25 * Rk / (0.11 * rd / (0.5 + 1.2 * Rk) - Ra);

    // Timing (normalized to period = 1.0)
    LFTimingParams params;
    params.Tp = 1.0 / (2.0 * Rg);
    params.Te = std::min (params.Tp * (1.0 + Rk), 0.99);
    params.Tc = 1.0;

    // Keep Ta inside the return phase so solveEpsilon always has a root.
    params.Ta = juce::jlimit (0.001, 0.9 * (params.Tc - params.Te), Ra);

    return params;
}

//==============================================================================
double GlottalTableGenerator::netArea (double alpha, const LFTimingParams& p, double epsilon)
{
    // Net area of one LF period with Ee = 1 (E0 chosen so g(Te) = -1).
    // Open phase:  E0 e^{alpha t} sin(wt),  E0 = -1 / (e^{alpha Te} sin(wTe)),
    //              written with e^{-alpha Te} so it never overflows.
    // Return phase: -(e^{-eps(t-Te)} - e^{-eps(Tc-Te)}) / (eps Ta).
    const double w = juce::MathConstants<double>::pi / p.Tp;
    const double s = std::sin (w * p.Te);
    const double c = std::cos (w * p.Te);
    const double D = p.Tc - p.Te;

    const double openArea = -((alpha * s - w * c) + w * std::exp (-alpha * p.Te))
                            / ((alpha * alpha + w * w) * s);
    const double returnArea = -(1.0 / epsilon - D * std::exp (-epsilon * D) / (epsilon * p.Ta));

    return openArea + returnArea;
}

double GlottalTableGenerator::solveAlpha (const LFTimingParams& p, double epsilon)
{
    // CR-01: the old solver zeroed the open-phase integral alone, un-normalized.
    // That goes to 0 as alpha -> -inf, so Newton ran to alpha ~ -4000 for every
    // Rd <= ~0.8 and the pressed half rendered a near-silent open phase with a
    // positive spike. LF balances open + return over the whole period, and its
    // alpha is positive: netArea(0) > 0 and falls through a single root. Bisect.
    double lo = 0.0;
    double hi = 1.0;

    jassert (netArea (lo, p, epsilon) > 0.0);

    while (netArea (hi, p, epsilon) > 0.0 && hi < 1.0e6)
        hi *= 2.0;

    jassert (netArea (hi, p, epsilon) <= 0.0);

    for (int iter = 0; iter < 100; ++iter)
    {
        const double mid = 0.5 * (lo + hi);
        if (netArea (mid, p, epsilon) > 0.0)
            lo = mid;
        else
            hi = mid;
    }

    return 0.5 * (lo + hi);
}

//==============================================================================
double GlottalTableGenerator::solveEpsilon (const LFTimingParams& p)
{
    // Solve: eps * Ta = 1 - exp(-eps * (Tc - Te)).
    // f(eps) = 1 - e^{-eps D} - eps Ta is positive just above 0 (D > Ta) and
    // negative at 1/Ta, so the positive root is bracketed. Bisect.
    const double D = p.Tc - p.Te;
    jassert (D > p.Ta);

    double lo = 1.0e-9;
    double hi = 1.0 / p.Ta;

    for (int iter = 0; iter < 100; ++iter)
    {
        const double mid = 0.5 * (lo + hi);
        if (1.0 - std::exp (-mid * D) - mid * p.Ta > 0.0)
            lo = mid;
        else
            hi = mid;
    }

    return 0.5 * (lo + hi);
}

//==============================================================================
void GlottalTableGenerator::renderLFPeriod (float* buffer, int size, const LFTimingParams& params)
{
    const double Tp = params.Tp;
    const double Te = params.Te;
    const double Ta = params.Ta;
    const double Tc = params.Tc;

    const double omega_g = juce::MathConstants<double>::pi / Tp;
    const double epsilon = solveEpsilon (params);
    const double alpha = solveAlpha (params, epsilon);

    // Ee = 1: open phase scaled so it meets the return phase at -1 at Te
    const double E0 = -1.0 / std::sin (omega_g * Te);
    const double expEnd = std::exp (-epsilon * (Tc - Te));

    const double invSize = 1.0 / static_cast<double> (size);

    for (int i = 0; i < size; ++i)
    {
        const double t = static_cast<double> (i) * invSize; // Normalized time [0, 1)

        if (t < Te)
            buffer[i] = static_cast<float> (E0 * std::exp (alpha * (t - Te)) * std::sin (omega_g * t));
        else
            buffer[i] = static_cast<float> (-(std::exp (-epsilon * (t - Te)) - expEnd) / (epsilon * Ta));
    }

    // Sanitize NaN/Inf (defensive; the bracketed solvers cannot diverge)
    for (int i = 0; i < size; ++i)
    {
        if (! std::isfinite (buffer[i]))
            buffer[i] = 0.0f;
    }

    // Normalize to peak = 1.0
    float maxVal = 0.0f;
    for (int i = 0; i < size; ++i)
        maxVal = std::max (maxVal, std::abs (buffer[i]));

    if (maxVal > 0.0f)
    {
        float invMax = 1.0f / maxVal;
        for (int i = 0; i < size; ++i)
            buffer[i] *= invMax;
    }
}

//==============================================================================
void GlottalTableGenerator::generateMipmaps (GlottalWavetable& table)
{
    static constexpr int fftOrder = 11; // log2(2048)
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft (fftOrder);
    std::vector<float> fftBuffer (fftSize * 2, 0.0f);
    std::vector<float> workBuffer (fftSize * 2, 0.0f);

    for (int rdStep = 0; rdStep < GlottalWavetable::kNumRdSteps; ++rdStep)
    {
        // Copy level 0 into FFT buffer
        const float* src = table.getFrameData (0, rdStep);
        std::copy (src, src + fftSize, fftBuffer.begin());
        std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

        // Forward FFT
        fft.performRealOnlyForwardTransform (fftBuffer.data(), false);

        // Generate each mipmap level
        for (int level = 0; level < GlottalWavetable::kNumMipmapLevels; ++level)
        {
            int maxHarmonic = (fftSize / 2) >> level;

            // Copy spectral data
            std::copy (fftBuffer.begin(), fftBuffer.end(), workBuffer.begin());

            // Zero DC bin
            workBuffer[0] = 0.0f;
            workBuffer[1] = 0.0f;

            // Zero bins above maxHarmonic
            for (int bin = maxHarmonic + 1; bin <= fftSize / 2; ++bin)
            {
                workBuffer[bin * 2] = 0.0f;
                workBuffer[bin * 2 + 1] = 0.0f;
            }

            // Zero negative frequency bins that correspond to zeroed positive bins
            for (int bin = 1; bin < fftSize / 2; ++bin)
            {
                if (bin > maxHarmonic)
                {
                    int negBin = fftSize - bin;
                    if (negBin < fftSize)
                    {
                        workBuffer[negBin * 2] = 0.0f;
                        workBuffer[negBin * 2 + 1] = 0.0f;
                    }
                }
            }

            // Inverse FFT
            fft.performRealOnlyInverseTransform (workBuffer.data());

            // Store result
            float* dest = table.getFrameData (level, rdStep);
            std::copy (workBuffer.begin(), workBuffer.begin() + fftSize, dest);
        }
    }

    // Set guard samples for wrap-around interpolation
    table.setGuardSamples();
}

//==============================================================================
void GlottalTableGenerator::generate (GlottalWavetable& table)
{
    table.allocate();

    // 128 log-spaced Rd values from 0.3 to 2.7
    // log(0.3) = -1.2040, log(2.7) = 0.9933
    float logRdMin = std::log (0.3f);
    float logRdMax = std::log (2.7f);

    for (int rdStep = 0; rdStep < GlottalWavetable::kNumRdSteps; ++rdStep)
    {
        // Log-spaced Rd value
        float t = static_cast<float> (rdStep) / static_cast<float> (GlottalWavetable::kNumRdSteps - 1);
        float Rd = std::exp (logRdMin + t * (logRdMax - logRdMin));

        // Compute LF timing parameters from Rd
        LFTimingParams params = computeTimingFromRd (Rd);

        // Render one period into level 0
        float* frameData = table.getFrameData (0, rdStep);
        renderLFPeriod (frameData, GlottalWavetable::kTableSize, params);
    }

    // Generate mipmap levels via FFT spectral truncation
    generateMipmaps (table);
}
