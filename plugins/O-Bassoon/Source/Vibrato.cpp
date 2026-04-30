/*
  ==============================================================================

    Vibrato.cpp
    Modal Synthesis Bassoon - Per-Voice Sine LFO + Onset Envelope (Phase 2.3)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "Vibrato.h"

void Vibrato::prepare (double sr) noexcept
{
    sampleRate = sr;
    recomputeIncrement();
    onset.reset (sampleRate, 0.0);
    onset.setCurrentAndTargetValue (1.0f);
}

void Vibrato::reset () noexcept
{
    // Random initial phase per startNote — O-Wind FluteSynthVoice.cpp:114-116
    // precedent (locked OQ#9-rev-3). juce::Random::getSystemRandom() shared-
    // state risk accepted per D7-rev-3 (Synthesiser::renderVoices is single-
    // threaded within a renderNextBlock call).
    phase = juce::Random::getSystemRandom().nextFloat()
          * juce::MathConstants<float>::twoPi;

    // rev-4 fix: re-arm onset envelope using the cached duration. Previously
    // `onset.reset(0.0f)` was misread by the compiler as `reset(int 0)`
    // (SmoothedValue has no reset(SampleType) overload), which sets numSteps=0
    // — i.e. countdown to 0, current jumps to target. Combined with
    // setOnsetMs's reset(sr, ms) that internally calls setCurrentAndTargetValue
    // (target), the onset gain stayed pinned at 1.0 from prepare(), making
    // every note's vibrato instant regardless of vibrato_onset (Gate 3 item 6).
    onset.reset (sampleRate, onsetDurationSeconds);
    onset.setCurrentAndTargetValue (0.0f);  // jump to 0
    onset.setTargetValue (1.0f);             // re-arm ramp to 1 over cached duration
}

void Vibrato::setRateHz (float r) noexcept
{
    rateHz = r;
    recomputeIncrement();
}

void Vibrato::setDepthCents (float d) noexcept
{
    // No smoothing — locked CONTEXT-rev-3 line 526; LFO modulation masks zipper.
    depthCents = d;
}

void Vibrato::setOnsetMs (float ms) noexcept
{
    // rev-4 fix: cache only — DO NOT touch the SmoothedValue here. Previously
    // the rampLength reset path called setCurrentAndTargetValue(target) under
    // the hood, which jumped current to 1.0 and killed the fade ramp on every
    // setExpression dispatch (which fires on the block AFTER startNote due to
    // shadow init `lastAppliedVibOnsetMs = -1` in BassoonVoice::startNote).
    // The cached duration is consumed by Vibrato::reset() at the next note-on,
    // which is the correct lifecycle event for re-arming the onset envelope.
    onsetDurationSeconds = juce::jmax (0.0, static_cast<double> (ms) / 1000.0);
}

float Vibrato::getCurrentCents() noexcept
{
    const float onsetGain = onset.getNextValue();
    const float cents     = depthCents * onsetGain * std::sin (phase);

    phase += phaseIncrement;
    if (phase > juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;

    return cents;
}

void Vibrato::recomputeIncrement() noexcept
{
    phaseIncrement = juce::MathConstants<float>::twoPi
                   * rateHz
                   / static_cast<float> (sampleRate);
}
