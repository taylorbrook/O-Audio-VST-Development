/*
  ==============================================================================

    BassoonVoice.cpp
    Modal Synthesis Bassoon - Synthesiser Voice (Stage 1 silent stub)
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1: silent voice stub — full SynthesiserVoice method surface, no DSP.
    Wiring (APVTS, TuningEngine, PendingTuningTable) stored but not consumed
    until Phase 2.1+ (first audio + DSP).

  ==============================================================================
*/

#include "BassoonVoice.h"

bool BassoonVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<BassoonSound*> (sound) != nullptr;
}

void BassoonVoice::startNote (int /*midiNoteNumber*/, float /*velocity*/,
                              juce::SynthesiserSound* /*sound*/, int /*currentPitchWheelPosition*/)
{
    // Stage 1: no-op. Phase 2.1 wires modal voice (mode bank + exciter + ADSR).
}

void BassoonVoice::stopNote (float /*velocity*/, bool /*allowTailOff*/)
{
    // Stage 1: clear current note immediately so juce::Synthesiser can reuse the slot.
    clearCurrentNote();
}

void BassoonVoice::pitchWheelMoved (int /*newPitchWheelValue*/)
{
    // Stage 1: no-op. Phase 2.1+ wires per-voice pitch-bend coefficient recompute.
}

void BassoonVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // Stage 1: no-op. Phase 2.3 wires CC2 -> breath parameter routing.
}

void BassoonVoice::renderNextBlock (juce::AudioBuffer<float>& /*outputBuffer*/,
                                    int /*startSample*/, int /*numSamples*/)
{
    // Stage 1: silent stub. Output buffer is left untouched (host clears it before
    // synthesiser.renderNextBlock; voices SUM into the buffer at Phase 2.1+).
    // First audio lands at Phase 2.1.
}
