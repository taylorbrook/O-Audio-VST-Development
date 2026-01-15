/*
  ==============================================================================

    Ouaricon Polystutter - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconPolystutterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========================================================================================
    // LANE 1 PARAMETERS (14)
    // ========================================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane1_enabled", 1 },
        "Lane 1 Enabled",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lane1_subdivision", 1 },
        "Lane 1 Subdivision",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
        2  // Default: 1/16
    ));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "lane1_repeats", 1 },
        "Lane 1 Repeats",
        1, 16,
        4
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_decay", 1 },
        "Lane 1 Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        90.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_pitch", 1 },
        "Lane 1 Pitch",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "st"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_filter", 1 },
        "Lane 1 Filter",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_probability", 1 },
        "Lane 1 Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_volume", 1 },
        "Lane 1 Volume",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_pan", 1 },
        "Lane 1 Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane1_swing", 1 },
        "Lane 1 Swing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane1_pingpong", 1 },
        "Lane 1 Ping-Pong",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane1_reverse", 1 },
        "Lane 1 Reverse",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane1_manual_time_enabled", 1 },
        "Lane 1 Manual Time",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane1_freeze", 1 },
        "Lane 1 Freeze",
        false
    ));

    // ========================================================================================
    // LANE 2 PARAMETERS (14)
    // ========================================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane2_enabled", 1 },
        "Lane 2 Enabled",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lane2_subdivision", 1 },
        "Lane 2 Subdivision",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
        1  // Default: 1/8
    ));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "lane2_repeats", 1 },
        "Lane 2 Repeats",
        1, 16,
        4
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_decay", 1 },
        "Lane 2 Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        90.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_pitch", 1 },
        "Lane 2 Pitch",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "st"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_filter", 1 },
        "Lane 2 Filter",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_probability", 1 },
        "Lane 2 Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_volume", 1 },
        "Lane 2 Volume",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_pan", 1 },
        "Lane 2 Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane2_swing", 1 },
        "Lane 2 Swing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane2_pingpong", 1 },
        "Lane 2 Ping-Pong",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane2_reverse", 1 },
        "Lane 2 Reverse",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane2_manual_time_enabled", 1 },
        "Lane 2 Manual Time",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane2_freeze", 1 },
        "Lane 2 Freeze",
        false
    ));

    // ========================================================================================
    // LANE 3 PARAMETERS (14)
    // ========================================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane3_enabled", 1 },
        "Lane 3 Enabled",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lane3_subdivision", 1 },
        "Lane 3 Subdivision",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
        4  // Default: 1/8T (triplet)
    ));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "lane3_repeats", 1 },
        "Lane 3 Repeats",
        1, 16,
        4
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_decay", 1 },
        "Lane 3 Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        90.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_pitch", 1 },
        "Lane 3 Pitch",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "st"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_filter", 1 },
        "Lane 3 Filter",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_probability", 1 },
        "Lane 3 Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_volume", 1 },
        "Lane 3 Volume",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_pan", 1 },
        "Lane 3 Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane3_swing", 1 },
        "Lane 3 Swing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane3_pingpong", 1 },
        "Lane 3 Ping-Pong",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane3_reverse", 1 },
        "Lane 3 Reverse",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane3_manual_time_enabled", 1 },
        "Lane 3 Manual Time",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane3_freeze", 1 },
        "Lane 3 Freeze",
        false
    ));

    // ========================================================================================
    // LANE 4 PARAMETERS (14)
    // ========================================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane4_enabled", 1 },
        "Lane 4 Enabled",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lane4_subdivision", 1 },
        "Lane 4 Subdivision",
        juce::StringArray { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" },
        3  // Default: 1/32
    ));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "lane4_repeats", 1 },
        "Lane 4 Repeats",
        1, 16,
        4
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_decay", 1 },
        "Lane 4 Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        90.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_pitch", 1 },
        "Lane 4 Pitch",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "st"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_filter", 1 },
        "Lane 4 Filter",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_probability", 1 },
        "Lane 4 Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_volume", 1 },
        "Lane 4 Volume",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_pan", 1 },
        "Lane 4 Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lane4_swing", 1 },
        "Lane 4 Swing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane4_pingpong", 1 },
        "Lane 4 Ping-Pong",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane4_reverse", 1 },
        "Lane 4 Reverse",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane4_manual_time_enabled", 1 },
        "Lane 4 Manual Time",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lane4_freeze", 1 },
        "Lane 4 Freeze",
        false
    ));

    // ========================================================================================
    // TAPE DEGRADATION PARAMETERS (6)
    // ========================================================================================

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tape_saturation", 1 },
        "Tape Saturation",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tape_wow", 1 },
        "Tape Wow",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tape_flutter", 1 },
        "Tape Flutter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tape_hiss", 1 },
        "Tape Hiss",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tape_rolloff", 1 },
        "Tape Rolloff",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tape_dropout", 1 },
        "Tape Dropout",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    // ========================================================================================
    // GLOBAL CONTROL PARAMETERS (4)
    // ========================================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "envelope_enabled", 1 },
        "Envelope Trigger",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "sidechain_enabled", 1 },
        "Sidechain Trigger",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "midi_enabled", 1 },
        "MIDI Trigger",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "manual_trigger", 1 },
        "Manual Trigger",
        false
    ));

    // ========================================================================================
    // PATTERN SEQUENCER PARAMETERS (64 steps: 4 lanes × 16 steps)
    // ========================================================================================

    // Lane 1 Pattern Steps (16)
    for (int step = 1; step <= 16; ++step)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "pattern_lane1_step" + juce::String(step), 1 },
            "Lane 1 Step " + juce::String(step),
            true  // Default: all steps enabled
        ));
    }

    // Lane 2 Pattern Steps (16)
    for (int step = 1; step <= 16; ++step)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "pattern_lane2_step" + juce::String(step), 1 },
            "Lane 2 Step " + juce::String(step),
            true
        ));
    }

    // Lane 3 Pattern Steps (16)
    for (int step = 1; step <= 16; ++step)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "pattern_lane3_step" + juce::String(step), 1 },
            "Lane 3 Step " + juce::String(step),
            true
        ));
    }

    // Lane 4 Pattern Steps (16)
    for (int step = 1; step <= 16; ++step)
    {
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "pattern_lane4_step" + juce::String(step), 1 },
            "Lane 4 Step " + juce::String(step),
            true
        ));
    }

    return layout;
}

OuariconPolystutterAudioProcessor::OuariconPolystutterAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconPolystutterAudioProcessor::~OuariconPolystutterAudioProcessor()
{
}

void OuariconPolystutterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialization will be added in Stage 2 (DSP)
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OuariconPolystutterAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2 (DSP)
}

void OuariconPolystutterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Parameter access example (for Stage 2 DSP implementation):
    // auto* lane1EnabledParam = parameters.getRawParameterValue("lane1_enabled");
    // bool lane1Enabled = lane1EnabledParam->load() > 0.5f;

    // Pass-through for Stage 1 (DSP implementation happens in Stage 2)
    // Audio routing is already handled by JUCE
}

juce::AudioProcessorEditor* OuariconPolystutterAudioProcessor::createEditor()
{
    return new OuariconPolystutterAudioProcessorEditor(*this);
}

void OuariconPolystutterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconPolystutterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconPolystutterAudioProcessor();
}
