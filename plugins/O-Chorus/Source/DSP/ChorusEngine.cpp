/*
  ==============================================================================

    O-Chorus - Chorus DSP Engine Implementation
    Multi-voice BBD-style chorus with modulated delay lines
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "ChorusEngine.h"

ChorusEngine::ChorusEngine()
{
    // Generate seeded depth variations for each voice
    for (size_t i = 0; i < static_cast<size_t> (maxVoices); ++i)
    {
        juce::Random rng (static_cast<int> (i) + 42);
        voices[i].depthVariation = 0.85f + rng.nextFloat() * 0.3f; // 0.85 to 1.15
    }
}

void ChorusEngine::prepare (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    juce::ignoreUnused (samplesPerBlock);

    auto maxDelaySamples = static_cast<int> (sampleRate * maxDelayMs / 1000.0);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    for (auto& voice : voices)
    {
        voice.delayLine.setMaximumDelayInSamples (maxDelaySamples);
        voice.delayLine.prepare (spec);
    }

    // Init smoothed values (50ms ramp, 100ms for tone)
    smoothedRate.reset (sampleRate, 0.05);
    smoothedDepth.reset (sampleRate, 0.05);
    smoothedSpread.reset (sampleRate, 0.05);
    smoothedWidth.reset (sampleRate, 0.05);
    smoothedMix.reset (sampleRate, 0.05);
    smoothedDrive.reset (sampleRate, 0.05);
    smoothedTone.reset (sampleRate, 0.1);

    // Tone filters
    toneFilterL.reset();
    toneFilterR.reset();
    lastToneParam = 0.0f;
    updateToneFilter (0.0f);

    // Crossfade
    crossfadeIncrement = 1.0f / static_cast<float> (sampleRate * crossfadeDurationMs / 1000.0f);

    // Initialize voice distribution
    setVoiceCount (currentVoiceCount);

    lfoPhase = 0.0f;
}

void ChorusEngine::reset()
{
    for (auto& voice : voices)
        voice.delayLine.reset();

    toneFilterL.reset();
    toneFilterR.reset();
    lfoPhase = 0.0f;
    crossfadeProgress = 1.0f;
}

void ChorusEngine::setVoiceCount (int newCount)
{
    newCount = juce::jlimit (1, maxVoices, newCount);

    for (size_t i = 0; i < static_cast<size_t> (newCount); ++i)
    {
        voices[i].lfoPhaseOffset = (juce::MathConstants<float>::twoPi * static_cast<float> (i))
                                   / static_cast<float> (newCount);

        if (newCount == 1)
            voices[i].panPosition = 0.5f;
        else
            voices[i].panPosition = static_cast<float> (i) / static_cast<float> (newCount - 1);
    }
}

void ChorusEngine::updateToneFilter (float toneParam)
{
    float cutoff = mapToneParamToCutoff (toneParam);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, cutoff);
    *toneFilterL.coefficients = *coeffs;
    *toneFilterR.coefficients = *coeffs;
    lastToneParam = toneParam;
}

float ChorusEngine::saturate (float sample, float drive)
{
    if (drive < 0.01f)
        return sample;

    float scaledDrive = drive * 0.5f; // Keep saturation subtle

    // Asymmetric drive for BBD character: positive half driven at 1.0x, negative at 0.9x
    float driveMultiplier = (sample >= 0.0f) ? 1.0f : 0.9f;
    float driven = sample * (1.0f + scaledDrive * driveMultiplier);
    float normalizer = std::tanh (1.0f + scaledDrive * driveMultiplier);

    return std::tanh (driven) / normalizer;
}

float ChorusEngine::mapToneParamToCutoff (float toneParam)
{
    // toneParam: -1.0 to +1.0 -> 2kHz to 20kHz (8kHz center)
    constexpr float minCutoff = 2000.0f;
    constexpr float maxCutoff = 20000.0f;
    constexpr float centerCutoff = 8000.0f;

    if (toneParam < 0.0f)
    {
        float t = toneParam + 1.0f; // 0.0 to 1.0
        return minCutoff + (centerCutoff - minCutoff) * t;
    }

    float t = toneParam; // 0.0 to 1.0
    return centerCutoff + (maxCutoff - centerCutoff) * t;
}

void ChorusEngine::process (juce::AudioBuffer<float>& buffer,
                            float rate, float depth, int numVoices, float spread,
                            float width, float tone, float mix, float drive)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels < 2)
        return;

    // Set smoothed parameter targets
    smoothedRate.setTargetValue (rate);
    smoothedDepth.setTargetValue (depth);
    smoothedSpread.setTargetValue (spread);
    smoothedWidth.setTargetValue (width);
    smoothedMix.setTargetValue (mix);
    smoothedDrive.setTargetValue (drive);
    smoothedTone.setTargetValue (tone);

    // Handle voice count change with crossfade
    numVoices = juce::jlimit (1, maxVoices, numVoices);
    if (numVoices != targetVoiceCount)
    {
        targetVoiceCount = numVoices;
        crossfadeProgress = 0.0f;
    }

    auto* leftChannel = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);

    float phaseIncrement = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Read smoothed params per sample
        float curRate = smoothedRate.getNextValue();
        float curDepth = smoothedDepth.getNextValue();
        float curSpread = smoothedSpread.getNextValue();
        float curWidth = smoothedWidth.getNextValue();
        float curMix = smoothedMix.getNextValue();
        float curDrive = smoothedDrive.getNextValue();
        float curTone = smoothedTone.getNextValue();

        // Update tone filter if changed significantly
        if (std::abs (curTone - lastToneParam) > 0.001f)
            updateToneFilter (curTone);

        // LFO phase increment
        phaseIncrement = (curRate * juce::MathConstants<float>::twoPi) / static_cast<float> (sampleRate);

        // Mono sum input for chorus processing
        float dryL = leftChannel[sample];
        float dryR = rightChannel[sample];
        float monoInput = (dryL + dryR) * 0.5f;

        // Accumulate wet signal
        float wetL = 0.0f;
        float wetR = 0.0f;

        // Process voices for current count
        auto processVoices = [&] (int count, float gain)
        {
            for (size_t v = 0; v < static_cast<size_t> (count); ++v)
            {
                auto& voice = voices[v];

                // Per-voice LFO
                float voicePhase = lfoPhase + voice.lfoPhaseOffset;
                float lfoValue = std::sin (voicePhase);

                // Per-voice base delay offset from spread parameter
                // Distributes voices symmetrically around baseDelayMs: ±spreadRangeMs
                float voiceOffset = 0.0f;
                if (count > 1)
                    voiceOffset = curSpread * spreadRangeMs
                                  * (2.0f * static_cast<float> (v) / static_cast<float> (count - 1) - 1.0f);

                // Modulated delay time with per-voice spread offset
                float effectiveDepth = curDepth * voice.depthVariation;
                float voiceBaseDelayMs = baseDelayMs + voiceOffset;
                float modulatedDelayMs = voiceBaseDelayMs + (lfoValue * effectiveDepth * delayRangeMs);
                float modulatedDelaySamples = (modulatedDelayMs / 1000.0f) * static_cast<float> (sampleRate);

                // Read from delay line
                float delayed = voice.delayLine.popSample (0, modulatedDelaySamples);

                // Write input to delay line
                voice.delayLine.pushSample (0, monoInput);

                // Saturation (after delay, before tone)
                float saturated = saturate (delayed, curDrive);

                // Equal-power stereo panning
                float effectivePan = 0.5f + (voice.panPosition - 0.5f) * curWidth;
                float panAngle = effectivePan * juce::MathConstants<float>::halfPi;
                float leftGain = std::cos (panAngle);
                float rightGain = std::sin (panAngle);

                wetL += saturated * leftGain * gain;
                wetR += saturated * rightGain * gain;
            }
        };

        // Voice crossfade logic
        if (crossfadeProgress < 1.0f)
        {
            // Blend old and new voice counts
            processVoices (currentVoiceCount, 1.0f - crossfadeProgress);
            processVoices (targetVoiceCount, crossfadeProgress);

            crossfadeProgress += crossfadeIncrement;
            if (crossfadeProgress >= 1.0f)
            {
                crossfadeProgress = 1.0f;
                currentVoiceCount = targetVoiceCount;
                setVoiceCount (currentVoiceCount);
            }
        }
        else
        {
            processVoices (currentVoiceCount, 1.0f);
        }

        // Normalize by voice count to prevent loudness increase
        float voiceScale = 1.0f / std::sqrt (static_cast<float> (currentVoiceCount));
        wetL *= voiceScale;
        wetR *= voiceScale;

        // Apply tone filter to wet signal
        wetL = toneFilterL.processSample (wetL);
        wetR = toneFilterR.processSample (wetR);

        // Mix: dry * (1 - mix) + wet * mix
        leftChannel[sample] = dryL * (1.0f - curMix) + wetL * curMix;
        rightChannel[sample] = dryR * (1.0f - curMix) + wetR * curMix;

        // Advance global LFO phase
        lfoPhase += phaseIncrement;
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;
    }
}
