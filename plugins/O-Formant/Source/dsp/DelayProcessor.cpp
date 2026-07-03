/*
  ==============================================================================

    DelayProcessor.cpp
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DelayProcessor.h"

DelayProcessor::DelayProcessor() = default;

void DelayProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    delayL.prepare (spec);
    delayR.prepare (spec);
    feedbackFilterL.prepare (spec);
    feedbackFilterR.prepare (spec);
    dryWetMixer.prepare (spec);

    feedbackFilterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    feedbackFilterL.setCutoffFrequency (8000.0f);
    feedbackFilterR.setCutoffFrequency (8000.0f);

    delaySamples = 0.375f * currentSampleRate;
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
    // Clamp to the delay line's capacity. The lines are fixed at 192000 samples,
    // so a 2.0 s request above 96 kHz would otherwise exceed the buffer and
    // silently alias (popSample masks by % totalSize) to a wrong, shorter time
    // — and trip the jassert in Debug builds. (REVIEW.md WR-07)
    float requested = seconds * currentSampleRate;
    delaySamples = juce::jmin (requested,
                               static_cast<float> (delayL.getMaximumDelayInSamples()));
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

        float wetL = delayL.popSample (0, delaySamples);
        float wetR = delayR.popSample (0, delaySamples);

        feedbackL = feedbackFilterL.processSample (0, wetL);
        feedbackR = feedbackFilterR.processSample (0, wetR);

        leftData[i] = wetL;
        // IN-14: in a mono block rightData aliases leftData; guard the second
        // write so the left channel keeps its own delay line's output. (No audible
        // change today — both lines get identical input/time so wetL == wetR — but
        // the intent is now explicit and left isn't clobbered by the right line.)
        if (block.getNumChannels() > 1)
            rightData[i] = wetR;
    }

    dryWetMixer.mixWetSamples (block);
}
