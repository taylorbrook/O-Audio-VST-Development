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

    readPos += 2.0f;
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
        int delayLen = static_cast<int> (static_cast<float> (kDiffusionDelays[stage]) * srRatio) + 1;
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].resize (delayLen + 32);
            diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] = 0.0f;
        }
    }

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        int maxDelay = static_cast<int> (static_cast<float> (kBaseDelays[ch]) * srRatio * 2.0f) + 64;
        tankDelays[static_cast<size_t> (ch)].resize (maxDelay);
        tankFilters[static_cast<size_t> (ch)].clear();
        tankState[static_cast<size_t> (ch)] = 0.0f;

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

    prevSizeForDelays = -1.0f;
    prevSize = -999.0f;
    prevDamping = -999.0f;
    prevMix = -999.0f;
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
        tankState[static_cast<size_t> (ch)] = 0.0f;
    }

    for (int i = 0; i < 4; ++i)
        lfoBank[static_cast<size_t> (i)].reset();

    shimmerL.clear();
    shimmerR.clear();
    shimmerAccumL = 0.0f;
    shimmerAccumR = 0.0f;

    dryWetMixer.reset();
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

    float srRatio = currentSampleRate / 48000.0f;
    if (size != prevSizeForDelays)
    {
        float delayScale = (0.5f + size) * srRatio;
        for (int ch = 0; ch < kNumChannels; ++ch)
            scaledDelays[static_cast<size_t> (ch)] = static_cast<float> (kBaseDelays[ch]) * delayScale;
        prevSizeForDelays = size;
    }

    float feedbackGain = computeFeedbackGain (size);

    float dampCoeff = damping * 0.7f;
    for (int ch = 0; ch < kNumChannels; ++ch)
        tankFilters[static_cast<size_t> (ch)].setCoefficient (dampCoeff);

    float preDelaySamples = predelayMs * 0.001f * currentSampleRate;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float inL = leftData[i];
        float inR = rightData[i];

        if (preDelaySamples > 0.0f)
        {
            preDelayL.push (inL);
            preDelayR.push (inR);
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
                delay += mod * modDepth * kMaxModExcursion;
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
