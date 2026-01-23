/*
  ==============================================================================

    OBass - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Bass enhancement plugin with crossover filtering and harmonic generation.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OBassAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // crossover_freq - Crossover frequency for bass/high split
    // Range: 40-200Hz, default 80Hz, with frequency skew for natural feel
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crossover_freq", 1 },
        "Crossover",
        juce::NormalisableRange<float>(40.0f, 200.0f, 1.0f, 0.5f),  // skew 0.5 for frequency
        80.0f,
        "Hz"
    ));

    // latency_mode - Processing quality/latency tradeoff
    // 0 = Low Latency (minimum-phase IIR), 1 = High Fidelity (linear-phase FIR)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "latency_mode", 1 },
        "Mode",
        juce::StringArray { "Low Latency", "High Fidelity" },
        0  // Default: Low Latency
    ));

    // bypass - Full plugin bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false
    ));

    return layout;
}

OBassAudioProcessor::OBassAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OBassAudioProcessor::~OBassAudioProcessor()
{
}

void OBassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store DSP spec for use by processors
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Crossover filter preparation will be added in Plan 01-02
    // Mono summer preparation will be added in Plan 01-03
}

void OBassAudioProcessor::releaseResources()
{
    // Release crossover filter resources (Plan 01-02)
    // Release intermediate buffers (Plan 01-03)
}

void OBassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Check bypass - if true, return immediately (true bypass, no processing)
    auto* bypassParam = parameters.getRawParameterValue("bypass");
    if (bypassParam->load() > 0.5f)
        return;

    //==========================================================================
    // Current: Audio pass-through (unity gain)
    // Plan 01-02 adds: Crossover filtering (split low/high)
    // Plan 01-03 adds: Mono summing of bass content
    // Phase 2 adds: Harmonic generation
    //==========================================================================

    // For now, audio passes through unchanged (unity gain)
    // No processing required - buffer already contains input audio
}

juce::AudioProcessorEditor* OBassAudioProcessor::createEditor()
{
    return new OBassAudioProcessorEditor(*this);
}

void OBassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state using APVTS
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state using APVTS
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function - creates plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBassAudioProcessor();
}
