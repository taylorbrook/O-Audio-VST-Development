/*
  ==============================================================================

    EQProcessor.cpp
    O-IntonationPad - Wavetable Pad Synthesizer
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

    // Force coefficient rebuild after prepare
    appliedLowGainDB = -999.0f;
    updateCoefficients();
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

    *lowShelf.state = *FilterCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f,
        juce::Decibels::decibelsToGain (lowGain));
    *midPeak.state = *FilterCoeffs::makePeakFilter (
        currentSampleRate, midFreq, 1.0f,
        juce::Decibels::decibelsToGain (midGain));
    *highShelf.state = *FilterCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f,
        juce::Decibels::decibelsToGain (highGain));

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
