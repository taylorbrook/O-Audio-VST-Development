/*
   This file is part of O-Chorus, an Ouaricon Audio plugin.
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

    O-Chorus - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

juce::AudioProcessorValueTreeState::ParameterLayout OChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rate", 1}, "Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.35f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1}, "Depth", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voices", 1}, "Voices", 1, 8, 4));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"spread", 1}, "Spread", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"width", 1}, "Width", 0.0f, 1.0f, 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tone", 1}, "Tone",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1}, "Drive", 0.0f, 1.0f, 0.3f));

    return {params.begin(), params.end()};
}

OChorusAudioProcessor::OChorusAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Chorus")
{
    initializeFactoryPresets();
}

void OChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    chorusEngine.prepare(sampleRate, samplesPerBlock);
}

void OChorusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read all parameters via atomic loads (real-time safe)
    float rate   = parameters.getRawParameterValue("rate")->load();
    float depth  = parameters.getRawParameterValue("depth")->load();
    int   voices = static_cast<int>(parameters.getRawParameterValue("voices")->load());
    float spread = parameters.getRawParameterValue("spread")->load();
    float width  = parameters.getRawParameterValue("width")->load();
    float tone   = parameters.getRawParameterValue("tone")->load();
    float mix    = parameters.getRawParameterValue("mix")->load();
    float drive  = parameters.getRawParameterValue("drive")->load();

    chorusEngine.process(buffer, rate, depth, voices, spread, width, tone, mix, drive);
}

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OChorusAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OChorusAudioProcessorEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void OChorusAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.3.0: the UI language rides the same tree as one more plain property.
    // Written BEFORE getStateAsXml(), because that method serialises
    // parameters.copyState() and would otherwise take a snapshot without it.
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OChorusAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());

    // v1.3.0: the UI language. Read AFTER setStateFromXml, which calls
    // parameters.replaceState() and therefore rebuilds the whole tree.
    //
    // isVoid() is the ONLY correct guard and toString() the only correct read.
    // getStateInformation writes a STRING var, but even a bool or int written
    // there would not survive: the XML round-trip does not preserve the type,
    // because NamedValueSet::setFromXmlAttributes rebuilds every property as
    // `var (value)` over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type). A pre-1.3.0 session has no
    // such property at all and the default (English) stands. languageIndex()
    // clamps anything that is neither "fr" nor "zh-Hans" to 0, so a hand-edited
    // value degrades to English rather than to a bad index.
    //
    // The editor PULLS this through the getUiLanguage native fn at page init
    // rather than being pushed from here — a push would race the WebView's load.
    const juce::var lang = parameters.state.getProperty("uiLanguage");

    if (! lang.isVoid())
        uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
}

//==============================================================================
// Program (Preset) API
//==============================================================================
int OChorusAudioProcessor::getNumPrograms()
{
    auto list = presetManager.getPresetList();
    return juce::jmax(1, list.size());
}

int OChorusAudioProcessor::getCurrentProgram()
{
    auto list = presetManager.getPresetList();
    return juce::jmax(0, list.indexOf(presetManager.getCurrentPresetName()));
}

void OChorusAudioProcessor::setCurrentProgram(int index)
{
    auto list = presetManager.getPresetList();
    if (index >= 0 && index < list.size())
        presetManager.loadPreset(list[index]);
}

const juce::String OChorusAudioProcessor::getProgramName(int index)
{
    auto list = presetManager.getPresetList();
    if (index >= 0 && index < list.size())
        return list[index];
    return {};
}

//==============================================================================
// Factory Presets
//==============================================================================
void OChorusAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();

    if (factoryDir.isDirectory() && factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
        return;

    std::vector<OuariconPresetManager::FactoryPresetDef> presets = {
        {
            "Classic",
            {{"rate", 0.432f}, {"depth", 0.4f}, {"voices", 0.143f},
             {"spread", 0.3f}, {"width", 0.7f}, {"tone", 0.5f},
             {"mix", 0.5f}, {"drive", 0.2f}},
            juce::var()
        },
        {
            "Lush",
            {{"rate", 0.352f}, {"depth", 0.6f}, {"voices", 0.714f},
             {"spread", 0.8f}, {"width", 0.9f}, {"tone", 0.4f},
             {"mix", 0.6f}, {"drive", 0.3f}},
            juce::var()
        },
        {
            "Shimmer",
            {{"rate", 0.722f}, {"depth", 0.3f}, {"voices", 0.429f},
             {"spread", 0.5f}, {"width", 0.8f}, {"tone", 0.75f},
             {"mix", 0.4f}, {"drive", 0.15f}},
            juce::var()
        },
        {
            "Ensemble",
            {{"rate", 0.517f}, {"depth", 0.5f}, {"voices", 1.0f},
             {"spread", 1.0f}, {"width", 1.0f}, {"tone", 0.35f},
             {"mix", 0.55f}, {"drive", 0.25f}},
            juce::var()
        },
        {
            "Vibrato",
            {{"rate", 0.834f}, {"depth", 0.7f}, {"voices", 0.0f},
             {"spread", 0.0f}, {"width", 0.0f}, {"tone", 0.5f},
             {"mix", 1.0f}, {"drive", 0.0f}},
            juce::var()
        },
        {
            "Warm",
            {{"rate", 0.517f}, {"depth", 0.45f}, {"voices", 0.286f},
             {"spread", 0.4f}, {"width", 0.6f}, {"tone", 0.25f},
             {"mix", 0.5f}, {"drive", 0.5f}},
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(presets);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OChorusAudioProcessor();
}
