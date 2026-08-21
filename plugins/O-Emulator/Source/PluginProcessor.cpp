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

    Stage 2 / Phase 2.2: SNES + PS1 pipelines + SPU reverb. The APVTS
    parameters below are the BINDING contract from .planning/parameter-spec.md
    — IDs, types, ranges and defaults are frozen. Wired so far: `crush`
    (drive + shift floor), `mix` (latency-compensated DryWetMixer), `console`
    (SNES/PS1; 2.2 interim hard switch, indices 2-4 map to SNES until the
    Phase 2.3 crossfader) and `reverb` (SPU send level). `age` drives nothing
    until Phase 2.4.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

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
    crushParam   = apvts.getRawParameterValue("crush");
    mixParam     = apvts.getRawParameterValue("mix");
    consoleParam = apvts.getRawParameterValue("console");
    reverbParam  = apvts.getRawParameterValue("reverb");
    jassert(crushParam != nullptr && mixParam != nullptr
            && consoleParam != nullptr && reverbParam != nullptr);
}

OEmulatorAudioProcessor::~OEmulatorAudioProcessor() = default;

void OEmulatorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Constant worst-case latency, all console modes (plan decision #2).
    // Order proven in O-Bitrot PluginProcessor.cpp:1263-1310: compute ->
    // jassert bound -> setLatencySamples -> prepare the stages (handing the
    // SAME integer to the engine, which aligns its structural delay onto it)
    // -> dryWetMixer.prepare -> linear rule -> setWetLatency AFTER prepare.
    totalLatencySamples = oemu::ConsoleEngine::computeLatencySamples(sampleRate);
    jassert(totalLatencySamples <= kMaxWetLatencySamples);
    setLatencySamples(totalLatencySamples);

    engine.prepare(sampleRate, samplesPerBlock, totalLatencySamples);

    const juce::dsp::ProcessSpec spec { sampleRate,
                                        (juce::uint32) juce::jmax(1, samplesPerBlock),
                                        2u };
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::linear);
    dryWetMixer.setWetLatency((float) totalLatencySamples);
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));
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

    const int numSamples = buffer.getNumSamples();

    // Bound by buffer.getNumChannels() (Standalone canonical-channelset trap).
    for (int ch = getTotalNumInputChannels();
         ch < getTotalNumOutputChannels() && ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);

    // Defensive: prepared (latency computed), stereo buffer, non-empty block.
    if (numSamples == 0 || buffer.getNumChannels() < 2 || totalLatencySamples <= 0)
        return;

    // Scrub non-finite INPUT at the boundary (QUAL-01). The DryWetMixer's
    // Thiran dry delay computes alpha*(x - v) and holds NaN FOREVER once
    // poisoned (0 * NaN == NaN even at integer delay) — and the codec/filter
    // state behind it fares no better. Finite samples are never touched, so
    // the mix-0% null stays bit-exact (FUNC-02). O-Bitrot pattern,
    // PluginProcessor.cpp:1364-1379.
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int n = 0; n < numSamples; ++n)
            if (! std::isfinite(d[n]))
                d[n] = 0.0f;
    }

    // Block-header parameter latch (atomics; consumed at chunk boundaries
    // inside the engine). Console read once per block (ARCHITECTURE thread
    // boundaries); the switch applies at the next chunk boundary.
    engine.setConsoleIndex((int) consoleParam->load());
    engine.setCrushPercent(crushParam->load());
    engine.setReverbSendPercent(reverbParam->load());
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));

    juce::dsp::AudioBlock<float> block(buffer);
    auto stereo = block.getSubsetChannelBlock(0, 2);

    dryWetMixer.pushDrySamples(stereo);   // clean dry BEFORE the pipeline
    engine.process(buffer);               // wet, in place on ch 0/1
    dryWetMixer.mixWetSamples(stereo);    // dry delayed by setWetLatency
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
