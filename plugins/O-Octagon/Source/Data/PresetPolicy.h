/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

// For oo::params::kCount. The partition assert below must count against the SINGLE parameter-count
// source of truth, not against a literal 17 mirrored here
// (pattern_test_fixture_mirrors_drift_silently).
#include "../DSP/GainStage.h"
#include "../DSP/DbapSolver.h"   // blurFractionLinearToSquare — the v1.13.0 preset migration

#include "OuariconPresetManager.h"

namespace oo
{

//==============================================================================
/**
    THE FACTORY PRESETS, AND THE ONE CALL SITE THAT LOADS THEM SAFELY (PLAN-4.1 P92, P93, P96).

    ── Why this is a header and not PluginEditor.cpp (P92) ───────────────────────────────────────

    The natural home would be beside the editor-owned OuariconPresetManager. It cannot be:
    PluginEditor.cpp is permanently excluded from the render harness (that target compiles with
    JUCE_WEB_BROWSER=0 and its CMakeLists forbids the TU by name), so definitions written there are
    UNREACHABLE BY ANY PROBE. The preset work is the one place in Phase 4.1 where a green result can
    be wrong, so the rule has to sit where a probe can call it.

    The harness already puts modules/persistence/preset-manager/cpp on its include path — probes BW
    and BX drive OuariconPresetManager directly for FUNC-05 — so the scaffold exists.

    CONSTRAINT: the GEOMETRY UNIT target must never include this header. It needs
    juce_audio_processors, and that target's narrow link line is a structural property gate 11
    protects. RigPolicy.h is the header that may cross into the unit target; this one may not.
*/
namespace presets
{

//==============================================================================
/** The THIRTEEN parameters a preset load never writes — the SOURCE, the SCENE, the MATERIAL and,
    since v1.14.0, THE HALL'S MASTER LEVEL.

    Not a stylistic list: it is the complement of the fifteen a preset writes, and 13 + 15 = 28 is
    asserted below against oo::params::kCount so the two halves cannot drift apart.

    ── WHY v1.5.0's `decorr` IS PRESERVED AND NOT AUTHORED (the one judgement call in the list) ──

    Every other entry here is preserved because it describes WHERE the source is or WHICH speakers
    are live. `decorr` is preserved because it describes THE MATERIAL: it is the answer to "is this
    stem effectively mono?", which is a fact about the file on the track, not about the room the
    preset is painting. Wide Hall does not know whether you fed it a mono stem or a decorrelated
    stereo pair, and a preset that flipped the setting either way would be guessing.

    ── v1.14.0 (B5) — `outputGain` MOVED HERE FROM kAuthored ────────────────────────────────────

    The suite rule: presets never set output gain, and a load must not touch it. Through v1.13.2
    three factory rows authored -1.5 / -3 / -2 dB and every other load reset it to 0 dB, so an
    operator at -9 dB for the PA who loaded Concert Default got +9 dB mid-show. It is the level the
    rig was calibrated at — a fact about the hall, never about the room character. A stored user
    preset that still carries the key is IGNORED for it, by the same rule as every other entry.
*/
inline constexpr std::array<const char*, 13> kPreserved
    = { "srcX", "srcY", "srcZ", "decorr",
        "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
        "outputGain" };

/** The five a factory preset writes for ROOM CHARACTER, and nothing that says where the source is
    or how loud the hall is. */
/** ...plus, since v1.8.0, THE TEN MOTION PARAMETERS. Authored, not preserved (PLAN overrides
    RESEARCH here): R7 says motion state travels in presets. The six original factory rows write
    nothing for them, so a load lands motionOn at its default (0) on every one of those and they
    stay audibly identical to v1.7.0 (probe CP). A pre-1.8.0 user preset omits the keys, which
    lands them the same way — motion off (probe DK). */
inline constexpr std::array<const char*, 15> kAuthored
    = { "width", "rolloff", "blur", "hullAtten", "airAmount",
        "motionOn", "motionPath", "motionSync", "motionRate", "motionSize",
        "motionRatio", "motionAngle", "motionHeight", "motionPhase", "motionSeed" };

static_assert (kPreserved.size() + kAuthored.size() == params::kCount,
               "The preserved set and the authored set PARTITION the parameters. If this fires, a "
               "parameter was added and belongs in exactly one of the two lists — leaving it out of "
               "BOTH means a preset load never touches it AND no preset can carry it, and leaving "
               "it in kAuthored when it describes the source, the scene or the hall means a load "
               "silently moves it.");

//==============================================================================
/**
    The six presets, in ENGINEERING UNITS.

    | Preset          | width m | rolloff dB/2x | blur | hullAtten dB/m | airAmount |
    |-----------------|---------|---------------|------|----------------|-----------|
    | Dry Point       |   0.0   |      6.0      | 0.00 |      2.2       |   0.00    |
    | Concert Default |   0.0   |      4.0      | 0.10 |      1.0       |   0.35    |
    | Chamber         |   1.5   |      4.5      | 0.18 |      1.4       |   0.25    |
    | Wide Hall       |   3.0   |      3.5      | 0.35 |      0.7       |   0.55    |
    | Distant Field   |   4.5   |      3.0      | 0.55 |      0.4       |   0.85    |
    | Enveloping      |   6.0   |      3.0      | 0.80 |      0.0       |   0.45    |

    v1.14.0 (B5) — the outputGain column (0 / 0 / 0 / -1.5 / -3 / -2 dB) is GONE. Output gain is
    the hall's calibration level and sits in kPreserved; no preset writes it.

    ── The axis is a constraint, not a description ───────────────────────────────────────────────

    rolloff falls 6.0 -> 3.0 while blur rises 0.00 -> 0.80, and hullAtten falls with them, because a
    diffuse image under a steep outside-hull trim fights itself.

    CONCERT DEFAULT'S FIVE ARE EXACTLY THE SHIPPED DEFAULTS (verified against
    createParameterLayout()), which is what makes it "back to neutral in one click" rather than a
    taste.

    ── Why six and not five ──────────────────────────────────────────────────────────────────────

    The sixth is what puts hullAtten = 0 on a user-reachable path. Two presets sit deliberately on
    EXACT-NO-OP branches — airAmount = 0 (bit-transparent by construction) and hullAtten = 0
    (bit-exact unity, probe AV over 201 swept distances) — so the factory set doubles as a
    reachability check on both, exercised from where a user actually goes. Drop Enveloping and
    hullAtten = 0 is reachable only by dragging a slider to its endpoint.

    ── Conversion is MANDATORY, and never by hand (P96) ──────────────────────────────────────────

    FactoryPresetDef::parameters holds NORMALISED values, so every number above goes through the
    LIVE NormalisableRange read off the APVTS. All 17 skews are linear today, so the arithmetic is
    one subtract and one divide and it is tempting to write the fraction instead. DO NOT. Writing
    0.75f for rolloff = 5.25 bakes the 3.0-6.0 range in forever and silently
    (pattern_factory_preset_normalized_ignores_skew,
    critical_apvts_denormalised_vs_preset_normalised).

    NOTE FOR THE READER OF A .json: presets store blur (0-1), NOT metres, so they are
    venue-portable by construction — on a rig with a different rigScale the same blur gives a
    proportionally different radius. Correct behaviour, and not drift.
*/
inline std::vector<OuariconPresetManager::FactoryPresetDef>
factoryDefs (const juce::AudioProcessorValueTreeState& apvts)
{
    // v1.8.0 — the ten motion fields, DEFAULTED OFF so the six original rows are written exactly
    // as before: they name no motion value, WR-01's reset lands motionOn = 0 on their load, and
    // probe CP holds them audibly identical to v1.7.0. The two new rows below DO set motion.
    struct Motion
    {
        float on { 0.0f }, path { 0.0f }, sync { 0.0f }, rate { 0.1f }, size { 6.0f },
              ratio { 1.0f }, angle { 0.0f }, height { 0.0f }, phase { 0.0f }, seed { 1.0f };
    };

    struct Row
    {
        const char* name;
        float width, rolloff, blur, hullAtten, airAmount;
        Motion motion {};
    };

    // v1.3.0 — blur column rescaled ÷3 (rounded to 2 dp): kBlurScale tripled (0.5 → 1.5), and
    // these rows were AUTHORED as radii, not knob positions — the sound each preset ships is the
    // point, so the radius is what survives the rescale. Width and rolloff columns stay put: they
    // are engineering values and the live-range conversion below moves them onto the widened
    // ranges untouched. Concert Default's blur must remain EXACTLY the shipped default.
    //
    // v1.13.0 — blur column re-mapped blur → ½√blur (rounded to 2 dp) for the square law with
    // kBlurScale 6: the same radii again (0.36 → 0.39, 0.71 → 0.69, 1.43 → 1.38, 2.14 → 2.10,
    // 3.21 → 3.22 m — every one under 0.1 dB of spread). Concert Default = the shipped 0.09.
    static constexpr Row rows[] = {
        //  name              width  rolloff  blur  hullAtten  air
        { "Dry Point",         0.0f,   6.0f,  0.00f,    2.2f, 0.00f },
        { "Concert Default",   0.0f,   4.0f,  0.09f,    1.0f, 0.35f },
        { "Chamber",           1.5f,   4.5f,  0.12f,    1.4f, 0.25f },
        { "Wide Hall",         3.0f,   3.5f,  0.17f,    0.7f, 0.55f },
        { "Distant Field",     4.5f,   3.0f,  0.21f,    0.4f, 0.85f },
        { "Enveloping",        6.0f,   3.0f,  0.26f,    0.0f, 0.45f },

        // v1.8.0 — TWO ROWS THAT MOVE. Room character from Wide Hall / Chamber respectively, so
        // the motion is what distinguishes them.
        //   Slow Orbit: Orbit (0), synced to 4 Bars (14), 8 m across, ratio 0.8, 1 m of height.
        //   Wander:     Drift (3), Free (0) at 0.05 Hz, 6 m, seed 7.
        //                                                     on  path sync rate  size ratio ang  hgt  phase seed
        { "Slow Orbit",        3.0f,   3.5f,  0.17f,    0.7f, 0.55f, { 1.0f, 0.0f, 14.0f, 0.1f,  8.0f, 0.8f, 0.0f, 1.0f, 0.0f, 1.0f } },
        { "Wander",            1.5f,   4.5f,  0.12f,    1.4f, 0.25f, { 1.0f, 3.0f,  0.0f, 0.05f, 6.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f } },
    };

    // The single conversion. Reads the LIVE range, so a range change moves the presets with it
    // rather than leaving them pointing at an old one.
    const auto norm = [&apvts] (const char* id, float engineeringValue)
    {
        return apvts.getParameterRange (id).convertTo0to1 (engineeringValue);
    };

    std::vector<OuariconPresetManager::FactoryPresetDef> defs;
    defs.reserve (std::size (rows));

    for (const auto& r : rows)
    {
        OuariconPresetManager::FactoryPresetDef def;
        def.name = r.name;

        def.parameters["width"]      = norm ("width",      r.width);
        def.parameters["rolloff"]    = norm ("rolloff",    r.rolloff);
        def.parameters["blur"]       = norm ("blur",       r.blur);
        def.parameters["hullAtten"]  = norm ("hullAtten",  r.hullAtten);
        def.parameters["airAmount"]  = norm ("airAmount",  r.airAmount);

        // v1.8.0 — the ten motion keys, through the SAME live-range conversion. A Bool's range is
        // 0..1 and a Choice's is 0..N-1 in engineering terms, so norm() handles all three kinds;
        // motionRate is the first skewed range and this is exactly the case P96 exists for.
        def.parameters["motionOn"]     = norm ("motionOn",     r.motion.on);
        def.parameters["motionPath"]   = norm ("motionPath",   r.motion.path);
        def.parameters["motionSync"]   = norm ("motionSync",   r.motion.sync);
        def.parameters["motionRate"]   = norm ("motionRate",   r.motion.rate);
        def.parameters["motionSize"]   = norm ("motionSize",   r.motion.size);
        def.parameters["motionRatio"]  = norm ("motionRatio",  r.motion.ratio);
        def.parameters["motionAngle"]  = norm ("motionAngle",  r.motion.angle);
        def.parameters["motionHeight"] = norm ("motionHeight", r.motion.height);
        def.parameters["motionPhase"]  = norm ("motionPhase",  r.motion.phase);
        def.parameters["motionSeed"]   = norm ("motionSeed",   r.motion.seed);

        defs.push_back (std::move (def));
    }

    return defs;
}

//==============================================================================
/** The module's filename rule, mirrored: OuariconPresetManager::sanitizePresetName is private, and
    a load that resolved a different file from the one savePreset wrote would be a quiet miss. */
inline juce::String fileSafeName (const juce::String& presetName)
{
    return presetName.trim().replaceCharacters ("/\\:", "___");
}

/** Factory first, then User — the module's own lookup order, so a user file can never shadow a
    factory name (savePreset refuses to write one anyway). */
inline juce::File presetFile (const OuariconPresetManager& manager, const juce::String& presetName)
{
    const auto fileName = fileSafeName (presetName) + ".json";
    const auto factory  = manager.getFactoryPresetsDirectory().getChildFile (fileName);

    return factory.existsAsFile() ? factory
                                  : manager.getUserPresetsDirectory().getChildFile (fileName);
}

//==============================================================================
/**
    v1.3.0 / v1.13.0 PRESET MIGRATION — the stored NORMALISED fractions, moved onto today's ranges.

    Moved here from the editor's setMigrationCallback lambda at v1.14.0, because loadPreserving
    below no longer goes through the module's apply and has to run it itself. ONE definition, so
    the module hook (still registered, for any module path) and the local apply cannot diverge.

    Presets store NORMALISED 0..1 fractions, and v1.3.0 moved three encodings:
      rolloff  range 3-6  → 3-12 dB/2x   (same engineering value ⇒ old fraction ÷ 3)
      width    range 0-6  → 0-12 m       (same metres            ⇒ old fraction ÷ 2)
      blur     kBlurScale 0.5 → 1.5      (same RADIUS            ⇒ old fraction ÷ 3)
    Sessions are unaffected — the APVTS stores denormalised values
    (critical_apvts_denormalised_vs_preset_normalised) — so only this JSON path migrates.

    The gate is the MAJOR.MINOR pair, parsed leniently: an unparseable version string is treated as
    pre-1.3 (every shipped version that wrote presets predates 1.3, so the failure mode of a mangled
    stamp is the migration running — correct for all presets that exist today).
*/
inline void migrateParameters (juce::DynamicObject& parameters, const juce::String& presetVersion)
{
    const auto tokens = juce::StringArray::fromTokens (presetVersion, ".", {});
    const int  major  = tokens.size() > 0 ? tokens[0].getIntValue() : 0;
    const int  minor  = tokens.size() > 1 ? tokens[1].getIntValue() : 0;

    const auto before = [major, minor] (int gateMinor)
    {
        return ! (major > 1 || (major == 1 && minor >= gateMinor));
    };

    const auto rescale = [&parameters] (const char* id, float factor)
    {
        const juce::Identifier key { id };

        if (parameters.hasProperty (key))
            parameters.setProperty (key,
                juce::jlimit (0.0f, 1.0f,
                              static_cast<float> (parameters.getProperty (key)) * factor));
    };

    // ── < 1.3: the three v1.3.0 range moves ──────────────────────────────────────────────────
    if (before (3))
    {
        rescale ("rolloff", 1.0f / 3.0f);
        rescale ("width",   1.0f / 2.0f);
        rescale ("blur",    1.0f / 3.0f);
    }

    // ── < 1.13: blur's law went SQUARE with kBlurScale 1.5 → 6 (v1.13.0) ─────────────────────
    //
    // ITS OWN GATE, not a widening of the v1.3 one
    // (pattern_preset_migration_per_param_version_gate): every preset stamped 1.3 through 1.12
    // carries a fraction against the LINEAR law. Same authored radius ⇒
    //   1.5 · b_old = 6 · b_new²  ⇒  b_new = ½ √b_old   (rigScale cancels).
    // A pre-1.3 preset passes through both arms in order: ÷3 first, then the root.
    if (before (13))
    {
        const juce::Identifier key { "blur" };

        if (parameters.hasProperty (key))
            parameters.setProperty (key,
                dbap::blurFractionLinearToSquare (static_cast<float> (parameters.getProperty (key))));
    }
}

//==============================================================================
/**
    LOAD A PRESET WITHOUT TOUCHING THE SOURCE, THE SCENE OR THE HALL LEVEL (P93, rebuilt v1.14.0).

    ── The defect this exists to prevent ─────────────────────────────────────────────────────────

    OuariconPresetManager::applyPresetJson (WR-01, module v1.0.3) UNCONDITIONALLY resets EVERY
    parameter to its default before applying anything. WR-01 is correct for a plugin whose preset
    is the whole patch (pattern_preset_apply_needs_reset_to_defaults) — but here it would re-centre
    the source, drop srcZ to 0 m, return all eight weights to 1.0 and the hall to 0 dB on every load.

    ── v1.14.0 (B5): THE PRESERVED SET IS NEVER WRITTEN, NOT WRITTEN AND THEN RESTORED ──────────

    Through v1.13.2 this function snapshotted the preserved set, let the module reset + apply all
    28, then wrote the snapshot back. The END state was bit-exact (probe CP), but in between srcX
    went 0.1 → 0.5 → 0.1 and w2 0 → 1.0 → 0 through setValueNotifyingHost — inside the open gesture
    brackets, so a Touch/Latch lane RECORDED the swing, and any audio block that started inside the
    window solved at centre with every speaker live. So the module's apply is not used at all: the
    file is read here, migrated by the same function the module hook runs, and ONLY the fifteen
    kAuthored values are written — the stored value, or the default for a key the preset omits
    (WR-01's reset, kept, but scoped to what a preset owns). A write of the value a parameter
    already holds is skipped, so an unchanged parameter puts nothing on the host's lane.

    Probe CP counts listener callbacks on the thirteen during a load and requires ZERO.

    ── Why the fix is HERE and never in the module ───────────────────────────────────────────────

    Nine plugins include that header and seven more carry vendored copies; a module opt-in filter
    is correct in the abstract and wrong for a nine-plugin shared header. STATIC GATE
    (ui_frontend_check §28): no call to the module's loadPreset / loadPresetFromFile /
    applyPresetData anywhere in Source/, so no future call site can reach the reset-everything
    apply by accident.

    @param applyCustomState  the SCENES half (the editor passes scenesFromVar). Called on every
                             successful load with the stored customState, or a void var when the
                             preset has none — the module v1.0.7 contract, and scenesFromVar treats
                             a void var as "leave the slots alone".
    @returns false — with NOTHING written — when the file is missing or not a preset object.
    @note Gesture brackets are the CALLER's job (PluginEditor.cpp's loadPreset opens one on each of
          the fifteen kAuthored parameters around this call).
*/
inline bool loadPreserving (OuariconPresetManager& manager,
                            juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& presetName,
                            const OuariconPresetManager::CustomLoadCallback& applyCustomState = {})
{
    if (fileSafeName (presetName).isEmpty())
        return false;

    const auto file = presetFile (manager, presetName);

    if (! file.existsAsFile())
        return false;

    const auto data   = juce::JSON::parse (file.loadFileAsString());
    auto*      preset = data.getDynamicObject();

    if (preset == nullptr)
        return false;

    const auto storedVar = preset->getProperty ("parameters");
    auto*      stored    = storedVar.getDynamicObject();

    if (stored == nullptr)
        return false;

    migrateParameters (*stored, preset->getProperty ("version").toString());

    for (const char* id : kAuthored)
    {
        auto* param = apvts.getParameter (id);

        if (param == nullptr)
            continue;

        const juce::Identifier key { id };
        const float target = stored->hasProperty (key)
                                 ? juce::jlimit (0.0f, 1.0f, static_cast<float> (stored->getProperty (key)))
                                 : param->getDefaultValue();

        if (! juce::exactlyEqual (param->getValue(), target))
            param->setValueNotifyingHost (target);
    }

    if (applyCustomState)
        applyCustomState (preset->hasProperty ("customState") ? preset->getProperty ("customState")
                                                              : juce::var());

    manager.setCurrentPresetName (fileSafeName (presetName));
    return true;
}

} // namespace presets

} // namespace oo
