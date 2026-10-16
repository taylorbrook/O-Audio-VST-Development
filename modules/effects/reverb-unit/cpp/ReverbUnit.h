/*
   This file is part of the Ouaricon Audio reverb-unit module.
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

    ReverbUnit - Embeddable FDN Plate Reverb Module
    Ouaricon Audio Module System v1.0.0

    8-channel FDN plate (Householder feedback, 4-stage input diffusion,
    4-LFO delay modulation, octave-up shimmer). The tank is the O-Formant
    v1.32 ReverbProcessor — the copy that carries the WR-16 per-sample size /
    pre-delay smoothing and the IN-12 sample-rate fixes — with three changes:

    - Enable and Mix ramp (50 ms). Switching off fades the wet path out and
      only then idles the tank; the plugin copies stopped processing on the
      same block and cut the tail with a step.
    - Mono output gets (L + R) / 2 of the tank. The plugin copies aliased the
      right pointer onto the left and kept only the right half.
    - Parameters are read from cached APVTS atomics (bind() once), not pushed
      through setters every block.

    Parameters (prefix + ...): enabled, size, damp, predelay (ms), mix, mod, shimmer

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

class ReverbUnit
{
public:
    static constexpr const char* PARAM_PREFIX = "rev_";
    static constexpr float MAX_PREDELAY_MS = 200.0f;
    static constexpr double RAMP_SECONDS = 0.05;

    ReverbUnit() = default;

    static void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                               const juce::String& prefix = PARAM_PREFIX)
    {
        // OFF by default, so a plugin that adopts the unit renders its existing
        // presets and sessions unchanged. Mix defaults to a usable 25 % so the
        // first click on the bypass button is audible.
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { prefix + "enabled", 1 }, "Reverb Enabled", false));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "size", 1 }, "Reverb Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "damp", 1 }, "Reverb Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "predelay", 1 }, "Reverb Pre-delay",
            juce::NormalisableRange<float> (0.0f, MAX_PREDELAY_MS, 0.0f), 20.0f, "ms"));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "mix", 1 }, "Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.25f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "mod", 1 }, "Reverb Modulation",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.2f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "shimmer", 1 }, "Reverb Shimmer",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.0f));
    }

    // Resolve the parameter atomics once (message thread, e.g. the processor ctor).
    void bind (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix = PARAM_PREFIX)
    {
        pEnabled  = apvts.getRawParameterValue (prefix + "enabled");
        pSize     = apvts.getRawParameterValue (prefix + "size");
        pDamp     = apvts.getRawParameterValue (prefix + "damp");
        pPredelay = apvts.getRawParameterValue (prefix + "predelay");
        pMix      = apvts.getRawParameterValue (prefix + "mix");
        pMod      = apvts.getRawParameterValue (prefix + "mod");
        pShimmer  = apvts.getRawParameterValue (prefix + "shimmer");
        jassert (pEnabled && pSize && pDamp && pPredelay && pMix && pMod && pShimmer);
    }

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        currentSampleRate = static_cast<float> (sampleRate);
        const float srRatio = currentSampleRate / 48000.0f;

        const int maxPredelay = static_cast<int> (MAX_PREDELAY_MS * 0.001f * currentSampleRate) + 1;
        preDelayL.resize (maxPredelay);
        preDelayR.resize (maxPredelay);

        for (int stage = 0; stage < kNumDiffusionStages; ++stage)
        {
            // IN-12 (O-Formant v1.32.1): the diffusers read the raw constant,
            // so the ring must hold it at every rate, including below 36 kHz.
            const int delayLen = std::max (static_cast<int> (static_cast<float> (kDiffusionDelays[stage]) * srRatio),
                                           kDiffusionDelays[stage]) + 1;
            for (auto& d : diffusionDelays[(size_t) stage])
                d.resize (delayLen + 32);
        }

        // IN-12: excursion is authored in 48 kHz samples.
        modExcursionSamples = kMaxModExcursion * srRatio;

        for (int ch = 0; ch < kNumChannels; ++ch)
            tankDelays[(size_t) ch].resize (static_cast<int> (static_cast<float> (kBaseDelays[ch]) * srRatio * 2.0f) + 64);

        for (int i = 0; i < 4; ++i)
            lfoBank[(size_t) i].setRate (kLfoRates[i], currentSampleRate);

        shimmerL.prepare (currentSampleRate);
        shimmerR.prepare (currentSampleRate);

        sizeSmoothed.reset (sampleRate, 0.1);
        predelaySmoothed.reset (sampleRate, 0.1);
        modSmoothed.reset (sampleRate, RAMP_SECONDS);
        shimmerSmoothed.reset (sampleRate, RAMP_SECONDS);
        mixSmoothed.reset (sampleRate, RAMP_SECONDS);
        activeGain.reset (sampleRate, RAMP_SECONDS);
        activeGain.setCurrentAndTargetValue (0.0f);   // idle until the first active block
        running = false;

        clearTank();
    }

    void reset()
    {
        activeGain.setCurrentAndTargetValue (0.0f);
        running = false;
        clearTank();
    }

    // Insert processing, in place. Mono or stereo buffers.
    void process (juce::AudioBuffer<float>& buffer)
    {
        if (pEnabled == nullptr)
            return;

        const float mixParam = pMix->load();
        const bool wantActive = pEnabled->load() >= 0.5f && mixParam > 0.001f;

        // Re-derived from the parameters every block, never latched behind a
        // dirty check: idle only once the fade-out has fully landed on zero.
        if (wantActive && ! running)
        {
            clearTank();   // never resume a stale tail
            running = true;
        }
        else if (! wantActive && running && ! activeGain.isSmoothing() && activeGain.getCurrentValue() <= 0.0f)
        {
            running = false;
        }

        if (! running)
            return;

        activeGain.setTargetValue (wantActive ? 1.0f : 0.0f);

        const float predelayTarget = juce::jlimit (0.0f, MAX_PREDELAY_MS, pPredelay->load()) * 0.001f * currentSampleRate;
        const float sizeTarget     = juce::jlimit (0.0f, 1.0f, pSize->load());
        if (snapOnNextProcess)
        {
            // First block after a clear: snap, so the tank never chirps from a stale length.
            sizeSmoothed.setCurrentAndTargetValue (sizeTarget);
            predelaySmoothed.setCurrentAndTargetValue (predelayTarget);
            modSmoothed.setCurrentAndTargetValue (pMod->load());
            shimmerSmoothed.setCurrentAndTargetValue (pShimmer->load());
            mixSmoothed.setCurrentAndTargetValue (mixParam);
            setTankSize (sizeTarget);
            snapOnNextProcess = false;
        }
        else
        {
            sizeSmoothed.setTargetValue (sizeTarget);
            predelaySmoothed.setTargetValue (predelayTarget);
            modSmoothed.setTargetValue (pMod->load());
            shimmerSmoothed.setTargetValue (pShimmer->load());
            mixSmoothed.setTargetValue (mixParam);
        }

        // IN-12: one-pole coefficient authored at 48 kHz, mapped to the running rate.
        const float dampCoeff = std::pow (juce::jlimit (0.0f, 1.0f, pDamp->load()) * 0.7f,
                                          48000.0f / currentSampleRate);
        for (auto& f : tankFilters)
            f.setCoefficient (dampCoeff);

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        if (numChannels == 0)
            return;
        float* left  = buffer.getWritePointer (0);
        float* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            if (sizeSmoothed.isSmoothing())
                setTankSize (sizeSmoothed.getNextValue());

            const float modDepth   = modSmoothed.getNextValue();
            const float shimmerAmt = shimmerSmoothed.getNextValue();
            const float wet        = mixSmoothed.getNextValue() * activeGain.getNextValue();

            const float dryL = left[i];
            const float dryR = right != nullptr ? right[i] : dryL;

            // WR-16: always write the pre-delay line; below one sample the
            // input passes straight through.
            preDelayL.push (dryL);
            preDelayR.push (dryR);
            float inL = dryL, inR = dryR;
            const float preDelaySamples = predelaySmoothed.getNextValue();
            if (preDelaySamples >= 1.0f)
            {
                inL = preDelayL.read (preDelaySamples);
                inR = preDelayR.read (preDelaySamples);
            }

            float monoIn = (inL + inR) * 0.5f;
            if (shimmerAmt > 0.001f)
                monoIn += (shimmerAccumL + shimmerAccumR) * shimmerAmt * 0.5f;

            float channels[kNumChannels];
            for (auto& c : channels)
                c = monoIn;
            applyInputDiffusion (channels);

            float tankOut[kNumChannels];
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                float delay = scaledDelays[(size_t) ch];
                if ((ch & 1) == 0)
                    delay += lfoBank[(size_t) (ch >> 1)].next() * modDepth * modExcursionSamples;
                tankOut[ch] = tankDelays[(size_t) ch].read (std::max (delay, 1.0f));
            }

            applyHouseholder (tankOut);

            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                const float fb = tankFilters[(size_t) ch].process (tankOut[ch] * tankFeedbackGain);
                tankDelays[(size_t) ch].push (juce::jlimit (-2.0f, 2.0f, channels[ch] + fb));
            }

            float outL = 0.0f, outR = 0.0f;
            for (int ch = 0; ch < kNumChannels; ch += 2)
            {
                outL += tankOut[ch];
                outR += tankOut[ch + 1];
            }
            outL *= 0.5f;
            outR *= 0.5f;

            if (shimmerAmt > 0.001f)
            {
                shimmerAccumL = shimmerL.process (outL);
                shimmerAccumR = shimmerR.process (outR);
            }
            else
            {
                shimmerAccumL = shimmerAccumR = 0.0f;   // no stale burst when shimmer comes back
            }

            // Linear dry/wet law, as juce::dsp::DryWetMixer's default in the plugin copies.
            const float dry = 1.0f - wet;
            if (right != nullptr)
            {
                left[i]  = dryL * dry + outL * wet;
                right[i] = dryR * dry + outR * wet;
            }
            else
            {
                left[i] = dryL * dry + (outL + outR) * 0.5f * wet;
            }
        }
    }

    // Nominal −60 dB tail of the current settings; 0 when the unit is off.
    // Safe to call from the message thread.
    double getTailLengthSeconds() const
    {
        if (pEnabled == nullptr || pEnabled->load() < 0.5f || pMix->load() <= 0.001f)
            return 0.0;
        const double size = juce::jlimit (0.0, 1.0, (double) pSize->load());
        return pPredelay->load() * 0.001 + rt60ForSize ((float) size);
    }

private:
    static constexpr int kNumChannels = 8;
    static constexpr int kBaseDelays[kNumChannels] = { 809, 877, 937, 1049, 1151, 1249, 1373, 1499 };
    static constexpr int kNumDiffusionStages = 4;
    static constexpr int kDiffusionDelays[kNumDiffusionStages] = { 142, 107, 79, 53 };
    static constexpr float kDiffusionCoeff = 0.625f;
    static constexpr float kLfoRates[4] = { 0.15f, 0.33f, 0.57f, 0.97f };
    static constexpr float kMaxModExcursion = 16.0f;   // at 48 kHz

    static float rt60ForSize (float size) noexcept { return 0.2f + size * size * 11.8f; }

    struct DelayLine
    {
        void resize (int maxSamples)
        {
            int size = 1;
            while (size < maxSamples + 1)
                size <<= 1;
            buffer.assign ((size_t) size, 0.0f);
            mask = size - 1;
            writePos = 0;
        }
        void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); writePos = 0; }
        void push (float s) { buffer[(size_t) writePos] = s; writePos = (writePos + 1) & mask; }
        float read (float delaySamples) const
        {
            const float readPos = static_cast<float> (writePos) - delaySamples;
            const int iPart = static_cast<int> (std::floor (readPos));
            const float frac = readPos - static_cast<float> (iPart);
            return buffer[(size_t) (iPart & mask)] * (1.0f - frac) + buffer[(size_t) ((iPart + 1) & mask)] * frac;
        }
        float readNearest (int delaySamples) const { return buffer[(size_t) ((writePos - delaySamples) & mask)]; }

    private:
        std::vector<float> buffer;
        int writePos = 0;
        int mask = 0;
    };

    struct OnePole
    {
        void setCoefficient (float c) { a = c; }
        float process (float x) { z = x + a * (z - x); return z; }
        void clear() { z = 0.0f; }
    private:
        float a = 0.0f, z = 0.0f;
    };

    struct LFO
    {
        void setRate (float hz, float sr) { delta = hz / sr; }
        float next() { phase += delta; if (phase >= 1.0f) phase -= 1.0f; return std::sin (phase * juce::MathConstants<float>::twoPi); }
        void reset() { phase = 0.0f; }
    private:
        float phase = 0.0f, delta = 0.0f;
    };

    // 4-head octave-up granular shifter, 2 kHz HP.
    struct ShimmerShifter
    {
        void prepare (float sr)
        {
            grainBuffer.resize (kGrainSize * 2);
            const float rc = 1.0f / (juce::MathConstants<float>::twoPi * 2000.0f);
            const float dt = 1.0f / sr;
            hpAlpha = rc / (rc + dt);
            clear();
        }
        float process (float input)
        {
            grainBuffer.push (input);
            constexpr float grainSizeF  = static_cast<float> (kGrainSize);
            constexpr float headSpacing = grainSizeF / static_cast<float> (kNumHeads);
            float out = 0.0f;
            for (int h = 0; h < kNumHeads; ++h)
            {
                float headPos = readPos + static_cast<float> (h) * headSpacing;
                if (headPos >= grainSizeF) headPos -= grainSizeF;
                float delay = grainSizeF - headPos;
                if (delay < 1.0f) delay += grainSizeF;
                const float window = 0.5f - 0.5f * std::cos (headPos / grainSizeF * juce::MathConstants<float>::twoPi);
                out += grainBuffer.read (delay) * window;
            }
            out *= 0.5f;
            const float hpOut = hpAlpha * (hpPrevOut + out - hpPrevIn);
            hpPrevIn = out;
            hpPrevOut = hpOut;
            readPos += 1.0f;
            if (readPos >= grainSizeF) readPos -= grainSizeF;
            return hpOut;
        }
        void clear() { grainBuffer.clear(); readPos = 0.0f; hpPrevIn = 0.0f; hpPrevOut = 0.0f; }
    private:
        static constexpr int kGrainSize = 4096;
        static constexpr int kNumHeads = 4;
        DelayLine grainBuffer;
        float readPos = 0.0f, hpPrevIn = 0.0f, hpPrevOut = 0.0f, hpAlpha = 0.0f;
    };

    void clearTank()
    {
        preDelayL.clear();
        preDelayR.clear();
        for (int stage = 0; stage < kNumDiffusionStages; ++stage)
        {
            for (auto& d : diffusionDelays[(size_t) stage]) d.clear();
            diffusionState[(size_t) stage].fill (0.0f);
        }
        for (auto& d : tankDelays) d.clear();
        for (auto& f : tankFilters) f.clear();
        for (auto& l : lfoBank) l.reset();
        shimmerL.clear();
        shimmerR.clear();
        shimmerAccumL = shimmerAccumR = 0.0f;
        snapOnNextProcess = true;
    }

    void setTankSize (float size) noexcept
    {
        const float delayScale = (0.5f + size) * (currentSampleRate / 48000.0f);
        float avgDelay = 0.0f;
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            scaledDelays[(size_t) ch] = static_cast<float> (kBaseDelays[ch]) * delayScale;
            avgDelay += scaledDelays[(size_t) ch];
        }
        avgDelay = std::max (avgDelay / static_cast<float> (kNumChannels), 1.0f);
        tankFeedbackGain = std::pow (10.0f, -3.0f * avgDelay / (currentSampleRate * rt60ForSize (size)));
    }

    static void applyHouseholder (float* data) noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < kNumChannels; ++i) sum += data[i];
        sum *= 2.0f / static_cast<float> (kNumChannels);
        for (int i = 0; i < kNumChannels; ++i) data[i] -= sum;
    }

    void applyInputDiffusion (float* channels) noexcept
    {
        static constexpr int polarity[kNumDiffusionStages][kNumChannels] = {
            { 1, -1,  1,  1, -1,  1, -1, -1 },
            { 1,  1, -1,  1, -1, -1,  1, -1 },
            {-1,  1,  1, -1,  1, -1, -1,  1 },
            { 1, -1, -1,  1,  1, -1,  1, -1 }
        };

        for (int stage = 0; stage < kNumDiffusionStages; ++stage)
        {
            auto& delays = diffusionDelays[(size_t) stage];
            auto& state  = diffusionState[(size_t) stage];
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                // IN-12: diffusion time fixed in samples (colour-preserving); prepare() sizes the ring.
                const float delayed = delays[(size_t) ch].readNearest (kDiffusionDelays[stage]);
                const float v = channels[ch] - kDiffusionCoeff * state[(size_t) ch];
                const float output = state[(size_t) ch] + kDiffusionCoeff * v;
                state[(size_t) ch] = delayed;
                delays[(size_t) ch].push (v);
                channels[ch] = output * static_cast<float> (polarity[stage][ch]);
            }
            applyHouseholder (channels);
        }
    }

    // Parameters
    std::atomic<float>* pEnabled  = nullptr;
    std::atomic<float>* pSize     = nullptr;
    std::atomic<float>* pDamp     = nullptr;
    std::atomic<float>* pPredelay = nullptr;
    std::atomic<float>* pMix      = nullptr;
    std::atomic<float>* pMod      = nullptr;
    std::atomic<float>* pShimmer  = nullptr;

    float currentSampleRate = 44100.0f;
    float modExcursionSamples = kMaxModExcursion;

    DelayLine preDelayL, preDelayR;
    std::array<std::array<DelayLine, kNumChannels>, kNumDiffusionStages> diffusionDelays;
    std::array<std::array<float, kNumChannels>, kNumDiffusionStages> diffusionState {};
    std::array<DelayLine, kNumChannels> tankDelays;
    std::array<OnePole, kNumChannels> tankFilters;
    std::array<LFO, 4> lfoBank;
    ShimmerShifter shimmerL, shimmerR;
    float shimmerAccumL = 0.0f, shimmerAccumR = 0.0f;

    std::array<float, kNumChannels> scaledDelays {};
    float tankFeedbackGain = 0.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sizeSmoothed, predelaySmoothed,
        modSmoothed, shimmerSmoothed, mixSmoothed, activeGain;
    bool snapOnNextProcess = true;
    bool running = false;
};
