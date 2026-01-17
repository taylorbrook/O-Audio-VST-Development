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
}

bool HarpSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<HarpSynthSound*>(sound) != nullptr;
}

void HarpSynthVoice::prepare(double sampleRate, int maxBlockSize)
{
    stringModel.prepare(sampleRate, maxBlockSize);
}

void HarpSynthVoice::setAPVTS(juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;
}

void HarpSynthVoice::startNote(int midiNoteNumber, float velocity,
                                juce::SynthesiserSound*,
                                int /*currentPitchWheelPosition*/)
{
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    currentVelocity = velocity;

    // Read parameters from APVTS (if available)
    if (parameters != nullptr)
    {
        auto* brightnessParam = parameters->getRawParameterValue("brightness");
        auto* sustainParam = parameters->getRawParameterValue("sustain");

        if (brightnessParam != nullptr)
            stringModel.setBrightness(brightnessParam->load());

        if (sustainParam != nullptr)
        {
            // Invert sustain to get damping (sustain=1.0 means low damping)
            float damping = 1.0f - sustainParam->load();
            stringModel.setDamping(damping);
        }
    }

    // Trigger string model
    stringModel.trigger(currentFrequency, velocity);
}

void HarpSynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        // For plucked strings, we let the natural decay continue
        // The string model will automatically fade out
    }
    else
    {
        // Hard stop - reset everything
        clearCurrentNote();
        stringModel.reset();
    }
}

void HarpSynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
    // Pitch bend will be implemented in Phase 2.8 (Tuning Engine)
}

void HarpSynthVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
    // MIDI CC will be implemented in later phases
}

void HarpSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      int startSample, int numSamples)
{
    // Check if voice should still be active
    if (!stringModel.isActive())
    {
        clearCurrentNote();
        return;
    }

    // Process samples through string model
    while (--numSamples >= 0)
    {
        // Generate one sample from physical model
        float sample = stringModel.processSample();

        // Add to output buffer (all channels)
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
            outputBuffer.addSample(i, startSample, sample);

        ++startSample;

        // Check if voice has decayed to silence
        if (!stringModel.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}
