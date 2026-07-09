/*
  ==============================================================================

    MarimbaVoice.cpp
    Phase 2.2: Modal synthesis implementation

  ==============================================================================
*/

#include "MarimbaVoice.h"
#include "TuningEngine.h"
#include <cmath>

MarimbaVoice::MarimbaVoice()
{
    // Initialize modal modes
    for (auto& mode : modes)
    {
        mode.reset();
    }

    exciter.reset();
}

bool MarimbaVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<MarimbaSound*>(sound) != nullptr;
}

void MarimbaVoice::startNote(int midiNoteNumber, float velocityValue,
                             juce::SynthesiserSound* /*sound*/,
                             int /*currentPitchWheelPosition*/)
{
    // Store velocity for amplitude scaling
    velocity = velocityValue;

    // Calculate base frequency (12-TET)
    float baseFreq = static_cast<float>(noteToFrequency(midiNoteNumber));

    // Calculate modal coefficients (done ONCE per note, not per sample)
    calculateModalCoefficients(baseFreq);

    // Reset all modal state
    for (auto& mode : modes)
    {
        mode.reset();
    }

    // Configure mallet exciter based on MALLET_HARDNESS and velocity
    float scaledVelocity = applyVelocityCurve(velocityValue);

    // v1.6.1: Exciter duration: 2ms (soft) to 25ms (hard) - 2x more extreme range
    float durationMS = 2.0f + malletHardness * 23.0f;
    exciter.samplesRemaining = static_cast<int>(durationMS * sampleRate / 1000.0f);

    // v1.6.1: Exciter filter coefficient: 2x more extreme
    // Maps to effective cutoff: ~800Hz (soft) to ~14kHz (hard)
    exciter.filterCoefficient = 0.03f + malletHardness * 0.77f;

    // Exciter amplitude scaled by velocity
    exciter.amplitude = scaledVelocity;

    // Reset filter state
    exciter.filterState = 0.0f;

    // Mark voice as active
    isActive = true;

    // Calculate approximate release time (when modes decay below audible threshold)
    // Use fundamental mode decay time as reference
    float maxDecayTime = getDecayTime(0, resonance, overtoneDamping);
    samplesUntilRelease = static_cast<int>(maxDecayTime * sampleRate * 1.5f); // 1.5x for safety

    // WR-04: length of the linear tail fade (5 ms) applied just before termination so the
    // voice ramps to zero instead of hard-cutting at ~-13 dB (which clicked on every note).
    // The total voice lifetime is unchanged — only the final 5 ms is ramped.
    fadeOutSamples = juce::jmax(1, static_cast<int>(0.005 * sampleRate));

    // v1.6.0: Reset tone filter state for new note
    toneFilterState = 0.0f;
}

void MarimbaVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (!allowTailOff)
    {
        // Immediate cutoff - clear voice
        clearCurrentNote();
        isActive = false;
        for (auto& mode : modes)
        {
            mode.reset();
        }
        exciter.reset();
    }
    // If allowTailOff, let modal resonators naturally decay
    // (no explicit noteOff needed - modes decay on their own)
}

void MarimbaVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
    // Pitch wheel not implemented in Phase 2.2 (can be added later)
}

void MarimbaVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // CC not implemented in Phase 2.2
}

void MarimbaVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                   int startSample, int numSamples)
{
    // Check if voice is active
    if (!isActive)
        return;

    juce::ScopedNoDenormals noDenormals;

    // Render each sample
    while (--numSamples >= 0)
    {
        // Generate exciter sample (filtered noise burst)
        float excitation = exciter.nextSample();

        // Feed excitation through all modal resonators and sum
        float modalSum = 0.0f;

        for (auto& mode : modes)
        {
            // Each mode processes the excitation independently
            float modeOutput = mode.processSample(excitation);
            modalSum += modeOutput * mode.amplitude;
        }

        // v1.12.0: +6dB synthesis gain applied at modal output (not output scaling)
        static constexpr float SYNTHESIS_GAIN = 2.0f;  // +6dB
        float finalSample = modalSum * SYNTHESIS_GAIN;

        // v1.6.0: Apply tone lowpass filter (one-pole)
        // y[n] = y[n-1] + coeff * (x[n] - y[n-1])
        toneFilterState += toneFilterCoeff * (finalSample - toneFilterState);
        finalSample = toneFilterState;

        // WR-04: linear tail fade over the final fadeOutSamples so the note ramps to
        // silence instead of hard-cutting at full ring amplitude (the source of the click).
        if (samplesUntilRelease < fadeOutSamples && fadeOutSamples > 0)
            finalSample *= static_cast<float>(samplesUntilRelease) / static_cast<float>(fadeOutSamples);

        // Write to all output channels
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample, finalSample);
        }

        ++startSample;

        // Decrement release counter
        if (samplesUntilRelease > 0)
        {
            --samplesUntilRelease;
        }
        else
        {
            // Voice has decayed - can be stolen
            clearCurrentNote();
            isActive = false;
            break;
        }
    }
}

void MarimbaVoice::setOutputGain(float gainDB)
{
    // Convert dB to linear gain
    outputGain = juce::Decibels::decibelsToGain(gainDB);
}

void MarimbaVoice::setVelocityCurve(float curve)
{
    velocityCurve = juce::jlimit(0.0f, 1.0f, curve);
}

void MarimbaVoice::setMalletHardness(float hardness)
{
    malletHardness = juce::jlimit(0.0f, 1.0f, hardness);
}

void MarimbaVoice::setBarMaterial(float material)
{
    barMaterial = juce::jlimit(0.0f, 1.0f, material);
}

void MarimbaVoice::setResonance(float resonanceParam)
{
    resonance = juce::jlimit(0.0f, 1.0f, resonanceParam);
}

void MarimbaVoice::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
}

// v1.6.0: Strike Position - affects mode amplitude distribution
void MarimbaVoice::setStrikePosition(float position)
{
    strikePosition = juce::jlimit(0.0f, 1.0f, position);
}

// v1.6.0: Overtone Damping - controls upper mode decay rate
void MarimbaVoice::setOvertoneDamping(float damping)
{
    overtoneDamping = juce::jlimit(0.0f, 1.0f, damping);
}

// v1.6.1: Tone - post-synthesis brightness control, 2x more extreme range
void MarimbaVoice::setTone(float toneVal)
{
    toneValue = juce::jlimit(0.0f, 1.0f, toneVal);
    // Calculate lowpass filter coefficient
    // Maps 0.0 → 400Hz cutoff (very dark), 1.0 → 20kHz (fully open)
    float cutoffHz = 400.0f + toneValue * 19600.0f;  // 400Hz to 20kHz
    float omega = juce::MathConstants<float>::twoPi * cutoffHz / static_cast<float>(sampleRate);
    toneFilterCoeff = omega / (omega + 1.0f);  // One-pole lowpass coefficient
}

double MarimbaVoice::noteToFrequency(int midiNote) const
{
    // Phase 2.3: Use tuning engine if available
    if (tuningEngine)
        return tuningEngine->getFrequency(midiNote);

    // Fallback to 12-TET if no tuning engine
    return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

float MarimbaVoice::applyVelocityCurve(float rawVelocity) const
{
    // VEL_CURVE parameter: 0.0 = linear, 1.0 = steep exponential
    // v1.12.0: Extended range for wider dynamics (exp 1.0→3.0, was 1.0→2.0)
    float exponent = 1.0f + velocityCurve * 2.0f;
    float shapedVelocity = std::pow(rawVelocity, exponent);

    // v1.12.0: Extended velocity-dependent boost for wider dynamic range
    // Low velocity = -6dB, High velocity = +6dB (12dB range, was 6dB)
    float boostDB = -6.0f + rawVelocity * 12.0f;
    float boostMultiplier = std::pow(10.0f, boostDB / 20.0f);

    return shapedVelocity * boostMultiplier;
}

// v1.6.0: Strike position multiplier - simulates mallet strike location on bar
float MarimbaVoice::getStrikePositionMultiplier(int modeIndex, float strikePos) const
{
    // Physical model: When striking at a nodal point for a given mode,
    // that mode is suppressed. Center strikes emphasize even modes (especially fundamental
    // and mode 1 - the double octave), while edge strikes bring out higher odd modes.
    //
    // strikePos: 0.0 = edge, 0.5 = center, 1.0 = other edge (symmetric)
    //
    // Simplified nodal model using sine-based modulation:
    // - Mode 0 (fundamental): always present but slightly reduced at extreme edges
    // - Mode 1 (4x): strongest at center
    // - Higher modes: alternate between center/edge preference

    // Normalize to 0-1 range with center at 0.5
    float centerDist = std::abs(strikePos - 0.5f) * 2.0f;  // 0 at center, 1 at edges

    // v1.6.1: Mode-specific response - 2x more extreme effect
    switch (modeIndex)
    {
        case 0:  // Fundamental - reduced at edges
            return 1.0f - centerDist * 0.3f;   // 1.0 at center, 0.7 at edges
        case 1:  // Double octave (4x) - strongly center-focused
            return 1.0f - centerDist * 0.8f;   // 1.0 at center, 0.2 at edges
        case 2:  // 9.24x - strongly edge-focused
            return 0.4f + centerDist * 0.6f;   // 0.4 at center, 1.0 at edges
        case 3:  // 16.27x - center-focused
            return 1.0f - centerDist * 0.6f;   // 1.0 at center, 0.4 at edges
        case 4:  // 24.22x - strongly edge-focused
            return 0.2f + centerDist * 0.8f;   // 0.2 at center, 1.0 at edges
        case 5:  // 33.54x - center-focused
            return 1.0f - centerDist * 0.7f;   // 1.0 at center, 0.3 at edges
        case 6:  // 42.97x - strongly edge-focused
            return 0.1f + centerDist * 0.9f;   // 0.1 at center, 1.0 at edges
        case 7:  // 54x - center-focused
            return 1.0f - centerDist * 0.8f;   // 1.0 at center, 0.2 at edges
        default:
            return 1.0f;
    }
}

float MarimbaVoice::getModeAmplitude(int modeIndex, float material, float strikePos) const
{
    // BAR_MATERIAL: 0.0 = dark rosewood (fundamental emphasis)
    //               1.0 = bright synthetic (high modes emphasis)
    //
    // v1.5.0: Improved amplitude distribution based on acoustic research
    // - Strong fundamental (mode 0)
    // - Strong mode 2 (the tuned double octave - characteristic marimba sound)
    // - Faster exponential rolloff for higher modes (more natural)

    // Base amplitudes derived from marimba spectral analysis
    // Mode 2 (4x fundamental) is deliberately strong - this is the "marimba character"
    static constexpr std::array<float, NUM_MODES> BASE_AMPLITUDES = {
        1.00f,   // Mode 0: Fundamental (strongest)
        0.65f,   // Mode 1: 4x - tuned double octave (strong - marimba signature)
        0.25f,   // Mode 2: 9.24x
        0.12f,   // Mode 3: 16.27x
        0.06f,   // Mode 4: 24.22x
        0.03f,   // Mode 5: 33.54x
        0.015f,  // Mode 6: 42.97x
        0.008f   // Mode 7: 54x (barely audible, adds shimmer)
    };

    float baseAmp = BASE_AMPLITUDES[static_cast<size_t>(modeIndex)];

    // v1.6.1: Material adjustment: 2x more extreme range
    // Dark rosewood (0.0): Attenuate higher modes to 0.4x
    // Bright synthetic (1.0): Boost higher modes to 4x
    if (modeIndex > 1)  // Don't adjust fundamental or mode 2
    {
        float materialBoost = 0.4f + material * 3.6f;  // 0.4x to 4.0x
        baseAmp *= materialBoost;
    }

    // v1.6.0: Apply strike position modulation
    float strikeMultiplier = getStrikePositionMultiplier(modeIndex, strikePos);
    baseAmp *= strikeMultiplier;

    return baseAmp;
}

float MarimbaVoice::getDecayTime(int modeIndex, float resonanceParam, float overtoneD) const
{
    // v1.6.1: RESONANCE: 2x more extreme range
    // 0.0 = 0.15s decay (very short, staccato)
    // 1.0 = 10.0s decay (very long, pad-like)

    // Base decay time from resonance parameter
    float baseDecay = 0.15f + resonanceParam * 9.85f; // 0.15 to 10.0 seconds

    // v1.6.1: OVERTONE_DAMPING: 2x more extreme range
    // overtoneD = 0.0: All modes sustain almost equally (bell/pad-like shimmer)
    //             0.5: Natural marimba behavior (default)
    //             1.0: Upper modes decay extremely fast (very tight, dry)
    //
    // The damping factor per mode ranges from 0.02 (minimal) to 0.9 (aggressive)
    float dampingPerMode = 0.02f + overtoneD * 0.88f;  // 0.02 to 0.9

    // Higher modes decay faster (physical characteristic of marimba bars)
    float modeFactor = 1.0f / (1.0f + static_cast<float>(modeIndex) * dampingPerMode);

    return baseDecay * modeFactor;
}

void MarimbaVoice::calculateModalCoefficients(float baseFreq)
{
    // Calculate biquad coefficients for each modal resonator
    // This is called ONCE per note in startNote(), not in the audio loop

    for (int i = 0; i < NUM_MODES; ++i)
    {
        // Calculate mode frequency from base frequency and ratio
        float modeFreq = baseFreq * MODE_RATIOS[i];

        // WR-03: Nyquist guard. A two-pole resonator with theta > pi resonates at the
        // aliased (fs - modeFreq). With ratios up to 54x, upper modes exceed Nyquist for
        // high notes (e.g. mode 3 at MIDI 96 lands ~34 kHz -> folds to ~10 kHz as an
        // inharmonic partial). Silence any mode at/above ~0.45*fs so it can't fold back.
        if (modeFreq >= 0.45f * static_cast<float>(sampleRate))
        {
            modes[i].amplitude = 0.0f;
            modes[i].b0 = 0.0f;
            modes[i].a1 = 0.0f;
            modes[i].a2 = 0.0f;
            continue;
        }

        // Get decay time for this mode (v1.6.0: now uses overtoneDamping)
        float decayTime = getDecayTime(i, resonance, overtoneDamping);

        // Get amplitude for this mode (v1.6.0: now uses strikePosition)
        modes[i].amplitude = getModeAmplitude(i, barMaterial, strikePosition);

        // Calculate biquad coefficients
        // θ = 2π * f / fs (normalized frequency)
        float theta = juce::MathConstants<float>::twoPi * modeFreq / static_cast<float>(sampleRate);

        // r = e^(-1 / (decay_time * sample_rate)) (pole radius - controls decay)
        float r = std::exp(-1.0f / (decayTime * static_cast<float>(sampleRate)));

        // Clamp r to prevent instability (max 0.9999)
        r = juce::jlimit(0.0f, 0.9999f, r);

        // g = amplitude * (1 - r) (gain compensation)
        float g = modes[i].amplitude * (1.0f - r);

        // Biquad coefficients for resonator
        modes[i].b0 = g;
        modes[i].a1 = 2.0f * r * std::cos(theta);
        modes[i].a2 = -(r * r);
    }
}
