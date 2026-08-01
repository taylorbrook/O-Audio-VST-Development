/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    BowNoiseGenerator.h
    O-Contrabass - Bow Noise Generator (Phase 2.5)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.5 implementation per RESEARCH §21.2.7 + §21.10.
    Bass-tuned 3-band BPF (700 / 1500 / 3000 Hz) on white noise +
    period-heuristic slip-burst trigger (CONTEXT 4-file scope-strict v1.0
    substitute per RESEARCH §21.3.3).
    Deterministic per-voice seed: voiceIndex * 31337 (O-Bowed pattern
    verbatim per RESEARCH §21.6).
    Header-only per O-Bowed convention (no CMake update required).

    Public API:
      - prepare(double sampleRate, int voiceIndex) noexcept
      - setNoiseLevel(float)    noexcept   [0, 1]
      - setBowEnergy(float)     noexcept   [0, 1]
      - setFundamentalHz(float) noexcept
      - processSample()         noexcept
      - reset()                 noexcept

    Output composition (per ARCHITECTURE §"Bow Noise Generator"):
      - 3 parallel bandpass filters on white noise, averaged.
      - bandSum * bowEnergy = continuous component.
      - bandSum * slipEnvelope = burst component (period-heuristic
        re-trigger every fundamental period; decays via kSlipDecayAtSr).
      - Final = (continuous + burst) * noiseLevel.

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class BowNoiseGenerator
{
public:
    inline void prepare (double sampleRate, int voiceIndex) noexcept
    {
        sr = sampleRate;
        // O-Bowed pattern verbatim — deterministic per-voice seed.
        noiseRandom.setSeed (static_cast<juce::int64> (voiceIndex * 31337));

        juce::dsp::ProcessSpec spec { sampleRate, 1u, 1u };
        for (size_t i = 0; i < kNumBpf; ++i)
        {
            bpfs[i].prepare (spec);
            bpfs[i].reset();
            *bpfs[i].coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass (
                sampleRate, kBpfFc[i], kBpfQ[i]);
        }

        // ARCHITECTURE §165 reference at 48 kHz; rescale per sample-rate.
        kSlipDecayAtSr = std::pow (kSlipDecay,
                                   static_cast<float> (48000.0 / sampleRate));

        slipPeriodSamples = 0;
        slipCounter       = 0;
        slipEnvelope      = 0.0f;
        bowEnergy         = 0.0f;
    }

    inline void setNoiseLevel (float v) noexcept
    {
        noiseLevel = juce::jlimit (0.0f, 1.0f, v);
    }

    inline void setBowEnergy (float v) noexcept
    {
        bowEnergy = juce::jlimit (0.0f, 1.0f, v);
    }

    inline void setFundamentalHz (float f0) noexcept
    {
        if (f0 < 1.0f)
        {
            slipPeriodSamples = 0;
            return;
        }
        const int newPeriod = juce::roundToInt (sr / f0);
        slipPeriodSamples = newPeriod;
        if (slipCounter > newPeriod)
            slipCounter = newPeriod;
    }

    inline float processSample() noexcept
    {
        if (noiseLevel < 0.001f)
            return 0.0f;

        // Period-heuristic slip-burst trigger
        if (slipPeriodSamples > 0 && --slipCounter <= 0)
        {
            slipEnvelope = bowEnergy;
            slipCounter  = slipPeriodSamples;
        }
        slipEnvelope *= kSlipDecayAtSr;

        // 3-band BPF on white noise, averaged
        const float white = noiseRandom.nextFloat() * 2.0f - 1.0f;
        float bandSum = 0.0f;
        for (size_t i = 0; i < kNumBpf; ++i)
            bandSum += bpfs[i].processSample (white);
        bandSum *= (1.0f / static_cast<float> (kNumBpf));

        const float continuous = bandSum * bowEnergy;
        const float burst      = bandSum * slipEnvelope;
        return (continuous + burst) * noiseLevel;
    }

    inline void reset() noexcept
    {
        for (auto& f : bpfs)
            f.reset();
        slipCounter  = 0;
        slipEnvelope = 0.0f;
        bowEnergy    = 0.0f;
    }

private:
    static constexpr int   kNumBpf = 3;
    // ARCHITECTURE §163 bass-tuned BPF center frequencies.
    static constexpr float kBpfFc[kNumBpf] = { 700.0f, 1500.0f, 3000.0f };
    // Research recommendation; tune at execute pre-flight if R38 audition
    // reveals mis-calibration per RESEARCH §21.11 FAIL-handling table.
    static constexpr float kBpfQ [kNumBpf] = { 1.0f, 1.2f, 1.5f };

    // ARCHITECTURE §165 slip-burst decay (per-sample at 48 kHz).
    static constexpr float kSlipDecay = 0.999f;

    juce::Random noiseRandom;
    std::array<juce::dsp::IIR::Filter<float>, kNumBpf> bpfs;

    // Period-heuristic slip-burst state
    int   slipPeriodSamples = 0;
    int   slipCounter       = 0;
    float slipEnvelope      = 0.0f;
    float kSlipDecayAtSr    = kSlipDecay;

    // Per-block-pushed state
    float  bowEnergy  = 0.0f;
    float  noiseLevel = 0.0f;
    double sr         = 44100.0;
};
