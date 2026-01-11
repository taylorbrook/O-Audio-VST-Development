/*
  ==============================================================================

    OuariconComp - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconCompAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // threshold - Compression threshold level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "threshold", 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        "dB"
    ));

    // ratio - Compression ratio
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ratio", 1 },
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
        4.0f,
        ":1"
    ));

    // attack_time - Attack time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attack_time", 1 },
        "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
        10.0f,
        "ms"
    ));

    // release_time - Release time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "release_time", 1 },
        "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f),
        100.0f,
        "ms"
    ));

    // knee - Knee width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "knee", 1 },
        "Knee",
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f),
        6.0f,
        "dB"
    ));

    // output_gain - Output makeup gain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // auto_gain - Automatic makeup gain
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "auto_gain", 1 },
        "Auto Gain",
        false
    ));

    return layout;
}

OuariconCompAudioProcessor::OuariconCompAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconCompAudioProcessor::~OuariconCompAudioProcessor()
{
}

void OuariconCompAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // DSP initialization will be added in Stage 2 (DSP)
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OuariconCompAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2 (DSP)
}

void OuariconCompAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Parameter access example (for Stage 2 DSP implementation):
    // auto* thresholdParam = parameters.getRawParameterValue("threshold");
    // float thresholdValue = thresholdParam->load();  // Atomic read (real-time safe)

    // Pass-through for Stage 1 (DSP implementation happens in Stage 2)
    // Audio routing is already handled by JUCE
}

juce::AudioProcessorEditor* OuariconCompAudioProcessor::createEditor()
{
    return new OuariconCompAudioProcessorEditor(*this);
}

void OuariconCompAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconCompAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconCompAudioProcessor();
}
