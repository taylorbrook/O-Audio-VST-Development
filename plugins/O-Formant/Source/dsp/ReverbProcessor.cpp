/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    ReverbProcessor.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

    8-channel FDN plate reverb.

  ==============================================================================
*/

#include "ReverbProcessor.h"

// ═══════════════════════════════════════════════════════════════════════════════
// DelayLine
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::DelayLine::resize (int maxSamples)
{
    int size = 1;
    while (size < maxSamples + 1)
        size <<= 1;
    buffer.assign (static_cast<size_t> (size), 0.0f);
    mask = size - 1;
    writePos = 0;
}

void ReverbProcessor::DelayLine::clear()
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
}

void ReverbProcessor::DelayLine::push (float sample)
{
    buffer[static_cast<size_t> (writePos)] = sample;
    writePos = (writePos + 1) & mask;
}

float ReverbProcessor::DelayLine::read (float delaySamples) const
{
    float readPos = static_cast<float> (writePos) - delaySamples;
    int iPart = static_cast<int> (std::floor (readPos));
    float frac = readPos - static_cast<float> (iPart);
    int i0 = iPart & mask;
    int i1 = (iPart + 1) & mask;
    return buffer[static_cast<size_t> (i0)] * (1.0f - frac) + buffer[static_cast<size_t> (i1)] * frac;
}

float ReverbProcessor::DelayLine::readNearest (int delaySamples) const
{
    int pos = (writePos - delaySamples) & mask;
    return buffer[static_cast<size_t> (pos)];
}

// ═══════════════════════════════════════════════════════════════════════════════
// ShimmerShifter
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::ShimmerShifter::prepare (float sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    grainBuffer.resize (kGrainSize * 2);
    readPos = 0.0f;
    hpPrevIn = 0.0f;
    hpPrevOut = 0.0f;

    float cutoff = 2000.0f;
    float rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoff);
    float dt = 1.0f / sr;
    hpAlpha = rc / (rc + dt);
}

float ReverbProcessor::ShimmerShifter::process (float input)
{
    grainBuffer.push (input);

    constexpr float grainSizeF = static_cast<float> (kGrainSize);
    constexpr float headSpacing = grainSizeF / static_cast<float> (kNumHeads);

    float out = 0.0f;

    for (int h = 0; h < kNumHeads; ++h)
    {
        float headOffset = static_cast<float> (h) * headSpacing;
        float headPos = readPos + headOffset;
        if (headPos >= grainSizeF)
            headPos -= grainSizeF;

        float delay = grainSizeF - headPos;
        if (delay < 1.0f) delay += grainSizeF;

        float phase = headPos / grainSizeF;
        float window = 0.5f - 0.5f * std::cos (phase * juce::MathConstants<float>::twoPi);

        out += grainBuffer.read (delay) * window;
    }

    out *= 0.5f;

    float hpOut = hpAlpha * (hpPrevOut + out - hpPrevIn);
    hpPrevIn = out;
    hpPrevOut = hpOut;
    out = hpOut;

    readPos += 1.0f;
    if (readPos >= grainSizeF)
        readPos -= grainSizeF;

    return out;
}

void ReverbProcessor::ShimmerShifter::clear()
{
    grainBuffer.clear();
    readPos = 0.0f;
    hpPrevIn = 0.0f;
    hpPrevOut = 0.0f;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Householder matrix
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::applyHouseholder (float* data)
{
    constexpr float scale = 2.0f / static_cast<float> (kNumChannels);

    float sum = 0.0f;
    for (int i = 0; i < kNumChannels; ++i)
        sum += data[i];

    sum *= scale;

    for (int i = 0; i < kNumChannels; ++i)
        data[i] -= sum;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Input diffusion
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::applyInputDiffusion (float* channels)
{
    static constexpr int polarityPatterns[kNumDiffusionStages][kNumChannels] = {
        { 1, -1,  1,  1, -1,  1, -1, -1 },
        { 1,  1, -1,  1, -1, -1,  1, -1 },
        {-1,  1,  1, -1,  1, -1, -1,  1 },
        { 1, -1, -1,  1,  1, -1,  1, -1 }
    };

    for (int stage = 0; stage < kNumDiffusionStages; ++stage)
    {
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float input = channels[ch];
            // IN-12: diffusion time is fixed in samples, not seconds (unlike
            // the tank, which uses scaledDelays). Kept deliberately — scaling
            // would change the colour at 44.1 and 96 kHz. prepare() sizes the
            // buffer to hold the raw constant at every rate, so it never wraps.
            float delayed = diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].readNearest (kDiffusionDelays[stage]);

            float v = input - kDiffusionCoeff * diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)];
            float output = diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] + kDiffusionCoeff * v;
            diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] = delayed;

            diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].push (v);
            channels[ch] = output * static_cast<float> (polarityPatterns[stage][ch]);
        }

        applyHouseholder (channels);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Feedback gain
// ═══════════════════════════════════════════════════════════════════════════════

float ReverbProcessor::computeFeedbackGain (float size) const
{
    float rt60 = 0.2f + size * size * 11.8f;

    float avgDelay = 0.0f;
    for (int i = 0; i < kNumChannels; ++i)
        avgDelay += scaledDelays[static_cast<size_t> (i)];
    avgDelay /= static_cast<float> (kNumChannels);

    if (avgDelay < 1.0f) avgDelay = 1.0f;

    return std::pow (10.0f, -3.0f * avgDelay / (currentSampleRate * rt60));
}

// ═══════════════════════════════════════════════════════════════════════════════
// prepare / reset
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);
    float srRatio = currentSampleRate / 48000.0f;

    int maxPredelay = static_cast<int> (0.2f * currentSampleRate) + 1;
    preDelayL.resize (maxPredelay);
    preDelayR.resize (maxPredelay);

    for (int stage = 0; stage < kNumDiffusionStages; ++stage)
    {
        // v1.32.1 (review IN-12): process() reads the raw kDiffusionDelays
        // constant, so the buffer must hold it at every rate. Sized from the
        // scaled length alone, stage 0 (142) got a 128-sample ring below
        // ~36 kHz and the read wrapped to 14 samples (stages 1-2 also wrapped
        // at <=16 kHz). Sizes at 44.1-192 kHz are unchanged.
        int delayLen = std::max (static_cast<int> (static_cast<float> (kDiffusionDelays[stage]) * srRatio),
                                 kDiffusionDelays[stage]) + 1;
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].resize (delayLen + 32);
            diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] = 0.0f;
        }
    }

    // IN-12: kMaxModExcursion is in 48 kHz samples; scale it like the delay
    // lengths so the chorus depth in ms doesn't shrink at high rates. The
    // tank's worst read is base·1.5·srRatio + 16·srRatio, well inside the
    // base·2·srRatio + 64 buffer (the smallest base, 809, leaves 404·srRatio).
    modExcursionSamples = kMaxModExcursion * srRatio;

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        int maxDelay = static_cast<int> (static_cast<float> (kBaseDelays[ch]) * srRatio * 2.0f) + 64;
        tankDelays[static_cast<size_t> (ch)].resize (maxDelay);
        tankFilters[static_cast<size_t> (ch)].clear();

        scaledDelays[static_cast<size_t> (ch)] = static_cast<float> (kBaseDelays[ch]) * srRatio;
    }

    for (int i = 0; i < 4; ++i)
    {
        lfoBank[static_cast<size_t> (i)].setRate (kLfoRates[i], currentSampleRate);
        lfoBank[static_cast<size_t> (i)].reset();
    }

    shimmerL.prepare (currentSampleRate, static_cast<int> (spec.maximumBlockSize));
    shimmerR.prepare (currentSampleRate, static_cast<int> (spec.maximumBlockSize));
    shimmerAccumL = 0.0f;
    shimmerAccumR = 0.0f;

    dryWetMixer.prepare (spec);

    // WR-16: size (all 8 tank lengths + loop gain) and pre-delay glide per
    // sample instead of stepping at block rate, which clicked on every move.
    sizeSmoothed.reset (spec.sampleRate, 0.1);
    sizeSmoothed.setCurrentAndTargetValue (0.5f);
    predelaySmoothed.reset (spec.sampleRate, 0.1);
    predelaySmoothed.setCurrentAndTargetValue (0.0f);
    setTankSize (0.5f);
    snapSmoothersOnNextProcess = true;

    prevMix = -999.0f;
}

void ReverbProcessor::setTankSize (float size) noexcept
{
    const float delayScale = (0.5f + size) * (currentSampleRate / 48000.0f);
    for (int ch = 0; ch < kNumChannels; ++ch)
        scaledDelays[static_cast<size_t> (ch)] = static_cast<float> (kBaseDelays[ch]) * delayScale;
    tankFeedbackGain = computeFeedbackGain (size);
}

void ReverbProcessor::reset()
{
    preDelayL.clear();
    preDelayR.clear();

    for (int stage = 0; stage < kNumDiffusionStages; ++stage)
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].clear();
            diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] = 0.0f;
        }

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        tankDelays[static_cast<size_t> (ch)].clear();
        tankFilters[static_cast<size_t> (ch)].clear();
    }

    for (int i = 0; i < 4; ++i)
        lfoBank[static_cast<size_t> (i)].reset();

    shimmerL.clear();
    shimmerR.clear();
    shimmerAccumL = 0.0f;
    shimmerAccumR = 0.0f;

    dryWetMixer.reset();

    // v1.30.1: the smoothers kept their pre-bypass (or prepare()-seeded 0.5 /
    // 0 ms) value, so the first block glided from it and chirped the tank.
    snapSmoothersOnNextProcess = true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Setters
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::setSize (float size)      { targetSize.store (size, std::memory_order_relaxed); }
void ReverbProcessor::setDamping (float damp)   { targetDamping.store (damp, std::memory_order_relaxed); }
void ReverbProcessor::setPredelay (float ms)    { targetPredelayMs.store (ms, std::memory_order_relaxed); }
void ReverbProcessor::setMix (float mix)        { targetMix.store (mix, std::memory_order_relaxed); }
void ReverbProcessor::setMod (float mod)        { targetMod.store (mod, std::memory_order_relaxed); }
void ReverbProcessor::setShimmer (float shimmer) { targetShimmer.store (shimmer, std::memory_order_relaxed); }

// ═══════════════════════════════════════════════════════════════════════════════
// process
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    auto numSamples = block.getNumSamples();
    auto* leftData  = block.getChannelPointer (0);
    auto* rightData = block.getNumChannels() > 1 ? block.getChannelPointer (1) : leftData;

    float size       = targetSize.load (std::memory_order_relaxed);
    float damping    = targetDamping.load (std::memory_order_relaxed);
    float predelayMs = targetPredelayMs.load (std::memory_order_relaxed);
    float mix        = targetMix.load (std::memory_order_relaxed);
    float modDepth   = targetMod.load (std::memory_order_relaxed);
    float shimmerAmt = targetShimmer.load (std::memory_order_relaxed);

    if (mix != prevMix)
    {
        dryWetMixer.setWetMixProportion (mix);
        prevMix = mix;
    }

    dryWetMixer.pushDrySamples (block);

    const float predelaySamplesTarget = predelayMs * 0.001f * currentSampleRate;
    if (snapSmoothersOnNextProcess)
    {
        sizeSmoothed.setCurrentAndTargetValue (size);
        predelaySmoothed.setCurrentAndTargetValue (predelaySamplesTarget);
        setTankSize (size);
        snapSmoothersOnNextProcess = false;
    }
    else
    {
        sizeSmoothed.setTargetValue (size);
        predelaySmoothed.setTargetValue (predelaySamplesTarget);
    }

    // v1.31.2 (review IN-12): the one-pole damping coefficient is per sample,
    // so a fixed value darkened the tail more at high rates. It is authored at
    // the 48 kHz reference every delay length here uses (srRatio) and mapped to
    // the running rate as a^(48000/sr) — identical at 48 kHz.
    float dampCoeff = std::pow (damping * 0.7f, 48000.0f / currentSampleRate);
    for (int ch = 0; ch < kNumChannels; ++ch)
        tankFilters[static_cast<size_t> (ch)].setCoefficient (dampCoeff);

    for (size_t i = 0; i < numSamples; ++i)
    {
        if (sizeSmoothed.isSmoothing())
            setTankSize (sizeSmoothed.getNextValue());
        const float feedbackGain = tankFeedbackGain;

        float inL = leftData[i];
        float inR = rightData[i];

        // WR-16: always write the pre-delay line, so raising pre-delay from 0
        // reads recent input instead of whatever was left from the last time
        // it was on. read(1) after push is the current sample, so below one
        // sample of delay the input passes straight through (continuous).
        preDelayL.push (inL);
        preDelayR.push (inR);
        const float preDelaySamples = predelaySmoothed.getNextValue();
        if (preDelaySamples >= 1.0f)
        {
            inL = preDelayL.read (preDelaySamples);
            inR = preDelayR.read (preDelaySamples);
        }

        float monoIn = (inL + inR) * 0.5f;

        if (shimmerAmt > 0.001f)
        {
            monoIn += shimmerAccumL * shimmerAmt * 0.5f;
            monoIn += shimmerAccumR * shimmerAmt * 0.5f;
        }

        float channels[kNumChannels];
        for (int ch = 0; ch < kNumChannels; ++ch)
            channels[ch] = monoIn;

        applyInputDiffusion (channels);

        float tankOut[kNumChannels];
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float delay = scaledDelays[static_cast<size_t> (ch)];

            if ((ch & 1) == 0)
            {
                int lfoIdx = ch >> 1;
                float mod = lfoBank[static_cast<size_t> (lfoIdx)].next();
                delay += mod * modDepth * modExcursionSamples;
            }

            delay = std::max (delay, 1.0f);
            tankOut[ch] = tankDelays[static_cast<size_t> (ch)].read (delay);
        }

        applyHouseholder (tankOut);

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float fb = tankOut[ch] * feedbackGain;
            fb = tankFilters[static_cast<size_t> (ch)].process (fb);

            float tankInput = channels[ch] + fb;

            if (tankInput > 2.0f) tankInput = 2.0f;
            else if (tankInput < -2.0f) tankInput = -2.0f;

            tankDelays[static_cast<size_t> (ch)].push (tankInput);
        }

        float outL = 0.0f, outR = 0.0f;
        for (int ch = 0; ch < kNumChannels; ch += 2)
        {
            outL += tankOut[ch];
            outR += tankOut[ch + 1];
        }

        constexpr float outputScale = 1.0f / 2.0f;
        outL *= outputScale;
        outR *= outputScale;

        if (shimmerAmt > 0.001f)
        {
            shimmerAccumL = shimmerL.process (outL);
            shimmerAccumR = shimmerR.process (outR);
        }

        leftData[i]  = outL;
        rightData[i] = outR;
    }

    dryWetMixer.mixWetSamples (block);
}
