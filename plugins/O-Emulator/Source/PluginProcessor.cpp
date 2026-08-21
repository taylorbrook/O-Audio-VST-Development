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

    Stage 2 COMPLETE (Phase 2.4): all five console pipelines, SPU reverb,
    click-safe crossfaded switching, age model (noise/hum bed, dulling,
    drift) and the full crush macro (drive, integer steps with micro-fades,
    AA-open). The APVTS parameters below are the BINDING contract from
    .planning/parameter-spec.md — IDs, types, ranges and defaults are frozen.
    All five macros are live.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

// NOTE: no top-of-file PluginEditor.h include in this TU — the WebView
// editor's include lives inside a #if JUCE_WEB_BROWSER guard directly above
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
    ageParam     = apvts.getRawParameterValue("age");
    jassert(crushParam != nullptr && mixParam != nullptr && consoleParam != nullptr
            && reverbParam != nullptr && ageParam != nullptr);

    // ── Factory preset bank (Stage 4) — 16 presets ──────────────────────────
    // Authored DENORMALIZED in engineering units (console = choice index:
    // SNES=0, PS1=1, NES=2, Game Boy=3, Genesis=4; floats = percent), then
    // batch-converted through each parameter's NormalisableRange below. All
    // ranges are linear today, but the conversion is kept as the house idiom —
    // robust to any future range change.
    //
    // Every preset lists all 5 IDs (defense in depth over the module's
    // reset-to-defaults pass). Names are ASCII-only and slash-free — a name is
    // a FILENAME (sanitizer rewrites '/' silently). getPresetList() sorts
    // case-insensitively, so the browser order is alphabetical regardless of
    // declaration order here. Renaming a shipped factory preset strands its
    // old .json permanently — adding is safe, renaming is not.
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets {
        { "Crush Extreme",
          { { "console", 2.0f }, { "crush", 100.0f }, { "age", 30.0f }, { "reverb", 0.0f }, { "mix", 100.0f } } },
        { "GB Pocket Speaker",
          { { "console", 3.0f }, { "crush", 75.0f }, { "age", 55.0f }, { "reverb", 0.0f }, { "mix", 100.0f } } },
        { "GB Signature",
          { { "console", 3.0f }, { "crush", 40.0f }, { "age", 12.0f }, { "reverb", 0.0f }, { "mix", 100.0f } } },
        { "Genesis Arcade Floor",
          { { "console", 4.0f }, { "crush", 70.0f }, { "age", 50.0f }, { "reverb", 25.0f }, { "mix", 100.0f } } },
        { "Genesis Signature",
          { { "console", 4.0f }, { "crush", 45.0f }, { "age", 10.0f }, { "reverb", 0.0f }, { "mix", 100.0f } } },
        { "Lo-Fi Drums",
          { { "console", 0.0f }, { "crush", 55.0f }, { "age", 25.0f }, { "reverb", 8.0f }, { "mix", 100.0f } } },
        { "NES Basement",
          { { "console", 2.0f }, { "crush", 80.0f }, { "age", 70.0f }, { "reverb", 10.0f }, { "mix", 100.0f } } },
        { "NES Signature",
          { { "console", 2.0f }, { "crush", 45.0f }, { "age", 15.0f }, { "reverb", 0.0f }, { "mix", 100.0f } } },
        { "Parallel Grit",
          { { "console", 4.0f }, { "crush", 85.0f }, { "age", 35.0f }, { "reverb", 0.0f }, { "mix", 45.0f } } },
        { "PS1 Demo Disc",
          { { "console", 1.0f }, { "crush", 70.0f }, { "age", 45.0f }, { "reverb", 55.0f }, { "mix", 100.0f } } },
        { "PS1 Signature",
          { { "console", 1.0f }, { "crush", 40.0f }, { "age", 8.0f }, { "reverb", 30.0f }, { "mix", 100.0f } } },
        { "Reverb Chamber",
          { { "console", 1.0f }, { "crush", 20.0f }, { "age", 10.0f }, { "reverb", 85.0f }, { "mix", 100.0f } } },
        { "SNES Signature",
          { { "console", 0.0f }, { "crush", 35.0f }, { "age", 10.0f }, { "reverb", 12.0f }, { "mix", 100.0f } } },
        { "SNES Worn Cart",
          { { "console", 0.0f }, { "crush", 65.0f }, { "age", 60.0f }, { "reverb", 20.0f }, { "mix", 100.0f } } },
        { "Subtle Glue",
          { { "console", 0.0f }, { "crush", 15.0f }, { "age", 5.0f }, { "reverb", 0.0f }, { "mix", 35.0f } } },
        { "Tape Wash",
          { { "console", 1.0f }, { "crush", 30.0f }, { "age", 65.0f }, { "reverb", 40.0f }, { "mix", 85.0f } } },
    };

    // Engineering units → normalized through each parameter's
    // NormalisableRange, once, here. initializeFactoryPresets stores the
    // values verbatim and applyPresetJson feeds them back through
    // setValueNotifyingHost (normalized domain).
    for (auto& preset : factoryPresets)
        for (auto& [paramId, value] : preset.parameters)
            if (auto* p = apvts.getParameter(paramId))
                value = p->convertTo0to1(value);

    // Sentinel-gated (module v1.0.4+): file writes happen only when
    // JucePlugin_VersionString changes — auval/pluginval scan-storm safe.
    presetManager.initializeFactoryPresets(factoryPresets);
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
    engine.setAgePercent(ageParam->load());
    engine.setReverbSendPercent(reverbParam->load());
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixParam->load() * 0.01f));

    juce::dsp::AudioBlock<float> block(buffer);
    auto stereo = block.getSubsetChannelBlock(0, 2);

    dryWetMixer.pushDrySamples(stereo);   // clean dry BEFORE the pipeline
    engine.process(buffer);               // wet, in place on ch 0/1
    dryWetMixer.mixWetSamples(stereo);    // dry delayed by setWetLatency
}

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OEmulatorAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OEmulatorAudioProcessorEditor(*this);
#else
    // Render harness builds with JUCE_WEB_BROWSER=0 and no editor sources.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void OEmulatorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Delegated to the preset manager (Stage 4) so `currentPreset` rides the
    // session state — the preset-name display survives a DAW save/reload.
    if (auto xml = presetManager.getStateAsXml())
    {
        // Version stamp kept from day one — every house preset-migration gate
        // keys off this attribute. getStateAsXml() does not write it, so it is
        // re-added here (as an XML attribute; the pre-Stage-4 code carried it
        // as a ValueTree property, which lands in the same place).
        xml->setAttribute("pluginVersion", JucePlugin_VersionString);
        copyXmlToBinary(*xml, destData);
    }
}

void OEmulatorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        presetManager.setStateFromXml(xmlState.get());
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OEmulatorAudioProcessor();
}
