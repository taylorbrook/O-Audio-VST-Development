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

#include "OuariconPresetManager.h"

namespace oo
{

//==============================================================================
/**
    THE SIX FACTORY PRESETS, AND THE ONE CALL SITE THAT LOADS THEM SAFELY (PLAN-4.1 P92, P93, P96).

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
/** The twelve parameters loadPreserving() holds still — the SOURCE, the SCENE, and the MATERIAL.

    Not a stylistic list: it is the complement of the six a factory preset writes, and 6 + 12 = 18
    is asserted below against oo::params::kCount so the two halves cannot drift apart.

    ── WHY v1.5.0's `decorr` IS PRESERVED AND NOT AUTHORED (the one judgement call in the list) ──

    Every other entry here is preserved because it describes WHERE the source is or WHICH speakers
    are live. `decorr` is preserved because it describes THE MATERIAL: it is the answer to "is this
    stem effectively mono?", which is a fact about the file on the track, not about the room the
    preset is painting. Wide Hall does not know whether you fed it a mono stem or a decorrelated
    stereo pair, and a preset that flipped the setting either way would be guessing.

    It also keeps all six factory presets bit-unchanged across the v1.4.0 -> v1.5.0 boundary, which
    is the same compatibility line the parameter's 0 default draws.
*/
inline constexpr std::array<const char*, 12> kPreserved
    = { "srcX", "srcY", "srcZ", "decorr",
        "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8" };

/** The six a factory preset writes — ROOM CHARACTER, and nothing that says where the source is. */
/** ...plus, since v1.8.0, THE TEN MOTION PARAMETERS. Authored, not preserved (PLAN overrides
    RESEARCH here): R7 says motion state travels in presets. The six original factory rows write
    nothing for them, so WR-01's reset-to-default lands motionOn = 0 on every one of those loads
    and they stay audibly identical to v1.7.0 (probe CP). A pre-1.8.0 user preset omits the keys,
    which resets them the same way — motion off (probe DK). */
inline constexpr std::array<const char*, 16> kAuthored
    = { "width", "rolloff", "blur", "hullAtten", "airAmount", "outputGain",
        "motionOn", "motionPath", "motionSync", "motionRate", "motionSize",
        "motionRatio", "motionAngle", "motionHeight", "motionPhase", "motionSeed" };

static_assert (kPreserved.size() + kAuthored.size() == params::kCount,
               "The preserved set and the authored set PARTITION the parameters. If this fires, a "
               "parameter was added and belongs in exactly one of the two lists — leaving it out of "
               "kPreserved means a preset load silently RESETS it to its default (WR-01), which is "
               "the whole defect loadPreserving() exists to prevent.");

//==============================================================================
/**
    The six presets, in ENGINEERING UNITS.

    | Preset          | width m | rolloff dB/2x | blur | hullAtten dB/m | airAmount | outputGain dB |
    |-----------------|---------|---------------|------|----------------|-----------|---------------|
    | Dry Point       |   0.0   |      6.0      | 0.00 |      2.2       |   0.00    |      0.0      |
    | Concert Default |   0.0   |      4.0      | 0.10 |      1.0       |   0.35    |      0.0      |
    | Chamber         |   1.5   |      4.5      | 0.18 |      1.4       |   0.25    |      0.0      |
    | Wide Hall       |   3.0   |      3.5      | 0.35 |      0.7       |   0.55    |     -1.5      |
    | Distant Field   |   4.5   |      3.0      | 0.55 |      0.4       |   0.85    |     -3.0      |
    | Enveloping      |   6.0   |      3.0      | 0.80 |      0.0       |   0.45    |     -2.0      |

    ── The axis is a constraint, not a description ───────────────────────────────────────────────

    rolloff falls 6.0 -> 3.0 while blur rises 0.00 -> 0.80, and hullAtten falls with them, because a
    diffuse image under a steep outside-hull trim fights itself. outputGain ONLY EVER TRIMS, so no
    preset can clip a rig calibrated on another.

    CONCERT DEFAULT'S SIX ARE EXACTLY THE SHIPPED DEFAULTS (verified against
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
        float width, rolloff, blur, hullAtten, airAmount, outputGain;
        Motion motion {};
    };

    // v1.3.0 — blur column rescaled ÷3 (rounded to 2 dp): kBlurScale tripled (0.5 → 1.5), and
    // these rows were AUTHORED as radii, not knob positions — the sound each preset ships is the
    // point, so the radius is what survives the rescale. Width and rolloff columns stay put: they
    // are engineering values and the live-range conversion below moves them onto the widened
    // ranges untouched. Concert Default's blur must remain EXACTLY the shipped default (0.03).
    static constexpr Row rows[] = {
        //  name              width  rolloff  blur  hullAtten  air   outGain
        { "Dry Point",         0.0f,   6.0f,  0.00f,    2.2f, 0.00f,   0.0f },
        { "Concert Default",   0.0f,   4.0f,  0.03f,    1.0f, 0.35f,   0.0f },
        { "Chamber",           1.5f,   4.5f,  0.06f,    1.4f, 0.25f,   0.0f },
        { "Wide Hall",         3.0f,   3.5f,  0.12f,    0.7f, 0.55f,  -1.5f },
        { "Distant Field",     4.5f,   3.0f,  0.18f,    0.4f, 0.85f,  -3.0f },
        { "Enveloping",        6.0f,   3.0f,  0.27f,    0.0f, 0.45f,  -2.0f },

        // v1.8.0 — TWO ROWS THAT MOVE. Room character from Wide Hall / Chamber respectively, so
        // the motion is what distinguishes them.
        //   Slow Orbit: Orbit (0), synced to 4 Bars (14), 8 m across, ratio 0.8, 1 m of height.
        //   Wander:     Drift (3), Free (0) at 0.05 Hz, 6 m, seed 7.
        //                                                            on  path sync rate  size ratio ang  hgt  phase seed
        { "Slow Orbit",        3.0f,   3.5f,  0.12f,    0.7f, 0.55f,  -1.5f, { 1.0f, 0.0f, 14.0f, 0.1f,  8.0f, 0.8f, 0.0f, 1.0f, 0.0f, 1.0f } },
        { "Wander",            1.5f,   4.5f,  0.06f,    1.4f, 0.25f,   0.0f, { 1.0f, 3.0f,  0.0f, 0.05f, 6.0f, 1.0f, 0.0f, 0.0f, 0.0f, 7.0f } },
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
        def.parameters["outputGain"] = norm ("outputGain", r.outputGain);

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
/**
    LOAD A PRESET WITHOUT MOVING THE SOURCE OR THE SCENE (P93 — the N5 fix).

    ── The defect this exists to prevent ─────────────────────────────────────────────────────────

    OuariconPresetManager::applyPresetJson (WR-01, module v1.0.3) UNCONDITIONALLY resets EVERY
    parameter to its default before applying anything. WR-01 is correct and exists for a good
    reason (pattern_preset_apply_needs_reset_to_defaults) — but it has a consequence a preset author
    would not draw: OMITTING THE ELEVEN NON-ROOM KEYS DOES NOT LEAVE THEM ALONE. IT RESETS THEM.
    srcX/srcY -> 0.5, srcZ -> 0 m, w1..w8 -> 1.0. Loading any factory preset mid-cue would re-centre
    the source and undo whatever scene is applied — the exact two-mechanism collision the
    room-character-only preset scope was written to prevent, arriving through the module's
    DEFENSIVE behaviour instead of through the preset's content.

    ── Why the fix is HERE and never in the module ───────────────────────────────────────────────

    Nine plugins include that header and seven more carry vendored copies. PluginEditor.cpp already
    ruled on this: the fix is at O-Octagon's call site. Rejected alternatives: authoring all 17 keys
    (same outcome — the eleven still move, just explicitly), and a module opt-out flag (correct in
    the abstract, wrong for a nine-plugin shared header).

    ── Bit-exact by construction ─────────────────────────────────────────────────────────────────

    The eleven are captured NORMALISED and written back NORMALISED — the same float out that went
    in, with no range round-trip to lose a bit in. Probe CP asserts them BIT-unchanged, and NC2
    (delete the restore; CP's six-changed clause still passes while the eleven-unchanged clause
    fails) is what proves that clause is the probe.

    STATIC GATE, receiver-agnostic:  grep -rnE '\.loadPreset[[:space:]]*\(' Source/  returns
    EXACTLY ONE hit, and that hit is the call inside this function. Two properties are deliberate:
    the pattern names no receiver, because the previous spelling pinned one ("presetManager") and
    so matched zero lines from the day it was written; and this description names the method bare,
    with no opening paren, so the gate does not count its own doc-comment and read as two. That
    single grep is what stops a future call site bypassing the restore.

    @note Gesture brackets are the CALLER's job and are already open around this call (P59's
          seventeen). The restore writes land inside a bracket whose correctness was established at
          Phase 3.2, so this creates no new bracketing obligation.
*/
inline bool loadPreserving (OuariconPresetManager& manager,
                            juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& presetName)
{
    std::array<juce::RangedAudioParameter*, kPreserved.size()> params {};
    std::array<float, kPreserved.size()>                       held {};

    for (std::size_t i = 0; i < kPreserved.size(); ++i)
    {
        params[i] = apvts.getParameter (kPreserved[i]);
        held[i]   = params[i] != nullptr ? params[i]->getValue() : 0.0f;
    }

    const bool ok = manager.loadPreset (presetName);

    // Written back on BOTH paths. A load that failed after WR-01's reset had already run would
    // otherwise leave the source centred and the scene cleared with nothing applied in exchange.
    for (std::size_t i = 0; i < kPreserved.size(); ++i)
        if (params[i] != nullptr)
            params[i]->setValueNotifyingHost (held[i]);

    return ok;
}

} // namespace presets

} // namespace oo
