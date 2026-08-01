/*
   This file is part of O-Lyrica, an Ouaricon Audio plugin.
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
    O-Lyrica - Physical Modeling Harp Synthesizer
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

    // Apply initial coefficients via the ArrayCoefficients operator= (Coefficients::operator=
    // (std::array) → assignImpl). Doing it here off the audio thread ALSO grows the coefficient
    // array's storage to capacity ≥8 (ensureStorageAllocated), so the identical assignments in
    // process() reuse the buffer with NO heap alloc (CR-03).
    *lowShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetLowGainDB.load()));
    *midPeak.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
        currentSampleRate, targetMidFreqHz.load(), 1.0f,
        juce::Decibels::decibelsToGain (targetMidGainDB.load()));
    *highShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetHighGainDB.load()));

    // IN-07: seed the dirty-flag cache so the first post-prepare block does NOT force a
    // spurious (heap-allocating) coefficient rebuild.
    prevLowGainDB  = targetLowGainDB.load();
    prevMidGainDB  = targetMidGainDB.load();
    prevMidFreqHz  = targetMidFreqHz.load();
    prevHighGainDB = targetHighGainDB.load();
}

void EQProcessor::reset()
{
    lowShelf.reset();
    midPeak.reset();
    highShelf.reset();
}

void EQProcessor::setLowGain (float dB)  { targetLowGainDB.store (dB, std::memory_order_relaxed); }
void EQProcessor::setMidGain (float dB)  { targetMidGainDB.store (dB, std::memory_order_relaxed); }
void EQProcessor::setMidFreq (float hz)  { targetMidFreqHz.store (hz, std::memory_order_relaxed); }
void EQProcessor::setHighGain (float dB) { targetHighGainDB.store (dB, std::memory_order_relaxed); }

void EQProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    // Read atomic targets and update coefficients only when changed
    float lowGain = targetLowGainDB.load (std::memory_order_relaxed);
    float midGain = targetMidGainDB.load (std::memory_order_relaxed);
    float midFreq = targetMidFreqHz.load (std::memory_order_relaxed);
    float highGain = targetHighGainDB.load (std::memory_order_relaxed);

    // CR-03: RT-safe coefficient updates. Assigning an ArrayCoefficients std::array uses
    // Coefficients::operator=(std::array) → assignImpl, which reuses the already-allocated
    // coefficient buffer (capacity ≥8 established in prepare) — NO heap alloc/free on the audio
    // thread. This replaces the previous *state = *Coefficients::makeXXX(...) which heap-allocated
    // (and freed) a Coefficients object every automated block.
    if (lowGain != prevLowGainDB)
    {
        *lowShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (
            currentSampleRate, 200.0f, 0.707f,
            juce::Decibels::decibelsToGain (lowGain));
        prevLowGainDB = lowGain;
    }

    if (midGain != prevMidGainDB || midFreq != prevMidFreqHz)
    {
        *midPeak.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
            currentSampleRate, midFreq, 1.0f,
            juce::Decibels::decibelsToGain (midGain));
        prevMidGainDB = midGain;
        prevMidFreqHz = midFreq;
    }

    if (highGain != prevHighGainDB)
    {
        *highShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
            currentSampleRate, 8000.0f, 0.707f,
            juce::Decibels::decibelsToGain (highGain));
        prevHighGainDB = highGain;
    }

    juce::dsp::ProcessContextReplacing<float> context (block);
    lowShelf.process (context);
    midPeak.process (context);
    highShelf.process (context);
}
