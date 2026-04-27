/*
  ==============================================================================

    BowedContrabassVoice.cpp
    O-Contrabass - 4-String Bank MPESynthesiserVoice (Phase 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BowedContrabassVoice.h"
#include "DSP/DispersionFilter.h"
#include <cmath>

namespace
{
    // Phase 2.2 locked constants (PLAN rev-6 preamble + RESEARCH §15.5/§15.6).
    constexpr float kOpenStringFrequencyHz[4] = { 41.20f, 55.00f, 73.42f, 98.00f };
    constexpr float kBOpen[4]                 = { 1.0e-4f, 7.0e-5f, 5.0e-5f, 3.0e-5f };
    constexpr int   kMPerString[4]            = { 4, 3, 2, 1 };
    constexpr const char* kDetuneParamIds[4]  = { "DETUNE_E", "DETUNE_A", "DETUNE_D", "DETUNE_G" };
}

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
    const int   midiNote = note.initialNote;
    const float velocity = note.noteOnVelocity.asUnsignedFloat();

    // 1. Resolve frequency (12-TET + MPE bend).
    double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const float bend = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bend) > 0.001f)
        freq *= std::pow (2.0, bend / 12.0);
    currentFrequency = static_cast<float> (freq);

    // 2. Resolve target string (closed-form ladder + ACTIVE_STRINGS clamp).
    const int activeStrings = static_cast<int> (parameters->getRawParameterValue ("ACTIVE_STRINGS")->load());
    const int newStringIndex = mapMidiNoteToStringIndex (midiNote, activeStrings);

    // 3. Decide trigger semantics.
    const bool isFirstNote    = (activeStringIndex < 0);
    const bool needsCrossfade = (! isFirstNote)
                              && bowModel.isActive()
                              && (newStringIndex != activeStringIndex);

    if (needsCrossfade)
    {
        previousStringIndex       = activeStringIndex;
        activeStringIndex         = newStringIndex;
        crossfadeRemainingSamples = crossfadeTotalSamples;
    }
    else
    {
        previousStringIndex       = -1;
        activeStringIndex         = newStringIndex;
        crossfadeRemainingSamples = 0;
    }

    // 4. Configure new string's delay-line immediately (avoid lag at old freq).
    const float detuneCents   = readDetuneForString (newStringIndex);
    const float targetSamples = computeDelaySamples (currentFrequency, detuneCents);
    detuneSmoothed[newStringIndex].setCurrentAndTargetValue (targetSamples);
    strings[newStringIndex].trigger (currentFrequency);
    strings[newStringIndex].setDelaySamples (targetSamples);

    // 5. Engage bow.
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
        for (auto& s : strings)
            s.reset();
        bowModel.reset();
        oversampling.reset();
        previousStringIndex       = -1;
        crossfadeRemainingSamples = 0;
    }
}

void BowedContrabassVoice::notePitchbendChanged()
{
    auto note = getCurrentlyPlayingNote();
    const int midiNote = note.initialNote;

    // Recompute base frequency + apply current MPE bend.
    double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const float bend = static_cast<float> (note.totalPitchbendInSemitones);
    if (std::abs (bend) > 0.001f)
        freq *= std::pow (2.0, bend / 12.0);
    currentFrequency = static_cast<float> (freq);

    if (activeStringIndex >= 0)
    {
        const float detuneCents   = readDetuneForString (activeStringIndex);
        const float targetSamples = computeDelaySamples (currentFrequency, detuneCents);
        detuneSmoothed[activeStringIndex].setTargetValue (targetSamples);
    }
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
    // No special handling needed for sustain/sostenuto in Phase 2.2.
}

//==============================================================================
// Lifecycle
//==============================================================================

void BowedContrabassVoice::prepareToPlay (double hostSampleRate, int maxBlockSize)
{
    currentMaxBlockSize = maxBlockSize;
    sr_internal         = hostSampleRate * 2.0;

    // 2× oversampler — initProcessing sized to maxBlockSize (host-rate block size).
    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
    oversampling.reset();

    // Mono scratch: maxBlockSize × 2 covers the upsampled buffer footprint.
    voiceBuffer.setSize (1, maxBlockSize * 2, /*keepExistingContent*/ false,
                         /*clearExtraSpace*/ true, /*avoidReallocating*/ false);
    voiceBuffer.clear();

    // Phase 2.2 — per-slot prepare. Each string gets its own M (E=4, A=3, D=2, G=1)
    // configured via R22's setDispersionActiveSections pass-through. Slot 0 (E):
    // prepare()'s internal setActiveSections(4) is followed by R22 setter to 4
    // (no-op re-set) — slot-0 final state matches Phase 2.1c bit-exactly.
    for (int s = 0; s < 4; ++s)
    {
        strings[s].prepare (sr_internal, maxBlockSize * 2);
        strings[s].setDispersionActiveSections (kMPerString[s]);
        detuneSmoothed[s].reset (sr_internal, 0.020);
        detuneSmoothed[s].setCurrentAndTargetValue (
            static_cast<float> (sr_internal) / kOpenStringFrequencyHz[s]);
    }

    bowModel.prepare (sr_internal);

    // Phase 2.1b — bass-string friction defaults (module init = treble defaults).
    frictionModel.setStaticFrictionCoefficient  (0.85f);
    frictionModel.setDynamicFrictionCoefficient (0.25f);

    // Crossfade ramp precompute (RESEARCH §15.3 — 5 ms equal-power, two array
    // loads/sample, ~3.5 KiB at 88.2 k internal SR).
    crossfadeTotalSamples = static_cast<int> (std::ceil (0.005 * sr_internal));
    crossfadeRamp.resize (static_cast<size_t> (crossfadeTotalSamples + 1));
    const float invN   = 1.0f / static_cast<float> (crossfadeTotalSamples);
    const float halfPi = juce::MathConstants<float>::halfPi;
    for (int i = 0; i <= crossfadeTotalSamples; ++i)
    {
        const float t = static_cast<float> (i) * invN;
        crossfadeRamp[static_cast<size_t> (i)] = { std::cos (t * halfPi), std::sin (t * halfPi) };
    }

    activeStringIndex         = -1;
    previousStringIndex       = -1;
    crossfadeRemainingSamples = 0;
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

    // Voice cleanup — bow released and all 4 strings decayed: clear the slot.
    if (! bowModel.isActive())
    {
        bool anyActive = false;
        for (auto& s : strings)
            if (s.isActive()) { anyActive = true; break; }

        if (! anyActive)
        {
            clearCurrentNote();
            return;
        }
    }

    // 2. Per-block dispersion + detune update sequence (RESEARCH §15.4).
    const float stringStiffness = parameters->getRawParameterValue ("STRING_STIFFNESS")->load();
    for (int s = 0; s < 4; ++s)
        strings[s].setStringStiffness (stringStiffness);

    for (int s = 0; s < 4; ++s)
        strings[s].advanceStiffnessSmootherBy (numSamples);

    for (int s = 0; s < 4; ++s)
    {
        const float currentStiffness = strings[s].getCurrentSmoothedStiffness();
        const float B = kBOpen[s] * juce::jlimit (0.0f, 1.0f, currentStiffness);
        const int   M = kMPerString[s];

        // Active + previous slots use played frequency (post-MPE-bend); idle
        // slots use their open-string default (no bend applied to silent strings).
        const float f0 = (s == activeStringIndex || s == previousStringIndex)
                       ? juce::jlimit (20.0f, 5000.0f, currentFrequency)
                       : juce::jlimit (20.0f, 5000.0f, kOpenStringFrequencyHz[s]);

        // Short-circuit at STRING_STIFFNESS=0 to preserve bit-exact regression bar.
        float a = (currentStiffness <= 0.0f)
                ? 0.0f
                : DispersionFilter<4>::computeAllpassCoefficient (f0, B, M);
        if (! std::isfinite (a)) a = 0.0f;
        strings[s].setDispersionCoefficient (a);
    }

    // Per-block detune ramp targets — only update active + previous; idle slots
    // stay at their last-set target (open-string default since prepare()).
    if (activeStringIndex >= 0)
    {
        const float dc = readDetuneForString (activeStringIndex);
        const float ds = computeDelaySamples (currentFrequency, dc);
        detuneSmoothed[activeStringIndex].setTargetValue (ds);
    }
    if (previousStringIndex >= 0)
    {
        const float dcPrev = readDetuneForString (previousStringIndex);
        const float dsPrev = computeDelaySamples (currentFrequency, dcPrev);
        detuneSmoothed[previousStringIndex].setTargetValue (dsPrev);
    }

    // 3. Clear the host-rate slice of voiceBuffer.
    voiceBuffer.clear (0, 0, numSamples);

    // 4. Wrap voiceBuffer in an AudioBlock for the host-rate slice.
    juce::dsp::AudioBlock<float> hostBlock (voiceBuffer);
    auto block = hostBlock.getSubBlock (0, static_cast<size_t> (numSamples));

    // 5. Upsample to 2× rate (silence → 2× scratch buffer).
    auto upBlock = oversampling.processSamplesUp (block);
    auto numUp = static_cast<int> (upBlock.getNumSamples());
    auto* upData = upBlock.getChannelPointer (0);

    // 6. Per-sample friction + 4-string bank loop at 2× rate.
    //    HARD RULE §15.9.5: use early-return on activeStringIndex (NOT
    //    unconditional sum) to preserve slot-0 bit-exact mix path.
    for (int i = 0; i < numUp; ++i)
    {
        bowModel.updateEnvelope();
        const float v_bow = bowModel.getBowVelocity();
        const float F_bow = bowModel.getBowForce();

        float mixedSample = 0.0f;

        if (crossfadeRemainingSamples > 0 && previousStringIndex >= 0)
        {
            const int idx = juce::jlimit (0, crossfadeTotalSamples,
                                          crossfadeTotalSamples - crossfadeRemainingSamples);
            const auto gains = crossfadeRamp[static_cast<size_t> (idx)];
            const float oldGain = gains.first;
            const float newGain = gains.second;

            // Per-sample setDelay BEFORE processSample (matches vibrato pattern).
            // Gate on isSmoothing() to avoid steady-state delay-line state churn
            // that could shift bit-exact regression for idle slots (§15.4 caveat).
            for (int s = 0; s < 4; ++s)
            {
                if (detuneSmoothed[s].isSmoothing())
                    strings[s].setDelaySamples (detuneSmoothed[s].getNextValue());
                else
                    detuneSmoothed[s].getNextValue();   // advance ramp without writing
            }

            float oldOut = 0.0f, newOut = 0.0f;
            for (int s = 0; s < 4; ++s)
            {
                if (s == previousStringIndex)
                    oldOut = strings[s].processSample (0.0f, 0.0f, frictionModel);
                else if (s == activeStringIndex)
                    newOut = strings[s].processSample (v_bow, F_bow, frictionModel);
                else
                    (void) strings[s].processSample (0.0f, 0.0f, frictionModel);
            }

            mixedSample = oldOut * oldGain + newOut * newGain;
            --crossfadeRemainingSamples;
            if (crossfadeRemainingSamples == 0)
                previousStringIndex = -1;
        }
        else
        {
            // Standard path. Slot-0 bit-exact mix path: when activeStringIndex==0,
            // mixedSample = strings[0].processSample(v_bow, F_bow, friction) verbatim.
            for (int s = 0; s < 4; ++s)
            {
                if (detuneSmoothed[s].isSmoothing())
                    strings[s].setDelaySamples (detuneSmoothed[s].getNextValue());
                else
                    detuneSmoothed[s].getNextValue();   // advance ramp without writing

                if (s == activeStringIndex)
                    mixedSample = strings[s].processSample (v_bow, F_bow, frictionModel);
                else
                    (void) strings[s].processSample (0.0f, 0.0f, frictionModel);
            }
        }

        upData[i] = mixedSample;
    }

    // 7. Downsample back to host rate (writes into `block` which aliases voiceBuffer).
    oversampling.processSamplesDown (block);

    // 8. Mix host-rate voiceBuffer into the output buffer (mono → stereo split).
    constexpr float kVoiceNorm = 0.35f;
    const int numOutChans = outputBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        float s = voiceBuffer.getSample (0, i) * kVoiceNorm * outputGainLinear;
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
    float outputLevel  = parameters->getRawParameterValue ("OUTPUT_GAIN")->load();

    // MPE pressure modulates pressure (Z); CC74 timbre modulates Y.
    auto note = getCurrentlyPlayingNote();
    float mpePressure = note.pressure.asUnsignedFloat();
    float mpeTimbre   = note.timbre.asSignedFloat();

    float effectivePressure = bowPressure * (0.5f + mpePressure * 1.5f);
    effectivePosition       = juce::jlimit (0.02f, 0.25f, bowPos + mpeTimbre * 0.05f);
    float effectiveSpeed    = bowSpeed * mpeExpression;

    bowModel.setBowSpeed (effectiveSpeed);
    bowModel.setBowPressure (effectivePressure);
    frictionModel.setRosin (rosin);

    // Push bow / brightness / sustain to all 4 strings — global parameters.
    // STRING_STIFFNESS push happens in renderNextBlock (Step A) for clarity.
    for (auto& s : strings)
    {
        s.setBowPosition (effectivePosition);
        s.setBrightness (brightness);
        s.setInfiniteSustain (infSustain);
    }

    outputGainLinear = juce::Decibels::decibelsToGain (outputLevel);
}

//==============================================================================
// Phase 2.2 Helpers
//==============================================================================

int BowedContrabassVoice::mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept
{
    int idx = 0;
    if      (midiNote >= 43) idx = 3;
    else if (midiNote >= 38) idx = 2;
    else if (midiNote >= 33) idx = 1;
    const int maxIdx = juce::jlimit (0, 3, activeStrings - 1);
    return juce::jmin (idx, maxIdx);
}

float BowedContrabassVoice::readDetuneForString (int s) const noexcept
{
    jassert (s >= 0 && s < 4);
    return parameters->getRawParameterValue (kDetuneParamIds[s])->load();
}

float BowedContrabassVoice::computeDelaySamples (float playedFreqHz, float detuneCents) const noexcept
{
    const float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
    const float detunedFreq = playedFreqHz * detuneRatio;
    return static_cast<float> (sr_internal) / juce::jmax (1.0f, detunedFreq);
}
