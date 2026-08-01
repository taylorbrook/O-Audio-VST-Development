/*
   This file is part of O-Chorus, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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

    auto maxDelaySamples = static_cast<int> (sampleRate * maxDelayMs / 1000.0);
    maxDelaySamplesAllocated = static_cast<float> (maxDelaySamples);

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

    // Tone filters — pre-allocate coefficient arrays (non-RT, allocation OK here)
    auto initCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 8000.0f);
    *toneFilterL.coefficients = *initCoeffs;
    *toneFilterR.coefficients = *initCoeffs;
    toneFilterL.reset();
    toneFilterR.reset();
    lastToneParam = 0.0f;

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

    // Clamp cutoff below Nyquist so the bilinear-transform coefficients stay stable.
    // Without this, at sample rates <= ~40 kHz the 20 kHz max cutoff meets/exceeds
    // Nyquist, tan(pi*cutoff/fs) blows up or goes negative, and the biquad poles leave
    // the unit circle (NaN/Inf output). (WR-03)
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;
    cutoff = juce::jmin (cutoff, nyquist * 0.49f);

    // Butterworth LPF coefficients computed directly (RT-safe, no heap allocation)
    // Replicates JUCE makeLowPass with Q = 1/sqrt(2)
    float n = 1.0f / std::tan (juce::MathConstants<float>::pi * cutoff / static_cast<float> (sampleRate));
    float nSq = n * n;
    float invQ = juce::MathConstants<float>::sqrt2;
    float c1 = 1.0f / (1.0f + invQ * n + nSq);

    float b0 = c1;
    float b1 = c1 * 2.0f;
    float a1 = c1 * 2.0f * (1.0f - nSq);
    float a2 = c1 * (1.0f - invQ * n + nSq);

    auto* cL = toneFilterL.coefficients->getRawCoefficients();
    cL[0] = b0;  cL[1] = b1;  cL[2] = b0;  cL[3] = a1;  cL[4] = a2;

    auto* cR = toneFilterR.coefficients->getRawCoefficients();
    cR[0] = b0;  cR[1] = b1;  cR[2] = b0;  cR[3] = a1;  cR[4] = a2;

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

        // Determine the two voice-count "layers" to blend this sample. Normal operation is
        // a single layer (oldCount voices at unity gain). During a voice-count change the
        // old layer fades out while the new layer fades in. Each delay line must be
        // popped/pushed EXACTLY ONCE per sample no matter how many layers reference it,
        // otherwise voices shared by both layers advance their read/write pointers at 2x
        // the real sample rate for the crossfade duration. (WR-02)
        const bool crossfading = (crossfadeProgress < 1.0f);
        const int  oldCount    = currentVoiceCount;
        const int  newCount    = crossfading ? targetVoiceCount : 0;
        const float oldGain    = crossfading ? (1.0f - crossfadeProgress) : 1.0f;
        const float newGain    = crossfading ? crossfadeProgress : 0.0f;
        const int  activeCount = juce::jmax (oldCount, newCount);

        // Per-voice modulated delay in samples for a given layer voice-count, clamped to a
        // valid positive range. At high Spread the raw per-voice delay can go negative
        // (e.g. voice 0 at Spread 1.0 → base 10ms − spread 15ms), which the JUCE DelayLine
        // silently pins to its last-set clamped value, collapsing that voice toward ~0ms
        // and killing symmetric modulation. Clamping keeps every voice modulating. (WR-01)
        auto delaySamplesForCount = [&] (size_t v, int count, float lfoValue, float effectiveDepth)
        {
            float voiceOffset = 0.0f;
            if (count > 1)
                voiceOffset = curSpread * spreadRangeMs
                              * (2.0f * static_cast<float> (v) / static_cast<float> (count - 1) - 1.0f);

            float voiceBaseDelayMs = baseDelayMs + voiceOffset;
            float modulatedDelayMs = voiceBaseDelayMs + (lfoValue * effectiveDepth * delayRangeMs);
            float modulatedDelaySamples = (modulatedDelayMs / 1000.0f) * static_cast<float> (sampleRate);
            return juce::jlimit (1.0f, maxDelaySamplesAllocated, modulatedDelaySamples);
        };

        for (size_t v = 0; v < static_cast<size_t> (activeCount); ++v)
        {
            auto& voice = voices[v];

            // Per-voice LFO and pan are independent of the layer voice-count.
            float voicePhase = lfoPhase + voice.lfoPhaseOffset;
            float lfoValue = std::sin (voicePhase);
            float effectiveDepth = curDepth * voice.depthVariation;

            // Equal-power stereo panning (shared by both layers for this voice)
            float effectivePan = 0.5f + (voice.panPosition - 0.5f) * curWidth;
            float panAngle = effectivePan * juce::MathConstants<float>::halfPi;
            float leftGain = std::cos (panAngle);
            float rightGain = std::sin (panAngle);

            const bool inOld = (static_cast<int> (v) < oldCount);
            const bool inNew = (static_cast<int> (v) < newCount);

            // Read whichever layer(s) this voice belongs to, then push the input exactly
            // once. Only the final pop advances the read pointer (updateReadPointer=true);
            // an earlier multi-tap pop leaves it in place. This keeps read/write pointers
            // in lockstep at 1x the real sample rate even for voices shared by both layers.
            if (inOld && inNew)
            {
                float oldDelayed = voice.delayLine.popSample (0, delaySamplesForCount (v, oldCount, lfoValue, effectiveDepth), false);
                float newDelayed = voice.delayLine.popSample (0, delaySamplesForCount (v, newCount, lfoValue, effectiveDepth), true);
                voice.delayLine.pushSample (0, monoInput);

                float blended = saturate (oldDelayed, curDrive) * oldGain
                              + saturate (newDelayed, curDrive) * newGain;
                wetL += blended * leftGain;
                wetR += blended * rightGain;
            }
            else if (inOld)
            {
                float delayed = voice.delayLine.popSample (0, delaySamplesForCount (v, oldCount, lfoValue, effectiveDepth), true);
                voice.delayLine.pushSample (0, monoInput);

                float saturated = saturate (delayed, curDrive) * oldGain;
                wetL += saturated * leftGain;
                wetR += saturated * rightGain;
            }
            else // inNew only
            {
                float delayed = voice.delayLine.popSample (0, delaySamplesForCount (v, newCount, lfoValue, effectiveDepth), true);
                voice.delayLine.pushSample (0, monoInput);

                float saturated = saturate (delayed, curDrive) * newGain;
                wetL += saturated * leftGain;
                wetR += saturated * rightGain;
            }
        }

        // Advance the crossfade after all of this sample's voices are processed, so the
        // voiceScale interpolation below sees the same progress the original code did.
        if (crossfading)
        {
            crossfadeProgress += crossfadeIncrement;
            if (crossfadeProgress >= 1.0f)
            {
                crossfadeProgress = 1.0f;
                currentVoiceCount = targetVoiceCount;
                setVoiceCount (currentVoiceCount);
            }
        }

        // Normalize by voice count (interpolate during crossfade to prevent volume bump)
        float voiceScale;
        if (crossfadeProgress < 1.0f)
        {
            float oldScale = 1.0f / std::sqrt (static_cast<float> (currentVoiceCount));
            float newScale = 1.0f / std::sqrt (static_cast<float> (targetVoiceCount));
            voiceScale = oldScale + (newScale - oldScale) * crossfadeProgress;
        }
        else
        {
            voiceScale = 1.0f / std::sqrt (static_cast<float> (currentVoiceCount));
        }
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
