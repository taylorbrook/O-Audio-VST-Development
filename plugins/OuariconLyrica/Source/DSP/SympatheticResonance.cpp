/*
  ==============================================================================

    SympatheticResonance.cpp
    Sympathetic Resonance Engine - Phase 2.7 (Optimized Phase 2.12)
    Ouaricon Audio
    Developer: Taylor Brook

    OPTIMIZATION NOTES:
    - Changed from std::map to std::unordered_map for O(1) voice lookup
    - Precomputes harmonic coupling matrix on voice registration (not per-sample)
    - Replaces log2-based cents calculation with fast ratio comparison
    - Reduces per-sample complexity from O(n²) to O(n) where n = coupled voices only

  ==============================================================================
*/

#include "SympatheticResonance.h"
#include <algorithm>

SympatheticResonanceEngine::SympatheticResonanceEngine()
    : intensity(0.3f)
    , sampleRate(44100.0)
{
    activeVoiceIds.reserve(MAX_VOICES);
}

SympatheticResonanceEngine::~SympatheticResonanceEngine()
{
}

void SympatheticResonanceEngine::prepare(double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;

    // Reset all existing resonators with new sample rate
    for (auto& pair : activeVoices)
    {
        auto& voiceInfo = pair.second;
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 1;
        spec.numChannels = 1;

        voiceInfo.resonatorFilter.prepare(spec);
        designResonatorFilter(voiceInfo.resonatorFilter, voiceInfo.frequency, 5.0f);
    }

    // Rebuild coupling matrix with new sample rate
    rebuildCouplingMatrix();
}

void SympatheticResonanceEngine::registerVoice(int voiceId, double frequency, const StringMaterial& material)
{
    // Insert or get existing entry, then configure in place
    auto& voiceInfo = activeVoices[voiceId];
    voiceInfo.frequency = frequency;
    voiceInfo.materialCoupling = material.sympatheticCoupling;
    voiceInfo.lastSample = 0.0f;

    // Energy decay based on material damping (less damped = longer resonance)
    voiceInfo.energyDecay = 0.995f + (1.0f - material.dampingCoeff) * 0.0048f;

    // Prepare resonator filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 1;
    spec.numChannels = 1;

    voiceInfo.resonatorFilter.prepare(spec);
    designResonatorFilter(voiceInfo.resonatorFilter, frequency, 5.0f);

    // Add to active voice list if not already present
    if (std::find(activeVoiceIds.begin(), activeVoiceIds.end(), voiceId) == activeVoiceIds.end())
    {
        activeVoiceIds.push_back(voiceId);
    }

    // OPTIMIZATION: Rebuild coupling matrix with new voice
    rebuildCouplingMatrix();
}

void SympatheticResonanceEngine::unregisterVoice(int voiceId)
{
    activeVoices.erase(voiceId);
    couplingMatrix.erase(voiceId);

    // Remove from active voice list
    auto it = std::find(activeVoiceIds.begin(), activeVoiceIds.end(), voiceId);
    if (it != activeVoiceIds.end())
    {
        activeVoiceIds.erase(it);
    }

    // OPTIMIZATION: Rebuild coupling matrix without this voice
    rebuildCouplingMatrix();
}

void SympatheticResonanceEngine::setIntensity(float newIntensity)
{
    float oldIntensity = intensity;
    intensity = juce::jlimit(0.0f, 1.0f, newIntensity);

    // If intensity changed significantly, rebuild coupling matrix
    if (std::abs(intensity - oldIntensity) > 0.01f)
    {
        rebuildCouplingMatrix();
    }
}

void SympatheticResonanceEngine::rebuildCouplingMatrix()
{
    couplingMatrix.clear();

    // For each active voice, precompute its coupling to all other voices
    for (int voiceId : activeVoiceIds)
    {
        auto voiceIt = activeVoices.find(voiceId);
        if (voiceIt == activeVoices.end())
            continue;

        const auto& currentVoice = voiceIt->second;
        std::vector<CouplingPair>& couplings = couplingMatrix[voiceId];
        couplings.clear();
        couplings.reserve(activeVoiceIds.size());

        for (int otherVoiceId : activeVoiceIds)
        {
            if (otherVoiceId == voiceId)
                continue;

            auto otherIt = activeVoices.find(otherVoiceId);
            if (otherIt == activeVoices.end())
                continue;

            const auto& otherVoice = otherIt->second;

            // Compute harmonic coupling strength (done ONCE, not per-sample)
            float harmonicCoupling = computeCouplingStrength(currentVoice.frequency, otherVoice.frequency);

            if (harmonicCoupling > 0.0f)
            {
                // Precompute total coupling factor
                float materialFactor = currentVoice.materialCoupling * otherVoice.materialCoupling;
                float totalCoupling = harmonicCoupling * materialFactor * intensity * 0.05f;

                CouplingPair pair;
                pair.otherVoiceId = otherVoiceId;
                pair.totalCoupling = totalCoupling;
                couplings.push_back(pair);
            }
        }
    }
}

float SympatheticResonanceEngine::computeSympatheticContribution(int voiceId, float voiceOutput)
{
    // Fast path: if intensity is zero, skip all processing
    if (intensity <= 0.0f)
        return 0.0f;

    // O(1) lookup in unordered_map
    auto couplingIt = couplingMatrix.find(voiceId);
    if (couplingIt == couplingMatrix.end())
        return 0.0f;

    const auto& couplings = couplingIt->second;
    if (couplings.empty())
        return 0.0f;

    float sympatheticSignal = 0.0f;

    // OPTIMIZED: Only iterate through precomputed coupled voices (not all voices)
    for (const auto& coupling : couplings)
    {
        auto otherIt = activeVoices.find(coupling.otherVoiceId);
        if (otherIt == activeVoices.end())
            continue;

        auto& otherVoice = otherIt->second;

        // Apply energy decay
        otherVoice.lastSample *= otherVoice.energyDecay;

        // Excite resonator with precomputed coupling (no calculations needed)
        otherVoice.lastSample += voiceOutput * coupling.totalCoupling;

        // Process through resonator filter
        float resonatorOutput = otherVoice.resonatorFilter.processSample(otherVoice.lastSample);

        sympatheticSignal += resonatorOutput;
    }

    // Soft clipping to prevent buildup
    if (sympatheticSignal > 0.1f)
        sympatheticSignal = 0.1f + std::tanh((sympatheticSignal - 0.1f) * 2.0f) * 0.05f;
    else if (sympatheticSignal < -0.1f)
        sympatheticSignal = -0.1f + std::tanh((sympatheticSignal + 0.1f) * 2.0f) * 0.05f;

    return sympatheticSignal;
}

void SympatheticResonanceEngine::reset()
{
    activeVoices.clear();
    couplingMatrix.clear();
    activeVoiceIds.clear();
}

float SympatheticResonanceEngine::computeCouplingStrength(double freq1, double freq2) const
{
    // OPTIMIZED: Use ratio-based tolerances instead of log2 cents calculation
    // Precomputed tolerance ratios (avoiding expensive log2 in hot path):
    // 10 cents ≈ 2^(10/1200) ≈ 1.00578
    // 15 cents ≈ 2^(15/1200) ≈ 1.00868
    // 20 cents ≈ 2^(20/1200) ≈ 1.01159
    // 25 cents ≈ 2^(25/1200) ≈ 1.01451

    // Unison (1/1) - strongest coupling
    float unisonCoupling = checkHarmonicIntervalFast(freq1, freq2, 1.0, 1.00578);
    if (unisonCoupling > 0.0f)
        return unisonCoupling * 0.9f;

    // Octave (1/2, 2/1) - strong coupling
    float octaveDownCoupling = checkHarmonicIntervalFast(freq1, freq2, 0.5, 1.00868);
    float octaveUpCoupling = checkHarmonicIntervalFast(freq1, freq2, 2.0, 1.00868);
    float octaveCoupling = std::max(octaveDownCoupling, octaveUpCoupling);
    if (octaveCoupling > 0.0f)
        return octaveCoupling * 0.7f;

    // Perfect Fifth (2/3, 3/2) - medium coupling
    float fifthDownCoupling = checkHarmonicIntervalFast(freq1, freq2, 2.0/3.0, 1.01159);
    float fifthUpCoupling = checkHarmonicIntervalFast(freq1, freq2, 3.0/2.0, 1.01159);
    float fifthCoupling = std::max(fifthDownCoupling, fifthUpCoupling);
    if (fifthCoupling > 0.0f)
        return fifthCoupling * 0.5f;

    // Major Third (4/5, 5/4) - weak coupling
    float thirdDownCoupling = checkHarmonicIntervalFast(freq1, freq2, 4.0/5.0, 1.01451);
    float thirdUpCoupling = checkHarmonicIntervalFast(freq1, freq2, 5.0/4.0, 1.01451);
    float thirdCoupling = std::max(thirdDownCoupling, thirdUpCoupling);
    if (thirdCoupling > 0.0f)
        return thirdCoupling * 0.3f;

    return 0.0f;
}

float SympatheticResonanceEngine::checkHarmonicIntervalFast(double freq1, double freq2, double ratio, double toleranceRatio) const
{
    // OPTIMIZED: Direct ratio comparison instead of log2 cents calculation
    // This avoids the expensive std::log2 call in the original implementation

    double actualRatio = freq2 / freq1;
    double targetRatio = ratio;

    // Compute deviation as a ratio (not cents)
    double deviationRatio = actualRatio / targetRatio;

    // Check if within tolerance (tolerance is already in ratio form)
    double invTolerance = 1.0 / toleranceRatio;

    if (deviationRatio >= invTolerance && deviationRatio <= toleranceRatio)
    {
        // Linear falloff based on how close to exact ratio
        // 1.0 at exact match, 0.0 at tolerance edge
        double maxDeviation = toleranceRatio - 1.0;
        double actualDeviation = std::abs(deviationRatio - 1.0);
        return 1.0f - static_cast<float>(actualDeviation / maxDeviation);
    }

    return 0.0f;
}

void SympatheticResonanceEngine::designResonatorFilter(juce::dsp::IIR::Filter<float>& filter, double frequency, float Q)
{
    double clampedFreq = juce::jlimit(20.0, sampleRate * 0.45, frequency);

    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRate,
        clampedFreq,
        Q
    );

    *filter.coefficients = *coefficients;
    filter.reset();
}
