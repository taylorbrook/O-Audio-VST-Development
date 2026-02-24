/*
  ==============================================================================

    WavetableVoice.cpp
    Implementation of wavetable voice with ADSR envelope

  ==============================================================================
*/

#include "WavetableVoice.h"
#include "TuningEngine.h"

namespace {
    // Bhaskara I sine approximation — max error ~0.2%, sufficient for LFO modulation
    inline float fastSin(float x)
    {
        const float twoPi = 6.283185307f;
        const float pi = 3.141592654f;

        x = std::fmod(x, twoPi);
        if (x < 0.0f) x += twoPi;

        bool negate = x >= pi;
        if (negate) x -= pi;

        float xpi = x * (pi - x);
        float result = 16.0f * xpi / (5.0f * pi * pi - 4.0f * xpi);

        return negate ? -result : result;
    }
}

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

void WavetableVoice::prepare(int maxBlockSize)
{
    preparedBlockSize = maxBlockSize;
    scratchL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    scratchR.resize(static_cast<size_t>(maxBlockSize), 0.0f);
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
            subVoiceOscillators2[idx].setFrequency(baseFreq, currentSampleRate);
            subVoiceOscillators2[idx].reset();
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
            subVoiceSpacingOscillators2[idx].setFrequency(spacingFreq, currentSampleRate);
            subVoiceSpacingOscillators2[idx].reset();
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
            subVoiceInversionOscillators2[idx].setFrequency(inversionFreq, currentSampleRate);
            subVoiceInversionOscillators2[idx].reset();
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

            // --- v1.13.0: Stereo pan factor ---
            if (i == 0)
            {
                subVoicePanFactor[idx] = 0.0f;  // Root always centered
            }
            else
            {
                // Alternating L/R with width proportional to index
                float normalizedWidth = static_cast<float>(i) / static_cast<float>(MAX_SUB_VOICES - 1);
                float direction = (i % 2 == 0) ? -1.0f : 1.0f;
                float basePan = direction * normalizedWidth;

                // Add small random variation for ensemble feel
                if (randomPtr != nullptr)
                    basePan += (randomPtr->nextFloat() * 2.0f - 1.0f) * 0.05f;

                subVoicePanFactor[idx] = juce::jlimit(-1.0f, 1.0f, basePan);
            }

            // Initialize smoothed pan to current target
            {
                float targetPan = subVoicePanFactor[idx] * cachedStereoSpread;
                if (i == 1)
                    targetPan = juce::jlimit(-0.15f, 0.15f, targetPan);
                smoothedPan[idx] = juce::jlimit(-1.0f, 1.0f, targetPan);
            }

            // v1.14.0: Per-sub-voice LFO phase offset (root tracks global LFO exactly)
            subVoiceLFOPhaseOffsets[idx] = (i == 0) ? 0.0f
                : (randomPtr != nullptr ? randomPtr->nextFloat() * juce::MathConstants<float>::twoPi : 0.0f);
        }
    }
    else
    {
        // Fallback: single voice at root frequency
        activeSubVoices = 1;
        float frequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
        subVoiceOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceOscillators[0].reset();
        subVoiceOscillators2[0].setFrequency(frequency, currentSampleRate);
        subVoiceOscillators2[0].reset();
        subVoiceInfos[0] = { midiNoteNumber, frequency };
        subVoiceSpacingOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceSpacingOscillators[0].reset();
        subVoiceSpacingOscillators2[0].setFrequency(frequency, currentSampleRate);
        subVoiceSpacingOscillators2[0].reset();
        subVoiceSpacingInfos[0] = { midiNoteNumber, frequency };
        subVoiceInversionOscillators[0].setFrequency(frequency, currentSampleRate);
        subVoiceInversionOscillators[0].reset();
        subVoiceInversionOscillators2[0].setFrequency(frequency, currentSampleRate);
        subVoiceInversionOscillators2[0].reset();
        subVoiceInversionInfos[0] = { midiNoteNumber, frequency };
        subVoiceComplexityGains[0] = 1.0f;
        subVoiceVoiceCountGains[0] = 1.0f;
        subVoiceSpacingGains[0] = 0.0f;
        subVoiceInversionGains[0] = 0.0f;
        subVoiceSpacingThresholds[0] = 1.0f;
        subVoiceInversionThresholds[0] = 1.0f;
        subVoicePanFactor[0] = 0.0f;
        smoothedPan[0] = 0.0f;
        subVoiceLFOPhaseOffsets[0] = 0.0f;
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

    // --- v1.15.0: Voice-major (block-based) processing for cache locality ---
    // v1.15.4: Heap-allocated scratch buffers sized in prepare(), hard clamp
    if (preparedBlockSize <= 0)
        return;
    numSamples = juce::jmin(numSamples, preparedBlockSize);
    if (numSamples <= 0)
        return;

    std::fill(scratchL.begin(), scratchL.begin() + numSamples, 0.0f);
    std::fill(scratchR.begin(), scratchR.begin() + numSamples, 0.0f);

    // Block-rate smoothing: single-step equivalent of N per-sample steps
    float blockCoeff = 1.0f - std::pow(1.0f - gainSmoothCoeff, static_cast<float>(numSamples));

    // Smooth oscillator A/B gains at block rate
    smoothedGainA += (cachedGainA - smoothedGainA) * blockCoeff;
    smoothedGainB += (cachedGainB - smoothedGainB) * blockCoeff;

    // Voice-major loop: each sub-voice processes the entire block
    for (int i = 0; i < activeSubVoices; ++i)
    {
        auto idx = static_cast<size_t>(i);

        // --- Block-rate gain smoothing for all per-sub-voice parameters ---

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

        float voiceCountTarget = (i < cachedVoiceCount) ? 1.0f : 0.0f;
        float spacingTarget = (cachedSpacing >= subVoiceSpacingThresholds[idx]) ? 1.0f : 0.0f;
        float inversionTarget = (cachedInversion >= subVoiceInversionThresholds[idx]) ? 1.0f : 0.0f;

        subVoiceComplexityGains[idx] += (complexityTarget - subVoiceComplexityGains[idx]) * blockCoeff;
        subVoiceVoiceCountGains[idx] += (voiceCountTarget - subVoiceVoiceCountGains[idx]) * blockCoeff;
        subVoiceSpacingGains[idx] += (spacingTarget - subVoiceSpacingGains[idx]) * blockCoeff;
        subVoiceInversionGains[idx] += (inversionTarget - subVoiceInversionGains[idx]) * blockCoeff;

        // Smooth pan position at block rate
        float targetPan = subVoicePanFactor[idx] * cachedStereoSpread;
        if (i == 1)
            targetPan = juce::jlimit(-0.15f, 0.15f, targetPan);
        targetPan = juce::jlimit(-1.0f, 1.0f, targetPan);
        smoothedPan[idx] += (targetPan - smoothedPan[idx]) * blockCoeff;

        // Constant-power pan law
        float panAngle = (smoothedPan[idx] + 1.0f) * 0.5f;
        float panL = std::cos(panAngle * juce::MathConstants<float>::halfPi);
        float panR = std::sin(panAngle * juce::MathConstants<float>::halfPi);

        float amplitudeGain = subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx];
        float spacingMix = subVoiceSpacingGains[idx];
        float inversionMix = subVoiceInversionGains[idx];

        // Early-out: skip entire sub-voice if effectively silent
        if (amplitudeGain < 0.0001f)
        {
            subVoiceOscillators[idx].advancePhase(numSamples);
            subVoiceOscillators2[idx].advancePhase(numSamples);
            subVoiceSpacingOscillators[idx].advancePhase(numSamples);
            subVoiceSpacingOscillators2[idx].advancePhase(numSamples);
            subVoiceInversionOscillators[idx].advancePhase(numSamples);
            subVoiceInversionOscillators2[idx].advancePhase(numSamples);

            if (subVoiceDelayCounters[idx] < subVoiceDelays[idx])
            {
                int delayRemaining = subVoiceDelays[idx] - subVoiceDelayCounters[idx];
                subVoiceDelayCounters[idx] += juce::jmin(delayRemaining, numSamples);
            }
            continue;
        }

        float baseMix = (1.0f - spacingMix) * (1.0f - inversionMix);

        // Determine active portion of block (handle sub-voice delay)
        int activeStart = 0;
        int activeSamples = numSamples;

        if (subVoiceDelayCounters[idx] < subVoiceDelays[idx])
        {
            int delayRemaining = subVoiceDelays[idx] - subVoiceDelayCounters[idx];

            if (delayRemaining >= numSamples)
            {
                // Entire block is in delay period
                subVoiceDelayCounters[idx] += numSamples;
                subVoiceOscillators[idx].advancePhase(numSamples);
                subVoiceOscillators2[idx].advancePhase(numSamples);
                subVoiceSpacingOscillators[idx].advancePhase(numSamples);
                subVoiceSpacingOscillators2[idx].advancePhase(numSamples);
                subVoiceInversionOscillators[idx].advancePhase(numSamples);
                subVoiceInversionOscillators2[idx].advancePhase(numSamples);
                continue;
            }

            // Delay expires mid-block: advance through delay portion, process remainder
            activeStart = delayRemaining;
            activeSamples = numSamples - delayRemaining;
            subVoiceDelayCounters[idx] = subVoiceDelays[idx];

            subVoiceOscillators[idx].advancePhase(delayRemaining);
            subVoiceOscillators2[idx].advancePhase(delayRemaining);
            subVoiceSpacingOscillators[idx].advancePhase(delayRemaining);
            subVoiceSpacingOscillators2[idx].advancePhase(delayRemaining);
            subVoiceInversionOscillators[idx].advancePhase(delayRemaining);
            subVoiceInversionOscillators2[idx].advancePhase(delayRemaining);
        }

        // Process active oscillators into scratch buffers
        float* destL = scratchL.data() + activeStart;
        float* destR = scratchR.data() + activeStart;

        // Base oscillators (skip when spacing + inversion fully replace base)
        if (baseMix >= 0.0001f)
        {
            subVoiceOscillators[idx].processBlockStereo(destL, destR, activeSamples,
                baseMix * amplitudeGain * smoothedGainA * panL,
                baseMix * amplitudeGain * smoothedGainA * panR);
            subVoiceOscillators2[idx].processBlockStereo(destL, destR, activeSamples,
                baseMix * amplitudeGain * smoothedGainB * panL,
                baseMix * amplitudeGain * smoothedGainB * panR);
        }
        else
        {
            subVoiceOscillators[idx].advancePhase(activeSamples);
            subVoiceOscillators2[idx].advancePhase(activeSamples);
        }

        // Spacing oscillators (skip when spacing mix near zero)
        if (spacingMix >= 0.0001f)
        {
            subVoiceSpacingOscillators[idx].processBlockStereo(destL, destR, activeSamples,
                spacingMix * amplitudeGain * smoothedGainA * panL,
                spacingMix * amplitudeGain * smoothedGainA * panR);
            subVoiceSpacingOscillators2[idx].processBlockStereo(destL, destR, activeSamples,
                spacingMix * amplitudeGain * smoothedGainB * panL,
                spacingMix * amplitudeGain * smoothedGainB * panR);
        }
        else
        {
            subVoiceSpacingOscillators[idx].advancePhase(activeSamples);
            subVoiceSpacingOscillators2[idx].advancePhase(activeSamples);
        }

        // Inversion oscillators (skip when inversion mix near zero)
        if (inversionMix >= 0.0001f)
        {
            subVoiceInversionOscillators[idx].processBlockStereo(destL, destR, activeSamples,
                inversionMix * amplitudeGain * smoothedGainA * panL,
                inversionMix * amplitudeGain * smoothedGainA * panR);
            subVoiceInversionOscillators2[idx].processBlockStereo(destL, destR, activeSamples,
                inversionMix * amplitudeGain * smoothedGainB * panL,
                inversionMix * amplitudeGain * smoothedGainB * panR);
        }
        else
        {
            subVoiceInversionOscillators[idx].advancePhase(activeSamples);
            subVoiceInversionOscillators2[idx].advancePhase(activeSamples);
        }
    }

    // Final pass: normalize, apply envelope, write to output
    float normFactor = 1.0f / static_cast<float>(MAX_SUB_VOICES);

    if (outputBuffer.getNumChannels() >= 2)
    {
        auto* outL = outputBuffer.getWritePointer(0, startSample);
        auto* outR = outputBuffer.getWritePointer(1, startSample);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float envGain = envelope.getNextSample() * currentVelocity * normFactor;
            auto s = static_cast<size_t>(sample);
            outL[sample] += scratchL[s] * envGain;
            outR[sample] += scratchR[s] * envGain;

            if (!envelope.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }
    else if (outputBuffer.getNumChannels() == 1)
    {
        auto* outMono = outputBuffer.getWritePointer(0, startSample);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float envGain = envelope.getNextSample() * currentVelocity * normFactor;
            auto s = static_cast<size_t>(sample);
            outMono[sample] += (scratchL[s] + scratchR[s]) * 0.5f * envGain;

            if (!envelope.isActive())
            {
                clearCurrentNote();
                break;
            }
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

void WavetableVoice::setWavetableBank2(int bankIndex)
{
    const auto* bank = &WavetableData::BankCache::getBank(bankIndex);
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        subVoiceOscillators2[idx].setWavetableBank(bank);
        subVoiceSpacingOscillators2[idx].setWavetableBank(bank);
        subVoiceInversionOscillators2[idx].setWavetableBank(bank);
    }
}

void WavetableVoice::setWavetablePosition2(float pos)
{
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        subVoiceOscillators2[idx].setWavetablePosition(pos);
        subVoiceSpacingOscillators2[idx].setWavetablePosition(pos);
        subVoiceInversionOscillators2[idx].setWavetablePosition(pos);
    }
}

void WavetableVoice::setWavetablePositionWithLFO(float basePos, float lfoPhase, float lfoDepth)
{
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        float offsetPhase = lfoPhase + subVoiceLFOPhaseOffsets[idx];
        float perVoiceLFO = fastSin(offsetPhase) * lfoDepth;
        float modulatedPos = juce::jlimit(0.0f, 1.0f, basePos + perVoiceLFO);
        subVoiceOscillators[idx].setWavetablePosition(modulatedPos);
        subVoiceSpacingOscillators[idx].setWavetablePosition(modulatedPos);
        subVoiceInversionOscillators[idx].setWavetablePosition(modulatedPos);
    }
}

void WavetableVoice::setWavetablePosition2WithLFO(float basePos, float lfoPhase, float lfoDepth)
{
    for (int i = 0; i < MAX_SUB_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        float offsetPhase = lfoPhase + subVoiceLFOPhaseOffsets[idx];
        float perVoiceLFO = fastSin(offsetPhase) * lfoDepth;
        float modulatedPos = juce::jlimit(0.0f, 1.0f, basePos + perVoiceLFO);
        subVoiceOscillators2[idx].setWavetablePosition(modulatedPos);
        subVoiceSpacingOscillators2[idx].setWavetablePosition(modulatedPos);
        subVoiceInversionOscillators2[idx].setWavetablePosition(modulatedPos);
    }
}

void WavetableVoice::setGainA(float gain)
{
    cachedGainA = gain;
}

void WavetableVoice::setGainB(float gain)
{
    cachedGainB = gain;
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

void WavetableVoice::setStereoSpread(float spread)
{
    cachedStereoSpread = spread;
}
