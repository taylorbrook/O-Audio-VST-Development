/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-Bitrot - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OBitrotAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========================================================================
    // GLOBAL (6)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CLOCK_MODE", 1 },
        "Clock Mode",
        juce::StringArray { "Sync", "Free" },
        0  // Default: Sync
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CLOCK_SYNC_DIV", 1 },
        "Clock Division",
        juce::StringArray { "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar" },
        2  // Default: 1/8
    ));

    {
        auto range = juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f);
        range.setSkewForCentre(1.414f);  // sqrt(0.1 * 20) — exponential feel
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "CLOCK_FREE_RATE", 1 },
            "Clock Rate",
            range,
            2.0f,
            "Hz"
        ));
    }

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "SEED", 1 },
        "Seed",
        0, 9999,
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "HARD_EDGES", 1 },
        "Hard Edges",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // TAPE (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "TAPE_ENABLE", 1 },
        "Tape Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_PROB", 1 },
        "Tape Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_STOP_PROB", 1 },
        "Tape-Stop Share",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        10.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TAPE_RAMP", 1 },
        "Tape Ramp",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f),
        150.0f,
        "ms"
    ));

    // ========================================================================
    // CD SKIP (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CD_ENABLE", 1 },
        "CD Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_PROB", 1 },
        "CD Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_SEVERITY", 1 },
        "CD Severity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CD_SEGMENT", 1 },
        "CD Segment",
        juce::NormalisableRange<float>(10.0f, 400.0f, 0.1f),
        100.0f,
        "ms"
    ));

    // ========================================================================
    // VINYL (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "VINYL_ENABLE", 1 },
        "Vinyl Enable",
        true
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_PROB", 1 },
        "Vinyl Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "VINYL_RPM", 1 },
        "Vinyl RPM",
        juce::StringArray { "33 1/3", "45" },  // ASCII-only; UI renders 33 1/3 glyph in Stage 3
        0  // Default: 33 1/3
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VINYL_POP", 1 },
        "Vinyl Pop",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // ========================================================================
    // PACKET LOSS (4)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "PACKET_ENABLE", 1 },
        "Packet Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_LOSS", 1 },
        "Packet Loss",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        20.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PACKET_BURST", 1 },
        "Packet Burstiness",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        30.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "PACKET_CONCEAL", 1 },
        "Concealment",
        juce::StringArray { "Silence", "Repeat", "Decay", "Substitute" },
        2  // Default: Decay
    ));

    // ========================================================================
    // CODEC (3)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CODEC_ENABLE", 1 },
        "Codec Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "CODEC_MODE", 1 },
        "Codec Mode",
        juce::StringArray { "Mu-law", "GSM" },  // ASCII-only; UI renders the mu glyph in Stage 3
        0  // Default: Mu-law
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CODEC_MIX", 1 },
        "Codec Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // ========================================================================
    // CRUSH (6)
    // ========================================================================

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "CRUSH_ENABLE", 1 },
        "Crush Enable",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_BITS", 1 },
        "Crush Bits",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.01f),
        16.0f,
        "bits"
    ));

    {
        auto range = juce::NormalisableRange<float>(500.0f, 20000.0f, 1.0f);
        range.setSkewForCentre(3162.0f);  // sqrt(500 * 20000) — exponential feel
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "CRUSH_RATE", 1 },
            "Crush Rate",
            range,
            20000.0f,
            "Hz"
        ));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_JITTER", 1 },
        "Crush Jitter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_ENV_AMT", 1 },
        "Crush Env Amount",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CRUSH_DITHER", 1 },
        "Crush Dither",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        0.0f,
        "LSB"
    ));

    return layout;
}

OBitrotAudioProcessor::OBitrotAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter pointers (atomic, real-time safe).
    // Global
    clockModeParam     = apvts.getRawParameterValue("CLOCK_MODE");
    clockSyncDivParam  = apvts.getRawParameterValue("CLOCK_SYNC_DIV");
    clockFreeRateParam = apvts.getRawParameterValue("CLOCK_FREE_RATE");
    seedParam          = apvts.getRawParameterValue("SEED");
    hardEdgesParam     = apvts.getRawParameterValue("HARD_EDGES");
    mixParam           = apvts.getRawParameterValue("MIX");

    // Tape
    tapeEnableParam   = apvts.getRawParameterValue("TAPE_ENABLE");
    tapeProbParam     = apvts.getRawParameterValue("TAPE_PROB");
    tapeStopProbParam = apvts.getRawParameterValue("TAPE_STOP_PROB");
    tapeRampParam     = apvts.getRawParameterValue("TAPE_RAMP");

    // CD Skip
    cdEnableParam   = apvts.getRawParameterValue("CD_ENABLE");
    cdProbParam     = apvts.getRawParameterValue("CD_PROB");
    cdSeverityParam = apvts.getRawParameterValue("CD_SEVERITY");
    cdSegmentParam  = apvts.getRawParameterValue("CD_SEGMENT");

    // Vinyl
    vinylEnableParam = apvts.getRawParameterValue("VINYL_ENABLE");
    vinylProbParam   = apvts.getRawParameterValue("VINYL_PROB");
    vinylRpmParam    = apvts.getRawParameterValue("VINYL_RPM");
    vinylPopParam    = apvts.getRawParameterValue("VINYL_POP");

    // Packet Loss
    packetEnableParam  = apvts.getRawParameterValue("PACKET_ENABLE");
    packetLossParam    = apvts.getRawParameterValue("PACKET_LOSS");
    packetBurstParam   = apvts.getRawParameterValue("PACKET_BURST");
    packetConcealParam = apvts.getRawParameterValue("PACKET_CONCEAL");

    // Codec
    codecEnableParam = apvts.getRawParameterValue("CODEC_ENABLE");
    codecModeParam   = apvts.getRawParameterValue("CODEC_MODE");
    codecMixParam    = apvts.getRawParameterValue("CODEC_MIX");

    // Crush
    crushEnableParam = apvts.getRawParameterValue("CRUSH_ENABLE");
    crushBitsParam   = apvts.getRawParameterValue("CRUSH_BITS");
    crushRateParam   = apvts.getRawParameterValue("CRUSH_RATE");
    crushJitterParam = apvts.getRawParameterValue("CRUSH_JITTER");
    crushEnvAmtParam = apvts.getRawParameterValue("CRUSH_ENV_AMT");
    crushDitherParam = apvts.getRawParameterValue("CRUSH_DITHER");
}

OBitrotAudioProcessor::~OBitrotAudioProcessor()
{
}

void OBitrotAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Stage 1: passthrough shell — DSP initialization lands in Stage 2.
    // NOTE: no setLatencySamples() call in Stage 1 (reports 0); the constant
    // ceil(0.020 * fs) scheme arrives with the compensated read head in Stage 2.
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OBitrotAudioProcessor::releaseResources()
{
    // Stage 1: nothing to release — DSP lands in Stage 2.
}

bool OBitrotAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo-in / stereo-out only
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void OBitrotAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have corresponding input data.
    // Bound by buffer.getNumChannels() (Standalone canonical-channelset trap).
    for (int channel = totalNumInputChannels;
         channel < totalNumOutputChannels && channel < buffer.getNumChannels();
         ++channel)
    {
        buffer.clear(channel, 0, buffer.getNumSamples());
    }

    // Stage 1: bit-transparent passthrough — no DSP.
}

juce::AudioProcessorEditor* OBitrotAudioProcessor::createEditor()
{
    return new OBitrotAudioProcessorEditor(*this);
}

void OBitrotAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OBitrotAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBitrotAudioProcessor();
}
