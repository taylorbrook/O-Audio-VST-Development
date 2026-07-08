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

    // One-time full assignment (non-RT) allocates the 6-element coefficient arrays
    // so the audio-thread path in updateCoefficients() can copy into them in place.
    float lowGain  = targetLowGainDB.load (std::memory_order_relaxed);
    float midGain  = targetMidGainDB.load (std::memory_order_relaxed);
    float midFreq  = targetMidFreqHz.load (std::memory_order_relaxed);
    float highGain = targetHighGainDB.load (std::memory_order_relaxed);

    *lowShelf.state = *FilterCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain (lowGain));
    *midPeak.state = *FilterCoeffs::makePeakFilter (
        currentSampleRate, midFreq, 1.0f, juce::Decibels::decibelsToGain (midGain));
    *highShelf.state = *FilterCoeffs::makeHighShelf (
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
    // std::array<float,6> (no heap alloc, unlike Coefficients::makeXXX which
    // returns a new-allocated ref-counted Ptr); copy the 6 values in place into
    // the arrays that prepare() already sized. See pattern_arraycoefficients_rt_safe_iir.
    using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;
    const auto ls = ArrayCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain (lowGain));
    const auto mp = ArrayCoeffs::makePeakFilter (
        currentSampleRate, midFreq, 1.0f, juce::Decibels::decibelsToGain (midGain));
    const auto hs = ArrayCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain (highGain));

    std::copy (ls.begin(), ls.end(), lowShelf.state->getRawCoefficients());
    std::copy (mp.begin(), mp.end(), midPeak.state->getRawCoefficients());
    std::copy (hs.begin(), hs.end(), highShelf.state->getRawCoefficients());

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
