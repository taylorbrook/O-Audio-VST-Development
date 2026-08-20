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

    Output composition (per ARCHITECTURE §"Bow Noise Generator", revised v1.5.0):
      - 3 parallel bandpass filters on white noise, averaged.
      - bandSum → pitch-synchronous feedback comb tuned to the fundamental
        (damped loop), mixed kCombMix wet against the un-combed bandSum.
        Pitches the hiss at harmonics of f0 like a real bowed string.
      - pitched * bowEnergy = continuous component.
      - pitched * slipEnvelope = burst component. Slip re-trigger period is
        jittered ±kSlipPeriodJitter and burst amplitude ±30% per event
        (separate RNG stream from the white noise — one stream per phase
        preserves block-size invariance).
      - Final = (continuous + burst) * noiseLevel.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class BowNoiseGenerator
{
public:
    // Not noexcept: allocates the comb delay buffer (prepare-time only).
    inline void prepare (double sampleRate, int voiceIndex)
    {
        sr = sampleRate;
        // O-Bowed pattern verbatim — deterministic per-voice seed.
        noiseRandom.setSeed (static_cast<juce::int64> (voiceIndex * 31337));
        // Separate stream for slip jitter draws (event-rate, sample-position
        // determined) so interleaving never perturbs the white-noise sequence.
        jitterRandom.setSeed (static_cast<juce::int64> (voiceIndex * 31337 + 7919));

        // Comb sized for the lowest trackable fundamental (kCombMinF0).
        combBuffer.assign (static_cast<size_t> (std::ceil (sampleRate / kCombMinF0)) + 1, 0.0f);
        combWrite     = 0;
        combPeriod    = 0;
        combDampState = 0.0f;
        // One-pole LP in the comb loop, ~4 kHz corner: keeps the pitched
        // hiss from ringing metallic at high harmonics.
        combDampCoeff = 1.0f - std::exp (static_cast<float> (-juce::MathConstants<double>::twoPi
                                                             * 4000.0 / sampleRate));

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
            combPeriod        = 0;
            return;
        }
        const int newPeriod = juce::roundToInt (sr / f0);
        slipPeriodSamples = newPeriod;
        if (slipCounter > newPeriod)
            slipCounter = newPeriod;

        combPeriod = combBuffer.empty() ? 0
                   : juce::jlimit (2, static_cast<int> (combBuffer.size()) - 1, newPeriod);
    }

    inline float processSample() noexcept
    {
        if (noiseLevel < 0.001f)
            return 0.0f;

        // Period-heuristic slip-burst trigger, jittered per event: real slip
        // events wander in timing (±kSlipPeriodJitter) and strength (±30%) —
        // an exactly metronomic, constant-amplitude retrigger reads as a buzz.
        if (slipPeriodSamples > 0 && --slipCounter <= 0)
        {
            slipEnvelope = bowEnergy * (0.7f + 0.6f * jitterRandom.nextFloat());
            const float pj = 1.0f + kSlipPeriodJitter
                                    * (2.0f * jitterRandom.nextFloat() - 1.0f);
            slipCounter  = juce::jmax (1, juce::roundToInt (
                               static_cast<float> (slipPeriodSamples) * pj));
        }
        slipEnvelope *= kSlipDecayAtSr;

        // 3-band BPF on white noise, averaged
        const float white = noiseRandom.nextFloat() * 2.0f - 1.0f;
        float bandSum = 0.0f;
        for (size_t i = 0; i < kNumBpf; ++i)
            bandSum += bpfs[i].processSample (white);
        bandSum *= (1.0f / static_cast<float> (kNumBpf));

        // Pitch-synchronous feedback comb (damped loop) pitches the hiss at
        // harmonics of the played fundamental. kCombNorm ≈ sqrt(1 − g²)
        // restores approx. unity RMS through the resonant loop.
        float pitched = bandSum;
        if (combPeriod > 0)
        {
            const size_t n = combBuffer.size();
            const float delayed = combBuffer[(combWrite + n - static_cast<size_t> (combPeriod)) % n];
            combDampState += combDampCoeff * (delayed - combDampState);
            const float combed = bandSum + kCombFeedback * combDampState;
            combBuffer[combWrite] = combed;
            combWrite = (combWrite + 1) % n;
            pitched = kCombMix * (combed * kCombNorm) + (1.0f - kCombMix) * bandSum;
        }

        const float continuous = pitched * bowEnergy;
        const float burst      = pitched * slipEnvelope;
        return (continuous + burst) * noiseLevel;
    }

    inline void reset() noexcept
    {
        for (auto& f : bpfs)
            f.reset();
        std::fill (combBuffer.begin(), combBuffer.end(), 0.0f);
        combWrite     = 0;
        combDampState = 0.0f;
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

    // v1.5.0 realism constants (tuned by rendered A/B).
    static constexpr float kCombMinF0       = 20.0f;   // comb buffer sizing floor
    static constexpr float kCombFeedback    = 0.85f;   // loop gain — comb peak strength
    static constexpr float kCombNorm        = 0.527f;  // ≈ sqrt(1 − 0.85²), unity-RMS trim
    static constexpr float kCombMix         = 0.70f;   // pitched vs raw hiss blend
    static constexpr float kSlipPeriodJitter = 0.04f;  // ±4% slip re-trigger timing wander

    juce::Random noiseRandom;
    juce::Random jitterRandom;   // slip-event draws only — separate stream per phase
    std::array<juce::dsp::IIR::Filter<float>, kNumBpf> bpfs;

    // Pitch-synchronous comb state (buffer allocated in prepare()).
    std::vector<float> combBuffer;
    size_t combWrite     = 0;
    int    combPeriod    = 0;
    float  combDampState = 0.0f;
    float  combDampCoeff = 0.4f;

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
