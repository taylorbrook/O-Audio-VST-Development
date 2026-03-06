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

// v1.32.1: File-scope lookup tables (previously duplicated inside startNote)
static constexpr int kIntervalSemitones[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 19, 24, 30, 36 };

// Beat multipliers: index 1=1/32, 2=1/16, 3=1/16D, 4=1/8T, 5=1/8, 6=1/8D,
// 7=1/4T, 8=1/4, 9=1/4D, 10=1/2, 11=1/2D, 12=1Bar, 13=2Bars, 14=4Bars
static constexpr double kSyncBeats[] = {
    0.0, 0.125, 0.25, 0.375, 1.0/3.0, 0.5, 0.75,
    2.0/3.0, 1.0, 1.5, 2.0, 3.0, 4.0, 8.0, 16.0
};

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

void HarpSynthVoice::setActiveGlissandoMode(std::atomic<int>* modePtr)
{
    activeGlissandoModePtr = modePtr;
}

void HarpSynthVoice::setCustomDegreeMask(std::atomic<uint64_t>* maskPtr)
{
    customDegreeMaskPtr = maskPtr;
}

void HarpSynthVoice::setHostBpm(std::atomic<double>* bpmPtr)
{
    hostBpmPtr = bpmPtr;
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

        // v1.32.7: Velocity-sensitive brightness — harder plucks excite more HF content
        // Maps vel 0→0.85x (darker) to vel 1→1.10x (brighter)
        brightness *= juce::jmap(velocity, 0.0f, 1.0f, 0.85f, 1.10f);
        brightness = juce::jlimit(0.0f, 1.0f, brightness);

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

        // v1.32.7: Velocity-sensitive hardness — harder plucks have firmer finger contact
        // Maps vel 0→0.78x (softer) to vel 1→1.10x (harder)
        fingerHardness *= juce::jmap(velocity, 0.0f, 1.0f, 0.78f, 1.10f);
        fingerHardness = juce::jlimit(0.0f, 1.0f, fingerHardness);

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

        // v1.30.0: Read glissando mode from processor atomic (keyswitches + toggles)
        int modeIndex = activeGlissandoModePtr ? activeGlissandoModePtr->load(std::memory_order_acquire) : 0;
        GlissandoMode glissandoMode = glissandoModeFromIndex(modeIndex);
        glissandoController.setMode(glissandoMode);

        // v1.32.3: Delegate glissando setup to extracted helpers
        if (glissandoMode == GlissandoMode::ScaleLocked && tuningEngine != nullptr)
            setupScaleLockedGlissando(midiNoteNumber);

        if (glissandoMode == GlissandoMode::Free)
            setupFreeGlissando();

        // v1.26.0: Set glissando excitation softening (brush vs deliberate pluck)
        if (glissandoMode != GlissandoMode::Off)
        {
            float glissExcitation = parameters->getRawParameterValue("glissandoExcitation")->load();
            stringModel.setGlissandoExcitation(glissExcitation);
        }
        else
        {
            stringModel.setGlissandoExcitation(0.0f);
        }
    }

    // v1.32.3: Delegate direction-dependent excitation to extracted helper
    applyDirectionExcitation(pluckPosition, fingerHardness);

    // Trigger string model with pluck position and hardness
    stringModel.trigger(currentFrequency, velocity, pluckPosition, fingerHardness);

    // Phase 2.7: Register voice with sympathetic resonance engine
    if (sympatheticEngine != nullptr)
    {
        cachedSympatheticSlot = sympatheticEngine->registerVoice(voiceId, currentFrequency, currentMaterial);
    }
}

// v1.32.3: Extracted from startNote() — ScaleLocked glissando setup
void HarpSynthVoice::setupScaleLockedGlissando(int midiNoteNumber)
{
    int intervalIndex = static_cast<int>(parameters->getRawParameterValue("glissandoInterval")->load());
    int directionIndex = static_cast<int>(parameters->getRawParameterValue("glissandoDirection")->load());

    int semitones;
    if (intervalIndex >= 16) // Custom
        semitones = static_cast<int>(parameters->getRawParameterValue("glissandoCustomSemitones")->load());
    else
        semitones = kIntervalSemitones[intervalIndex];

    // Fetch scale frequencies covering the full gliss range
    int scaleStart = midiNoteNumber - semitones - 1;
    int scaleCount = semitones * 2 + 3;
    if (scaleStart < 0) scaleStart = 0;
    if (scaleStart + scaleCount > 127) scaleCount = 128 - scaleStart;
    if (scaleCount > 64) scaleCount = 64;

    std::vector<double> scaleFreqs = tuningEngine->getScaleFrequencies(scaleStart, scaleCount);

    // v1.30.0: Wire glissandoScale — filter frequencies by selected scale
    int scaleIndex = static_cast<int>(parameters->getRawParameterValue("glissandoScale")->load());
    int glissTonic = static_cast<int>(parameters->getRawParameterValue("glissandoTonic")->load());
    int numScaleDegrees = tuningEngine->getScaleDegrees();

    // For non-12-note tunings, force Custom mode (Major/Minor/Pentatonic are 12-note patterns)
    if (numScaleDegrees != 12 && scaleIndex < 3)
        scaleIndex = 3;

    if (scaleIndex < 3) // Major=0, Minor=1, Pentatonic=2 (only for 12-note scales)
    {
        static const std::vector<std::vector<int>> scalePatterns = {
            {0, 2, 4, 5, 7, 9, 11},   // Major
            {0, 2, 3, 5, 7, 8, 10},    // Minor (natural)
            {0, 2, 4, 7, 9}             // Pentatonic
        };

        int anchorNote = 60 + glissTonic;
        std::vector<double> filtered;

        for (int n = scaleStart; n < scaleStart + scaleCount && n <= 127; ++n)
        {
            if (n < 0) continue;
            int degreeInScale = ((n - anchorNote) % 12 + 12) % 12;
            bool inScale = false;
            for (int d : scalePatterns[scaleIndex])
            {
                if (d == degreeInScale) { inScale = true; break; }
            }
            if (inScale)
                filtered.push_back(tuningEngine->getFrequency(n));
        }

        if (filtered.size() >= 2)
            glissandoController.setScale(filtered);
        else
            glissandoController.setScale(scaleFreqs);
    }
    else // Custom mode — use bitmask for any scale size
    {
        uint64_t degreeMask = customDegreeMaskPtr ? customDegreeMaskPtr->load(std::memory_order_acquire)
                                                   : 0xFFFFFFFFFFFFFFFF;

        int anchorNote = 60 + glissTonic;
        std::vector<double> filtered;

        for (int n = scaleStart; n < scaleStart + scaleCount && n <= 127; ++n)
        {
            if (n < 0) continue;
            int degreeInScale = ((n - anchorNote) % numScaleDegrees + numScaleDegrees) % numScaleDegrees;
            if ((degreeMask >> degreeInScale) & 1)
                filtered.push_back(tuningEngine->getFrequency(n));
        }

        if (filtered.size() >= 2)
            glissandoController.setScale(filtered);
        else
            glissandoController.setScale(scaleFreqs);
    }

    // v1.31.0: Tempo sync overrides manual speed when enabled
    int scaleTempoSyncIdx = static_cast<int>(parameters->getRawParameterValue("scaleTempoSync")->load());
    if (scaleTempoSyncIdx > 0 && hostBpmPtr != nullptr)
    {
        double bpm = hostBpmPtr->load(std::memory_order_acquire);
        double beatsPerSec = bpm / 60.0;
        double divBeats = kSyncBeats[scaleTempoSyncIdx];
        double totalDurationSec = divBeats / beatsPerSec;
        // Estimate step count from interval to derive notes-per-second
        int intervalIdx = static_cast<int>(parameters->getRawParameterValue("glissandoInterval")->load());
        int semitonesEst;
        if (intervalIdx >= 16)
            semitonesEst = static_cast<int>(parameters->getRawParameterValue("glissandoCustomSemitones")->load());
        else
            semitonesEst = kIntervalSemitones[intervalIdx];
        // Rough step count estimate (scale-locked steps vary, but semitones is a decent proxy)
        float stepsEst = juce::jmax(1.0f, static_cast<float>(semitonesEst));
        float notesPerSec = static_cast<float>(stepsEst / totalDurationSec);
        glissandoController.setSpeed(juce::jlimit(0.1f, 200.0f, notesPerSec));
    }
    else
    {
        float glissSpeed = parameters->getRawParameterValue("glissandoSpeed")->load();
        glissandoController.setSpeed(glissSpeed);
    }

    int shapeIndex = static_cast<int>(parameters->getRawParameterValue("glissandoShape")->load());
    glissandoController.setShape(glissandoShapeFromIndex(shapeIndex));

    float glissHumanize = parameters->getRawParameterValue("glissandoHumanize")->load();
    glissandoController.setHumanize(glissHumanize);

    // Direction 0 = Up to Note (start below), 1 = Down to Note (start above)
    double startFreq;
    if (directionIndex == 0)
        startFreq = currentFrequency / std::pow(2.0, semitones / 12.0);
    else
        startFreq = currentFrequency * std::pow(2.0, semitones / 12.0);

    float glissVelStart = parameters->getRawParameterValue("glissandoVelStart")->load();
    float glissVelEnd = parameters->getRawParameterValue("glissandoVelEnd")->load();
    glissandoController.setVelocityProfile(glissVelStart, glissVelEnd);

    glissandoController.startGlissando(startFreq, currentFrequency);
}

// v1.32.3: Extracted from startNote() — Free glissando setup
void HarpSynthVoice::setupFreeGlissando()
{
    // v1.31.0: Tempo sync overrides manual time when enabled
    int freeTempoSyncIdx = static_cast<int>(parameters->getRawParameterValue("freeTempoSync")->load());
    if (freeTempoSyncIdx > 0 && hostBpmPtr != nullptr)
    {
        double bpm = hostBpmPtr->load(std::memory_order_acquire);
        double beatsPerSec = bpm / 60.0;
        float rampSeconds = static_cast<float>(kSyncBeats[freeTempoSyncIdx] / beatsPerSec);
        glissandoController.setRampTime(juce::jlimit(0.01f, 10.0f, rampSeconds));
    }
    else
    {
        float glissTime = parameters->getRawParameterValue("glissandoTime")->load();
        glissandoController.setRampTime(glissTime);
    }

    // v1.30.0: Free mode shape
    int freeShapeIndex = static_cast<int>(parameters->getRawParameterValue("freeShape")->load());
    glissandoController.setShape(glissandoShapeFromIndex(freeShapeIndex));

    int intervalIndex = static_cast<int>(parameters->getRawParameterValue("freeInterval")->load());
    int directionIndex = static_cast<int>(parameters->getRawParameterValue("freeDirection")->load());

    int semitones;
    if (intervalIndex >= 16) // Custom
        semitones = static_cast<int>(parameters->getRawParameterValue("freeCustomSemitones")->load());
    else
        semitones = kIntervalSemitones[intervalIndex];

    double startFreq;
    if (directionIndex == 0)
        startFreq = currentFrequency / std::pow(2.0, semitones / 12.0);
    else
        startFreq = currentFrequency * std::pow(2.0, semitones / 12.0);

    glissandoController.startGlissando(startFreq, currentFrequency);
}

// v1.32.3: Extracted from startNote() — direction-dependent excitation character
void HarpSynthVoice::applyDirectionExcitation(float& pluckPosition, float& fingerHardness)
{
    if (parameters == nullptr)
        return;

    // v1.30.0: Read mode from atomic instead of old APVTS param
    GlissandoMode glissMode = glissandoModeFromIndex(
        activeGlissandoModePtr ? activeGlissandoModePtr->load(std::memory_order_acquire) : 0);

    if (glissMode == GlissandoMode::Off)
        return;

    // v1.28.0: Real harp glissandos use different body parts: ascending = finger pad (warm),
    // descending = thumb (brighter with subtle nail edge)
    float attackNoise = parameters->getRawParameterValue("attackNoise")->load();
    float humanizeAmt = parameters->getRawParameterValue("humanize")->load();
    attackNoise = applyHumanization(attackNoise, 0.10f, humanizeAmt);

    if (glissandoController.getDirection() == GlissandoDirection::Ascending)
    {
        // Finger pad: warmer, softer contact toward string center
        pluckPosition = juce::jlimit(0.0f, 1.0f, pluckPosition - 0.05f);
        fingerHardness *= 0.85f;
    }
    else
    {
        // Thumb: firmer, slightly brighter with subtle nail scrape
        pluckPosition = juce::jlimit(0.0f, 1.0f, pluckPosition + 0.04f);
        fingerHardness *= 1.1f;
        attackNoise = juce::jlimit(0.0f, 1.0f, attackNoise + 0.08f);
    }

    stringModel.setAttackNoise(attackNoise);
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
            cachedSympatheticSlot = -1;
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
            cachedSympatheticSlot = -1;
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
            cachedSympatheticSlot = -1;
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

            // v1.27.0: Apply velocity-dependent damping scaling
            float glissVel = glissandoController.getNextVelocity();
            float baseDamping = 1.0f - (parameters ? parameters->getRawParameterValue("timbre")->load() : 0.5f);
            stringModel.setDamping(baseDamping * (1.0f - glissVel * 0.3f));
        }

        // Generate one sample from physical model (string)
        float stringSample = stringModel.processSample();

        // Apply body resonance (Phase 2.6)
        float bodySample = bodyResonance.process(stringSample);

        // Phase 2.7: Compute sympathetic contribution from other voices
        float sympatheticContribution = 0.0f;
        if (sympatheticEngine != nullptr)
        {
            sympatheticContribution = sympatheticEngine->computeSympatheticContribution(cachedSympatheticSlot, bodySample);
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
                cachedSympatheticSlot = -1;
            }

            break;
        }
    }
}
