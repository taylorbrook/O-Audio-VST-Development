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

    // Allocate comb filter buffer for position modeling
    combBuffer.assign(MAX_COMB_DELAY, 0.0f);
    combWriteIndex = 0;
    combDelaySamples = 0.0f;

    // CR-06: Establish the filter coefficient buffers (capacity ≥8) and size the filter state OFF
    // the audio thread. Assigning an ArrayCoefficients std::array uses
    // Coefficients::operator=(std::array) → assignImpl, which grows the buffer once here; the
    // per-note updateBrightnessFilter()/updateTechniqueFilter() then reuse it with NO heap alloc.
    // brightnessFilter is always first-order; techniqueFilter is kept order-2 for ALL techniques
    // (first-order designs are zero-padded into the biquad form — identical transfer function) so
    // its filter order never changes and no state reallocation is triggered on the audio thread.
    *brightnessFilter.coefficients =
        juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass(sampleRate, 3000.0f);
    *techniqueFilter.coefficients =
        juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(sampleRate, 1000.0f, 1.0f, 1.0f);
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
    pluckPosition = juce::jlimit(0.0f, 1.0f, position);
    fingerHardness = juce::jlimit(0.0f, 1.0f, hardness);
    currentFrequency = frequency;

    // Noise burst duration based on velocity (harder pluck = shorter burst)
    // Typical range: 2-10ms
    float burstDurationMs = juce::jmap(velocity, 0.0f, 1.0f, 10.0f, 2.0f);
    noiseBurstSamples = static_cast<int>(burstDurationMs * 0.001f * currentSampleRate);
    noiseBurstRemaining = noiseBurstSamples;

    // v1.20.0: Power curve velocity response with +6dB headroom
    // v^1.4 gives 28dB dynamic range (was 16dB with sqrt*0.5)
    // vel=0.1 → -12dB vs old, vel=1.0 → +6dB vs old
    burstAmplitude = std::pow(velocity, 1.4f);

    // v1.26.0: Glissando excitation softening — lighter brush-style excitation
    // Real harp glissandos have 40-70% energy of a deliberate pluck
    if (glissAmount > 0.0f)
    {
        // Reduce effective velocity (lighter brush contact)
        float velScale = 1.0f - (glissAmount * 0.4f);
        burstAmplitude *= velScale;

        // Narrow the noise burst (shorter contact time = thinner transient)
        noiseBurstSamples = static_cast<int>(noiseBurstSamples * (1.0f - glissAmount * 0.5f));
        noiseBurstRemaining = noiseBurstSamples;

        // Soften finger hardness (brushes are softer, lowering brightness cutoff)
        fingerHardness = juce::jlimit(0.0f, 1.0f, fingerHardness - glissAmount * 0.3f);
    }

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

void PluckExciter::setNoiseAmount(float amount)
{
    noiseAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void PluckExciter::setGlissandoAmount(float amount)
{
    glissAmount = juce::jlimit(0.0f, 1.0f, amount);
}

float PluckExciter::process()
{
    if (!isActive())
        return 0.0f;

    // Generate base noise (only during burst period)
    float noiseSample = 0.0f;
    if (noiseBurstRemaining > 0)
    {
        noiseSample = generateExcitation() * burstAmplitude;
        --noiseBurstRemaining;
    }

    // Apply position comb filter: y[n] = x[n] - x[n - D]
    // Creates spectral nulls at harmonics k/position, matching real pluck physics
    combBuffer[static_cast<size_t>(combWriteIndex)] = noiseSample;
    float delayedSample = 0.0f;
    if (combDelaySamples > 0.0f)
    {
        // Fractional delay with linear interpolation
        int delayInt = static_cast<int>(combDelaySamples);
        float delayFrac = combDelaySamples - static_cast<float>(delayInt);
        int readIndex0 = (combWriteIndex - delayInt + MAX_COMB_DELAY) % MAX_COMB_DELAY;
        int readIndex1 = (readIndex0 - 1 + MAX_COMB_DELAY) % MAX_COMB_DELAY;
        delayedSample = combBuffer[static_cast<size_t>(readIndex0)] * (1.0f - delayFrac)
                      + combBuffer[static_cast<size_t>(readIndex1)] * delayFrac;
    }
    combWriteIndex = (combWriteIndex + 1) % MAX_COMB_DELAY;
    float positionFiltered = noiseSample - delayedSample;

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
    std::fill(combBuffer.begin(), combBuffer.end(), 0.0f);
    combWriteIndex = 0;
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

    // v1.26.0: Glissando brush — shorten attack (quicker contact)
    if (glissAmount > 0.0f)
        attack *= (1.0f - glissAmount * 0.5f);

    envelopeParams.attack = attack;
    envelopeParams.decay = decay;
    envelopeParams.sustain = sustain;
    envelopeParams.release = release;
    envelope.setParameters(envelopeParams);
}

void PluckExciter::updatePositionFilter()
{
    // Feedforward comb filter models physical pluck position.
    // Plucking at position P (fraction of string length) excites harmonics
    // with amplitudes proportional to sin(n*pi*P). Nulls occur at n = k/P.
    //
    // The comb filter y[n] = x[n] - x[n-D] with D = P * (sampleRate/f0)
    // creates nulls at f = k * f0/P, matching the physical null pattern.
    //
    // Examples:
    //   P=0.5 (center): nulls at 2f0, 4f0, 6f0 — only odd harmonics survive (hollow)
    //   P=0.2 (near nut): nulls at 5f0, 10f0 — bright, full spectrum
    //   P=0.8 (near bridge): nulls at 1.25f0, 2.5f0 — thin, nasal

    float clampedPosition = juce::jlimit(0.05f, 0.95f, pluckPosition);
    float period = static_cast<float>(currentSampleRate / currentFrequency);
    combDelaySamples = clampedPosition * period;

    // Clamp to buffer size
    combDelaySamples = juce::jlimit(1.0f, static_cast<float>(MAX_COMB_DELAY - 2), combDelaySamples);
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

    // CR-06: Design one-pole lowpass via the ArrayCoefficients operator= (reuses the pre-seeded
    // buffer, no heap alloc on the audio thread). brightnessFilter is always first-order → its
    // order never changes → no state reallocation.
    *brightnessFilter.coefficients =
        juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass(currentSampleRate, cutoffHz);
}

void PluckExciter::updateTechniqueFilter()
{
    // Technique-specific filtering to shape timbre.
    // CR-06: Fill the (always order-2) technique-filter coefficients via the ArrayCoefficients
    // operator= (reuses the pre-seeded buffer — no heap alloc on the audio thread). First-order
    // designs are zero-padded into the biquad form {b0,b1,0,a0,a1,0} — mathematically identical
    // transfer function — so the filter order never changes and no state reallocation is triggered.
    auto embedFirstOrder = [] (const std::array<float, 4>& fo) -> std::array<float, 6>
    {
        // first-order {b0,b1,a0,a1} → biquad {b0,b1,0,a0,a1,0}
        return { fo[0], fo[1], 0.0f, fo[2], fo[3], 0.0f };
    };

    std::array<float, 6> coeffs {};

    switch (currentTechnique)
    {
        case PlayingTechnique::Normal:
            // Neutral filter (wide bandwidth)
            coeffs = embedFirstOrder(juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass(
                currentSampleRate, 10000.0f));
            break;

        case PlayingTechnique::Harmonic:
            // Position-aware harmonic isolation: touching at 1/N of string → Nth harmonic
            // position 0.5 → 2nd, 0.33 → 3rd, 0.25 → 4th, etc.
            {
                float clampedPos = juce::jlimit(0.1f, 0.5f, pluckPosition);
                float harmonicNumber = std::round(1.0f / clampedPos);
                harmonicNumber = juce::jlimit(2.0f, 8.0f, harmonicNumber);
                float centerFreq = static_cast<float>(currentFrequency * harmonicNumber);
                centerFreq = juce::jlimit(100.0f, 10000.0f, centerFreq);
                float Q = 2.0f;  // Moderate bandwidth

                coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
                    currentSampleRate, centerFreq, Q, 3.0f);  // 3x gain at harmonic
            }
            break;

        case PlayingTechnique::Muted:
            // Heavy lowpass for muted sound
            coeffs = embedFirstOrder(juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass(
                currentSampleRate, 600.0f));
            break;

        case PlayingTechnique::PresDeLaTable:
            // High-shelf boost for metallic/bright character
            coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
                currentSampleRate, 2000.0f, 0.7f, 2.5f);  // Boost highs
            break;
    }

    *techniqueFilter.coefficients = coeffs;
}

float PluckExciter::generateExcitation()
{
    // v1.20.1 FIX: Blend between clean impulse and noise based on noiseAmount
    // Previously, noiseAmount=0 produced zero excitation → silence (bug)
    // Now noiseAmount controls texture: 0=clean half-sine impulse, 1=full noise

    // Clean impulse: half-sine shape over burst duration
    float phase = static_cast<float>(noiseBurstSamples - noiseBurstRemaining)
                / static_cast<float>(noiseBurstSamples);
    float cleanImpulse = std::sin(phase * juce::MathConstants<float>::pi);

    // Noisy excitation: white noise in range [-1.0, 1.0]
    float noise = (noiseSource.nextFloat() * 2.0f) - 1.0f;

    // Crossfade: noiseAmount=0 → pure impulse, noiseAmount=1 → pure noise
    return cleanImpulse * (1.0f - noiseAmount) + noise * noiseAmount;
}
