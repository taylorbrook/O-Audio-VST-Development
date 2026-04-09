/*
  ==============================================================================

    ReverbProcessor.cpp
    O-Lyrica - Physical Modeling Harp Synthesizer
    Ouaricon Audio

    v2.1.0: 8-channel FDN plate reverb.

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
// ShimmerShifter (dual-grain octave-up pitch shifter)
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::ShimmerShifter::prepare (float sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    grainBuffer.resize (kGrainSize * 2);
    readPos = 0.0f;
    hpPrevIn = 0.0f;
    hpPrevOut = 0.0f;

    // HP filter at ~2 kHz — keep only bright, airy content for shimmer feedback
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

    // 4 read heads at 90-degree intervals, each with Hann crossfade
    for (int h = 0; h < kNumHeads; ++h)
    {
        float headOffset = static_cast<float> (h) * headSpacing;
        float headPos = readPos + headOffset;
        if (headPos >= grainSizeF)
            headPos -= grainSizeF;

        // Delay from write position
        float delay = grainSizeF - headPos;
        if (delay < 1.0f) delay += grainSizeF;

        // Hann window based on position within the grain cycle
        float phase = headPos / grainSizeF;
        float window = 0.5f - 0.5f * std::cos (phase * juce::MathConstants<float>::twoPi);

        out += grainBuffer.read (delay) * window;
    }

    // Normalize: 4 Hann windows at 90-degree spacing sum to 2.0
    out *= 0.5f;

    // HP filter: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
    // Strips low/mid content, keeps only bright airy shimmer
    float hpOut = hpAlpha * (hpPrevOut + out - hpPrevIn);
    hpPrevIn = out;
    hpPrevOut = hpOut;
    out = hpOut;

    // Advance read position at 2x rate (octave up)
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
// Householder matrix: H = I - (2/N) * ones(N,N)
// For N=8: diagonal = 0.75, off-diagonal = -0.25
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::applyHouseholder (float* data)
{
    constexpr float scale = 2.0f / static_cast<float> (kNumChannels); // 0.25

    float sum = 0.0f;
    for (int i = 0; i < kNumChannels; ++i)
        sum += data[i];

    sum *= scale;

    for (int i = 0; i < kNumChannels; ++i)
        data[i] -= sum;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Input diffusion: 4 cascaded stages of allpass filters with Hadamard mixing
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::applyInputDiffusion (float* channels)
{
    // Polarity flip pattern per stage (pseudo-random, different per stage)
    static constexpr int polarityPatterns[kNumDiffusionStages][kNumChannels] = {
        { 1, -1,  1,  1, -1,  1, -1, -1 },
        { 1,  1, -1,  1, -1, -1,  1, -1 },
        {-1,  1,  1, -1,  1, -1, -1,  1 },
        { 1, -1, -1,  1,  1, -1,  1, -1 }
    };

    for (int stage = 0; stage < kNumDiffusionStages; ++stage)
    {
        // Allpass each channel through the diffusion delay
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float input = channels[ch];
            float delayed = diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].readNearest (kDiffusionDelays[stage]);

            // Allpass: y = delayed + coeff * (input - delayed_state)
            float v = input - kDiffusionCoeff * diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)];
            float output = diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] + kDiffusionCoeff * v;
            diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] = delayed;

            diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].push (v);
            channels[ch] = output * static_cast<float> (polarityPatterns[stage][ch]);
        }

        // Hadamard-like mixing: use Householder (close approximation, O(N) cost)
        applyHouseholder (channels);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Feedback gain from Size parameter
// ═══════════════════════════════════════════════════════════════════════════════

float ReverbProcessor::computeFeedbackGain (float size) const
{
    // Map size (0-1) to RT60 (0.2s - 12s), then compute per-sample feedback gain
    float rt60 = 0.2f + size * size * 11.8f; // quadratic mapping for better feel

    // Average delay length in samples
    float avgDelay = 0.0f;
    for (int i = 0; i < kNumChannels; ++i)
        avgDelay += scaledDelays[static_cast<size_t> (i)];
    avgDelay /= static_cast<float> (kNumChannels);

    if (avgDelay < 1.0f) avgDelay = 1.0f;

    // gain = 10^(-3 * avgDelay / (sampleRate * RT60))
    return std::pow (10.0f, -3.0f * avgDelay / (currentSampleRate * rt60));
}

// ═══════════════════════════════════════════════════════════════════════════════
// prepare / reset
// ═══════════════════════════════════════════════════════════════════════════════

void ReverbProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);
    float srRatio = currentSampleRate / 48000.0f;

    // Pre-delay (up to 200ms)
    int maxPredelay = static_cast<int> (0.2f * currentSampleRate) + 1;
    preDelayL.resize (maxPredelay);
    preDelayR.resize (maxPredelay);

    // Input diffusion delays
    for (int stage = 0; stage < kNumDiffusionStages; ++stage)
    {
        int delayLen = static_cast<int> (static_cast<float> (kDiffusionDelays[stage]) * srRatio) + 1;
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            diffusionDelays[static_cast<size_t> (stage)][static_cast<size_t> (ch)].resize (delayLen + 32);
            diffusionState[static_cast<size_t> (stage)][static_cast<size_t> (ch)] = 0.0f;
        }
    }

    // Tank delays (allow up to 2x base for size scaling + modulation excursion)
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        int maxDelay = static_cast<int> (static_cast<float> (kBaseDelays[ch]) * srRatio * 2.0f) + 64;
        tankDelays[static_cast<size_t> (ch)].resize (maxDelay);
        tankFilters[static_cast<size_t> (ch)].clear();
        tankState[static_cast<size_t> (ch)] = 0.0f;

        scaledDelays[static_cast<size_t> (ch)] = static_cast<float> (kBaseDelays[ch]) * srRatio;
    }

    // LFOs
    for (int i = 0; i < 4; ++i)
    {
        lfoBank[static_cast<size_t> (i)].setRate (kLfoRates[i], currentSampleRate);
        lfoBank[static_cast<size_t> (i)].reset();
    }

    // Shimmer
    shimmerL.prepare (currentSampleRate, static_cast<int> (spec.maximumBlockSize));
    shimmerR.prepare (currentSampleRate, static_cast<int> (spec.maximumBlockSize));
    shimmerAccumL = 0.0f;
    shimmerAccumR = 0.0f;

    // Dry/wet
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

    // Read atomic targets
    float size       = targetSize.load (std::memory_order_relaxed);
    float damping    = targetDamping.load (std::memory_order_relaxed);
    float predelayMs = targetPredelayMs.load (std::memory_order_relaxed);
    float mix        = targetMix.load (std::memory_order_relaxed);
    float modDepth   = targetMod.load (std::memory_order_relaxed);
    float shimmerAmt = targetShimmer.load (std::memory_order_relaxed);

    // Update dry/wet
    if (mix != prevMix)
    {
        dryWetMixer.setWetMixProportion (mix);
        prevMix = mix;
    }

    // Push dry signal
    dryWetMixer.pushDrySamples (block);

    // Update delay lengths when size changes
    float srRatio = currentSampleRate / 48000.0f;
    if (size != prevSizeForDelays)
    {
        // Scale delays: at size=0 use 0.5x base, at size=1 use 1.5x base
        float delayScale = (0.5f + size) * srRatio;
        for (int ch = 0; ch < kNumChannels; ++ch)
            scaledDelays[static_cast<size_t> (ch)] = static_cast<float> (kBaseDelays[ch]) * delayScale;
        prevSizeForDelays = size;
    }

    // Compute feedback gain from size
    float feedbackGain = computeFeedbackGain (size);

    // HF damping coefficient from damping parameter
    // At 0: no damping (bright plate). At 1: heavy damping (very dark).
    float dampCoeff = damping * 0.7f; // cap at 0.7 to prevent total HF kill
    for (int ch = 0; ch < kNumChannels; ++ch)
        tankFilters[static_cast<size_t> (ch)].setCoefficient (dampCoeff);

    // LF boost in feedback: slight LF decay extension for plate warmth
    // (handled implicitly — LF passes through the one-pole unattenuated)

    float preDelaySamples = predelayMs * 0.001f * currentSampleRate;

    // Per-sample processing
    for (size_t i = 0; i < numSamples; ++i)
    {
        // ── 1. Pre-delay ───────────────────────────────────────────────
        float inL = leftData[i];
        float inR = rightData[i];

        if (preDelaySamples > 0.0f)
        {
            preDelayL.push (inL);
            preDelayR.push (inR);
            inL = preDelayL.read (preDelaySamples);
            inR = preDelayR.read (preDelaySamples);
        }

        // ── 2. Mono sum for FDN input ──────────────────────────────────
        float monoIn = (inL + inR) * 0.5f;

        // Add shimmer feedback into input
        if (shimmerAmt > 0.001f)
        {
            monoIn += shimmerAccumL * shimmerAmt * 0.5f;
            monoIn += shimmerAccumR * shimmerAmt * 0.5f;
        }

        // ── 3. Distribute to 8 channels + input diffusion ──────────────
        float channels[kNumChannels];
        for (int ch = 0; ch < kNumChannels; ++ch)
            channels[ch] = monoIn;

        applyInputDiffusion (channels);

        // ── 4. Read from tank delays (with modulation) ─────────────────
        float tankOut[kNumChannels];
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float delay = scaledDelays[static_cast<size_t> (ch)];

            // Modulate channels 0,2,4,6
            if ((ch & 1) == 0)
            {
                int lfoIdx = ch >> 1;
                float mod = lfoBank[static_cast<size_t> (lfoIdx)].next();
                delay += mod * modDepth * kMaxModExcursion;
            }

            delay = std::max (delay, 1.0f);
            tankOut[ch] = tankDelays[static_cast<size_t> (ch)].read (delay);
        }

        // ── 5. Householder feedback matrix ─────────────────────────────
        applyHouseholder (tankOut);

        // ── 6. Apply feedback gain + HF damping, then write back ───────
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float fb = tankOut[ch] * feedbackGain;
            fb = tankFilters[static_cast<size_t> (ch)].process (fb);

            // Add diffused input
            float tankInput = channels[ch] + fb;

            // Soft clip to prevent runaway
            if (tankInput > 2.0f) tankInput = 2.0f;
            else if (tankInput < -2.0f) tankInput = -2.0f;

            tankDelays[static_cast<size_t> (ch)].push (tankInput);
        }

        // ── 7. Output taps: decorrelated L/R from different channels ───
        // L: channels 0,2,4,6 (even)  R: channels 1,3,5,7 (odd)
        float outL = 0.0f, outR = 0.0f;
        for (int ch = 0; ch < kNumChannels; ch += 2)
        {
            outL += tankOut[ch];
            outR += tankOut[ch + 1];
        }

        // Normalize by sqrt(N/2) for energy preservation
        constexpr float outputScale = 1.0f / 2.0f; // 1/sqrt(4) ≈ 0.5
        outL *= outputScale;
        outR *= outputScale;

        // ── 8. Shimmer: pitch-shift output and accumulate for feedback ──
        if (shimmerAmt > 0.001f)
        {
            shimmerAccumL = shimmerL.process (outL);
            shimmerAccumR = shimmerR.process (outR);
        }

        leftData[i]  = outL;
        rightData[i] = outR;
    }

    // Mix wet with dry
    dryWetMixer.mixWetSamples (block);
}
