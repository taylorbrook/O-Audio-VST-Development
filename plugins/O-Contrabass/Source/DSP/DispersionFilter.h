/*
  ==============================================================================

    DispersionFilter.h
    O-Contrabass — Cascaded First-Order Allpass Dispersion (Rauhala/Välimäki 2006)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1c. Lives on the bridge rail of the split-rail waveguide,
    between popSample and the bridge LP one-pole. Identity at a=0.

    Closed-form coefficient computation per Rauhala & Välimäki (2006),
    "Tunable dispersion filter design for piano synthesis", IEEE Sig.
    Proc. Letters Vol. 13 No. 5, Table 1. Constants and validity envelope
    documented in RESEARCH §14.2.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

template <int MaxSections = 4>
class DispersionFilter
{
public:
    static_assert (MaxSections >= 1, "DispersionFilter requires at least one section");

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : sections) s.z = 0.0f;
    }

    // Set the cascade depth at runtime (Phase 2.2 per-string M-table hook).
    // Phase 2.1c: voice constructs DispersionFilter<4> and calls setActiveSections(4).
    void setActiveSections (int M) noexcept
    {
        activeSections = juce::jlimit (0, MaxSections, M);
    }

    // Per-block setter — voice computes `a` from (f0, B, M) and pushes here.
    // Defensive clamp to [-0.99, 0.99] mirrors the closed-form's clamp; safe
    // even though computeAllpassCoefficient already returns a clamped value.
    void setCoefficient (float a) noexcept
    {
        const float clamped = juce::jlimit (-0.99f, 0.99f, a);
        for (int i = 0; i < MaxSections; ++i)
            sections[i].a = clamped;
    }

    // Per-sample processing.
    inline float processSample (float x) noexcept
    {
        for (int i = 0; i < activeSections; ++i)
        {
            // Transposed direct form II — single state element per section.
            //   y[n] = a * x[n] + z[n-1]
            //   z[n] = x[n] - a * y[n]
            const float a = sections[i].a;
            const float y = a * x + sections[i].z;
            sections[i].z = x - a * y;
            x = y;
        }
        return x;
    }

    // Closed-form coefficient — voice calls this once per block.
    //
    // Citation: Rauhala & Välimäki (2006), IEEE Sig. Proc. Letters Vol. 13 No. 5,
    //           Table 1. See RESEARCH §14.2 for derivation, validity envelope,
    //           and the I=8.0 (E1) clamp-saturation anomaly.
    static float computeAllpassCoefficient (float f0Hz, float B, int M) noexcept
    {
        constexpr float k1 = -0.0135f, k2 = 0.0058f, k3 = -0.000004f;
        constexpr float m1 =  0.0034f, m2 = 0.0179f, m3 = -0.0009f, m4 = -0.4986f;

        const float I  = std::log2 (juce::jmax (f0Hz, 1.0f) / 440.0f) * 12.0f + 49.0f;
        const float lB = std::log  (juce::jmax (B,    1e-9f));
        const float lM = std::log  (static_cast<float> (juce::jmax (M, 1)));

        const float C  = m1 * lB + m2 * lM + m3 * lB * lM + m4;
        const float k  = k1 + k2 * I + k3 * I * I;

        return juce::jlimit (-0.99f, 0.99f, -C / k);
    }

    // Total group delay of the active cascade at frequency f0Hz, in samples.
    // Closed form (option (b) at-f0 per RESEARCH §14.3):
    //   D = M · (1 - a²) / |1 + a·e^{-j·2π·f0/sr}|²
    //
    // Used by WaveguideString::updateDelayLengths() to compensate base round-trip.
    // Identity at a=0: numerator=1, denominator=1, per-section=1, total=M (exact).
    float getGroupDelaySamples (float f0Hz) const noexcept
    {
        if (activeSections == 0)
            return 0.0f;

        const float a    = sections[0].a;          // all sections share the same coefficient
        const float w    = juce::MathConstants<float>::twoPi
                         * f0Hz / static_cast<float> (sampleRate);
        const float cosW = std::cos (w);
        const float oneMinusASq = 1.0f - a * a;
        const float denom       = 1.0f + 2.0f * a * cosW + a * a;
        const float perSection  = oneMinusASq / juce::jmax (denom, 1e-9f);

        return static_cast<float> (activeSections) * perSection;
    }

    int getActiveSections() const noexcept { return activeSections; }

private:
    struct AllpassSection
    {
        float a = 0.0f;     // coefficient, |a| < 0.99 (clamped at setCoefficient)
        float z = 0.0f;     // single state element
    };

    AllpassSection sections[MaxSections];
    int            activeSections = 0;
    double         sampleRate     = 88200.0;
};
