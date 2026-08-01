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

    BodyResonance.h
    Phase 2.4: Convolution-based body resonance for marimba tube character
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class BodyResonance
{
public:
    BodyResonance();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process audio block (in-place, stereo)
    void process(juce::AudioBuffer<float>& buffer);

    // Set dry/wet mix (0.0 = dry only, 1.0 = full wet blend)
    void setMix(float newMix);

private:
    juce::dsp::Convolution convolution;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    double currentSampleRate = 44100.0;
    float mix = 0.5f;

    // Generate synthetic IR
    void generateSyntheticIR();
    juce::AudioBuffer<float> irBuffer;
};
