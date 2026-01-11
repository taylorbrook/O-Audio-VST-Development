/*
  ==============================================================================

    BodyResonance.cpp
    Phase 2.4: Convolution-based body resonance implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BodyResonance.h"

BodyResonance::BodyResonance()
{
}

void BodyResonance::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Generate synthetic IR at current sample rate
    generateSyntheticIR();

    // Prepare convolution
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;  // Stereo

    convolution.prepare(spec);

    // Load synthetic IR into convolution engine
    // Clone mono IR to stereo for convolution
    juce::AudioBuffer<float> stereoIR(2, irBuffer.getNumSamples());
    stereoIR.copyFrom(0, 0, irBuffer, 0, 0, irBuffer.getNumSamples());
    stereoIR.copyFrom(1, 0, irBuffer, 0, 0, irBuffer.getNumSamples());

    convolution.loadImpulseResponse(
        std::move(stereoIR),
        currentSampleRate,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::yes
    );

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);
    dryWetMixer.setWetMixProportion(mix);
}

void BodyResonance::reset()
{
    convolution.reset();
    dryWetMixer.reset();
}

void BodyResonance::process(juce::AudioBuffer<float>& buffer)
{
    // Push dry signal
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);

    // Process through convolution
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);

    // Mix dry/wet
    dryWetMixer.mixWetSamples(block);
}

void BodyResonance::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
    dryWetMixer.setWetMixProportion(mix);
}

void BodyResonance::generateSyntheticIR()
{
    // Generate ~75ms IR at current sample rate
    const int irLength = static_cast<int>(currentSampleRate * 0.075);
    irBuffer.setSize(1, irLength);  // Mono IR
    irBuffer.clear();

    auto* ir = irBuffer.getWritePointer(0);

    // Generate resonator tube characteristics:
    // - Initial transient (wood impact)
    // - Low-mid resonance (200-800 Hz emphasis)
    // - Exponential decay

    juce::Random random;

    for (int i = 0; i < irLength; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(irLength);

        // Exponential decay envelope
        float envelope = std::exp(-t * 6.0f);

        // Add resonant modes (simulating tube resonances)
        float sample = 0.0f;

        // Primary resonance (~300 Hz)
        float freq1 = 300.0f;
        sample += 0.6f * std::sin(2.0f * juce::MathConstants<float>::pi * freq1 * i / static_cast<float>(currentSampleRate));

        // Secondary resonance (~500 Hz)
        float freq2 = 500.0f;
        sample += 0.3f * std::sin(2.0f * juce::MathConstants<float>::pi * freq2 * i / static_cast<float>(currentSampleRate));

        // Tertiary resonance (~750 Hz)
        float freq3 = 750.0f;
        sample += 0.15f * std::sin(2.0f * juce::MathConstants<float>::pi * freq3 * i / static_cast<float>(currentSampleRate));

        // Add slight noise for diffusion
        sample += 0.05f * (random.nextFloat() * 2.0f - 1.0f);

        ir[i] = sample * envelope;
    }

    // Normalize IR
    float maxVal = irBuffer.getMagnitude(0, 0, irBuffer.getNumSamples());
    if (maxVal > 0.0f)
        irBuffer.applyGain(0, 0, irBuffer.getNumSamples(), 0.5f / maxVal);
}
