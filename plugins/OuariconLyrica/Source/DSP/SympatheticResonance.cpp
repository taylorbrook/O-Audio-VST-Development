/*
  ==============================================================================

    SympatheticResonance.cpp
    Sympathetic Resonance Engine - Phase 2.7
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "SympatheticResonance.h"

SympatheticResonanceEngine::SympatheticResonanceEngine()
    : intensity(0.3f)
    , sampleRate(44100.0)
{
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
        spec.maximumBlockSize = 1; // Process one sample at a time
        spec.numChannels = 1;

        voiceInfo.resonatorFilter.prepare(spec);
        designResonatorFilter(voiceInfo.resonatorFilter, voiceInfo.frequency, 5.0f);
    }
}

void SympatheticResonanceEngine::registerVoice(int voiceId, double frequency, const StringMaterial& material)
{
    // Insert or get existing entry, then configure in place
    // (IIR::Filter is not copyable, so we must modify in place)
    auto& voiceInfo = activeVoices[voiceId];
    voiceInfo.frequency = frequency;
    voiceInfo.materialCoupling = material.sympatheticCoupling;
    voiceInfo.lastSample = 0.0f;

    // Energy decay based on material damping (less damped = longer resonance)
    // Range: 0.995 (highly damped) to 0.9998 (very sustained)
    voiceInfo.energyDecay = 0.995f + (1.0f - material.dampingCoeff) * 0.0048f;

    // Prepare resonator filter (bandpass centered at fundamental)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 1;
    spec.numChannels = 1;

    voiceInfo.resonatorFilter.prepare(spec);

    // Design bandpass filter: moderate Q for natural resonance
    designResonatorFilter(voiceInfo.resonatorFilter, frequency, 5.0f);
}

void SympatheticResonanceEngine::unregisterVoice(int voiceId)
{
    activeVoices.erase(voiceId);
}

void SympatheticResonanceEngine::setIntensity(float newIntensity)
{
    intensity = juce::jlimit(0.0f, 1.0f, newIntensity);
}

float SympatheticResonanceEngine::computeSympatheticContribution(int voiceId, float voiceOutput)
{
    // If intensity is zero or voice not found, no sympathetic resonance
    if (intensity <= 0.0f || activeVoices.find(voiceId) == activeVoices.end())
        return 0.0f;

    auto& currentVoice = activeVoices[voiceId];
    float sympatheticSignal = 0.0f;

    // Iterate through all other active voices
    for (auto& pair : activeVoices)
    {
        int otherVoiceId = pair.first;

        // Skip self
        if (otherVoiceId == voiceId)
            continue;

        auto& otherVoice = pair.second;

        // Compute coupling strength based on harmonic relationship
        float coupling = computeCouplingStrength(currentVoice.frequency, otherVoice.frequency);

        if (coupling > 0.0f)
        {
            // Material coupling: both strings must be sympathetically active
            float materialFactor = currentVoice.materialCoupling * otherVoice.materialCoupling;

            // Total coupling: harmonic + material + user intensity
            float totalCoupling = coupling * materialFactor * intensity;

            // Apply energy decay to resonator (simulates energy loss)
            otherVoice.lastSample *= otherVoice.energyDecay;

            // Excite the other voice's resonator with current voice's output
            // This creates the sympathetic vibration
            otherVoice.lastSample += voiceOutput * totalCoupling * 0.05f; // Scale down excitation

            // Process through resonator filter (bandpass at other voice's frequency)
            float resonatorOutput = otherVoice.resonatorFilter.processSample(otherVoice.lastSample);

            // Add to sympathetic signal
            sympatheticSignal += resonatorOutput;
        }
    }

    // Apply gentle compression to prevent buildup with many voices
    // Use soft clipping to maintain natural character
    if (sympatheticSignal > 0.1f)
        sympatheticSignal = 0.1f + std::tanh((sympatheticSignal - 0.1f) * 2.0f) * 0.05f;
    else if (sympatheticSignal < -0.1f)
        sympatheticSignal = -0.1f + std::tanh((sympatheticSignal + 0.1f) * 2.0f) * 0.05f;

    return sympatheticSignal;
}

void SympatheticResonanceEngine::reset()
{
    activeVoices.clear();
}

float SympatheticResonanceEngine::computeCouplingStrength(double freq1, double freq2) const
{
    // Unison (1/1) - strongest coupling
    float unisonCoupling = checkHarmonicInterval(freq1, freq2, 1.0, 10.0f); // ±10 cents
    if (unisonCoupling > 0.0f)
        return unisonCoupling * 0.9f; // Very strong

    // Octave (1/2, 2/1) - strong coupling
    float octaveDownCoupling = checkHarmonicInterval(freq1, freq2, 0.5, 15.0f);
    float octaveUpCoupling = checkHarmonicInterval(freq1, freq2, 2.0, 15.0f);
    float octaveCoupling = std::max(octaveDownCoupling, octaveUpCoupling);
    if (octaveCoupling > 0.0f)
        return octaveCoupling * 0.7f; // Strong

    // Perfect Fifth (2/3, 3/2) - medium coupling
    float fifthDownCoupling = checkHarmonicInterval(freq1, freq2, 2.0/3.0, 20.0f);
    float fifthUpCoupling = checkHarmonicInterval(freq1, freq2, 3.0/2.0, 20.0f);
    float fifthCoupling = std::max(fifthDownCoupling, fifthUpCoupling);
    if (fifthCoupling > 0.0f)
        return fifthCoupling * 0.5f; // Medium

    // Major Third (4/5, 5/4) - weak coupling
    float thirdDownCoupling = checkHarmonicInterval(freq1, freq2, 4.0/5.0, 25.0f);
    float thirdUpCoupling = checkHarmonicInterval(freq1, freq2, 5.0/4.0, 25.0f);
    float thirdCoupling = std::max(thirdDownCoupling, thirdUpCoupling);
    if (thirdCoupling > 0.0f)
        return thirdCoupling * 0.3f; // Weak

    // No significant harmonic relationship
    return 0.0f;
}

float SympatheticResonanceEngine::checkHarmonicInterval(double freq1, double freq2, double ratio, float tolerance) const
{
    // Compute actual frequency ratio
    double actualRatio = freq2 / freq1;

    // Compute deviation from target ratio in cents
    // cents = 1200 * log2(actualRatio / targetRatio)
    double cents = 1200.0 * std::log2(actualRatio / ratio);
    double absCents = std::abs(cents);

    // If within tolerance, return coupling strength based on proximity
    if (absCents < tolerance)
    {
        // Linear falloff: 1.0 at exact match, 0.0 at tolerance edge
        return 1.0f - static_cast<float>(absCents / tolerance);
    }

    return 0.0f;
}

void SympatheticResonanceEngine::designResonatorFilter(juce::dsp::IIR::Filter<float>& filter, double frequency, float Q)
{
    // Design bandpass filter centered at frequency with given Q
    // Q controls bandwidth: higher Q = narrower bandwidth

    // Clamp frequency to valid range
    double clampedFreq = juce::jlimit(20.0, sampleRate * 0.45, frequency);

    // Use JUCE's bandpass filter design
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRate,
        clampedFreq,
        Q
    );

    *filter.coefficients = *coefficients;
    filter.reset();
}
