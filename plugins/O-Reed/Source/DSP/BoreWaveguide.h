/*
  ==============================================================================

    BoreWaveguide.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    2-segment bore waveguide with spherical wave scaling
    Keefe tone hole scattering (lumped at bell junction)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

class BoreWaveguide
{
    using DelayLineType = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>;

public:
    BoreWaveguide()
        : segForwardDelay{ DelayLineType(2048), DelayLineType(2048) }
        , segBackwardDelay{ DelayLineType(2048), DelayLineType(2048) }
    {
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = static_cast<float>(sampleRate);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1;

        for (int i = 0; i < 2; ++i)
        {
            segForwardDelay[i].prepare(spec);
            segBackwardDelay[i].prepare(spec);
        }

        bellFilter.reset();
        viscFilter.reset();
        radiationFilter.prepare(spec);
        radiationFilter.reset();

        // Smoothing coefficient for bore morphing (~50ms)
        float smoothTime = 0.05f;
        smoothCoeff = 1.0f - std::exp(-1.0f / (sr * smoothTime));

        reset();
    }

    void setFrequency(float hz)
    {
        if (hz < 20.0f || sr < 1.0f) return;

        targetFrequency = hz;

        float totalDelay = (sr / hz) * cylindricalDelayScale;

        // Compute filter PHASE delays at the target frequency.
        // Phase delay (not group delay) determines pitch in a waveguide.
        // Using group delay caused 5-17% pitch error (too high) because
        // the bell allpass GD >> PD at playing frequencies.
        constexpr float twoPi = 6.2831853f;
        float omega = twoPi * hz / sr;
        float sinW = std::sin(omega);
        float cosW = std::cos(omega);

        // Viscothermal one-pole lowpass phase delay:
        // H(z) = g*(1-p)/(1-p*z^-1)
        // Phase: phi(w) = -atan(p*sin(w)/(1-p*cos(w)))
        // Phase delay: -phi/omega
        float viscPD = 0.0f;
        {
            float p = viscPole;
            if (p > 0.0f && omega > 1e-6f)
            {
                float denom = 1.0f - p * cosW;
                if (std::abs(denom) > 1e-10f)
                    viscPD = std::max(0.0f, std::atan2(p * sinW, denom) / omega);
            }
        }

        // Bell first-order allpass phase delay:
        // H(z) = (a + z^-1) / (1 + a*z^-1)
        // Phase: phi(w) = atan2(-sin(w), a+cos(w)) - atan2(-a*sin(w), 1+a*cos(w))
        // Phase delay: -phi/omega
        float bellPD = 0.0f;
        {
            float a = bellAllpassA;
            if (omega > 1e-6f)
            {
                float phase = std::atan2(-sinW, a + cosW)
                            - std::atan2(-a * sinW, 1.0f + a * cosW);
                // Unwrap: phase should be negative (represents delay)
                if (phase > 0.0f) phase -= twoPi;
                bellPD = std::max(0.0f, -phase / omega);
            }
        }

        // Subtract 1 for the prevBellReflection one-sample storage delay
        float compensatedDelay = totalDelay - viscPD - bellPD - 1.0f;

        // Safety clamp: prevent negative or near-zero delay
        compensatedDelay = std::max(4.0f, compensatedDelay);

        // DIAGNOSTIC: All delay in segment 0 (1-segment bore test)
        float halfDelay = compensatedDelay * 0.5f;
        halfDelay = std::max(2.0f, halfDelay);
        segForwardDelay[0].setDelay(halfDelay);
        segBackwardDelay[0].setDelay(halfDelay);

        // DIAGNOSTIC: print delay values once per note
        static int diagCount = 0;
        if (diagCount++ < 10)
            fprintf(stderr, "[O-Reed DIAG] hz=%.1f sr=%.0f cylScale=%.3f totalDelay=%.1f viscPD=%.2f bellPD=%.2f bellA=%.4f viscP=%.4f compDelay=%.1f halfDelay=%.1f\n",
                    hz, sr, cylindricalDelayScale, totalDelay, viscPD, bellPD, bellAllpassA, viscPole, compensatedDelay, halfDelay);
        // Segment 1 unused
        segForwardDelay[1].setDelay(2.0f);
        segBackwardDelay[1].setDelay(2.0f);
    }

    void updateParams(float boreCharacter, float bellSize, float boreDiameter, float boreLength,
                      float infiniteSustain = 0.0f, float reverseBore = 0.0f, float boreProfile = 0.0f)
    {
        constexpr float pi = 3.14159265f;

        // Cylindrical-to-conical delay correction:
        // Cylindrical bore (closed-open) resonates at quarter-wave: f = c/(4L)
        // Conical bore (open-open via Strategy C) resonates at half-wave: f = c/(2L)
        cylindricalDelayScale = 0.5f + boreCharacter * 0.5f;

        // --- Per-segment conical scale factors (Strategy C) ---
        float halfAngle = boreCharacter * 1.6f * (pi / 180.0f);

        // Throat radius from bore diameter param (0-1 -> 2mm to 20mm)
        float throatRadius = 0.002f + boreDiameter * 0.018f;

        // Bore length: 0-1 -> 0.2m to 1.5m
        float effectiveBoreLength = 0.2f + boreLength * 1.3f;

        // Segment center positions (2 segments)
        constexpr float normalCenters[2]   = { 0.25f, 0.75f };
        constexpr float reversedCenters[2] = { 0.75f, 0.25f };

        // Multi-segment bore profile taper ratios
        constexpr float taperSimple[2] = { 1.0f, 1.0f };
        constexpr float taperMulti[2]  = { 0.5f, 1.5f };

        if (halfAngle < 1e-6f)
        {
            // Cylindrical bore -- all scales 1.0
            for (int i = 0; i < 2; ++i)
            {
                targetScaleForward[i]  = 1.0f;
                targetScaleBackward[i] = 1.0f;
            }
        }
        else
        {
            // Conical bore: per-segment spherical wave scaling
            for (int i = 0; i < 2; ++i)
            {
                float center = normalCenters[i] + reverseBore * (reversedCenters[i] - normalCenters[i]);
                float ratio = taperSimple[i] + boreProfile * (taperMulti[i] - taperSimple[i]);
                float effectiveHalfAngle = std::min(halfAngle * ratio, 5.0f * (pi / 180.0f));

                if (effectiveHalfAngle < 1e-6f)
                {
                    targetScaleForward[i]  = 1.0f;
                    targetScaleBackward[i] = 1.0f;
                    continue;
                }

                float r_in = throatRadius / std::tan(effectiveHalfAngle);
                float L = effectiveBoreLength;
                float r_at_seg = r_in + center * L * std::tan(effectiveHalfAngle);

                targetScaleForward[i]  = r_in / r_at_seg;
                targetScaleBackward[i] = r_at_seg / r_in;
            }
        }

        // --- Bell reflection filter (first-order allpass) ---
        float bellCutoff = 800.0f + bellSize * 5200.0f;

        // Infinite sustain: push bell cutoff toward Nyquist (total reflection)
        float sustainedBellCutoff = bellCutoff + infiniteSustain * (sr * 0.499f - bellCutoff);

        float t = std::tan(pi * sustainedBellCutoff / sr);
        float a = (1.0f - t) / (1.0f + t);
        bellAllpassA = a;
        *bellFilter.coefficients = juce::dsp::IIR::Coefficients<float>(a, 1.0f, 1.0f, a);

        // --- Radiation output filter (first-order highpass at bell cutoff) ---
        float radiationCutoff = juce::jlimit(20.0f, sr * 0.45f, sustainedBellCutoff);
        *radiationFilter.coefficients = juce::dsp::IIR::Coefficients<float>(
            juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(static_cast<double>(sr), radiationCutoff));

        // --- Viscothermal loss filter (one-pole lowpass) ---
        float boreDiameterMm = 4.0f + boreDiameter * 36.0f;
        float viscCutoff = boreDiameterMm * 150.0f;
        currentViscCutoff = viscCutoff;

        float p = std::exp(-2.0f * pi * viscCutoff / sr);
        viscPole = p;

        // Infinite sustain: viscothermal gain approaches 1.0 (lossless)
        float g = 0.995f + infiniteSustain * 0.005f;
        *viscFilter.coefficients = juce::dsp::IIR::Coefficients<float>(g * (1.0f - p), 0.0f, 1.0f, -p);

        feedbackGain = 1.0f;
    }

    void updateToneHoles(float toneHoleCutoff, float registerHole)
    {
        // Map TONE_HOLE_CUTOFF (200-8000 Hz) to progressive hole openings
        // cutoff_norm: 0 = dark (all open), 1 = bright (all closed)
        float cutoff_norm = (toneHoleCutoff - 200.0f) / 7800.0f;

        // Progressive opening: holes open as cutoff drops
        float hole4_opening = 1.0f - std::clamp((cutoff_norm - 0.00f) / 0.25f, 0.0f, 1.0f);
        float hole3_opening = 1.0f - std::clamp((cutoff_norm - 0.25f) / 0.25f, 0.0f, 1.0f);
        float hole2_opening = 1.0f - std::clamp((cutoff_norm - 0.50f) / 0.25f, 0.0f, 1.0f);
        float hole1_opening = 1.0f - std::clamp((cutoff_norm - 0.75f) / 0.25f, 0.0f, 1.0f);

        // Convert openings to scatter coefficients via Keefe model
        constexpr float holeRadiusRatio = 0.6f;
        constexpr float tEffNormalized  = 1.0f;
        constexpr float hrr2 = holeRadiusRatio * holeRadiusRatio;  // 0.36

        auto computeScatter = [](float opening, float rr2, float tEff) -> float
        {
            if (opening < 1e-6f) return 0.0f;
            float holeStrength = opening * rr2 / (2.0f * tEff);
            return -2.0f * holeStrength / (2.0f + holeStrength);
        };

        float s1 = computeScatter(hole1_opening, hrr2, tEffNormalized);
        float s2 = computeScatter(hole2_opening, hrr2, tEffNormalized);
        float s3 = computeScatter(hole3_opening, hrr2, tEffNormalized);
        float s4 = computeScatter(hole4_opening, hrr2, tEffNormalized);

        // Cascade 4 scattering junctions into single lumped scatter:
        // Total transmission = product of individual (1 + s_i)
        // Equivalent single scatter = total_transmission - 1
        float totalTransmission = (1.0f + s1) * (1.0f + s2) * (1.0f + s3) * (1.0f + s4);
        lumpedToneHoleScatter = totalTransmission - 1.0f;

        // Register hole: smaller radius ratio
        constexpr float regRadiusRatio = 0.3f;
        constexpr float regRR2 = regRadiusRatio * regRadiusRatio;  // 0.09
        registerScatter = computeScatter(registerHole, regRR2, tEffNormalized);
    }

    void modulateScaleFactor(float modulation)
    {
        scaleModulation = modulation;
    }

    // Process one sample through the bore waveguide
    // Returns p_bore_minus (wave arriving at reed from bore)
    float processSample(float p_reed_out)
    {
        // --- Smooth conical scale factors ---
        for (int i = 0; i < 2; ++i)
        {
            currentScaleForward[i]  += (targetScaleForward[i]  - currentScaleForward[i])  * smoothCoeff;
            currentScaleBackward[i] += (targetScaleBackward[i] - currentScaleBackward[i]) * smoothCoeff;
        }
        float mod = 1.0f + scaleModulation;
        scaleModulation = 0.0f;

        // --- DIAGNOSTIC: Single-segment bore (proven working in tests) ---
        // Using only segment 0 as a single forward+backward delay pair.
        // Segment 1 is unused (kept prepared but not in signal path).
        float fwd = segForwardDelay[0].popSample(0) * currentScaleForward[0] * mod;
        float bwd = segBackwardDelay[0].popSample(0) * currentScaleBackward[0] * mod;

        // Pop segment 1 to keep delay line state advancing (prevent stale data)
        segForwardDelay[1].popSample(0);
        segBackwardDelay[1].popSample(0);

        // Bell processing (direct, no junctions)
        float bellFiltered = bellFilter.processSample(fwd);
        float p_reflected = -bellFiltered;
        lastRadiatedOutput = radiationFilter.processSample(fwd);
        float p_backward_lossy = viscFilter.processSample(p_reflected);
        prevBellReflection = p_backward_lossy;

        // Push into single segment
        segForwardDelay[0].pushSample(0, p_reed_out);
        segBackwardDelay[0].pushSample(0, p_backward_lossy);

        // Push zeros into unused segment 1
        segForwardDelay[1].pushSample(0, 0.0f);
        segBackwardDelay[1].pushSample(0, 0.0f);

        float returnWave = bwd * feedbackGain;
        energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs(returnWave);
        return returnWave;
    }

    float getRadiatedOutput() const { return lastRadiatedOutput; }

    float getEnergy() const { return energyEstimate; }

    void snapFiltersToZero()
    {
        bellFilter.snapToZero();
        viscFilter.snapToZero();
        radiationFilter.snapToZero();
    }

    void reset()
    {
        for (int i = 0; i < 2; ++i)
        {
            segForwardDelay[i].reset();
            segBackwardDelay[i].reset();
            currentScaleForward[i]  = 1.0f;
            currentScaleBackward[i] = 1.0f;
            targetScaleForward[i]   = 1.0f;
            targetScaleBackward[i]  = 1.0f;
        }

        bellFilter.reset();
        viscFilter.reset();
        radiationFilter.reset();

        lumpedToneHoleScatter = 0.0f;
        registerScatter = 0.0f;
        prevBellReflection = 0.0f;
        scaleModulation = 0.0f;
        lastRadiatedOutput = 0.0f;
        energyEstimate = 0.0f;
        targetFrequency = 440.0f;
        currentViscCutoff = 1500.0f;
        feedbackGain = 1.0f;
        viscPole = 0.0f;
        bellAllpassA = 0.0f;
        cylindricalDelayScale = 0.5f;
    }

private:
    DelayLineType segForwardDelay[2];
    DelayLineType segBackwardDelay[2];

    juce::dsp::IIR::Filter<float> bellFilter;
    juce::dsp::IIR::Filter<float> viscFilter;
    juce::dsp::IIR::Filter<float> radiationFilter;

    float sr = 44100.0f;

    // Per-segment conical scale factors
    float currentScaleForward[2]  = { 1.0f, 1.0f };
    float currentScaleBackward[2] = { 1.0f, 1.0f };
    float targetScaleForward[2]   = { 1.0f, 1.0f };
    float targetScaleBackward[2]  = { 1.0f, 1.0f };
    float smoothCoeff             = 0.001f;

    // Lumped tone hole scatter (cascaded from 4 individual holes)
    float lumpedToneHoleScatter = 0.0f;
    float registerScatter = 0.0f;

    // Bell reflection one-sample memory
    float prevBellReflection = 0.0f;

    // Throat vibrato modulation
    float scaleModulation = 0.0f;

    // State
    float lastRadiatedOutput = 0.0f;
    float energyEstimate     = 0.0f;
    float targetFrequency    = 440.0f;
    float currentViscCutoff  = 1500.0f;
    float feedbackGain       = 1.0f;

    // Stored filter coefficients for phase delay in setFrequency()
    float viscPole       = 0.0f;   // Viscothermal one-pole coefficient
    float bellAllpassA   = 0.0f;   // Bell allpass coefficient

    // Cylindrical-to-conical delay correction (0.5 = cylindrical, 1.0 = full cone)
    float cylindricalDelayScale = 0.5f;
};
