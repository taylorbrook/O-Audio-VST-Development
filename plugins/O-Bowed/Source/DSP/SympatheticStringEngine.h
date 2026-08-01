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

    SympatheticStringEngine.h
    O-Bowed - Passive Karplus-Strong Sympathetic String Resonators (0-12)
    Ouaricon Audio
    Developer: Taylor Brook

    Passive KS waveguide strings excited by bridge output. Tuned to harmonic
    partials of active voices. Energy-gated for CPU efficiency. Per-string
    stereo panning.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class SympatheticStringEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Configuration
    void setCount (int count);
    void setAmount (float amount);
    void setDecay (float decay);  // 0.0-1.0 mapped to loss coefficient range

    // Recompute sympathetic tunings when active notes change
    void updateTunings (const float* fundamentals, int numFundamentals);

    // Per-sample processing: excitation in, stereo out
    struct StereoSample { float left; float right; };
    StereoSample processSample (float excitation);

private:
    static constexpr int MAX_SYMPATHETICS = 12;
    static constexpr float HALF_PI = 1.5707963f;

    // Fixed damping-lowpass pole (frequency-dependent decay / string tone). Decay is
    // NOT controlled here — the Decay knob drives a separate sub-unity feedback gain
    // so the fundamental actually decays and any DC in the loop drains. WR-05.
    static constexpr float DAMP_POLE = 0.995f;

    struct SympatheticString {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delay { 4410 };
        float decayGain = 0.9995f;   // per-loop feedback gain (< 1) — sets ring time
        float filterState = 0.0f;
        float frequency = 0.0f;
        float energyEstimate = 0.0f;
        float panL = 0.707f;
        float panR = 0.707f;
        bool active = false;
    };

    std::array<SympatheticString, MAX_SYMPATHETICS> strings;
    int activeCount = 0;
    float amountParam = 0.0f;
    float decayParam = 0.5f;  // 0.0-1.0 -> per-string decayGain 0.990-0.9995 (WR-05)
    double currentSampleRate = 44100.0;

    void tuneString (int index, float frequency);
    void computeHarmonics (const float* fundamentals, int numFundamentals,
                           float* outFreqs, int maxOut, int& numOut);
};
