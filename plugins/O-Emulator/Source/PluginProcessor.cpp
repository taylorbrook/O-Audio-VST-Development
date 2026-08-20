/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): passthrough shell. The APVTS parameters below are the
    BINDING contract from .planning/parameter-spec.md — IDs, types, ranges and
    defaults are frozen; they drive nothing until Stage 2.

  ==============================================================================
*/

#include "PluginProcessor.h"

// NOTE: no PluginEditor.h include in this TU — when Stage 3 adds the WebView
// editor, its include goes inside a #if JUCE_WEB_BROWSER guard directly above
// createEditor() so the render harness (JUCE_WEB_BROWSER=0, no editor sources)
// keeps compiling this file (pattern_render_harness_breaks_on_webview_editor).

juce::AudioProcessorValueTreeState::ParameterLayout OEmulatorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // console — selects the emulated system as one coherent pipeline (codec,
    // fixed internal rate, interpolation, output stage). 5 entries satisfies
    // the AudioParameterChoice >= 2-choices constraint. ASCII-only labels.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "console", 1 },
        "Console",
        juce::StringArray { "SNES", "PS1", "NES", "Game Boy", "Genesis" },
        0));

    // Four macro floats, all 0-100 % linear (no skew — factory presets
    // authored later as fractions stay safe from the skew trap).
    const auto percent = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            def,
            juce::AudioParameterFloatAttributes().withLabel("%"));
    };

    layout.add(percent("crush",  "Crush",  50.0f));
    layout.add(percent("age",    "Age",    20.0f));
    layout.add(percent("reverb", "Reverb",  0.0f));
    layout.add(percent("mix",    "Mix",   100.0f));

    return layout;
}

OEmulatorAudioProcessor::OEmulatorAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

OEmulatorAudioProcessor::~OEmulatorAudioProcessor() = default;

void OEmulatorAudioProcessor::prepareToPlay(double, int)
{
    // Stage 1: nothing to prepare. Phase 2.1 computes the constant worst-case
    // latency figure here and pairs setLatencySamples(N) with
    // DryWetMixer::setWetLatency(N) atomically.
}

void OEmulatorAudioProcessor::releaseResources()
{
}

bool OEmulatorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Strictly stereo in / stereo out — the whole pipeline (codecs L/R,
    // SPU stereo-cross reverb) is stereo-native.
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void OEmulatorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Stage 1: pure passthrough — input stays in the buffer untouched.
}

juce::AudioProcessorEditor* OEmulatorAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);   // Stages 1-2; WebView editor lands in Stage 3
}

void OEmulatorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Version stamp from day one — every house preset-migration gate keys off
    // this attribute, and adding it at v1.0.0 costs nothing.
    state.setProperty("pluginVersion", JucePlugin_VersionString, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OEmulatorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OEmulatorAudioProcessor();
}
