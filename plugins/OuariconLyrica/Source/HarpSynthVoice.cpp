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
            int materialIndex = static_cast<int>(materialParam->load() + 0.5f);  // Round to nearest
            MaterialType materialType = StringMaterial::typeFromIndex(materialIndex);
            currentMaterial = StringMaterial::fromType(materialType);
            currentMaterialType = materialType;  // Also update tracking variable
            stringModel.setMaterial(currentMaterial);
        }

        auto* brightnessParam = parameters->getRawParameterValue("brightness");
        auto* timbreParam = parameters->getRawParameterValue("timbre");
        auto* decayTimeParam = parameters->getRawParameterValue("decayTime");
        auto* pluckPositionParam = parameters->getRawParameterValue("pluckPosition");
        auto* fingerHardnessParam = parameters->getRawParameterValue("fingerHardness");
        auto* techniqueParam = parameters->getRawParameterValue("technique");
        auto* stiffnessParam = parameters->getRawParameterValue("stringStiffness");

        if (brightnessParam != nullptr)
            stringModel.setBrightness(brightnessParam->load());

        if (timbreParam != nullptr)
        {
            // v1.1.0: Renamed from sustain to timbre - controls tonal damping
            // Invert so timbre=1.0 means bright (low damping), timbre=0.0 means dark
            float damping = 1.0f - timbreParam->load();
            stringModel.setDamping(damping);
        }

        if (decayTimeParam != nullptr)
        {
            // v1.1.0: New decay time parameter - controls overall sustain duration
            stringModel.setDecayTime(decayTimeParam->load());
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
            stringModel.setTechnique(techniqueFromIndex(techniqueIndex));
        }

        // Phase 2.4: Set string stiffness (inharmonicity)
        if (stiffnessParam != nullptr)
            stringModel.setStiffness(stiffnessParam->load());

        // v1.2.0: Set advanced string parameters (tension, gauge, length)
        auto* tensionParam = parameters->getRawParameterValue("stringTension");
        auto* gaugeParam = parameters->getRawParameterValue("stringGauge");
        auto* lengthParam = parameters->getRawParameterValue("stringLength");

        if (tensionParam != nullptr)
            stringModel.setTension(tensionParam->load());

        if (gaugeParam != nullptr)
            stringModel.setGauge(gaugeParam->load());

        if (lengthParam != nullptr)
            stringModel.setLength(lengthParam->load());

        // v1.3.0: Set advanced physical modeling parameters
        auto* attackNoiseParam = parameters->getRawParameterValue("attackNoise");
        auto* bridgeBrightnessParam = parameters->getRawParameterValue("bridgeBrightness");

        if (attackNoiseParam != nullptr)
            stringModel.setAttackNoise(attackNoiseParam->load());

        if (bridgeBrightnessParam != nullptr)
            stringModel.setBridgeBrightness(bridgeBrightnessParam->load());

        // Phase 2.6: Set body resonance parameters
        auto* bodySizeParam = parameters->getRawParameterValue("bodySize");
        auto* bodyResonanceParam = parameters->getRawParameterValue("bodyResonance");
        auto* woodTypeParam = parameters->getRawParameterValue("woodType");
        auto* bodyModeSpreadParam = parameters->getRawParameterValue("bodyModeSpread");  // v1.3.0

        if (bodySizeParam != nullptr && bodyResonanceParam != nullptr && woodTypeParam != nullptr)
        {
            float bodySize = bodySizeParam->load();
            float bodyAmount = bodyResonanceParam->load();
            int woodTypeIndex = static_cast<int>(woodTypeParam->load());
            bodyResonance.setBodyParameters(bodySize, woodTypeFromIndex(woodTypeIndex), bodyAmount);
        }

        // v1.3.0: Set body mode spread
        if (bodyModeSpreadParam != nullptr)
            bodyResonance.setModeSpread(bodyModeSpreadParam->load());

        // Phase 2.9: Configure glissando controller
        auto* glissandoModeParam = parameters->getRawParameterValue("glissandoMode");
        if (glissandoModeParam != nullptr)
        {
            int glissandoModeIndex = static_cast<int>(glissandoModeParam->load());
            GlissandoMode glissandoMode = glissandoModeFromIndex(glissandoModeIndex);

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

void HarpSynthVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr)
        return;

    // Update brightness (affects tone color)
    auto* brightnessParam = parameters->getRawParameterValue("brightness");
    if (brightnessParam != nullptr)
        stringModel.setBrightness(brightnessParam->load());

    // v1.1.0: Update timbre (renamed from sustain - controls tonal damping)
    auto* timbreParam = parameters->getRawParameterValue("timbre");
    if (timbreParam != nullptr)
    {
        float damping = 1.0f - timbreParam->load();
        stringModel.setDamping(damping);
    }

    // v1.1.0: Update decay time (new parameter - controls overall sustain duration)
    auto* decayTimeParam = parameters->getRawParameterValue("decayTime");
    if (decayTimeParam != nullptr)
    {
        stringModel.setDecayTime(decayTimeParam->load());
    }

    // Update string stiffness
    auto* stiffnessParam = parameters->getRawParameterValue("stringStiffness");
    if (stiffnessParam != nullptr)
        stringModel.setStiffness(stiffnessParam->load());

    // v1.2.0: Update advanced string parameters (tension, gauge, length) in real-time
    auto* tensionParam = parameters->getRawParameterValue("stringTension");
    auto* gaugeParam = parameters->getRawParameterValue("stringGauge");
    auto* lengthParam = parameters->getRawParameterValue("stringLength");

    if (tensionParam != nullptr)
        stringModel.setTension(tensionParam->load());

    if (gaugeParam != nullptr)
        stringModel.setGauge(gaugeParam->load());

    if (lengthParam != nullptr)
        stringModel.setLength(lengthParam->load());

    // v1.3.0: Update advanced physical modeling parameters
    auto* attackNoiseParam = parameters->getRawParameterValue("attackNoise");
    auto* bridgeBrightnessParam = parameters->getRawParameterValue("bridgeBrightness");

    if (attackNoiseParam != nullptr)
        stringModel.setAttackNoise(attackNoiseParam->load());

    if (bridgeBrightnessParam != nullptr)
        stringModel.setBridgeBrightness(bridgeBrightnessParam->load());

    // Update string material
    auto* materialParam = parameters->getRawParameterValue("stringMaterial");
    if (materialParam != nullptr)
    {
        float rawValue = materialParam->load();
        int materialIndex = static_cast<int>(rawValue + 0.5f);  // Round to nearest to avoid float precision issues
        MaterialType materialType = StringMaterial::typeFromIndex(materialIndex);
        StringMaterial newMaterial = StringMaterial::fromType(materialType);

        // Only update if material changed (avoid unnecessary filter recalculations)
        if (materialType != currentMaterialType)
        {
            currentMaterialType = materialType;
            currentMaterial = newMaterial;
            stringModel.setMaterial(currentMaterial);
        }
    }

    // Update body resonance parameters
    auto* bodySizeParam = parameters->getRawParameterValue("bodySize");
    auto* bodyResonanceParam = parameters->getRawParameterValue("bodyResonance");
    auto* woodTypeParam = parameters->getRawParameterValue("woodType");
    auto* bodyModeSpreadParam = parameters->getRawParameterValue("bodyModeSpread");  // v1.3.0

    if (bodySizeParam != nullptr && bodyResonanceParam != nullptr && woodTypeParam != nullptr)
    {
        float bodySize = bodySizeParam->load();
        float bodyAmount = bodyResonanceParam->load();
        int woodTypeIndex = static_cast<int>(woodTypeParam->load());
        bodyResonance.setBodyParameters(bodySize, woodTypeFromIndex(woodTypeIndex), bodyAmount);
    }

    // v1.3.0: Update body mode spread
    if (bodyModeSpreadParam != nullptr)
        bodyResonance.setModeSpread(bodyModeSpreadParam->load());
}

void HarpSynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                      int startSample, int numSamples)
{
    // Update DSP parameters from APVTS at block boundaries (real-time modulation)
    updateParametersFromAPVTS();

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
