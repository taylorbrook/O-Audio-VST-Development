/*
  ==============================================================================

    BowedContrabassVoice.cpp
    O-Contrabass - Single-String E1 MPESynthesiserVoice (Phase 2.1a)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BowedContrabassVoice.h"
#include <cmath>

BowedContrabassVoice::BowedContrabassVoice (juce::AudioProcessorValueTreeState* apvts)
    : parameters (apvts)
{
}

//==============================================================================
// MPE Voice Callbacks
//==============================================================================

void BowedContrabassVoice::noteStarted()
{
    auto note = getCurrentlyPlayingNote();
    int midiNote = note.initialNote;
    float velocity = note.noteOnVelocity.asUnsignedFloat();

    // Phase 2.1a: 12-TET fallback. Tuning engine + per-string detune lands in
    // Phase 2.6. MPE pitch bend composes multiplicatively on top.
    double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    float bendSemitones = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bendSemitones) > 0.001f)
        freq *= std::pow (2.0, bendSemitones / 12.0f);
    currentFrequency = static_cast<float> (freq);

    waveguideString.trigger (currentFrequency);
    bowModel.startBow (velocity);
    oversampling.reset();
}

void BowedContrabassVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        // Bow lifts; release ramp begins, voice stays active until energy decays.
        bowModel.stopBow();
    }
    else
    {
        // Hard stop — immediate silence.
        clearCurrentNote();
        waveguideString.reset();
        bowModel.reset();
        oversampling.reset();
    }
}

void BowedContrabassVoice::notePitchbendChanged()
{
    auto note = getCurrentlyPlayingNote();
    int midiNote = note.initialNote;

    // Recompute base frequency + apply current MPE bend.
    double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    float bendSemitones = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bendSemitones) > 0.001f)
        freq *= std::pow (2.0, bendSemitones / 12.0f);
    currentFrequency = static_cast<float> (freq);

    waveguideString.trigger (currentFrequency);
}

void BowedContrabassVoice::notePressureChanged()
{
    // Pressure modulation will be wired in Phase 2.6 (Note Expression / MPE-Z).
}

void BowedContrabassVoice::noteTimbreChanged()
{
    // Timbre modulation (CC74 / MPE-Y) will be wired in Phase 2.6.
}

void BowedContrabassVoice::noteKeyStateChanged()
{
    // No special handling needed for sustain/sostenuto in Phase 2.1a.
}

//==============================================================================
// Lifecycle
//==============================================================================

void BowedContrabassVoice::prepareToPlay (double hostSampleRate, int maxBlockSize)
{
    currentMaxBlockSize = maxBlockSize;

    // 2× oversampler — initProcessing sized to maxBlockSize (the host-rate
    // block size; oversampler reports 2× internally).
    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
    oversampling.reset();

    // Mono scratch: maxBlockSize × 2 covers the upsampled buffer footprint.
    voiceBuffer.setSize (1, maxBlockSize * 2, /*keepExistingContent*/ false,
                         /*clearExtraSpace*/ true, /*avoidReallocating*/ false);
    voiceBuffer.clear();

    // Waveguide + bow envelope run at internal (oversampled) rate.
    juce::dsp::ProcessSpec spec_at_2x {
        hostSampleRate * 2.0,
        static_cast<juce::uint32> (maxBlockSize * 2),
        1u
    };

    waveguideString.prepare (spec_at_2x.sampleRate,
                             static_cast<int> (spec_at_2x.maximumBlockSize));
    bowModel.prepare (spec_at_2x.sampleRate);

    // Phase 2.1b — bass-string friction defaults (module init = treble defaults).
    // RESEARCH §13.3-Q5 setter API; PLAN rev-4 R13. v_0 (0.05) and R_s (0.5)
    // match between treble and bass — only mu_s and mu_d differ. setRosin
    // continues to be called per-block from updateExpressionParameters; that
    // path overwrites v_0 from the APVTS ROSIN value, not mu_s/mu_d.
    frictionModel.setStaticFrictionCoefficient  (0.85f);
    frictionModel.setDynamicFrictionCoefficient (0.25f);
}

//==============================================================================
// Render
//==============================================================================

void BowedContrabassVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                             int startSample, int numSamples)
{
    // Defensive double-cover (also lives at processor processBlock entry).
    juce::ScopedNoDenormals noDenormals;

    if (numSamples <= 0)
        return;

    // 1. Read APVTS atomics into block-cached locals.
    updateParametersFromAPVTS();

    // Voice cleanup — bow released and string decayed: clear the slot.
    if (! bowModel.isActive() && ! waveguideString.isActive())
    {
        clearCurrentNote();
        return;
    }

    // 2. Clear the host-rate slice of voiceBuffer.
    voiceBuffer.clear (0, 0, numSamples);

    // 3. Wrap voiceBuffer in an AudioBlock for the host-rate slice.
    juce::dsp::AudioBlock<float> hostBlock (voiceBuffer);
    auto block = hostBlock.getSubBlock (0, static_cast<size_t> (numSamples));

    // 4. Upsample to 2× rate (silence → 2× scratch buffer).
    auto upBlock = oversampling.processSamplesUp (block);
    auto numUp = static_cast<int> (upBlock.getNumSamples());
    auto* upData = upBlock.getChannelPointer (0);

    // 5. Per-sample friction + waveguide loop at 2× rate.
    for (int i = 0; i < numUp; ++i)
    {
        bowModel.updateEnvelope();
        float v_bow = bowModel.getBowVelocity();
        float F_bow = bowModel.getBowForce();

        upData[i] = waveguideString.processSample (v_bow, F_bow, frictionModel);
    }

    // 6. Downsample back to host rate (writes into `block` which aliases voiceBuffer).
    oversampling.processSamplesDown (block);

    // 7. Mix host-rate voiceBuffer into the output buffer (mono → stereo split,
    //    centred). Phase 2.1a uses a fixed 0.35 normalisation factor matching
    //    O-Bowed; per-voice panning lands with multi-string voicing in 2.2.
    constexpr float kVoiceNorm = 0.35f;
    const int numOutChans = outputBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float s = voiceBuffer.getSample (0, i) * kVoiceNorm * outputGainLinear;

        // Safety hard-clip: prevents any one-shot runaway from leaking past
        // the voice. The in-loop saturator already bounds within ±1 in the
        // common case; this is the belt-and-braces final guard.
        s = juce::jlimit (-1.0f, 1.0f, s);

        outputBuffer.addSample (0, startSample + i, s);
        if (numOutChans >= 2)
            outputBuffer.addSample (1, startSample + i, s);
    }
}

//==============================================================================
// Parameter Reading
//==============================================================================

void BowedContrabassVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr)
        return;

    // UPPER_SNAKE_CASE per parameter-spec.md (frozen contract).
    float bowSpeed     = parameters->getRawParameterValue ("BOW_SPEED")->load();
    float bowPressure  = parameters->getRawParameterValue ("BOW_PRESSURE")->load();
    float bowPos       = parameters->getRawParameterValue ("BOW_POSITION")->load();
    float rosin        = parameters->getRawParameterValue ("ROSIN")->load();
    float brightness   = parameters->getRawParameterValue ("BRIGHTNESS")->load();
    float infSustain   = parameters->getRawParameterValue ("INFINITE_SUSTAIN")->load();
    float stiffness    = parameters->getRawParameterValue ("STRING_STIFFNESS")->load();
    float outputLevel  = parameters->getRawParameterValue ("OUTPUT_GAIN")->load();

    // MPE pressure modulates pressure (Z). Phase 2.6 will combine with CC11
    // expression macro; for now we apply the same ×0.5–2.0 envelope O-Bowed uses.
    auto note = getCurrentlyPlayingNote();
    float mpePressure = note.pressure.asUnsignedFloat();
    float mpeTimbre   = note.timbre.asSignedFloat();

    float effectivePressure = bowPressure * (0.5f + mpePressure * 1.5f);
    effectivePosition       = juce::jlimit (0.02f, 0.25f, bowPos + mpeTimbre * 0.05f);
    float effectiveSpeed    = bowSpeed * mpeExpression;

    // Push to DSP components.
    bowModel.setBowSpeed (effectiveSpeed);
    bowModel.setBowPressure (effectivePressure);
    frictionModel.setRosin (rosin);

    waveguideString.setBowPosition (effectivePosition);
    waveguideString.setBrightness (brightness);
    waveguideString.setInfiniteSustain (infSustain);
    waveguideString.setStringStiffness (stiffness);

    outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);
}
