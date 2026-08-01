/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
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

    BodyResonance.cpp
    Phase 2.4: Convolution-based body resonance implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BodyResonance.h"

BodyResonance::BodyResonance()
{
}

void BodyResonance::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Generate synthetic IR at current sample rate
    generateSyntheticIR();

    // Prepare convolution
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;  // Stereo

    convolution.prepare(spec);

    // Load synthetic IR into convolution engine
    // Clone mono IR to stereo for convolution
    juce::AudioBuffer<float> stereoIR(2, irBuffer.getNumSamples());
    stereoIR.copyFrom(0, 0, irBuffer, 0, 0, irBuffer.getNumSamples());
    stereoIR.copyFrom(1, 0, irBuffer, 0, 0, irBuffer.getNumSamples());

    convolution.loadImpulseResponse(
        std::move(stereoIR),
        currentSampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::yes
    );

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);
    dryWetMixer.setWetMixProportion(mix);

    // WR-10: delay the dry path to match the convolution's reported latency so dry + wet
    // stay phase-aligned (otherwise partial mixes comb-filter). The default-constructed
    // juce::dsp::Convolution runs the zero-latency algorithm, so getLatency() is 0 today
    // and this is a no-op — but it keeps the mix correct if the engine ever reports latency.
    dryWetMixer.setWetLatency(static_cast<float>(convolution.getLatency()));
}

void BodyResonance::reset()
{
    convolution.reset();
    dryWetMixer.reset();
}

void BodyResonance::process(juce::AudioBuffer<float>& buffer)
{
    // Push dry signal
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);

    // Process through convolution
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);

    // Mix dry/wet
    dryWetMixer.mixWetSamples(block);
}

void BodyResonance::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
    dryWetMixer.setWetMixProportion(mix);
}

void BodyResonance::generateSyntheticIR()
{
    // v1.5.0: Enhanced body resonance with wood-like characteristics
    // Generate ~100ms IR for richer resonance (was 75ms)
    const int irLength = static_cast<int>(currentSampleRate * 0.100);
    irBuffer.setSize(1, irLength);  // Mono IR
    irBuffer.clear();

    auto* ir = irBuffer.getWritePointer(0);

    // Marimba resonator tube characteristics:
    // - Quick initial transient (wood impact character)
    // - Rich low-mid resonance (200-600 Hz emphasis for warmth)
    // - Multi-stage decay: fast initial, slow tail (wood-like)
    // - Subtle diffuse reflections for spatial depth

    juce::Random random;
    const float twoPi = juce::MathConstants<float>::twoPi;

    // Resonant mode frequencies (Hz) - based on typical marimba resonator tubes
    // Lower frequencies for warmth, spread across low-mid range
    struct ResonantMode {
        float freq;
        float amplitude;
        float decayRate;  // Higher = faster decay
    };

    // 6 resonant modes for richer, more realistic body character
    const std::array<ResonantMode, 6> modes = {{
        { 180.0f, 0.50f, 4.0f },   // Sub-bass warmth
        { 280.0f, 0.70f, 5.0f },   // Primary low resonance (strongest)
        { 420.0f, 0.45f, 6.0f },   // Mid-low body
        { 580.0f, 0.30f, 7.0f },   // Mid presence
        { 750.0f, 0.18f, 9.0f },   // Upper mid character
        { 1100.0f, 0.08f, 12.0f }  // High frequency air/shimmer
    }};

    for (int i = 0; i < irLength; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(irLength);
        float tSeconds = static_cast<float>(i) / static_cast<float>(currentSampleRate);

        float sample = 0.0f;

        // Sum all resonant modes with individual decay rates
        for (const auto& mode : modes)
        {
            // Each mode has its own decay envelope (higher modes decay faster)
            float modeEnvelope = std::exp(-tSeconds * mode.decayRate);

            // Add slight random phase offset per mode for more natural sound
            float phase = twoPi * mode.freq * tSeconds;
            sample += mode.amplitude * modeEnvelope * std::sin(phase);
        }

        // Wood-like multi-stage decay envelope
        // Stage 1: Quick initial attack (first 5ms)
        // Stage 2: Fast initial decay (5-30ms)
        // Stage 3: Slow sustain tail (30-100ms)
        float envelope;
        if (t < 0.05f)
        {
            // Initial transient - quick attack
            envelope = t / 0.05f;
        }
        else if (t < 0.3f)
        {
            // Fast decay phase (wood characteristic)
            float decayT = (t - 0.05f) / 0.25f;
            envelope = 1.0f - (decayT * 0.6f);  // Drops to 40% by t=0.3
        }
        else
        {
            // Slow tail (resonant sustain)
            float tailT = (t - 0.3f) / 0.7f;
            envelope = 0.4f * std::exp(-tailT * 2.5f);
        }

        // Add subtle early reflections (wood diffusion character)
        // Small amount of filtered noise in first 20ms
        if (t < 0.2f)
        {
            float noiseEnv = (1.0f - t / 0.2f) * 0.08f;
            sample += noiseEnv * (random.nextFloat() * 2.0f - 1.0f);
        }

        ir[i] = sample * envelope;
    }

    // Normalize IR with headroom
    float maxVal = irBuffer.getMagnitude(0, 0, irBuffer.getNumSamples());
    if (maxVal > 0.0f)
        irBuffer.applyGain(0, 0, irBuffer.getNumSamples(), 0.45f / maxVal);
}
