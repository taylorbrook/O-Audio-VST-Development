/*
  ==============================================================================

    Ouaricon Marimba - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MicroMarimbaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MALLET_HARDNESS - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MALLET_HARDNESS", 1 },
        "Mallet Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // BAR_MATERIAL - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BAR_MATERIAL", 1 },
        "Bar Material",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // RESONANCE - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "RESONANCE", 1 },
        "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    // TUNING_MODE - Choice (0-2)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "TUNING_MODE", 1 },
        "Tuning Mode",
        juce::StringArray { "12-TET", "Scala", "MTS-ESP" },
        0  // Default: 12-TET
    ));

    // REFERENCE_PITCH - Float (400.0 to 480.0 Hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "REFERENCE_PITCH", 1 },
        "Reference Pitch",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    // VEL_CURVE - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VEL_CURVE", 1 },
        "Velocity Curve",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // OUTPUT_GAIN - Float (-24.0 to 12.0 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    return layout;
}

MicroMarimbaAudioProcessor::MicroMarimbaAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

MicroMarimbaAudioProcessor::~MicroMarimbaAudioProcessor()
{
}

void MicroMarimbaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialization will be added in Stage 2 (DSP)
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void MicroMarimbaAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2 (DSP)
}

void MicroMarimbaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer (synth generates audio from scratch, no input)
    buffer.clear();

    // MIDI will be processed in Stage 2 when synthesizer is implemented
    juce::ignoreUnused(midiMessages);

    // Parameter access example (for Stage 2 DSP implementation):
    // auto* malletHardness = parameters.getRawParameterValue("MALLET_HARDNESS");
    // float hardnessValue = malletHardness->load();  // Atomic read (real-time safe)

    // Stage 1: Silent output (DSP implementation happens in Stage 2)
}

juce::AudioProcessorEditor* MicroMarimbaAudioProcessor::createEditor()
{
    return new MicroMarimbaAudioProcessorEditor(*this);
}

void MicroMarimbaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MicroMarimbaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicroMarimbaAudioProcessor();
}
