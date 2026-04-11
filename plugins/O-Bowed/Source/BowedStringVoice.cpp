/*
  ==============================================================================

    BowedStringVoice.cpp
    O-Bowed - Physical Modeling Bowed String Voice (MPE)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BowedStringVoice.h"
#include "TuningEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>

BowedStringVoice::BowedStringVoice (juce::AudioProcessorValueTreeState* apvts)
    : parameters (apvts)
{
}

//==============================================================================
// MPE Voice Callbacks
//==============================================================================

void BowedStringVoice::noteStarted()
{
    auto note = getCurrentlyPlayingNote();
    int midiNote = note.initialNote;
    float velocity = note.noteOnVelocity.asUnsignedFloat();

    // Get base frequency from tuning engine (Scala/MTS-ESP/12-TET)
    currentFrequency = getBaseFrequencyFromTuning (midiNote);

    // Apply initial MPE pitch bend on top of tuning engine frequency
    float bendSemitones = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bendSemitones) > 0.001f)
        currentFrequency *= std::pow (2.0f, bendSemitones / 12.0f);

    waveguideString.trigger (currentFrequency);
    bowModel.startBow (velocity);
    oversampling.reset();
    bowNoiseGen.reset();
}

void BowedStringVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        // Bow lifts -- release ramp begins, voice stays active until energy decays
        bowModel.stopBow();
    }
    else
    {
        // Hard stop -- immediate silence
        clearCurrentNote();
        waveguideString.reset();
        bowModel.reset();
    }
}

void BowedStringVoice::notePitchbendChanged()
{
    auto note = getCurrentlyPlayingNote();
    int midiNote = note.initialNote;

    // Recompute from tuning engine base + new MPE bend
    currentFrequency = getBaseFrequencyFromTuning (midiNote);
    float bendSemitones = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bendSemitones) > 0.001f)
        currentFrequency *= std::pow (2.0f, bendSemitones / 12.0f);

    waveguideString.trigger (currentFrequency);
}

void BowedStringVoice::notePressureChanged()
{
    // Pressure modulation applied in updateParametersFromAPVTS()
}

void BowedStringVoice::noteTimbreChanged()
{
    // Timbre modulation applied in updateParametersFromAPVTS()
}

void BowedStringVoice::noteKeyStateChanged()
{
    // No special handling needed for sustain/sostenuto
}

//==============================================================================
// Prepare and Render
//==============================================================================

void BowedStringVoice::prepareToPlay (double sampleRate, int maxBlockSize)
{
    // Prepare waveguide and bow at 2x oversampled rate
    waveguideString.prepare (sampleRate * 2.0, maxBlockSize * 2);
    bowModel.prepare (sampleRate * 2.0);
    qualityFriction.prepare (sampleRate * 2.0);

    // dt at oversampled rate
    dt = 1.0f / static_cast<float> (sampleRate * 2.0);

    // Initialize oversampling (mono)
    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));

    // Allocate mono voice buffer
    voiceBuffer.setSize (1, maxBlockSize);
    voiceBuffer.clear();

    // Prepare bow noise at native rate (not oversampled)
    bowNoiseGen.prepare (sampleRate, voiceIndex);
}

void BowedStringVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                         int startSample, int numSamples)
{
    updateParametersFromAPVTS();

    // Check if voice should be cleared (bow released and string decayed)
    if (! bowModel.isActive() && ! waveguideString.isActive())
    {
        clearCurrentNote();
        return;
    }

    // --- Block-based oversampling pathway ---

    // 1. Prepare mono input block of silence
    voiceBuffer.clear (0, 0, numSamples);
    juce::dsp::AudioBlock<float> inputBlock (voiceBuffer);
    auto block = inputBlock.getSubBlock (0, static_cast<size_t> (numSamples));

    // 2. Upsample (silence in -> 2x oversampled buffer)
    auto oversampledBlock = oversampling.processSamplesUp (block);
    auto numOversampledSamples = static_cast<int> (oversampledBlock.getNumSamples());
    auto* oversampledData = oversampledBlock.getChannelPointer (0);

    // 3. Per-sample loop at 2x rate (friction + waveguide inner loop)
    for (int i = 0; i < numOversampledSamples; ++i)
    {
        // Step 1: Update bow envelope
        bowModel.updateEnvelope();
        float v_bow = bowModel.getBowVelocity();
        float F_bow = bowModel.getBowForce();

        // Steps 2-4: Read junction (pop from delays, compute incoming)
        auto jState = waveguideString.readJunction (v_bow);
        float v_delta = v_bow - jState.v_string_incoming;

        // Step 5: Friction tier dispatch -> compute rho
        float rho;
        switch (currentTier)
        {
            case 1:  // Enhanced: Elasto-Plastic
                rho = enhancedFriction.computeReflectionCoefficient (v_delta, F_bow, dt);
                break;
            case 2:  // Quality: Thermal
                rho = qualityFriction.computeReflectionCoefficient (v_delta, F_bow, dt);
                break;
            default: // Core: Hyperbolic (tier 0)
                rho = frictionModel.computeReflectionCoefficient (v_delta, F_bow);
                break;
        }

        // Reversed friction: interpolate rho toward (1 - rho)
        if (reversedAmount >= 0.001f)
            rho = rho + reversedAmount * (1.0f - 2.0f * rho);

        // Clamp rho to prevent excessive velocity injection
        rho = std::min (rho, 0.85f);

        // Energy-aware excitation limiting for high-sustain modes.
        // Reduces excitation as waveguide energy builds — physically motivated:
        // bow loses grip on strongly oscillating string.
        if (cachedInfSustain > 0.05f)
        {
            float energy = waveguideString.getEnergyEstimate();
            float targetEnergy = 0.5f * (1.0f - 0.75f * cachedInfSustain);
            if (energy > targetEnergy)
                rho *= targetEnergy / energy;
        }

        // Steps 6-8: Write junction (inject velocity, push to delays, output)
        float sample = waveguideString.writeJunction (rho, v_delta, jState);

        // Sub-harmonics waveshaper (post-waveguide, pre-body)
        if (subHarmonicsAmount >= 0.001f)
            sample = subHarmonicsGen.process (sample, subHarmonicsAmount);

        oversampledData[i] = sample;
    }

    // 4. Downsample back to native rate
    oversampling.processSamplesDown (block);

    // 5. Mix voiceBuffer into outputBuffer with panning + bow noise (at native rate)
    for (int i = 0; i < numSamples; ++i)
    {
        // Normalize waveguide velocity units to audio range
        float sample = voiceBuffer.getSample (0, i) * 0.35f;

        // Add bow noise (post-body, post-downsample)
        float noise = bowNoiseGen.processSample (effectivePressure, effectiveSpeed, bowNoiseAmount);
        sample += noise;

        // Safety hard-clip to prevent runaway
        sample = juce::jlimit (-1.0f, 1.0f, sample);

        // Write to stereo output with per-voice panning
        outputBuffer.addSample (0, startSample + i, sample * panL);
        if (outputBuffer.getNumChannels() >= 2)
            outputBuffer.addSample (1, startSample + i, sample * panR);
    }
}

//==============================================================================
// Parameter Reading
//==============================================================================

void BowedStringVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr)
        return;

    // Read APVTS parameters atomically (once per block)
    float bowSpeed       = parameters->getRawParameterValue ("bowSpeed")->load();
    float bowPressure    = parameters->getRawParameterValue ("bowPressure")->load();
    float bowPos         = parameters->getRawParameterValue ("bowPosition")->load();
    float rosin          = parameters->getRawParameterValue ("rosin")->load();
    float brightness     = parameters->getRawParameterValue ("brightness")->load();
    float infSustain     = parameters->getRawParameterValue ("infiniteSustain")->load();
    float outputLevel    = parameters->getRawParameterValue ("outputLevel")->load();
    bowNoiseAmount       = parameters->getRawParameterValue ("bowNoise")->load();

    // Read friction tier parameters
    int newTier          = static_cast<int> (parameters->getRawParameterValue ("frictionTier")->load());
    reversedAmount       = parameters->getRawParameterValue ("reversedFriction")->load();
    subHarmonicsAmount   = parameters->getRawParameterValue ("subHarmonics")->load();
    float stringGauge    = parameters->getRawParameterValue ("stringGauge")->load();
    float bowHairStiff   = parameters->getRawParameterValue ("bowHairStiffness")->load();

    // Detect friction tier switch -- reset state to avoid discontinuity
    currentTier = newTier;
    if (currentTier != previousTier)
    {
        enhancedFriction.resetState();
        qualityFriction.resetState();
        previousTier = currentTier;
    }

    // Apply MPE modulation on top of knob base values
    auto note = getCurrentlyPlayingNote();
    float mpePressure = note.pressure.asUnsignedFloat();
    float mpeTimbre   = note.timbre.asSignedFloat();

    effectivePressure = bowPressure * (0.5f + mpePressure * 1.5f);
    effectivePosition = bowPos + mpeTimbre * 0.1f;
    effectivePosition = juce::jlimit (0.02f, 0.30f, effectivePosition);
    effectiveSpeed    = bowSpeed * mpeExpression;

    // Update DSP components with effective values
    bowModel.setBowSpeed (effectiveSpeed);
    bowModel.setBowPressure (effectivePressure);
    waveguideString.setBowPosition (effectivePosition);
    frictionModel.setRosin (rosin);
    waveguideString.setBrightness (brightness);
    waveguideString.setInfiniteSustain (infSustain);
    cachedInfSustain = infSustain;
    outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);

    // Update advanced friction models with matching parameters
    frictionModel.setStringImpedance (stringGauge);
    enhancedFriction.setRosin (rosin);
    enhancedFriction.setStringImpedance (stringGauge);
    enhancedFriction.setBristleStiffness (bowHairStiff);
    qualityFriction.setRosin (rosin);
    qualityFriction.setStringImpedance (stringGauge);
    qualityFriction.setBristleStiffness (bowHairStiff);
}

float BowedStringVoice::getBaseFrequencyFromTuning (int midiNote) const
{
    if (tuningEngine != nullptr)
        return static_cast<float> (tuningEngine->getFrequency (midiNote));

    return static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNote));
}
