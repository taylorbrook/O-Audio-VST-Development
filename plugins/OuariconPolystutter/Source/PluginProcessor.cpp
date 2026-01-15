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
    // Create lane 1
    lane1 = std::make_unique<RepeatLane>();

    // Cache parameter pointers (avoid string lookups in processBlock)
    lane1EnabledParam = parameters.getRawParameterValue("lane1_enabled");
    lane1SubdivParam = parameters.getRawParameterValue("lane1_subdivision");
    lane1RepeatsParam = parameters.getRawParameterValue("lane1_repeats");
    lane1DecayParam = parameters.getRawParameterValue("lane1_decay");
    lane1VolumeParam = parameters.getRawParameterValue("lane1_volume");
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

    // Prepare lane 1
    if (lane1)
        lane1->prepare(spec);

    // Store max block size for buffer safety checks
    maxBlockSize = samplesPerBlock;

    // Preallocate dry/wet buffers for mixing (real-time safe - no allocations in processBlock)
    dryBuffer.setSize(2, samplesPerBlock);
    wetBuffer.setSize(2, samplesPerBlock);

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
    wetBuffer.setSize(0, 0);
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
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();

    // Safety check: ensure we don't exceed pre-allocated buffer size
    // This prevents memory allocation if host sends larger blocks than expected
    if (numSamples > maxBlockSize)
    {
        // Fallback: pass-through without processing (safer than allocating in real-time)
        return;
    }

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    // Read parameters using cached pointers (no string lookups - real-time safe)
    bool lane1Enabled = lane1EnabledParam->load() > 0.5f;
    int subdivIndex = static_cast<int>(lane1SubdivParam->load());
    int numRepeats = static_cast<int>(lane1RepeatsParam->load());
    float decayPercent = lane1DecayParam->load();
    float volumePercent = lane1VolumeParam->load();

    // Update lane 1 parameters
    if (lane1)
    {
        lane1->setEnabled(lane1Enabled);
        lane1->setSubdivision(subdivIndex);
        lane1->setRepeats(numRepeats);
        lane1->setDecay(decayPercent);
        lane1->setVolume(volumePercent);
    }

    // Get playhead position for beat sync
    auto posInfo = getPlayHead() ? getPlayHead()->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>();

    // Update beat sync and trigger
    updateBeatSync(posInfo);

    // Copy dry signal using SIMD-optimized copy (real-time safe - no allocations)
    const int numChannels = juce::jmin(buffer.getNumChannels(), dryBuffer.getNumChannels());
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::FloatVectorOperations::copy(dryBuffer.getWritePointer(ch),
                                          buffer.getReadPointer(ch),
                                          numSamples);
    }

    // Copy to wet buffer for processing (real-time safe - no allocations)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::FloatVectorOperations::copy(wetBuffer.getWritePointer(ch),
                                          dryBuffer.getReadPointer(ch),
                                          numSamples);
    }

    // Process lane 1 (wet signal)
    if (lane1 && lane1Enabled)
    {
        lane1->processBlock(wetBuffer, numSamples);
    }
    else
    {
        wetBuffer.clear(0, numSamples);
    }

    // Mix dry + wet (70/30 for now, mix parameter will be added later)
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* outData = buffer.getWritePointer(channel);
        const auto* dryData = dryBuffer.getReadPointer(channel);
        const auto* wetData = wetBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            outData[sample] = dryData[sample] * 0.7f + wetData[sample] * 0.3f;
        }
    }
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

// Helper functions
void OuariconPolystutterAudioProcessor::updateBeatSync(const juce::Optional<juce::AudioPlayHead::PositionInfo>& posInfo)
{
    // Check if we have valid playhead info
    if (!posInfo.hasValue())
    {
        // Offline rendering or no playhead available - use default BPM
        currentBPM = 120.0;

        // Update lane timing with default BPM
        if (lane1)
            lane1->updateTempo(currentBPM, spec.sampleRate);

        return;
    }

    auto& info = *posInfo;

    // Get BPM (use default if not available, with validation)
    if (info.getBpm().hasValue())
    {
        double bpm = *info.getBpm();
        // Validate BPM to prevent division by zero and unreasonable values
        currentBPM = juce::jlimit(20.0, 999.0, bpm);
    }
    else
    {
        currentBPM = 120.0;
    }

    // Update lane timing
    if (lane1)
        lane1->updateTempo(currentBPM, spec.sampleRate);

    // Get PPQ position
    if (!info.getPpqPosition().hasValue())
        return;

    double ppqPosition = *info.getPpqPosition();

    // Check if transport is playing
    bool isPlaying = info.getIsPlaying();

    // Detect subdivision boundaries and trigger
    if (isPlaying)
    {
        // Use cached subdivision parameter (no string lookup)
        int subdivIndex = static_cast<int>(lane1SubdivParam->load());

        // Calculate subdivision in PPQ units
        double subdivisionPPQ = 0.0;
        switch (subdivIndex)
        {
            case 0: subdivisionPPQ = 1.0;       // 1/4 note
                break;
            case 1: subdivisionPPQ = 0.5;       // 1/8 note
                break;
            case 2: subdivisionPPQ = 0.25;      // 1/16 note
                break;
            case 3: subdivisionPPQ = 0.125;     // 1/32 note
                break;
            case 4: subdivisionPPQ = 1.0 / 3.0; // 1/8T (triplet)
                break;
            case 5: subdivisionPPQ = 1.0 / 6.0; // 1/16T (triplet)
                break;
        }

        // Check if we crossed a subdivision boundary
        if (wasPlaying)
        {
            // Calculate current and last subdivision positions
            int currentSubdiv = static_cast<int>(ppqPosition / subdivisionPPQ);
            int lastSubdiv = static_cast<int>(lastPPQPosition / subdivisionPPQ);

            // Trigger on subdivision boundary
            if (currentSubdiv > lastSubdiv)
            {
                if (lane1)
                    lane1->trigger();
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
