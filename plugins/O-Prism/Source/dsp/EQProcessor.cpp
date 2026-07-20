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

    prevLowGainDB = prevMidGainDB = prevMidFreqHz = prevHighGainDB = -999.0f;
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
    float lowGain = targetLowGainDB.load (std::memory_order_relaxed);
    float midGain = targetMidGainDB.load (std::memory_order_relaxed);
    float midFreq = targetMidFreqHz.load (std::memory_order_relaxed);
    float highGain = targetHighGainDB.load (std::memory_order_relaxed);

    // Recompute coefficients in place on the audio thread. ArrayCoeffs::makeXXX
    // returns a stack std::array<float,6>; assigning it into *state reuses the
    // storage allocated in prepare() — no ref-counted Coefficients heap alloc
    // per changed block, unlike FilterCoeffs::makeXXX() (CR-04).
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

    juce::dsp::ProcessContextReplacing<float> context (block);
    lowShelf.process (context);
    midPeak.process (context);
    highShelf.process (context);
}
