#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout Creation
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OuariconTremoloAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // SPEED_PARAM - Tremolo Speed (0.1 - 20.0 Hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SPEED_PARAM", 1 },
        "Speed",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f),
        4.5f,
        "Hz"
    ));

    // DEPTH_PARAM - Tremolo Depth (0 - 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DEPTH_PARAM", 1 },
        "Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f,
        "%"
    ));

    // WAVEFORM_PARAM - Waveform Type (Choice: 0-5)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "WAVEFORM_PARAM", 1 },
        "Waveform",
        juce::StringArray { "Sine", "Triangle", "Phasor", "Noise", "Square", "Pulse" },
        0  // Default: Sine
    ));

    // SMOOTHING_PARAM - Waveform Smoothing (0 - 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SMOOTHING_PARAM", 1 },
        "Smoothing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        30.0f,
        "%"
    ));

    // PAN_SYNC_PARAM - Pan Sync Enable (Boolean)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "PAN_SYNC_PARAM", 1 },
        "Pan Sync",
        false  // Default: OFF
    ));

    // TEMPO_SYNC_PARAM - Tempo Sync Enable (Boolean)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "TEMPO_SYNC_PARAM", 1 },
        "Tempo Sync",
        false  // Default: OFF
    ));

    return layout;
}

//==============================================================================
// Constructor & Destructor
//==============================================================================
OuariconTremoloAudioProcessor::OuariconTremoloAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconTremoloAudioProcessor::~OuariconTremoloAudioProcessor()
{
}

//==============================================================================
// Audio Processing
//==============================================================================
void OuariconTremoloAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // DSP initialization will be added in Stage 2
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OuariconTremoloAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2
}

void OuariconTremoloAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Pass-through for Stage 1 (DSP implementation in Stage 2)
    // Audio routing already handled by JUCE

    // Parameter access example (for Stage 2 DSP implementation):
    // auto* speedParam = parameters.getRawParameterValue("SPEED_PARAM");
    // float speedValue = speedParam->load();  // Atomic read (real-time safe)
}

//==============================================================================
// Editor Creation
//==============================================================================
juce::AudioProcessorEditor* OuariconTremoloAudioProcessor::createEditor()
{
    return new OuariconTremoloAudioProcessorEditor(*this);
}

//==============================================================================
// State Management (Save/Load)
//==============================================================================
void OuariconTremoloAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconTremoloAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Factory Function
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconTremoloAudioProcessor();
}
