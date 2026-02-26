/*
  ==============================================================================

    HarpSynthVoice.cpp
    Physical Modeling Harp Synthesizer Voice
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "HarpSynthVoice.h"

// v1.3.2: Static atomic counter for guaranteed unique voice IDs
std::atomic<int> HarpSynthVoice::nextVoiceId{0};

HarpSynthVoice::HarpSynthVoice()
{
    // v1.3.2: Generate unique voice ID using atomic counter (replaces pointer-based ID)
    // This guarantees uniqueness across all allocator patterns on 64-bit systems
    voiceId = nextVoiceId.fetch_add(1, std::memory_order_relaxed);
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

// v1.19.0: Apply humanization offset to a parameter value
float HarpSynthVoice::applyHumanization(float baseValue, float maxOffset, float humanizeAmount)
{
    if (humanizeAmount <= 0.0f)
        return baseValue;

    // Generate random offset in range [-maxOffset, +maxOffset]
    float offset = (humanizeRandom.nextFloat() * 2.0f - 1.0f) * maxOffset * humanizeAmount;
    return juce::jlimit(0.0f, 1.0f, baseValue + offset);
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

    // v1.19.0: Apply micro-tuning humanization (±3 cents variation)
    // Read humanize parameter early for frequency variation
    if (parameters != nullptr)
    {
        float humanizeAmount = parameters->getRawParameterValue("humanize")->load();
        if (humanizeAmount > 0.0f)
        {
            // Random cents deviation: ±3 cents at full humanization
            float centsDeviation = (humanizeRandom.nextFloat() * 2.0f - 1.0f) * 3.0f * humanizeAmount;
            // Convert cents to frequency ratio: 2^(cents/1200)
            double freqRatio = std::pow(2.0, centsDeviation / 1200.0);
            currentFrequency *= freqRatio;
        }
    }

    // Default pluck parameters
    float pluckPosition = 0.5f;
    float fingerHardness = 0.5f;

    // Read parameters from APVTS (if available)
    // v1.3.2: Removed per-parameter null checks - APVTS guarantees non-null for registered params
    if (parameters != nullptr)
    {
        // v1.19.0: Read humanize amount for per-note variation
        float humanizeAmount = parameters->getRawParameterValue("humanize")->load();

        // Phase 2.5: Read and apply string material
        int materialIndex = static_cast<int>(parameters->getRawParameterValue("stringMaterial")->load() + 0.5f);
        MaterialType materialType = StringMaterial::typeFromIndex(materialIndex);
        currentMaterial = StringMaterial::fromType(materialType);
        currentMaterialType = materialType;
        stringModel.setMaterial(currentMaterial);

        // Core string parameters
        // v1.19.0: Apply humanization to brightness (±4% variation)
        float brightness = parameters->getRawParameterValue("brightness")->load();
        brightness = applyHumanization(brightness, 0.04f, humanizeAmount);
        stringModel.setBrightness(brightness);

        // v1.1.0: Renamed from sustain to timbre - controls tonal damping
        // Invert so timbre=1.0 means bright (low damping), timbre=0.0 means dark
        float damping = 1.0f - parameters->getRawParameterValue("timbre")->load();
        stringModel.setDamping(damping);

        // v1.1.0: New decay time parameter - controls overall sustain duration
        // v1.19.0: Apply humanization to decay time (±3% variation)
        float decayTime = parameters->getRawParameterValue("decayTime")->load();
        decayTime = applyHumanization(decayTime / 20.0f, 0.03f, humanizeAmount) * 20.0f; // Normalize to 0-1 range for humanization
        stringModel.setDecayTime(decayTime);

        // v1.19.0: Apply humanization to pluck position (±5% variation)
        pluckPosition = parameters->getRawParameterValue("pluckPosition")->load();
        pluckPosition = applyHumanization(pluckPosition, 0.05f, humanizeAmount);
        stringModel.setPluckPosition(pluckPosition);

        // v1.19.0: Apply humanization to finger hardness (±8% variation)
        fingerHardness = parameters->getRawParameterValue("fingerHardness")->load();
        fingerHardness = applyHumanization(fingerHardness, 0.08f, humanizeAmount);

        int techniqueIndex = static_cast<int>(parameters->getRawParameterValue("technique")->load());
        stringModel.setTechnique(techniqueFromIndex(techniqueIndex));

        // Phase 2.4: Set string stiffness (inharmonicity)
        stringModel.setStiffness(parameters->getRawParameterValue("stringStiffness")->load());

        // v1.2.0: Set advanced string parameters (tension, gauge, length)
        stringModel.setTension(parameters->getRawParameterValue("stringTension")->load());
        stringModel.setGauge(parameters->getRawParameterValue("stringGauge")->load());
        stringModel.setLength(parameters->getRawParameterValue("stringLength")->load());

        // v1.3.0: Set advanced physical modeling parameters
        // v1.19.0: Apply humanization to attack noise (±10% variation)
        float attackNoise = parameters->getRawParameterValue("attackNoise")->load();
        attackNoise = applyHumanization(attackNoise, 0.10f, humanizeAmount);
        stringModel.setAttackNoise(attackNoise);
        stringModel.setBridgeBrightness(parameters->getRawParameterValue("bridgeBrightness")->load());

        // Phase 2.6: Set body resonance parameters
        float bodySize = parameters->getRawParameterValue("bodySize")->load();
        float bodyAmount = parameters->getRawParameterValue("bodyResonance")->load();
        int woodTypeIndex = static_cast<int>(parameters->getRawParameterValue("woodType")->load());
        bodyResonance.setBodyParameters(bodySize, woodTypeFromIndex(woodTypeIndex), bodyAmount);

        // v1.3.0: Set body mode spread
        bodyResonance.setModeSpread(parameters->getRawParameterValue("bodyModeSpread")->load());

        // Phase 2.9: Configure glissando controller
        int glissandoModeIndex = static_cast<int>(parameters->getRawParameterValue("glissandoMode")->load());
        GlissandoMode glissandoMode = glissandoModeFromIndex(glissandoModeIndex);

        glissandoController.setMode(glissandoMode);

        // For scale-locked mode, get scale from tuning engine
        if (glissandoMode == GlissandoMode::ScaleLocked && tuningEngine != nullptr)
        {
            // Get 2 octaves of scale frequencies starting from current note
            std::vector<double> scaleFreqs = tuningEngine->getScaleFrequencies(midiNoteNumber - 12, 36);
            glissandoController.setScale(scaleFreqs);

            float glissSpeed = parameters->getRawParameterValue("glissandoSpeed")->load();
            glissandoController.setSpeed(glissSpeed);

            // v1.22.0: Set glissando shape (acceleration curve)
            int shapeIndex = static_cast<int>(parameters->getRawParameterValue("glissandoShape")->load());
            glissandoController.setShape(glissandoShapeFromIndex(shapeIndex));
        }

        // Start glissando from previous frequency to new frequency
        if (glissandoMode != GlissandoMode::Off)
        {
            glissandoController.startGlissando(previousFrequency, currentFrequency);
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
    // v1.3.2: Removed per-parameter null checks - APVTS guarantees non-null for registered params
    if (parameters == nullptr)
        return;

    // Update brightness (affects tone color)
    stringModel.setBrightness(parameters->getRawParameterValue("brightness")->load());

    // v1.1.0: Update timbre (renamed from sustain - controls tonal damping)
    float damping = 1.0f - parameters->getRawParameterValue("timbre")->load();
    stringModel.setDamping(damping);

    // v1.1.0: Update decay time (new parameter - controls overall sustain duration)
    stringModel.setDecayTime(parameters->getRawParameterValue("decayTime")->load());

    // Update string stiffness
    stringModel.setStiffness(parameters->getRawParameterValue("stringStiffness")->load());

    // v1.2.0: Update advanced string parameters (tension, gauge, length) in real-time
    stringModel.setTension(parameters->getRawParameterValue("stringTension")->load());
    stringModel.setGauge(parameters->getRawParameterValue("stringGauge")->load());
    stringModel.setLength(parameters->getRawParameterValue("stringLength")->load());

    // v1.3.0: Update advanced physical modeling parameters
    stringModel.setAttackNoise(parameters->getRawParameterValue("attackNoise")->load());
    stringModel.setBridgeBrightness(parameters->getRawParameterValue("bridgeBrightness")->load());

    // Update string material
    float rawValue = parameters->getRawParameterValue("stringMaterial")->load();
    int materialIndex = static_cast<int>(rawValue + 0.5f);  // Round to nearest to avoid float precision issues
    MaterialType materialType = StringMaterial::typeFromIndex(materialIndex);

    // Only update if material changed (avoid unnecessary filter recalculations)
    if (materialType != currentMaterialType)
    {
        currentMaterialType = materialType;
        currentMaterial = StringMaterial::fromType(materialType);
        stringModel.setMaterial(currentMaterial);
    }

    // Update body resonance parameters
    float bodySize = parameters->getRawParameterValue("bodySize")->load();
    float bodyAmount = parameters->getRawParameterValue("bodyResonance")->load();
    int woodTypeIndex = static_cast<int>(parameters->getRawParameterValue("woodType")->load());
    bodyResonance.setBodyParameters(bodySize, woodTypeFromIndex(woodTypeIndex), bodyAmount);

    // v1.3.0: Update body mode spread
    bodyResonance.setModeSpread(parameters->getRawParameterValue("bodyModeSpread")->load());
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
