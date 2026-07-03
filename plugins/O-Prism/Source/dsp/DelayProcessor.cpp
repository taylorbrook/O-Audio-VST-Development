/*
  ==============================================================================

    DelayProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DelayProcessor.h"

DelayProcessor::DelayProcessor() = default;

void DelayProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    // Size from the actual sample rate — the compile-time {192000} capacity
    // only covers 2 s up to 96 kHz and silently clamps at 192 kHz (WR-05)
    const int maxDelaySamples = static_cast<int> (std::ceil (kMaxDelaySeconds * spec.sampleRate)) + 8;
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

    delaySamples.reset (spec.sampleRate, 0.05);
    delaySamples.setCurrentAndTargetValue (0.375f * currentSampleRate);
}

void DelayProcessor::reset()
{
    delayL.reset();
    delayR.reset();
    feedbackFilterL.reset();
    feedbackFilterR.reset();
    dryWetMixer.reset();
    feedbackL = feedbackR = 0.0f;
}

void DelayProcessor::setTime (float seconds)
{
    delaySamples.setTargetValue (juce::jlimit (0.0f, kMaxDelaySeconds, seconds) * currentSampleRate);
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
            delayL.pushSample (0, inputL + feedbackR * feedbackAmount);
            delayR.pushSample (0, inputR + feedbackL * feedbackAmount);
        }

        const float delayPos = delaySamples.getNextValue();
        float wetL = delayL.popSample (0, delayPos);
        float wetR = delayR.popSample (0, delayPos);

        feedbackL = feedbackFilterL.processSample (0, wetL);
        feedbackR = feedbackFilterR.processSample (0, wetR);

        leftData[i] = wetL;
        rightData[i] = wetR;
    }

    dryWetMixer.mixWetSamples (block);
}
