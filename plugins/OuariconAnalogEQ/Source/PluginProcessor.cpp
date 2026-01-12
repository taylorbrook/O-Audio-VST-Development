/*
  ==============================================================================

    Ouaricon Analog EQ - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconAnalogEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // LF Band (Low Frequency Shelf)
    // lf_freq - Corner frequency (30-500 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_freq", 1 },
        "LF Frequency",
        juce::NormalisableRange<float>(30.0f, 500.0f, 0.1f, 0.3f),  // Logarithmic skew
        100.0f,
        "Hz"
    ));

    // lf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_gain", 1 },
        "LF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // lf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lf_on", 1 },
        "LF On",
        true
    ));

    // LMF Band (Low-Mid Frequency Bell)
    // lmf_freq - Center frequency (100-2000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_freq", 1 },
        "LMF Frequency",
        juce::NormalisableRange<float>(100.0f, 2000.0f, 0.1f, 0.3f),
        500.0f,
        "Hz"
    ));

    // lmf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_gain", 1 },
        "LMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // lmf_q - Q factor selection (WIDE/MED/TIGHT)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lmf_q", 1 },
        "LMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" },
        1  // Default: MED
    ));

    // lmf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lmf_on", 1 },
        "LMF On",
        true
    ));

    // HMF Band (High-Mid Frequency Bell)
    // hmf_freq - Center frequency (500-8000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_freq", 1 },
        "HMF Frequency",
        juce::NormalisableRange<float>(500.0f, 8000.0f, 0.1f, 0.3f),
        2000.0f,
        "Hz"
    ));

    // hmf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_gain", 1 },
        "HMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // hmf_q - Q factor selection (WIDE/MED/TIGHT)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "hmf_q", 1 },
        "HMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" },
        1  // Default: MED
    ));

    // hmf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hmf_on", 1 },
        "HMF On",
        true
    ));

    // HF Band (High Frequency Shelf)
    // hf_freq - Corner frequency (2000-20000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_freq", 1 },
        "HF Frequency",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.1f, 0.3f),
        8000.0f,
        "Hz"
    ));

    // hf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_gain", 1 },
        "HF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // hf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hf_on", 1 },
        "HF On",
        true
    ));

    // Global Controls
    // output_gain - Master output level (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // analog - Analog warmth/saturation enable
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "analog", 1 },
        "Analog",
        true
    ));

    return layout;
}

OuariconAnalogEQAudioProcessor::OuariconAnalogEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconAnalogEQAudioProcessor::~OuariconAnalogEQAudioProcessor()
{
}

void OuariconAnalogEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialization will be added in Stage 3 (DSP)
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OuariconAnalogEQAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 3 (DSP)
}

void OuariconAnalogEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Parameter access example (for Stage 3 DSP implementation):
    // auto* lfFreqParam = parameters.getRawParameterValue("lf_freq");
    // float lfFreq = lfFreqParam->load();  // Atomic read (real-time safe)

    // Pass-through for Stage 1 (DSP implementation happens in Stage 3)
    // Audio routing is already handled by JUCE
}

juce::AudioProcessorEditor* OuariconAnalogEQAudioProcessor::createEditor()
{
    return new OuariconAnalogEQAudioProcessorEditor(*this);
}

void OuariconAnalogEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconAnalogEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconAnalogEQAudioProcessor();
}
