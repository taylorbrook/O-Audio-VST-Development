/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    WavetableOscillator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include <cmath>

enum class WarpType
{
    Off = 0,
    Sync,
    Bend,
    FM,
    Window
};

class WavetableOscillator
{
public:
    WavetableOscillator() = default;

    void prepare (double sampleRate);
    void setFrequency (double freq);
    void setPosition (float pos);
    void setWavetable (const WavetableData* table);
    void reset();
    void resetWithPhase (double phase);

    void getNextSampleStereo (double& outL, double& outR);

    // Unison
    void setUnison (int count, float detune, float width);
    void resetWithRandomPhases();

    // Warp
    void setWarpType (WarpType type);
    void setWarpAmount (float amount);
    void setFMInput (double value);

    double getFrequency() const { return frequency; }

private:
    const WavetableData* wavetable = nullptr;
    double currentSampleRate = 44100.0;
    double frequency = 440.0;
    double phaseIncrement = 0.0;
    float position = 0.0f; // Frame position 0-1

    // Phase accumulator(s) for unison
    static constexpr int kMaxUnison = 8;
    double phaseAccumulators[kMaxUnison] = {};
    double unisonDetuneFactors[kMaxUnison] = { 1.0 };
    double unisonPanL[kMaxUnison] = { 1.0 };
    double unisonPanR[kMaxUnison] = { 1.0 };
    double unisonGain = 1.0;
    int unisonCount = 1;

    // Warp state
    WarpType warpType = WarpType::Off;
    float warpAmount = 0.0f;
    double fmInput = 0.0;
    double masterPhases[kMaxUnison] = {};

    double readSample (double phase) const;
    double applyWarp (double phase, int voiceIndex) const;
};
