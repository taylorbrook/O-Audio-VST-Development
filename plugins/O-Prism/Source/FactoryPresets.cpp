/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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
    O-Prism - Factory preset library
    96 presets across 9 categories:
      Pads (18), Lead (15), Drone (12), Sequence (12), Bass (10),
      Pluck (10), Keys (8), FX (8), Percussion (3)

    Each preset is a complete parameter snapshot (except excluded tuning
    params, preserved across preset switches). Raw values are converted to
    normalized [0,1] at build time via APVTS NormalisableRange.

  ==============================================================================
*/

#include "FactoryPresets.h"

namespace
{
using Preset = OuariconPresetManager::FactoryPresetDef;
using RawMap = std::map<juce::String, float>;

// ─── Helpers ──────────────────────────────────────────────────────────

/** Merge `overrides` into `base`, overriding any duplicate keys. */
static RawMap merge (RawMap base, const RawMap& overrides)
{
    for (const auto& [k, v] : overrides)
        base[k] = v;
    return base;
}

/** Convert raw parameter values to APVTS-normalized [0,1]. */
static std::map<juce::String, float>
normalize (juce::AudioProcessorValueTreeState& apvts, const RawMap& raw)
{
    std::map<juce::String, float> out;
    for (const auto& [id, value] : raw)
    {
        if (auto* p = apvts.getParameter (id))
            out[id] = p->convertTo0to1 (value);
    }
    return out;
}

static Preset makePreset (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& category,
                          const juce::String& name,
                          const RawMap& raw)
{
    Preset def;
    def.category = category;
    def.name = name;
    def.parameters = normalize (apvts, raw);
    return def;
}

/** Set a mod matrix slot: src (ModSource enum int), dst (ModDest enum int), amt [-1..1]. */
static RawMap modSlot (int slot, int src, int dst, float amt)
{
    auto n = juce::String (slot);
    return {
        { "modSlot" + n + "Src", (float) src },
        { "modSlot" + n + "Dst", (float) dst },
        { "modSlot" + n + "Amt", amt },
        { "modSlot" + n + "On",  1.0f }
    };
}

// ─── Mod source / dest constants (matches ModulationMatrix.h enums) ───
enum : int {
    SRC_None=0, SRC_LFO1=1, SRC_LFO2=2, SRC_LFO3=3, SRC_LFO4=4,
    SRC_AmpEnv=5, SRC_FiltEnv=6, SRC_Vel=7, SRC_Note=8, SRC_Wheel=9, SRC_AT=10
};
enum : int {
    DST_None=0, DST_OscAPos=1, DST_OscBPos=2, DST_FiltACut=3, DST_FiltBCut=4,
    DST_FiltARes=5, DST_FiltBRes=6, DST_OscMix=7, DST_SubLevel=8, DST_NoiseLevel=9,
    DST_LFO1Rate=10, DST_LFO2Rate=11, DST_LFO3Rate=12, DST_LFO4Rate=13,
    DST_OscADetune=14, DST_OscBDetune=15, DST_OscAPan=16, DST_OscBPan=17,
    DST_ReverbMix=18, DST_DelayMix=19, DST_ChorusMix=20, DST_DistMix=21,
    DST_MasterVol=22, DST_Pitch=23, DST_OscAWarp=24, DST_OscBWarp=25
};

// ─── Wavetable indices (matches WavetableFactory table order) ─────────
enum : int {
    WT_Saw=0, WT_Square=1, WT_Triangle=2, WT_Sine=3,
    WT_PWMSweep=4, WT_Supersaw=5, WT_SyncSweep=6,
    WT_FMEPiano=7, WT_FMBell=8, WT_FMMetallic=9, WT_Wavefold=10, WT_Bitcrush=11,
    WT_VowelMorph=12, WT_ChoirPad=13, WT_VocalLead=14, WT_FormantFilter=15,
    WT_HarmonicSeries=16, WT_SpectralTilt=17, WT_OddHarmonics=18,
    WT_HarmonicStretch=19, WT_CombSweep=20, WT_PrismSpectrum=21,
    WT_Breath=22, WT_PluckedString=23, WT_ChurchBell=24, WT_OrganSweep=25,
    WT_Wind=26, WT_FilteredNoise=27
};

// ─── Neutral default for every non-excluded param ─────────────────────
// Used as the foundation for every preset so that loading a preset is
// fully deterministic regardless of the previous preset's state.
static RawMap completeBase()
{
    RawMap m;
    // Osc A (full layer on, basic saw)
    m["oscATable"] = WT_Saw; m["oscAPos"] = 0.0f; m["oscALevel"] = 0.8f;
    m["oscAPan"] = 0.0f; m["oscACoarse"] = 0; m["oscAFine"] = 0.0f;
    m["oscAPhase"] = 0.0f; m["oscAUnison"] = 1; m["oscADetune"] = 0.2f;
    m["oscAWidth"] = 0.5f; m["oscAWarpType"] = 0; m["oscAWarpAmt"] = 0.0f;
    // Osc B (off by default)
    m["oscBTable"] = WT_Saw; m["oscBPos"] = 0.0f; m["oscBLevel"] = 0.0f;
    m["oscBPan"] = 0.0f; m["oscBCoarse"] = 0; m["oscBFine"] = 0.0f;
    m["oscBPhase"] = 0.0f; m["oscBUnison"] = 1; m["oscBDetune"] = 0.2f;
    m["oscBWidth"] = 0.5f; m["oscBWarpType"] = 0; m["oscBWarpAmt"] = 0.0f;
    // Sub / Noise
    m["subShape"] = 0; m["subOctave"] = 0; m["subLevel"] = 0.0f;
    m["noiseType"] = 0; m["noiseLevel"] = 0.0f; m["subRouting"] = 0;
    // Amp envelope
    m["ampAttack"] = 0.01f; m["ampDecay"] = 0.3f;
    m["ampSustain"] = 0.7f; m["ampRelease"] = 0.5f;
    // Filter envelope (neutral: 0 depth so filters stay open)
    m["filtAttack"] = 0.01f; m["filtDecay"] = 0.5f;
    m["filtSustain"] = 0.5f; m["filtRelease"] = 0.5f;
    m["filtAEnvDepth"] = 0.0f; m["filtBEnvDepth"] = 0.0f;
    // Filters (wide open LP24)
    m["filtAType"] = 1; m["filtACutoff"] = 20000.0f;
    m["filtARes"] = 0.0f; m["filtADrive"] = 0.0f; m["filtAKeyTrack"] = 0.0f;
    m["filtBType"] = 1; m["filtBCutoff"] = 20000.0f;
    m["filtBRes"] = 0.0f; m["filtBDrive"] = 0.0f; m["filtBKeyTrack"] = 0.0f;
    m["filtRouting"] = 0;
    // FX all off / neutral
    m["reverbBypass"] = 0; m["reverbSize"] = 0.5f; m["reverbDamp"] = 0.5f;
    m["reverbPredelay"] = 20.0f; m["reverbMix"] = 0.0f;
    m["reverbModDepth"] = 0.3f; m["reverbModRate"] = 1.0f;
    m["delayBypass"] = 0; m["delayTime"] = 0.375f; m["delayFeedback"] = 0.3f;
    m["delaySync"] = 0; m["delayMode"] = 0; m["delayMix"] = 0.0f;
    m["chorusBypass"] = 0; m["chorusRate"] = 1.0f;
    m["chorusDepth"] = 0.5f; m["chorusMix"] = 0.0f;
    m["distBypass"] = 0; m["distType"] = 0; m["distDrive"] = 0.0f; m["distMix"] = 0.0f;
    m["eqBypass"] = 0; m["eqLowGain"] = 0.0f; m["eqMidGain"] = 0.0f;
    m["eqMidFreq"] = 1000.0f; m["eqHighGain"] = 0.0f;
    // 4 LFOs (all off / 1/4 sync / retrig)
    for (int i = 1; i <= 4; ++i)
    {
        auto n = juce::String (i);
        m["lfo" + n + "Rate"] = 1.0f;
        m["lfo" + n + "Shape"] = 0;
        m["lfo" + n + "Sync"] = 0;
        m["lfo" + n + "Division"] = 2;
        m["lfo" + n + "FreeRun"] = 0;
    }
    // 16 mod slots (all disabled)
    for (int i = 0; i < 16; ++i)
    {
        auto n = juce::String (i);
        m["modSlot" + n + "Src"] = 0;
        m["modSlot" + n + "Dst"] = 0;
        m["modSlot" + n + "Amt"] = 0.0f;
        m["modSlot" + n + "On"] = 0;
    }
    // Global
    m["masterVol"] = 0.75f; m["stereoWidth"] = 1.0f;
    m["oscMix"] = 0.5f; m["velocityCurve"] = 0;
    return m;
}

// ─── Category archetypes (common feel per category) ──────────────────

static RawMap padArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_Supersaw }, { "oscAPos", 0.3f }, { "oscAUnison", 5 },
        { "oscADetune", 0.35f }, { "oscAWidth", 0.8f },
        { "oscBLevel", 0.55f }, { "oscBTable", WT_PWMSweep }, { "oscBCoarse", -12 },
        { "oscBUnison", 3 }, { "oscBDetune", 0.25f }, { "oscBWidth", 0.6f },
        { "filtAType", 1 }, { "filtACutoff", 2200.0f }, { "filtARes", 0.15f },
        { "filtAKeyTrack", 0.35f },
        { "ampAttack", 1.5f }, { "ampDecay", 1.0f },
        { "ampSustain", 0.9f }, { "ampRelease", 3.5f },
        { "filtAttack", 2.5f }, { "filtDecay", 1.2f },
        { "filtSustain", 0.8f }, { "filtRelease", 3.0f },
        { "filtAEnvDepth", 0.3f },
        { "lfo1Rate", 0.3f }, { "lfo1Shape", 0 }, { "lfo1FreeRun", 1 },
        { "reverbMix", 0.5f }, { "reverbSize", 0.8f }, { "reverbDamp", 0.4f },
        { "delayMix", 0.15f }, { "delayTime", 0.55f }, { "delayFeedback", 0.35f },
        { "chorusMix", 0.22f }, { "chorusDepth", 0.6f },
        { "stereoWidth", 1.3f }
    });
    m = merge (m, modSlot (0, SRC_LFO1, DST_OscAPos,  0.12f));
    m = merge (m, modSlot (1, SRC_Vel,  DST_FiltACut, 0.3f));
    return m;
}

static RawMap droneArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_HarmonicSeries }, { "oscAPos", 0.4f },
        { "oscAUnison", 7 }, { "oscADetune", 0.5f }, { "oscAWidth", 1.0f },
        { "oscBLevel", 0.65f }, { "oscBTable", WT_PrismSpectrum },
        { "oscBCoarse", 7 }, { "oscBUnison", 5 },
        { "oscBDetune", 0.4f }, { "oscBWidth", 0.85f },
        { "subLevel", 0.15f }, { "subShape", 0 },
        { "noiseType", 1 }, { "noiseLevel", 0.05f },
        { "filtAType", 1 }, { "filtACutoff", 3000.0f }, { "filtARes", 0.1f },
        { "ampAttack", 4.0f }, { "ampDecay", 2.0f },
        { "ampSustain", 1.0f }, { "ampRelease", 6.0f },
        { "filtAttack", 5.0f }, { "filtRelease", 5.0f },
        { "filtAEnvDepth", 0.15f },
        { "lfo1Rate", 0.08f }, { "lfo1FreeRun", 1 },
        { "lfo2Rate", 0.15f }, { "lfo2Shape", 1 }, { "lfo2FreeRun", 1 },
        { "reverbMix", 0.6f }, { "reverbSize", 0.95f }, { "reverbPredelay", 40.0f },
        { "delayMix", 0.2f }, { "delayTime", 0.75f }, { "delayFeedback", 0.4f },
        { "stereoWidth", 1.5f }
    });
    m = merge (m, modSlot (0, SRC_LFO1, DST_OscAPos,  0.18f));
    m = merge (m, modSlot (1, SRC_LFO2, DST_OscBPos,  0.18f));
    m = merge (m, modSlot (2, SRC_LFO1, DST_FiltACut, 0.1f));
    return m;
}

static RawMap leadArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_Saw }, { "oscAPos", 0.0f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f }, { "oscAWidth", 0.55f },
        { "oscBLevel", 0.5f }, { "oscBTable", WT_Square },
        { "oscBCoarse", -12 }, { "oscBUnison", 1 },
        { "filtAType", 1 }, { "filtACutoff", 2500.0f }, { "filtARes", 0.35f },
        { "filtADrive", 0.2f }, { "filtAKeyTrack", 0.5f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.4f },
        { "ampSustain", 0.85f }, { "ampRelease", 0.3f },
        { "filtAttack", 0.005f }, { "filtDecay", 0.4f },
        { "filtSustain", 0.5f }, { "filtRelease", 0.3f },
        { "filtAEnvDepth", 0.6f },
        { "reverbMix", 0.2f }, { "reverbSize", 0.5f },
        { "delayMix", 0.18f }, { "delayTime", 0.375f }, { "delayFeedback", 0.3f },
        { "chorusMix", 0.12f },
        { "stereoWidth", 1.1f }
    });
    m = merge (m, modSlot (0, SRC_Vel,   DST_FiltACut, 0.4f));
    m = merge (m, modSlot (1, SRC_Wheel, DST_LFO1Rate, 0.3f));
    m = merge (m, modSlot (2, SRC_LFO1,  DST_Pitch,    0.04f));
    return m;
}

static RawMap bassArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_Saw }, { "oscAPos", 0.0f }, { "oscAUnison", 2 },
        { "oscADetune", 0.15f }, { "oscAWidth", 0.3f },
        { "oscBLevel", 0.4f }, { "oscBTable", WT_Square },
        { "oscBCoarse", -12 }, { "oscBUnison", 1 },
        { "subLevel", 0.55f }, { "subShape", 0 }, { "subOctave", 0 },
        { "filtAType", 1 }, { "filtACutoff", 800.0f }, { "filtARes", 0.3f },
        { "filtADrive", 0.25f }, { "filtAKeyTrack", 0.4f },
        { "ampAttack", 0.003f }, { "ampDecay", 0.5f },
        { "ampSustain", 0.75f }, { "ampRelease", 0.2f },
        { "filtAttack", 0.003f }, { "filtDecay", 0.35f },
        { "filtSustain", 0.3f }, { "filtRelease", 0.2f },
        { "filtAEnvDepth", 0.55f },
        { "reverbMix", 0.05f },
        { "delayMix", 0.0f },
        { "stereoWidth", 0.7f },
        { "masterVol", 0.7f }
    });
    m = merge (m, modSlot (0, SRC_Vel,   DST_FiltACut, 0.5f));
    m = merge (m, modSlot (1, SRC_Vel,   DST_FiltARes, 0.2f));
    return m;
}

static RawMap pluckArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.3f },
        { "oscAUnison", 2 }, { "oscADetune", 0.15f }, { "oscAWidth", 0.5f },
        { "oscBLevel", 0.35f }, { "oscBTable", WT_Triangle },
        { "oscBCoarse", 12 },
        { "filtAType", 1 }, { "filtACutoff", 3500.0f }, { "filtARes", 0.15f },
        { "filtAKeyTrack", 0.5f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.8f },
        { "ampSustain", 0.0f }, { "ampRelease", 0.8f },
        { "filtAttack", 0.002f }, { "filtDecay", 0.5f },
        { "filtSustain", 0.0f }, { "filtRelease", 0.4f },
        { "filtAEnvDepth", 0.5f },
        { "reverbMix", 0.35f }, { "reverbSize", 0.7f },
        { "delayMix", 0.2f }, { "delayTime", 0.25f }, { "delayFeedback", 0.35f },
        { "chorusMix", 0.15f },
        { "stereoWidth", 1.1f }
    });
    m = merge (m, modSlot (0, SRC_Vel,    DST_FiltACut, 0.6f));
    m = merge (m, modSlot (1, SRC_Vel,    DST_OscMix,   0.15f));
    return m;
}

static RawMap keysArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.3f }, { "oscAUnison", 1 },
        { "oscAWidth", 0.4f },
        { "oscBLevel", 0.3f }, { "oscBTable", WT_Sine }, { "oscBCoarse", 0 },
        { "filtAType", 1 }, { "filtACutoff", 4500.0f }, { "filtARes", 0.1f },
        { "filtAKeyTrack", 0.4f },
        { "ampAttack", 0.003f }, { "ampDecay", 2.5f },
        { "ampSustain", 0.3f }, { "ampRelease", 0.6f },
        { "filtAttack", 0.003f }, { "filtDecay", 1.5f },
        { "filtSustain", 0.3f }, { "filtRelease", 0.5f },
        { "filtAEnvDepth", 0.4f },
        { "reverbMix", 0.3f }, { "reverbSize", 0.6f },
        { "delayMix", 0.1f }, { "delayTime", 0.3f },
        { "chorusMix", 0.2f },
        { "stereoWidth", 1.15f }
    });
    m = merge (m, modSlot (0, SRC_Vel, DST_FiltACut, 0.55f));
    m = merge (m, modSlot (1, SRC_Vel, DST_OscMix,   0.15f));
    return m;
}

static RawMap seqArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_Square }, { "oscAPos", 0.3f }, { "oscAUnison", 2 },
        { "oscADetune", 0.15f },
        { "oscBLevel", 0.4f }, { "oscBTable", WT_Saw }, { "oscBCoarse", 0 },
        { "filtAType", 1 }, { "filtACutoff", 1800.0f }, { "filtARes", 0.35f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.25f },
        { "ampSustain", 0.5f }, { "ampRelease", 0.2f },
        { "filtAttack", 0.002f }, { "filtDecay", 0.25f },
        { "filtSustain", 0.2f }, { "filtAEnvDepth", 0.5f },
        { "lfo1Rate", 4.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "lfo2Rate", 2.0f }, { "lfo2Shape", 0 }, { "lfo2Sync", 1 }, { "lfo2Division", 3 },
        { "delaySync", 1 }, { "delayTime", 0.25f }, { "delayFeedback", 0.45f }, { "delayMix", 0.3f },
        { "reverbMix", 0.25f }, { "reverbSize", 0.55f },
        { "chorusMix", 0.15f },
        { "stereoWidth", 1.25f }
    });
    m = merge (m, modSlot (0, SRC_LFO1, DST_FiltACut, 0.6f));
    m = merge (m, modSlot (1, SRC_LFO2, DST_OscAPos,  0.25f));
    m = merge (m, modSlot (2, SRC_Vel,  DST_FiltARes, 0.2f));
    return m;
}

static RawMap fxArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.5f },
        { "oscAUnison", 5 }, { "oscADetune", 0.5f }, { "oscAWidth", 1.0f },
        { "oscBLevel", 0.5f }, { "oscBTable", WT_FilteredNoise },
        { "noiseType", 2 }, { "noiseLevel", 0.3f },
        { "filtAType", 1 }, { "filtACutoff", 2000.0f }, { "filtARes", 0.3f },
        { "ampAttack", 0.5f }, { "ampDecay", 1.0f },
        { "ampSustain", 0.6f }, { "ampRelease", 1.5f },
        { "filtAttack", 0.5f }, { "filtAEnvDepth", 0.5f },
        { "reverbMix", 0.6f }, { "reverbSize", 0.95f },
        { "delayMix", 0.35f }, { "delayFeedback", 0.55f },
        { "chorusMix", 0.35f },
        { "distMix", 0.1f }, { "distDrive", 0.3f },
        { "stereoWidth", 1.6f }
    });
    m = merge (m, modSlot (0, SRC_AmpEnv, DST_FiltACut, 0.7f));
    m = merge (m, modSlot (1, SRC_AmpEnv, DST_OscAPos,  0.5f));
    m = merge (m, modSlot (2, SRC_LFO1,   DST_Pitch,    0.1f));
    return m;
}

static RawMap percArchetype()
{
    auto m = merge (completeBase(), {
        { "oscATable", WT_Sine }, { "oscAPos", 0.0f },
        { "oscBLevel", 0.0f },
        { "subLevel", 0.8f }, { "subShape", 0 }, { "subOctave", 0 },
        { "noiseType", 0 }, { "noiseLevel", 0.3f },
        { "filtAType", 1 }, { "filtACutoff", 2500.0f }, { "filtARes", 0.2f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.25f },
        { "ampSustain", 0.0f }, { "ampRelease", 0.1f },
        { "filtAttack", 0.002f }, { "filtDecay", 0.2f },
        { "filtSustain", 0.0f }, { "filtAEnvDepth", 0.7f },
        { "reverbMix", 0.15f }, { "reverbSize", 0.4f },
        { "stereoWidth", 0.9f }
    });
    m = merge (m, modSlot (0, SRC_AmpEnv, DST_Pitch,    0.08f));
    m = merge (m, modSlot (1, SRC_Vel,    DST_FiltACut, 0.5f));
    return m;
}

// ═══════════════════════════════════════════════════════════════════
// Preset definitions — all 96
// ═══════════════════════════════════════════════════════════════════

static void addPads (juce::AudioProcessorValueTreeState& apvts,
                     std::vector<Preset>& out)
{
    const juce::String cat = "Pads";
    auto base = padArchetype();

    // 1. Refracted Glass — bright supersaw with prism shimmer
    out.push_back (makePreset (apvts, cat, "Refracted Glass", merge (base, {
        { "oscATable", WT_Supersaw }, { "oscAUnison", 7 }, { "oscADetune", 0.5f },
        { "oscBTable", WT_PrismSpectrum }, { "oscBLevel", 0.5f }, { "oscBUnison", 5 },
        { "filtACutoff", 3200.0f },
        { "reverbMix", 0.55f }, { "reverbSize", 0.85f }
    })));

    // 2. Prism Bloom — FM pad with slow position sweep
    out.push_back (makePreset (apvts, cat, "Prism Bloom", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.2f }, { "oscAUnison", 5 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.45f }, { "oscBCoarse", 12 },
        { "filtACutoff", 4200.0f }, { "filtARes", 0.2f },
        { "lfo1Rate", 0.18f }
    })));

    // 3. Cathedral Ice — slow attack, long reverb, choir
    out.push_back (makePreset (apvts, cat, "Cathedral Ice", merge (base, {
        { "oscATable", WT_ChoirPad }, { "oscAPos", 0.4f }, { "oscAUnison", 6 },
        { "oscBTable", WT_VowelMorph }, { "oscBLevel", 0.55f }, { "oscBPos", 0.3f },
        { "ampAttack", 3.5f }, { "ampRelease", 6.0f },
        { "filtACutoff", 2500.0f },
        { "reverbMix", 0.7f }, { "reverbSize", 0.95f }, { "reverbPredelay", 60.0f }
    })));

    // 4. Velvet Morning — warm analog
    out.push_back (makePreset (apvts, cat, "Velvet Morning", merge (base, {
        { "oscATable", WT_PWMSweep }, { "oscAPos", 0.4f },
        { "oscBTable", WT_Supersaw }, { "oscBLevel", 0.6f },
        { "filtACutoff", 1400.0f }, { "filtARes", 0.25f },
        { "reverbMix", 0.42f }, { "eqLowGain", 2.0f }
    })));

    // 5. Nebula Drift — evolving spectral wash
    out.push_back (makePreset (apvts, cat, "Nebula Drift", merge (base, {
        { "oscATable", WT_SpectralTilt }, { "oscAPos", 0.5f }, { "oscAUnison", 6 },
        { "oscBTable", WT_HarmonicStretch }, { "oscBLevel", 0.6f }, { "oscBPos", 0.3f },
        { "filtACutoff", 3500.0f },
        { "lfo1Rate", 0.12f }, { "modSlot0Amt", 0.22f },
        { "reverbMix", 0.6f }, { "delayMix", 0.2f }
    })));

    // 6. Silk Road — exotic detuned wide
    out.push_back (makePreset (apvts, cat, "Silk Road", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscAPos", 0.35f }, { "oscAUnison", 5 },
        { "oscBTable", WT_OddHarmonics }, { "oscBLevel", 0.55f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2800.0f },
        { "chorusMix", 0.35f }, { "stereoWidth", 1.5f }
    })));

    // 7. Crystal Cavern — bell-like harmonic
    out.push_back (makePreset (apvts, cat, "Crystal Cavern", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.3f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.5f }, { "oscBCoarse", 19 },
        { "filtACutoff", 5500.0f }, { "filtARes", 0.1f },
        { "ampAttack", 2.2f }, { "ampRelease", 5.0f },
        { "reverbMix", 0.65f }, { "reverbSize", 0.92f }
    })));

    // 8. Aurora — slow position sweep pad
    out.push_back (makePreset (apvts, cat, "Aurora", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.1f }, { "oscAUnison", 6 },
        { "oscBTable", WT_HarmonicSeries }, { "oscBLevel", 0.55f },
        { "filtACutoff", 3000.0f },
        { "lfo1Rate", 0.07f }, { "modSlot0Amt", 0.35f },
        { "reverbMix", 0.55f }
    })));

    // 9. Sacred Space — choir pad
    out.push_back (makePreset (apvts, cat, "Sacred Space", merge (base, {
        { "oscATable", WT_ChoirPad }, { "oscAPos", 0.5f }, { "oscAUnison", 6 },
        { "oscBTable", WT_VowelMorph }, { "oscBLevel", 0.6f },
        { "filtACutoff", 2000.0f },
        { "ampAttack", 2.5f }, { "ampRelease", 5.5f },
        { "reverbMix", 0.68f }, { "reverbSize", 0.93f }, { "reverbPredelay", 80.0f }
    })));

    // 10. Twilight Shimmer — bright airy
    out.push_back (makePreset (apvts, cat, "Twilight Shimmer", merge (base, {
        { "oscATable", WT_Sine }, { "oscAPos", 0.0f }, { "oscAUnison", 3 },
        { "oscBTable", WT_PrismSpectrum }, { "oscBLevel", 0.5f }, { "oscBCoarse", 12 },
        { "filtAType", 2 }, { "filtACutoff", 180.0f }, { "filtARes", 0.2f },
        { "reverbMix", 0.55f }, { "eqHighGain", 3.0f }
    })));

    // 11. Underground River — dark flowing
    out.push_back (makePreset (apvts, cat, "Underground River", merge (base, {
        { "oscATable", WT_PWMSweep }, { "oscAPos", 0.7f },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.55f }, { "oscBCoarse", -12 },
        { "filtACutoff", 900.0f }, { "filtARes", 0.35f }, { "filtADrive", 0.3f },
        { "subLevel", 0.3f },
        { "reverbMix", 0.5f }, { "eqLowGain", 3.0f }
    })));

    // 12. Moth Garden — fragile detuned
    out.push_back (makePreset (apvts, cat, "Moth Garden", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.25f }, { "oscAUnison", 4 },
        { "oscADetune", 0.45f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f },
        { "filtACutoff", 2400.0f },
        { "chorusMix", 0.4f }, { "reverbMix", 0.45f }
    })));

    // 13. Signal Fade — noise-layered ambient
    out.push_back (makePreset (apvts, cat, "Signal Fade", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscAPos", 0.4f }, { "oscAUnison", 4 },
        { "oscBTable", WT_Breath }, { "oscBLevel", 0.6f },
        { "noiseType", 1 }, { "noiseLevel", 0.12f },
        { "filtACutoff", 3500.0f },
        { "reverbMix", 0.58f }, { "delayMix", 0.22f }
    })));

    // 14. Marble Hall — stately reverb-heavy
    out.push_back (makePreset (apvts, cat, "Marble Hall", merge (base, {
        { "oscATable", WT_OrganSweep }, { "oscAPos", 0.3f },
        { "oscBTable", WT_Supersaw }, { "oscBLevel", 0.45f }, { "oscBCoarse", -12 },
        { "filtACutoff", 2000.0f },
        { "ampAttack", 1.8f }, { "ampRelease", 4.5f },
        { "reverbMix", 0.72f }, { "reverbSize", 0.96f }, { "reverbPredelay", 100.0f }
    })));

    // 15. Ember Pulse — warm breathing
    out.push_back (makePreset (apvts, cat, "Ember Pulse", merge (base, {
        { "oscATable", WT_PWMSweep }, { "oscAPos", 0.5f }, { "oscAUnison", 5 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.5f }, { "oscBCoarse", -12 },
        { "filtACutoff", 1600.0f }, { "filtADrive", 0.2f },
        { "lfo1Rate", 0.5f }, { "lfo1Shape", 0 }, { "lfo1FreeRun", 1 },
        { "modSlot0Dst", DST_MasterVol }, { "modSlot0Amt", 0.12f }
    })));

    // 16. Paper Sky — thin whispery
    out.push_back (makePreset (apvts, cat, "Paper Sky", merge (base, {
        { "oscATable", WT_Breath }, { "oscAPos", 0.3f }, { "oscAUnison", 4 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.35f }, { "oscBCoarse", 12 },
        { "noiseType", 1 }, { "noiseLevel", 0.08f },
        { "filtACutoff", 4500.0f },
        { "ampAttack", 2.8f }, { "reverbMix", 0.58f }, { "eqHighGain", 2.5f }
    })));

    // 17. Monument Moss — dense organic
    out.push_back (makePreset (apvts, cat, "Monument Moss", merge (base, {
        { "oscATable", WT_OrganSweep }, { "oscAPos", 0.4f }, { "oscAUnison", 5 },
        { "oscBTable", WT_HarmonicSeries }, { "oscBLevel", 0.6f }, { "oscBCoarse", 7 },
        { "filtACutoff", 1800.0f }, { "filtARes", 0.2f },
        { "reverbMix", 0.55f }, { "chorusMix", 0.3f }
    })));

    // 18. Constellation — wide twinkling
    out.push_back (makePreset (apvts, cat, "Constellation", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.3f }, { "oscAUnison", 5 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", 19 },
        { "filtACutoff", 5500.0f },
        { "lfo1Rate", 0.4f },
        { "modSlot0Dst", DST_OscBPan }, { "modSlot0Amt", 0.5f },
        { "reverbMix", 0.62f }, { "delayMix", 0.2f }, { "stereoWidth", 1.5f }
    })));

    // 19. Glass Curtain — formant-shaped vocal pad
    out.push_back (makePreset (apvts, cat, "Glass Curtain", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.4f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f }, { "oscAWidth", 0.85f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.5f }, { "oscBCoarse", 7 },
        { "oscBUnison", 3 },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.2f },
        { "ampAttack", 1.8f }, { "ampRelease", 4.0f },
        { "lfo1Rate", 0.18f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.2f },
        { "reverbMix", 0.55f }, { "reverbSize", 0.9f },
        { "chorusMix", 0.3f }
    })));

    // 20. Stratosphere — spectral-tilt high register
    out.push_back (makePreset (apvts, cat, "Stratosphere", merge (base, {
        { "oscATable", WT_SpectralTilt }, { "oscAPos", 0.65f }, { "oscAUnison", 5 },
        { "oscADetune", 0.4f }, { "oscAWidth", 1.0f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.45f }, { "oscBCoarse", 12 },
        { "filtACutoff", 5800.0f }, { "filtARes", 0.05f },
        { "ampAttack", 3.5f }, { "ampRelease", 5.5f },
        { "lfo1Rate", 0.12f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.18f },
        { "reverbMix", 0.65f }, { "reverbSize", 0.95f },
        { "delayMix", 0.22f }, { "delayTime", 0.625f },
        { "stereoWidth", 1.55f }
    })));

    // 21. Bone Cathedral — bell-pad hybrid
    out.push_back (makePreset (apvts, cat, "Bone Cathedral", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.25f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.6f }, { "oscBCoarse", 7 },
        { "oscBUnison", 5 }, { "oscBDetune", 0.3f },
        { "filtACutoff", 2800.0f }, { "filtARes", 0.1f },
        { "ampAttack", 2.2f }, { "ampRelease", 5.0f },
        { "reverbMix", 0.7f }, { "reverbSize", 0.95f }, { "reverbPredelay", 50.0f },
        { "delayMix", 0.18f }, { "delayTime", 0.75f },
        { "chorusMix", 0.18f },
        { "stereoWidth", 1.45f }
    })));

    // 22. Lichen Drift — slow harmonic stretch
    out.push_back (makePreset (apvts, cat, "Lichen Drift", merge (base, {
        { "oscATable", WT_HarmonicStretch }, { "oscAPos", 0.3f }, { "oscAUnison", 5 },
        { "oscADetune", 0.45f }, { "oscAWidth", 0.95f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.45f }, { "oscBCoarse", -12 },
        { "filtACutoff", 1900.0f }, { "filtARes", 0.18f },
        { "ampAttack", 2.5f }, { "ampRelease", 4.5f },
        { "lfo1Rate", 0.1f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.3f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_FiltACut },
        { "modSlot1Amt", 0.2f }, { "modSlot1On", 1.0f },
        { "reverbMix", 0.55f }, { "delayMix", 0.18f },
        { "stereoWidth", 1.4f }
    })));

    // 23. Hollow Star — organ sweep with breath
    out.push_back (makePreset (apvts, cat, "Hollow Star", merge (base, {
        { "oscATable", WT_OrganSweep }, { "oscAPos", 0.35f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f }, { "oscAWidth", 0.85f },
        { "oscBTable", WT_Breath }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "oscBUnison", 3 }, { "oscBDetune", 0.25f },
        { "filtACutoff", 2600.0f }, { "filtARes", 0.12f },
        { "ampAttack", 2.0f }, { "ampRelease", 4.0f },
        { "lfo1Rate", 0.22f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.18f },
        { "reverbMix", 0.6f }, { "reverbSize", 0.88f },
        { "chorusMix", 0.25f },
        { "stereoWidth", 1.5f }
    })));

    // 24. Salt Air — airy vowel pad with wind
    out.push_back (makePreset (apvts, cat, "Salt Air", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscAPos", 0.45f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f }, { "oscAWidth", 0.85f },
        { "oscBTable", WT_Wind }, { "oscBLevel", 0.4f },
        { "oscBUnison", 3 }, { "oscBDetune", 0.4f },
        { "noiseType", 1 }, { "noiseLevel", 0.08f },
        { "filtACutoff", 2200.0f },
        { "ampAttack", 1.8f }, { "ampRelease", 3.5f },
        { "lfo1Rate", 0.15f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.22f },
        { "reverbMix", 0.5f }, { "reverbSize", 0.85f },
        { "stereoWidth", 1.45f }
    })));

    // 25. Lithium Glow — glowing vocal pad
    out.push_back (makePreset (apvts, cat, "Lithium Glow", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscAPos", 0.55f }, { "oscAUnison", 5 },
        { "oscADetune", 0.4f }, { "oscAWidth", 0.95f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.55f }, { "oscBCoarse", 0 },
        { "oscBUnison", 5 }, { "oscBDetune", 0.3f },
        { "filtACutoff", 3400.0f }, { "filtARes", 0.1f },
        { "ampAttack", 2.0f }, { "ampRelease", 4.5f },
        { "lfo1Rate", 0.2f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.25f },
        { "reverbMix", 0.55f }, { "reverbSize", 0.9f },
        { "chorusMix", 0.32f }, { "chorusDepth", 0.7f },
        { "stereoWidth", 1.5f }
    })));

    // 26. Snowmelt — soft breath pad
    out.push_back (makePreset (apvts, cat, "Snowmelt", merge (base, {
        { "oscATable", WT_Breath }, { "oscAPos", 0.3f }, { "oscAUnison", 5 },
        { "oscADetune", 0.5f }, { "oscAWidth", 1.0f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.5f }, { "oscBCoarse", 5 },
        { "oscBUnison", 3 }, { "oscBDetune", 0.25f },
        { "filtACutoff", 2000.0f }, { "filtARes", 0.08f },
        { "ampAttack", 2.8f }, { "ampRelease", 5.0f },
        { "lfo1Rate", 0.1f },
        { "modSlot0Dst", DST_FiltACut }, { "modSlot0Amt", 0.15f },
        { "reverbMix", 0.62f }, { "reverbSize", 0.92f },
        { "delayMix", 0.15f },
        { "stereoWidth", 1.55f }
    })));
}

static void addDrones (juce::AudioProcessorValueTreeState& apvts,
                       std::vector<Preset>& out)
{
    const juce::String cat = "Drone";
    auto base = droneArchetype();

    out.push_back (makePreset (apvts, cat, "Deep Source", merge (base, {
        { "oscATable", WT_HarmonicSeries }, { "oscBTable", WT_OddHarmonics },
        { "filtACutoff", 2400.0f }, { "subLevel", 0.25f },
        { "reverbMix", 0.55f }
    })));

    out.push_back (makePreset (apvts, cat, "Standing Wave", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscBTable", WT_HarmonicStretch },
        { "lfo1Rate", 0.05f }, { "modSlot0Amt", 0.25f },
        { "reverbMix", 0.6f }
    })));

    out.push_back (makePreset (apvts, cat, "Iron Sustain", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.5f },
        { "oscBTable", WT_FMMetallic }, { "oscBCoarse", 7 },
        { "filtACutoff", 3500.0f }, { "filtADrive", 0.15f },
        { "reverbMix", 0.65f }, { "delayFeedback", 0.5f }
    })));

    out.push_back (makePreset (apvts, cat, "Cosmic Hum", merge (base, {
        { "oscATable", WT_Sine }, { "oscAUnison", 6 }, { "oscADetune", 0.6f },
        { "oscBTable", WT_HarmonicSeries }, { "oscBCoarse", 0 },
        { "noiseType", 2 }, { "noiseLevel", 0.08f },
        { "filtACutoff", 2800.0f },
        { "reverbMix", 0.72f }
    })));

    out.push_back (makePreset (apvts, cat, "Memory Fog", merge (base, {
        { "oscATable", WT_Breath }, { "oscBTable", WT_ChoirPad },
        { "filtACutoff", 1800.0f },
        { "lfo1Rate", 0.04f }, { "lfo2Rate", 0.07f },
        { "reverbMix", 0.75f }, { "reverbSize", 0.98f }
    })));

    out.push_back (makePreset (apvts, cat, "Tectonic", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscACoarse", -12 },
        { "oscBTable", WT_Supersaw }, { "oscBCoarse", -24 },
        { "subLevel", 0.4f }, { "filtACutoff", 1200.0f },
        { "reverbMix", 0.55f }, { "eqLowGain", 4.0f }
    })));

    out.push_back (makePreset (apvts, cat, "Prayer Wheel", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscBTable", WT_OrganSweep },
        { "filtACutoff", 2400.0f },
        { "lfo1Rate", 0.12f }, { "modSlot0Amt", 0.2f },
        { "reverbMix", 0.65f }
    })));

    out.push_back (makePreset (apvts, cat, "Low Pressure", merge (base, {
        { "oscATable", WT_Wind }, { "oscBTable", WT_FilteredNoise },
        { "noiseType", 5 }, { "noiseLevel", 0.15f },
        { "filtACutoff", 2000.0f },
        { "reverbMix", 0.6f }
    })));

    out.push_back (makePreset (apvts, cat, "Ancient Bell Field", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscBTable", WT_FMBell },
        { "oscBCoarse", 5 },
        { "filtACutoff", 4000.0f },
        { "ampAttack", 6.0f }, { "ampRelease", 8.0f },
        { "reverbMix", 0.75f }, { "reverbSize", 0.98f }, { "delayFeedback", 0.6f }
    })));

    out.push_back (makePreset (apvts, cat, "Whispered Secret", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscBTable", WT_Breath },
        { "noiseType", 1 }, { "noiseLevel", 0.12f },
        { "filtACutoff", 3000.0f },
        { "lfo1Rate", 0.09f },
        { "reverbMix", 0.7f }, { "chorusMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Mountain Wind", merge (base, {
        { "oscATable", WT_Wind }, { "oscBTable", WT_Breath },
        { "noiseType", 5 }, { "noiseLevel", 0.2f },
        { "filtACutoff", 2200.0f }, { "filtAType", 4 },
        { "reverbMix", 0.62f }, { "stereoWidth", 1.55f }
    })));

    out.push_back (makePreset (apvts, cat, "Dormant Forge", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscBTable", WT_Wavefold },
        { "oscACoarse", -12 }, { "oscBCoarse", -7 },
        { "filtACutoff", 1500.0f }, { "filtADrive", 0.3f },
        { "subLevel", 0.25f },
        { "reverbMix", 0.55f }, { "distMix", 0.1f }, { "distDrive", 0.2f }
    })));

    // 13. Continental Plate — sub-anchored harmonic series
    out.push_back (makePreset (apvts, cat, "Continental Plate", merge (base, {
        { "oscATable", WT_HarmonicSeries }, { "oscAPos", 0.25f },
        { "oscAUnison", 7 }, { "oscADetune", 0.45f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.5f }, { "oscBCoarse", -12 },
        { "subLevel", 0.4f }, { "subShape", 0 }, { "subOctave", -1 },
        { "filtACutoff", 1800.0f },
        { "ampAttack", 5.0f }, { "ampRelease", 7.0f },
        { "reverbMix", 0.65f }, { "reverbSize", 0.95f },
        { "stereoWidth", 1.4f }
    })));

    // 14. Glacier Breath — spectral tilt with wind layer
    out.push_back (makePreset (apvts, cat, "Glacier Breath", merge (base, {
        { "oscATable", WT_SpectralTilt }, { "oscAPos", 0.5f },
        { "oscAUnison", 7 }, { "oscADetune", 0.5f },
        { "oscBTable", WT_Wind }, { "oscBLevel", 0.5f }, { "oscBCoarse", 7 },
        { "noiseType", 1 }, { "noiseLevel", 0.1f },
        { "filtACutoff", 2400.0f },
        { "ampAttack", 4.5f }, { "ampRelease", 6.5f },
        { "lfo1Rate", 0.07f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.25f },
        { "reverbMix", 0.62f }, { "reverbSize", 0.92f },
        { "stereoWidth", 1.55f }
    })));

    // 15. Stone Choir — organ + choir foundation
    out.push_back (makePreset (apvts, cat, "Stone Choir", merge (base, {
        { "oscATable", WT_OrganSweep }, { "oscAPos", 0.4f },
        { "oscAUnison", 5 }, { "oscADetune", 0.35f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.7f }, { "oscBCoarse", 0 },
        { "oscBUnison", 5 }, { "oscBDetune", 0.3f },
        { "filtACutoff", 2900.0f }, { "filtARes", 0.1f },
        { "ampAttack", 4.0f }, { "ampRelease", 6.0f },
        { "reverbMix", 0.7f }, { "reverbSize", 0.96f },
        { "delayMix", 0.18f },
        { "stereoWidth", 1.5f }
    })));

    // 16. Salt Ocean — wide harmonic stretch
    out.push_back (makePreset (apvts, cat, "Salt Ocean", merge (base, {
        { "oscATable", WT_HarmonicStretch }, { "oscAPos", 0.4f },
        { "oscAUnison", 7 }, { "oscADetune", 0.55f }, { "oscAWidth", 1.0f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f }, { "oscBCoarse", -7 },
        { "subLevel", 0.2f },
        { "filtACutoff", 2200.0f },
        { "ampAttack", 5.0f }, { "ampRelease", 7.0f },
        { "lfo1Rate", 0.06f }, { "lfo2Rate", 0.09f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.3f },
        { "reverbMix", 0.6f }, { "delayMix", 0.25f }, { "delayTime", 0.85f },
        { "stereoWidth", 1.55f }
    })));

    // 17. Bell Forest — sustained ChurchBell field
    out.push_back (makePreset (apvts, cat, "Bell Forest", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.3f },
        { "oscAUnison", 5 }, { "oscADetune", 0.4f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.55f }, { "oscBCoarse", 7 },
        { "oscBUnison", 3 },
        { "filtACutoff", 4000.0f }, { "filtARes", 0.05f },
        { "ampAttack", 3.5f }, { "ampRelease", 8.0f },
        { "reverbMix", 0.78f }, { "reverbSize", 0.98f }, { "reverbPredelay", 60.0f },
        { "delayMix", 0.3f }, { "delayFeedback", 0.55f },
        { "stereoWidth", 1.5f }
    })));

    // 18. Iron Hum — odd harmonics with sub
    out.push_back (makePreset (apvts, cat, "Iron Hum", merge (base, {
        { "oscATable", WT_OddHarmonics }, { "oscAPos", 0.3f },
        { "oscAUnison", 5 }, { "oscADetune", 0.4f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.45f }, { "subShape", 1 },
        { "filtACutoff", 1700.0f }, { "filtADrive", 0.2f },
        { "ampAttack", 4.5f }, { "ampRelease", 6.5f },
        { "lfo1Rate", 0.07f },
        { "modSlot0Dst", DST_FiltACut }, { "modSlot0Amt", 0.2f },
        { "reverbMix", 0.55f }, { "distMix", 0.1f }, { "distDrive", 0.25f },
        { "stereoWidth", 1.3f }
    })));

    // 19. Cathedral Furnace — deep prism roar
    out.push_back (makePreset (apvts, cat, "Cathedral Furnace", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.55f },
        { "oscAUnison", 7 }, { "oscADetune", 0.5f },
        { "oscBTable", WT_HarmonicStretch }, { "oscBLevel", 0.6f }, { "oscBCoarse", -7 },
        { "subLevel", 0.3f },
        { "filtACutoff", 2600.0f }, { "filtADrive", 0.3f },
        { "ampAttack", 5.0f }, { "ampRelease", 7.5f },
        { "lfo1Rate", 0.08f }, { "lfo2Rate", 0.11f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.25f },
        { "reverbMix", 0.72f }, { "reverbSize", 0.97f },
        { "distMix", 0.15f }, { "distDrive", 0.3f },
        { "stereoWidth", 1.5f }
    })));

    // 20. Coral Tide — slow filtered noise wash
    out.push_back (makePreset (apvts, cat, "Coral Tide", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscAPos", 0.5f },
        { "oscAUnison", 5 }, { "oscADetune", 0.5f },
        { "oscBTable", WT_HarmonicSeries }, { "oscBLevel", 0.5f }, { "oscBCoarse", 0 },
        { "noiseType", 1 }, { "noiseLevel", 0.15f },
        { "filtACutoff", 1900.0f }, { "filtAType", 1 },
        { "ampAttack", 5.5f }, { "ampRelease", 7.0f },
        { "lfo1Rate", 0.05f },
        { "modSlot0Dst", DST_FiltACut }, { "modSlot0Amt", 0.3f },
        { "reverbMix", 0.65f }, { "reverbSize", 0.95f },
        { "stereoWidth", 1.6f }
    })));

    // 21. Volcanic Throat — driven harmonic stretch
    out.push_back (makePreset (apvts, cat, "Volcanic Throat", merge (base, {
        { "oscATable", WT_HarmonicStretch }, { "oscAPos", 0.6f },
        { "oscAUnison", 5 }, { "oscADetune", 0.45f },
        { "oscBTable", WT_OddHarmonics }, { "oscBLevel", 0.55f }, { "oscBCoarse", -12 },
        { "subLevel", 0.35f },
        { "filtACutoff", 2200.0f }, { "filtADrive", 0.4f },
        { "ampAttack", 4.0f }, { "ampRelease", 6.0f },
        { "lfo1Rate", 0.06f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.2f },
        { "reverbMix", 0.6f }, { "distMix", 0.18f }, { "distDrive", 0.4f },
        { "stereoWidth", 1.4f }
    })));

    // 22. Fossil Bell — ancient bell shimmer
    out.push_back (makePreset (apvts, cat, "Fossil Bell", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.45f },
        { "oscAUnison", 3 }, { "oscADetune", 0.3f },
        { "oscBTable", WT_SpectralTilt }, { "oscBLevel", 0.45f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3800.0f }, { "filtARes", 0.05f },
        { "ampAttack", 5.0f }, { "ampRelease", 8.0f },
        { "lfo1Rate", 0.09f },
        { "modSlot0Dst", DST_OscBPos }, { "modSlot0Amt", 0.25f },
        { "reverbMix", 0.78f }, { "reverbSize", 0.98f }, { "reverbPredelay", 80.0f },
        { "delayMix", 0.35f }, { "delayFeedback", 0.6f }, { "delayTime", 1.0f },
        { "stereoWidth", 1.55f }
    })));
}

static void addLeads (juce::AudioProcessorValueTreeState& apvts,
                      std::vector<Preset>& out)
{
    const juce::String cat = "Lead";
    auto base = leadArchetype();

    out.push_back (makePreset (apvts, cat, "Screaming Comet", merge (base, {
        { "oscATable", WT_Supersaw }, { "oscAUnison", 5 }, { "oscADetune", 0.4f },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.45f }, { "filtADrive", 0.35f },
        { "distMix", 0.15f }, { "distDrive", 0.4f },
        { "reverbMix", 0.18f }, { "delayMix", 0.22f }
    })));

    out.push_back (makePreset (apvts, cat, "Liquid Fire", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.5f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.5f },
        { "filtACutoff", 2800.0f }, { "filtARes", 0.5f },
        { "lfo1Rate", 5.0f }, { "modSlot0Src", SRC_LFO1 },
        { "modSlot0Dst", DST_FiltACut }, { "modSlot0Amt", 0.25f }, { "modSlot0On", 1 }
    })));

    out.push_back (makePreset (apvts, cat, "Solar Wind", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.4f }, { "oscAUnison", 4 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f },
        { "filtACutoff", 4200.0f },
        { "reverbMix", 0.3f }, { "chorusMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Neon Glass", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.2f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.45f },
        { "filtACutoff", 5000.0f }, { "filtARes", 0.3f },
        { "delayMix", 0.25f }, { "reverbMix", 0.22f }
    })));

    out.push_back (makePreset (apvts, cat, "Hornet", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.7f }, { "oscAUnison", 3 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.55f }, { "oscBCoarse", -12 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.55f }, { "filtADrive", 0.4f },
        { "distMix", 0.2f }, { "distDrive", 0.5f }, { "distType", 2 }
    })));

    out.push_back (makePreset (apvts, cat, "Spectral Cry", merge (base, {
        { "oscATable", WT_VocalLead }, { "oscAPos", 0.4f },
        { "oscBTable", WT_FormantFilter }, { "oscBLevel", 0.55f },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.25f },
        { "reverbMix", 0.28f }, { "delayMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Warm Honey", merge (base, {
        { "oscATable", WT_Triangle }, { "oscAUnison", 2 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.5f },
        { "filtACutoff", 1800.0f }, { "filtARes", 0.2f },
        { "eqLowGain", 2.0f }, { "eqMidGain", 1.5f }, { "eqMidFreq", 800.0f },
        { "reverbMix", 0.22f }
    })));

    out.push_back (makePreset (apvts, cat, "Plasma Line", merge (base, {
        { "oscATable", WT_SyncSweep }, { "oscAPos", 0.6f }, { "oscAWarpType", 1 }, { "oscAWarpAmt", 0.5f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.45f },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.4f },
        { "distMix", 0.1f }, { "delayMix", 0.25f }
    })));

    out.push_back (makePreset (apvts, cat, "Echo Mountain", merge (base, {
        { "oscATable", WT_Saw }, { "oscAUnison", 4 }, { "oscADetune", 0.3f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.35f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3000.0f },
        { "delaySync", 1 }, { "delayDivision", 2 }, { "delayFeedback", 0.5f }, { "delayMix", 0.35f },
        { "reverbMix", 0.3f }
    })));

    out.push_back (makePreset (apvts, cat, "Bright Saw", merge (base, {
        { "oscATable", WT_Saw }, { "oscAUnison", 3 }, { "oscADetune", 0.22f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.25f },
        { "chorusMix", 0.2f }, { "reverbMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Mellow Ember", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.25f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.45f },
        { "filtACutoff", 2000.0f }, { "filtARes", 0.15f },
        { "ampAttack", 0.05f }, { "reverbMix", 0.28f }
    })));

    out.push_back (makePreset (apvts, cat, "Sun Pierce", merge (base, {
        { "oscATable", WT_HarmonicSeries }, { "oscAPos", 0.6f }, { "oscAUnison", 3 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.5f },
        { "filtACutoff", 4500.0f }, { "filtARes", 0.3f },
        { "distMix", 0.1f }, { "reverbMix", 0.22f }, { "delayMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Razor Wire", merge (base, {
        { "oscATable", WT_SyncSweep }, { "oscAPos", 0.75f }, { "oscAUnison", 5 }, { "oscADetune", 0.35f },
        { "oscBTable", WT_Bitcrush }, { "oscBLevel", 0.4f }, { "oscBPos", 0.5f },
        { "filtACutoff", 3800.0f }, { "filtARes", 0.5f }, { "filtADrive", 0.35f },
        { "distMix", 0.2f }, { "distDrive", 0.45f }, { "distType", 1 },
        { "delayMix", 0.2f }, { "reverbMix", 0.15f }
    })));

    out.push_back (makePreset (apvts, cat, "Phantom Vocal", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscAPos", 0.3f }, { "oscAUnison", 3 },
        { "oscBTable", WT_FormantFilter }, { "oscBLevel", 0.5f }, { "oscBPos", 0.6f },
        { "filtACutoff", 2600.0f }, { "filtARes", 0.3f },
        { "lfo1Rate", 3.5f }, { "modSlot0Src", SRC_LFO1 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.35f }, { "modSlot0On", 1 },
        { "chorusMix", 0.22f }, { "reverbMix", 0.3f }
    })));

    out.push_back (makePreset (apvts, cat, "Acid Bite", merge (base, {
        { "oscATable", WT_Saw }, { "oscAUnison", 2 }, { "oscADetune", 0.18f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.45f }, { "oscBCoarse", 0 },
        { "filtACutoff", 1200.0f }, { "filtARes", 0.65f }, { "filtADrive", 0.4f },
        { "filtAEnvDepth", 0.8f }, { "filtDecay", 0.2f }, { "filtSustain", 0.1f },
        { "distMix", 0.15f }, { "distDrive", 0.35f },
        { "delayMix", 0.22f }, { "reverbMix", 0.12f }
    })));

    // 16. Glass Throat — VocalLead vocal lead
    out.push_back (makePreset (apvts, cat, "Glass Throat", merge (base, {
        { "oscATable", WT_VocalLead }, { "oscAPos", 0.45f }, { "oscAUnison", 3 },
        { "oscADetune", 0.2f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.3f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.25f },
        { "ampAttack", 0.01f }, { "ampDecay", 0.5f }, { "ampSustain", 0.85f },
        { "lfo1Rate", 5.0f },
        { "modSlot2Src", SRC_LFO1 }, { "modSlot2Dst", DST_Pitch }, { "modSlot2Amt", 0.03f }, { "modSlot2On", 1.0f },
        { "reverbMix", 0.3f }, { "delayMix", 0.22f }, { "chorusMix", 0.18f },
        { "stereoWidth", 1.2f }
    })));

    // 17. Phantom Choir — VocalLead with reverb halo
    out.push_back (makePreset (apvts, cat, "Phantom Choir", merge (base, {
        { "oscATable", WT_VocalLead }, { "oscAPos", 0.55f }, { "oscAUnison", 5 },
        { "oscADetune", 0.35f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.45f }, { "oscBCoarse", 0 },
        { "filtACutoff", 3000.0f },
        { "ampAttack", 0.05f }, { "ampDecay", 0.6f }, { "ampSustain", 0.9f }, { "ampRelease", 0.6f },
        { "reverbMix", 0.5f }, { "reverbSize", 0.85f },
        { "delayMix", 0.25f }, { "delayTime", 0.5f },
        { "chorusMix", 0.25f },
        { "stereoWidth", 1.35f }
    })));

    // 18. Ember Tongue — FMMetallic blade
    out.push_back (makePreset (apvts, cat, "Ember Tongue", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.4f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 2800.0f }, { "filtARes", 0.4f }, { "filtADrive", 0.3f },
        { "filtAEnvDepth", 0.55f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.5f },
        { "modSlot2Src", SRC_LFO1 }, { "modSlot2Dst", DST_OscAPos }, { "modSlot2Amt", 0.18f }, { "modSlot2On", 1.0f },
        { "lfo1Rate", 6.0f },
        { "reverbMix", 0.25f }, { "delayMix", 0.2f }, { "distMix", 0.1f }, { "distDrive", 0.25f }
    })));

    // 19. Static Halo — wavefold + harmonic series
    out.push_back (makePreset (apvts, cat, "Static Halo", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_HarmonicSeries }, { "oscBLevel", 0.45f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.45f }, { "filtADrive", 0.35f },
        { "ampAttack", 0.01f }, { "ampDecay", 0.4f }, { "ampSustain", 0.75f },
        { "modSlot2Src", SRC_LFO1 }, { "modSlot2Dst", DST_OscAWarp }, { "modSlot2Amt", 0.3f }, { "modSlot2On", 1.0f },
        { "lfo1Rate", 4.0f },
        { "delayMix", 0.22f }, { "reverbMix", 0.3f }, { "distMix", 0.12f }
    })));

    // 20. Iron Howl — Comb sweep howl
    out.push_back (makePreset (apvts, cat, "Iron Howl", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.6f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "filtACutoff", 1800.0f }, { "filtARes", 0.55f }, { "filtADrive", 0.4f },
        { "filtAEnvDepth", 0.65f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.5f }, { "ampSustain", 0.7f },
        { "modSlot2Src", SRC_AmpEnv }, { "modSlot2Dst", DST_OscAPos }, { "modSlot2Amt", 0.4f }, { "modSlot2On", 1.0f },
        { "delayMix", 0.25f }, { "reverbMix", 0.25f }, { "distMix", 0.15f }, { "distDrive", 0.35f }
    })));

    // 21. Mantis Cry — odd harmonics screech
    out.push_back (makePreset (apvts, cat, "Mantis Cry", merge (base, {
        { "oscATable", WT_OddHarmonics }, { "oscAPos", 0.55f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.4f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.6f }, { "filtADrive", 0.3f },
        { "filtAEnvDepth", 0.7f }, { "filtDecay", 0.3f },
        { "ampAttack", 0.005f },
        { "lfo1Rate", 6.5f },
        { "modSlot2Dst", DST_Pitch }, { "modSlot2Amt", 0.05f },
        { "reverbMix", 0.2f }, { "delayMix", 0.18f }, { "distMix", 0.12f }
    })));

    // 22. Velvet Razor — formant + choir lead
    out.push_back (makePreset (apvts, cat, "Velvet Razor", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.45f }, { "oscAUnison", 3 },
        { "oscADetune", 0.2f },
        { "oscBTable", WT_ChoirPad }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 2700.0f }, { "filtARes", 0.3f },
        { "ampAttack", 0.02f }, { "ampDecay", 0.5f }, { "ampSustain", 0.85f },
        { "modSlot2Src", SRC_AT }, { "modSlot2Dst", DST_OscAPos }, { "modSlot2Amt", 0.3f }, { "modSlot2On", 1.0f },
        { "reverbMix", 0.32f }, { "delayMix", 0.2f }, { "chorusMix", 0.2f },
        { "stereoWidth", 1.25f }
    })));

    // 23. Solder Burn — FM metallic + wavefold
    out.push_back (makePreset (apvts, cat, "Solder Burn", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.45f }, { "oscBCoarse", -12 },
        { "filtACutoff", 1900.0f }, { "filtARes", 0.5f }, { "filtADrive", 0.4f },
        { "filtAEnvDepth", 0.7f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.4f },
        { "modSlot2Src", SRC_AmpEnv }, { "modSlot2Dst", DST_OscBWarp }, { "modSlot2Amt", 0.4f }, { "modSlot2On", 1.0f },
        { "delayMix", 0.18f }, { "reverbMix", 0.2f }, { "distMix", 0.18f }, { "distDrive", 0.4f }
    })));

    // 24. Magma Wire — driven harmonic stretch
    out.push_back (makePreset (apvts, cat, "Magma Wire", merge (base, {
        { "oscATable", WT_HarmonicStretch }, { "oscAPos", 0.55f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "filtACutoff", 2000.0f }, { "filtARes", 0.5f }, { "filtADrive", 0.45f },
        { "filtAEnvDepth", 0.6f }, { "filtDecay", 0.3f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.45f }, { "ampSustain", 0.75f },
        { "modSlot2Src", SRC_LFO1 }, { "modSlot2Dst", DST_OscAPos }, { "modSlot2Amt", 0.25f }, { "modSlot2On", 1.0f },
        { "lfo1Rate", 4.0f },
        { "delayMix", 0.22f }, { "reverbMix", 0.25f }, { "distMix", 0.2f }, { "distDrive", 0.45f }
    })));
}

static void addBass (juce::AudioProcessorValueTreeState& apvts,
                     std::vector<Preset>& out)
{
    const juce::String cat = "Bass";
    auto base = bassArchetype();

    out.push_back (makePreset (apvts, cat, "Dark Matter", merge (base, {
        { "oscATable", WT_Saw }, { "subLevel", 0.7f },
        { "filtACutoff", 600.0f }, { "filtARes", 0.35f },
        { "eqLowGain", 3.0f }
    })));

    out.push_back (makePreset (apvts, cat, "Sub Harmonic", merge (base, {
        { "oscATable", WT_Sine }, { "oscALevel", 0.6f },
        { "subLevel", 0.85f }, { "subOctave", 1 },
        { "filtACutoff", 400.0f },
        { "eqLowGain", 4.0f }
    })));

    out.push_back (makePreset (apvts, cat, "Fold Engine", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.6f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.45f },
        { "filtACutoff", 900.0f }, { "filtARes", 0.45f }, { "filtADrive", 0.5f },
        { "distMix", 0.25f }, { "distDrive", 0.5f }, { "distType", 3 }
    })));

    out.push_back (makePreset (apvts, cat, "Reese Foundation", merge (base, {
        { "oscATable", WT_Saw }, { "oscAUnison", 4 }, { "oscADetune", 0.3f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.55f }, { "oscBFine", 12.0f },
        { "filtACutoff", 700.0f }, { "filtARes", 0.5f },
        { "chorusMix", 0.15f }
    })));

    out.push_back (makePreset (apvts, cat, "Voltage Bass", merge (base, {
        { "oscATable", WT_Square }, { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f },
        { "filtACutoff", 800.0f }, { "filtARes", 0.4f },
        { "lfo1Rate", 8.0f }, { "lfo1Shape", 3 },
        { "modSlot2Src", SRC_LFO1 }, { "modSlot2Dst", DST_FiltACut },
        { "modSlot2Amt", 0.15f }, { "modSlot2On", 1 }
    })));

    out.push_back (makePreset (apvts, cat, "Acid Crawler", merge (base, {
        { "oscATable", WT_Saw }, { "subLevel", 0.3f },
        { "filtACutoff", 500.0f }, { "filtARes", 0.7f }, { "filtADrive", 0.4f },
        { "filtAEnvDepth", 0.75f }, { "filtDecay", 0.2f },
        { "distMix", 0.15f }
    })));

    out.push_back (makePreset (apvts, cat, "Wooden Thump", merge (base, {
        { "oscATable", WT_Triangle }, { "subLevel", 0.75f },
        { "filtACutoff", 550.0f }, { "filtARes", 0.25f },
        { "ampDecay", 0.3f }, { "ampSustain", 0.6f },
        { "eqLowGain", 2.5f }, { "eqMidGain", -1.5f }
    })));

    out.push_back (makePreset (apvts, cat, "Pulse Bass", merge (base, {
        { "oscATable", WT_Square }, { "oscAPos", 0.4f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.5f },
        { "filtACutoff", 800.0f }, { "filtARes", 0.35f },
        { "subLevel", 0.6f }
    })));

    out.push_back (makePreset (apvts, cat, "Deep Throat", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscAPos", 0.2f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.5f },
        { "filtACutoff", 700.0f }, { "filtARes", 0.35f }
    })));

    out.push_back (makePreset (apvts, cat, "Growl Machine", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.5f },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.5f },
        { "subLevel", 0.4f },
        { "filtACutoff", 800.0f }, { "filtARes", 0.45f }, { "filtADrive", 0.55f },
        { "distMix", 0.2f }, { "distDrive", 0.45f }, { "distType", 2 }
    })));

    // 11. Iron Wolf — prism spectrum bass
    out.push_back (makePreset (apvts, cat, "Iron Wolf", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.4f }, { "oscAUnison", 3 },
        { "oscADetune", 0.2f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.35f }, { "oscBCoarse", -12 },
        { "subLevel", 0.6f },
        { "filtACutoff", 750.0f }, { "filtARes", 0.4f }, { "filtADrive", 0.35f },
        { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.2f }
    })));

    // 12. Tar Pit — slow harmonic stretch sub
    out.push_back (makePreset (apvts, cat, "Tar Pit", merge (base, {
        { "oscATable", WT_HarmonicStretch }, { "oscAPos", 0.35f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.7f }, { "subShape", 1 }, { "subOctave", -1 },
        { "filtACutoff", 600.0f }, { "filtARes", 0.3f }, { "filtADrive", 0.3f },
        { "ampAttack", 0.01f }, { "ampDecay", 0.6f }, { "ampSustain", 0.7f },
        { "distMix", 0.12f }, { "distDrive", 0.25f }
    })));

    // 13. Magnet Bass — FMMetallic with sub
    out.push_back (makePreset (apvts, cat, "Magnet Bass", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.45f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.3f }, { "oscBCoarse", 0 },
        { "subLevel", 0.55f }, { "subShape", 0 },
        { "filtACutoff", 900.0f }, { "filtARes", 0.4f }, { "filtADrive", 0.3f },
        { "filtAEnvDepth", 0.6f }, { "filtDecay", 0.3f },
        { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.15f },
        { "delayMix", 0.08f }
    })));

    // 14. Bone Saw — wavefold rip
    out.push_back (makePreset (apvts, cat, "Bone Saw", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.55f }, { "oscAUnison", 2 },
        { "oscADetune", 0.15f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.5f },
        { "filtACutoff", 850.0f }, { "filtARes", 0.5f }, { "filtADrive", 0.5f },
        { "filtAEnvDepth", 0.65f }, { "filtDecay", 0.25f },
        { "modSlot1Dst", DST_OscAWarp }, { "modSlot1Amt", 0.4f },
        { "distMix", 0.2f }, { "distDrive", 0.4f }, { "distType", 1 }
    })));

    // 15. Black Tide — comb sweep + sub
    out.push_back (makePreset (apvts, cat, "Black Tide", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.5f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.35f }, { "oscBCoarse", -12 },
        { "subLevel", 0.65f },
        { "filtACutoff", 700.0f }, { "filtARes", 0.35f }, { "filtADrive", 0.4f },
        { "lfo1Rate", 0.5f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.3f }, { "modSlot1On", 1.0f },
        { "distMix", 0.15f }, { "distDrive", 0.3f }
    })));

    // 16. Stutter Gut — bitcrush LFO bass
    out.push_back (makePreset (apvts, cat, "Stutter Gut", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.5f },
        { "filtACutoff", 950.0f }, { "filtARes", 0.45f }, { "filtADrive", 0.35f },
        { "lfo1Rate", 8.0f }, { "lfo1Sync", 1 }, { "lfo1Division", 5 },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_OscAWarp }, { "modSlot1Amt", 0.45f }, { "modSlot1On", 1.0f },
        { "distMix", 0.18f }, { "distDrive", 0.35f }
    })));

    // 17. Velvet Floor — formant sub
    out.push_back (makePreset (apvts, cat, "Velvet Floor", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.35f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.6f }, { "subShape", 0 },
        { "filtACutoff", 1100.0f }, { "filtARes", 0.25f }, { "filtADrive", 0.2f },
        { "ampAttack", 0.01f }, { "ampDecay", 0.7f }, { "ampSustain", 0.85f },
        { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.18f },
        { "chorusMix", 0.1f }
    })));

    // 18. Plasma Trench — odd harmonics drive
    out.push_back (makePreset (apvts, cat, "Plasma Trench", merge (base, {
        { "oscATable", WT_OddHarmonics }, { "oscAPos", 0.45f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.4f }, { "oscBCoarse", -12 },
        { "subLevel", 0.55f },
        { "filtACutoff", 800.0f }, { "filtARes", 0.5f }, { "filtADrive", 0.5f },
        { "filtAEnvDepth", 0.65f }, { "filtDecay", 0.3f },
        { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.2f },
        { "distMix", 0.22f }, { "distDrive", 0.45f }, { "distType", 2 }
    })));

    // 19. Coral Reese — supersaw + wavefold cross
    out.push_back (makePreset (apvts, cat, "Coral Reese", merge (base, {
        { "oscATable", WT_Supersaw }, { "oscAPos", 0.4f }, { "oscAUnison", 7 },
        { "oscADetune", 0.5f }, { "oscAWidth", 0.6f },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.5f }, { "oscBCoarse", -12 },
        { "subLevel", 0.45f },
        { "filtACutoff", 1100.0f }, { "filtARes", 0.4f }, { "filtADrive", 0.35f },
        { "modSlot1Dst", DST_OscBWarp }, { "modSlot1Amt", 0.3f },
        { "chorusMix", 0.15f }, { "distMix", 0.12f }, { "distDrive", 0.3f },
        { "stereoWidth", 1.05f }
    })));

    // 20. Glass Hammer — plucked-string low impact
    out.push_back (makePreset (apvts, cat, "Glass Hammer", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.4f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.45f }, { "oscBCoarse", -12 },
        { "subLevel", 0.5f },
        { "filtACutoff", 950.0f }, { "filtARes", 0.35f }, { "filtADrive", 0.3f },
        { "filtAEnvDepth", 0.7f }, { "filtDecay", 0.25f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.4f }, { "ampSustain", 0.5f },
        { "modSlot1Src", SRC_Vel }, { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.25f }, { "modSlot1On", 1.0f },
        { "distMix", 0.1f }, { "distDrive", 0.25f }
    })));
}

static void addPlucks (juce::AudioProcessorValueTreeState& apvts,
                       std::vector<Preset>& out)
{
    const juce::String cat = "Pluck";
    auto base = pluckArchetype();

    out.push_back (makePreset (apvts, cat, "Water Droplet", merge (base, {
        { "oscATable", WT_Sine }, { "oscBTable", WT_FMBell },
        { "oscBCoarse", 12 }, { "oscBLevel", 0.35f },
        { "ampDecay", 0.5f }, { "filtACutoff", 5500.0f },
        { "reverbMix", 0.42f }, { "delayMix", 0.25f }
    })));

    out.push_back (makePreset (apvts, cat, "Crystal Dew", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.3f },
        { "oscBTable", WT_Sine }, { "oscBCoarse", 19 }, { "oscBLevel", 0.3f },
        { "filtACutoff", 6000.0f },
        { "reverbMix", 0.5f }
    })));

    out.push_back (makePreset (apvts, cat, "Glass Harp", merge (base, {
        { "oscATable", WT_FMBell }, { "oscBTable", WT_Triangle },
        { "oscBCoarse", 7 },
        { "filtACutoff", 5000.0f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayMix", 0.3f }
    })));

    out.push_back (makePreset (apvts, cat, "Tiny Bell", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscACoarse", 12 },
        { "oscBTable", WT_Sine }, { "oscBCoarse", 24 }, { "oscBLevel", 0.25f },
        { "ampDecay", 0.7f }, { "filtACutoff", 6500.0f },
        { "reverbMix", 0.5f }
    })));

    out.push_back (makePreset (apvts, cat, "String Pluck", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.4f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.3f },
        { "filtACutoff", 3800.0f },
        { "chorusMix", 0.18f }, { "reverbMix", 0.32f }
    })));

    out.push_back (makePreset (apvts, cat, "Rain on Bronze", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.5f },
        { "oscBTable", WT_ChurchBell }, { "oscBLevel", 0.45f },
        { "ampDecay", 0.9f },
        { "filtACutoff", 4500.0f },
        { "reverbMix", 0.55f }, { "delayMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Harp Dream", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAUnison", 2 },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.35f }, { "oscBCoarse", 12 },
        { "filtACutoff", 4200.0f },
        { "reverbMix", 0.52f }, { "chorusMix", 0.15f }
    })));

    out.push_back (makePreset (apvts, cat, "Plucked Light", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.2f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.3f },
        { "ampDecay", 0.6f },
        { "filtACutoff", 5500.0f },
        { "reverbMix", 0.45f }
    })));

    out.push_back (makePreset (apvts, cat, "Ceramic Ping", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.3f },
        { "oscBTable", WT_Sine }, { "oscBCoarse", 19 }, { "oscBLevel", 0.3f },
        { "ampDecay", 0.5f },
        { "filtACutoff", 6500.0f },
        { "reverbMix", 0.38f }
    })));

    out.push_back (makePreset (apvts, cat, "Night Rain", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.4f },
        { "oscBTable", WT_PluckedString }, { "oscBLevel", 0.4f },
        { "filtACutoff", 3500.0f },
        { "delaySync", 1 }, { "delayDivision", 5 }, { "delayFeedback", 0.5f }, { "delayMix", 0.35f },
        { "reverbMix", 0.48f }
    })));

    // 11. Brass Bell — short church bell pluck
    out.push_back (makePreset (apvts, cat, "Brass Bell", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.3f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 4500.0f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.6f }, { "ampRelease", 0.5f },
        { "reverbMix", 0.45f }, { "delayMix", 0.18f }
    })));

    // 12. Wire Fence — plucked string pluck with LFO
    out.push_back (makePreset (apvts, cat, "Wire Fence", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.3f }, { "oscBCoarse", 7 },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.25f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.7f },
        { "lfo1Rate", 6.0f },
        { "modSlot0Src", SRC_LFO1 }, { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.15f }, { "modSlot0On", 1.0f },
        { "delayMix", 0.22f }, { "reverbMix", 0.3f }
    })));

    // 13. Gut String — warm plucked string
    out.push_back (makePreset (apvts, cat, "Gut String", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.25f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.45f }, { "oscBCoarse", -12 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.15f },
        { "ampAttack", 0.003f }, { "ampDecay", 0.9f }, { "ampSustain", 0.0f },
        { "reverbMix", 0.32f }, { "delayMix", 0.1f }
    })));

    // 14. Comb Drop — short comb sweep
    out.push_back (makePreset (apvts, cat, "Comb Drop", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.55f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.3f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.6f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.3f },
        { "delayMix", 0.25f }, { "delayTime", 0.25f }, { "reverbMix", 0.32f }
    })));

    // 15. Solder Drop — FM metallic short pluck
    out.push_back (makePreset (apvts, cat, "Solder Drop", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.45f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.35f }, { "oscBCoarse", 19 },
        { "filtACutoff", 4000.0f }, { "filtARes", 0.2f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.5f },
        { "filtAEnvDepth", 0.55f }, { "filtDecay", 0.3f },
        { "delayMix", 0.22f }, { "reverbMix", 0.3f }, { "chorusMix", 0.12f }
    })));

    // 16. Vellum Harp — formant filter pluck
    out.push_back (makePreset (apvts, cat, "Vellum Harp", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.45f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.35f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.2f },
        { "ampAttack", 0.003f }, { "ampDecay", 0.7f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.2f },
        { "reverbMix", 0.4f }, { "delayMix", 0.15f }, { "chorusMix", 0.18f }
    })));

    // 17. Shell Click — odd harmonics very short
    out.push_back (makePreset (apvts, cat, "Shell Click", merge (base, {
        { "oscATable", WT_OddHarmonics }, { "oscAPos", 0.55f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.3f }, { "oscBCoarse", 24 },
        { "filtACutoff", 5000.0f }, { "filtARes", 0.3f },
        { "ampAttack", 0.001f }, { "ampDecay", 0.3f }, { "ampRelease", 0.3f },
        { "filtAEnvDepth", 0.6f }, { "filtDecay", 0.15f },
        { "reverbMix", 0.35f }, { "delayMix", 0.18f }
    })));

    // 18. Frost Bell — chorus-shimmered church bell
    out.push_back (makePreset (apvts, cat, "Frost Bell", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.4f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.45f }, { "oscBCoarse", 7 },
        { "filtACutoff", 4200.0f },
        { "ampAttack", 0.003f }, { "ampDecay", 1.0f }, { "ampRelease", 0.8f },
        { "chorusMix", 0.4f }, { "chorusDepth", 0.7f }, { "chorusRate", 0.6f },
        { "reverbMix", 0.45f }, { "delayMix", 0.22f }
    })));

    // 19. Wax Pluck — wavefold short
    out.push_back (makePreset (apvts, cat, "Wax Pluck", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.45f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.25f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.55f },
        { "filtAEnvDepth", 0.55f }, { "filtDecay", 0.25f },
        { "modSlot0Dst", DST_OscAWarp }, { "modSlot0Amt", 0.3f },
        { "delayMix", 0.18f }, { "reverbMix", 0.3f }
    })));

    // 20. Spectral Spark — prism spectrum delay tail
    out.push_back (makePreset (apvts, cat, "Spectral Spark", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 19 },
        { "filtACutoff", 4500.0f }, { "filtARes", 0.2f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.7f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayFeedback", 0.55f }, { "delayMix", 0.4f },
        { "reverbMix", 0.42f }, { "stereoWidth", 1.3f }
    })));
}

static void addKeys (juce::AudioProcessorValueTreeState& apvts,
                     std::vector<Preset>& out)
{
    const juce::String cat = "Keys";
    auto base = keysArchetype();

    out.push_back (makePreset (apvts, cat, "E.Piano Vintage", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.3f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.25f },
        { "filtACutoff", 3800.0f },
        { "chorusMix", 0.28f }, { "eqLowGain", 1.5f }
    })));

    out.push_back (makePreset (apvts, cat, "FM Bell Keys", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.25f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.3f }, { "oscBCoarse", 12 },
        { "filtACutoff", 5500.0f },
        { "reverbMix", 0.38f }, { "delayMix", 0.18f }
    })));

    out.push_back (makePreset (apvts, cat, "Tine Warm", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.45f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.3f },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.15f },
        { "chorusMix", 0.3f }, { "reverbMix", 0.32f }
    })));

    out.push_back (makePreset (apvts, cat, "Glass Keys", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.2f },
        { "oscBTable", WT_Sine }, { "oscBCoarse", 19 }, { "oscBLevel", 0.25f },
        { "filtACutoff", 6500.0f },
        { "reverbMix", 0.45f }
    })));

    out.push_back (makePreset (apvts, cat, "Church Organ", merge (base, {
        { "oscATable", WT_OrganSweep }, { "oscAPos", 0.3f }, { "oscAUnison", 4 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.5f }, { "oscBCoarse", 12 },
        { "ampAttack", 0.01f }, { "ampRelease", 0.8f },
        { "filtACutoff", 4500.0f },
        { "reverbMix", 0.6f }, { "reverbSize", 0.9f }
    })));

    out.push_back (makePreset (apvts, cat, "Mellow Wurli", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.5f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.35f },
        { "filtACutoff", 2800.0f },
        { "distMix", 0.08f }, { "distDrive", 0.2f }, { "distType", 2 },
        { "chorusMix", 0.22f }
    })));

    out.push_back (makePreset (apvts, cat, "Celesta Dream", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.4f },
        { "oscBTable", WT_Triangle }, { "oscBCoarse", 12 }, { "oscBLevel", 0.3f },
        { "filtACutoff", 5500.0f },
        { "ampDecay", 1.8f }, { "ampSustain", 0.2f },
        { "reverbMix", 0.5f }
    })));

    out.push_back (makePreset (apvts, cat, "Electric Piano Bright", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.15f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.3f }, { "oscBCoarse", 12 },
        { "filtACutoff", 5000.0f },
        { "eqHighGain", 3.0f }, { "chorusMix", 0.25f }
    })));

    // 9. Cathedral Pipe — organ sweep with reverb
    out.push_back (makePreset (apvts, cat, "Cathedral Pipe", merge (base, {
        { "oscATable", WT_OrganSweep }, { "oscAPos", 0.35f }, { "oscAUnison", 3 },
        { "oscADetune", 0.2f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", 7 },
        { "filtACutoff", 3500.0f },
        { "ampAttack", 0.05f }, { "ampDecay", 1.5f }, { "ampSustain", 0.85f }, { "ampRelease", 0.8f },
        { "reverbMix", 0.55f }, { "reverbSize", 0.9f },
        { "stereoWidth", 1.3f }
    })));

    // 10. Reed Organ — formant filter slow attack
    out.push_back (makePreset (apvts, cat, "Reed Organ", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.4f }, { "oscAUnison", 3 },
        { "oscADetune", 0.2f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.15f },
        { "ampAttack", 0.15f }, { "ampDecay", 1.0f }, { "ampSustain", 0.85f }, { "ampRelease", 0.6f },
        { "reverbMix", 0.4f }, { "chorusMix", 0.25f }
    })));

    // 11. Vibraphone Cold — FM bell with chorus
    out.push_back (makePreset (apvts, cat, "Vibraphone Cold", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.35f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.45f }, { "oscBCoarse", 0 },
        { "filtACutoff", 4500.0f }, { "filtARes", 0.1f },
        { "ampAttack", 0.005f }, { "ampDecay", 1.8f }, { "ampSustain", 0.3f }, { "ampRelease", 0.7f },
        { "lfo1Rate", 5.0f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_Pitch }, { "modSlot1Amt", 0.02f }, { "modSlot1On", 1.0f },
        { "chorusMix", 0.45f }, { "chorusDepth", 0.7f }, { "reverbMix", 0.35f }
    })));

    // 12. Mbira — plucked thumb piano with vibrato
    out.push_back (makePreset (apvts, cat, "Mbira", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.3f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.2f },
        { "ampAttack", 0.003f }, { "ampDecay", 1.4f }, { "ampSustain", 0.0f }, { "ampRelease", 0.5f },
        { "lfo1Rate", 5.5f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_Pitch }, { "modSlot1Amt", 0.03f }, { "modSlot1On", 1.0f },
        { "reverbMix", 0.32f }, { "delayMix", 0.18f }
    })));

    // 13. Music Box — small FM metallic chime
    out.push_back (makePreset (apvts, cat, "Music Box", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.2f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 19 },
        { "filtACutoff", 5500.0f },
        { "ampAttack", 0.002f }, { "ampDecay", 1.5f }, { "ampSustain", 0.0f }, { "ampRelease", 0.7f },
        { "reverbMix", 0.5f }, { "reverbSize", 0.7f },
        { "delayMix", 0.18f }, { "delayTime", 0.375f }
    })));

    // 14. Harmonium — harmonic series with vibrato
    out.push_back (makePreset (apvts, cat, "Harmonium", merge (base, {
        { "oscATable", WT_HarmonicSeries }, { "oscAPos", 0.35f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 2800.0f }, { "filtARes", 0.15f },
        { "ampAttack", 0.1f }, { "ampDecay", 1.2f }, { "ampSustain", 0.85f }, { "ampRelease", 0.7f },
        { "lfo1Rate", 4.5f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_Pitch }, { "modSlot1Amt", 0.025f }, { "modSlot1On", 1.0f },
        { "reverbMix", 0.4f }, { "chorusMix", 0.2f }
    })));

    // 15. Wood Rhodes — warm electric piano
    out.push_back (makePreset (apvts, cat, "Wood Rhodes", merge (base, {
        { "oscATable", WT_FMEPiano }, { "oscAPos", 0.3f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.35f }, { "oscBCoarse", 0 },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.1f },
        { "ampAttack", 0.005f }, { "ampDecay", 2.5f }, { "ampSustain", 0.4f }, { "ampRelease", 0.7f },
        { "eqLowGain", 1.5f }, { "eqHighGain", -1.0f },
        { "chorusMix", 0.25f }, { "reverbMix", 0.32f }
    })));

    // 16. Vox Continental — formant organ
    out.push_back (makePreset (apvts, cat, "Vox Continental", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.5f }, { "oscAUnison", 3 },
        { "oscADetune", 0.2f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 4000.0f }, { "filtARes", 0.2f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.4f }, { "ampSustain", 0.95f }, { "ampRelease", 0.4f },
        { "chorusMix", 0.45f }, { "chorusDepth", 0.65f }, { "chorusRate", 4.5f },
        { "reverbMix", 0.25f }
    })));

    // 17. Toy Piano — short FM metallic
    out.push_back (makePreset (apvts, cat, "Toy Piano", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.15f },
        { "oscBTable", WT_PluckedString }, { "oscBLevel", 0.35f }, { "oscBCoarse", 0 },
        { "filtACutoff", 4200.0f }, { "filtARes", 0.15f },
        { "ampAttack", 0.002f }, { "ampDecay", 1.2f }, { "ampSustain", 0.15f }, { "ampRelease", 0.5f },
        { "reverbMix", 0.3f }, { "delayMix", 0.1f }
    })));

    // 18. Gamelan — church bell metallic
    out.push_back (makePreset (apvts, cat, "Gamelan", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.5f },
        { "oscBTable", WT_FMMetallic }, { "oscBLevel", 0.5f }, { "oscBCoarse", 7 },
        { "filtACutoff", 4500.0f }, { "filtARes", 0.15f },
        { "ampAttack", 0.002f }, { "ampDecay", 1.6f }, { "ampSustain", 0.0f }, { "ampRelease", 0.8f },
        { "reverbMix", 0.5f }, { "reverbSize", 0.85f },
        { "delayMix", 0.22f }, { "stereoWidth", 1.3f }
    })));

    // 19. Glass Marimba — FM bell pluck
    out.push_back (makePreset (apvts, cat, "Glass Marimba", merge (base, {
        { "oscATable", WT_FMBell }, { "oscAPos", 0.25f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 4000.0f }, { "filtARes", 0.18f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.9f }, { "ampSustain", 0.0f }, { "ampRelease", 0.5f },
        { "filtAEnvDepth", 0.4f }, { "filtDecay", 0.3f },
        { "reverbMix", 0.38f }, { "delayMix", 0.18f }
    })));

    // 20. Choir Organ — choir pad slow attack
    out.push_back (makePreset (apvts, cat, "Choir Organ", merge (base, {
        { "oscATable", WT_ChoirPad }, { "oscAPos", 0.4f }, { "oscAUnison", 3 },
        { "oscADetune", 0.25f },
        { "oscBTable", WT_OrganSweep }, { "oscBLevel", 0.45f }, { "oscBCoarse", 0 },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.1f },
        { "ampAttack", 0.2f }, { "ampDecay", 1.2f }, { "ampSustain", 0.9f }, { "ampRelease", 1.0f },
        { "reverbMix", 0.5f }, { "reverbSize", 0.9f },
        { "chorusMix", 0.3f }, { "stereoWidth", 1.35f }
    })));
}

static void addSeq (juce::AudioProcessorValueTreeState& apvts,
                    std::vector<Preset>& out)
{
    const juce::String cat = "Sequence";
    auto base = seqArchetype();

    out.push_back (makePreset (apvts, cat, "Pulse Grid", merge (base, {
        { "oscATable", WT_Square }, { "oscBTable", WT_Saw },
        { "filtACutoff", 2500.0f }, { "filtARes", 0.35f },
        { "lfo1Rate", 4.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 },
        { "delayDivision", 3 }
    })));

    out.push_back (makePreset (apvts, cat, "Mod Sequence", merge (base, {
        { "oscATable", WT_Supersaw }, { "oscAUnison", 3 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f },
        { "filtACutoff", 2800.0f },
        { "lfo1Shape", 2 }, { "lfo1Division", 2 }
    })));

    out.push_back (makePreset (apvts, cat, "Tempo Echo", merge (base, {
        { "oscATable", WT_Saw }, { "oscBTable", WT_Sine }, { "oscBCoarse", 12 },
        { "filtACutoff", 3000.0f },
        { "delayDivision", 2 }, { "delayFeedback", 0.6f }, { "delayMix", 0.4f }
    })));

    out.push_back (makePreset (apvts, cat, "Ping Delay Arp", merge (base, {
        { "oscATable", WT_Triangle }, { "oscBTable", WT_FMBell }, { "oscBLevel", 0.35f },
        { "filtACutoff", 4000.0f }, { "filtARes", 0.2f },
        { "delayMode", 1 }, { "delayDivision", 3 }, { "delayFeedback", 0.55f }, { "delayMix", 0.4f }
    })));

    out.push_back (makePreset (apvts, cat, "Binary Clock", merge (base, {
        { "oscATable", WT_Square }, { "oscBTable", WT_Square }, { "oscBCoarse", 7 }, { "oscBLevel", 0.45f },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.45f },
        { "lfo1Rate", 8.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 5 }
    })));

    out.push_back (makePreset (apvts, cat, "Stepping Stone", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscBTable", WT_Triangle },
        { "ampAttack", 0.005f }, { "ampDecay", 0.4f }, { "ampSustain", 0.2f },
        { "filtACutoff", 3200.0f },
        { "delayFeedback", 0.5f }, { "delayMix", 0.35f }
    })));

    out.push_back (makePreset (apvts, cat, "Clockwork", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscBTable", WT_Square }, { "oscBLevel", 0.4f },
        { "filtACutoff", 2600.0f },
        { "lfo1Sync", 1 }, { "lfo1Division", 4 }, { "lfo2Division", 5 }
    })));

    out.push_back (makePreset (apvts, cat, "Trance Gate", merge (base, {
        { "oscATable", WT_Supersaw }, { "oscAUnison", 5 }, { "oscADetune", 0.35f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.55f }, { "oscBCoarse", -12 },
        { "filtACutoff", 2800.0f },
        { "lfo1Rate", 8.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "modSlot0Src", SRC_LFO1 }, { "modSlot0Dst", DST_MasterVol },
        { "modSlot0Amt", 0.6f }, { "modSlot0On", 1 },
        { "reverbMix", 0.3f }
    })));

    out.push_back (makePreset (apvts, cat, "Stutter Glitch", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.6f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.45f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2000.0f }, { "filtARes", 0.5f },
        { "lfo1Rate", 16.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 5 },
        { "modSlot0Src", SRC_LFO1 }, { "modSlot0Dst", DST_MasterVol },
        { "modSlot0Amt", 0.7f }, { "modSlot0On", 1 },
        { "distMix", 0.15f }, { "delayMix", 0.25f }
    })));

    out.push_back (makePreset (apvts, cat, "Ripple Chain", merge (base, {
        { "oscATable", WT_Triangle }, { "oscAUnison", 3 }, { "oscADetune", 0.2f },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.25f },
        { "lfo1Rate", 6.0f }, { "lfo1Shape", 0 }, { "lfo1Sync", 1 }, { "lfo1Division", 3 },
        { "delayMode", 1 }, { "delayDivision", 4 }, { "delayFeedback", 0.6f }, { "delayMix", 0.4f },
        { "reverbMix", 0.28f }
    })));

    out.push_back (makePreset (apvts, cat, "Spectral Chop", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.5f },
        { "oscBTable", WT_CombSweep }, { "oscBLevel", 0.5f }, { "oscBPos", 0.4f },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.4f },
        { "lfo1Rate", 4.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "lfo2Rate", 2.0f }, { "lfo2Shape", 1 }, { "lfo2Sync", 1 }, { "lfo2Division", 2 },
        { "modSlot1Src", SRC_LFO2 }, { "modSlot1Dst", DST_OscAPos },
        { "modSlot1Amt", 0.4f }, { "modSlot1On", 1 },
        { "reverbMix", 0.22f }, { "delayMix", 0.3f }
    })));

    out.push_back (makePreset (apvts, cat, "Neon Pulse", merge (base, {
        { "oscATable", WT_PWMSweep }, { "oscAPos", 0.4f }, { "oscAUnison", 4 }, { "oscADetune", 0.25f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.45f },
        { "filtACutoff", 1800.0f }, { "filtARes", 0.45f }, { "filtAEnvDepth", 0.6f },
        { "lfo1Rate", 4.0f }, { "lfo1Shape", 0 }, { "lfo1Sync", 1 }, { "lfo1Division", 3 },
        { "chorusMix", 0.2f }, { "delayMix", 0.3f }, { "reverbMix", 0.25f }
    })));

    // 13. Crystal Gate — bitcrush LFO sequence
    out.push_back (makePreset (apvts, cat, "Crystal Gate", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.4f }, { "filtAEnvDepth", 0.55f },
        { "lfo1Rate", 4.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "modSlot0Dst", DST_OscAWarp }, { "modSlot0Amt", 0.4f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayMix", 0.32f },
        { "reverbMix", 0.28f }, { "chorusMix", 0.15f }
    })));

    // 14. Mantra Arp — FMMetallic delayed arp
    out.push_back (makePreset (apvts, cat, "Mantra Arp", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.4f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3200.0f }, { "filtARes", 0.3f }, { "filtAEnvDepth", 0.5f },
        { "lfo1Rate", 6.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 5 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.3f },
        { "delaySync", 1 }, { "delayDivision", 5 }, { "delayFeedback", 0.55f }, { "delayMix", 0.35f },
        { "reverbMix", 0.32f }
    })));

    // 15. Beam Walk — formant filter LFO sequence
    out.push_back (makePreset (apvts, cat, "Beam Walk", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.4f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.4f },
        { "lfo1Rate", 8.0f }, { "lfo1Shape", 0 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.4f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_FiltACut }, { "modSlot1Amt", 0.5f }, { "modSlot1On", 1.0f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayMix", 0.3f },
        { "chorusMix", 0.2f }, { "reverbMix", 0.25f }
    })));

    // 16. Marble Step — plucked string arp gate
    out.push_back (makePreset (apvts, cat, "Marble Step", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.35f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.35f }, { "oscBCoarse", 12 },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.3f },
        { "lfo1Rate", 6.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "modSlot0Dst", DST_OscMix }, { "modSlot0Amt", 0.4f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.3f }, { "ampSustain", 0.3f }, { "ampRelease", 0.15f },
        { "delaySync", 1 }, { "delayDivision", 5 }, { "delayMix", 0.3f }, { "reverbMix", 0.3f }
    })));

    // 17. Solder Loop — wavefold LFO loop
    out.push_back (makePreset (apvts, cat, "Solder Loop", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.5f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMMetallic }, { "oscBLevel", 0.45f }, { "oscBCoarse", 0 },
        { "filtACutoff", 1900.0f }, { "filtARes", 0.5f }, { "filtADrive", 0.3f }, { "filtAEnvDepth", 0.55f },
        { "lfo1Rate", 8.0f }, { "lfo1Shape", 1 }, { "lfo1Sync", 1 }, { "lfo1Division", 5 },
        { "modSlot0Dst", DST_OscAWarp }, { "modSlot0Amt", 0.45f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayMix", 0.32f },
        { "distMix", 0.15f }, { "distDrive", 0.3f }
    })));

    // 18. Vowel Drift — vowel morph slow LFO
    out.push_back (makePreset (apvts, cat, "Vowel Drift", merge (base, {
        { "oscATable", WT_VowelMorph }, { "oscAPos", 0.4f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.35f }, { "oscBCoarse", -12 },
        { "filtACutoff", 2600.0f }, { "filtARes", 0.3f },
        { "lfo1Rate", 1.0f }, { "lfo1Shape", 0 }, { "lfo1Sync", 1 }, { "lfo1Division", 6 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.4f },
        { "lfo2Rate", 4.0f }, { "lfo2Sync", 1 }, { "lfo2Division", 4 },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayMix", 0.28f },
        { "reverbMix", 0.32f }, { "chorusMix", 0.2f }
    })));

    // 19. Strobe Forest — comb sweep delay sequence
    out.push_back (makePreset (apvts, cat, "Strobe Forest", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.5f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f },
        { "oscBTable", WT_Square }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.45f },
        { "lfo1Rate", 8.0f }, { "lfo1Shape", 0 }, { "lfo1Sync", 1 }, { "lfo1Division", 5 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.4f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayFeedback", 0.55f }, { "delayMix", 0.4f },
        { "reverbMix", 0.3f }, { "stereoWidth", 1.4f }
    })));

    // 20. Liquid Counter — harmonic series filter LFO
    out.push_back (makePreset (apvts, cat, "Liquid Counter", merge (base, {
        { "oscATable", WT_HarmonicSeries }, { "oscAPos", 0.35f }, { "oscAUnison", 2 },
        { "oscBTable", WT_Saw }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.45f }, { "filtAEnvDepth", 0.5f },
        { "lfo1Rate", 6.0f }, { "lfo1Shape", 2 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "modSlot0Dst", DST_FiltACut }, { "modSlot0Amt", 0.55f },
        { "delaySync", 1 }, { "delayDivision", 5 }, { "delayMix", 0.3f },
        { "reverbMix", 0.3f }, { "chorusMix", 0.18f }
    })));

    // 21. Glass Telegraph — prism spectrum delay
    out.push_back (makePreset (apvts, cat, "Glass Telegraph", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.45f }, { "oscAUnison", 2 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "filtACutoff", 2800.0f }, { "filtARes", 0.4f },
        { "lfo1Rate", 6.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 5 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.35f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.4f }, { "ampSustain", 0.4f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayFeedback", 0.6f }, { "delayMix", 0.4f },
        { "reverbMix", 0.32f }, { "stereoWidth", 1.4f }
    })));

    // 22. Steam Cycle — breath LFO gate
    out.push_back (makePreset (apvts, cat, "Steam Cycle", merge (base, {
        { "oscATable", WT_Breath }, { "oscAPos", 0.45f }, { "oscAUnison", 3 },
        { "oscADetune", 0.3f },
        { "oscBTable", WT_FilteredNoise }, { "oscBLevel", 0.4f },
        { "noiseType", 1 }, { "noiseLevel", 0.1f },
        { "filtACutoff", 2000.0f }, { "filtARes", 0.4f },
        { "lfo1Rate", 4.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 1 }, { "lfo1Division", 4 },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.3f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_OscMix }, { "modSlot1Amt", 0.4f }, { "modSlot1On", 1.0f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayMix", 0.3f },
        { "reverbMix", 0.32f }, { "stereoWidth", 1.45f }
    })));
}

static void addFX (juce::AudioProcessorValueTreeState& apvts,
                   std::vector<Preset>& out)
{
    const juce::String cat = "FX";
    auto base = fxArchetype();

    out.push_back (makePreset (apvts, cat, "Rise to Heaven", merge (base, {
        { "oscATable", WT_Supersaw }, { "oscAUnison", 7 },
        { "oscBTable", WT_FilteredNoise }, { "oscBLevel", 0.5f },
        { "ampAttack", 4.0f }, { "ampRelease", 2.0f },
        { "filtACutoff", 1000.0f }, { "filtAEnvDepth", 0.9f },
        { "filtAttack", 5.0f }, { "reverbMix", 0.7f }
    })));

    out.push_back (makePreset (apvts, cat, "Downward Spiral", merge (base, {
        { "oscATable", WT_Saw }, { "oscAUnison", 5 },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.5f },
        { "ampAttack", 0.02f }, { "ampRelease", 3.0f },
        { "filtAEnvDepth", -0.8f }, { "filtACutoff", 5000.0f }, { "filtARes", 0.4f },
        { "modSlot2Src", SRC_AmpEnv }, { "modSlot2Dst", DST_Pitch },
        { "modSlot2Amt", -0.25f }, { "modSlot2On", 1 }
    })));

    out.push_back (makePreset (apvts, cat, "Impact Bloom", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscBTable", WT_FilteredNoise },
        { "noiseType", 2 }, { "noiseLevel", 0.5f },
        { "ampAttack", 0.002f }, { "ampDecay", 0.3f }, { "ampSustain", 0.0f }, { "ampRelease", 2.5f },
        { "filtACutoff", 1500.0f }, { "filtAEnvDepth", 0.7f },
        { "reverbMix", 0.7f }, { "distMix", 0.2f }
    })));

    out.push_back (makePreset (apvts, cat, "Whoosh Pass", merge (base, {
        { "oscATable", WT_Wind }, { "oscBTable", WT_FilteredNoise },
        { "noiseType", 5 }, { "noiseLevel", 0.6f },
        { "filtAType", 4 }, { "filtACutoff", 2000.0f }, { "filtARes", 0.4f },
        { "ampAttack", 1.5f }, { "ampRelease", 1.5f },
        { "modSlot0Amt", 0.8f },
        { "reverbMix", 0.55f }, { "stereoWidth", 1.7f }
    })));

    out.push_back (makePreset (apvts, cat, "Tape Stop", merge (base, {
        { "oscATable", WT_Saw }, { "oscAUnison", 4 },
        { "oscBTable", WT_Sine }, { "oscBLevel", 0.4f },
        { "ampAttack", 0.05f }, { "ampRelease", 2.0f },
        { "modSlot2Src", SRC_AmpEnv }, { "modSlot2Dst", DST_Pitch },
        { "modSlot2Amt", -0.5f }, { "modSlot2On", 1 },
        { "delayMix", 0.3f }, { "reverbMix", 0.4f }
    })));

    out.push_back (makePreset (apvts, cat, "Metallic Rain", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.6f }, { "oscAUnison", 3 },
        { "oscBTable", WT_ChurchBell }, { "oscBLevel", 0.45f }, { "oscBPos", 0.4f },
        { "ampAttack", 0.8f }, { "ampDecay", 1.5f }, { "ampSustain", 0.4f }, { "ampRelease", 3.0f },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.2f },
        { "delayMode", 1 }, { "delayFeedback", 0.65f }, { "delayMix", 0.45f },
        { "reverbMix", 0.6f }, { "reverbSize", 0.9f }
    })));

    out.push_back (makePreset (apvts, cat, "Glitch Storm", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.8f }, { "oscAUnison", 6 }, { "oscADetune", 0.6f },
        { "oscBTable", WT_FilteredNoise }, { "oscBLevel", 0.5f },
        { "noiseType", 3 }, { "noiseLevel", 0.4f },
        { "ampAttack", 0.01f }, { "ampDecay", 0.5f }, { "ampSustain", 0.3f }, { "ampRelease", 1.5f },
        { "filtACutoff", 1500.0f }, { "filtARes", 0.5f }, { "filtAEnvDepth", 0.6f },
        { "lfo1Rate", 12.0f }, { "lfo1Shape", 4 },
        { "distMix", 0.3f }, { "distDrive", 0.5f },
        { "delayMix", 0.35f }, { "reverbMix", 0.5f }
    })));

    out.push_back (makePreset (apvts, cat, "Frozen Drift", merge (base, {
        { "oscATable", WT_ChoirPad }, { "oscAPos", 0.5f }, { "oscAUnison", 7 }, { "oscADetune", 0.5f },
        { "oscBTable", WT_Wind }, { "oscBLevel", 0.4f },
        { "noiseType", 1 }, { "noiseLevel", 0.15f },
        { "ampAttack", 3.0f }, { "ampRelease", 5.0f },
        { "filtACutoff", 2500.0f }, { "filtAEnvDepth", 0.3f },
        { "lfo1Rate", 0.08f }, { "lfo1FreeRun", 1 },
        { "modSlot0Src", SRC_LFO1 }, { "modSlot0Dst", DST_OscAPos },
        { "modSlot0Amt", 0.4f }, { "modSlot0On", 1 },
        { "reverbMix", 0.75f }, { "reverbSize", 0.98f }, { "reverbDamp", 0.2f },
        { "stereoWidth", 1.8f }
    })));

    // 9. Reverse Surge — wind reverse swell
    out.push_back (makePreset (apvts, cat, "Reverse Surge", merge (base, {
        { "oscATable", WT_Wind }, { "oscAPos", 0.4f }, { "oscAUnison", 5 },
        { "oscBTable", WT_FilteredNoise }, { "oscBLevel", 0.5f },
        { "noiseType", 2 }, { "noiseLevel", 0.4f },
        { "filtACutoff", 1800.0f }, { "filtAEnvDepth", 0.7f },
        { "ampAttack", 1.5f }, { "ampDecay", 0.05f }, { "ampSustain", 0.0f }, { "ampRelease", 0.05f },
        { "filtAttack", 1.5f }, { "filtDecay", 0.05f }, { "filtRelease", 0.05f },
        { "reverbMix", 0.65f }, { "delayMix", 0.35f }, { "delayFeedback", 0.6f },
        { "stereoWidth", 1.7f }
    })));

    // 10. Solar Flare — harmonic stretch sweep
    out.push_back (makePreset (apvts, cat, "Solar Flare", merge (base, {
        { "oscATable", WT_HarmonicStretch }, { "oscAPos", 0.5f }, { "oscAUnison", 7 },
        { "oscADetune", 0.55f },
        { "oscBTable", WT_PrismSpectrum }, { "oscBLevel", 0.55f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.4f }, { "filtAEnvDepth", 0.7f },
        { "ampAttack", 0.5f }, { "ampDecay", 1.0f }, { "ampSustain", 0.6f }, { "ampRelease", 1.5f },
        { "lfo1Rate", 0.3f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.4f },
        { "reverbMix", 0.65f }, { "delayMix", 0.35f }, { "distMix", 0.15f }, { "distDrive", 0.3f },
        { "stereoWidth", 1.7f }
    })));

    // 11. Atom Split — bitcrush delay storm
    out.push_back (makePreset (apvts, cat, "Atom Split", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.6f }, { "oscAUnison", 3 },
        { "oscBTable", WT_FMMetallic }, { "oscBLevel", 0.5f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.5f }, { "filtAEnvDepth", 0.6f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.5f }, { "ampSustain", 0.4f }, { "ampRelease", 1.0f },
        { "lfo1Rate", 8.0f },
        { "modSlot0Dst", DST_OscAWarp }, { "modSlot0Amt", 0.5f },
        { "delaySync", 1 }, { "delayDivision", 4 }, { "delayFeedback", 0.7f }, { "delayMix", 0.5f },
        { "reverbMix", 0.45f }, { "distMix", 0.18f }
    })));

    // 12. Comet Tail — filtered noise long verb
    out.push_back (makePreset (apvts, cat, "Comet Tail", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscAPos", 0.55f }, { "oscAUnison", 3 },
        { "oscBTable", WT_Wind }, { "oscBLevel", 0.4f },
        { "noiseType", 1 }, { "noiseLevel", 0.25f },
        { "filtACutoff", 2000.0f }, { "filtAEnvDepth", 0.5f },
        { "ampAttack", 0.4f }, { "ampDecay", 1.5f }, { "ampSustain", 0.3f }, { "ampRelease", 3.0f },
        { "reverbMix", 0.78f }, { "reverbSize", 0.98f }, { "reverbPredelay", 60.0f },
        { "delayMix", 0.4f }, { "delayFeedback", 0.65f }, { "delayTime", 0.85f },
        { "stereoWidth", 1.75f }
    })));

    // 13. Storm Approach — wind slow LFO
    out.push_back (makePreset (apvts, cat, "Storm Approach", merge (base, {
        { "oscATable", WT_Wind }, { "oscAPos", 0.45f }, { "oscAUnison", 7 },
        { "oscADetune", 0.55f },
        { "oscBTable", WT_FilteredNoise }, { "oscBLevel", 0.55f },
        { "noiseType", 2 }, { "noiseLevel", 0.4f },
        { "filtACutoff", 1500.0f },
        { "ampAttack", 2.5f }, { "ampDecay", 1.0f }, { "ampSustain", 0.7f }, { "ampRelease", 2.5f },
        { "lfo1Rate", 0.15f },
        { "modSlot0Dst", DST_FiltACut }, { "modSlot0Amt", 0.4f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.3f }, { "modSlot1On", 1.0f },
        { "reverbMix", 0.65f }, { "delayMix", 0.3f },
        { "stereoWidth", 1.85f }
    })));

    // 14. Bell Rain Backwards — reverse church bell
    out.push_back (makePreset (apvts, cat, "Bell Rain Backwards", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.4f }, { "oscAUnison", 5 },
        { "oscADetune", 0.45f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.5f }, { "oscBCoarse", 12 },
        { "filtACutoff", 4500.0f }, { "filtAEnvDepth", 0.5f },
        { "ampAttack", 2.0f }, { "ampDecay", 0.05f }, { "ampSustain", 0.0f }, { "ampRelease", 0.05f },
        { "filtAttack", 2.0f }, { "filtRelease", 0.05f },
        { "reverbMix", 0.78f }, { "reverbSize", 0.98f },
        { "delayMix", 0.4f }, { "delayFeedback", 0.65f }, { "delayTime", 0.5f },
        { "stereoWidth", 1.7f }
    })));

    // 15. Vapor Trail — breath tail
    out.push_back (makePreset (apvts, cat, "Vapor Trail", merge (base, {
        { "oscATable", WT_Breath }, { "oscAPos", 0.5f }, { "oscAUnison", 5 },
        { "oscADetune", 0.5f },
        { "oscBTable", WT_VowelMorph }, { "oscBLevel", 0.4f }, { "oscBCoarse", 0 },
        { "noiseType", 1 }, { "noiseLevel", 0.12f },
        { "filtACutoff", 2200.0f },
        { "ampAttack", 1.0f }, { "ampDecay", 1.5f }, { "ampSustain", 0.5f }, { "ampRelease", 2.5f },
        { "lfo1Rate", 0.2f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.3f },
        { "reverbMix", 0.7f }, { "reverbSize", 0.95f },
        { "delayMix", 0.3f }, { "stereoWidth", 1.7f }
    })));

    // 16. Mech Bloom — comb sweep impact
    out.push_back (makePreset (apvts, cat, "Mech Bloom", merge (base, {
        { "oscATable", WT_CombSweep }, { "oscAPos", 0.55f }, { "oscAUnison", 5 },
        { "oscADetune", 0.4f },
        { "oscBTable", WT_FMMetallic }, { "oscBLevel", 0.5f }, { "oscBCoarse", -7 },
        { "filtACutoff", 2400.0f }, { "filtARes", 0.4f }, { "filtAEnvDepth", 0.65f },
        { "ampAttack", 0.05f }, { "ampDecay", 1.5f }, { "ampSustain", 0.4f }, { "ampRelease", 2.0f },
        { "lfo1Rate", 0.4f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.4f },
        { "reverbMix", 0.7f }, { "delayMix", 0.4f }, { "delayFeedback", 0.55f },
        { "distMix", 0.15f }, { "distDrive", 0.3f },
        { "stereoWidth", 1.75f }
    })));

    // 17. Pressure Vent — filtered noise slow open
    out.push_back (makePreset (apvts, cat, "Pressure Vent", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscAPos", 0.5f }, { "oscAUnison", 5 },
        { "oscBTable", WT_Breath }, { "oscBLevel", 0.45f },
        { "noiseType", 2 }, { "noiseLevel", 0.45f },
        { "filtACutoff", 1200.0f }, { "filtAEnvDepth", 0.85f },
        { "ampAttack", 0.05f }, { "ampDecay", 2.5f }, { "ampSustain", 0.7f }, { "ampRelease", 1.5f },
        { "filtAttack", 2.0f }, { "filtDecay", 1.5f }, { "filtSustain", 0.7f },
        { "reverbMix", 0.55f }, { "delayMix", 0.25f },
        { "stereoWidth", 1.75f }
    })));

    // 18. Spectral Fold — wavefold spectral
    out.push_back (makePreset (apvts, cat, "Spectral Fold", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.55f }, { "oscAUnison", 5 },
        { "oscADetune", 0.45f },
        { "oscBTable", WT_SpectralTilt }, { "oscBLevel", 0.5f }, { "oscBCoarse", 7 },
        { "filtACutoff", 2200.0f }, { "filtARes", 0.45f }, { "filtAEnvDepth", 0.55f },
        { "ampAttack", 0.5f }, { "ampDecay", 1.0f }, { "ampSustain", 0.6f }, { "ampRelease", 1.5f },
        { "lfo1Rate", 0.35f },
        { "modSlot0Dst", DST_OscAWarp }, { "modSlot0Amt", 0.55f },
        { "modSlot1Src", SRC_LFO1 }, { "modSlot1Dst", DST_OscAPos }, { "modSlot1Amt", 0.35f }, { "modSlot1On", 1.0f },
        { "reverbMix", 0.6f }, { "delayMix", 0.35f }, { "distMix", 0.15f },
        { "stereoWidth", 1.7f }
    })));

    // 19. Subspace Echo — prism spectrum ping delay
    out.push_back (makePreset (apvts, cat, "Subspace Echo", merge (base, {
        { "oscATable", WT_PrismSpectrum }, { "oscAPos", 0.5f }, { "oscAUnison", 3 },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.45f }, { "oscBCoarse", 19 },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.3f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.8f }, { "ampSustain", 0.4f }, { "ampRelease", 1.5f },
        { "lfo1Rate", 0.5f },
        { "modSlot0Dst", DST_OscAPos }, { "modSlot0Amt", 0.4f },
        { "delayMode", 1 }, { "delaySync", 1 }, { "delayDivision", 4 },
        { "delayFeedback", 0.7f }, { "delayMix", 0.5f }, { "delayTime", 0.5f },
        { "reverbMix", 0.55f }, { "stereoWidth", 1.85f }
    })));

    // 20. Static Bloom — bitcrush reverb wash
    out.push_back (makePreset (apvts, cat, "Static Bloom", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.45f }, { "oscAUnison", 5 },
        { "oscADetune", 0.5f },
        { "oscBTable", WT_FilteredNoise }, { "oscBLevel", 0.4f },
        { "noiseType", 1 }, { "noiseLevel", 0.2f },
        { "filtACutoff", 2200.0f }, { "filtAEnvDepth", 0.4f },
        { "ampAttack", 0.3f }, { "ampDecay", 1.5f }, { "ampSustain", 0.5f }, { "ampRelease", 2.5f },
        { "lfo1Rate", 0.6f },
        { "modSlot0Dst", DST_OscAWarp }, { "modSlot0Amt", 0.5f },
        { "reverbMix", 0.8f }, { "reverbSize", 0.97f }, { "reverbPredelay", 50.0f },
        { "delayMix", 0.3f }, { "stereoWidth", 1.8f }
    })));
}

static void addPercussion (juce::AudioProcessorValueTreeState& apvts,
                           std::vector<Preset>& out)
{
    const juce::String cat = "Percussion";
    auto base = percArchetype();

    out.push_back (makePreset (apvts, cat, "Wavetable Kick", merge (base, {
        { "oscATable", WT_Sine }, { "subLevel", 0.9f }, { "subOctave", 0 },
        { "filtACutoff", 1500.0f }, { "filtAEnvDepth", 0.85f }, { "filtDecay", 0.08f },
        { "ampDecay", 0.18f }, { "ampRelease", 0.06f },
        { "distMix", 0.1f }, { "distDrive", 0.3f }
    })));

    out.push_back (makePreset (apvts, cat, "Noise Snare", merge (base, {
        { "oscATable", WT_Triangle }, { "oscALevel", 0.3f },
        { "noiseType", 0 }, { "noiseLevel", 0.8f }, { "subLevel", 0.2f },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.2f },
        { "ampDecay", 0.15f }
    })));

    out.push_back (makePreset (apvts, cat, "Digital Hit", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.5f },
        { "oscBTable", WT_FMMetallic }, { "oscBLevel", 0.6f },
        { "filtACutoff", 4000.0f }, { "filtADrive", 0.3f },
        { "ampDecay", 0.2f }, { "ampSustain", 0.0f },
        { "distMix", 0.15f }, { "reverbMix", 0.3f }
    })));

    // 4. Sub Boom — sine sub kick
    out.push_back (makePreset (apvts, cat, "Sub Boom", merge (base, {
        { "oscATable", WT_Sine }, { "subLevel", 0.95f }, { "subShape", 0 }, { "subOctave", -1 },
        { "filtACutoff", 800.0f }, { "filtAEnvDepth", 0.9f }, { "filtDecay", 0.05f },
        { "ampDecay", 0.25f }, { "ampRelease", 0.1f },
        { "modSlot0Dst", DST_Pitch }, { "modSlot0Amt", 0.12f },
        { "distMix", 0.05f }
    })));

    // 5. Glass Hat — short filtered noise hat
    out.push_back (makePreset (apvts, cat, "Glass Hat", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscAPos", 0.7f },
        { "oscBLevel", 0.0f },
        { "subLevel", 0.0f },
        { "noiseType", 0 }, { "noiseLevel", 0.7f },
        { "filtAType", 2 }, { "filtACutoff", 6000.0f }, { "filtARes", 0.3f },
        { "ampDecay", 0.06f }, { "ampRelease", 0.04f },
        { "reverbMix", 0.12f }
    })));

    // 6. Wood Knock — plucked string body
    out.push_back (makePreset (apvts, cat, "Wood Knock", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.3f },
        { "oscALevel", 0.7f }, { "subLevel", 0.5f },
        { "noiseLevel", 0.0f },
        { "filtACutoff", 1800.0f }, { "filtAEnvDepth", 0.6f }, { "filtDecay", 0.08f },
        { "ampDecay", 0.12f }, { "ampRelease", 0.05f },
        { "reverbMix", 0.15f }
    })));

    // 7. FM Tom — FM metallic tom
    out.push_back (makePreset (apvts, cat, "FM Tom", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.3f },
        { "oscALevel", 0.85f }, { "subLevel", 0.55f }, { "subShape", 0 },
        { "noiseLevel", 0.0f },
        { "filtACutoff", 1400.0f }, { "filtAEnvDepth", 0.7f }, { "filtDecay", 0.1f },
        { "ampDecay", 0.3f }, { "ampRelease", 0.12f },
        { "modSlot0Dst", DST_Pitch }, { "modSlot0Amt", 0.15f },
        { "reverbMix", 0.18f }
    })));

    // 8. Click Snap — bitcrush + wavefold
    out.push_back (makePreset (apvts, cat, "Click Snap", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.5f },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.5f },
        { "subLevel", 0.0f },
        { "noiseType", 0 }, { "noiseLevel", 0.4f },
        { "filtAType", 2 }, { "filtACutoff", 4000.0f }, { "filtARes", 0.3f },
        { "ampDecay", 0.05f }, { "ampRelease", 0.03f },
        { "reverbMix", 0.1f }
    })));

    // 9. Ride Cymbal — filtered noise + bell tail
    out.push_back (makePreset (apvts, cat, "Ride Cymbal", merge (base, {
        { "oscATable", WT_FilteredNoise }, { "oscAPos", 0.7f },
        { "oscBTable", WT_FMBell }, { "oscBLevel", 0.4f }, { "oscBCoarse", 19 },
        { "subLevel", 0.0f },
        { "noiseType", 0 }, { "noiseLevel", 0.5f },
        { "filtAType", 2 }, { "filtACutoff", 7000.0f }, { "filtARes", 0.25f },
        { "ampDecay", 0.4f }, { "ampSustain", 0.1f }, { "ampRelease", 0.6f },
        { "reverbMix", 0.32f }, { "delayMix", 0.12f }
    })));

    // 10. Bell Tap — short church bell
    out.push_back (makePreset (apvts, cat, "Bell Tap", merge (base, {
        { "oscATable", WT_ChurchBell }, { "oscAPos", 0.3f },
        { "oscALevel", 0.85f }, { "subLevel", 0.0f },
        { "noiseLevel", 0.0f },
        { "filtACutoff", 4500.0f },
        { "ampDecay", 0.4f }, { "ampSustain", 0.0f }, { "ampRelease", 0.3f },
        { "reverbMix", 0.35f }, { "delayMix", 0.12f }
    })));

    // 11. Snare Shimmer — noise + harmonic ring
    out.push_back (makePreset (apvts, cat, "Snare Shimmer", merge (base, {
        { "oscATable", WT_OddHarmonics }, { "oscAPos", 0.5f }, { "oscALevel", 0.5f },
        { "noiseType", 0 }, { "noiseLevel", 0.6f },
        { "subLevel", 0.25f },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.3f },
        { "ampDecay", 0.18f }, { "ampRelease", 0.08f },
        { "reverbMix", 0.22f }
    })));

    // 12. Bongo — warm plucked bongo
    out.push_back (makePreset (apvts, cat, "Bongo", merge (base, {
        { "oscATable", WT_PluckedString }, { "oscAPos", 0.4f }, { "oscALevel", 0.85f },
        { "subLevel", 0.4f }, { "subShape", 0 },
        { "noiseLevel", 0.05f },
        { "filtACutoff", 2000.0f }, { "filtAEnvDepth", 0.5f }, { "filtDecay", 0.1f },
        { "ampDecay", 0.18f }, { "ampRelease", 0.08f },
        { "modSlot0Dst", DST_Pitch }, { "modSlot0Amt", 0.06f },
        { "reverbMix", 0.18f }
    })));

    // 13. Tribal Drum — formant filter low pluck
    out.push_back (makePreset (apvts, cat, "Tribal Drum", merge (base, {
        { "oscATable", WT_FormantFilter }, { "oscAPos", 0.25f }, { "oscALevel", 0.8f },
        { "subLevel", 0.5f }, { "subShape", 1 },
        { "noiseLevel", 0.1f },
        { "filtACutoff", 1500.0f }, { "filtAEnvDepth", 0.6f }, { "filtDecay", 0.12f },
        { "ampDecay", 0.25f }, { "ampRelease", 0.1f },
        { "modSlot0Dst", DST_Pitch }, { "modSlot0Amt", 0.1f },
        { "reverbMix", 0.2f }
    })));

    // 14. Synthetic Conga — FM metallic conga
    out.push_back (makePreset (apvts, cat, "Synthetic Conga", merge (base, {
        { "oscATable", WT_FMMetallic }, { "oscAPos", 0.4f }, { "oscALevel", 0.85f },
        { "subLevel", 0.4f }, { "subShape", 0 },
        { "noiseLevel", 0.0f },
        { "filtACutoff", 2000.0f }, { "filtAEnvDepth", 0.55f }, { "filtDecay", 0.1f },
        { "ampDecay", 0.2f }, { "ampRelease", 0.08f },
        { "modSlot0Dst", DST_Pitch }, { "modSlot0Amt", 0.08f },
        { "reverbMix", 0.18f }
    })));

    // 15. Glitch Hit — bitcrush very short
    out.push_back (makePreset (apvts, cat, "Glitch Hit", merge (base, {
        { "oscATable", WT_Bitcrush }, { "oscAPos", 0.6f }, { "oscALevel", 0.7f },
        { "oscBTable", WT_Wavefold }, { "oscBLevel", 0.5f }, { "oscBCoarse", 7 },
        { "subLevel", 0.0f },
        { "noiseLevel", 0.0f },
        { "filtACutoff", 3500.0f }, { "filtARes", 0.4f }, { "filtADrive", 0.4f },
        { "ampDecay", 0.06f }, { "ampRelease", 0.04f },
        { "distMix", 0.18f }, { "distDrive", 0.4f }
    })));

    // 16. Wind Burst — short wind hit
    out.push_back (makePreset (apvts, cat, "Wind Burst", merge (base, {
        { "oscATable", WT_Wind }, { "oscAPos", 0.5f }, { "oscALevel", 0.7f },
        { "subLevel", 0.0f },
        { "noiseType", 2 }, { "noiseLevel", 0.4f },
        { "filtACutoff", 2500.0f }, { "filtAEnvDepth", 0.7f }, { "filtDecay", 0.12f },
        { "ampAttack", 0.005f }, { "ampDecay", 0.15f }, { "ampRelease", 0.08f },
        { "reverbMix", 0.25f }
    })));

    // 17. Tom Floor — sine sub floor tom
    out.push_back (makePreset (apvts, cat, "Tom Floor", merge (base, {
        { "oscATable", WT_Sine }, { "subLevel", 0.85f }, { "subShape", 0 }, { "subOctave", -1 },
        { "noiseLevel", 0.05f },
        { "filtACutoff", 700.0f }, { "filtAEnvDepth", 0.85f }, { "filtDecay", 0.1f },
        { "ampDecay", 0.5f }, { "ampRelease", 0.18f },
        { "modSlot0Dst", DST_Pitch }, { "modSlot0Amt", 0.18f },
        { "distMix", 0.05f }
    })));

    // 18. Synth Clave — wavefold short tone
    out.push_back (makePreset (apvts, cat, "Synth Clave", merge (base, {
        { "oscATable", WT_Wavefold }, { "oscAPos", 0.4f }, { "oscALevel", 0.85f },
        { "oscBTable", WT_Triangle }, { "oscBLevel", 0.4f }, { "oscBCoarse", 12 },
        { "subLevel", 0.0f },
        { "noiseLevel", 0.0f },
        { "filtACutoff", 3000.0f }, { "filtARes", 0.3f },
        { "ampDecay", 0.07f }, { "ampRelease", 0.04f },
        { "reverbMix", 0.18f }
    })));
}

} // namespace (anonymous)

// ═══════════════════════════════════════════════════════════════════
// FactoryPresets::build — public entry point
// ═══════════════════════════════════════════════════════════════════

std::vector<Preset>
FactoryPresets::build (juce::AudioProcessorValueTreeState& apvts)
{
    std::vector<Preset> out;
    out.reserve (96);

    addPads       (apvts, out);  // 18
    addLeads      (apvts, out);  // 15
    addDrones     (apvts, out);  // 12
    addSeq        (apvts, out);  // 12
    addBass       (apvts, out);  // 10
    addPlucks     (apvts, out);  // 10
    addKeys       (apvts, out);  //  8
    addFX         (apvts, out);  //  8
    addPercussion (apvts, out);  //  3

    jassert (out.size() == 96);
    return out;
}
