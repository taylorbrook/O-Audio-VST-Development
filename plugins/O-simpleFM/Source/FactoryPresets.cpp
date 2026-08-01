/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
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

    FactoryPresets.cpp
    O-simpleFM — factory preset library.

    Each lesson preset is a complete 17-parameter snapshot in RAW (real) units,
    matching the in-UI "Lesson Presets" tour. Raw values are converted to
    APVTS-normalized [0,1] via convertTo0to1 at build time so the on-disk JSON
    (which stores normalized values) round-trips through setValueNotifyingHost.

    "Default" is captured from each parameter's own normalized default value so
    it always tracks createParameterLayout() with no hand-maintained numbers.

  ==============================================================================
*/

#include "FactoryPresets.h"
#include "PluginProcessor.h"   // OSimpleFM::ParamIDs
#include <map>

namespace
{
using Preset = OuariconPresetManager::FactoryPresetDef;
using RawMap = std::map<juce::String, float>;

/** Convert a raw-value map to APVTS-normalized [0,1] (skips unknown IDs). */
static std::map<juce::String, float>
normalize (juce::AudioProcessorValueTreeState& apvts, const RawMap& raw)
{
    std::map<juce::String, float> out;
    for (const auto& [id, value] : raw)
        if (auto* p = apvts.getParameter (id))
            out[id] = p->convertTo0to1 (value);
    return out;
}

static Preset makePreset (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& name, const RawMap& raw)
{
    Preset def;
    def.name = name;
    def.parameters = normalize (apvts, raw);
    return def;
}

/** "Default" = every parameter at its own (already normalized) default value. */
static Preset makeDefaultPreset (juce::AudioProcessorValueTreeState& apvts)
{
    using namespace OSimpleFM::ParamIDs;
    const juce::StringArray ids {
        ratio, ratioSnap, modIndex, feedback, modFixedMode, modFixedHz,
        modEnvToIndex, velToIndex, modAttack, modDecay, modSustain, modRelease,
        ampAttack, ampDecay, ampSustain, ampRelease, outputLevel
    };

    Preset def;
    def.name = "Default";
    for (const auto& id : ids)
        if (auto* p = apvts.getParameter (id))
            def.parameters[id] = p->getDefaultValue();   // already normalized
    return def;
}
} // namespace

namespace FactoryPresets
{
std::vector<OuariconPresetManager::FactoryPresetDef>
build (juce::AudioProcessorValueTreeState& apvts)
{
    using namespace OSimpleFM::ParamIDs;

    std::vector<Preset> presets;
    presets.push_back (makeDefaultPreset (apvts));

    // E-Piano — ratio 1:1 + fast mod-envelope: bright pluck that mellows to a sine.
    presets.push_back (makePreset (apvts, "E-Piano", {
        { ratio, 1.0f }, { ratioSnap, 1.0f }, { modIndex, 5.5f }, { feedback, 0.0f },
        { modFixedMode, 0.0f }, { modFixedHz, 220.0f }, { modEnvToIndex, 1.0f }, { velToIndex, 0.6f },
        { modAttack, 0.001f }, { modDecay, 0.45f }, { modSustain, 0.0f }, { modRelease, 0.3f },
        { ampAttack, 0.002f }, { ampDecay, 1.2f }, { ampSustain, 0.0f }, { ampRelease, 0.4f },
        { outputLevel, -3.0f } }));

    // Tubular Bell — inharmonic ratio 1.41 sprays non-integer sidebands → metallic ring.
    presets.push_back (makePreset (apvts, "Tubular Bell", {
        { ratio, 1.41f }, { ratioSnap, 0.0f }, { modIndex, 8.0f }, { feedback, 0.0f },
        { modFixedMode, 0.0f }, { modFixedHz, 220.0f }, { modEnvToIndex, 1.0f }, { velToIndex, 0.0f },
        { modAttack, 0.001f }, { modDecay, 2.5f }, { modSustain, 0.0f }, { modRelease, 2.0f },
        { ampAttack, 0.001f }, { ampDecay, 3.0f }, { ampSustain, 0.0f }, { ampRelease, 3.0f },
        { outputLevel, -4.0f } }));

    // Brass — ratio 1:1, index rises with the amp envelope; sustained, vowel-bright.
    presets.push_back (makePreset (apvts, "Brass", {
        { ratio, 1.0f }, { ratioSnap, 1.0f }, { modIndex, 4.0f }, { feedback, 0.0f },
        { modFixedMode, 0.0f }, { modFixedHz, 220.0f }, { modEnvToIndex, 0.8f }, { velToIndex, 0.4f },
        { modAttack, 0.08f }, { modDecay, 0.2f }, { modSustain, 0.7f }, { modRelease, 0.15f },
        { ampAttack, 0.06f }, { ampDecay, 0.1f }, { ampSustain, 0.85f }, { ampRelease, 0.2f },
        { outputLevel, -4.0f } }));

    // Clarinet — ratio 2:1 + low index emphasises odd harmonics → hollow, woody tone.
    presets.push_back (makePreset (apvts, "Clarinet", {
        { ratio, 2.0f }, { ratioSnap, 1.0f }, { modIndex, 2.2f }, { feedback, 0.0f },
        { modFixedMode, 0.0f }, { modFixedHz, 220.0f }, { modEnvToIndex, 0.3f }, { velToIndex, 0.2f },
        { modAttack, 0.04f }, { modDecay, 0.1f }, { modSustain, 0.9f }, { modRelease, 0.1f },
        { ampAttack, 0.03f }, { ampDecay, 0.1f }, { ampSustain, 0.9f }, { ampRelease, 0.12f },
        { outputLevel, -3.0f } }));

    // Clang Bell — high index + feedback smears the spectrum into a dense, noisy strike.
    presets.push_back (makePreset (apvts, "Clang Bell", {
        { ratio, 3.46f }, { ratioSnap, 0.0f }, { modIndex, 14.0f }, { feedback, 0.6f },
        { modFixedMode, 0.0f }, { modFixedHz, 220.0f }, { modEnvToIndex, 1.0f }, { velToIndex, 0.0f },
        { modAttack, 0.001f }, { modDecay, 1.8f }, { modSustain, 0.1f }, { modRelease, 1.5f },
        { ampAttack, 0.001f }, { ampDecay, 2.2f }, { ampSustain, 0.0f }, { ampRelease, 2.2f },
        { outputLevel, -6.0f } }));

    return presets;
}
} // namespace FactoryPresets
