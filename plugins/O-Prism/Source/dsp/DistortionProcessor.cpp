/*
  ==============================================================================

    DistortionProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "DistortionProcessor.h"
#include "MathConstants.h"

DistortionProcessor::DistortionProcessor()
    : oversampling (2, 1, // 2 channels, 2^1 = 2x oversampling
                    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
{
}

void DistortionProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    oversampling.initProcessing (spec.maximumBlockSize);
    dryWetMixer.prepare (spec);
    dryWetMixer.setWetLatency (static_cast<float> (oversampling.getLatencyInSamples()));
}

void DistortionProcessor::reset()
{
    oversampling.reset();
    dryWetMixer.reset();
}

void DistortionProcessor::setType (int type)
{
    distType = type;
}

void DistortionProcessor::setDrive (float drive)
{
    driveAmount = drive;
}

void DistortionProcessor::setMix (float mix)
{
    dryWetMixer.setWetMixProportion (mix);
}

void DistortionProcessor::applyDistortion (juce::dsp::AudioBlock<float>& block)
{
    float gain = 1.0f + driveAmount * 9.0f;

    for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer (ch);
        auto numSamples = block.getNumSamples();

        for (size_t i = 0; i < numSamples; ++i)
        {
            float x = data[i] * gain;

            switch (distType)
            {
                case 0: // Soft Clip
                    data[i] = std::tanh (x);
                    break;
                case 1: // Hard Clip
                    data[i] = juce::jlimit (-1.0f, 1.0f, x);
                    break;
                case 2: // Tube (asymmetric)
                    if (x >= 0.0f)
                        data[i] = x / (1.0f + std::abs (x));
                    else
                        data[i] = std::tanh (x * 1.5f) / 1.5f;
                    break;
                case 3: // Fold
                    data[i] = static_cast<float> (std::sin (x * kPi));
                    break;
            }
        }
    }
}

void DistortionProcessor::process (juce::dsp::AudioBlock<float>& block)
{
    dryWetMixer.pushDrySamples (block);

    auto osBlock = oversampling.processSamplesUp (block);
    applyDistortion (osBlock);
    oversampling.processSamplesDown (block);

    dryWetMixer.mixWetSamples (block);
}
