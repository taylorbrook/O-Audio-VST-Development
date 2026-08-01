/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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
    O-Bells - Physical Modeling Bell Synthesizer
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

    // Apply initial coefficients
    *lowShelf.state = *FilterCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetLowGainDB.load()));
    *midPeak.state = *FilterCoeffs::makePeakFilter (
        currentSampleRate, targetMidFreqHz.load(), 1.0f,
        juce::Decibels::decibelsToGain (targetMidGainDB.load()));
    *highShelf.state = *FilterCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetHighGainDB.load()));
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

    // CR-02: update coefficients in place with the stack-allocating
    // ArrayCoefficients factories. The ref-counted Coefficients::makeXXX
    // factories heap-allocate a new object every call — a malloc+free on the
    // audio thread each block while a gain/freq is dragged or automated.
    // ArrayCoefficients returns a std::array<float,6> (identical math) that we
    // copy into the already-allocated storage. (pattern_arraycoefficients_rt_safe_iir)
    if (lowGain != prevLowGainDB)
    {
        auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (
            currentSampleRate, 200.0f, 0.707f,
            juce::Decibels::decibelsToGain (lowGain));
        std::copy (c.begin(), c.end(), lowShelf.state->getRawCoefficients());
        prevLowGainDB = lowGain;
    }

    if (midGain != prevMidGainDB || midFreq != prevMidFreqHz)
    {
        auto c = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
            currentSampleRate, midFreq, 1.0f,
            juce::Decibels::decibelsToGain (midGain));
        std::copy (c.begin(), c.end(), midPeak.state->getRawCoefficients());
        prevMidGainDB = midGain;
        prevMidFreqHz = midFreq;
    }

    if (highGain != prevHighGainDB)
    {
        auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
            currentSampleRate, 8000.0f, 0.707f,
            juce::Decibels::decibelsToGain (highGain));
        std::copy (c.begin(), c.end(), highShelf.state->getRawCoefficients());
        prevHighGainDB = highGain;
    }

    juce::dsp::ProcessContextReplacing<float> context (block);
    lowShelf.process (context);
    midPeak.process (context);
    highShelf.process (context);
}
