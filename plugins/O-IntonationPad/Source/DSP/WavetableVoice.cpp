/*
  ==============================================================================

    WavetableVoice.cpp
    Implementation of wavetable voice with ADSR envelope

  ==============================================================================
*/

#include "WavetableVoice.h"
#include "TuningSystem.h"

WavetableVoice::WavetableVoice()
{
    // Initialize ADSR with default parameters
    envelopeParams.attack = 0.5f;
    envelopeParams.decay = 0.1f;
    envelopeParams.sustain = 1.0f;
    envelopeParams.release = 2.0f;
    envelope.setParameters(envelopeParams);
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
        // Always generate MAX sub-voices with full complexity so voice count and
        // complexity can be changed in real-time on held notes
        auto chordVoices = chordGeneratorPtr->generateChord(midiNoteNumber, MAX_SUB_VOICES,
                                                             1.0f, cachedKeyRoot, cachedKeyScale);

        // Apply randomization to chord voices
        for (auto& chordVoice : chordVoices)
        {
            // Inversion randomization (octave shifts)
            if (randomPtr != nullptr && randomPtr->nextFloat() < cachedInversionRandom)
            {
                int octaveShift = randomPtr->nextInt(3) - 1;  // -1, 0, or +1 octave
                chordVoice.midiNote += octaveShift * 12;
            }
        }

        // All 12 sub-voices are always initialized
        activeSubVoices = juce::jmin(static_cast<int>(chordVoices.size()), MAX_SUB_VOICES);

        // Calculate max delay in samples from timingRandom (ms)
        int maxDelaySamples = static_cast<int>((cachedTimingRandom / 1000.0f) * currentSampleRate);

        for (int i = 0; i < activeSubVoices; ++i)
        {
            auto idx = static_cast<size_t>(i);

            float frequency;
            if (tuningSystemPtr != nullptr)
            {
                // Get tuned frequency with optional random detuning
                double centOffset = 0.0;
                if (randomPtr != nullptr && cachedDetuneRandom > 0.0f)
                {
                    centOffset = (randomPtr->nextFloat() * 2.0f - 1.0f) * static_cast<double>(cachedDetuneRandom);
                }
                frequency = static_cast<float>(tuningSystemPtr->getFrequencyWithOffset(
                    chordVoices[idx].midiNote, centOffset));
            }
            else
            {
                frequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(
                    chordVoices[idx].midiNote));
            }

            subVoiceOscillators[idx].setFrequency(frequency, currentSampleRate);
            subVoiceOscillators[idx].reset();

            // Store info for UI visualization
            subVoiceInfos[idx] = { chordVoices[idx].midiNote, frequency };

            // Store complexity threshold and set initial complexity gain
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

            // Set initial voice count gain (voices beyond cachedVoiceCount start silent)
            subVoiceVoiceCountGains[idx] = (i < cachedVoiceCount) ? 1.0f : 0.0f;

            // Assign random timing delay (first voice always starts immediately)
            if (i == 0 || randomPtr == nullptr || maxDelaySamples == 0)
            {
                subVoiceDelays[idx] = 0;
            }
            else
            {
                subVoiceDelays[idx] = randomPtr->nextInt(maxDelaySamples + 1);
            }
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
        subVoiceComplexityGains[0] = 1.0f;
        subVoiceVoiceCountGains[0] = 1.0f;
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
        // Sum all sub-voice oscillators with independent complexity and voice-count gains
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

            // Smooth both gains independently
            subVoiceComplexityGains[idx] += (complexityTarget - subVoiceComplexityGains[idx]) * gainSmoothCoeff;
            subVoiceVoiceCountGains[idx] += (voiceCountTarget - subVoiceVoiceCountGains[idx]) * gainSmoothCoeff;

            float combinedGain = subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx];

            // Check if this sub-voice's delay has elapsed
            if (subVoiceDelayCounters[idx] >= subVoiceDelays[idx])
            {
                float oscSample = subVoiceOscillators[idx].getNextSample();
                mixedSample += oscSample * combinedGain;
            }
            else
            {
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

void WavetableVoice::setWavetablePosition(float pos)
{
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        subVoiceOscillators[static_cast<size_t>(i)].setWavetablePosition(pos);
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

void WavetableVoice::setChordGenerationParams(int voiceCount, float complexity, int keyRoot, int keyScale,
                                                float inversionRandom, float detuneRandom, float timingRandom,
                                                ChordGenerator* chordGen, TuningSystem* tuning,
                                                juce::Random* random)
{
    cachedVoiceCount = voiceCount;
    cachedComplexity = complexity;
    cachedKeyRoot = keyRoot;
    cachedKeyScale = keyScale;
    cachedInversionRandom = inversionRandom;
    cachedDetuneRandom = detuneRandom;
    cachedTimingRandom = timingRandom;
    chordGeneratorPtr = chordGen;
    tuningSystemPtr = tuning;
    randomPtr = random;
}
