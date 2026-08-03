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

    EQProcessor.cpp
    O-IntonationPad - Wavetable Pad Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "EQProcessor.h"
#include <algorithm>

void EQProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);
    lowShelf.prepare (spec);
    midPeak.prepare (spec);
    highShelf.prepare (spec);

    // One-time full assignment (non-RT). Assigning the raw 6-element ArrayCoefficients
    // through Coefficients::operator=(std::array) normalises by a0 AND reserves the
    // internal Array's storage (>= 8 slots), so the audio-thread path in
    // updateCoefficients() can reuse the same operator= without ever allocating.
    float lowGain  = targetLowGainDB.load (std::memory_order_relaxed);
    float midGain  = targetMidGainDB.load (std::memory_order_relaxed);
    float midFreq  = targetMidFreqHz.load (std::memory_order_relaxed);
    float highGain = targetHighGainDB.load (std::memory_order_relaxed);

    using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;
    *lowShelf.state = ArrayCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain (lowGain));
    *midPeak.state = ArrayCoeffs::makePeakFilter (
        currentSampleRate, midFreq, 1.0f, juce::Decibels::decibelsToGain (midGain));
    *highShelf.state = ArrayCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain (highGain));

    appliedLowGainDB  = lowGain;
    appliedMidGainDB  = midGain;
    appliedMidFreqHz  = midFreq;
    appliedHighGainDB = highGain;
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

void EQProcessor::updateCoefficients()
{
    float lowGain = targetLowGainDB.load (std::memory_order_relaxed);
    float midGain = targetMidGainDB.load (std::memory_order_relaxed);
    float midFreq = targetMidFreqHz.load (std::memory_order_relaxed);
    float highGain = targetHighGainDB.load (std::memory_order_relaxed);

    if (lowGain == appliedLowGainDB && midGain == appliedMidGainDB
        && midFreq == appliedMidFreqHz && highGain == appliedHighGainDB)
        return;

    // CR-04: RT-safe coefficient update. ArrayCoefficients returns a stack
    // std::array<float,6> of RAW {b0,b1,b2,a0,a1,a2}; Coefficients stores 5
    // NORMALISED values (each divided by a0, a0 itself dropped). Assign through
    // Coefficients::operator=(std::array), which performs that normalisation —
    // a raw std::copy of 6 values mis-aligns the feedback polynomial (a0 lands
    // in the a1 slot) and the filter goes unstable to Inf on the first update
    // (caught by Windows CI pluginval fuzz, v2.8.3). operator= is allocation-free
    // here: prepare() already reserved the Array's >= 8-slot storage, and
    // clearQuick()+add() never reallocate within capacity.
    // See pattern_arraycoefficients_rt_safe_iir.
    using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;
    *lowShelf.state = ArrayCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain (lowGain));
    *midPeak.state = ArrayCoeffs::makePeakFilter (
        currentSampleRate, midFreq, 1.0f, juce::Decibels::decibelsToGain (midGain));
    *highShelf.state = ArrayCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain (highGain));

    appliedLowGainDB = lowGain;
    appliedMidGainDB = midGain;
    appliedMidFreqHz = midFreq;
    appliedHighGainDB = highGain;
}

void EQProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    updateCoefficients();

    juce::dsp::ProcessContextReplacing<float> context (block);
    lowShelf.process (context);
    midPeak.process (context);
    highShelf.process (context);
}
