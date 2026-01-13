/*
  ==============================================================================

    OuariconSimpleReverb - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconSimpleReverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // TYPE - Choice parameter (6 options: Booth, Room, Hall, Spring, Plate, Ambient)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "TYPE", 1 },
        "Type",
        juce::StringArray { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" },
        1  // Default: Room (index 1)
    ));

    // CHARACTER - Bipolar float (-100% to +100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CHARACTER", 1 },
        "Character",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f  // Default: Neutral
    ));

    // WET - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "WET", 1 },
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f  // Default: 25%
    ));

    // DRY - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRY", 1 },
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f  // Default: 100%
    ));

    // DECAY - Float (0.1s to 10.0s, logarithmic skew)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),  // Skew 0.5 for logarithmic
        1.5f  // Default: 1.5s
    ));

    // SIZE - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SIZE", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f  // Default: 50% (Medium)
    ));

    return layout;
}

OuariconSimpleReverbAudioProcessor::OuariconSimpleReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconSimpleReverbAudioProcessor::~OuariconSimpleReverbAudioProcessor()
{
}

void OuariconSimpleReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare reverb with Room preset (Phase 2.1: Core Processing)
    reverb.prepare(spec);
    reverb.reset();

    // Set initial reverb parameters (Room type preset)
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.5f;      // Room type base
    reverbParams.damping = 0.5f;       // Room type base
    reverbParams.width = 1.0f;         // Full stereo
    reverbParams.wetLevel = 0.25f;     // Default 25% wet
    reverbParams.dryLevel = 1.0f;      // Default 100% dry
    reverbParams.freezeMode = 0.0f;    // Disabled
    reverb.setParameters(reverbParams);
}

void OuariconSimpleReverbAudioProcessor::releaseResources()
{
    // Optional: Release resources when plugin not in use
    // JUCE DSP components handle their own cleanup
}

void OuariconSimpleReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Early return for zero-length buffers
    if (buffer.getNumSamples() == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    auto* sizeParam = parameters.getRawParameterValue("SIZE");
    auto* decayParam = parameters.getRawParameterValue("DECAY");
    auto* wetParam = parameters.getRawParameterValue("WET");
    auto* dryParam = parameters.getRawParameterValue("DRY");

    float sizeValue = sizeParam->load();      // 0-100%
    float decayValue = decayParam->load();    // 0.1-10s
    float wetValue = wetParam->load();        // 0-100%
    float dryValue = dryParam->load();        // 0-100%

    // Convert parameter values to DSP ranges
    // SIZE: 0-100% scales roomSize (0.5-1.5x base value of 0.5)
    float baseRoomSize = 0.5f;  // Room type preset
    float finalRoomSize = baseRoomSize * (0.5f + (sizeValue / 100.0f) * 0.5f);
    finalRoomSize = juce::jlimit(0.0f, 1.0f, finalRoomSize);  // Clamp to valid range

    // DECAY: 0.1-10s maps to damping inverse (longer decay = less damping)
    float finalDamping = 1.0f - (decayValue / 10.0f) * 0.8f;
    finalDamping = juce::jlimit(0.0f, 1.0f, finalDamping);

    // WET/DRY: Convert 0-100% to 0.0-1.0
    float wetGain = wetValue / 100.0f;
    float dryGain = dryValue / 100.0f;

    // Update reverb parameters
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = finalRoomSize;
    reverbParams.damping = finalDamping;
    reverbParams.width = 1.0f;         // Full stereo (Room type)
    reverbParams.wetLevel = wetGain;   // Apply wet gain directly in reverb
    reverbParams.dryLevel = dryGain;   // Apply dry gain directly in reverb
    reverbParams.freezeMode = 0.0f;    // Disabled
    reverb.setParameters(reverbParams);

    // Create AudioBlock for DSP processing
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Process reverb (handles dry/wet mixing internally)
    reverb.process(context);
}

juce::AudioProcessorEditor* OuariconSimpleReverbAudioProcessor::createEditor()
{
    return new OuariconSimpleReverbAudioProcessorEditor(*this);
}

void OuariconSimpleReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconSimpleReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconSimpleReverbAudioProcessor();
}
