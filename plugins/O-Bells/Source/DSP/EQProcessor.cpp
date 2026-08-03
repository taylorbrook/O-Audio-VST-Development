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

    // Apply initial coefficients. Assigning the raw 6-element ArrayCoefficients
    // through Coefficients::operator=(std::array) normalises by a0 AND reserves
    // the internal Array's >= 8-slot storage, so the audio-thread updates in
    // process() can reuse the same operator= without ever allocating.
    *lowShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f,
        juce::Decibels::decibelsToGain (targetLowGainDB.load()));
    *midPeak.state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (
        currentSampleRate, targetMidFreqHz.load(), 1.0f,
        juce::Decibels::decibelsToGain (targetMidGainDB.load()));
    *highShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
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
    // ArrayCoefficients returns a RAW std::array<float,6> {b0,b1,b2,a0,a1,a2};
    // Coefficients stores 5 NORMALISED values (divided by a0, a0 dropped), so
    // assign through Coefficients::operator=(std::array) — a raw std::copy of
    // 6 values mis-aligns the feedback polynomial and the filter goes unstable
    // to Inf on the first update (O-IntonationPad v2.8.4, Windows CI pluginval
    // fuzz). operator= is allocation-free here: prepare() reserved the >= 8-slot
    // storage. (pattern_arraycoefficients_rt_safe_iir)
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
