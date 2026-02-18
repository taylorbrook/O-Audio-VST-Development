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
        // complexity, and inversion can all be changed in real-time on held notes
        auto chordVoices = chordGeneratorPtr->generateChord(midiNoteNumber, MAX_SUB_VOICES,
                                                             1.0f, cachedKeyRoot, cachedKeyScale);

        // All 12 sub-voices are always initialized
        activeSubVoices = juce::jmin(static_cast<int>(chordVoices.size()), MAX_SUB_VOICES);

        // Calculate max delay in samples from timingRandom (ms)
        int maxDelaySamples = static_cast<int>((cachedTimingRandom / 1000.0f) * currentSampleRate);

        for (int i = 0; i < activeSubVoices; ++i)
        {
            auto idx = static_cast<size_t>(i);
            int baseMidiNote = chordVoices[idx].midiNote;

            // --- Base (uninverted) oscillator ---
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

            // --- Inverted oscillator (random ±1 octave shift) ---
            int octaveShift = (randomPtr != nullptr) ? (randomPtr->nextBool() ? 12 : -12) : 12;
            int invertedMidiNote = baseMidiNote + octaveShift;

            // Clamp to valid MIDI range, flip direction if needed
            if (invertedMidiNote < 0 || invertedMidiNote > 127)
                invertedMidiNote = baseMidiNote - octaveShift;

            float invertedFreq;
            if (tuningEnginePtr != nullptr)
            {
                double freq = tuningEnginePtr->getFrequency(invertedMidiNote);
                if (centOffset != 0.0)
                    freq *= std::pow(2.0, centOffset / 1200.0);
                invertedFreq = static_cast<float>(freq);
            }
            else
                invertedFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(invertedMidiNote));

            subVoiceInvertedOscillators[idx].setFrequency(invertedFreq, currentSampleRate);
            subVoiceInvertedOscillators[idx].reset();
            subVoiceInvertedInfos[idx] = { invertedMidiNote, invertedFreq };

            // Assign random inversion threshold (determines when this voice inverts)
            subVoiceInversionThresholds[idx] = (randomPtr != nullptr) ? randomPtr->nextFloat() : 0.5f;

            // Set initial inversion gain based on current knob position
            subVoiceInversionGains[idx] = (cachedInversionRandom >= subVoiceInversionThresholds[idx]) ? 1.0f : 0.0f;

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
        subVoiceInvertedOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceInvertedOscillators[0].reset();
        subVoiceInvertedInfos[0] = { midiNoteNumber, frequency };
        subVoiceComplexityGains[0] = 1.0f;
        subVoiceVoiceCountGains[0] = 1.0f;
        subVoiceInversionGains[0] = 0.0f;
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
        // Sum all sub-voice oscillators with independent complexity, voice-count, and inversion gains
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

            // --- Inversion crossfade target ---
            float inversionTarget = (cachedInversionRandom >= subVoiceInversionThresholds[idx]) ? 1.0f : 0.0f;

            // Smooth all three gains independently
            subVoiceComplexityGains[idx] += (complexityTarget - subVoiceComplexityGains[idx]) * gainSmoothCoeff;
            subVoiceVoiceCountGains[idx] += (voiceCountTarget - subVoiceVoiceCountGains[idx]) * gainSmoothCoeff;
            subVoiceInversionGains[idx] += (inversionTarget - subVoiceInversionGains[idx]) * gainSmoothCoeff;

            float amplitudeGain = subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx];
            float invMix = subVoiceInversionGains[idx];

            // Check if this sub-voice's delay has elapsed
            if (subVoiceDelayCounters[idx] >= subVoiceDelays[idx])
            {
                // Crossfade between base and inverted oscillators
                float baseSample = subVoiceOscillators[idx].getNextSample() * (1.0f - invMix);
                float invertedSample = subVoiceInvertedOscillators[idx].getNextSample() * invMix;
                mixedSample += (baseSample + invertedSample) * amplitudeGain;
            }
            else
            {
                // Still advance both oscillators during delay to keep them in sync
                subVoiceOscillators[idx].getNextSample();
                subVoiceInvertedOscillators[idx].getNextSample();
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
        auto idx = static_cast<size_t>(i);
        subVoiceOscillators[idx].setWavetablePosition(pos);
        subVoiceInvertedOscillators[idx].setWavetablePosition(pos);
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
                                                ChordGenerator* chordGen, TuningEngine* tuning,
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
    tuningEnginePtr = tuning;
    randomPtr = random;
}
