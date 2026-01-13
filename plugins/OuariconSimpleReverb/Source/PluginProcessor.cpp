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
    auto* typeParam = parameters.getRawParameterValue("TYPE");
    auto* sizeParam = parameters.getRawParameterValue("SIZE");
    auto* decayParam = parameters.getRawParameterValue("DECAY");
    auto* wetParam = parameters.getRawParameterValue("WET");
    auto* dryParam = parameters.getRawParameterValue("DRY");

    int typeValue = static_cast<int>(typeParam->load());  // 0-5 (Booth, Room, Hall, Spring, Plate, Ambient)
    float sizeValue = sizeParam->load();      // 0-100%
    float decayValue = decayParam->load();    // 0.1-10s
    float wetValue = wetParam->load();        // 0-100%
    float dryValue = dryParam->load();        // 0-100%

    // Define six type presets (architecture.md Phase 2.2)
    // {baseRoomSize, baseDamping, width}
    struct TypePreset {
        float baseRoomSize;
        float baseDamping;
        float width;
    };

    const TypePreset typePresets[6] = {
        {0.15f, 0.85f, 1.0f},  // 0: Booth - tight, intimate
        {0.50f, 0.50f, 1.0f},  // 1: Room - natural, versatile
        {0.85f, 0.20f, 1.0f},  // 2: Hall - large concert hall
        {0.30f, 0.45f, 0.8f},  // 3: Spring - narrower stereo
        {0.60f, 0.30f, 1.0f},  // 4: Plate - dense, smooth
        {0.95f, 0.10f, 1.0f}   // 5: Ambient - washy, ethereal
    };

    // Clamp type to valid range (safety)
    typeValue = juce::jlimit(0, 5, typeValue);

    // Get base preset values from TYPE parameter
    float baseRoomSize = typePresets[typeValue].baseRoomSize;
    float baseDamping = typePresets[typeValue].baseDamping;
    float width = typePresets[typeValue].width;

    // Apply SIZE scaling to baseRoomSize (0-100% scales 0.5-1.5x base)
    float sizeNorm = sizeValue / 100.0f;  // 0.0-1.0
    float finalRoomSize = baseRoomSize * (0.5f + sizeNorm * 0.5f);
    finalRoomSize = juce::jlimit(0.0f, 1.0f, finalRoomSize);  // Clamp to valid range

    // Apply DECAY to damping (longer decay = less damping)
    // Formula from architecture.md: damping = 1.0 - (decayValue / 10.0) * 0.8
    // But we start from baseDamping instead of 1.0 to preserve type character
    float finalDamping = baseDamping * (1.0f - (decayValue / 10.0f) * 0.8f);
    finalDamping = juce::jlimit(0.0f, 1.0f, finalDamping);

    // WET/DRY: Convert 0-100% to 0.0-1.0
    float wetGain = wetValue / 100.0f;
    float dryGain = dryValue / 100.0f;

    // Update reverb parameters
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = finalRoomSize;
    reverbParams.damping = finalDamping;
    reverbParams.width = width;           // Type-specific width (0.8 for Spring, 1.0 for others)
    reverbParams.wetLevel = wetGain;      // Apply wet gain directly in reverb
    reverbParams.dryLevel = dryGain;      // Apply dry gain directly in reverb
    reverbParams.freezeMode = 0.0f;       // Disabled
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
