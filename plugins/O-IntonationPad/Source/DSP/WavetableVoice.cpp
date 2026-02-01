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

    // Generate chord voicing if chord generator is available
    if (chordGeneratorPtr != nullptr)
    {
        auto chordVoices = chordGeneratorPtr->generateChord(midiNoteNumber, cachedVoiceCount,
                                                             cachedComplexity, cachedKeyRoot, cachedKeyScale);

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

        // Set frequencies for each sub-voice
        activeSubVoices = juce::jmin(static_cast<int>(chordVoices.size()), MAX_SUB_VOICES);

        // Calculate max delay in samples from timingRandom (ms)
        int maxDelaySamples = static_cast<int>((cachedTimingRandom / 1000.0f) * currentSampleRate);

        for (int i = 0; i < activeSubVoices; ++i)
        {
            float frequency;
            if (tuningSystemPtr != nullptr)
            {
                // Get tuned frequency with optional random detuning
                double centOffset = 0.0;
                if (randomPtr != nullptr && cachedDetuneRandom > 0.0f)
                {
                    // Gaussian-ish random detuning
                    centOffset = (randomPtr->nextFloat() * 2.0f - 1.0f) * static_cast<double>(cachedDetuneRandom);
                }
                frequency = static_cast<float>(tuningSystemPtr->getFrequencyWithOffset(
                    chordVoices[static_cast<size_t>(i)].midiNote, centOffset));
            }
            else
            {
                frequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(
                    chordVoices[static_cast<size_t>(i)].midiNote));
            }

            subVoiceOscillators[static_cast<size_t>(i)].setFrequency(frequency, currentSampleRate);
            subVoiceOscillators[static_cast<size_t>(i)].reset();

            // Assign random timing delay (first voice always starts immediately)
            if (i == 0 || randomPtr == nullptr || maxDelaySamples == 0)
            {
                subVoiceDelays[static_cast<size_t>(i)] = 0;
            }
            else
            {
                subVoiceDelays[static_cast<size_t>(i)] = randomPtr->nextInt(maxDelaySamples + 1);
            }
            subVoiceDelayCounters[static_cast<size_t>(i)] = 0;
        }
    }
    else
    {
        // Fallback: single voice at root frequency
        activeSubVoices = 1;
        float frequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
        subVoiceOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceOscillators[0].reset();
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
        // Sum all sub-voice oscillators (respecting timing delays)
        float mixedSample = 0.0f;
        int activeCount = 0;

        for (int i = 0; i < activeSubVoices; ++i)
        {
            // Check if this sub-voice's delay has elapsed
            if (subVoiceDelayCounters[static_cast<size_t>(i)] >= subVoiceDelays[static_cast<size_t>(i)])
            {
                mixedSample += subVoiceOscillators[static_cast<size_t>(i)].getNextSample();
                ++activeCount;
            }
            else
            {
                // Increment delay counter (voice not yet sounding)
                ++subVoiceDelayCounters[static_cast<size_t>(i)];
            }
        }

        // Average to prevent clipping (divide by number of currently sounding sub-voices)
        if (activeCount > 0)
            mixedSample /= static_cast<float>(activeCount);

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
    for (int i = 0; i < activeSubVoices; ++i)
    {
        subVoiceOscillators[i].setWavetablePosition(pos);
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
