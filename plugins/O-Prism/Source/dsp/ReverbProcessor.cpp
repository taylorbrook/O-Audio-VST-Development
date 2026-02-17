/*
  ==============================================================================

    ReverbProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "ReverbProcessor.h"

void ReverbProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = static_cast<float> (spec.sampleRate);

    reverb.prepare (spec);
    preDelayL.prepare (spec);
    preDelayR.prepare (spec);
    dryWetMixer.prepare (spec);

    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 1.0f;  // External mixer handles dry/wet
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters (reverbParams);
}

void ReverbProcessor::reset()
{
    reverb.reset();
    preDelayL.reset();
    preDelayR.reset();
    dryWetMixer.reset();
}

void ReverbProcessor::setSize (float size)
{
    reverbParams.roomSize = size;
    reverb.setParameters (reverbParams);
}

void ReverbProcessor::setDamping (float damp)
{
    reverbParams.damping = damp;
    reverb.setParameters (reverbParams);
}

void ReverbProcessor::setPredelay (float ms)
{
    preDelaySamples = ms * 0.001f * currentSampleRate;
}

void ReverbProcessor::setMix (float mix)
{
    dryWetMixer.setWetMixProportion (mix);
}

void ReverbProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    dryWetMixer.pushDrySamples (block);

    // Pre-delay
    if (preDelaySamples > 0.0f)
    {
        auto numSamples = block.getNumSamples();
        auto* leftData = block.getChannelPointer (0);
        auto* rightData = block.getNumChannels() > 1 ? block.getChannelPointer (1) : leftData;

        for (size_t i = 0; i < numSamples; ++i)
        {
            preDelayL.pushSample (0, leftData[i]);
            preDelayR.pushSample (0, rightData[i]);
            leftData[i] = preDelayL.popSample (0, preDelaySamples);
            rightData[i] = preDelayR.popSample (0, preDelaySamples);
        }
    }

    // Reverb
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);

    dryWetMixer.mixWetSamples (block);
}
