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

    NoiseGenerator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class NoiseGenerator
{
public:
    NoiseGenerator() = default;

    void prepare (double sampleRate);
    void reset();
    void setType (int type);
    void getNextSampleStereo (double& outL, double& outR);

private:
    juce::Random randomL, randomR;
    double currentSampleRate = 44100.0;
    int currentType = 0; // 0=White, 1=Pink, 2=Brown, 3=Digital, 4=Vinyl, 5=Wind

    // Pink noise state (Paul Kellet economy) — per channel
    double b0L = 0.0, b1L = 0.0, b2L = 0.0;
    double b0R = 0.0, b1R = 0.0, b2R = 0.0;

    // Brown noise state — per channel
    double brownStateL = 0.0;
    double brownStateR = 0.0;

    // Digital noise state — per channel
    double digitalHoldValueL = 0.0, digitalHoldValueR = 0.0;
    int digitalCounterL = 0, digitalCounterR = 0;
    int digitalHoldSamples = 8;

    // Vinyl noise state — per channel
    double vinylBP1L = 0.0, vinylBP2L = 0.0;
    double vinylBP1R = 0.0, vinylBP2R = 0.0;
    double crackleDecayL = 0.0, crackleDecayR = 0.0;

    // Wind noise state — shared LFO, per-channel filter
    double windLFOPhase = 0.0;
    double windLPStateL = 0.0, windLPStateR = 0.0;
    double windBrownStateL = 0.0, windBrownStateR = 0.0;
};
