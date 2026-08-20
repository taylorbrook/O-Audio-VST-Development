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

    CrossoverNetwork.h
    Linkwitz-Riley 4th order crossover (24 dB/octave)
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <algorithm>

class CrossoverNetwork
{
public:
    CrossoverNetwork() = default;
    ~CrossoverNetwork() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        currentSampleRate = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = static_cast<juce::uint32>(numChannels);

        // Prepare all filters (3 crossover points × 2 filters (LP/HP) × 2 cascades = 12 filters per channel)
        for (int i = 0; i < 3; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                // Linkwitz-Riley = 2 cascaded Butterworth 2nd order
                lowpass1[i][ch].prepare(spec);
                lowpass2[i][ch].prepare(spec);
                highpass1[i][ch].prepare(spec);
                highpass2[i][ch].prepare(spec);
            }
        }

        // Phase-compensation all-passes (WR-03): 3 per channel
        for (int ch = 0; ch < 2; ++ch)
        {
            allpassF2Low[ch].prepare(spec);
            allpassF3Low[ch].prepare(spec);
            allpassF3LoMid[ch].prepare(spec);
        }

        // Seed every filter's coefficient storage to a full 2nd-order (6-tap) array so
        // RT updates (updateCoefficients) can overwrite the taps in place, without
        // reallocating the coefficient Array on the audio thread. (CR-01)
        seedCoefficientStorage();

        // Force the first real design (cache is invalid).
        lastXover1 = lastXover2 = lastXover3 = -1.0f;
        lastSampleRate = 0.0;
        updateCoefficients(200.0f, 2000.0f, 8000.0f);
    }

    void reset()
    {
        for (int i = 0; i < 3; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                lowpass1[i][ch].reset();
                lowpass2[i][ch].reset();
                highpass1[i][ch].reset();
                highpass2[i][ch].reset();
            }
        }

        for (int ch = 0; ch < 2; ++ch)
        {
            allpassF2Low[ch].reset();
            allpassF3Low[ch].reset();
            allpassF3LoMid[ch].reset();
        }
    }

    // Update crossover frequencies.
    // RT-safe: skips all work when the (clamped) frequencies and sample rate are unchanged,
    // and when they do change it designs into stack-allocated coefficient arrays and writes
    // the taps in place — no heap allocation and no trig-heavy redesign on the audio thread
    // when the params are static. (CR-01)
    // The DSP-side frequency-ordering clamp. Static and public so PhaseMatchChain
    // (v1.6.1) reproduces EXACTLY the frequencies the crossover actually runs — the
    // phase-match all-passes cancel the crossover's rotation only at identical corners.
    static void clampCrossoverFrequencies(float& xover1Hz, float& xover2Hz, float& xover3Hz)
    {
        // Validate frequency ordering: xover1 < xover2 < xover3
        xover1Hz = juce::jlimit(20.0f, 500.0f, xover1Hz);
        xover2Hz = juce::jlimit(std::max(xover1Hz + 100.0f, 200.0f), 5000.0f, xover2Hz);
        xover3Hz = juce::jlimit(std::max(xover2Hz + 100.0f, 2000.0f), 16000.0f, xover3Hz);
    }

    void updateCoefficients(float xover1Hz, float xover2Hz, float xover3Hz)
    {
        clampCrossoverFrequencies(xover1Hz, xover2Hz, xover3Hz);

        // Early-out when nothing has moved — the common case, every block.
        if (xover1Hz == lastXover1 && xover2Hz == lastXover2 && xover3Hz == lastXover3
            && currentSampleRate == lastSampleRate)
            return;

        lastXover1 = xover1Hz;
        lastXover2 = xover2Hz;
        lastXover3 = xover3Hz;
        lastSampleRate = currentSampleRate;

        // ArrayCoefficients::makeXXX returns a std::array<float, 6> on the stack (no heap
        // allocation). A 2nd-order Butterworth (Q = 1/sqrt(2)) is numerically identical to
        // the order-2 designIIR...ButterworthMethod used previously, so the sound is unchanged.
        const float xoverHz[3] = { xover1Hz, xover2Hz, xover3Hz };

        for (int i = 0; i < 3; ++i)
        {
            const auto lp = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
                currentSampleRate, xoverHz[i], butterworthQ);
            const auto hp = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                currentSampleRate, xoverHz[i], butterworthQ);

            for (int ch = 0; ch < 2; ++ch)
            {
                // Both cascades of the Linkwitz-Riley pair use the same Butterworth taps.
                assignCoeffs(lowpass1[i][ch],  lp);
                assignCoeffs(lowpass2[i][ch],  lp);
                assignCoeffs(highpass1[i][ch], hp);
                assignCoeffs(highpass2[i][ch], hp);
            }
        }

        // Phase-compensation all-passes (WR-03). An LR4 pair sums to a 2nd-order
        // all-pass at its crossover frequency (Q = 1/sqrt(2)), so each lower band must
        // pass through one such all-pass per crossover it skips: LOW through AP(f2) and
        // AP(f3), LOMID through AP(f3). The 4-band sum then equals
        // AP(f1)·AP(f2)·AP(f3) — flat magnitude at unity.
        const auto ap2 = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(
            currentSampleRate, xover2Hz, butterworthQ);
        const auto ap3 = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(
            currentSampleRate, xover3Hz, butterworthQ);

        for (int ch = 0; ch < 2; ++ch)
        {
            assignCoeffs(allpassF2Low[ch],   ap2);
            assignCoeffs(allpassF3Low[ch],   ap3);
            assignCoeffs(allpassF3LoMid[ch], ap3);
        }
    }

    // Process buffer and split into 4 bands
    // bandBuffers[0] = LOW, bandBuffers[1] = LOMID, bandBuffers[2] = HIMID, bandBuffers[3] = HIGH
    void processSplit(const juce::AudioBuffer<float>& input,
                     juce::AudioBuffer<float>* bandBuffers)
    {
        const int numSamples = input.getNumSamples();
        const int numChannels = input.getNumChannels();

        // Clear all band buffers
        for (int band = 0; band < 4; ++band)
        {
            bandBuffers[band].clear();
        }

        // Process each channel independently
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* inputData = input.getReadPointer(ch);

            // Cache band write pointers once per channel (IN-01) — no per-sample bounds checks.
            float* lowData   = bandBuffers[0].getWritePointer(ch);   // LOW
            float* loMidData = bandBuffers[1].getWritePointer(ch);   // LOMID
            float* hiMidData = bandBuffers[2].getWritePointer(ch);   // HIMID
            float* highData  = bandBuffers[3].getWritePointer(ch);   // HIGH

            // Temporary storage for crossover outputs
            float xover1_low, xover1_high;
            float xover2_low, xover2_high;
            float xover3_low, xover3_high;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                float inputSample = inputData[sample];

                // ===== Crossover 1: Split input into LOW and REST =====
                // Low band: LP through both cascades, then phase-matched to the upper
                // crossovers via AP(f2)·AP(f3) (WR-03).
                xover1_low = lowpass1[0][ch].processSample(inputSample);
                xover1_low = lowpass2[0][ch].processSample(xover1_low);
                xover1_low = allpassF2Low[ch].processSample(xover1_low);
                xover1_low = allpassF3Low[ch].processSample(xover1_low);

                // High band (to next crossover): HP through both cascades
                xover1_high = highpass1[0][ch].processSample(inputSample);
                xover1_high = highpass2[0][ch].processSample(xover1_high);

                // ===== Crossover 2: Split xover1_high into LOMID and REST =====
                // Low-Mid band: LP through both cascades, then phase-matched to
                // crossover 3 via AP(f3) (WR-03).
                xover2_low = lowpass1[1][ch].processSample(xover1_high);
                xover2_low = lowpass2[1][ch].processSample(xover2_low);
                xover2_low = allpassF3LoMid[ch].processSample(xover2_low);

                // High band (to next crossover): HP through both cascades
                xover2_high = highpass1[1][ch].processSample(xover1_high);
                xover2_high = highpass2[1][ch].processSample(xover2_high);

                // ===== Crossover 3: Split xover2_high into HIMID and HIGH =====
                // High-Mid band (final): LP through both cascades
                xover3_low = lowpass1[2][ch].processSample(xover2_high);
                xover3_low = lowpass2[2][ch].processSample(xover3_low);

                // High band (final): HP through both cascades
                xover3_high = highpass1[2][ch].processSample(xover2_high);
                xover3_high = highpass2[2][ch].processSample(xover3_high);

                // Write to band buffers
                lowData[sample]   = xover1_low;    // LOW
                loMidData[sample] = xover2_low;    // LOMID
                hiMidData[sample] = xover3_low;    // HIMID
                highData[sample]  = xover3_high;   // HIGH
            }
        }
    }

private:
    // Standard 2nd-order Butterworth Q (1/sqrt(2)) — one cascade of the LR-4 pair.
    static constexpr float butterworthQ = 0.70710678118654752440f;

    // Seed each filter's coefficient Array once at prepare time (allocation OK here). The
    // std::array assignment routes through Coefficients::assignImpl, which calls
    // ensureStorageAllocated(>= 8) — so the backing storage is large enough that every later
    // RT assignment in assignCoeffs() reuses it without reallocating.
    void seedCoefficientStorage()
    {
        const auto lp = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, 1000.0f, butterworthQ);
        const auto hp = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(currentSampleRate, 1000.0f, butterworthQ);
        const auto ap = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(currentSampleRate, 1000.0f, butterworthQ);

        for (int i = 0; i < 3; ++i)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                assignCoeffs(lowpass1[i][ch],  lp);
                assignCoeffs(lowpass2[i][ch],  lp);
                assignCoeffs(highpass1[i][ch], hp);
                assignCoeffs(highpass2[i][ch], hp);
            }
        }

        for (int ch = 0; ch < 2; ++ch)
        {
            assignCoeffs(allpassF2Low[ch],   ap);
            assignCoeffs(allpassF3Low[ch],   ap);
            assignCoeffs(allpassF3LoMid[ch], ap);
        }
    }

    // Overwrite a filter's biquad taps with no heap allocation.
    // IIR::Coefficients::operator=(std::array<float,6>) normalises by a0 and stores the 5-tap
    // form {b0,b1,b2,a1,a2}; because the backing juce::Array was pre-allocated (capacity >= 8)
    // in seedCoefficientStorage(), that assignment reuses the storage without reallocating.
    static void assignCoeffs(juce::dsp::IIR::Filter<float>& filter, const std::array<float, 6>& taps)
    {
        *filter.coefficients = taps;
    }

    double currentSampleRate = 44100.0;

    // Cached last-applied crossover state for the RT early-out (CR-01).
    float lastXover1 = -1.0f;
    float lastXover2 = -1.0f;
    float lastXover3 = -1.0f;
    double lastSampleRate = 0.0;

    // Linkwitz-Riley 4th order = 2 cascaded 2nd order Butterworth filters
    // [crossover_index][channel]
    juce::dsp::IIR::Filter<float> lowpass1[3][2];   // First cascade, LP
    juce::dsp::IIR::Filter<float> lowpass2[3][2];   // Second cascade, LP
    juce::dsp::IIR::Filter<float> highpass1[3][2];  // First cascade, HP
    juce::dsp::IIR::Filter<float> highpass2[3][2];  // Second cascade, HP

    // WR-03 phase compensation, [channel]. Each lower band passes through a 2nd-order
    // all-pass per higher crossover it skips, matching the phase the upper bands
    // accumulate through those LR4 splits.
    juce::dsp::IIR::Filter<float> allpassF2Low[2];    // AP @ xover2 on LOW
    juce::dsp::IIR::Filter<float> allpassF3Low[2];    // AP @ xover3 on LOW
    juce::dsp::IIR::Filter<float> allpassF3LoMid[2];  // AP @ xover3 on LOMID

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossoverNetwork)
};
