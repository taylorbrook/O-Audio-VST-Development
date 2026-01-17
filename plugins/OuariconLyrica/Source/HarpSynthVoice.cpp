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

    // Default pluck parameters
    float pluckPosition = 0.5f;
    float fingerHardness = 0.5f;

    // Read parameters from APVTS (if available)
    if (parameters != nullptr)
    {
        // Phase 2.5: Read and apply string material
        auto* materialParam = parameters->getRawParameterValue("stringMaterial");
        if (materialParam != nullptr)
        {
            int materialIndex = static_cast<int>(materialParam->load());
            MaterialType materialType = StringMaterial::typeFromIndex(materialIndex);
            StringMaterial material = StringMaterial::fromType(materialType);
            stringModel.setMaterial(material);
        }

        auto* brightnessParam = parameters->getRawParameterValue("brightness");
        auto* sustainParam = parameters->getRawParameterValue("sustain");
        auto* pluckPositionParam = parameters->getRawParameterValue("pluckPosition");
        auto* fingerHardnessParam = parameters->getRawParameterValue("fingerHardness");
        auto* techniqueParam = parameters->getRawParameterValue("technique");
        auto* stiffnessParam = parameters->getRawParameterValue("stringStiffness");

        if (brightnessParam != nullptr)
            stringModel.setBrightness(brightnessParam->load());

        if (sustainParam != nullptr)
        {
            // Invert sustain to get damping (sustain=1.0 means low damping)
            float damping = 1.0f - sustainParam->load();
            stringModel.setDamping(damping);
        }

        if (pluckPositionParam != nullptr)
        {
            pluckPosition = pluckPositionParam->load();
            stringModel.setPluckPosition(pluckPosition);
        }

        if (fingerHardnessParam != nullptr)
            fingerHardness = fingerHardnessParam->load();

        if (techniqueParam != nullptr)
        {
            int techniqueIndex = static_cast<int>(techniqueParam->load());
            PlayingTechnique technique;
            switch (techniqueIndex)
            {
                case 0: technique = PlayingTechnique::Normal; break;
                case 1: technique = PlayingTechnique::Harmonic; break;
                case 2: technique = PlayingTechnique::Muted; break;
                case 3: technique = PlayingTechnique::PresDeLaTable; break;
                default: technique = PlayingTechnique::Normal; break;
            }
            stringModel.setTechnique(technique);
        }

        // Phase 2.4: Set string stiffness (inharmonicity)
        if (stiffnessParam != nullptr)
            stringModel.setStiffness(stiffnessParam->load());
    }

    // Trigger string model with pluck position and hardness
    stringModel.trigger(currentFrequency, velocity, pluckPosition, fingerHardness);
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
