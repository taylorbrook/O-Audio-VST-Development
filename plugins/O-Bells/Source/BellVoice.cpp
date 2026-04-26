/*
  ==============================================================================

    BellVoice.cpp
    O-Bells - Physical Modeling Bell Synthesizer
    Modal synthesis voice implementation

  ==============================================================================
*/

#include "BellVoice.h"
#include "TuningEngine.h"
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

    // Prepare resonator filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    for (int i = 0; i < StrikeExciter::NUM_RESONATORS; ++i)
    {
        strikeNoise.resonators[i].prepare(spec);
        strikeNoise.resonators[i].reset();
    }
}

void BellVoice::updateParameters(float inharmonicity, float damping, float overtoneBrightness, float acousticBrightness,
                                 float airAbsorption, float airAbsorptionTime,
                                 float strikePosition, float malletHardness, float material,
                                 float bloomSpeed, float bloomAmount,
                                 bool bloomFineEnabled,
                                 float bloomSpeedLow, float bloomSpeedMid, float bloomSpeedHigh,
                                 float bloomAmountLow, float bloomAmountMid, float bloomAmountHigh,
                                 float shimmer,
                                 int unisonCount, float unisonDetune,
                                 float octaveBlendSub, float octaveBlendOct, float stereoSpread,
                                 float partialTuning, float pitchEnvelope, float pitchEnvTime,
                                 int velocityCurve, float nonlinearEffects,
                                 int strikeNoiseChar, float attackLevel, float outputGain,
                                 float strikeTime, float brilliance, float bodyTime, float humSustain,
                                 float humanize)
{
    currentInharmonicity = inharmonicity;
    currentDamping = damping;
    currentOvertoneBrightness = overtoneBrightness;
    currentAcousticBrightness = acousticBrightness;
    currentAirAbsorption = airAbsorption;
    currentAirAbsorptionTime = airAbsorptionTime;
    currentStrikePosition = strikePosition;
    currentMalletHardness = malletHardness;
    currentMaterial = material;
    currentBloomSpeed = bloomSpeed;    // v1.4.0: Split bloom
    currentBloomAmount = bloomAmount;
    // v1.5.0: Bloom fine controls
    currentBloomFineEnabled = bloomFineEnabled;
    currentBloomSpeedLow = bloomSpeedLow;
    currentBloomSpeedMid = bloomSpeedMid;
    currentBloomSpeedHigh = bloomSpeedHigh;
    currentBloomAmountLow = bloomAmountLow;
    currentBloomAmountMid = bloomAmountMid;
    currentBloomAmountHigh = bloomAmountHigh;
    currentShimmer = shimmer;
    currentUnisonCount = juce::jlimit(1, MAX_UNISON, unisonCount);
    currentUnisonDetune = unisonDetune;
    currentOctaveBlendSub = octaveBlendSub;
    currentOctaveBlendOct = octaveBlendOct;
    currentStereoSpread = stereoSpread;
    currentPartialTuning = partialTuning;
    currentPitchEnvelope = pitchEnvelope;
    currentPitchEnvTime = pitchEnvTime;
    currentVelocityCurve = velocityCurve;
    currentNonlinearEffects = nonlinearEffects;
    currentStrikeNoiseChar = strikeNoiseChar;
    currentAttackLevel = attackLevel;
    currentOutputGain = outputGain;
    // Multi-stage envelope params (always active in v1.2.0)
    currentStrikeTime = strikeTime;
    currentBrilliance = brilliance;
    currentBodyTime = bodyTime;
    currentHumSustain = humSustain;
    // Humanize (v2.4.0)
    currentHumanize = humanize;
}

void BellVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentMidiNote = midiNoteNumber;
    currentVelocity = applyVelocityCurve(velocity, currentVelocityCurve);

    // v3.2.0: Expanded velocity dynamic range (~18dB wider at low velocities)
    // Squaring the velocity after curve mapping gives:
    //   v=1.0 → 1.0 (no change), v=0.5 → 0.25 (-12dB more), v=0.3 → 0.09 (-21dB more)
    currentVelocity = std::pow(currentVelocity, 2.0f);

    noteActive = true;
    tailOff = false;
    releaseEnvelope = 1.0f;
    releaseRate = 1.0f;
    samplesSinceNoteOn = 0;  // Reset sample counter for multi-stage envelope
    decayProgress = 0.0f;    // Reset decay progress for shimmer
    silentSampleCount = 0;   // v3.1.2: Reset silence gating counter

    // ========== v2.4.0: Calculate per-note humanization variations ==========
    // Each note gets random variations scaled by the humanize parameter
    // Variations are multiplicative factors around 1.0
    if (currentHumanize > 0.0f)
    {
        // Strike position: ±5% variation (subtle position change)
        noteVariationStrikePos = 1.0f + gaussianApprox() * 0.05f * currentHumanize;
        noteVariationStrikePos = juce::jlimit(0.8f, 1.2f, noteVariationStrikePos);

        // Mallet hardness: ±10% variation (strike force varies)
        noteVariationMalletHardness = 1.0f + gaussianApprox() * 0.10f * currentHumanize;
        noteVariationMalletHardness = juce::jlimit(0.7f, 1.3f, noteVariationMalletHardness);

        // Decay time: ±15% variation (each ring is slightly different)
        noteVariationDecay = 1.0f + gaussianApprox() * 0.15f * currentHumanize;
        noteVariationDecay = juce::jlimit(0.6f, 1.4f, noteVariationDecay);

        // Attack time: ±20% variation (soft mallet bounces vary)
        noteVariationAttack = 1.0f + gaussianApprox() * 0.20f * currentHumanize;
        noteVariationAttack = juce::jlimit(0.5f, 1.5f, noteVariationAttack);

        // Inharmonicity: ±3% variation (bell shape micro-variations)
        noteVariationInharmonicity = 1.0f + gaussianApprox() * 0.03f * currentHumanize;
        noteVariationInharmonicity = juce::jlimit(0.9f, 1.1f, noteVariationInharmonicity);
    }
    else
    {
        // No humanization - deterministic behavior
        noteVariationStrikePos = 1.0f;
        noteVariationMalletHardness = 1.0f;
        noteVariationDecay = 1.0f;
        noteVariationAttack = 1.0f;
        noteVariationInharmonicity = 1.0f;
    }

    // Reset air absorption filter state (v2.1.0)
    airLPFilterStateL1 = 0.0f;
    airLPFilterStateL2 = 0.0f;
    airLPFilterStateR1 = 0.0f;
    airLPFilterStateR2 = 0.0f;
    airAbsorptionElapsedSamples = 0;  // v2.2.0: reset independent time

    // Calculate fundamental frequency (v3.0.0: use TuningEngine if available)
    float fundamental = tuningEngine
        ? static_cast<float>(tuningEngine->getFrequency(midiNoteNumber))
        : static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));

    // VST3 Note Expression tuning delta (Dorico microtonal).
    // Compose multiplicatively after TuningEngine, before any DSP coefficient setup.
    // Helper uses exchange(0.0) internally so retriggered notes don't inherit stale offsets.
    if (pendingTuningSource != nullptr)
    {
        fundamental = static_cast<float>(Ouaricon::NoteExpression::applyPendingTuning(
            *pendingTuningSource, midiNoteNumber, static_cast<double>(fundamental)));
    }

    // Pre-calculate multi-stage coefficients (always active in v1.2.0)
    calculateMultiStageCoefficients(fundamental);

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

    // Initialize mallet attack ramp (v1.2.0)
    initializeMalletAttack();

    // Initialize stereo movement (v1.2.0)
    initializeStereoMovement();

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

            // Initialize bloom for this partial
            initializeBloom(fundamentalVoices[i].partials[p], p);

            // Initialize shimmer for this partial
            initializeShimmer(fundamentalVoices[i].partials[p], p);
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

                // Per-note pitch randomization (v1.3.0)
                float pitchOffset = gaussianApprox() * 10.0f;
                subOctaveVoices[i].partials[p].frequency *= std::pow(2.0f, pitchOffset / 1200.0f);

                subOctaveVoices[i].partials[p].phaseIncrement =
                    subOctaveVoices[i].partials[p].frequency / static_cast<float>(currentSampleRate);

                // Per-note amplitude randomization (v1.3.0)
                float ampVariation = 1.0f + gaussianApprox() * 0.25f;
                ampVariation = juce::jlimit(0.5f, 1.5f, ampVariation);
                subOctaveVoices[i].partials[p].amplitude *= ampVariation;

                // Initialize bloom for sub-octave partial
                initializeBloom(subOctaveVoices[i].partials[p], p);

                // Initialize shimmer for sub-octave partial
                initializeShimmer(subOctaveVoices[i].partials[p], p);
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

                // Per-note pitch randomization (v1.3.0)
                float pitchOffset = gaussianApprox() * 10.0f;
                upperOctaveVoices[i].partials[p].frequency *= std::pow(2.0f, pitchOffset / 1200.0f);

                upperOctaveVoices[i].partials[p].phaseIncrement =
                    upperOctaveVoices[i].partials[p].frequency / static_cast<float>(currentSampleRate);

                // Per-note amplitude randomization (v1.3.0)
                float ampVariation = 1.0f + gaussianApprox() * 0.25f;
                ampVariation = juce::jlimit(0.5f, 1.5f, ampVariation);
                upperOctaveVoices[i].partials[p].amplitude *= ampVariation;

                // Initialize bloom for upper-octave partial
                initializeBloom(upperOctaveVoices[i].partials[p], p);

                // Initialize shimmer for upper-octave partial
                initializeShimmer(upperOctaveVoices[i].partials[p], p);
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

    // Initialize resonant filter bank (v1.3.0)
    // Q based on strike character: Click=high Q, Thud=low Q, Ping=medium Q
    float resonatorQ;
    switch (currentStrikeNoiseChar)
    {
        case 0:  // Click - high Q, sharp transient
            resonatorQ = 10.0f;
            break;
        case 1:  // Thud - low Q, soft impact
            resonatorQ = 2.0f;
            break;
        case 2:  // Ping - medium Q, metallic
        default:
            resonatorQ = 5.0f;
            break;
    }

    // Configure resonators tuned to first 4 partials
    for (int i = 0; i < StrikeExciter::NUM_RESONATORS; ++i)
    {
        float partialFreq = calculatePartialFrequency(i, fundamental, currentInharmonicity);
        strikeNoise.resonators[i].reset();
        strikeNoise.resonators[i].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        strikeNoise.resonators[i].setCutoffFrequency(partialFreq);
        strikeNoise.resonators[i].setResonance(resonatorQ);

        // Gains based on mallet hardness (harder = more high partials)
        float partialGain = 1.0f - (i * 0.15f);  // Rolloff
        strikeNoise.resonatorGains[i] = partialGain * (1.0f + currentMalletHardness * (i * 0.3f));
    }

    // Impulse: 2 samples
    strikeNoise.impulseSamplesRemaining = 2;
    strikeNoise.impulseAmplitude = 1.0f;
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

    // v3.0.0: Clear pitch bend for this note
    if (tuningEngine)
        tuningEngine->clearPitchBend(currentMidiNote);
}

// v3.0.0: Pitch wheel support for tuning engine
void BellVoice::pitchWheelMoved(int newPitchWheelValue)
{
    if (tuningEngine && noteActive)
    {
        // Convert 14-bit MIDI pitch wheel (0-16383, center 8192) to -1.0..+1.0
        float bendNormalized = (static_cast<float>(newPitchWheelValue) - 8192.0f) / 8192.0f;
        tuningEngine->setPitchBend(currentMidiNote, bendNormalized);
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

                    // Apply shimmer (frequency modulation)
                    modulatedPhaseInc = applyShimmer(partial, modulatedPhaseInc);

                    partial.phase += modulatedPhaseInc;
                    if (partial.phase >= 1.0f)
                        partial.phase -= 1.0f;

                    float partialSample = std::sin(partial.phase * juce::MathConstants<float>::twoPi);
                    voiceSample += partialSample * partial.amplitude;

                    // Apply bloom (spectral swelling before decay)
                    applyBloom(partial);

                    // Apply multi-stage decay (always active in v1.2.0)
                    applyMultiStageDecay(partial, p);

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
                        modulatedPhaseInc = applyShimmer(partial, modulatedPhaseInc);
                        partial.phase += modulatedPhaseInc;
                        if (partial.phase >= 1.0f)
                            partial.phase -= 1.0f;

                        float partialSample = std::sin(partial.phase * juce::MathConstants<float>::twoPi);
                        voiceSample += partialSample * partial.amplitude;

                        // Apply multi-stage decay (always active in v1.2.0)
                        applyMultiStageDecay(partial, p);

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
                        modulatedPhaseInc = applyShimmer(partial, modulatedPhaseInc);
                        partial.phase += modulatedPhaseInc;
                        if (partial.phase >= 1.0f)
                            partial.phase -= 1.0f;

                        float partialSample = std::sin(partial.phase * juce::MathConstants<float>::twoPi);
                        voiceSample += partialSample * partial.amplitude;

                        // Apply multi-stage decay (always active in v1.2.0)
                        applyMultiStageDecay(partial, p);

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
        ++samplesSinceNoteOn;

        // Update decay progress for shimmer (0.0 = start, 1.0 = fully decayed)
        // Approximate using time-based calculation
        float totalDecayTime = (currentStrikeTime + currentBodyTime) / 1000.0f;
        float currentTime = static_cast<float>(samplesSinceNoteOn) / static_cast<float>(currentSampleRate);
        decayProgress = juce::jlimit(0.0f, 1.0f, currentTime / totalDecayTime);

        // Add strike noise transient
        leftOutput += noiseSignal * 0.3f;
        rightOutput += noiseSignal * 0.3f;

        // Apply mallet attack ramp (v1.2.0)
        if (attackRampPosition < attackRampSamples)
        {
            // Cosine curve for smooth attack
            float rampProgress = static_cast<float>(attackRampPosition) / static_cast<float>(attackRampSamples);
            float attackGain = (1.0f - std::cos(rampProgress * juce::MathConstants<float>::pi)) * 0.5f;

            leftOutput *= attackGain;
            rightOutput *= attackGain;

            ++attackRampPosition;
        }

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
        float partialNorm = 0.2f;  // v3.2.0: -6dB from 0.4f — quieter synthesis baseline
        float unisonNorm = 1.0f / std::sqrt(static_cast<float>(currentUnisonCount));
        float layerNorm = 1.0f / (1.0f + currentOctaveBlendSub + currentOctaveBlendOct);
        float totalNorm = partialNorm * unisonNorm * layerNorm;

        leftOutput *= totalNorm;
        rightOutput *= totalNorm;

        // Apply air absorption lowpass filter (v2.1.0, enhanced v2.2.0)
        // Two-pole filter (12dB/octave) simulating progressive HF loss
        // v2.2.0: Independent time control with exponential curve (fast initial darkening)
        if (currentAirAbsorption > 0.0f)
        {
            // Calculate time-based progress (v2.2.0: independent of decay envelope)
            float totalSamples = currentAirAbsorptionTime * static_cast<float>(currentSampleRate);
            float timeProgress = juce::jlimit(0.0f, 1.0f,
                static_cast<float>(airAbsorptionElapsedSamples) / totalSamples);
            airAbsorptionElapsedSamples++;  // Increment time tracker

            // Exponential curve: fast initial darkening, slows toward end
            // y = 1 - (1-x)^2  (quadratic ease-out)
            float t = 1.0f - timeProgress;
            float shapedProgress = 1.0f - (t * t);

            // Calculate cutoff frequency
            // At start: 18kHz (transparent), at end: scales down with air absorption
            // airAbsorption=1: 18kHz -> 800Hz over time
            float minCutoff = 18000.0f - (17200.0f * currentAirAbsorption);  // 800Hz at max absorption
            float cutoff = 18000.0f - (18000.0f - minCutoff) * shapedProgress;
            cutoff = juce::jlimit(400.0f, 18000.0f, cutoff);  // Safety clamp

            // One-pole coefficient (used twice for two-pole / 12dB/octave)
            float coeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / static_cast<float>(currentSampleRate));

            // Apply cascaded two-pole filter to both channels (12dB/octave)
            // First pole
            airLPFilterStateL1 += coeff * (leftOutput - airLPFilterStateL1);
            airLPFilterStateR1 += coeff * (rightOutput - airLPFilterStateR1);
            // Second pole
            airLPFilterStateL2 += coeff * (airLPFilterStateL1 - airLPFilterStateL2);
            airLPFilterStateR2 += coeff * (airLPFilterStateR1 - airLPFilterStateR2);

            leftOutput = airLPFilterStateL2;
            rightOutput = airLPFilterStateR2;
        }

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

        // v3.1.2: Voice-level silence gating - kill voices whose output is negligible
        // Bypassed in High Fidelity mode for maximum sustain fidelity
        if (!currentHighFidelity)
        {
            float samplePeak = std::max(std::abs(leftOutput), std::abs(rightOutput));
            if (samplePeak < VOICE_SILENCE_THRESHOLD)
            {
                ++silentSampleCount;
                if (silentSampleCount >= SILENCE_SAMPLES_TO_KILL)
                {
                    clearCurrentNote();
                    noteActive = false;
                    break;
                }
            }
            else
            {
                silentSampleCount = 0;
            }
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
    // Strike position models where the bell is struck:
    // Center (0): warm, fundamental-heavy - like striking the bell's center
    // Edge (1): bright, partial-rich - like striking near the rim

    float normalizedPartial = partialIndex / 7.0f;  // 0.0 to 1.0 for 8 partials

    // Spectral tilt: center favors low partials, edge favors high partials
    // More extreme range for obvious tonal difference
    float lowPartialGain = 1.0f - position * 0.85f;   // 1.0 → 0.15 as position 0 → 1
    float highPartialGain = 0.15f + position * 0.85f; // 0.15 → 1.0 as position 0 → 1

    // Interpolate based on which partial we're calculating
    return lowPartialGain + normalizedPartial * (highPartialGain - lowPartialGain);
}

float BellVoice::calculatePartialAmplitude(int partialIndex, float overtoneBrightness)
{
    // Base amplitude decreases with partial number
    float baseAmp = 1.0f / (partialIndex + 1.0f);

    // Overtone brightness scales upper partials with expanded range [0.1, 2.0]
    // v2.0.0: Renamed from "brightness" to clarify it controls initial partial balance
    // overtoneBrightness=0: highest partial scaled to 0.1x (dark)
    // overtoneBrightness=0.5: neutral (1.0x scaling)
    // overtoneBrightness=1: highest partial scaled to 2.0x (bright)
    float partialRatio = partialIndex / static_cast<float>(NUM_PARTIALS);
    float brightnessScale = 1.0f + partialRatio * (1.9f * overtoneBrightness - 0.9f);

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

BellVoice::MaterialProperties BellVoice::getMaterialProperties(float material)
{
    // v1.3.0: Discrete lookup (Choice parameter normalizes to 0.0, 0.25, 0.5, 0.75, 1.0)
    int index = static_cast<int>(std::round(material * 4.0f));
    index = juce::jlimit(0, 4, index);

    switch (index)
    {
        case 0: return MATERIAL_BRONZE;
        case 1: return MATERIAL_BRASS;
        case 2: return MATERIAL_STEEL;
        case 3: return MATERIAL_ALUMINUM;
        case 4: return MATERIAL_CAST_IRON;
        default: return MATERIAL_BRONZE;
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
    // Get material properties
    auto materialProps = getMaterialProperties(currentMaterial);

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        auto& partial = fundamentalVoices[0].partials[p];

        // Apply material inharmonicity offset to base inharmonicity
        // v2.4.0: Apply per-note inharmonicity variation for humanization
        float effectiveInharmonicity = currentInharmonicity * noteVariationInharmonicity + materialProps.inharmonicity;
        effectiveInharmonicity = juce::jlimit(0.0f, 1.0f, effectiveInharmonicity);

        partial.frequency = calculatePartialFrequency(p, fundamental, effectiveInharmonicity);

        // Per-note pitch randomization (v1.3.0 - ±10 cents std dev)
        float pitchOffset = gaussianApprox() * 10.0f;  // ±10 cents
        partial.frequency *= std::pow(2.0f, pitchOffset / 1200.0f);

        partial.phaseIncrement = partial.frequency / static_cast<float>(currentSampleRate);
        partial.phase = 0.0f;

        // Calculate initial amplitude with material brightness offset
        float effectiveBrightness = currentOvertoneBrightness + materialProps.brightnessOffset;
        effectiveBrightness = juce::jlimit(0.0f, 1.0f, effectiveBrightness);

        // v2.4.0: Apply per-note strike position and mallet hardness variations
        float effectiveStrikePosition = juce::jlimit(0.0f, 1.0f, currentStrikePosition * noteVariationStrikePos);
        float effectiveMalletHardness = juce::jlimit(0.0f, 1.0f, currentMalletHardness * noteVariationMalletHardness);

        float baseAmplitude = calculatePartialAmplitude(p, effectiveBrightness);
        float strikeGain = calculateStrikePositionGain(p, effectiveStrikePosition);
        float malletGain = 1.0f + effectiveMalletHardness * (p / static_cast<float>(NUM_PARTIALS));

        partial.amplitude = baseAmplitude * strikeGain * malletGain * velocity;

        // Per-note amplitude randomization (v1.3.0 - ±25% std dev, clamped 50%-150%)
        float ampVariation = 1.0f + gaussianApprox() * 0.25f;
        ampVariation = juce::jlimit(0.5f, 1.5f, ampVariation);
        partial.amplitude *= ampVariation;
        partial.targetAmplitude = partial.amplitude;

        // Calculate decay rate (exponential decay) with material multiplier
        float baseDecayTime = juce::jmap(currentDamping, 0.5f, 5.0f);  // 0.5s to 5s

        // v2.0.0: Acoustic brightness - higher partials decay faster when acousticBrightness is low
        // This simulates air absorption and natural bell physics where HF content fades first
        // acousticBrightness=0: highest partial decays 4x faster (partialRatio * 0.75 = 0.75 reduction)
        // acousticBrightness=1: no additional decay (normal behavior)
        float partialRatio = static_cast<float>(p) / NUM_PARTIALS;
        float acousticDecayMultiplier = 1.0f - (1.0f - currentAcousticBrightness) * partialRatio * 0.75f;

        // v2.4.0: Apply per-note decay variation for humanization
        float partialDecayTime = baseDecayTime * DECAY_MULTIPLIERS[p] * materialProps.decayMultiplier * acousticDecayMultiplier * noteVariationDecay;
        partial.decayRate = std::exp(-1.0f / (partialDecayTime * static_cast<float>(currentSampleRate)));

        partial.active = partial.amplitude > 0.001f;
    }
}

float BellVoice::generateStrikeNoise()
{
    // v1.3.0: Impulse-driven resonant filter bank
    float input = 0.0f;

    // Generate impulse for first 2 samples
    if (strikeNoise.impulseSamplesRemaining > 0)
    {
        input = strikeNoise.impulseAmplitude;
        --strikeNoise.impulseSamplesRemaining;
    }

    // Process impulse through resonator bank in parallel
    float output = 0.0f;
    for (int i = 0; i < StrikeExciter::NUM_RESONATORS; ++i)
    {
        float resonatorOutput = strikeNoise.resonators[i].processSample(0, input);
        output += resonatorOutput * strikeNoise.resonatorGains[i];
    }

    // Apply overall decay envelope
    output *= strikeNoise.amplitude;

    // Scale by attack level (50% = original level, so multiply by 2.0)
    output *= currentAttackLevel * 2.0f;

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
    auto materialProps = getMaterialProperties(currentMaterial);
    float materialMult = materialProps.decayMultiplier;

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
        // v2.0.0: Apply acoustic brightness - higher partials decay faster when acousticBrightness is low
        float partialRatio = static_cast<float>(p) / NUM_PARTIALS;
        float acousticDecayMult = 1.0f - (1.0f - currentAcousticBrightness) * partialRatio * 0.75f;
        float bodyDecayTime = bodyTimeSec * DECAY_MULTIPLIERS[p] * materialMult * acousticDecayMult;
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
    // Skip decay during bloom phase (v1.3.0 bug fix)
    if (partial.bloomPhase < 1.0f)
        return;

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

// ============================================================================
// Bloom Implementation (v1.2.0 Realism Overhaul)
// Spectral swelling: partials swell from initial to peak amplitude before decay
// ============================================================================

void BellVoice::initializeBloom(ModalPartial& partial, int partialIndex)
{
    // v1.5.0: Per-band bloom control when fine controls enabled
    float speedValue, amountValue;

    if (currentBloomFineEnabled)
    {
        // Use per-band values (Override Mode)
        if (partialIndex < 2)
        {
            speedValue = currentBloomSpeedLow;
            amountValue = currentBloomAmountLow;
        }
        else if (partialIndex < 5)
        {
            speedValue = currentBloomSpeedMid;
            amountValue = currentBloomAmountMid;
        }
        else
        {
            speedValue = currentBloomSpeedHigh;
            amountValue = currentBloomAmountHigh;
        }
    }
    else
    {
        // Use global values (original behavior)
        speedValue = currentBloomSpeed;
        amountValue = currentBloomAmount;
    }

    // v1.4.0: Bloom Amount controls whether bloom is active, Speed controls duration
    if (amountValue <= 0.0f)
    {
        // No bloom - skip initialization
        partial.bloomPhase = 1.0f;  // Completed immediately
        return;
    }

    // Spectral bloom: staggered timing based on partial index
    // v1.4.0: Speed controls duration independently from amount
    float bloomDurationMs;
    float initialFraction;

    if (partialIndex < 2)
    {
        // Low partials (0-1): Slower bloom for foundation tones
        bloomDurationMs = juce::jmap(speedValue, 15.0f, 250.0f);  // Speed: 15-250ms
        initialFraction = 1.0f - (amountValue * 0.4f);  // Amount: 60-100% initial
    }
    else if (partialIndex < 5)
    {
        // Mid partials (2-4): Medium bloom for body tones
        bloomDurationMs = juce::jmap(speedValue, 25.0f, 400.0f);  // Speed: 25-400ms
        // Amount controls initial fraction: 100% (amount=0) to 40% (amount=1)
        initialFraction = 1.0f - (amountValue * 0.6f);
    }
    else
    {
        // High partials (5-7): Longest bloom for shimmer/brilliance
        bloomDurationMs = juce::jmap(speedValue, 50.0f, 800.0f);  // Speed: 50-800ms
        // Amount controls initial fraction: 100% (amount=0) to 10% (amount=1)
        initialFraction = 1.0f - (amountValue * 0.9f);
    }

    float bloomDurationSamples = (bloomDurationMs / 1000.0f) * static_cast<float>(currentSampleRate);

    partial.bloomPhase = 0.0f;
    partial.bloomRate = 1.0f / bloomDurationSamples;

    partial.initialAmplitude = partial.amplitude * initialFraction;
    partial.peakAmplitude = partial.amplitude;

    // Set amplitude to initial value
    partial.amplitude = partial.initialAmplitude;
}

void BellVoice::applyBloom(ModalPartial& partial)
{
    if (partial.bloomPhase >= 1.0f)
        return;  // Bloom complete

    // Cosine interpolation for smooth swelling
    float t = partial.bloomPhase;
    float cosineT = (1.0f - std::cos(t * juce::MathConstants<float>::pi)) * 0.5f;

    // Interpolate from initial to peak amplitude
    partial.amplitude = partial.initialAmplitude + (partial.peakAmplitude - partial.initialAmplitude) * cosineT;

    // Advance bloom phase
    partial.bloomPhase += partial.bloomRate;

    // Clamp to prevent overshoot
    if (partial.bloomPhase >= 1.0f)
    {
        partial.bloomPhase = 1.0f;
        partial.amplitude = partial.peakAmplitude;
    }
}

// ============================================================================
// Shimmer Implementation (v1.2.0 Realism Overhaul)
// Frequency modulation that increases during decay for metallic shimmering
// ============================================================================

void BellVoice::initializeShimmer(ModalPartial& partial, int partialIndex)
{
    if (currentShimmer <= 0.0f)
    {
        partial.shimmerLFORate = 0.0f;
        partial.shimmerDepth = 0.0f;
        return;
    }

    // LFO frequency: use prime ratios to prevent phase locking between partials
    // Range: 0.1 Hz to 8.0 Hz (v1.3.0 - wider range for organic shimmer)
    static const float primeLFORatios[NUM_PARTIALS] = {1.0f, 1.31f, 1.73f, 2.17f, 2.71f, 3.31f, 4.13f, 5.03f};
    float baseLFOFreq = juce::jmap(currentShimmer, 0.1f, 8.0f);
    partial.shimmerLFORate = baseLFOFreq * primeLFORatios[partialIndex] / static_cast<float>(currentSampleRate);

    // Modulation depth: subtle (1-5 cents depending on shimmer amount)
    partial.shimmerDepth = juce::jmap(currentShimmer, 1.0f, 5.0f);

    // Random initial phase to decorrelate partials
    partial.shimmerLFOPhase = static_cast<float>(rand()) / RAND_MAX;
}

float BellVoice::applyShimmer(ModalPartial& partial, float basePhaseInc)
{
    if (currentShimmer <= 0.0f || partial.shimmerLFORate == 0.0f)
        return basePhaseInc;

    // Advance LFO phase
    partial.shimmerLFOPhase += partial.shimmerLFORate;
    if (partial.shimmerLFOPhase >= 1.0f)
        partial.shimmerLFOPhase -= 1.0f;

    // Generate LFO (sine wave)
    float lfoValue = std::sin(partial.shimmerLFOPhase * juce::MathConstants<float>::twoPi);

    // Shimmer intensity increases with decay progress (0.0 = none, 1.0 = full)
    // This creates the "metallic shimmer gets stronger as bell fades" effect
    float shimmerIntensity = decayProgress * currentShimmer;

    // Calculate frequency modulation in cents
    float modulationCents = lfoValue * partial.shimmerDepth * shimmerIntensity;

    // Convert cents to frequency ratio
    float freqRatio = std::pow(2.0f, modulationCents / 1200.0f);

    return basePhaseInc * freqRatio;
}

// ============================================================================
// Mallet Attack Enhancement (v1.2.0 Realism Overhaul)
// Temporal spreading: soft mallets have gradual attack, hard have instant
// ============================================================================

void BellVoice::initializeMalletAttack()
{
    // Soft mallets (0.0) have gradual attack (up to 50ms)
    // Hard mallets (1.0) have instant attack (0ms)
    // v2.4.0: Apply per-note mallet hardness and attack variation for humanization
    float effectiveMalletHardness = juce::jlimit(0.0f, 1.0f, currentMalletHardness * noteVariationMalletHardness);
    float attackTimeMs = juce::jmap(effectiveMalletHardness, 50.0f, 0.0f);

    // v2.4.0: Apply per-note attack variation
    attackTimeMs *= noteVariationAttack;

    if (attackTimeMs > 0.0f)
    {
        attackRampSamples = static_cast<int>((attackTimeMs / 1000.0f) * currentSampleRate);
        attackRampPosition = 0;
    }
    else
    {
        attackRampSamples = 0;
        attackRampPosition = 0;
    }
}

// ============================================================================
// Stereo Enhancement (v1.2.0 Realism Overhaul)
// Partial-based panning, slow LFO movement, Haas delay for width
// ============================================================================

float BellVoice::getPartialPan(int partialIndex)
{
    // Low partials (0-1): centered (mono foundation)
    // Mid partials (2-4): moderate spread
    // High partials (5-7): wide spread, alternating L/R

    if (partialIndex < 2)
        return 0.0f;  // Center

    if (partialIndex < 5)
    {
        // Moderate spread: ±0.3
        float spread = 0.3f;
        return (partialIndex % 2 == 0) ? -spread : spread;
    }

    // Wide spread: ±0.7, alternating
    float spread = 0.7f;
    return (partialIndex % 2 == 0) ? -spread : spread;
}

void BellVoice::initializeStereoMovement()
{
    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        // Base pan position (static)
        partialStereo[p].basePan = getPartialPan(p);

        // Slow LFO for subtle movement (0.1 Hz to 0.5 Hz)
        // Use prime ratios to decorrelate
        static const float primePanRatios[NUM_PARTIALS] = {1.0f, 1.1f, 1.2f, 1.3f, 1.5f, 1.7f, 1.9f, 2.1f};
        float baseLFOFreq = 0.3f;  // 0.3 Hz base
        partialStereo[p].panLFORate = baseLFOFreq * primePanRatios[p] / static_cast<float>(currentSampleRate);

        // Random initial phase
        partialStereo[p].panLFOPhase = static_cast<float>(rand()) / RAND_MAX;
    }
}

float BellVoice::getModulatedPan(int partialIndex)
{
    auto& stereo = partialStereo[partialIndex];

    // Advance LFO
    stereo.panLFOPhase += stereo.panLFORate;
    if (stereo.panLFOPhase >= 1.0f)
        stereo.panLFOPhase -= 1.0f;

    // Generate LFO (sine)
    float lfoValue = std::sin(stereo.panLFOPhase * juce::MathConstants<float>::twoPi);

    // Subtle modulation depth (±0.1)
    float modulation = lfoValue * 0.1f;

    // Combine base pan + modulation
    float finalPan = stereo.basePan + modulation;

    return juce::jlimit(-1.0f, 1.0f, finalPan);
}
