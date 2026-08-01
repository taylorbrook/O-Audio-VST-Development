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

    BodyResonator.h
    O-Contrabass - 8-Mode Parallel Bandpass Body Resonator (Phase 2.5)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.5 implementation per RESEARCH §21.2.6 + §21.13 + §21.10.
    Bass-tuned 8-mode parallel bandpass bank using
    juce::dsp::IIR::Coefficients<float>::makeBandPass per ARCHITECTURE §134
    + RESEARCH §21.2.2 delta. Mono single-bank topology (voice output is
    mono pre-stereo-split). Per-block coefficient recompute reading
    current Size/Damping. 35 Hz HP one-pole on dry path. Wet/dry blend
    `(1−mix)·HP35(in) + mix·wet`.

    Public API:
      - prepare(double sampleRate, int maxBlockSize)
      - reset()
      - setSize(float)     [0, 1]
      - setDamping(float)  [0, 1]
      - setMix(float)      [0, 1]
      - processBlock(float* mono, int numSamples)

    Defaults match parameter-spec.md:
      - Size    = 0.75
      - Damping = 0.40
      - Mix     = 0.80

    Mode table (ARCHITECTURE §"Body Resonator (Parallel Biquad Bank)"):
      i: fc(Hz)  Q     gainDb
      0:  60.0  14.0   -2
      1:  98.0  11.0    0
      2: 115.0   9.0   -1
      3: 175.0   8.0   -3
      4: 235.0   7.0   -4
      5: 340.0   6.0   -5
      6: 700.0   5.0   -7
      7:1200.0   2.5   -6

  ==============================================================================
*/

#pragma once

#include <array>
#include <juce_dsp/juce_dsp.h>

class BodyResonator
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setSize    (float v) noexcept;
    void setDamping (float v) noexcept;
    void setMix     (float v) noexcept;

    void processBlock (float* mono, int numSamples) noexcept;

private:
    void recomputeCoefficients() noexcept;

    static constexpr int kNumModes = 8;
    static constexpr float kDefaultFreq[kNumModes]
        = { 60.0f, 98.0f, 115.0f, 175.0f, 235.0f, 340.0f, 700.0f, 1200.0f };
    static constexpr float kDefaultQ[kNumModes]
        = { 14.0f, 11.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 2.5f };
    static constexpr float kDefaultGainDb[kNumModes]
        = { -2.0f, 0.0f, -1.0f, -3.0f, -4.0f, -5.0f, -7.0f, -6.0f };

    std::array<juce::dsp::IIR::Filter<float>, kNumModes> modes;
    float gainLinear[kNumModes] {};

    // 35 Hz HP one-pole (dry path)
    float hp35_x1 = 0.0f;
    float hp35_y1 = 0.0f;
    float hp35_a  = 0.99544f;   // exp(-2π·35/sr); recomputed in prepare()

    double currentSampleRate = 48000.0;
    float  currentSize    = 0.75f;
    float  currentDamping = 0.40f;
    float  currentMix     = 0.80f;
};
