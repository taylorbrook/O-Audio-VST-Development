/*
  ==============================================================================

    ReverbProcessor.cpp
    O-IntonationPad - Wavetable Pad Synthesizer
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

    // Force parameter application after prepare
    appliedSize = -1.0f;
    applyReverbParams();
}

void ReverbProcessor::reset()
{
    reverb.reset();
    preDelayL.reset();
    preDelayR.reset();
    dryWetMixer.reset();
}

void ReverbProcessor::setSize (float size)   { targetSize.store (size, std::memory_order_relaxed); }
void ReverbProcessor::setDamping (float damp) { targetDamping.store (damp, std::memory_order_relaxed); }
void ReverbProcessor::setPredelay (float ms)  { targetPredelayMs.store (ms, std::memory_order_relaxed); }
void ReverbProcessor::setMix (float mix)      { targetMix.store (mix, std::memory_order_relaxed); }

void ReverbProcessor::applyReverbParams()
{
    float size = targetSize.load (std::memory_order_relaxed);
    float damping = targetDamping.load (std::memory_order_relaxed);

    if (size == appliedSize && damping == appliedDamping)
        return;

    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = size;
    reverbParams.damping = damping;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters (reverbParams);

    appliedSize = size;
    appliedDamping = damping;
}

void ReverbProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    applyReverbParams();

    float predelayMs = targetPredelayMs.load (std::memory_order_relaxed);
    float mix = targetMix.load (std::memory_order_relaxed);
    float preDelaySamples = predelayMs * 0.001f * currentSampleRate;
    dryWetMixer.setWetMixProportion (mix);

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
