/*
  ==============================================================================

    EQProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
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

    updateLowShelf();
    updateMidPeak();
    updateHighShelf();
}

void EQProcessor::reset()
{
    lowShelf.reset();
    midPeak.reset();
    highShelf.reset();
}

void EQProcessor::setLowGain (float dB)
{
    lowGainDB = dB;
    updateLowShelf();
}

void EQProcessor::setMidGain (float dB)
{
    midGainDB = dB;
    updateMidPeak();
}

void EQProcessor::setMidFreq (float hz)
{
    midFreqHz = hz;
    updateMidPeak();
}

void EQProcessor::setHighGain (float dB)
{
    highGainDB = dB;
    updateHighShelf();
}

void EQProcessor::updateLowShelf()
{
    *lowShelf.state = *FilterCoeffs::makeLowShelf (
        currentSampleRate, 200.0f, 0.707f,
        juce::Decibels::decibelsToGain (lowGainDB));
}

void EQProcessor::updateMidPeak()
{
    *midPeak.state = *FilterCoeffs::makePeakFilter (
        currentSampleRate, midFreqHz, 1.0f,
        juce::Decibels::decibelsToGain (midGainDB));
}

void EQProcessor::updateHighShelf()
{
    *highShelf.state = *FilterCoeffs::makeHighShelf (
        currentSampleRate, 8000.0f, 0.707f,
        juce::Decibels::decibelsToGain (highGainDB));
}

void EQProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> context (block);
    lowShelf.process (context);
    midPeak.process (context);
    highShelf.process (context);
}
