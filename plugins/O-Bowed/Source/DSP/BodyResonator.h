/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    BodyResonator.h
    O-Bowed - 8-Section Parallel Peaking EQ Body Resonator
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class BodyResonator
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Called from processBlock with current APVTS values
    void setMaterial (float material);   // 0.0 - 1.0
    void setSize (float size);           // 0.0 - 1.0
    void setBodyAmount (float amount);   // 0.0 - 1.0 (dry/wet blend)

    // Per-sample stereo processing (parallel biquad bank, separate state per channel)
    void processStereo (float& left, float& right);

private:
    static constexpr int NUM_MODES = 8;
    static constexpr int NUM_PRESETS = 4;

    struct ModePreset {
        float freq[NUM_MODES];
        float q[NUM_MODES];
        float gainDb[NUM_MODES];
    };

    static const ModePreset presets[NUM_PRESETS];

    // Stereo filter banks (shared coefficients, separate filter state per channel)
    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesL;
    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesR;

    // Pre-allocated coefficient objects (one per mode, shared L/R). Seeded once in
    // prepare() so updateCoefficients() can mutate them in place on the audio thread
    // without heap alloc/free. CR-01/CR-02 (pattern_arraycoefficients_rt_safe_iir).
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, NUM_MODES> modeCoeffs;

    // Current state
    double currentSampleRate = 44100.0;
    float currentMaterial = -1.0f;   // -1 forces initial update
    float currentSize = -1.0f;

    // Normalization
    float normGain = 1.0f;
    float dryMix = 0.4f;
    float wetMix = 0.6f;

    // Precomputed gain sums for normalization
    float presetGainSums[NUM_PRESETS] {};
    float referenceGainSum = 1.0f;

    void updateCoefficients();
    static float computePresetGainSum (const ModePreset& preset);
};
