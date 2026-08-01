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

    DelayProcessor.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class DelayProcessor
{
public:
    DelayProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::dsp::AudioBlock<float>& block);
    void reset();

    void setTime (float seconds);
    void setFeedback (float fb);
    void setMode (int mode);
    void setMix (float mix);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayL { 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayR { 192000 };
    juce::dsp::StateVariableTPTFilter<float> feedbackFilterL;
    juce::dsp::StateVariableTPTFilter<float> feedbackFilterR;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    float delaySamples = 0.0f;
    float feedbackAmount = 0.3f;
    int delayMode = 0; // 0=Normal, 1=PingPong
    float currentSampleRate = 44100.0f;
    float feedbackL = 0.0f, feedbackR = 0.0f;
};
