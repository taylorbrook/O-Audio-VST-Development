/*
  ==============================================================================

    BassoonVoice.cpp
    Modal Synthesis Bassoon - Synthesiser Voice (Phase 2.1 first audio)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1: working modal-synthesis voice — sustained, in-tune tone for any
    single MIDI note (C1-C6). NO bassoon-specific timbre yet (placeholder
    integer harmonics + flat amplitudes), NO APVTS reads, NO TuningEngine call,
    NO vibrato/breath/voice manager — strict ROADMAP minimal wiring.

  ==============================================================================
*/

#include "BassoonVoice.h"

bool BassoonVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<BassoonSound*> (sound) != nullptr;
}

void BassoonVoice::prepareToPlay (double sampleRate, int /*maxBlockSize*/)
{
    setCurrentPlaybackSampleRate (sampleRate);   // JUCE bookkeeping
    modeBank.prepare (sampleRate);
    exciter.prepare  (sampleRate);

    // CRITICAL: setSampleRate MUST be called BEFORE setParameters
    // (JUCE 8 ADSR contract — juce_ADSR.h:115-119; recalculateRates uses
    // the stored sample rate, defaulted to 44100.0 if setSampleRate not called).
    adsr.setSampleRate (sampleRate);
    adsr.setParameters (juce::ADSR::Parameters { 0.010f, 0.0f, 1.0f, 0.200f });
}

void BassoonVoice::startNote (int midiNoteNumber, float /*velocity*/,
                              juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    pitchWheelValue       = currentPitchWheelPosition;
    pitchBendSemitones    = ((static_cast<float> (pitchWheelValue) - 8192.0f) / 8192.0f)
                            * PITCH_BEND_RANGE_SEMITONES;

    // Phase 2.1: plain MIDI A=440 12-TET frequency (NOT TuningEngine — locked Q2).
    // Phase 2.4 will replace this with tuningEngine->getFrequency() per-voice.
    currentFrequencyBase  = static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));

    const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
    modeBank.setFundamental (fBent);
    modeBank.strike();           // rev-3: inject modal sustain energy (state init)
    exciter.start();             // exciter still fires for attack-transient flavor
    adsr.noteOn();
}

void BassoonVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
        modeBank.reset();
        exciter.reset();
        currentFrequencyBase = 0.0f;
    }
}

void BassoonVoice::pitchWheelMoved (int newPitchWheelValue)
{
    pitchWheelValue    = newPitchWheelValue;
    pitchBendSemitones = ((static_cast<float> (pitchWheelValue) - 8192.0f) / 8192.0f)
                         * PITCH_BEND_RANGE_SEMITONES;

    // Guard: only recompute coefficients if a note is currently sounding.
    // Pitch-wheel events arriving before the first note-on must not mutate
    // the mode bank with f0 = 0.
    if (currentFrequencyBase > 0.0f)
    {
        const float fBent = currentFrequencyBase * std::pow (2.0f, pitchBendSemitones / 12.0f);
        modeBank.setFundamental (fBent);
    }
}

void BassoonVoice::controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/)
{
    // Phase 2.1: no-op. Phase 2.3 wires CC2 -> breath parameter routing.
}

void BassoonVoice::setTone (float tone01) noexcept
{
    // Phase 2.2: throttle gate is at the processor dispatch site, not here.
    modeBank.setTone (tone01);
    modeBank.applyToneChange();
}

void BassoonVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    if (! adsr.isActive())
        return;

    const int numChannels = outputBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const float ex    = exciter.getNextSample();
        const float voice = modeBank.processSample (ex);
        const float env   = adsr.getNextSample();
        const float out   = voice * env;

        // Voices SUM into the buffer — host's processBlock clears the buffer
        // BEFORE synthesiser.renderNextBlock (PluginProcessor.cpp:165).
        // juce::Synthesiser::renderVoices does NOT zero the buffer.
        for (int ch = numChannels; --ch >= 0;)
            outputBuffer.addSample (ch, startSample + i, out);

        if (! adsr.isActive())
        {
            clearCurrentNote();
            modeBank.reset();
            exciter.reset();
            currentFrequencyBase = 0.0f;
            return;
        }
    }
}
