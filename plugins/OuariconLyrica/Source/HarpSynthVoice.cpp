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
    // Generate unique voice ID (using pointer address as unique identifier)
    voiceId = static_cast<int>(reinterpret_cast<intptr_t>(this) & 0xFFFFFF);
}

bool HarpSynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<HarpSynthSound*>(sound) != nullptr;
}

void HarpSynthVoice::prepare(double sampleRate, int maxBlockSize)
{
    stringModel.prepare(sampleRate, maxBlockSize);
    bodyResonance.prepare(sampleRate, maxBlockSize);
    glissandoController.prepare(sampleRate);
}

void HarpSynthVoice::setAPVTS(juce::AudioProcessorValueTreeState* apvts)
{
    parameters = apvts;
}

void HarpSynthVoice::setSympatheticEngine(SympatheticResonanceEngine* engine)
{
    sympatheticEngine = engine;
}

void HarpSynthVoice::setTuningEngine(TuningEngine* engine)
{
    tuningEngine = engine;
}

int HarpSynthVoice::getVoiceId() const
{
    return voiceId;
}

void HarpSynthVoice::startNote(int midiNoteNumber, float velocity,
                                juce::SynthesiserSound*,
                                int /*currentPitchWheelPosition*/)
{
    currentMidiNote = midiNoteNumber;
    currentVelocity = velocity;

    // Phase 2.8: Use TuningEngine for frequency calculation (if available)
    if (tuningEngine != nullptr)
    {
        currentFrequency = tuningEngine->getFrequency(midiNoteNumber);
    }
    else
    {
        // Fallback to standard JUCE MIDI to frequency conversion
        currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    }

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
            currentMaterial = StringMaterial::fromType(materialType);
            stringModel.setMaterial(currentMaterial);
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

        // Phase 2.6: Set body resonance parameters
        auto* bodySizeParam = parameters->getRawParameterValue("bodySize");
        auto* bodyResonanceParam = parameters->getRawParameterValue("bodyResonance");
        auto* woodTypeParam = parameters->getRawParameterValue("woodType");

        if (bodySizeParam != nullptr && bodyResonanceParam != nullptr && woodTypeParam != nullptr)
        {
            float bodySize = bodySizeParam->load();
            float bodyAmount = bodyResonanceParam->load();
            int woodTypeIndex = static_cast<int>(woodTypeParam->load());

            WoodType woodType;
            switch (woodTypeIndex)
            {
                case 0: woodType = WoodType::Spruce; break;
                case 1: woodType = WoodType::Maple; break;
                case 2: woodType = WoodType::Exotic; break;
                case 3: woodType = WoodType::Synthetic; break;
                default: woodType = WoodType::Spruce; break;
            }

            bodyResonance.setBodyParameters(bodySize, woodType, bodyAmount);
        }

        // Phase 2.9: Configure glissando controller
        auto* glissandoModeParam = parameters->getRawParameterValue("glissandoMode");
        if (glissandoModeParam != nullptr)
        {
            int glissandoModeIndex = static_cast<int>(glissandoModeParam->load());
            GlissandoMode glissandoMode;
            switch (glissandoModeIndex)
            {
                case 0: glissandoMode = GlissandoMode::Off; break;
                case 1: glissandoMode = GlissandoMode::Free; break;
                case 2: glissandoMode = GlissandoMode::ScaleLocked; break;
                default: glissandoMode = GlissandoMode::Off; break;
            }

            glissandoController.setMode(glissandoMode);

            // For scale-locked mode, get scale from tuning engine
            if (glissandoMode == GlissandoMode::ScaleLocked && tuningEngine != nullptr)
            {
                // Get 2 octaves of scale frequencies starting from current note
                std::vector<double> scaleFreqs = tuningEngine->getScaleFrequencies(midiNoteNumber - 12, 36);
                glissandoController.setScale(scaleFreqs);

                // Set glissando speed (default 10 notes per second)
                // TODO: Could add a parameter for this in future
                glissandoController.setSpeed(10.0f);
            }

            // Start glissando from previous frequency to new frequency
            if (glissandoMode != GlissandoMode::Off)
            {
                glissandoController.startGlissando(previousFrequency, currentFrequency);
            }
        }
    }

    // Store frequency for next glissando
    previousFrequency = currentFrequency;

    // Trigger string model with pluck position and hardness
    stringModel.trigger(currentFrequency, velocity, pluckPosition, fingerHardness);

    // Phase 2.7: Register voice with sympathetic resonance engine
    if (sympatheticEngine != nullptr)
    {
        sympatheticEngine->registerVoice(voiceId, currentFrequency, currentMaterial);
    }
}

void HarpSynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    // Phase 2.8: Clear pitch bend for this note
    if (tuningEngine != nullptr && currentMidiNote >= 0)
    {
        tuningEngine->clearPitchBend(currentMidiNote);
    }

    if (allowTailOff)
    {
        // For plucked strings, we let the natural decay continue
        // The string model will automatically fade out
        // But we still unregister from sympathetic engine to avoid stale tracking
        if (sympatheticEngine != nullptr)
        {
            sympatheticEngine->unregisterVoice(voiceId);
        }
    }
    else
    {
        // Hard stop - reset everything
        clearCurrentNote();
        stringModel.reset();
        bodyResonance.reset();

        // Phase 2.7: Unregister from sympathetic engine
        if (sympatheticEngine != nullptr)
        {
            sympatheticEngine->unregisterVoice(voiceId);
        }
    }
}

void HarpSynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // Phase 2.8: Implement pitch bend via TuningEngine
    if (tuningEngine != nullptr && currentMidiNote >= 0)
    {
        // Convert MIDI pitch wheel (0-16383, center=8192) to normalized bend amount (-1.0 to +1.0)
        const float normalizedBend = (newPitchWheelValue - 8192.0f) / 8192.0f;

        // Set pitch bend for this note in tuning engine
        tuningEngine->setPitchBend(currentMidiNote, normalizedBend);

        // Recalculate frequency with pitch bend applied
        currentFrequency = tuningEngine->getFrequency(currentMidiNote);

        // Update string model frequency in real-time
        stringModel.setFrequency(currentFrequency);
    }
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

        // Phase 2.7: Unregister from sympathetic engine when voice becomes inactive
        if (sympatheticEngine != nullptr)
        {
            sympatheticEngine->unregisterVoice(voiceId);
        }

        return;
    }

    // Process samples through string model and body resonance
    while (--numSamples >= 0)
    {
        // Phase 2.9: Update frequency from glissando controller (if active)
        if (glissandoController.isActive())
        {
            double glissandoFreq = glissandoController.getNextFrequency();
            stringModel.setFrequency(glissandoFreq);
        }

        // Generate one sample from physical model (string)
        float stringSample = stringModel.processSample();

        // Apply body resonance (Phase 2.6)
        float bodySample = bodyResonance.process(stringSample);

        // Phase 2.7: Compute sympathetic contribution from other voices
        float sympatheticContribution = 0.0f;
        if (sympatheticEngine != nullptr)
        {
            sympatheticContribution = sympatheticEngine->computeSympatheticContribution(voiceId, bodySample);
        }

        // Combine string + body + sympathetic resonance
        float sample = bodySample + sympatheticContribution;

        // Add to output buffer (all channels)
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
            outputBuffer.addSample(i, startSample, sample);

        ++startSample;

        // Check if voice has decayed to silence
        if (!stringModel.isActive())
        {
            clearCurrentNote();

            // Phase 2.7: Unregister from sympathetic engine
            if (sympatheticEngine != nullptr)
            {
                sympatheticEngine->unregisterVoice(voiceId);
            }

            break;
        }
    }
}
