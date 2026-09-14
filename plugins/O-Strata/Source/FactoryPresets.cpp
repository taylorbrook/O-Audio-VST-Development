/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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
    O-Strata - Factory preset library: the 18-preset bank (Stage 4 Round A,
    plan Decisions 17-19; content = stages/4-polish/RESEARCH.md §5.1).

    Authoring pattern (O-Prism's, verbatim): RawMap of ENGINEERING-unit overrides
    -> merge (completeBase(), raw) -> normalize (convertTo0to1 per ID) -> makePreset.
    Every preset passes H2 at C4 on oscillator A (and B where present) with the
    measured worst h1-max figure recorded on its row (scratch harness, 2026-09-12).

    Coverage: Sine Product 1, 2, 12, 17 · Radial Rings 7, 13, 14 · Saddle 6, 10, 18 ·
    Ridged Cosines 3, 8 · Mitsuhashi 5, 11, 15 · Cosine Wells 4, 9, 16; all eleven
    orbits; feedback 3, 8, 13; Bandlimited 4, 9, 14; BRIEF use cases 1-5 = presets 2-6.

  ==============================================================================
*/

#include "FactoryPresets.h"
#include <map>

namespace
{
using Preset = OuariconPresetManager::FactoryPresetDef;
using RawMap = std::map<juce::String, float>;

// ─── Choice indices (asserted against the live lists by tests/render-harness/main.cpp
//     [index] and the static_asserts in PluginProcessor.cpp; ModulationMatrix.h enums) ───
enum : int { TER_SineProduct = 0, TER_RadialRings = 1, TER_Saddle = 2, TER_RidgedCosines = 3, TER_Mitsuhashi = 4, TER_CosineWells = 5 };
enum : int { ORB_Ellipse = 0, ORB_Superellipse = 1, ORB_Limacon = 2, ORB_Epi3 = 3, ORB_Epi5 = 4, ORB_Epi7 = 5,
             ORB_Hypo3 = 6, ORB_Hypo5 = 7, ORB_Hypo7 = 8, ORB_Butterfly = 9, ORB_Squarcle = 10 };
enum : int { Q_Bandlimited = 0, Q_2x = 1, Q_4x = 2 };
enum : int { SRC_None = 0, SRC_LFO1 = 1, SRC_LFO2 = 2, SRC_LFO3 = 3, SRC_LFO4 = 4,
             SRC_AmpEnv = 5, SRC_FiltEnv = 6, SRC_Vel = 7, SRC_Note = 8, SRC_Wheel = 9, SRC_AT = 10 };
enum : int { DST_OscASize = 1, DST_FiltACut = 3, DST_Pitch = 23,
             DST_OscAAspect = 26, DST_OscARot = 27, DST_OscACX = 28, DST_OscACY = 29, DST_OscAOrbMod = 30,
             DST_OscATerFreq = 31, DST_OscAModX = 32, DST_OscAModY = 33, DST_OscAFeedback = 34, DST_OscASat = 35 };
enum : int { FILT_LP24 = 1 };
enum : int { SUB_Minus1Oct = 0 };
enum : int { LFO_SH = 4 };
enum : int { DELAY_PingPong = 1 };
enum : int { DIST_SoftClip = 0 };

// Tuning parameters the manager never writes (mirrors OStrataAudioProcessor's
// presetManager.excludedParameterIds — 205 - 7 = 198 keys per file, smoke [5]).
const juce::StringArray kExcluded { "tuningPreset", "tonic", "masterTune", "octaveStretch", "pitchBendRange", "glideMode", "glideTime" };

// ─── Helpers ──────────────────────────────────────────────────────────

/** Merge `overrides` into `base`, overriding any duplicate keys. */
RawMap merge (RawMap base, const RawMap& overrides)
{
    for (const auto& [k, v] : overrides)
        base[k] = v;
    return base;
}

/** Every non-excluded parameter at its default, in ENGINEERING units (convertFrom0to1
    of getDefaultValue()) — so Init and the base of every preset agree by construction. */
RawMap completeBase (juce::AudioProcessorValueTreeState& apvts)
{
    RawMap m;
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! kExcluded.contains (rp->getParameterID()))
                m[rp->getParameterID()] = rp->convertFrom0to1 (rp->getDefaultValue());
    return m;
}

/** Convert engineering values to APVTS-normalised [0,1]; an unknown ID is asserted and skipped. */
std::map<juce::String, float> normalize (juce::AudioProcessorValueTreeState& apvts, const RawMap& raw)
{
    std::map<juce::String, float> out;
    for (const auto& [id, value] : raw)
    {
        if (auto* p = apvts.getParameter (id))
        {
            out[id] = p->convertTo0to1 (value);
        }
        else
        {
            DBG ("FactoryPresets: unknown parameter ID '" << id << "' - skipped");
            jassertfalse;
        }
    }
    return out;
}

/** Index of `label` in an AudioParameterChoice's list (delay / LFO divisions are authored by name). */
float choiceIndex (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& label)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
    {
        const int i = c->choices.indexOf (label);
        jassert (i >= 0);
        if (i >= 0) return static_cast<float> (i);
    }
    DBG ("FactoryPresets: choice '" << label << "' not found on '" << id << "'");
    jassertfalse;
    return 0.0f;
}

Preset makePreset (juce::AudioProcessorValueTreeState& apvts, const juce::String& category,
                   const juce::String& name, const RawMap& raw)
{
    Preset def;
    def.category = category;
    def.name = name;
    def.parameters = normalize (apvts, merge (completeBase (apvts), raw));
    return def;
}

/** One mod-matrix slot: src / dst (enum ints), amount [-1..1], enabled. */
RawMap modSlot (int slot, int src, int dst, float amt)
{
    const auto n = juce::String (slot);
    return { { "modSlot" + n + "Src", static_cast<float> (src) },
             { "modSlot" + n + "Dst", static_cast<float> (dst) },
             { "modSlot" + n + "Amt", amt },
             { "modSlot" + n + "On",  1.0f } };
}

RawMap amp (float a, float d, float s, float r)
{
    return { { "ampAttack", a }, { "ampDecay", d }, { "ampSustain", s }, { "ampRelease", r } };
}

RawMap lp24 (float cutoffHz, float res = 0.0f, float keyTrack = 0.0f, float envDepth = 0.0f)
{
    return { { "filtAType", static_cast<float> (FILT_LP24) }, { "filtACutoff", cutoffHz },
             { "filtARes", res }, { "filtAKeyTrack", keyTrack }, { "filtAEnvDepth", envDepth } };
}

RawMap oscA (int terrain, int orbit, float F)
{
    return { { "oscATerrain", static_cast<float> (terrain) }, { "oscAOrbit", static_cast<float> (orbit) }, { "oscATerFreq", F } };
}
} // namespace

// ─── The bank ─────────────────────────────────────────────────────────

std::vector<OuariconPresetManager::FactoryPresetDef>
FactoryPresets::build (juce::AudioProcessorValueTreeState& apvts)
{
    auto div = [&apvts] (const char* id, const char* label) { return choiceIndex (apvts, id, label); };
    std::vector<Preset> bank;

    // 1  Init — every default (Init, SP x Ellipse, F 1.0). H2 0.0 dB / 100 %.
    bank.push_back (makePreset (apvts, "Init", "Init", {}));

    // 2  Breathing Pad (use case 1) — SP x Ellipse, F 1.0; LFO1 0.08 Hz -> Orbit Size +0.35,
    //    LFO2 0.05 Hz -> Centre X +0.15 (<= 0.15: never through (0, y) with CY through 0);
    //    osc B SP x Ellipse level 0.5, Fine +6 c, centre (-0.17, 0.24). H2 -1.9 dB / 100 %.
    bank.push_back (makePreset (apvts, "Pads", "Breathing Pad", merge (merge (merge (
        oscA (TER_SineProduct, ORB_Ellipse, 1.0f),
        { { "lfo1Rate", 0.08f }, { "lfo2Rate", 0.05f },
          { "oscBTerrain", static_cast<float> (TER_SineProduct) }, { "oscBOrbit", static_cast<float> (ORB_Ellipse) },
          { "oscBLevel", 0.5f }, { "oscBFine", 6.0f }, { "oscBOrbCX", -0.17f }, { "oscBOrbCY", 0.24f },
          { "chorusBypass", 0.0f }, { "chorusMix", 0.25f },
          { "reverbBypass", 0.0f }, { "reverbMix", 0.35f }, { "reverbSize", 0.7f } }),
        merge (modSlot (0, SRC_LFO1, DST_OscASize, 0.35f), modSlot (1, SRC_LFO2, DST_OscACX, 0.15f))),
        merge (amp (1.5f, 1.0f, 0.9f, 3.0f), lp24 (4000.0f)))));

    // 3  Chatter Lead (use case 2) — RC x Epitrochoid 5, F 1.0; Feedback 0.4 / Damp 0.3;
    //    ModWheel -> Terrain Freq +0.4; delay 1/8D. H2 0.0 dB / 100 %.
    bank.push_back (makePreset (apvts, "Lead", "Chatter Lead", merge (merge (merge (
        oscA (TER_RidgedCosines, ORB_Epi5, 1.0f),
        { { "oscAOrbFeedback", 0.4f }, { "oscAOrbFbDamp", 0.3f },
          { "delayBypass", 0.0f }, { "delaySync", 1.0f }, { "delayDivision", div ("delayDivision", "1/8D") },
          { "delayMix", 0.2f }, { "delayFeedback", 0.35f } }),
        modSlot (0, SRC_Wheel, DST_OscATerFreq, 0.4f)),
        merge (amp (0.005f, 0.3f, 0.8f, 0.3f), lp24 (8000.0f, 0.2f)))));

    // 4  Pierce Bell (use case 3) — CW x Hypocycloid 3, F 1.5, BANDLIMITED; Tuning tab -> Bohlen-Pierce
    //    (tuning is never in a preset). H2 0.0 dB / 100 %.
    bank.push_back (makePreset (apvts, "Keys", "Pierce Bell", merge (merge (
        oscA (TER_CosineWells, ORB_Hypo3, 1.5f),
        { { "oscAQuality", static_cast<float> (Q_Bandlimited) },
          { "reverbBypass", 0.0f }, { "reverbMix", 0.3f }, { "reverbSize", 0.6f } }),
        merge (amp (0.002f, 2.5f, 0.0f, 2.5f), lp24 (12000.0f)))));

    // 5  Your Terrain Here (use case 4) — Mitsuhashi x Limacon, F 1.0; Blur 0.2 / Mirror / Orbit Mod 0.5:
    //    drop a greyscale PNG on the terrain view — a saved User preset keeps it. H2 0.0 dB / 100 %.
    bank.push_back (makePreset (apvts, "FX", "Your Terrain Here", merge (
        oscA (TER_Mitsuhashi, ORB_Limacon, 1.0f),
        { { "oscATerBlur", 0.2f }, { "oscATerEdge", 0.0f }, { "oscAOrbMod", 0.5f } })));

    // 6  PD Organ (use case 5) — Saddle x Squarcle, F 1.0; FilterEnv -> Rotation +0.5 (a 90 deg sweep),
    //    filt env 0.01 / 0.4 / 0.3 / 0.5. H2 -3.3 dB / 100 %.
    bank.push_back (makePreset (apvts, "Keys", "PD Organ", merge (merge (merge (
        oscA (TER_Saddle, ORB_Squarcle, 1.0f),
        { { "filtAttack", 0.01f }, { "filtDecay", 0.4f }, { "filtSustain", 0.3f }, { "filtRelease", 0.5f },
          { "chorusBypass", 0.0f }, { "chorusMix", 0.3f }, { "chorusRate", 0.8f } }),
        modSlot (0, SRC_FiltEnv, DST_OscARot, 0.5f)),
        amp (0.005f, 0.3f, 1.0f, 0.08f))));

    // 7  Radial Keys — RR x Superellipse, F 1.5; Saturation 0.2; LP24 6 kHz keytrack 0.5 env 0.3. H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Keys", "Radial Keys", merge (merge (
        oscA (TER_RadialRings, ORB_Superellipse, 1.5f),
        { { "oscATerSat", 0.2f } }),
        merge (amp (0.003f, 1.2f, 0.4f, 0.6f), lp24 (6000.0f, 0.0f, 0.5f, 0.3f)))));

    // 8  Folded Bass — RC x Hypocycloid 5, F 0.7; Feedback 0.25 / Damp 0.6; Coarse -12; sub -1 oct 0.3;
    //    LP24 1.2 kHz env 0.4, filter decay 0.3. H2 0.0 dB / 100 % at the played pitch (Coarse-corrected f0).
    bank.push_back (makePreset (apvts, "Bass", "Folded Bass", merge (merge (
        oscA (TER_RidgedCosines, ORB_Hypo5, 0.7f),
        { { "oscAOrbFeedback", 0.25f }, { "oscAOrbFbDamp", 0.6f }, { "oscACoarse", -12.0f },
          { "subOctave", static_cast<float> (SUB_Minus1Oct) }, { "subLevel", 0.3f }, { "filtDecay", 0.3f } }),
        merge (amp (0.003f, 0.4f, 0.7f, 0.2f), lp24 (1200.0f, 0.0f, 0.0f, 0.4f)))));

    // 9  Wells Drone — CW x Epitrochoid 7, F 2.0, BANDLIMITED; LFO1 0.03 Hz -> Mod Y +0.3;
    //    osc B CW x Ellipse level 0.4 Coarse -12. H2 0.0 dB / 100 % (A and B rows).
    bank.push_back (makePreset (apvts, "Drone", "Wells Drone", merge (merge (merge (
        oscA (TER_CosineWells, ORB_Epi7, 2.0f),
        { { "oscAQuality", static_cast<float> (Q_Bandlimited) }, { "lfo1Rate", 0.03f },
          { "oscBTerrain", static_cast<float> (TER_CosineWells) }, { "oscBOrbit", static_cast<float> (ORB_Ellipse) },
          { "oscBLevel", 0.4f }, { "oscBCoarse", -12.0f },
          { "reverbBypass", 0.0f }, { "reverbMix", 0.45f }, { "reverbSize", 0.85f } }),
        modSlot (0, SRC_LFO1, DST_OscAModY, 0.3f)),
        amp (3.0f, 1.0f, 1.0f, 4.0f))));

    // 10 Saddle Pluck — Saddle x Hypocycloid 7, F 3.0; LP24 3 kHz env 0.6 decay 0.25; Velocity -> Cutoff +0.3. H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Pluck", "Saddle Pluck", merge (merge (merge (
        oscA (TER_Saddle, ORB_Hypo7, 3.0f),
        { { "filtDecay", 0.25f } }),
        modSlot (0, SRC_Vel, DST_FiltACut, 0.3f)),
        merge (amp (0.001f, 0.5f, 0.0f, 0.4f), lp24 (3000.0f, 0.0f, 0.0f, 0.6f)))));

    // 11 Butterfly Choir — Mitsuhashi x Butterfly, F 1.0; Unison 3 Detune 0.3 Width 0.8. H2 -4.5 dB / 100 % (closest to -6).
    bank.push_back (makePreset (apvts, "Pads", "Butterfly Choir", merge (merge (
        oscA (TER_Mitsuhashi, ORB_Butterfly, 1.0f),
        { { "oscAUnison", 3.0f }, { "oscADetune", 0.3f }, { "oscAWidth", 0.8f },
          { "chorusBypass", 0.0f }, { "chorusMix", 0.35f }, { "reverbBypass", 0.0f }, { "reverbMix", 0.4f } }),
        amp (1.2f, 1.0f, 0.9f, 2.5f))));

    // 12 Limacon Lead — SP x Limacon, F 2.0; Orbit Mod 0.7, Saturation 0.4; soft-clip drive 0.3 mix 0.4; delay 1/4. H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Lead", "Limacon Lead", merge (merge (
        oscA (TER_SineProduct, ORB_Limacon, 2.0f),
        { { "oscAOrbMod", 0.7f }, { "oscATerSat", 0.4f },
          { "distBypass", 0.0f }, { "distType", static_cast<float> (DIST_SoftClip) }, { "distDrive", 0.3f }, { "distMix", 0.4f },
          { "delayBypass", 0.0f }, { "delaySync", 1.0f }, { "delayDivision", div ("delayDivision", "1/4") }, { "delayMix", 0.15f } }),
        amp (0.01f, 0.3f, 0.9f, 0.25f))));

    // 13 Squarcle Storm — RR x Squarcle, F 1.0; Feedback 0.6 / Damp 0.2; CENTRE (0.23, 0.21) — the default
    //    (0.13, 0.21) reads -6.9 dB / 0 % (the trajectory drags through the origin), +0.1 in X restores h1;
    //    LFO1 S&H 4 Hz -> Centre Y +0.05; ping-pong delay 0.3; soft clip 0.3. H2 -3.6 dB / 100 %
    //    (bit-stable since Stage 4 Round B Decision 32 — the S&H LFO draws from the harness phase
    //    seed; Round A's -2.2 ... -4.0 dB spread was the clock-seeded juce::Random).
    bank.push_back (makePreset (apvts, "FX", "Squarcle Storm", merge (merge (
        oscA (TER_RadialRings, ORB_Squarcle, 1.0f),
        { { "oscAOrbFeedback", 0.6f }, { "oscAOrbFbDamp", 0.2f }, { "oscAOrbCX", 0.23f }, { "oscAOrbCY", 0.21f },
          { "lfo1Shape", static_cast<float> (LFO_SH) }, { "lfo1Rate", 4.0f },
          { "delayBypass", 0.0f }, { "delayMode", static_cast<float> (DELAY_PingPong) }, { "delayMix", 0.3f },
          { "distBypass", 0.0f }, { "distType", static_cast<float> (DIST_SoftClip) }, { "distDrive", 0.3f }, { "distMix", 0.3f } }),
        modSlot (0, SRC_LFO1, DST_OscACY, 0.05f))));

    // 14 Glass Rings — RR x Hypocycloid 3, F 2.5, BANDLIMITED; reverb 0.35. H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Keys", "Glass Rings", merge (merge (
        oscA (TER_RadialRings, ORB_Hypo3, 2.5f),
        { { "oscAQuality", static_cast<float> (Q_Bandlimited) }, { "reverbBypass", 0.0f }, { "reverbMix", 0.35f } }),
        amp (0.002f, 1.8f, 0.0f, 1.8f))));

    // 15 Mitsuhashi Organ — Mitsuhashi x Ellipse, F 1.0; CENTRE (0, 0), Aspect 1.0 — the deliberate centred case
    //    (Mitsuhashi is odd under (x, y) -> (-x, -y): odd harmonics, h1 present). H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Keys", "Mitsuhashi Organ", merge (merge (
        oscA (TER_Mitsuhashi, ORB_Ellipse, 1.0f),
        { { "oscAOrbCX", 0.0f }, { "oscAOrbCY", 0.0f }, { "oscAOrbAspect", 1.0f },
          { "chorusBypass", 0.0f }, { "chorusMix", 0.2f } }),
        amp (0.005f, 0.3f, 1.0f, 0.08f))));

    // 16 Superellipse Bass — CW x Superellipse, F 1.0; Coarse -12; Orbit Mod 0.3; LP24 900 Hz env 0.5; sub 0.25. H2 0.0 / 100 at pitch.
    bank.push_back (makePreset (apvts, "Bass", "Superellipse Bass", merge (merge (
        oscA (TER_CosineWells, ORB_Superellipse, 1.0f),
        { { "oscACoarse", -12.0f }, { "oscAOrbMod", 0.3f },
          { "subOctave", static_cast<float> (SUB_Minus1Oct) }, { "subLevel", 0.25f } }),
        lp24 (900.0f, 0.0f, 0.0f, 0.5f))));

    // 17 Epi Keys — SP x Epitrochoid 3, F 1.5; Orbit Mod 0.6; Velocity -> Orbit Mod +0.3; chorus 0.2. H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Keys", "Epi Keys", merge (merge (merge (
        oscA (TER_SineProduct, ORB_Epi3, 1.5f),
        { { "oscAOrbMod", 0.6f }, { "chorusBypass", 0.0f }, { "chorusMix", 0.2f } }),
        modSlot (0, SRC_Vel, DST_OscAOrbMod, 0.3f)),
        amp (0.003f, 1.5f, 0.3f, 0.5f))));

    // 18 Saddle Sweep — Saddle x Epitrochoid 7, F 1.0; LFO1 0.2 Hz (sync 1/2) -> Terrain Freq +0.3; delay 1/8. H2 0.0 / 100.
    bank.push_back (makePreset (apvts, "Sequence", "Saddle Sweep", merge (merge (merge (
        oscA (TER_Saddle, ORB_Epi7, 1.0f),
        { { "lfo1Rate", 0.2f }, { "lfo1Sync", 1.0f }, { "lfo1Division", div ("lfo1Division", "1/2") },
          { "delayBypass", 0.0f }, { "delaySync", 1.0f }, { "delayDivision", div ("delayDivision", "1/8") }, { "delayMix", 0.25f } }),
        modSlot (0, SRC_LFO1, DST_OscATerFreq, 0.3f)),
        amp (0.005f, 0.3f, 0.5f, 0.2f))));

    return bank;
}

juce::String FactoryPresets::stamp (const std::vector<OuariconPresetManager::FactoryPresetDef>& defs)
{
    juce::MemoryOutputStream canonical;
    for (const auto& def : defs)
    {
        canonical.writeString (def.category);
        canonical.writeString (def.name);
        for (const auto& [id, value] : def.parameters)
        {
            canonical.writeString (id);
            canonical.writeFloat (value);   // the 4 IEEE bytes, little-endian
        }
    }
    return juce::String (JucePlugin_VersionString) + "+"
         + juce::SHA256 (canonical.getData(), canonical.getDataSize()).toHexString().substring (0, 12);
}
