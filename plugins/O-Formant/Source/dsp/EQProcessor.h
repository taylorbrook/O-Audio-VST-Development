/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    EQProcessor.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EQProcessor
{
public:
    EQProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setLowGain (float dB);
    void setMidGain (float dB);
    void setMidFreq (float hz);
    void setHighGain (float dB);

private:
    using FilterCoeffs = juce::dsp::IIR::Coefficients<float>;
    // Stack-allocating coefficient factory (returns std::array<float,6>, no heap).
    // Used to recompute coefficients in-place on the audio thread without the
    // ref-counted Coefficients allocation that makeXXX() would incur (WR-08).
    using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;
    using Filter = juce::dsp::IIR::Filter<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCoeffs>;

    StereoFilter lowShelf;
    StereoFilter midPeak;
    StereoFilter highShelf;

    float currentSampleRate = 44100.0f;

    std::atomic<float> targetLowGainDB { 0.0f };
    std::atomic<float> targetMidGainDB { 0.0f };
    std::atomic<float> targetMidFreqHz { 1000.0f };
    std::atomic<float> targetHighGainDB { 0.0f };

    float prevLowGainDB = -999.0f;
    float prevMidGainDB = -999.0f;
    float prevMidFreqHz = -999.0f;
    float prevHighGainDB = -999.0f;
};
