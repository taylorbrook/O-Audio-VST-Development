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

    DelayProcessor.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DelayProcessor.h"

DelayProcessor::DelayProcessor() = default;

void DelayProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    // v1.31.1 (review IN-16): size the lines from the sample rate so the full
    // 2 s range exists at every rate — the fixed 192000 samples topped the
    // knob out at 1 s at 192 kHz. The + 4 keeps the Lagrange3rd taps
    // (delayInt .. delayInt + 3) inside the buffer at the 2 s maximum.
    // prepare() runs off the audio thread, so the allocation is fine here.
    const int maxDelaySamples = static_cast<int> (std::ceil (kMaxDelaySeconds * spec.sampleRate)) + 4;
    delayL.setMaximumDelayInSamples (maxDelaySamples);
    delayR.setMaximumDelayInSamples (maxDelaySamples);

    delayL.prepare (spec);
    delayR.prepare (spec);
    feedbackFilterL.prepare (spec);
    feedbackFilterR.prepare (spec);
    dryWetMixer.prepare (spec);

    feedbackFilterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterL.setCutoffFrequency (8000.0f);
    feedbackFilterR.setCutoffFrequency (8000.0f);

    // WR-16: delay time glides per sample (tape-style) instead of stepping at
    // block rate, which clicked on every knob / automation move.
    delaySamples.reset (spec.sampleRate, 0.1);
    delaySamples.setCurrentAndTargetValue (0.375f * currentSampleRate);
    snapTimeOnNextSet = true;
}

void DelayProcessor::reset()
{
    delayL.reset();
    delayR.reset();
    feedbackFilterL.reset();
    feedbackFilterR.reset();
    dryWetMixer.reset();
    feedbackL = feedbackR = 0.0f;
    // v1.30.1: don't snap to the old target here — reset() runs BEFORE the
    // block's setTime(), so the line would then glide from a stale time and
    // chirp the first echo. The next setTime() snaps to the live value instead.
    snapTimeOnNextSet = true;
}

void DelayProcessor::setTime (float seconds)
{
    // Clamp to the delay line's capacity. Since v1.31.1 prepare() sizes the
    // lines for the full 2 s at the current rate (IN-16), so this no longer
    // bites; it stays as a guard, because a request past the buffer would
    // silently alias (popSample masks by % totalSize) to a wrong, shorter time
    // — and trip the jassert in Debug builds. (REVIEW.md WR-07)
    float requested = seconds * currentSampleRate;
    const float target = juce::jmin (requested, static_cast<float> (delayL.getMaximumDelayInSamples()));

    if (snapTimeOnNextSet)
    {
        delaySamples.setCurrentAndTargetValue (target);
        snapTimeOnNextSet = false;
    }
    else
    {
        delaySamples.setTargetValue (target);
    }
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
            // WR-18: feed the mono sum into L only. Feeding L and R each into
            // their own line made a centred source (L == R) produce identical
            // taps on both sides — no bounce at all.
            delayL.pushSample (0, 0.5f * (inputL + inputR) + feedbackR * feedbackAmount);
            delayR.pushSample (0, feedbackL * feedbackAmount);
        }

        const float d = delaySamples.getNextValue();
        float wetL = delayL.popSample (0, d);
        float wetR = delayR.popSample (0, d);

        feedbackL = feedbackFilterL.processSample (0, wetL);
        feedbackR = feedbackFilterR.processSample (0, wetR);

        leftData[i] = wetL;
        // IN-14: in a mono block rightData aliases leftData; guard the second
        // write so the left channel keeps its own delay line's output. (No audible
        // change today — both lines get identical input/time so wetL == wetR — but
        // the intent is now explicit and left isn't clobbered by the right line.)
        if (block.getNumChannels() > 1)
            rightData[i] = wetR;
    }

    dryWetMixer.mixWetSamples (block);
}
