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
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Create all 4 lanes
    lane1 = std::make_unique<RepeatLane>();
    lane2 = std::make_unique<RepeatLane>();
    lane3 = std::make_unique<RepeatLane>();
    lane4 = std::make_unique<RepeatLane>();

    // Create trigger router (Phase 2.3)
    triggerRouter = std::make_unique<TriggerRouter>();

    // Create tape degrader (Phase 2.4)
    tapeDegrader = std::make_unique<TapeDegrader>();

    // Cache lane 1 parameter pointers (avoid string lookups in processBlock)
    lane1EnabledParam = apvts.getRawParameterValue("lane1_enabled");
    lane1SubdivParam = apvts.getRawParameterValue("lane1_subdivision");
    lane1RepeatsParam = apvts.getRawParameterValue("lane1_repeats");
    lane1DecayParam = apvts.getRawParameterValue("lane1_decay");
    lane1VolumeParam = apvts.getRawParameterValue("lane1_volume");
    lane1PanParam = apvts.getRawParameterValue("lane1_pan");
    lane1ProbabilityParam = apvts.getRawParameterValue("lane1_probability");
    lane1SwingParam = apvts.getRawParameterValue("lane1_swing");

    // Cache lane 2 parameter pointers
    lane2EnabledParam = apvts.getRawParameterValue("lane2_enabled");
    lane2SubdivParam = apvts.getRawParameterValue("lane2_subdivision");
    lane2RepeatsParam = apvts.getRawParameterValue("lane2_repeats");
    lane2DecayParam = apvts.getRawParameterValue("lane2_decay");
    lane2VolumeParam = apvts.getRawParameterValue("lane2_volume");
    lane2PanParam = apvts.getRawParameterValue("lane2_pan");
    lane2ProbabilityParam = apvts.getRawParameterValue("lane2_probability");
    lane2SwingParam = apvts.getRawParameterValue("lane2_swing");

    // Cache lane 3 parameter pointers
    lane3EnabledParam = apvts.getRawParameterValue("lane3_enabled");
    lane3SubdivParam = apvts.getRawParameterValue("lane3_subdivision");
    lane3RepeatsParam = apvts.getRawParameterValue("lane3_repeats");
    lane3DecayParam = apvts.getRawParameterValue("lane3_decay");
    lane3VolumeParam = apvts.getRawParameterValue("lane3_volume");
    lane3PanParam = apvts.getRawParameterValue("lane3_pan");
    lane3ProbabilityParam = apvts.getRawParameterValue("lane3_probability");
    lane3SwingParam = apvts.getRawParameterValue("lane3_swing");

    // Cache lane 4 parameter pointers
    lane4EnabledParam = apvts.getRawParameterValue("lane4_enabled");
    lane4SubdivParam = apvts.getRawParameterValue("lane4_subdivision");
    lane4RepeatsParam = apvts.getRawParameterValue("lane4_repeats");
    lane4DecayParam = apvts.getRawParameterValue("lane4_decay");
    lane4VolumeParam = apvts.getRawParameterValue("lane4_volume");
    lane4PanParam = apvts.getRawParameterValue("lane4_pan");
    lane4ProbabilityParam = apvts.getRawParameterValue("lane4_probability");
    lane4SwingParam = apvts.getRawParameterValue("lane4_swing");

    // Cache pattern step pointers
    for (int step = 0; step < 16; ++step)
    {
        lane1PatternSteps[step] = apvts.getRawParameterValue("pattern_lane1_step" + juce::String(step + 1));
        lane2PatternSteps[step] = apvts.getRawParameterValue("pattern_lane2_step" + juce::String(step + 1));
        lane3PatternSteps[step] = apvts.getRawParameterValue("pattern_lane3_step" + juce::String(step + 1));
        lane4PatternSteps[step] = apvts.getRawParameterValue("pattern_lane4_step" + juce::String(step + 1));
    }

    // Cache Phase 2.3 trigger mode apvts
    envelopeEnabledParam = apvts.getRawParameterValue("envelope_enabled");
    sidechainEnabledParam = apvts.getRawParameterValue("sidechain_enabled");
    midiEnabledParam = apvts.getRawParameterValue("midi_enabled");
    manualTriggerParam = apvts.getRawParameterValue("manual_trigger");

    // Cache Phase 2.3 lane advanced mode apvts
    lane1PingPongParam = apvts.getRawParameterValue("lane1_pingpong");
    lane1ReverseParam = apvts.getRawParameterValue("lane1_reverse");
    lane1FreezeParam = apvts.getRawParameterValue("lane1_freeze");
    lane1ManualTimeParam = apvts.getRawParameterValue("lane1_manual_time_enabled");

    lane2PingPongParam = apvts.getRawParameterValue("lane2_pingpong");
    lane2ReverseParam = apvts.getRawParameterValue("lane2_reverse");
    lane2FreezeParam = apvts.getRawParameterValue("lane2_freeze");
    lane2ManualTimeParam = apvts.getRawParameterValue("lane2_manual_time_enabled");

    lane3PingPongParam = apvts.getRawParameterValue("lane3_pingpong");
    lane3ReverseParam = apvts.getRawParameterValue("lane3_reverse");
    lane3FreezeParam = apvts.getRawParameterValue("lane3_freeze");
    lane3ManualTimeParam = apvts.getRawParameterValue("lane3_manual_time_enabled");

    lane4PingPongParam = apvts.getRawParameterValue("lane4_pingpong");
    lane4ReverseParam = apvts.getRawParameterValue("lane4_reverse");
    lane4FreezeParam = apvts.getRawParameterValue("lane4_freeze");
    lane4ManualTimeParam = apvts.getRawParameterValue("lane4_manual_time_enabled");

    // Cache Phase 2.4 pitch apvts
    lane1PitchParam = apvts.getRawParameterValue("lane1_pitch");
    lane2PitchParam = apvts.getRawParameterValue("lane2_pitch");
    lane3PitchParam = apvts.getRawParameterValue("lane3_pitch");
    lane4PitchParam = apvts.getRawParameterValue("lane4_pitch");

    // Cache Phase 2.4 tape degradation apvts
    tapeSaturationParam = apvts.getRawParameterValue("tape_saturation");
    tapeWowParam = apvts.getRawParameterValue("tape_wow");
    tapeFlutterParam = apvts.getRawParameterValue("tape_flutter");
    tapeHissParam = apvts.getRawParameterValue("tape_hiss");
    tapeRolloffParam = apvts.getRawParameterValue("tape_rolloff");
    tapeDropoutParam = apvts.getRawParameterValue("tape_dropout");
}

OuariconPolystutterAudioProcessor::~OuariconPolystutterAudioProcessor()
{
}

void OuariconPolystutterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;  // Stereo

    // Prepare all 4 lanes
    if (lane1) lane1->prepare(spec);
    if (lane2) lane2->prepare(spec);
    if (lane3) lane3->prepare(spec);
    if (lane4) lane4->prepare(spec);

    // Prepare trigger router (Phase 2.3)
    if (triggerRouter) triggerRouter->prepare(spec);

    // Prepare tape degrader (Phase 2.4)
    if (tapeDegrader) tapeDegrader->prepare(spec);

    // Store max block size for buffer safety checks
    maxBlockSize = samplesPerBlock;

    // Preallocate buffers for mixing (real-time safe - no allocations in processBlock)
    dryBuffer.setSize(2, samplesPerBlock);
    lane1Buffer.setSize(2, samplesPerBlock);
    lane2Buffer.setSize(2, samplesPerBlock);
    lane3Buffer.setSize(2, samplesPerBlock);
    lane4Buffer.setSize(2, samplesPerBlock);

    // Initialize beat sync state
    currentBPM = 120.0;
    lastPPQPosition = 0.0;
    wasPlaying = false;
    samplesSinceLastBeat = 0;

    // Calculate initial subdivision samples
    subdivisionSamples = getSubdivisionSamples(2, currentBPM, sampleRate);  // Default 1/16
}

void OuariconPolystutterAudioProcessor::releaseResources()
{
    // Release large buffers to save memory
    dryBuffer.setSize(0, 0);
    lane1Buffer.setSize(0, 0);
    lane2Buffer.setSize(0, 0);
    lane3Buffer.setSize(0, 0);
    lane4Buffer.setSize(0, 0);
}

bool OuariconPolystutterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Main output must be stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Main input must be stereo
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Sidechain input (bus index 1) can be disabled or stereo
    if (layouts.getNumChannels(true, 1) != 0 && layouts.getNumChannels(true, 1) != 2)
        return false;

    return true;
}

void OuariconPolystutterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Safety check: ensure we don't exceed pre-allocated buffer size
    // Return early = true pass-through (input buffer unchanged)
    if (numSamples > maxBlockSize || numSamples == 0)
        return;

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // ========== Read Parameters for All Lanes ==========
    // Lane 1
    bool lane1Enabled = lane1EnabledParam->load() > 0.5f;
    int lane1Subdiv = static_cast<int>(lane1SubdivParam->load());
    int lane1Repeats = static_cast<int>(lane1RepeatsParam->load());
    float lane1Decay = lane1DecayParam->load();
    float lane1Volume = lane1VolumeParam->load();
    float lane1Pan = lane1PanParam->load();
    float lane1Probability = lane1ProbabilityParam->load();
    float lane1Swing = lane1SwingParam->load();

    // Lane 2
    bool lane2Enabled = lane2EnabledParam->load() > 0.5f;
    int lane2Subdiv = static_cast<int>(lane2SubdivParam->load());
    int lane2Repeats = static_cast<int>(lane2RepeatsParam->load());
    float lane2Decay = lane2DecayParam->load();
    float lane2Volume = lane2VolumeParam->load();
    float lane2Pan = lane2PanParam->load();
    float lane2Probability = lane2ProbabilityParam->load();
    float lane2Swing = lane2SwingParam->load();

    // Lane 3
    bool lane3Enabled = lane3EnabledParam->load() > 0.5f;
    int lane3Subdiv = static_cast<int>(lane3SubdivParam->load());
    int lane3Repeats = static_cast<int>(lane3RepeatsParam->load());
    float lane3Decay = lane3DecayParam->load();
    float lane3Volume = lane3VolumeParam->load();
    float lane3Pan = lane3PanParam->load();
    float lane3Probability = lane3ProbabilityParam->load();
    float lane3Swing = lane3SwingParam->load();

    // Lane 4
    bool lane4Enabled = lane4EnabledParam->load() > 0.5f;
    int lane4Subdiv = static_cast<int>(lane4SubdivParam->load());
    int lane4Repeats = static_cast<int>(lane4RepeatsParam->load());
    float lane4Decay = lane4DecayParam->load();
    float lane4Volume = lane4VolumeParam->load();
    float lane4Pan = lane4PanParam->load();
    float lane4Probability = lane4ProbabilityParam->load();
    float lane4Swing = lane4SwingParam->load();

    // ========== Read Phase 2.3 Parameters (cached pointers) ==========
    // Global trigger modes
    bool envelopeEnabled = envelopeEnabledParam->load() > 0.5f;
    bool sidechainEnabled = sidechainEnabledParam->load() > 0.5f;
    bool midiEnabled = midiEnabledParam->load() > 0.5f;
    bool currentManualTrigger = manualTriggerParam->load() > 0.5f;

    // Lane-specific advanced modes
    bool lane1PingPong = lane1PingPongParam->load() > 0.5f;
    bool lane1Reverse = lane1ReverseParam->load() > 0.5f;
    bool lane1Freeze = lane1FreezeParam->load() > 0.5f;
    bool lane1ManualTime = lane1ManualTimeParam->load() > 0.5f;

    bool lane2PingPong = lane2PingPongParam->load() > 0.5f;
    bool lane2Reverse = lane2ReverseParam->load() > 0.5f;
    bool lane2Freeze = lane2FreezeParam->load() > 0.5f;
    bool lane2ManualTime = lane2ManualTimeParam->load() > 0.5f;

    bool lane3PingPong = lane3PingPongParam->load() > 0.5f;
    bool lane3Reverse = lane3ReverseParam->load() > 0.5f;
    bool lane3Freeze = lane3FreezeParam->load() > 0.5f;
    bool lane3ManualTime = lane3ManualTimeParam->load() > 0.5f;

    bool lane4PingPong = lane4PingPongParam->load() > 0.5f;
    bool lane4Reverse = lane4ReverseParam->load() > 0.5f;
    bool lane4Freeze = lane4FreezeParam->load() > 0.5f;
    bool lane4ManualTime = lane4ManualTimeParam->load() > 0.5f;

    // ========== Update Lane Parameters ==========
    if (lane1)
    {
        lane1->setEnabled(lane1Enabled);
        lane1->setSubdivision(lane1Subdiv);
        lane1->setRepeats(lane1Repeats);
        lane1->setDecay(lane1Decay);
        lane1->setVolume(lane1Volume);
        lane1->setPan(lane1Pan);
        lane1->setProbability(lane1Probability);
        lane1->setSwing(lane1Swing);

        // Phase 2.4: Pitch shifting
        lane1->setPitch(lane1PitchParam->load());

        // Phase 2.3: Advanced modes
        lane1->setPingPong(lane1PingPong);
        lane1->setReverse(lane1Reverse);
        lane1->setFreeze(lane1Freeze);
        lane1->setManualTimeEnabled(lane1ManualTime);

        // Update pattern steps
        for (int step = 0; step < 16; ++step)
        {
            bool stepEnabled = lane1PatternSteps[step]->load() > 0.5f;
            lane1->setPatternStep(step, stepEnabled);
        }
    }

    if (lane2)
    {
        lane2->setEnabled(lane2Enabled);
        lane2->setSubdivision(lane2Subdiv);
        lane2->setRepeats(lane2Repeats);
        lane2->setDecay(lane2Decay);
        lane2->setVolume(lane2Volume);
        lane2->setPan(lane2Pan);
        lane2->setProbability(lane2Probability);
        lane2->setSwing(lane2Swing);

        // Phase 2.4: Pitch shifting
        lane2->setPitch(lane2PitchParam->load());

        // Phase 2.3: Advanced modes
        lane2->setPingPong(lane2PingPong);
        lane2->setReverse(lane2Reverse);
        lane2->setFreeze(lane2Freeze);
        lane2->setManualTimeEnabled(lane2ManualTime);

        for (int step = 0; step < 16; ++step)
        {
            bool stepEnabled = lane2PatternSteps[step]->load() > 0.5f;
            lane2->setPatternStep(step, stepEnabled);
        }
    }

    if (lane3)
    {
        lane3->setEnabled(lane3Enabled);
        lane3->setSubdivision(lane3Subdiv);
        lane3->setRepeats(lane3Repeats);
        lane3->setDecay(lane3Decay);
        lane3->setVolume(lane3Volume);
        lane3->setPan(lane3Pan);
        lane3->setProbability(lane3Probability);
        lane3->setSwing(lane3Swing);

        // Phase 2.4: Pitch shifting
        lane3->setPitch(lane3PitchParam->load());

        // Phase 2.3: Advanced modes
        lane3->setPingPong(lane3PingPong);
        lane3->setReverse(lane3Reverse);
        lane3->setFreeze(lane3Freeze);
        lane3->setManualTimeEnabled(lane3ManualTime);

        for (int step = 0; step < 16; ++step)
        {
            bool stepEnabled = lane3PatternSteps[step]->load() > 0.5f;
            lane3->setPatternStep(step, stepEnabled);
        }
    }

    if (lane4)
    {
        lane4->setEnabled(lane4Enabled);
        lane4->setSubdivision(lane4Subdiv);
        lane4->setRepeats(lane4Repeats);
        lane4->setDecay(lane4Decay);
        lane4->setVolume(lane4Volume);
        lane4->setPan(lane4Pan);
        lane4->setProbability(lane4Probability);
        lane4->setSwing(lane4Swing);

        // Phase 2.4: Pitch shifting
        lane4->setPitch(lane4PitchParam->load());

        // Phase 2.3: Advanced modes
        lane4->setPingPong(lane4PingPong);
        lane4->setReverse(lane4Reverse);
        lane4->setFreeze(lane4Freeze);
        lane4->setManualTimeEnabled(lane4ManualTime);

        for (int step = 0; step < 16; ++step)
        {
            bool stepEnabled = lane4PatternSteps[step]->load() > 0.5f;
            lane4->setPatternStep(step, stepEnabled);
        }
    }

    // ========== Phase 2.3: Trigger Router Processing ==========
    // Get sidechain input buffer (bus index 1)
    juce::AudioBuffer<float> sidechainBus;
    juce::AudioBuffer<float>* sidechainBuffer = nullptr;
    if (sidechainEnabled && getBusCount(true) > 1)
    {
        sidechainBus = getBusBuffer(buffer, true, 1);
        if (sidechainBus.getNumChannels() > 0)
            sidechainBuffer = &sidechainBus;
    }

    // Update trigger router modes
    if (triggerRouter)
    {
        triggerRouter->setEnvelopeEnabled(envelopeEnabled);
        triggerRouter->setSidechainEnabled(sidechainEnabled);
        triggerRouter->setMidiEnabled(midiEnabled);

        // Process trigger detection
        bool globalTrigger = triggerRouter->processTriggerDetection(buffer, sidechainBuffer, midiMessages, numSamples);

        // Handle envelope/sidechain triggers (trigger all enabled lanes)
        if (globalTrigger)
        {
            if (lane1 && lane1Enabled) lane1->trigger();
            if (lane2 && lane2Enabled) lane2->trigger();
            if (lane3 && lane3Enabled) lane3->trigger();
            if (lane4 && lane4Enabled) lane4->trigger();
        }

        // Handle MIDI triggers (per-lane or all lanes)
        int midiLane = triggerRouter->getMidiTriggeredLane();
        if (midiLane >= 0)
        {
            if (midiLane == 100)  // G3 triggers all lanes
            {
                if (lane1 && lane1Enabled) lane1->trigger();
                if (lane2 && lane2Enabled) lane2->trigger();
                if (lane3 && lane3Enabled) lane3->trigger();
                if (lane4 && lane4Enabled) lane4->trigger();
            }
            else if (midiLane == 0 && lane1 && lane1Enabled)
            {
                lane1->trigger();
            }
            else if (midiLane == 1 && lane2 && lane2Enabled)
            {
                lane2->trigger();
            }
            else if (midiLane == 2 && lane3 && lane3Enabled)
            {
                lane3->trigger();
            }
            else if (midiLane == 3 && lane4 && lane4Enabled)
            {
                lane4->trigger();
            }
        }

        // Handle global freeze toggle (MIDI note A3)
        if (triggerRouter->shouldToggleFreeze())
        {
            // Note: MIDI freeze toggle detected - actual parameter control is in UI
            // This clears the toggle flag to acknowledge the event
            triggerRouter->clearFreezeToggle();
        }
    }

    // Handle manual trigger button (rising edge detection - trigger once per press)
    if (currentManualTrigger && !previousManualTrigger)
    {
        if (lane1 && lane1Enabled) lane1->trigger();
        if (lane2 && lane2Enabled) lane2->trigger();
        if (lane3 && lane3Enabled) lane3->trigger();
        if (lane4 && lane4Enabled) lane4->trigger();
    }
    previousManualTrigger = currentManualTrigger;

    // ========== Get Playhead Position ==========
    auto* playHead = getPlayHead();
    auto posInfo = playHead ? playHead->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>();

    // Update pattern positions for all lanes
    if (posInfo.hasValue() && posInfo->getPpqPosition().hasValue())
    {
        double ppqPosition = *posInfo->getPpqPosition();

        if (lane1) lane1->updatePatternPosition(ppqPosition, lane1Subdiv);
        if (lane2) lane2->updatePatternPosition(ppqPosition, lane2Subdiv);
        if (lane3) lane3->updatePatternPosition(ppqPosition, lane3Subdiv);
        if (lane4) lane4->updatePatternPosition(ppqPosition, lane4Subdiv);
    }

    // Update beat sync and trigger
    updateBeatSync(posInfo);

    // ========== Copy Dry Signal ==========
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::FloatVectorOperations::copy(dryBuffer.getWritePointer(ch),
                                          buffer.getReadPointer(ch),
                                          numSamples);
    }

    // ========== Process Each Lane Independently ==========
    // Lane 1
    lane1Buffer.clear();
    if (lane1 && lane1Enabled)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::copy(lane1Buffer.getWritePointer(ch),
                                              dryBuffer.getReadPointer(ch),
                                              numSamples);
        }
        lane1->processBlock(lane1Buffer, numSamples);
    }

    // Lane 2
    lane2Buffer.clear();
    if (lane2 && lane2Enabled)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::copy(lane2Buffer.getWritePointer(ch),
                                              dryBuffer.getReadPointer(ch),
                                              numSamples);
        }
        lane2->processBlock(lane2Buffer, numSamples);
    }

    // Lane 3
    lane3Buffer.clear();
    if (lane3 && lane3Enabled)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::copy(lane3Buffer.getWritePointer(ch),
                                              dryBuffer.getReadPointer(ch),
                                              numSamples);
        }
        lane3->processBlock(lane3Buffer, numSamples);
    }

    // Lane 4
    lane4Buffer.clear();
    if (lane4 && lane4Enabled)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::copy(lane4Buffer.getWritePointer(ch),
                                              dryBuffer.getReadPointer(ch),
                                              numSamples);
        }
        lane4->processBlock(lane4Buffer, numSamples);
    }

    // ========== Mix All Lanes ==========
    // Start with dry signal (70%), wet signal split across 4 lanes (30% / 4 = 7.5% each)
    constexpr float dryGain = 0.7f;
    constexpr float wetPerLane = 0.075f;  // 0.3 / 4 = 0.075 (prevents clipping when all lanes active)

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* outData = buffer.getWritePointer(channel);
        const auto* dryData = dryBuffer.getReadPointer(channel);

        juce::FloatVectorOperations::multiply(outData, dryData, dryGain, numSamples);

        // Add lane outputs (30% total wet, split across 4 lanes)
        const auto* lane1Data = lane1Buffer.getReadPointer(channel);
        const auto* lane2Data = lane2Buffer.getReadPointer(channel);
        const auto* lane3Data = lane3Buffer.getReadPointer(channel);
        const auto* lane4Data = lane4Buffer.getReadPointer(channel);

        // Sum all lanes with equal weighting (total wet = 0.3 max)
        juce::FloatVectorOperations::addWithMultiply(outData, lane1Data, wetPerLane, numSamples);
        juce::FloatVectorOperations::addWithMultiply(outData, lane2Data, wetPerLane, numSamples);
        juce::FloatVectorOperations::addWithMultiply(outData, lane3Data, wetPerLane, numSamples);
        juce::FloatVectorOperations::addWithMultiply(outData, lane4Data, wetPerLane, numSamples);
    }

    // ========== Phase 2.4: Tape Degradation Post-Processing ==========
    if (tapeDegrader)
    {
        // Update tape degradation apvts
        tapeDegrader->setSaturation(tapeSaturationParam->load());
        tapeDegrader->setWow(tapeWowParam->load());
        tapeDegrader->setFlutter(tapeFlutterParam->load());
        tapeDegrader->setHiss(tapeHissParam->load());
        tapeDegrader->setRolloff(tapeRolloffParam->load());
        tapeDegrader->setDropout(tapeDropoutParam->load());

        // Process buffer in-place with tape effects
        tapeDegrader->processBlock(buffer, numSamples);
    }
}

juce::AudioProcessorEditor* OuariconPolystutterAudioProcessor::createEditor()
{
    return new OuariconPolystutterAudioProcessorEditor(*this);
}

void OuariconPolystutterAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconPolystutterAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Helper functions
void OuariconPolystutterAudioProcessor::updateBeatSync(const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo)
{
    // Check if we have valid playhead info
    if (!posInfo.hasValue())
    {
        // Offline rendering or no playhead available - use default BPM
        currentBPM = 120.0;

        // Update all lane timings with default BPM
        if (lane1) lane1->updateTempo(currentBPM, spec.sampleRate);
        if (lane2) lane2->updateTempo(currentBPM, spec.sampleRate);
        if (lane3) lane3->updateTempo(currentBPM, spec.sampleRate);
        if (lane4) lane4->updateTempo(currentBPM, spec.sampleRate);

        return;
    }

    auto& info = *posInfo;

    // Get BPM (use default if not available, with validation)
    if (info.getBpm().hasValue())
    {
        double bpm = *info.getBpm();
        currentBPM = juce::jlimit(20.0, 999.0, bpm);
    }
    else
    {
        currentBPM = 120.0;
    }

    // Update all lane timings
    if (lane1) lane1->updateTempo(currentBPM, spec.sampleRate);
    if (lane2) lane2->updateTempo(currentBPM, spec.sampleRate);
    if (lane3) lane3->updateTempo(currentBPM, spec.sampleRate);
    if (lane4) lane4->updateTempo(currentBPM, spec.sampleRate);

    // Get PPQ position
    if (!info.getPpqPosition().hasValue())
        return;

    double ppqPosition = *info.getPpqPosition();

    // Check if transport is playing
    bool isPlaying = info.getIsPlaying();

    // Detect subdivision boundaries and trigger each lane independently
    if (isPlaying)
    {
        // Get subdivision for each lane
        int lane1Subdiv = static_cast<int>(lane1SubdivParam->load());
        int lane2Subdiv = static_cast<int>(lane2SubdivParam->load());
        int lane3Subdiv = static_cast<int>(lane3SubdivParam->load());
        int lane4Subdiv = static_cast<int>(lane4SubdivParam->load());

        // Helper lambda to calculate subdivision PPQ
        auto getSubdivisionPPQ = [](int subdivIndex) -> double
        {
            switch (subdivIndex)
            {
                case 0: return 1.0;       // 1/4 note
                case 1: return 0.5;       // 1/8 note
                case 2: return 0.25;      // 1/16 note
                case 3: return 0.125;     // 1/32 note
                case 4: return 1.0 / 3.0; // 1/8T (triplet)
                case 5: return 1.0 / 6.0; // 1/16T (triplet)
                default: return 0.25;     // Default 1/16
            }
        };

        // Check if we crossed a subdivision boundary for each lane
        if (wasPlaying)
        {
            // Lane 1
            if (lane1 && lane1EnabledParam->load() > 0.5f)
            {
                double subdivPPQ = getSubdivisionPPQ(lane1Subdiv);
                int currentSubdiv = static_cast<int>(ppqPosition / subdivPPQ);
                int lastSubdiv = static_cast<int>(lastPPQPosition / subdivPPQ);

                if (currentSubdiv > lastSubdiv)
                    lane1->trigger();
            }

            // Lane 2
            if (lane2 && lane2EnabledParam->load() > 0.5f)
            {
                double subdivPPQ = getSubdivisionPPQ(lane2Subdiv);
                int currentSubdiv = static_cast<int>(ppqPosition / subdivPPQ);
                int lastSubdiv = static_cast<int>(lastPPQPosition / subdivPPQ);

                if (currentSubdiv > lastSubdiv)
                    lane2->trigger();
            }

            // Lane 3
            if (lane3 && lane3EnabledParam->load() > 0.5f)
            {
                double subdivPPQ = getSubdivisionPPQ(lane3Subdiv);
                int currentSubdiv = static_cast<int>(ppqPosition / subdivPPQ);
                int lastSubdiv = static_cast<int>(lastPPQPosition / subdivPPQ);

                if (currentSubdiv > lastSubdiv)
                    lane3->trigger();
            }

            // Lane 4
            if (lane4 && lane4EnabledParam->load() > 0.5f)
            {
                double subdivPPQ = getSubdivisionPPQ(lane4Subdiv);
                int currentSubdiv = static_cast<int>(ppqPosition / subdivPPQ);
                int lastSubdiv = static_cast<int>(lastPPQPosition / subdivPPQ);

                if (currentSubdiv > lastSubdiv)
                    lane4->trigger();
            }
        }
    }

    // Update state
    lastPPQPosition = ppqPosition;
    wasPlaying = isPlaying;
}

double OuariconPolystutterAudioProcessor::getSubdivisionSamples(int subdivIndex, double bpm, double sampleRate)
{
    // Calculate quarter note duration in samples
    double quarterNoteSamples = (60.0 / bpm) * sampleRate;

    // Calculate subdivision based on index
    switch (subdivIndex)
    {
        case 0: return quarterNoteSamples;        // 1/4
        case 1: return quarterNoteSamples / 2.0;  // 1/8
        case 2: return quarterNoteSamples / 4.0;  // 1/16
        case 3: return quarterNoteSamples / 8.0;  // 1/32
        case 4: return quarterNoteSamples / 3.0;  // 1/8T (triplet)
        case 5: return quarterNoteSamples / 6.0;  // 1/16T (triplet)
        default: return quarterNoteSamples / 4.0; // Default to 1/16
    }
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconPolystutterAudioProcessor();
}
