/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
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
    O-IntonationPad - Wavetable Pad Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DelayProcessor.h"
#include <cmath>

DelayProcessor::DelayProcessor() = default;

void DelayProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    delayL.prepare (spec);
    delayR.prepare (spec);

    // WR-03: size the delay lines for the actual sample rate (delayTime max is 2.0s).
    // The fixed 192000-sample ctor size overflows above 96 kHz; rescale here (non-RT).
    const int maxDelaySamples = static_cast<int> (std::ceil (2.0 * spec.sampleRate)) + 1;
    delayL.setMaximumDelayInSamples (maxDelaySamples);
    delayR.setMaximumDelayInSamples (maxDelaySamples);

    feedbackFilterL.prepare (spec);
    feedbackFilterR.prepare (spec);
    dryWetMixer.prepare (spec);

    feedbackFilterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterL.setCutoffFrequency (8000.0f);
    feedbackFilterR.setCutoffFrequency (8000.0f);

    delaySamples = 0.375f * currentSampleRate;
}

void DelayProcessor::reset()
{
    delayL.reset();
    delayR.reset();
    feedbackFilterL.reset();
    feedbackFilterR.reset();
    dryWetMixer.reset();
    feedbackL = feedbackR = 0.0f;
}

void DelayProcessor::setTime (float seconds)
{
    delaySamples = seconds * currentSampleRate;
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

        float wetL = delayL.popSample (0, delaySamples);
        float wetR = delayR.popSample (0, delaySamples);

        feedbackL = feedbackFilterL.processSample (0, wetL);
        feedbackR = feedbackFilterR.processSample (0, wetR);

        leftData[i] = wetL;
        rightData[i] = wetR;
    }

    dryWetMixer.mixWetSamples (block);
}
