/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

    PhaseMatchChain.h
    AP(f1)·AP(f2)·AP(f3) phase-match chain  (v1.6.1)
    O-MultiBandCompressor

    The crossover's 4-band unity sum is not transparent: each LR4 pair sums to a
    2nd-order all-pass at its crossover (Q = 1/sqrt(2)), so the full network's
    sum carries AP(f1)·AP(f2)·AP(f3) — flat magnitude, 0→−1080° of phase. Any
    signal later summed or M/S-decoded against that output must first accumulate
    the same rotation, or the two sit ~180° apart near each crossover:

      - the dry path of the Mix control combs against the wet signal at
        intermediate settings (WR-03), and
      - the passthrough channel in Mid/Side modes mirrors the stereo image
        around each crossover on decode (WR-02).

    This chain applies exactly those three all-passes. Frequencies are clamped
    through CrossoverNetwork::clampCrossoverFrequencies so the corners always
    match what the crossover actually runs.

    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "CrossoverNetwork.h"

class PhaseMatchChain
{
public:
    PhaseMatchChain() = default;
    ~PhaseMatchChain() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        currentSampleRate = sampleRate;
        numPreparedChannels = juce::jlimit(1, 2, numChannels);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1; // filters are per-channel scalars

        // Seed every filter's coefficient storage on the host thread so the RT
        // updates in updateCoefficients() overwrite the taps in place (the same
        // ArrayCoefficients discipline as CrossoverNetwork).
        const auto seed = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(
            sampleRate, 1000.0f, butterworthQ);

        for (int i = 0; i < 3; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                allpass[i][ch].prepare(spec);
                allpass[i][ch].reset();
                *allpass[i][ch].coefficients = seed;
            }
        }

        // Force the first real design (cache is invalid).
        lastXover1 = lastXover2 = lastXover3 = -1.0f;
        updateCoefficients(200.0f, 2000.0f, 8000.0f);
    }

    void reset()
    {
        for (int i = 0; i < 3; ++i)
            for (int ch = 0; ch < 2; ++ch)
                allpass[i][ch].reset();
    }

    // RT-safe: early-outs when the (clamped) frequencies are unchanged; otherwise
    // designs into stack arrays and writes the taps in place.
    void updateCoefficients(float xover1Hz, float xover2Hz, float xover3Hz)
    {
        CrossoverNetwork::clampCrossoverFrequencies(xover1Hz, xover2Hz, xover3Hz);

        if (xover1Hz == lastXover1 && xover2Hz == lastXover2 && xover3Hz == lastXover3)
            return;

        lastXover1 = xover1Hz;
        lastXover2 = xover2Hz;
        lastXover3 = xover3Hz;

        const float xoverHz[3] = { xover1Hz, xover2Hz, xover3Hz };

        for (int i = 0; i < 3; ++i)
        {
            const auto taps = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(
                currentSampleRate, xoverHz[i], butterworthQ);

            for (int ch = 0; ch < 2; ++ch)
                *allpass[i][ch].coefficients = taps;
        }
    }

    // Process all (prepared) channels of a buffer in place.
    void process(juce::AudioBuffer<float>& buffer, int numChannels, int numSamples)
    {
        numChannels = juce::jmin(numChannels, numPreparedChannels, buffer.getNumChannels());

        for (int ch = 0; ch < numChannels; ++ch)
            processChannel(buffer.getWritePointer(ch), ch, numSamples);
    }

    // Process one channel of audio through filter-state channel `stateChannel`.
    // (Mid/Side passthrough uses a dedicated 1-channel chain: stateChannel 0.)
    void processChannel(float* data, int stateChannel, int numSamples)
    {
        auto& ap1 = allpass[0][stateChannel];
        auto& ap2 = allpass[1][stateChannel];
        auto& ap3 = allpass[2][stateChannel];

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float x = data[sample];
            x = ap1.processSample(x);
            x = ap2.processSample(x);
            x = ap3.processSample(x);
            data[sample] = x;
        }
    }

private:
    // Same constant as CrossoverNetwork::butterworthQ — an LR4 pair sums to a
    // 2nd-order all-pass at this Q.
    static constexpr float butterworthQ = 0.70710678118654752440f;

    double currentSampleRate = 44100.0;
    int numPreparedChannels = 2;

    float lastXover1 = -1.0f;
    float lastXover2 = -1.0f;
    float lastXover3 = -1.0f;

    // [crossover_index][channel]
    juce::dsp::IIR::Filter<float> allpass[3][2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseMatchChain)
};
