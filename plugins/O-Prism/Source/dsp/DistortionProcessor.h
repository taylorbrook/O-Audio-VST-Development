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

    DistortionProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class DistortionProcessor
{
public:
    DistortionProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setType (int type);
    void setDrive (float drive);
    void setMix (float mix);

    float getLatencyInSamples() const { return static_cast<float> (oversampling.getLatencyInSamples()); }

private:
    void applyDistortion (juce::dsp::AudioBlock<float>& block);

    /** Channel count the oversampler is built for. The oversampled block can
        therefore never present more channels than this, which is what bounds
        `adaaPrevU` below -- the two MUST stay in agreement.
    */
    static constexpr size_t kNumChannels = 2;

    juce::dsp::Oversampling<float> oversampling;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    /** Previous oversampled input sample per channel, for the antialiased Fold
        (IN-09). Tracked for EVERY distortion type, not only Fold, so that
        switching into Fold never differences against a sample left over from
        whenever Fold last ran. Cleared by `prepare()` and `reset()`.
    */
    double adaaPrevU[kNumChannels] { 0.0, 0.0 };

    int distType = 0;
    float driveAmount = 0.0f;
};
