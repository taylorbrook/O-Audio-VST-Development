/*
  ==============================================================================

    HarpSynthVoice.cpp
    Physical Modeling Harp Synthesizer Voice
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "HarpSynthVoice.h"

HarpSynthVoice::HarpSynthVoice()
{
    // Simple plucked string-like envelope for Stage 1
    adsrParams.attack = 0.001f;   // Fast attack (pluck)
    adsrParams.decay = 0.5f;      // Medium decay
    adsrParams.sustain = 0.3f;    // Low sustain (plucked strings decay)
    adsrParams.release = 2.0f;    // Long release for harp character
    adsr.setParameters(adsrParams);
}

bool HarpSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<HarpSynthSound*>(sound) != nullptr;
}

void HarpSynthVoice::startNote(int midiNoteNumber, float velocity,
                                juce::SynthesiserSound*,
                                int /*currentPitchWheelPosition*/)
{
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    phaseIncrement = currentFrequency * 2.0 * juce::MathConstants<double>::pi / getSampleRate();
    phase = 0.0;
    currentVelocity = velocity;
    isPlaying = true;

    adsr.noteOn();
}

void HarpSynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
        isPlaying = false;
    }
}

void HarpSynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
    // Pitch bend will be implemented in Stage 2
}

void HarpSynthVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // MIDI CC will be implemented in Stage 2
}

void HarpSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      int startSample, int numSamples)
{
    if (!isPlaying)
        return;

    while (--numSamples >= 0)
    {
        // Simple sine wave placeholder for Stage 1
        // Physical modeling DSP will be implemented in Stage 2
        auto currentSample = std::sin(phase);
        phase += phaseIncrement;

        // Keep phase in reasonable range
        if (phase > juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;

        // Apply ADSR envelope
        auto envelope = adsr.getNextSample();
        currentSample *= envelope * currentVelocity;

        // Add to output buffer
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
            outputBuffer.addSample(i, startSample, static_cast<float>(currentSample));

        ++startSample;

        // Stop if envelope finished
        if (!adsr.isActive())
        {
            clearCurrentNote();
            isPlaying = false;
            break;
        }
    }
}
