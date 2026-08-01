/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

    DelayProcessor.cpp
    O-Wind - Physical Modeling Flute Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DelayProcessor.h"

DelayProcessor::DelayProcessor() = default;

void DelayProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    delayL.prepare (spec);
    delayR.prepare (spec);

    // Size for the full delayTime range (2.0 s) at the prepared rate
    const int maxDelaySamples = static_cast<int> (std::ceil (2.0 * spec.sampleRate)) + 4;
    delayL.setMaximumDelayInSamples (maxDelaySamples);
    delayR.setMaximumDelayInSamples (maxDelaySamples);

    feedbackFilterL.prepare (spec);
    feedbackFilterR.prepare (spec);
    dryWetMixer.prepare (spec);

    feedbackFilterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterL.setCutoffFrequency (8000.0f);
    feedbackFilterR.setCutoffFrequency (8000.0f);

    delaySmoothed.reset (spec.sampleRate, 0.03);
    delaySmoothed.setCurrentAndTargetValue (0.375f * currentSampleRate);
}

void DelayProcessor::reset()
{
    delayL.reset();
    delayR.reset();
    feedbackFilterL.reset();
    feedbackFilterR.reset();
    dryWetMixer.reset();
    feedbackL = feedbackR = 0.0f;
    delaySmoothed.setCurrentAndTargetValue (delaySmoothed.getTargetValue());
}

void DelayProcessor::setTime (float seconds)
{
    const float maxSamples = static_cast<float> (delayL.getMaximumDelayInSamples() - 4);
    delaySmoothed.setTargetValue (juce::jlimit (1.0f, maxSamples, seconds * currentSampleRate));
}

void DelayProcessor::setFeedback (float fb)
{
    feedbackAmount = fb;
}

void DelayProcessor::setMode (int mode)
{
    delayMode = mode;
}

void DelayProcessor::setMix (float mix)
{
    dryWetMixer.setWetMixProportion (mix);
}

void DelayProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    dryWetMixer.pushDrySamples (block);

    auto numSamples = block.getNumSamples();
    auto* leftData = block.getChannelPointer (0);
    auto* rightData = block.getNumChannels() > 1 ? block.getChannelPointer (1) : leftData;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float inputL = leftData[i];
        float inputR = rightData[i];

        if (delayMode == 0) // Normal
        {
            delayL.pushSample (0, inputL + feedbackL * feedbackAmount);
            delayR.pushSample (0, inputR + feedbackR * feedbackAmount);
        }
        else // PingPong (cross-feedback)
        {
            delayL.pushSample (0, inputL + feedbackR * feedbackAmount);
            delayR.pushSample (0, inputR + feedbackL * feedbackAmount);
        }

        const float delaySamples = delaySmoothed.getNextValue();
        float wetL = delayL.popSample (0, delaySamples);
        float wetR = delayR.popSample (0, delaySamples);

        feedbackL = feedbackFilterL.processSample (0, wetL);
        feedbackR = feedbackFilterR.processSample (0, wetR);

        leftData[i] = wetL;
        rightData[i] = wetR;
    }

    dryWetMixer.mixWetSamples (block);
}
