/*
  ==============================================================================

    GlottalTableGenerator.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Offline LF glottal pulse table generation with Fant 1995 regression,
    Newton-Raphson solvers for alpha/epsilon, and FFT mipmap generation.

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
    float Ra = std::max (0.001f, (-1.0f + 4.8f * Rd) / 100.0f);
    float Rk = (22.4f + 11.8f * Rd) / 100.0f;

    // OQ (Open Quotient) -- piecewise regression, clamped [0.3, 0.98]
    float OQ;
    if (Rd < 0.5f)
        OQ = 0.3f + 0.2f * (Rd - 0.3f) / 0.2f;
    else if (Rd < 1.2f)
        OQ = 0.5f + 0.3f * (Rd - 0.5f) / 0.7f;
    else
        OQ = 0.8f + 0.18f * (Rd - 1.2f) / 1.5f;
    OQ = juce::jlimit (0.3f, 0.98f, OQ);

    float Rg = (1.0f + Rk) / (2.0f * OQ);

    // Timing (normalized to period = 1.0)
    LFTimingParams params;
    params.Tp = 1.0f / (2.0f * Rg);
    params.Te = params.Tp * (1.0f + Rk);
    params.Ta = Ra;
    params.Tc = 1.0f;

    // Safety clamp Te to be within period
    params.Te = std::min (params.Te, 0.99f);
    params.Ta = std::max (params.Ta, 0.001f);

    return params;
}

//==============================================================================
float GlottalTableGenerator::solveAlpha (float Tp, float Te)
{
    // Solve for alpha such that integral of open-phase LF derivative is zero
    // E0 * exp(alpha*t) * sin(omega_g*t) from 0 to Te
    // omega_g = pi / Tp

    float omega_g = juce::MathConstants<float>::pi / Tp;
    float alpha = 1.0f / Tp; // Initial guess

    for (int iter = 0; iter < 50; ++iter)
    {
        // Compute integral: E0 * [exp(alpha*Te)*(alpha*sin(omega_g*Te) - omega_g*cos(omega_g*Te)) + omega_g]
        //                   / (alpha^2 + omega_g^2)
        float expATe = std::exp (alpha * Te);
        float sinWTe = std::sin (omega_g * Te);
        float cosWTe = std::cos (omega_g * Te);
        float denom = alpha * alpha + omega_g * omega_g;

        if (std::abs (denom) < 1e-12f)
            break;

        // f(alpha) = integral = 0
        // Numerator of integral (without E0 which cancels):
        float f = (expATe * (alpha * sinWTe - omega_g * cosWTe) + omega_g) / denom;

        // Derivative df/dalpha (numerical approximation for stability)
        float da = 0.01f;
        float alpha2 = alpha + da;
        float expATe2 = std::exp (alpha2 * Te);
        float denom2 = alpha2 * alpha2 + omega_g * omega_g;
        float f2 = (expATe2 * (alpha2 * sinWTe - omega_g * cosWTe) + omega_g) / denom2;

        float df = (f2 - f) / da;

        if (std::abs (df) < 1e-12f)
            break;

        alpha -= f / df;

        if (std::abs (f) < 1e-6f)
            break;
    }

    return alpha;
}

//==============================================================================
float GlottalTableGenerator::solveEpsilon (float Ta, float Te, float Tc)
{
    // Solve: 1 - exp(-epsilon*(Tc-Te)) = epsilon * Ta
    float returnDuration = Tc - Te;
    float epsilon = 1.0f / std::max (Ta, 0.001f); // Initial guess

    for (int iter = 0; iter < 30; ++iter)
    {
        float expTerm = std::exp (-epsilon * returnDuration);

        // f(epsilon) = 1 - exp(-epsilon*(Tc-Te)) - epsilon*Ta = 0
        float f = 1.0f - expTerm - epsilon * Ta;

        // f'(epsilon) = (Tc-Te)*exp(-epsilon*(Tc-Te)) - Ta
        float df = returnDuration * expTerm - Ta;

        if (std::abs (df) < 1e-12f)
            break;

        epsilon -= f / df;
        epsilon = std::max (epsilon, 0.1f); // Keep positive

        if (std::abs (f) < 1e-6f)
            break;
    }

    return epsilon;
}

//==============================================================================
void GlottalTableGenerator::renderLFPeriod (float* buffer, int size, const LFTimingParams& params)
{
    float Tp = params.Tp;
    float Te = params.Te;
    float Ta = params.Ta;
    float Tc = params.Tc;

    float omega_g = juce::MathConstants<float>::pi / Tp;
    float alpha = solveAlpha (Tp, Te);
    float epsilon = solveEpsilon (Ta, Te, Tc);

    // Compute Ee (peak negative excitation at Te)
    float Ee = -std::exp (alpha * Te) * std::sin (omega_g * Te);

    // Handle degenerate case
    if (std::abs (Ee) < 1e-10f)
        Ee = -1.0f;

    float invSize = 1.0f / static_cast<float> (size);

    for (int i = 0; i < size; ++i)
    {
        float t = static_cast<float> (i) * invSize; // Normalized time [0, 1)

        if (t < Te)
        {
            // Open phase: E0 * exp(alpha*t) * sin(omega_g*t)
            // E0 is implicit (will be normalized)
            buffer[i] = std::exp (alpha * t) * std::sin (omega_g * t);
        }
        else
        {
            // Return phase: (-Ee / (epsilon*Ta)) * [exp(-epsilon*(t-Te)) - exp(-epsilon*(Tc-Te))]
            float tRel = t - Te;
            float expDecay = std::exp (-epsilon * tRel);
            float expEnd = std::exp (-epsilon * (Tc - Te));
            buffer[i] = (-Ee / (epsilon * Ta)) * (expDecay - expEnd);
        }
    }

    // Sanitize NaN/Inf from Newton-Raphson edge cases
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
