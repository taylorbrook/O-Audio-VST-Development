/*
  ==============================================================================

    PluckExciter.cpp
    Pluck Excitation Generator Implementation - Phase 2.3
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluckExciter.h"

PluckExciter::PluckExciter()
{
    // Initialize ADSR with fast attack, moderate decay for pluck character
    envelopeParams.attack = 0.002f;   // 2ms attack for sharp transient
    envelopeParams.decay = 0.05f;     // 50ms decay
    envelopeParams.sustain = 0.0f;    // No sustain (pluck is percussive)
    envelopeParams.release = 0.01f;   // 10ms release
    envelope.setParameters(envelopeParams);
}

PluckExciter::~PluckExciter()
{
}

void PluckExciter::prepare(double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);

    currentSampleRate = sampleRate;

    // Prepare filters with ProcessSpec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 1;  // Processing sample-by-sample
    spec.numChannels = 1;

    positionFilter.prepare(spec);
    brightnessFilter.prepare(spec);
    techniqueFilter.prepare(spec);

    // Prepare ADSR
    envelope.setSampleRate(sampleRate);
    envelope.reset();

    // Reset state
    reset();
}

void PluckExciter::trigger(float velocity, float position, float hardness, double frequency)
{
    pluckVelocity = juce::jlimit(0.0f, 1.0f, velocity);
    pluckPosition = juce::jlimit(0.0f, 1.0f, position);
    fingerHardness = juce::jlimit(0.0f, 1.0f, hardness);
    currentFrequency = frequency;

    // Noise burst duration based on velocity (harder pluck = shorter burst)
    // Typical range: 2-10ms
    float burstDurationMs = juce::jmap(velocity, 0.0f, 1.0f, 10.0f, 2.0f);
    noiseBurstSamples = static_cast<int>(burstDurationMs * 0.001f * currentSampleRate);
    noiseBurstRemaining = noiseBurstSamples;

    // Burst amplitude scales with velocity (with slight compression for musicality)
    burstAmplitude = std::sqrt(velocity) * 0.5f;  // sqrt for velocity curve

    // Update all filters based on new parameters
    updateFilters();
    updateEnvelope();

    // Trigger envelope
    envelope.noteOn();
}

void PluckExciter::setTechnique(PlayingTechnique technique)
{
    currentTechnique = technique;
    updateEnvelope();
    updateTechniqueFilter();
}

float PluckExciter::process()
{
    if (!isActive())
        return 0.0f;

    // Generate base noise (only during burst period)
    float noiseSample = 0.0f;
    if (noiseBurstRemaining > 0)
    {
        noiseSample = generateNoise() * burstAmplitude;
        --noiseBurstRemaining;
    }

    // Apply position filtering (comb filter effect)
    float positionFiltered = positionFilter.processSample(noiseSample);

    // Apply brightness/hardness filtering
    float brightnessFiltered = brightnessFilter.processSample(positionFiltered);

    // Apply technique-specific filtering
    float techniqueFiltered = techniqueFilter.processSample(brightnessFiltered);

    // Apply ADSR envelope
    float envelopeValue = envelope.getNextSample();
    float output = techniqueFiltered * envelopeValue;

    return output;
}

bool PluckExciter::isActive() const
{
    return envelope.isActive();
}

void PluckExciter::reset()
{
    envelope.reset();
    positionFilter.reset();
    brightnessFilter.reset();
    techniqueFilter.reset();
    noiseBurstRemaining = 0;
    burstAmplitude = 0.0f;
}

void PluckExciter::updateFilters()
{
    updatePositionFilter();
    updateBrightnessFilter();
    updateTechniqueFilter();
}

void PluckExciter::updateEnvelope()
{
    // Base envelope parameters
    float attack = 0.002f;   // 2ms
    float decay = 0.05f;     // 50ms
    float sustain = 0.0f;
    float release = 0.01f;   // 10ms

    // Modify based on technique
    switch (currentTechnique)
    {
        case PlayingTechnique::Normal:
            // Use defaults
            break;

        case PlayingTechnique::Harmonic:
            // Longer attack for softer touch at harmonic node
            attack = 0.005f;   // 5ms
            decay = 0.08f;     // 80ms
            break;

        case PlayingTechnique::Muted:
            // Very short decay for muted technique
            attack = 0.001f;   // 1ms
            decay = 0.02f;     // 20ms
            release = 0.005f;  // 5ms
            break;

        case PlayingTechnique::PresDeLaTable:
            // Sharp attack, moderate decay for metallic sound
            attack = 0.001f;   // 1ms
            decay = 0.06f;     // 60ms
            break;
    }

    envelopeParams.attack = attack;
    envelopeParams.decay = decay;
    envelopeParams.sustain = sustain;
    envelopeParams.release = release;
    envelope.setParameters(envelopeParams);
}

void PluckExciter::updatePositionFilter()
{
    // Position filter creates comb filtering effect based on pluck position
    // Plucking at position P creates a spectral null at frequency = f0 / P
    //
    // For simplicity, we use a one-pole lowpass that approximates this:
    // - Position near bridge (1.0) = brighter (less filtering)
    // - Position near nut (0.0) = darker (more filtering)
    //
    // More sophisticated: implement actual comb filter with delay = position * period

    // Map position to cutoff frequency
    // Position 0.5 (center) = fundamental frequency
    // Position approaches 1.0 (bridge) = higher cutoffs (brighter)
    // Position approaches 0.0 (nut) = lower cutoffs (darker)

    float cutoffHz;
    if (pluckPosition < 0.5f)
    {
        // Nut side: darker, map 0.0-0.5 to 200Hz-fundamental
        cutoffHz = juce::jmap(pluckPosition, 0.0f, 0.5f, 200.0f, static_cast<float>(currentFrequency));
    }
    else
    {
        // Bridge side: brighter, map 0.5-1.0 to fundamental-4x fundamental
        cutoffHz = juce::jmap(pluckPosition, 0.5f, 1.0f,
                             static_cast<float>(currentFrequency),
                             static_cast<float>(currentFrequency * 4.0));
    }

    // Clamp to reasonable range
    cutoffHz = juce::jlimit(100.0f, 12000.0f, cutoffHz);

    // Design one-pole lowpass filter
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, cutoffHz);

    *positionFilter.coefficients = *coeffs;
}

void PluckExciter::updateBrightnessFilter()
{
    // Brightness filter models finger hardness
    // Soft finger (0.0) = heavy lowpass filtering (warm, mellow)
    // Hard finger (1.0) = minimal filtering (bright, percussive)

    // Map hardness to cutoff frequency
    // Soft: 800 Hz cutoff (warm)
    // Medium: 3000 Hz cutoff (balanced)
    // Hard: 12000 Hz cutoff (bright)
    float cutoffHz = juce::jmap(fingerHardness, 0.0f, 1.0f, 800.0f, 12000.0f);

    // Design one-pole lowpass filter
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, cutoffHz);

    *brightnessFilter.coefficients = *coeffs;
}

void PluckExciter::updateTechniqueFilter()
{
    // Technique-specific filtering to shape timbre

    switch (currentTechnique)
    {
        case PlayingTechnique::Normal:
            // Neutral filter (wide bandwidth)
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
                    currentSampleRate, 10000.0f);
                *techniqueFilter.coefficients = *coeffs;
            }
            break;

        case PlayingTechnique::Harmonic:
            // Bandpass filter to isolate harmonic (centered on 2x fundamental)
            {
                float centerFreq = static_cast<float>(currentFrequency * 2.0);
                centerFreq = juce::jlimit(100.0f, 10000.0f, centerFreq);
                float Q = 2.0f;  // Moderate bandwidth

                auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                    currentSampleRate, centerFreq, Q, 3.0f);  // 3x gain at harmonic
                *techniqueFilter.coefficients = *coeffs;
            }
            break;

        case PlayingTechnique::Muted:
            // Heavy lowpass for muted sound
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
                    currentSampleRate, 600.0f);
                *techniqueFilter.coefficients = *coeffs;
            }
            break;

        case PlayingTechnique::PresDeLaTable:
            // High-shelf boost for metallic/bright character
            {
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                    currentSampleRate, 2000.0f, 0.7f, 2.5f);  // Boost highs
                *techniqueFilter.coefficients = *coeffs;
            }
            break;
    }
}

float PluckExciter::generateNoise()
{
    // Generate white noise in range [-1.0, 1.0]
    return (noiseSource.nextFloat() * 2.0f) - 1.0f;
}
