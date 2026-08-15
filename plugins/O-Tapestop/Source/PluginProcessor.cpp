/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
#include "PluginProcessor.h"

// The editor is WebView-bound from Stage 3; the Stage-2 render harness builds
// this translation unit with JUCE_WEB_BROWSER=0 and no editor sources
// (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

namespace
{
    // The single shared tempo-division list — deliberately triplet-free
    // (7 entries; NOT O-Bitrot's list). Indices: 0=1/16, 1=1/8, 2=1/4,
    // 3=1/2, 4=1 bar, 5=2 bars, 6=4 bars.
    juce::StringArray syncDivisionChoices()
    {
        return { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars" };
    }

    // 10–8000 ms, skew 0.35 — shared by STOP_FREE_MS / START_FREE_MS /
    // ENV_FREE_MS (parameter-spec.md, BINDING).
    juce::NormalisableRange<float> freeMsRange()
    {
        return { 10.0f, 8000.0f, 0.0f, 0.35f };
    }

    juce::NormalisableRange<float> percentRange()
    {
        return { 0.0f, 100.0f, 0.0f, 1.0f };
    }
} // namespace

TapestopProcessor::TapestopProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter atomics once — read per block on the audio thread
    // (suite convention; research/ARCHITECTURE.md line 348).
    pEngage       = parameters.getRawParameterValue("ENGAGE");
    pMode         = parameters.getRawParameterValue("MODE");
    pSyncMode     = parameters.getRawParameterValue("SYNC_MODE");
    pStopSyncDiv  = parameters.getRawParameterValue("STOP_SYNC_DIV");
    pStopFreeMs   = parameters.getRawParameterValue("STOP_FREE_MS");
    pStopCurve    = parameters.getRawParameterValue("STOP_CURVE");
    pStartSyncDiv = parameters.getRawParameterValue("START_SYNC_DIV");
    pStartFreeMs  = parameters.getRawParameterValue("START_FREE_MS");
    pStartCurve   = parameters.getRawParameterValue("START_CURVE");
    pEnvSyncDiv   = parameters.getRawParameterValue("ENV_SYNC_DIV");
    pEnvFreeMs    = parameters.getRawParameterValue("ENV_FREE_MS");
    pToneTrack    = parameters.getRawParameterValue("TONE_TRACK");
    pMix          = parameters.getRawParameterValue("MIX");
    pOutputGain   = parameters.getRawParameterValue("OUTPUT_GAIN");

    // getRawParameterValue returns nullptr only for an id missing from the
    // layout — a typo, never a runtime condition. Fail loudly here, in debug,
    // on first instantiation (O-ReverseDelay v1.7.3 IN-06 posture).
    jassert(pEngage != nullptr && pMode != nullptr && pSyncMode != nullptr
            && pStopSyncDiv != nullptr && pStopFreeMs != nullptr && pStopCurve != nullptr
            && pStartSyncDiv != nullptr && pStartFreeMs != nullptr && pStartCurve != nullptr
            && pEnvSyncDiv != nullptr && pEnvFreeMs != nullptr
            && pToneTrack != nullptr && pMix != nullptr && pOutputGain != nullptr);
}

TapestopProcessor::~TapestopProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout TapestopProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── Trigger & Mode ──────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "ENGAGE", 1 },
        "Engage",
        false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 },
        "Mode",
        juce::StringArray { "Stop", "Scratch" },
        0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "SYNC_MODE", 1 },
        "Sync Mode",
        juce::StringArray { "Sync", "Free" },
        0));

    // ── Stop / Start (Stop mode) ────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "STOP_SYNC_DIV", 1 },
        "Stop Time",
        syncDivisionChoices(),
        3)); // 1/2

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STOP_FREE_MS", 1 },
        "Stop Time (Free)",
        freeMsRange(),
        500.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STOP_CURVE", 1 },
        "Stop Curve",
        percentRange(),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "START_SYNC_DIV", 1 },
        "Start Time",
        syncDivisionChoices(),
        2)); // 1/4

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "START_FREE_MS", 1 },
        "Start Time (Free)",
        freeMsRange(),
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "START_CURVE", 1 },
        "Start Curve",
        percentRange(),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // ── Scratch (Scratch mode) ──────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "ENV_SYNC_DIV", 1 },
        "Env Length",
        syncDivisionChoices(),
        4)); // 1 bar

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ENV_FREE_MS", 1 },
        "Env Length (Free)",
        freeMsRange(),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // ── Output ──────────────────────────────────────────────────────────────
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TONE_TRACK", 1 },
        "Tone Track",
        percentRange(),
        60.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        percentRange(),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float> { -24.0f, 12.0f, 0.0f, 1.0f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return layout;
}

void TapestopProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Stage 2 wires the varispeed engine here. Zero latency — no
    // setLatencySamples call anywhere (Stage-0 constraint).
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void TapestopProcessor::releaseResources()
{
}

bool TapestopProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo or mono only; in == out; no side-chain, no multi-out
    // (research/ARCHITECTURE.md File I/O).
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    if (layouts.inputBuses.size() > 1 || layouts.outputBuses.size() > 1)
        return false;

    return true;
}

void TapestopProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Stage 1: hard pass-through ONLY. Bitwise transparency is contractual
    // (Stage-0 decision #6) — Stage 2's null probes assume the disengaged path
    // never touches samples. No smoothing, no gain, no mix stage here.
    const int numInputChannels  = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* TapestopProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new TapestopEditor(*this);
#else
    return nullptr;
#endif
}

bool TapestopProcessor::hasEditor() const
{
#if JUCE_WEB_BROWSER
    return true;
#else
    return false;
#endif
}

const juce::String TapestopProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TapestopProcessor::acceptsMidi() const           { return false; }
bool TapestopProcessor::producesMidi() const          { return false; }
bool TapestopProcessor::isMidiEffect() const          { return false; }
double TapestopProcessor::getTailLengthSeconds() const { return 0.0; }

int TapestopProcessor::getNumPrograms()                                { return 1; }
int TapestopProcessor::getCurrentProgram()                             { return 0; }
void TapestopProcessor::setCurrentProgram(int index)                   { juce::ignoreUnused(index); }
const juce::String TapestopProcessor::getProgramName(int index)        { juce::ignoreUnused(index); return {}; }
void TapestopProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void TapestopProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Standard APVTS XML round-trip. Stage 2.3's scratchEnvelopeJson property
    // rides this same ValueTree — nothing extra needed now.
    auto state = parameters.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void TapestopProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapestopProcessor();
}
