/*
  ==============================================================================

    Ouaricon Digital Delay - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconDigitalDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // time - Float (1.0-2000.0 ms, default: 500.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "time", 1 },
        "Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 1.0f),
        500.0f,
        "ms"
    ));

    // sync - Bool (default: false)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "sync", 1 },
        "Sync",
        false
    ));

    // division - Choice (12 options, default: 1 "1/8")
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "division", 1 },
        "Division",
        juce::StringArray { "1/4", "1/8", "1/16", "1/4D", "1/8D", "1/16D",
                           "1/4T", "1/8T", "1/16T", "1/4(5)", "1/8(5)", "1/16(5)" },
        1  // Default: 1/8
    ));

    // feedback - Float (0.0-100.0%, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // spread - Float (0.0-100.0%, default: 0.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // mod - Float (0.0-100.0%, default: 0.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mod", 1 },
        "Mod",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // wet - Float (0.0-100.0%, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wet", 1 },
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // dry - Float (0.0-100.0%, default: 100.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dry", 1 },
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        100.0f,
        "%"
    ));

    return layout;
}

OuariconDigitalDelayAudioProcessor::OuariconDigitalDelayAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconDigitalDelayAudioProcessor::~OuariconDigitalDelayAudioProcessor()
{
}

void OuariconDigitalDelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // DSP initialization will be added in Stage 2 (DSP implementation)
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OuariconDigitalDelayAudioProcessor::releaseResources()
{
    // DSP cleanup will be added in Stage 2 (DSP implementation)
}

void OuariconDigitalDelayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Parameter access example (for Stage 2 DSP implementation):
    // auto* timeParam = parameters.getRawParameterValue("time");
    // float timeValue = timeParam->load();  // Atomic read (real-time safe)

    // Pass-through for Stage 1 (DSP implementation happens in Stage 2)
    // Audio routing is already handled by JUCE
}

juce::AudioProcessorEditor* OuariconDigitalDelayAudioProcessor::createEditor()
{
    return new OuariconDigitalDelayAudioProcessorEditor(*this);
}

void OuariconDigitalDelayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconDigitalDelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconDigitalDelayAudioProcessor();
}
