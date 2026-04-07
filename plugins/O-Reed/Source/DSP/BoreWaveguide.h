/*
  ==============================================================================

    BoreWaveguide.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Strategy C conical bore waveguide with spherical wave scaling
    5-segment bore with Keefe three-port tone hole scattering
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
        : segForwardDelay{ DelayLineType(2048), DelayLineType(2048), DelayLineType(2048), DelayLineType(2048), DelayLineType(2048) }
        , segBackwardDelay{ DelayLineType(2048), DelayLineType(2048), DelayLineType(2048), DelayLineType(2048), DelayLineType(2048) }
    {
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = static_cast<float>(sampleRate);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1;

        for (int i = 0; i < 5; ++i)
        {
            segForwardDelay[i].prepare(spec);
            segBackwardDelay[i].prepare(spec);
        }

        // Filters already have passthrough coefficients from default constructor
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

        float totalDelay = sr / hz;

        // Subtract filter group delays for pitch accuracy
        float viscGD = (currentViscCutoff > 0.0f)
                     ? sr / (6.2831853f * currentViscCutoff)
                     : 0.0f;
        float bellGD = 0.5f;

        float compensatedDelay = totalDelay - viscGD - bellGD;

        // Split into 5 segments with prescribed fractions
        constexpr float fractions[5] = { 0.10f, 0.20f, 0.20f, 0.25f, 0.25f };

        for (int i = 0; i < 5; ++i)
        {
            float halfDelay = compensatedDelay * fractions[i] * 0.5f;
            halfDelay = std::max(halfDelay, 2.0f);
            segForwardDelay[i].setDelay(halfDelay);
            segBackwardDelay[i].setDelay(halfDelay);
        }
    }

    void updateParams(float boreCharacter, float bellSize, float boreDiameter, float boreLength,
                      float infiniteSustain = 0.0f, float reverseBore = 0.0f, float boreProfile = 0.0f)
    {
        constexpr float pi = 3.14159265f;

        // --- Per-segment conical scale factors (Strategy C) ---
        float halfAngle = boreCharacter * 1.6f * (pi / 180.0f);

        // Throat radius from bore diameter param (0-1 -> 2mm to 20mm)
        float throatRadius = 0.002f + boreDiameter * 0.018f;

        // Bore length: 0-1 -> 0.2m to 1.5m
        float effectiveBoreLength = 0.2f + boreLength * 1.3f;

        // Segment center positions with reverse bore interpolation
        constexpr float normalCenters[5]   = { 0.05f, 0.20f, 0.40f, 0.625f, 0.875f };
        constexpr float reversedCenters[5] = { 0.95f, 0.80f, 0.60f, 0.375f, 0.125f };

        // Multi-segment bore profile taper ratios
        constexpr float taperSimple[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        constexpr float taperMulti[5]  = { 0.3f, 0.5f, 1.0f, 1.2f, 2.0f };

        if (halfAngle < 1e-6f)
        {
            // Cylindrical bore -- all scales 1.0 regardless of reverse/profile
            for (int i = 0; i < 5; ++i)
            {
                targetScaleForward[i]  = 1.0f;
                targetScaleBackward[i] = 1.0f;
            }
        }
        else
        {
            // Conical bore: per-segment spherical wave scaling
            for (int i = 0; i < 5; ++i)
            {
                // Interpolate center position for reverse bore
                float center = normalCenters[i] + reverseBore * (reversedCenters[i] - normalCenters[i]);

                // Interpolate taper ratio for bore profile
                float ratio = taperSimple[i] + boreProfile * (taperMulti[i] - taperSimple[i]);

                // Effective half angle per segment, clamped to max 5 degrees
                float effectiveHalfAngle = std::min(halfAngle * ratio, 5.0f * (pi / 180.0f));

                // Guard: if taper ratio drove halfAngle near zero, treat as cylindrical
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

        // Infinite sustain: viscothermal gain approaches 1.0 (lossless)
        float g = 0.995f + infiniteSustain * 0.005f;
        *viscFilter.coefficients = juce::dsp::IIR::Coefficients<float>(g * (1.0f - p), 0.0f, 1.0f, -p);
    }

    void updateToneHoles(float toneHoleCutoff, float registerHole)
    {
        // Map TONE_HOLE_CUTOFF (200-8000 Hz) to progressive hole openings
        // cutoff_norm: 0 = dark (all open), 1 = bright (all closed)
        float cutoff_norm = (toneHoleCutoff - 200.0f) / 7800.0f;

        // Progressive opening: hole nearest bell opens first as cutoff drops
        float hole4_opening = 1.0f - std::clamp((cutoff_norm - 0.00f) / 0.25f, 0.0f, 1.0f);
        float hole3_opening = 1.0f - std::clamp((cutoff_norm - 0.25f) / 0.25f, 0.0f, 1.0f);
        float hole2_opening = 1.0f - std::clamp((cutoff_norm - 0.50f) / 0.25f, 0.0f, 1.0f);
        float hole1_opening = 1.0f - std::clamp((cutoff_norm - 0.75f) / 0.25f, 0.0f, 1.0f);

        // Convert each opening to scatter coefficient via Keefe model
        constexpr float holeRadiusRatio = 0.6f;
        constexpr float tEffNormalized  = 1.0f;
        constexpr float hrr2 = holeRadiusRatio * holeRadiusRatio;  // 0.36

        auto computeScatter = [](float opening, float rr2, float tEff) -> float
        {
            if (opening < 1e-6f) return 0.0f;
            float holeStrength = opening * rr2 / (2.0f * tEff);
            return -2.0f * holeStrength / (2.0f + holeStrength);
        };

        toneHoleScatter[0] = computeScatter(hole1_opening, hrr2, tEffNormalized);
        toneHoleScatter[1] = computeScatter(hole2_opening, hrr2, tEffNormalized);
        toneHoleScatter[2] = computeScatter(hole3_opening, hrr2, tEffNormalized);
        toneHoleScatter[3] = computeScatter(hole4_opening, hrr2, tEffNormalized);

        // Register hole: smaller radius ratio
        constexpr float regRadiusRatio = 0.3f;
        constexpr float regRR2 = regRadiusRatio * regRadiusRatio;  // 0.09
        registerScatter = computeScatter(registerHole, regRR2, tEffNormalized);
    }

    void modulateScaleFactor(float modulation)
    {
        scaleModulation = modulation;
    }

    // Process one sample through the 5-segment bore waveguide
    // Returns p_bore_minus (wave arriving at reed from bore)
    float processSample(float p_reed_out)
    {
        // --- Smooth conical scale factors ---
        for (int i = 0; i < 5; ++i)
        {
            currentScaleForward[i]  += (targetScaleForward[i]  - currentScaleForward[i])  * smoothCoeff;
            currentScaleBackward[i] += (targetScaleBackward[i] - currentScaleBackward[i]) * smoothCoeff;
        }

        // Apply throat vibrato modulation to scale factors
        float mod = 1.0f + scaleModulation;
        scaleModulation = 0.0f;  // Reset after use

        // --- Step 1: Pop all delays ---
        float seg_fwd[5], seg_bwd[5];
        for (int i = 0; i < 5; ++i)
        {
            seg_fwd[i] = segForwardDelay[i].popSample(0) * currentScaleForward[i] * mod;
            seg_bwd[i] = segBackwardDelay[i].popSample(0) * currentScaleBackward[i] * mod;
        }

        // --- Step 2: Compute junctions ---

        // Register hole (between seg 0 and seg 1)
        float reg_sum = seg_fwd[0] + seg_bwd[1];
        float reg_p_scattered = registerScatter * reg_sum;
        float reg_fwd = seg_fwd[0] + reg_p_scattered;
        float reg_bwd = seg_bwd[1] + reg_p_scattered;
        float reg_radiated = -reg_p_scattered;

        // Tone hole 1 (between seg 1 and seg 2)
        float th1_sum = seg_fwd[1] + seg_bwd[2];
        float th1_scat = toneHoleScatter[0] * th1_sum;
        float th1_fwd = seg_fwd[1] + th1_scat;
        float th1_bwd = seg_bwd[2] + th1_scat;
        float th1_rad = -th1_scat;

        // Tone hole 2 (between seg 2 and seg 3)
        float th2_sum = seg_fwd[2] + seg_bwd[3];
        float th2_scat = toneHoleScatter[1] * th2_sum;
        float th2_fwd = seg_fwd[2] + th2_scat;
        float th2_bwd = seg_bwd[3] + th2_scat;
        float th2_rad = -th2_scat;

        // Tone hole 3 (between seg 3 and seg 4)
        float th3_sum = seg_fwd[3] + seg_bwd[4];
        float th3_scat = toneHoleScatter[2] * th3_sum;
        float th3_fwd = seg_fwd[3] + th3_scat;
        float th3_bwd = seg_bwd[4] + th3_scat;
        float th3_rad = -th3_scat;

        // Tone hole 4 (between seg 4 and bell)
        float th4_sum = seg_fwd[4] + prevBellReflection;
        float th4_scat = toneHoleScatter[3] * th4_sum;
        float th4_fwd = seg_fwd[4] + th4_scat;
        float th4_bwd = prevBellReflection + th4_scat;
        float th4_rad = -th4_scat;

        // --- Step 3: Bell processing ---
        float bellFiltered = bellFilter.processSample(th4_fwd);
        float p_reflected = -bellFiltered;
        lastRadiatedOutput = radiationFilter.processSample(th4_fwd);

        float p_backward_lossy = viscFilter.processSample(p_reflected);
        prevBellReflection = p_backward_lossy;

        // --- Step 4: Accumulate tone hole radiation ---
        totalToneHoleRadiation = reg_radiated + th1_rad + th2_rad + th3_rad + th4_rad;

        // --- Step 5: Push into delays ---
        segForwardDelay[0].pushSample(0, p_reed_out);
        segBackwardDelay[0].pushSample(0, reg_bwd);

        segForwardDelay[1].pushSample(0, reg_fwd);
        segBackwardDelay[1].pushSample(0, th1_bwd);

        segForwardDelay[2].pushSample(0, th1_fwd);
        segBackwardDelay[2].pushSample(0, th2_bwd);

        segForwardDelay[3].pushSample(0, th2_fwd);
        segBackwardDelay[3].pushSample(0, th3_bwd);

        segForwardDelay[4].pushSample(0, th3_fwd);
        segBackwardDelay[4].pushSample(0, th4_bwd);

        // --- Step 6: Energy tracking ---
        energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs(lastRadiatedOutput);

        return seg_bwd[0];  // Wave arriving at reed from bore
    }

    float getRadiatedOutput() const { return lastRadiatedOutput + totalToneHoleRadiation * toneHoleRadiationMix; }

    float getEnergy() const { return energyEstimate; }

    void snapFiltersToZero()
    {
        bellFilter.snapToZero();
        viscFilter.snapToZero();
        radiationFilter.snapToZero();
    }

    void reset()
    {
        for (int i = 0; i < 5; ++i)
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

        for (int i = 0; i < 4; ++i)
            toneHoleScatter[i] = 0.0f;

        registerScatter = 0.0f;
        prevBellReflection = 0.0f;
        totalToneHoleRadiation = 0.0f;
        scaleModulation = 0.0f;
        lastRadiatedOutput = 0.0f;
        energyEstimate = 0.0f;
        targetFrequency = 440.0f;
        currentViscCutoff = 1500.0f;
    }

private:
    DelayLineType segForwardDelay[5];
    DelayLineType segBackwardDelay[5];

    juce::dsp::IIR::Filter<float> bellFilter;
    juce::dsp::IIR::Filter<float> viscFilter;
    juce::dsp::IIR::Filter<float> radiationFilter;

    float sr = 44100.0f;

    // Per-segment conical scale factors
    float currentScaleForward[5]  = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float currentScaleBackward[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float targetScaleForward[5]   = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float targetScaleBackward[5]  = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float smoothCoeff             = 0.001f;

    // Tone hole scattering coefficients
    float toneHoleScatter[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float registerScatter = 0.0f;

    // Tone hole radiation
    float totalToneHoleRadiation = 0.0f;
    static constexpr float toneHoleRadiationMix = 0.4f;

    // Bell reflection one-sample memory
    float prevBellReflection = 0.0f;

    // Throat vibrato modulation
    float scaleModulation = 0.0f;

    // State
    float lastRadiatedOutput = 0.0f;
    float energyEstimate     = 0.0f;
    float targetFrequency    = 440.0f;
    float currentViscCutoff  = 1500.0f;
};
