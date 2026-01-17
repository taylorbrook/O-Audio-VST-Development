/*
  ==============================================================================

    OuariconLyrica - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconLyricaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Core Sound Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterVolume", 1 },
        "Master Volume",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f),
        0.0f,
        "dB"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "stringMaterial", 1 },
        "String Material",
        juce::StringArray { "Gut", "Nylon", "Wire", "Carbon", "Metal Alloy", "Glass", "Crystal", "Energy" },
        1  // Default: Nylon
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brightness", 1 },
        "Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sustain", 1 },
        "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f
    ));

    // Body Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodySize", 1 },
        "Body Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyResonance", 1 },
        "Body Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "woodType", 1 },
        "Wood Type",
        juce::StringArray { "Spruce", "Maple", "Exotic", "Synthetic" },
        0  // Default: Spruce
    ));

    // Sympathetic Resonance
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticAmount", 1 },
        "Sympathetic Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // Pluck Mechanics
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pluckPosition", 1 },
        "Pluck Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "fingerHardness", 1 },
        "Finger Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // Expression
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "technique", 1 },
        "Technique",
        juce::StringArray { "Normal", "Harmonic", "Muted", "Près de la table" },
        0  // Default: Normal
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoMode", 1 },
        "Glissando Mode",
        juce::StringArray { "Off", "Free", "Scale-Locked" },
        0  // Default: Off
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "glissandoScale", 1 },
        "Glissando Scale",
        juce::StringArray { "Major", "Minor", "Pentatonic", "Custom" },
        0  // Default: Major
    ));

    // Tuning
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterTune", 1 },
        "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchBendRange", 1 },
        "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        2.0f,
        "st"
    ));

    // Advanced String Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTension", 1 },
        "String Tension",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringGauge", 1 },
        "String Gauge",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringLength", 1 },
        "String Length",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringStiffness", 1 },
        "String Stiffness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f
    ));

    return layout;
}

OuariconLyricaAudioProcessor::OuariconLyricaAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize synthesiser with 16 voices
    for (int i = 0; i < 16; ++i)
        synthesiser.addVoice(new HarpSynthVoice());

    // Add sound that accepts all MIDI notes
    synthesiser.addSound(new HarpSynthSound());
}

OuariconLyricaAudioProcessor::~OuariconLyricaAudioProcessor()
{
}

void OuariconLyricaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void OuariconLyricaAudioProcessor::releaseResources()
{
    // Release any resources when playback stops
}

void OuariconLyricaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // Render MIDI to audio via synthesiser
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* OuariconLyricaAudioProcessor::createEditor()
{
    return new OuariconLyricaAudioProcessorEditor(*this);
}

void OuariconLyricaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconLyricaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconLyricaAudioProcessor();
}
