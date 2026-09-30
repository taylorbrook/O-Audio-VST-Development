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
    currentVoiceCount = targetVoiceCount;
    pendingVoiceCount = targetVoiceCount;
}

float ChorusEngine::layoutPhaseOffset (size_t v, int count)
{
    return (juce::MathConstants<float>::twoPi * static_cast<float> (v)) / static_cast<float> (count);
}

float ChorusEngine::layoutPan (size_t v, int count)
{
    return count == 1 ? 0.5f : static_cast<float> (v) / static_cast<float> (count - 1);
}

void ChorusEngine::updateToneFilter (float toneParam)
{
    float cutoff = mapToneParamToCutoff (toneParam);

    // Clamp cutoff below Nyquist so the bilinear-transform coefficients stay stable.
    // Without this, at sample rates <= ~40 kHz the 20 kHz max cutoff meets/exceeds
    // Nyquist, tan(pi*cutoff/fs) blows up or goes negative, and the biquad poles leave
    // the unit circle (NaN/Inf output). The ceiling is 0.45 * fs (19.8 kHz at 44.1 kHz).
    // v1.7.0 and earlier used nyquist * 0.49 = 0.245 * fs, which pinned Tone at
    // 10.8 kHz above +23% at 44.1 kHz. (v1.8.0, WR-01)
    cutoff = juce::jmin (cutoff, 0.45f * static_cast<float> (sampleRate));

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
    // Level-compensated tanh(d*x)/d. Its slope at x = 0 is exactly 1 for every d,
    // so Drive adds harmonics and compresses peaks without a level step. v1.7.0
    // used tanh((1+k)x)/tanh(1+k) behind a `drive < 0.01` bypass: +2.4 dB the
    // moment the knob left zero, then only 2 dB more across the rest of its
    // travel. (v1.8.0, WR-05)
    //
    // d = 4^drive (1 ... 4). The negative half runs at 0.9x the drive exponent
    // for the BBD-style asymmetry the old curve had.
    if (drive <= 0.0f)
        return sample;

    const float exponent = (sample >= 0.0f) ? drive : drive * 0.9f;
    const float d = std::exp2 (2.0f * exponent);
    return std::tanh (d * sample) / d;
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

    if (numSamples == 0 || numChannels < 1)
        return;

    // Set smoothed parameter targets
    smoothedRate.setTargetValue (rate);
    smoothedDepth.setTargetValue (depth);
    smoothedSpread.setTargetValue (spread);
    smoothedWidth.setTargetValue (width);
    smoothedMix.setTargetValue (mix);
    smoothedDrive.setTargetValue (drive);
    smoothedTone.setTargetValue (tone);

    // Handle voice count change with crossfade. A new count that arrives while a
    // fade is running is queued, not applied: restarting the fade would drop the
    // half-faded layer in one sample. Only the latest queued count is kept, so a
    // fast sweep costs at most one extra 50 ms fade. (v1.8.0, WR-03)
    pendingVoiceCount = juce::jlimit (1, maxVoices, numVoices);
    if (crossfadeProgress >= 1.0f && pendingVoiceCount != currentVoiceCount)
    {
        targetVoiceCount = pendingVoiceCount;
        crossfadeProgress = 0.0f;
    }

    // Mono (1 channel) runs the same voices and sums them unpanned into ch 0.
    // v1.7.0 returned early here, so a mono insert passed audio through. (v1.8.0, CR-01)
    const bool mono = (numChannels < 2);

    auto* leftChannel = buffer.getWritePointer (0);
    auto* rightChannel = mono ? nullptr : buffer.getWritePointer (1);

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
        const float phaseIncrement = (curRate * juce::MathConstants<float>::twoPi) / static_cast<float> (sampleRate);

        // Mono sum input for chorus processing
        float dryL = leftChannel[sample];
        float dryR = mono ? dryL : rightChannel[sample];
        float monoInput = (dryL + dryR) * 0.5f;

        // Determine the two voice-count "layers" to blend this sample. Normal operation is
        // a single layer (oldCount voices at unity gain). During a voice-count change the
        // old layer fades out while the new layer fades in. Each delay line must be
        // popped/pushed EXACTLY ONCE per sample no matter how many layers reference it,
        // otherwise voices shared by both layers advance their read/write pointers at 2x
        // the real sample rate for the crossfade duration.
        const bool crossfading = (crossfadeProgress < 1.0f);
        const int  oldCount    = currentVoiceCount;
        const int  newCount    = crossfading ? targetVoiceCount : 0;
        const float oldScale   = 1.0f / std::sqrt (static_cast<float> (oldCount));
        const float oldGain    = (crossfading ? (1.0f - crossfadeProgress) : 1.0f) * oldScale;
        const float newGain    = crossfading ? crossfadeProgress / std::sqrt (static_cast<float> (newCount)) : 0.0f;

        // One layer's tap for voice v: the layer's OWN phase offset, pan and spread
        // position. v1.7.0 stored a single phase/pan per voice and rewrote it when the
        // fade ended, so the incoming layer ran on the old layout and then snapped
        // (about 1.9 ms of tap jump at 4 -> 8 voices, depth 0.5). With the layout
        // computed per layer, the fade hides the difference. (v1.8.0, WR-02)
        float wetL = 0.0f, wetR = 0.0f, wetM = 0.0f;

        auto addTap = [&] (ChorusVoice& voice, size_t v, int count, float gain, bool advance)
        {
            // Spread is one-sided: voice 0 sits at the base delay and the last voice
            // at base + 15 ms. v1.7.0 spread symmetrically (base +/- 15 ms), so above
            // Spread 0.667 the outer voices went below zero and sat clamped at a static
            // 1-sample delay, an unmodulated near-dry copy. The shortest tap is now
            // 10 - 5 * 1.15 = 4.25 ms and the longest 30.75 ms, inside the 50 ms line.
            // (v1.8.0, WR-04)
            float voiceOffsetMs = 0.0f;
            if (count > 1)
                voiceOffsetMs = curSpread * spreadRangeMs * static_cast<float> (v) / static_cast<float> (count - 1);

            const float lfoValue = std::sin (lfoPhase + layoutPhaseOffset (v, count));
            const float modulatedDelayMs = baseDelayMs + voiceOffsetMs
                                         + lfoValue * curDepth * voice.depthVariation * delayRangeMs;
            const float delaySamples = juce::jlimit (1.0f, maxDelaySamplesAllocated,
                                                     modulatedDelayMs * 0.001f * static_cast<float> (sampleRate));

            const float tap = saturate (voice.delayLine.popSample (0, delaySamples, advance), curDrive) * gain;

            if (mono)
            {
                wetM += tap;
                return;
            }

            // Equal-power stereo panning
            const float effectivePan = 0.5f + (layoutPan (v, count) - 0.5f) * curWidth;
            const float panAngle = effectivePan * juce::MathConstants<float>::halfPi;
            wetL += tap * std::cos (panAngle);
            wetR += tap * std::sin (panAngle);
        };

        for (size_t v = 0; v < static_cast<size_t> (maxVoices); ++v)
        {
            auto& voice = voices[v];
            const bool inOld = (static_cast<int> (v) < oldCount);
            const bool inNew = (static_cast<int> (v) < newCount);

            // Only the final pop advances the read pointer (advance = true); an earlier
            // multi-tap pop leaves it in place, keeping read/write pointers in lockstep.
            if (inOld && inNew)
            {
                addTap (voice, v, oldCount, oldGain, false);
                addTap (voice, v, newCount, newGain, true);
            }
            else if (inOld)
            {
                addTap (voice, v, oldCount, oldGain, true);
            }
            else if (inNew)
            {
                addTap (voice, v, newCount, newGain, true);
            }
            else
            {
                // Idle voice: keep its line fed so it holds current audio, not a burst
                // of whatever it last heard, when a later voice-count change fades it in.
                voice.delayLine.popSample (0, -1.0f, true);
            }

            voice.delayLine.pushSample (0, monoInput);
        }

        // Advance the crossfade after all of this sample's voices are processed.
        if (crossfading)
        {
            crossfadeProgress += crossfadeIncrement;
            if (crossfadeProgress >= 1.0f)
            {
                crossfadeProgress = 1.0f;
                currentVoiceCount = targetVoiceCount;

                // A count queued during the fade starts its own fade now, from the
                // layer that is fully up. (WR-03)
                if (pendingVoiceCount != currentVoiceCount)
                {
                    targetVoiceCount = pendingVoiceCount;
                    crossfadeProgress = 0.0f;
                }
            }
        }

        if (mono)
        {
            const float wet = toneFilterL.processSample (wetM);
            leftChannel[sample] = dryL * (1.0f - curMix) + wet * curMix;
        }
        else
        {
            // Apply tone filter to wet signal
            wetL = toneFilterL.processSample (wetL);
            wetR = toneFilterR.processSample (wetR);

            // Mix: dry * (1 - mix) + wet * mix
            leftChannel[sample] = dryL * (1.0f - curMix) + wetL * curMix;
            rightChannel[sample] = dryR * (1.0f - curMix) + wetR * curMix;
        }

        // Advance global LFO phase
        lfoPhase += phaseIncrement;
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;
    }
}
