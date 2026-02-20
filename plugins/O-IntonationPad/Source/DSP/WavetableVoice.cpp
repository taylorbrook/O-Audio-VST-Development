/*
  ==============================================================================

    WavetableVoice.cpp
    Implementation of wavetable voice with ADSR envelope

  ==============================================================================
*/

#include "WavetableVoice.h"
#include "TuningEngine.h"

WavetableVoice::WavetableVoice()
{
    // Initialize ADSR with default parameters
    envelopeParams.attack = 0.5f;
    envelopeParams.decay = 0.1f;
    envelopeParams.sustain = 1.0f;
    envelopeParams.release = 2.0f;
    envelope.setParameters(envelopeParams);
}

int WavetableVoice::getRandomOctaveShift(juce::Random* rng)
{
    if (rng == nullptr) return 1;
    float r = rng->nextFloat();
    if (r < 0.6f) return 1;       // 60% chance: 1 octave
    if (r < 0.9f) return 2;       // 30% chance: 2 octaves
    return 3;                      // 10% chance: 3 octaves
}

bool WavetableVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<WavetableSound*>(sound) != nullptr;
}

void WavetableVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentVelocity = velocity;
    currentSampleRate = getSampleRate();

    // Calculate gain smoothing coefficient for ~250ms gradual crossfade
    gainSmoothCoeff = 1.0f - std::exp(-1.0f / (0.25f * static_cast<float>(currentSampleRate)));

    // Generate chord voicing if chord generator is available
    if (chordGeneratorPtr != nullptr)
    {
        // Always generate MAX sub-voices with full complexity so voice count,
        // complexity, spacing, and inversion can all be changed in real-time on held notes
        auto chordVoices = chordGeneratorPtr->generateChord(midiNoteNumber, MAX_SUB_VOICES,
                                                             1.0f, cachedKeyRoot, cachedEnabledDegrees,
                                                             cachedScaleDegreeCount);

        // All 12 sub-voices are always initialized
        activeSubVoices = juce::jmin(static_cast<int>(chordVoices.size()), MAX_SUB_VOICES);

        // Calculate max delay in samples from timingRandom (ms)
        int maxDelaySamples = static_cast<int>((cachedTimingRandom / 1000.0f) * currentSampleRate);

        for (int i = 0; i < activeSubVoices; ++i)
        {
            auto idx = static_cast<size_t>(i);
            int baseMidiNote = chordVoices[idx].midiNote;

            // --- Base oscillator ---
            double centOffset = 0.0;
            if (randomPtr != nullptr && cachedDetuneRandom > 0.0f)
                centOffset = (randomPtr->nextFloat() * 2.0f - 1.0f) * static_cast<double>(cachedDetuneRandom);

            float baseFreq;
            if (tuningEnginePtr != nullptr)
            {
                double freq = tuningEnginePtr->getFrequency(baseMidiNote);
                if (centOffset != 0.0)
                    freq *= std::pow(2.0, centOffset / 1200.0);
                baseFreq = static_cast<float>(freq);
            }
            else
                baseFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(baseMidiNote));

            subVoiceOscillators[idx].setFrequency(baseFreq, currentSampleRate);
            subVoiceOscillators[idx].reset();
            subVoiceInfos[idx] = { baseMidiNote, baseFreq };

            // --- Spacing oscillator (shift UP by 1-3 octaves) ---
            int spacingOctaves = getRandomOctaveShift(randomPtr);
            int spacingMidiNote = baseMidiNote + (spacingOctaves * 12);
            if (spacingMidiNote > 127)
                spacingMidiNote = 127;  // Clamp to valid range

            float spacingFreq;
            if (tuningEnginePtr != nullptr)
            {
                double freq = tuningEnginePtr->getFrequency(spacingMidiNote);
                if (centOffset != 0.0)
                    freq *= std::pow(2.0, centOffset / 1200.0);
                spacingFreq = static_cast<float>(freq);
            }
            else
                spacingFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(spacingMidiNote));

            subVoiceSpacingOscillators[idx].setFrequency(spacingFreq, currentSampleRate);
            subVoiceSpacingOscillators[idx].reset();
            subVoiceSpacingInfos[idx] = { spacingMidiNote, spacingFreq };

            // --- Inversion oscillator (shift DOWN by 1-3 octaves) ---
            int inversionOctaves = getRandomOctaveShift(randomPtr);
            int inversionMidiNote = baseMidiNote - (inversionOctaves * 12);
            if (inversionMidiNote < 0)
                inversionMidiNote = 0;  // Clamp to valid range

            float inversionFreq;
            if (tuningEnginePtr != nullptr)
            {
                double freq = tuningEnginePtr->getFrequency(inversionMidiNote);
                if (centOffset != 0.0)
                    freq *= std::pow(2.0, centOffset / 1200.0);
                inversionFreq = static_cast<float>(freq);
            }
            else
                inversionFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(inversionMidiNote));

            subVoiceInversionOscillators[idx].setFrequency(inversionFreq, currentSampleRate);
            subVoiceInversionOscillators[idx].reset();
            subVoiceInversionInfos[idx] = { inversionMidiNote, inversionFreq };

            // Assign random thresholds for spacing and inversion (determines when this voice activates)
            subVoiceSpacingThresholds[idx] = (randomPtr != nullptr) ? randomPtr->nextFloat() : 0.5f;
            subVoiceInversionThresholds[idx] = (randomPtr != nullptr) ? randomPtr->nextFloat() : 0.5f;

            // Set initial gains based on current knob positions
            subVoiceSpacingGains[idx] = (cachedSpacing >= subVoiceSpacingThresholds[idx]) ? 1.0f : 0.0f;
            subVoiceInversionGains[idx] = (cachedInversion >= subVoiceInversionThresholds[idx]) ? 1.0f : 0.0f;

            // --- Complexity gain ---
            subVoiceComplexityThresholds[idx] = chordVoices[idx].complexityThreshold;
            float threshold = subVoiceComplexityThresholds[idx];
            if (threshold <= 0.0f)
                subVoiceComplexityGains[idx] = 1.0f;
            else if (cachedComplexity >= threshold)
                subVoiceComplexityGains[idx] = 1.0f;
            else if (cachedComplexity <= threshold - 0.1f)
                subVoiceComplexityGains[idx] = 0.0f;
            else
                subVoiceComplexityGains[idx] = (cachedComplexity - (threshold - 0.1f)) / 0.1f;

            // --- Voice count gain ---
            subVoiceVoiceCountGains[idx] = (i < cachedVoiceCount) ? 1.0f : 0.0f;

            // --- Timing delay ---
            if (i == 0 || randomPtr == nullptr || maxDelaySamples == 0)
                subVoiceDelays[idx] = 0;
            else
                subVoiceDelays[idx] = randomPtr->nextInt(maxDelaySamples + 1);
            subVoiceDelayCounters[idx] = 0;
        }
    }
    else
    {
        // Fallback: single voice at root frequency
        activeSubVoices = 1;
        float frequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
        subVoiceOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceOscillators[0].reset();
        subVoiceInfos[0] = { midiNoteNumber, frequency };
        subVoiceSpacingOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceSpacingOscillators[0].reset();
        subVoiceSpacingInfos[0] = { midiNoteNumber, frequency };
        subVoiceInversionOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceInversionOscillators[0].reset();
        subVoiceInversionInfos[0] = { midiNoteNumber, frequency };
        subVoiceComplexityGains[0] = 1.0f;
        subVoiceVoiceCountGains[0] = 1.0f;
        subVoiceSpacingGains[0] = 0.0f;
        subVoiceInversionGains[0] = 0.0f;
        subVoiceSpacingThresholds[0] = 1.0f;
        subVoiceInversionThresholds[0] = 1.0f;
    }

    // Start envelope
    envelope.noteOn();
}

void WavetableVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        envelope.noteOff();
    }
    else
    {
        clearCurrentNote();
        envelope.reset();
    }
}

void WavetableVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Sum all sub-voice oscillators with independent complexity, voice-count, spacing, and inversion gains
        float mixedSample = 0.0f;

        for (int i = 0; i < activeSubVoices; ++i)
        {
            auto idx = static_cast<size_t>(i);

            // --- Complexity gain target ---
            float threshold = subVoiceComplexityThresholds[idx];
            float complexityTarget;
            if (threshold <= 0.0f)
                complexityTarget = 1.0f;
            else if (cachedComplexity >= threshold)
                complexityTarget = 1.0f;
            else if (cachedComplexity <= threshold - 0.1f)
                complexityTarget = 0.0f;
            else
                complexityTarget = (cachedComplexity - (threshold - 0.1f)) / 0.1f;

            // --- Voice count gain target ---
            float voiceCountTarget = (i < cachedVoiceCount) ? 1.0f : 0.0f;

            // --- Spacing crossfade target ---
            float spacingTarget = (cachedSpacing >= subVoiceSpacingThresholds[idx]) ? 1.0f : 0.0f;

            // --- Inversion crossfade target ---
            float inversionTarget = (cachedInversion >= subVoiceInversionThresholds[idx]) ? 1.0f : 0.0f;

            // Smooth all gains independently
            subVoiceComplexityGains[idx] += (complexityTarget - subVoiceComplexityGains[idx]) * gainSmoothCoeff;
            subVoiceVoiceCountGains[idx] += (voiceCountTarget - subVoiceVoiceCountGains[idx]) * gainSmoothCoeff;
            subVoiceSpacingGains[idx] += (spacingTarget - subVoiceSpacingGains[idx]) * gainSmoothCoeff;
            subVoiceInversionGains[idx] += (inversionTarget - subVoiceInversionGains[idx]) * gainSmoothCoeff;

            float amplitudeGain = subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx];
            float spacingMix = subVoiceSpacingGains[idx];
            float inversionMix = subVoiceInversionGains[idx];
            float baseMix = (1.0f - spacingMix) * (1.0f - inversionMix);

            // Check if this sub-voice's delay has elapsed
            if (subVoiceDelayCounters[idx] >= subVoiceDelays[idx])
            {
                // 3-way mix: base + spacing(up) + inversion(down)
                float baseSample = subVoiceOscillators[idx].getNextSample() * baseMix;
                float spacingSample = subVoiceSpacingOscillators[idx].getNextSample() * spacingMix;
                float inversionSample = subVoiceInversionOscillators[idx].getNextSample() * inversionMix;
                mixedSample += (baseSample + spacingSample + inversionSample) * amplitudeGain;
            }
            else
            {
                // Still advance all oscillators during delay to keep them in sync
                subVoiceOscillators[idx].getNextSample();
                subVoiceSpacingOscillators[idx].getNextSample();
                subVoiceInversionOscillators[idx].getNextSample();
                ++subVoiceDelayCounters[idx];
            }
        }

        // Normalize by MAX sub-voices for consistent volume
        mixedSample /= static_cast<float>(MAX_SUB_VOICES);

        // Apply envelope
        float envelopedSample = mixedSample * envelope.getNextSample() * currentVelocity;

        // Write to all output channels (stereo)
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample + sample, envelopedSample);
        }

        // Check if envelope finished
        if (!envelope.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

void WavetableVoice::setWavetableBank(int bankIndex)
{
    const auto* bank = &WavetableData::BankCache::getBank(bankIndex);
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        subVoiceOscillators[idx].setWavetableBank(bank);
        subVoiceSpacingOscillators[idx].setWavetableBank(bank);
        subVoiceInversionOscillators[idx].setWavetableBank(bank);
    }
}

void WavetableVoice::setWavetablePosition(float pos)
{
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        subVoiceOscillators[idx].setWavetablePosition(pos);
        subVoiceSpacingOscillators[idx].setWavetablePosition(pos);
        subVoiceInversionOscillators[idx].setWavetablePosition(pos);
    }
}

void WavetableVoice::setEnvelopeParameters(float attack, float release)
{
    envelopeParams.attack = attack;
    envelopeParams.release = release;
    envelopeParams.decay = 0.1f;  // Fixed short decay
    envelopeParams.sustain = 1.0f;  // Full sustain level
    envelope.setParameters(envelopeParams);
}

void WavetableVoice::setChordGenerationParams(int voiceCount, float complexity, int keyRoot,
                                                const std::vector<int>& enabledDegrees, int scaleDegreeCount,
                                                float spacing, float inversion,
                                                float detuneRandom, float timingRandom,
                                                ChordGenerator* chordGen, TuningEngine* tuning,
                                                juce::Random* random)
{
    cachedVoiceCount = voiceCount;
    cachedComplexity = complexity;
    cachedKeyRoot = keyRoot;
    cachedEnabledDegrees = enabledDegrees;
    cachedScaleDegreeCount = scaleDegreeCount;
    cachedSpacing = spacing;
    cachedInversion = inversion;
    cachedDetuneRandom = detuneRandom;
    cachedTimingRandom = timingRandom;
    chordGeneratorPtr = chordGen;
    tuningEnginePtr = tuning;
    randomPtr = random;
}
