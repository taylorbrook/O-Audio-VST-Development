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

    EQProcessor.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "EQProcessor.h"

void EQProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);
    lowShelf.prepare (spec);
    midPeak.prepare (spec);
    highShelf.prepare (spec);

    // Assign from the stack ArrayCoefficients factory. On the message thread this
    // also grows each Coefficients array to its full storage, so the in-place
    // assignments in process() below never reallocate on the audio thread.
    *lowShelf.state = ArrayCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetLowGainDB.load()));
    *midPeak.state = ArrayCoeffs::makePeakFilter (
        currentSampleRate, targetMidFreqHz.load(), 1.0f,
        juce::Decibels::decibelsToGain (targetMidGainDB.load()));
    *highShelf.state = ArrayCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetHighGainDB.load()));

    // IN-15: 20 ms glide on every band control. Multiplicative for the
    // frequency so a sweep moves evenly in octaves.
    lowGainSm.reset (spec.sampleRate, 0.02);
    midGainSm.reset (spec.sampleRate, 0.02);
    highGainSm.reset (spec.sampleRate, 0.02);
    midFreqSm.reset (spec.sampleRate, 0.02);
    snapOnNextProcess = true;
}

void EQProcessor::reset()
{
    lowShelf.reset();
    midPeak.reset();
    highShelf.reset();
    // Re-enable path: the caller resets before it pushes the current targets,
    // so snap on the next process() rather than glide from stale values.
    snapOnNextProcess = true;
}

void EQProcessor::setLowGain (float dB)  { targetLowGainDB.store (dB, std::memory_order_relaxed); }
void EQProcessor::setMidGain (float dB)  { targetMidGainDB.store (dB, std::memory_order_relaxed); }
void EQProcessor::setMidFreq (float hz)  { targetMidFreqHz.store (hz, std::memory_order_relaxed); }
void EQProcessor::setHighGain (float dB) { targetHighGainDB.store (dB, std::memory_order_relaxed); }

void EQProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    const float lowGain  = targetLowGainDB.load (std::memory_order_relaxed);
    const float midGain  = targetMidGainDB.load (std::memory_order_relaxed);
    const float midFreq  = juce::jmax (1.0f, targetMidFreqHz.load (std::memory_order_relaxed));
    const float highGain = targetHighGainDB.load (std::memory_order_relaxed);

    if (snapOnNextProcess)
    {
        snapOnNextProcess = false;
        lowGainSm.setCurrentAndTargetValue (lowGain);
        midGainSm.setCurrentAndTargetValue (midGain);
        midFreqSm.setCurrentAndTargetValue (midFreq);
        highGainSm.setCurrentAndTargetValue (highGain);
    }
    else
    {
        lowGainSm.setTargetValue (lowGain);
        midGainSm.setTargetValue (midGain);
        midFreqSm.setTargetValue (midFreq);
        highGainSm.setTargetValue (highGain);
    }

    const auto numSamples = block.getNumSamples();
    size_t pos = 0;

    // v1.31.2 (review IN-15): step the coefficients along the smoothed values
    // in kCoeffInterval sub-blocks while any control is gliding; once settled,
    // one update (skipped when unchanged) covers the rest of the block.
    while (pos < numSamples)
    {
        const bool smoothing = lowGainSm.isSmoothing() || midGainSm.isSmoothing()
                            || midFreqSm.isSmoothing() || highGainSm.isSmoothing();
        const size_t len = smoothing
            ? std::min (static_cast<size_t> (kCoeffInterval), numSamples - pos)
            : numSamples - pos;

        if (smoothing)
        {
            const int n = static_cast<int> (len);
            lowGainSm.skip (n);
            midGainSm.skip (n);
            midFreqSm.skip (n);
            highGainSm.skip (n);
        }
        updateCoefficients();

        auto sub = block.getSubBlock (pos, len);
        juce::dsp::ProcessContextReplacing<float> context (sub);
        lowShelf.process (context);
        midPeak.process (context);
        highShelf.process (context);
        pos += len;
    }
}

void EQProcessor::updateCoefficients() noexcept
{
    const float lowGain  = lowGainSm.getCurrentValue();
    const float midGain  = midGainSm.getCurrentValue();
    const float midFreq  = midFreqSm.getCurrentValue();
    const float highGain = highGainSm.getCurrentValue();

    // Recompute coefficients in place on the audio thread. ArrayCoeffs::makeXXX
    // returns a stack std::array<float,6>; assigning it into *state reuses the
    // storage allocated in prepare() — no ref-counted Coefficients heap alloc
    // per update, unlike FilterCoeffs::makeXXX() (WR-08).
    if (lowGain != prevLowGainDB)
    {
        *lowShelf.state = ArrayCoeffs::makeLowShelf (
            currentSampleRate, 200.0f, 0.707f,
            juce::Decibels::decibelsToGain (lowGain));
        prevLowGainDB = lowGain;
    }

    if (midGain != prevMidGainDB || midFreq != prevMidFreqHz)
    {
        *midPeak.state = ArrayCoeffs::makePeakFilter (
            currentSampleRate, midFreq, 1.0f,
            juce::Decibels::decibelsToGain (midGain));
        prevMidGainDB = midGain;
        prevMidFreqHz = midFreq;
    }

    if (highGain != prevHighGainDB)
    {
        *highShelf.state = ArrayCoeffs::makeHighShelf (
            currentSampleRate, 8000.0f, 0.707f,
            juce::Decibels::decibelsToGain (highGain));
        prevHighGainDB = highGain;
    }
}
