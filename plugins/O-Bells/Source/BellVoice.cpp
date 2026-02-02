/*
  ==============================================================================

    BellVoice.cpp
    O-Bells - Physical Modeling Bell Synthesizer
    Modal synthesis voice implementation

  ==============================================================================
*/

#include "BellVoice.h"
#include <cmath>

BellVoice::BellVoice()
{
}

bool BellVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<BellSound*>(sound) != nullptr;
}

void BellVoice::prepare(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
}

void BellVoice::updateParameters(float inharmonicity, float damping, float brightness,
                                 float strikePosition, float malletHardness, float material,
                                 int unisonCount, float unisonDetune,
                                 float octaveBlendSub, float octaveBlendOct, float stereoSpread,
                                 float partialTuning, float pitchEnvelope, float pitchEnvTime,
                                 int decayShape, int velocityCurve, float nonlinearEffects,
                                 int strikeNoiseChar, float outputGain,
                                 float strikeTime, float brilliance, float bodyTime, float humSustain)
{
    currentInharmonicity = inharmonicity;
    currentDamping = damping;
    currentBrightness = brightness;
    currentStrikePosition = strikePosition;
    currentMalletHardness = malletHardness;
    currentMaterial = material;
    currentUnisonCount = juce::jlimit(1, MAX_UNISON, unisonCount);
    currentUnisonDetune = unisonDetune;
    currentOctaveBlendSub = octaveBlendSub;
    currentOctaveBlendOct = octaveBlendOct;
    currentStereoSpread = stereoSpread;
    currentPartialTuning = partialTuning;
    currentPitchEnvelope = pitchEnvelope;
    currentPitchEnvTime = pitchEnvTime;
    currentDecayShape = decayShape;
    currentVelocityCurve = velocityCurve;
    currentNonlinearEffects = nonlinearEffects;
    currentStrikeNoiseChar = strikeNoiseChar;
    currentOutputGain = outputGain;
    // Multi-stage envelope params
    currentStrikeTime = strikeTime;
    currentBrilliance = brilliance;
    currentBodyTime = bodyTime;
    currentHumSustain = humSustain;
}

void BellVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentMidiNote = midiNoteNumber;
    currentVelocity = applyVelocityCurve(velocity, currentVelocityCurve);
    noteActive = true;
    tailOff = false;
    releaseEnvelope = 1.0f;
    releaseRate = 1.0f;
    samplesSinceNoteOn = 0;  // Reset sample counter for multi-stage envelope

    // Calculate fundamental frequency
    float fundamental = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    // Pre-calculate multi-stage coefficients if using multi-stage decay
    if (currentDecayShape == 2)
    {
        calculateMultiStageCoefficients(fundamental);
    }

    // Initialize pitch envelope
    pitchEnvelopePhase = 1.0f;  // Start at max deviation
    if (currentPitchEnvTime > 0.0f)
    {
        // Exponential decay rate for pitch envelope return
        float timeInSeconds = currentPitchEnvTime / 1000.0f;
        pitchEnvelopeDecayRate = std::exp(-1.0f / (timeInSeconds * static_cast<float>(currentSampleRate)));
    }
    else
    {
        pitchEnvelopeDecayRate = 0.0f;
    }

    // Calculate unison detune amounts and pan positions
    calculateUnisonDetunes(currentUnisonCount, currentUnisonDetune);

    // Initialize fundamental voices
    for (int i = 0; i < currentUnisonCount; ++i)
    {
        float detunedFundamental = fundamental * std::pow(2.0f, fundamentalVoices[i].detuneAmount / 1200.0f);
        initializePartials(detunedFundamental, currentVelocity);

        // Copy initialized partials to fundamental voice
        for (int p = 0; p < NUM_PARTIALS; ++p)
        {
            fundamentalVoices[i].partials[p] = fundamentalVoices[0].partials[p];
            fundamentalVoices[i].partials[p].frequency = detunedFundamental *
                calculatePartialFrequency(p, 1.0f, currentInharmonicity);
            fundamentalVoices[i].partials[p].phaseIncrement =
                fundamentalVoices[i].partials[p].frequency / static_cast<float>(currentSampleRate);
        }
    }

    // Initialize sub-octave voices if blend > 0
    if (currentOctaveBlendSub > 0.0f)
    {
        for (int i = 0; i < currentUnisonCount; ++i)
        {
            float subFundamental = fundamental * 0.5f * std::pow(2.0f, subOctaveVoices[i].detuneAmount / 1200.0f);
            for (int p = 0; p < NUM_PARTIALS; ++p)
            {
                subOctaveVoices[i].partials[p] = fundamentalVoices[i].partials[p];
                subOctaveVoices[i].partials[p].frequency = subFundamental *
                    calculatePartialFrequency(p, 1.0f, currentInharmonicity);
                subOctaveVoices[i].partials[p].phaseIncrement =
                    subOctaveVoices[i].partials[p].frequency / static_cast<float>(currentSampleRate);
            }
        }
    }

    // Initialize upper-octave voices if blend > 0
    if (currentOctaveBlendOct > 0.0f)
    {
        for (int i = 0; i < currentUnisonCount; ++i)
        {
            float octFundamental = fundamental * 2.0f * std::pow(2.0f, upperOctaveVoices[i].detuneAmount / 1200.0f);
            for (int p = 0; p < NUM_PARTIALS; ++p)
            {
                upperOctaveVoices[i].partials[p] = fundamentalVoices[i].partials[p];
                upperOctaveVoices[i].partials[p].frequency = octFundamental *
                    calculatePartialFrequency(p, 1.0f, currentInharmonicity);
                upperOctaveVoices[i].partials[p].phaseIncrement =
                    upperOctaveVoices[i].partials[p].frequency / static_cast<float>(currentSampleRate);
            }
        }
    }

    // Initialize strike noise transient
    strikeNoise.active = true;
    strikeNoise.amplitude = currentVelocity * (0.5f + currentMalletHardness * 0.5f);
    strikeNoise.filterState = 0.0f;
    strikeNoise.bp1 = 0.0f;
    strikeNoise.bp2 = 0.0f;

    // Configure based on strike noise character
    switch (currentStrikeNoiseChar)
    {
        case 0:  // Click - high-pass, very short, bright
        {
            float decayTime = juce::jmap(currentMalletHardness, 0.008f, 0.003f);  // 8ms to 3ms
            strikeNoise.decayRate = std::exp(-1.0f / (decayTime * static_cast<float>(currentSampleRate)));
            // High-pass: negative coefficient, cutoff around 2kHz
            strikeNoise.filterCoeff = -0.85f;
            strikeNoise.resonance = 0.0f;
            break;
        }
        case 1:  // Thud - low-pass, longer, dark
        {
            float decayTime = juce::jmap(currentMalletHardness, 0.030f, 0.015f);  // 30ms to 15ms
            strikeNoise.decayRate = std::exp(-1.0f / (decayTime * static_cast<float>(currentSampleRate)));
            // Low-pass: positive coefficient, cutoff around 500Hz
            strikeNoise.filterCoeff = 0.92f;
            strikeNoise.resonance = 0.0f;
            break;
        }
        case 2:  // Ping - bandpass resonant, medium, metallic
        default:
        {
            float decayTime = juce::jmap(currentMalletHardness, 0.020f, 0.008f);  // 20ms to 8ms
            strikeNoise.decayRate = std::exp(-1.0f / (decayTime * static_cast<float>(currentSampleRate)));
            strikeNoise.filterCoeff = 0.0f;  // Use bandpass instead
            // Resonant bandpass centered on fundamental frequency
            strikeNoise.centerFreq = fundamental;
            strikeNoise.resonance = 0.95f;  // High Q for metallic ring
            break;
        }
    }
}

void BellVoice::stopNote(float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity);

    if (allowTailOff)
    {
        tailOff = true;

        // Gradual release envelope - bell rings out naturally
        // Release time: 0.5s (high damping) to 3s (low damping)
        float releaseTimeSeconds = juce::jmap(currentDamping, 3.0f, 0.5f);
        releaseRate = std::exp(-1.0f / (releaseTimeSeconds * static_cast<float>(currentSampleRate)));
    }
    else
    {
        // Even "immediate" stop gets a short fade to prevent clicks (50ms)
        tailOff = true;
        float quickReleaseTime = 0.05f;
        releaseRate = std::exp(-1.0f / (quickReleaseTime * static_cast<float>(currentSampleRate)));
    }
}

void BellVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!noteActive)
        return;

    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftOutput = 0.0f;
        float rightOutput = 0.0f;

        // Generate strike noise transient
        float noiseSignal = 0.0f;
        if (strikeNoise.active)
        {
            noiseSignal = generateStrikeNoise();
            strikeNoise.amplitude *= strikeNoise.decayRate;

            // Deactivate when amplitude is very low
            if (strikeNoise.amplitude < 0.001f)
                strikeNoise.active = false;
        }

        // Process pitch envelope
        float pitchModulation = processPitchEnvelope();

        // Check if voice should stop (all partials silent)
        bool anyPartialActive = false;

        // Process fundamental voices
        for (int i = 0; i < currentUnisonCount; ++i)
        {
            float voiceSample = 0.0f;

            for (int p = 0; p < NUM_PARTIALS; ++p)
            {
                auto& partial = fundamentalVoices[i].partials[p];
                if (partial.active)
                {
                    // Apply pitch envelope modulation
                    float modulatedPhaseInc = partial.phaseIncrement * (1.0f + pitchModulation);
                    partial.phase += modulatedPhaseInc;
                    if (partial.phase >= 1.0f)
                        partial.phase -= 1.0f;

                    float partialSample = std::sin(partial.phase * juce::MathConstants<float>::twoPi);
                    voiceSample += partialSample * partial.amplitude;

                    // Apply decay envelope based on decay shape
                    if (currentDecayShape == 2)  // Multi-stage (research-based)
                        applyMultiStageDecay(partial, p);
                    else if (currentDecayShape == 1)  // Exponential
                        partial.amplitude *= partial.decayRate;
                    else  // Linear (decayShape == 0)
                        partial.amplitude -= partial.decayRate * 0.0001f;

                    // Check if partial is still active
                    if (partial.amplitude > 0.001f)
                        anyPartialActive = true;
                    else
                        partial.active = false;
                }
            }

            // Apply panning
            float panLeft = std::sqrt(0.5f * (1.0f - fundamentalVoices[i].panPosition));
            float panRight = std::sqrt(0.5f * (1.0f + fundamentalVoices[i].panPosition));

            leftOutput += voiceSample * panLeft;
            rightOutput += voiceSample * panRight;
        }

        // Process sub-octave voices
        if (currentOctaveBlendSub > 0.0f)
        {
            for (int i = 0; i < currentUnisonCount; ++i)
            {
                float voiceSample = 0.0f;

                for (int p = 0; p < NUM_PARTIALS; ++p)
                {
                    auto& partial = subOctaveVoices[i].partials[p];
                    if (partial.active)
                    {
                        float modulatedPhaseInc = partial.phaseIncrement * (1.0f + pitchModulation);
                        partial.phase += modulatedPhaseInc;
                        if (partial.phase >= 1.0f)
                            partial.phase -= 1.0f;

                        float partialSample = std::sin(partial.phase * juce::MathConstants<float>::twoPi);
                        voiceSample += partialSample * partial.amplitude;

                        // Apply decay envelope based on decay shape
                        if (currentDecayShape == 2)
                            applyMultiStageDecay(partial, p);
                        else if (currentDecayShape == 1)
                            partial.amplitude *= partial.decayRate;
                        else
                            partial.amplitude -= partial.decayRate * 0.0001f;

                        if (partial.amplitude > 0.001f)
                            anyPartialActive = true;
                        else
                            partial.active = false;
                    }
                }

                float panLeft = std::sqrt(0.5f * (1.0f - subOctaveVoices[i].panPosition));
                float panRight = std::sqrt(0.5f * (1.0f + subOctaveVoices[i].panPosition));

                leftOutput += voiceSample * currentOctaveBlendSub * panLeft;
                rightOutput += voiceSample * currentOctaveBlendSub * panRight;
            }
        }

        // Process upper-octave voices
        if (currentOctaveBlendOct > 0.0f)
        {
            for (int i = 0; i < currentUnisonCount; ++i)
            {
                float voiceSample = 0.0f;

                for (int p = 0; p < NUM_PARTIALS; ++p)
                {
                    auto& partial = upperOctaveVoices[i].partials[p];
                    if (partial.active)
                    {
                        float modulatedPhaseInc = partial.phaseIncrement * (1.0f + pitchModulation);
                        partial.phase += modulatedPhaseInc;
                        if (partial.phase >= 1.0f)
                            partial.phase -= 1.0f;

                        float partialSample = std::sin(partial.phase * juce::MathConstants<float>::twoPi);
                        voiceSample += partialSample * partial.amplitude;

                        // Apply decay envelope based on decay shape
                        if (currentDecayShape == 2)
                            applyMultiStageDecay(partial, p);
                        else if (currentDecayShape == 1)
                            partial.amplitude *= partial.decayRate;
                        else
                            partial.amplitude -= partial.decayRate * 0.0001f;

                        if (partial.amplitude > 0.001f)
                            anyPartialActive = true;
                        else
                            partial.active = false;
                    }
                }

                float panLeft = std::sqrt(0.5f * (1.0f - upperOctaveVoices[i].panPosition));
                float panRight = std::sqrt(0.5f * (1.0f + upperOctaveVoices[i].panPosition));

                leftOutput += voiceSample * currentOctaveBlendOct * panLeft;
                rightOutput += voiceSample * currentOctaveBlendOct * panRight;
            }
        }

        // Increment sample counter for multi-stage envelope timing
        if (currentDecayShape == 2)
            ++samplesSinceNoteOn;

        // Add strike noise transient
        leftOutput += noiseSignal * 0.3f;
        rightOutput += noiseSignal * 0.3f;

        // Apply release envelope (gradual fade on note-off)
        if (tailOff)
        {
            leftOutput *= releaseEnvelope;
            rightOutput *= releaseEnvelope;
            releaseEnvelope *= releaseRate;

            // Stop voice when release envelope fades out
            if (releaseEnvelope < 0.001f)
            {
                clearCurrentNote();
                noteActive = false;
                break;
            }
        }

        // Apply nonlinear effects (soft clipping/waveshaping)
        if (currentNonlinearEffects > 0.0f)
        {
            float nlAmount = currentNonlinearEffects;
            leftOutput = std::tanh(leftOutput * (1.0f + nlAmount * 2.0f)) / (1.0f + nlAmount * 2.0f);
            rightOutput = std::tanh(rightOutput * (1.0f + nlAmount * 2.0f)) / (1.0f + nlAmount * 2.0f);
        }

        // Normalize signal to prevent clipping
        // Account for: partial count (~2.7x from harmonic series), unison voices, and octave layers
        float partialNorm = 0.4f;  // Compensate for 8 partials summing to ~2.5-3x
        float unisonNorm = 1.0f / std::sqrt(static_cast<float>(currentUnisonCount));
        float layerNorm = 1.0f / (1.0f + currentOctaveBlendSub + currentOctaveBlendOct);
        float totalNorm = partialNorm * unisonNorm * layerNorm;

        leftOutput *= totalNorm;
        rightOutput *= totalNorm;

        // Apply output gain (after normalization so 0dB = unity)
        float outputGainLinear = juce::Decibels::decibelsToGain(currentOutputGain);
        leftOutput *= outputGainLinear;
        rightOutput *= outputGainLinear;

        // Write to output buffer
        int outputSample = startSample + sample;
        if (outputSample < outputBuffer.getNumSamples())
        {
            outputBuffer.addSample(0, outputSample, leftOutput);
            if (outputBuffer.getNumChannels() > 1)
                outputBuffer.addSample(1, outputSample, rightOutput);
        }

        // Check if voice should stop (all partials decayed naturally)
        if (!anyPartialActive && !strikeNoise.active && !tailOff)
        {
            clearCurrentNote();
            noteActive = false;
            break;
        }
    }
}

// Helper function implementations

float BellVoice::calculatePartialFrequency(int partialIndex, float fundamental, float inharmonicity)
{
    float ratio;

    if (inharmonicity < 0.5f)
    {
        // Interpolate harmonic → church bell
        ratio = juce::jmap(inharmonicity, 0.0f, 0.5f,
                          harmonicRatios[partialIndex],
                          bellRatios[partialIndex]);
    }
    else
    {
        // Interpolate church bell → gamelan
        ratio = juce::jmap(inharmonicity, 0.5f, 1.0f,
                          bellRatios[partialIndex],
                          gamelanRatios[partialIndex]);
    }

    // Apply partial tuning to the "tierce" partial (index 2, the minor third at ~2.4x)
    if (partialIndex == 2 && currentPartialTuning != 0.0f)
    {
        ratio *= std::pow(2.0f, currentPartialTuning / 1200.0f);
    }

    return fundamental * ratio;
}

float BellVoice::calculateStrikePositionGain(int partialIndex, float position)
{
    // Comb filter effect: nodes at integer multiples of position
    float phase = juce::MathConstants<float>::pi * position * (partialIndex + 1);
    return std::abs(std::sin(phase));
}

float BellVoice::calculatePartialAmplitude(int partialIndex, float brightness)
{
    // Base amplitude decreases with partial number
    float baseAmp = 1.0f / (partialIndex + 1.0f);

    // Brightness scales upper partials
    float brightnessScale = 1.0f + brightness * (partialIndex / static_cast<float>(NUM_PARTIALS));

    return baseAmp * brightnessScale;
}

float BellVoice::applyVelocityCurve(float velocity, int curve)
{
    switch (curve)
    {
        case 1:  // Exponential
            return velocity * velocity;
        case 2:  // Logarithmic
            return std::sqrt(velocity);
        default:  // Linear
            return velocity;
    }
}

float BellVoice::calculateMaterialDecayMultiplier(float material)
{
    // Material: 0 = Bronze, 0.33 = Steel, 0.67 = Glass, 1.0 = Crystal
    if (material < 0.33f)
    {
        // Bronze → Steel
        return juce::jmap(material, 0.0f, 0.33f, MATERIAL_DECAY_BRONZE, MATERIAL_DECAY_STEEL);
    }
    else if (material < 0.67f)
    {
        // Steel → Glass
        return juce::jmap(material, 0.33f, 0.67f, MATERIAL_DECAY_STEEL, MATERIAL_DECAY_GLASS);
    }
    else
    {
        // Glass → Crystal
        return juce::jmap(material, 0.67f, 1.0f, MATERIAL_DECAY_GLASS, MATERIAL_DECAY_CRYSTAL);
    }
}

void BellVoice::calculateUnisonDetunes(int count, float detuneAmount)
{
    if (count == 1)
    {
        fundamentalVoices[0].detuneAmount = 0.0f;
        fundamentalVoices[0].panPosition = 0.0f;
        return;
    }

    // Symmetric detune spread with stereo panning
    for (int i = 0; i < count; ++i)
    {
        float offset;
        if (count % 2 == 0)
        {
            // Even count: no center voice
            offset = (i - (count / 2.0f - 0.5f)) / (count / 2.0f);
        }
        else
        {
            // Odd count: center voice at 0
            offset = (i - (count / 2)) / static_cast<float>(count / 2);
        }

        fundamentalVoices[i].detuneAmount = offset * detuneAmount;
        subOctaveVoices[i].detuneAmount = offset * detuneAmount;
        upperOctaveVoices[i].detuneAmount = offset * detuneAmount;

        // Apply stereo spread
        float panAmount = offset * currentStereoSpread;
        fundamentalVoices[i].panPosition = juce::jlimit(-1.0f, 1.0f, panAmount);
        subOctaveVoices[i].panPosition = juce::jlimit(-1.0f, 1.0f, panAmount);
        upperOctaveVoices[i].panPosition = juce::jlimit(-1.0f, 1.0f, panAmount);
    }
}

void BellVoice::initializePartials(float fundamental, float velocity)
{
    float materialDecayMult = calculateMaterialDecayMultiplier(currentMaterial);

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        auto& partial = fundamentalVoices[0].partials[p];

        partial.frequency = calculatePartialFrequency(p, fundamental, currentInharmonicity);
        partial.phaseIncrement = partial.frequency / static_cast<float>(currentSampleRate);
        partial.phase = 0.0f;

        // Calculate initial amplitude
        float baseAmplitude = calculatePartialAmplitude(p, currentBrightness);
        float strikeGain = calculateStrikePositionGain(p, currentStrikePosition);
        float malletGain = 1.0f + currentMalletHardness * (p / static_cast<float>(NUM_PARTIALS));

        partial.amplitude = baseAmplitude * strikeGain * malletGain * velocity;
        partial.targetAmplitude = partial.amplitude;

        // Calculate decay rate (exponential decay)
        float baseDecayTime = juce::jmap(currentDamping, 0.5f, 5.0f);  // 0.5s to 5s
        float partialDecayTime = baseDecayTime * DECAY_MULTIPLIERS[p] * materialDecayMult;
        partial.decayRate = std::exp(-1.0f / (partialDecayTime * static_cast<float>(currentSampleRate)));

        partial.active = partial.amplitude > 0.001f;
    }
}

float BellVoice::generateStrikeNoise()
{
    // Generate white noise
    float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
    float output = 0.0f;

    if (currentStrikeNoiseChar == 2)  // Ping - resonant bandpass
    {
        // Simple state-variable bandpass filter
        float f = 2.0f * std::sin(juce::MathConstants<float>::pi * strikeNoise.centerFreq /
                                   static_cast<float>(currentSampleRate));
        f = juce::jlimit(0.0f, 1.0f, f);  // Clamp for stability

        float q = strikeNoise.resonance;
        strikeNoise.bp1 += f * (noise - strikeNoise.bp1 - q * strikeNoise.bp2);
        strikeNoise.bp2 += f * strikeNoise.bp1;

        output = strikeNoise.bp1 * strikeNoise.amplitude;
    }
    else if (strikeNoise.filterCoeff > 0.0f)  // Thud - lowpass
    {
        // One-pole lowpass: y[n] = coeff * y[n-1] + (1-coeff) * x[n]
        strikeNoise.filterState = strikeNoise.filterCoeff * strikeNoise.filterState +
                                  (1.0f - strikeNoise.filterCoeff) * noise;
        output = strikeNoise.filterState * strikeNoise.amplitude * 2.0f;  // Gain boost for low end
    }
    else  // Click - highpass
    {
        // Simple highpass: output difference between current and filtered
        float coeff = -strikeNoise.filterCoeff;
        float lowpassed = coeff * strikeNoise.filterState + (1.0f - coeff) * noise;
        output = (noise - lowpassed) * strikeNoise.amplitude * 1.5f;  // Boost for presence
        strikeNoise.filterState = lowpassed;
    }

    return output;
}

float BellVoice::processPitchEnvelope()
{
    if (currentPitchEnvelope <= 0.0f || pitchEnvelopePhase <= 0.0f)
        return 0.0f;

    // Pitch drop amount (up to 50 cents initial deviation)
    float maxPitchDrop = currentPitchEnvelope * 0.5f;  // 50 cents max
    float currentPitchDrop = maxPitchDrop * pitchEnvelopePhase;

    // Apply decay to envelope phase
    pitchEnvelopePhase *= pitchEnvelopeDecayRate;

    // Convert cents to frequency ratio
    return std::pow(2.0f, -currentPitchDrop / 1200.0f) - 1.0f;
}

float BellVoice::processPartial(ModalPartial& partial)
{
    if (!partial.active)
        return 0.0f;

    // Generate sine wave
    float output = std::sin(partial.phase * juce::MathConstants<float>::twoPi) * partial.amplitude;

    // Advance phase
    partial.phase += partial.phaseIncrement;
    if (partial.phase >= 1.0f)
        partial.phase -= 1.0f;

    // Apply decay
    partial.amplitude *= partial.decayRate;

    // Deactivate if amplitude too low
    if (partial.amplitude < 0.001f)
        partial.active = false;

    return output;
}

// ============================================================================
// Multi-Stage Envelope Implementation (Research-Based)
// Based on: CCRMA modal synthesis, Chaigne frequency-dependent damping,
// Pigments "Brilliance" concept, industry-standard bell decay phases
// ============================================================================

void BellVoice::calculateMultiStageCoefficients(float fundamental)
{
    // Frequency-dependent damping: R_k = b_1 + b_3 * f_k^2
    // b_1 = frequency-independent damping
    // b_3 = frequency-dependent coefficient (higher = more high-freq damping)

    float b1 = 0.5f;  // Base damping (s^-1)

    // Brilliance controls b_3: 0% = high-freq decays fast (warm), 100% = sustains (bright)
    // Scale b_3 from 2e-8 (warm) to near-zero (bright)
    float b3 = (100.0f - currentBrilliance) / 100.0f * 2e-8f;

    // Convert time parameters from ms to seconds
    float strikeTimeSec = currentStrikeTime / 1000.0f;
    float bodyTimeSec = currentBodyTime / 1000.0f;

    // Hum extension factor: 0% = 1x, 100% = 3x body time
    float humExtension = 1.0f + (currentHumSustain / 100.0f) * 2.0f;

    // Material affects all decay times
    float materialMult = calculateMaterialDecayMultiplier(currentMaterial);

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        // Calculate partial frequency
        float freq = calculatePartialFrequency(p, fundamental, currentInharmonicity);

        // Frequency-dependent loss factor (academic formula)
        float R_k = b1 + b3 * freq * freq;
        float baseDecayTime = 1.0f / R_k;

        // === STRIKE PHASE coefficients ===
        // High partials (5+) decay fast, low partials (0-1) hold, mid (2-4) moderate
        float strikeMultiplier;
        if (p < 2)
            strikeMultiplier = 2.0f;   // Hum partials: very slow during strike
        else if (p < 5)
            strikeMultiplier = 1.0f;   // Body partials: normal
        else
            strikeMultiplier = 0.3f;   // High partials: fast decay

        float strikeDecayTime = strikeTimeSec * strikeMultiplier * materialMult;
        strikeDecayCoeffs[p] = std::exp(-1.0f / (strikeDecayTime * static_cast<float>(currentSampleRate)));

        // === BODY PHASE coefficients ===
        // Use Risset-style duration ratios already in DECAY_MULTIPLIERS
        float bodyDecayTime = bodyTimeSec * DECAY_MULTIPLIERS[p] * materialMult;
        bodyDecayCoeffs[p] = std::exp(-1.0f / (bodyDecayTime * static_cast<float>(currentSampleRate)));

        // === HUM PHASE coefficients ===
        // Extended low partials, damping affects here only (user requirement)
        float humDecayTime = baseDecayTime * humExtension * materialMult;
        if (p < 2)
        {
            // Apply damping only to hum partials
            float dampingFactor = 1.0f - (currentDamping * 0.8f);
            humDecayTime *= (1.0f + dampingFactor * 2.0f);  // Damping extends hum
        }
        humDecayCoeffs[p] = std::exp(-1.0f / (humDecayTime * static_cast<float>(currentSampleRate)));
    }
}

void BellVoice::applyMultiStageDecay(ModalPartial& partial, int partialIndex)
{
    // Calculate current time in seconds
    float timeSeconds = static_cast<float>(samplesSinceNoteOn) / static_cast<float>(currentSampleRate);

    // Phase boundaries (in seconds)
    float strikeEndTime = currentStrikeTime / 1000.0f;
    float bodyEndTime = strikeEndTime + (currentBodyTime / 1000.0f);

    // State machine: determine which phase we're in and apply appropriate coefficient
    if (timeSeconds < strikeEndTime)
    {
        // STRIKE PHASE: Bright transient, high partials decay fast
        partial.amplitude *= strikeDecayCoeffs[partialIndex];
    }
    else if (timeSeconds < bodyEndTime)
    {
        // BODY PHASE: Main tonal decay, frequency-dependent
        partial.amplitude *= bodyDecayCoeffs[partialIndex];
    }
    else
    {
        // HUM PHASE: Only low partials remain, extended sustain
        partial.amplitude *= humDecayCoeffs[partialIndex];
    }
}
