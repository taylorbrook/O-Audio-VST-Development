/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - Audio Processor implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation). See PluginProcessor.h.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <iterator>
#include <memory>

namespace
{
    //==========================================================================
    // Layout table. One row per parameter kind; the builder below expands it.
    //==========================================================================
    enum class Kind { toggle, choice, linear, centred };

    struct ParamDef
    {
        const char* id;             // full ID (global rows) or ID suffix (band rows)
        const char* name;           // full name (global rows) or name suffix (band rows)
        Kind kind;
        float start;                // linear / centred
        float end;                  // linear / centred
        float centre;               // centred: the value that sits at normalised 0.5
        float defaultValue;         // linear / centred, engineering units
        const char* const* choices; // choice
        int numChoices;             // choice
        int defaultIndex;           // choice
    };

    // Host-visible and English by contract (they are not UI copy).
    const char* const kTypeChoices[] = { "Bell", "Low Shelf", "High Shelf", "Low Cut",
                                         "High Cut", "Notch", "Band Pass", "Tilt Shelf" };

    const char* const kSlopeChoices[] = { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct",
                                          "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct" };

    const char* const kPlacementChoices[] = { "Stereo", "Left", "Right", "Mid", "Side" };

    constexpr int kNumTypeChoices      = static_cast<int> (std::size (kTypeChoices));
    constexpr int kNumSlopeChoices     = static_cast<int> (std::size (kSlopeChoices));
    constexpr int kNumPlacementChoices = static_cast<int> (std::size (kPlacementChoices));

    static_assert (kNumTypeChoices == 8 && kNumSlopeChoices == 8 && kNumPlacementChoices == 5,
                   "choice counts are part of the frozen layout (parameter-spec.md)");

    // Group `global`, in registration order.
    const ParamDef kGlobalDefs[] =
    {
        { "bypass",      "Bypass",      Kind::toggle, 0.0f,   1.0f,   0.0f, 0.0f,   nullptr, 0, 0 },
        { "output_gain", "Output Gain", Kind::linear, -36.0f, 36.0f,  0.0f, 0.0f,   nullptr, 0, 0 },
        { "gain_scale",  "Gain Scale",  Kind::linear, 0.0f,   200.0f, 0.0f, 100.0f, nullptr, 0, 0 },
        { "auto_gain",   "Auto Gain",   Kind::toggle, 0.0f,   1.0f,   0.0f, 0.0f,   nullptr, 0, 0 },
    };

    // Groups `band01` ... `band24`, in registration order inside each band.
    // `slope` is registered on every band whatever its type. Nothing in Stage 1
    // interprets `type`, `slope`, `q` or `placement`.
    const ParamDef kBandDefs[] =
    {
        { "enabled",   "On",        Kind::toggle,  0.0f,   1.0f,     0.0f,    0.0f,    nullptr,           0,                    0 },
        { "bypass",    "Bypass",    Kind::toggle,  0.0f,   1.0f,     0.0f,    0.0f,    nullptr,           0,                    0 },
        { "type",      "Type",      Kind::choice,  0.0f,   0.0f,     0.0f,    0.0f,    kTypeChoices,      kNumTypeChoices,      0 },
        { "freq",      "Freq",      Kind::centred, 10.0f,  30000.0f, 1000.0f, 1000.0f, nullptr,           0,                    0 },
        { "gain",      "Gain",      Kind::linear,  -30.0f, 30.0f,    0.0f,    0.0f,    nullptr,           0,                    0 },
        { "q",         "Q",         Kind::centred, 0.025f, 40.0f,    1.0f,    1.0f,    nullptr,           0,                    0 },
        { "slope",     "Slope",     Kind::choice,  0.0f,   0.0f,     0.0f,    0.0f,    kSlopeChoices,     kNumSlopeChoices,     1 },
        { "placement", "Placement", Kind::choice,  0.0f,   0.0f,     0.0f,    0.0f,    kPlacementChoices, kNumPlacementChoices, 0 },
    };

    /** Every parameter carries version hint 1. Every float range has interval 0
        and is a plain start / end / skew range: the WebView slider frontend
        reads those three numbers from the relay, so a range built from mapping
        functions would be invisible to it. */
    std::unique_ptr<juce::RangedAudioParameter> makeParameter (const juce::String& id,
                                                               const juce::String& name,
                                                               const ParamDef& def)
    {
        const juce::ParameterID parameterId { id, 1 };

        if (def.kind == Kind::toggle)
            return std::make_unique<juce::AudioParameterBool> (parameterId, name, false);

        if (def.kind == Kind::choice)
            return std::make_unique<juce::AudioParameterChoice> (parameterId, name,
                                                                 juce::StringArray (def.choices, def.numChoices),
                                                                 def.defaultIndex);

        juce::NormalisableRange<float> range (def.start, def.end, 0.0f);

        if (def.kind == Kind::centred)
            range.setSkewForCentre (def.centre);

        return std::make_unique<juce::AudioParameterFloat> (parameterId, name, range, def.defaultValue);
    }

    //==========================================================================
    // State helpers
    //==========================================================================
    void removeChildrenOfType (juce::ValueTree& tree, const juce::Identifier& type)
    {
        for (int i = tree.getNumChildren(); --i >= 0;)
            if (tree.getChild (i).hasType (type))
                tree.removeChild (i, nullptr);
    }

    /** Reads an integer property. Everything read back from state XML is a
        string var, so the value is converted by type, never compared as text.
        An absent property returns false and leaves `out` untouched. */
    bool readInt (const juce::ValueTree& tree, const juce::Identifier& name, int& out)
    {
        const auto value = tree.getProperty (name);

        if (value.isVoid())
            return false;

        out = (int) value;
        return true;
    }
}

//==============================================================================
// RESERVED PARAMETER IDS - NOT REGISTERED IN v1 (dynamic EQ)
//
// 72 IDs, NN = 01 ... 24, reserved with version hint 2:
//
//     bNN_dyn_range        (24)
//     bNN_dyn_threshold    (24)
//     bNN_dyn_auto         (24)
//
// They are not registered here: registering them now would freeze ranges the
// dynamic-EQ design has not chosen. Hint-2 parameters added later move no v1
// parameter (AUv2 orders by version hint, then ID hash; VST3 parameter IDs are
// ID hashes). These names must not be used for anything else. A v1 session
// opened after they exist loads them at their defaults; a later session opened
// in v1 keeps their state children untouched and saves them back.
//
// REGISTRATION ORDER (parameter-spec.md "Full Registration Table (196)"):
//
//     group global            bypass, output_gain, gain_scale, auto_gain
//     group band01 ... band24  enabled, bypass, type, freq, gain, q, slope, placement
//
// IDs are zero-padded (b01_freq ... b24_freq); host-visible names are not
// (B1 Freq ... B24 Freq). 50 Bool / 72 Choice / 74 Float.
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OuariconParametricEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto global = std::make_unique<juce::AudioProcessorParameterGroup> ("global", "Global", "|");

    for (const auto& def : kGlobalDefs)
        global->addChild (makeParameter (def.id, def.name, def));

    layout.add (std::move (global));

    for (int n = 1; n <= kNumBands; ++n)
    {
        const auto number = juce::String (n);
        const auto padded = number.paddedLeft ('0', 2);

        const juce::String idPrefix   = "b" + padded + "_";
        const juce::String namePrefix = "B" + number + " ";

        auto band = std::make_unique<juce::AudioProcessorParameterGroup> ("band" + padded,
                                                                          "Band " + number,
                                                                          "|");

        for (const auto& def : kBandDefs)
            band->addChild (makeParameter (idPrefix + def.id, namePrefix + def.name, def));

        layout.add (std::move (band));
    }

    return layout;
}

//==============================================================================
OuariconParametricEQAudioProcessor::OuariconParametricEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OParametricEQ"), createParameterLayout())
{
    bypassParam = parameters.getParameter ("bypass");
    jassert (bypassParam != nullptr);
}

//==============================================================================
void OuariconParametricEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    // The shell adds no delay. This is the only place the reported latency is set.
    setLatencySamples (0);
}

bool OuariconParametricEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void OuariconParametricEQAudioProcessor::passThrough (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channel that has no input. With the two accepted
    // layouts there is none, so no sample is touched and no parameter is read.
    const int numChannels = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels());

    for (int channel = getTotalNumInputChannels(); channel < numChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

void OuariconParametricEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    passThrough (buffer);
}

void OuariconParametricEQAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                               juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    passThrough (buffer);
}

//==============================================================================
juce::AudioProcessorEditor* OuariconParametricEQAudioProcessor::createEditor()
{
    // Stage 1: the generic editor, unconditionally. Phase 3.1 swaps in the
    // WebView editor behind a JUCE_WEB_BROWSER guard.
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
// State format v1
//
//   <OParametricEQ stateVersion="1" pluginVersion="0.1.0" uiLanguage="en">
//     <PARAM id="bypass" value="0.0"/>            ... 196 ...
//     <UI analyzerMode="2" displayRange="12" selectedBand="0"
//         editorWidth="960" editorHeight="600"/>
//   </OParametricEQ>
//
// The UI child carries no `id` attribute: APVTS looks up the `id` of every
// top-level child as a parameter ID.
//==============================================================================
void OuariconParametricEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const juce::Identifier uiType ("UI");

    // A copy. The live tree is never edited here.
    auto state = parameters.copyState();

    // There should be none (restore strips them); this keeps the save idempotent.
    removeChildrenOfType (state, uiType);

    // Always version 1, also when the session came from a later version: its
    // unknown PARAM children are still in the tree and are written back.
    state.setProperty ("stateVersion", 1, nullptr);
    state.setProperty ("pluginVersion", juce::String (JucePlugin_VersionString), nullptr);
    state.setProperty ("uiLanguage", languageCode (uiLanguage.load()), nullptr);

    juce::ValueTree ui (uiType);
    ui.setProperty ("analyzerMode", analyzerMode.load(),   nullptr);
    ui.setProperty ("displayRange", uiDisplayRange.load(), nullptr);
    ui.setProperty ("selectedBand", uiSelectedBand.load(), nullptr);
    ui.setProperty ("editorWidth",  uiEditorWidth.load(),  nullptr);
    ui.setProperty ("editorHeight", uiEditorHeight.load(), nullptr);
    state.appendChild (ui, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OuariconParametricEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // 1. A blob that is not ours changes nothing: no parameter, no UI value.
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    // 2.
    auto tree = juce::ValueTree::fromXml (*xml);

    if (! tree.isValid())
        return;

    // Read, not branched on: v1 loads every version it is handed. A later
    // format that needs a migration gates on this number.
    const int stateVersion = (int) tree.getProperty ("stateVersion", 1);
    juce::ignoreUnused (stateVersion);

    // 3. UI state. An absent or invalid value leaves its member unchanged;
    //    the editor size is clamped instead of refused.
    const juce::Identifier uiType ("UI");
    const auto ui = tree.getChildWithName (uiType);

    if (ui.isValid())
    {
        int value = 0;

        if (readInt (ui, "analyzerMode", value) && value >= 0 && value <= 3)
            analyzerMode.store (value);

        if (readInt (ui, "displayRange", value) && (value == 6 || value == 12 || value == 30))
            uiDisplayRange.store (value);

        if (readInt (ui, "selectedBand", value) && value >= 0 && value <= kNumBands)
            uiSelectedBand.store (value);

        if (readInt (ui, "editorWidth", value))
            uiEditorWidth.store (juce::jlimit (800, 1600, value));

        if (readInt (ui, "editorHeight", value))
            uiEditorHeight.store (juce::jlimit (500, 1000, value));
    }

    const auto language = tree.getProperty ("uiLanguage");

    if (! language.isVoid())
        uiLanguage.store (languageIndex (language.toString()));

    // 4. A UI child left in the live tree would be saved again next to the
    //    fresh one and shadow it on the second reopen.
    removeChildrenOfType (tree, uiType);

    // 5. A parameter the blob does not mention must load at its default. APVTS
    //    would keep its current value instead, so the default goes into the
    //    incoming tree. This edits the tree; it writes no parameter.
    for (auto* parameter : getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter);

        if (ranged == nullptr)
            continue;

        const auto id = ranged->getParameterID();

        if (tree.getChildWithProperty ("id", id).isValid())
            continue;

        juce::ValueTree child ("PARAM");
        child.setProperty ("id", id, nullptr);
        child.setProperty ("value", ranged->convertFrom0to1 (ranged->getDefaultValue()), nullptr);
        tree.appendChild (child, nullptr);
    }

    // 6. Unknown PARAM children stay in the tree and are saved back.
    parameters.replaceState (tree);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconParametricEQAudioProcessor();
}
