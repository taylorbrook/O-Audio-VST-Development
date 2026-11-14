/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include "GiParamIds.h"
#include "OuariconPresetManager.h"
#include "dsp/GiFactory.h"
#include "dsp/GiHash.h"

//==============================================================================
// Factory presets (Stage 4, PLAN-4 PD1 – PD3; CONTEXT-4 D53; 1.0.0 bank).
//
// 64 presets, 8 per category: Drones, Expressive, Keys, Live, Motion, Pads,
// Rhythmic, Textures. 1.3.0 (preset-bank-expansion) grows the bank to
// kBankSize 160: the seven sourced categories to 20 each, Live 8, a new
// Field category of 12 (kCategoryCounts).
// Named "<Category> · <Name>" (U+00B7, <= 24 characters,
// never localized; the preset manager has no categories, so the
// case-insensitive name sort groups them in a flat menu).
//
// Values are ENGINEERING units — Hz, ms, %, dB, st, choice INDEX, bool 0 / 1 —
// converted once through each parameter's own range (convertTo0to1), so
// skewed ranges and choices land exactly (pattern_factory_preset_normalized_
// ignores_skew). Absent parameters read as default (the manager resets every
// non-excluded parameter before it applies). output_gain and rec_arm are
// never part of a preset (excluded in the manager; validateSpec refuses them).
// Every preset carries a fixed seed (an absent seed keeps the live one).
//
// R1 (D64, PLAN-R1 PD27): feedback and reverb are per stream. A key written
// "s*_<suffix>" is a WILDCARD: expandSpecValues() turns it into the four
// "sN_<suffix>" pairs in place (disabled streams included), so a preset that
// set a former global keeps that value on every stream. A literal "sN_<suffix>"
// pair after the wildcard overrides it for that stream. sN_fb_enable (On) and
// sN_reverb_pre (Post) are the defaults and are not written.
//
// Sources (PD2, D53): a preset lists its lanes as { index 0–3, "factory:<id>",
// name } with NO contentHash, so every load re-resolves a pristine tape (a
// hash would make an identical source a no-op that keeps a feedback-dirtied
// tape). Every enabled stream N reads slot N and must have a listed slot
// (validateSpec) — otherwise the preset would sound different per user. Live
// presets list NO `slots` key at all (D45: they keep the user's lanes).
// Never a blob (PD1).
//
// Keys presets keep pitch_rand 0 and no pitch-LFO route (realism titles must
// not wobble). Descriptions are documentation only; they are not written to
// the preset JSON.
//==============================================================================
namespace ogi::factory
{
    struct SlotRef
    {
        int         index;    // 0…3 (stream index + 1 = the stream that reads it)
        const char* source;   // factory:<id>
        const char* name;     // the lane name (user text, never localized)
    };

    struct FactoryPresetSpec
    {
        FactoryPresetSpec (const char* n, const char* d, std::int64_t sd, std::vector<std::pair<const char*, float>> v,
                           std::vector<SlotRef> sl, bool lv = false, juce::String cj = {})
            : name (n), description (d), seed (sd), values (std::move (v)), slots (std::move (sl)), live (lv), customJson (std::move (cj)) {}

        const char* name;
        const char* description;
        std::int64_t seed;
        std::vector<std::pair<const char*, float>> values;
        std::vector<SlotRef> slots;
        bool live = false;          // a Live template: no `slots` key (the user's lanes stay)
        juce::String customJson;    // optional {"custom":{…},"masks":[…]} merged into customState
    };

    /** modM_dest choice index of a destination parameter id (0 = None). */
    inline float modDest (const char* id)
    {
        for (int k = 1; k < GiParamIds::kModDests; ++k)
            if (juce::String (GiParamIds::kModDestIds[k]) == id)
                return static_cast<float> (k);
        jassertfalse;
        return 0.0f;
    }

    // Mod sources (index): 0 None, 1–4 LFO 1–4, 5 Mod Env 1, 6 Mod Env 2, 7 Velocity, 8 Key,
    //   9 Note Random, 10 Mod Wheel, 11 Pressure, 12 Slide, 13 Bend, 14–17 Macro 1–4.
    // LFO shapes: 0 Sine, 1 Triangle, 2 Saw, 3 Square, 4 S&H, 5 Smooth Random. LFO mode: 0 Global, 1 Per-voice.
    // Scan modes: 0 Free-run, 1 Retrigger, 2 Ping-pong. Filter types: 0 Off, 1 LP, 2 BP, 3 HP.
    // Voice modes: 0 Poly, 1 Mono, 2 Legato. grain_limit: 0 32, 1 64, 2 128, 3 256. sync_mode: 0 Free, 1 Tempo.
    // Tempo divisions (index): 3 1/32 T, 4 1/32, 6 1/16 T, 7 1/16, 8 1/16 D, 9 1/8 T, 10 1/8, 11 1/8 D,
    //   13 1/4, 16 1/2, 18 1 bar, 19 2 bars.
    // Scales (index): 2 Major, 3 Minor, 4 Pentatonic, 6 Dorian, 13 Minor Pentatonic, 17 Fifths + Octaves,
    //   24 5-Limit Just, 28 Slendro, 31 Custom.
    // rec_trigger: 0 Manual, 1 Threshold, 2 Next Note. rec_target: 0–3 = Slot 1–4.
    // rec_length_sync: 0 1 bar, 1 2 bars, 2 4 bars, 3 8 bars.

    /** 1.3.0 (preset-bank-expansion): the bank size and the per-category
        counts FP0 checks the table against (7 x 20, Live 8, Field 12). The
        table reaches them wave by wave; until then FP0 / FP1c / FP3c / FSU1 /
        FRT1 / PN1 carry the recorded expected-fail lines. */
    constexpr int kBankSize = 160;
    struct CategoryCount { const char* name; int count; };
    constexpr CategoryCount kCategoryCounts[] = {
        { "Drones", 20 }, { "Expressive", 20 }, { "Field", 12 }, { "Keys", 20 }, { "Live", 8 },
        { "Motion", 20 }, { "Pads", 20 }, { "Rhythmic", 20 }, { "Textures", 20 } };

    inline std::vector<FactoryPresetSpec> factoryPresetSpecs()
    {
        constexpr float Sine = 0, Tri = 1, Saw = 2, Square = 3, SH = 4, SmoothRnd = 5;
        constexpr float Lfo1 = 1, Lfo2 = 2, Lfo3 = 3, Lfo4 = 4, Env1 = 5, Env2 = 6, Vel = 7, Key = 8, NoteRnd = 9,
                        Wheel = 10, Press = 11, Slide = 12, Bend = 13, Mac1 = 14, Mac2 = 15, Mac3 = 16, Mac4 = 17;
        juce::ignoreUnused (Sine, Tri, Saw, Square, SH, SmoothRnd, Lfo3, Lfo4, Bend);

        return {
            //==================================================================
            // Drones — key track 0–50 %, slow / Ping-pong scan, feedback +
            // diffusion, long release; one with Freeze; S&H on a filter.
            //==================================================================
            { "Drones \xc2\xb7 Iron Sky",
              "A C-rooted drone stack read back and forth slowly; sample-and-hold steps a low-pass through it.",
              0x4F47490101000001ll,
              { { "polyphony", 6.0f }, { "amp_attack", 1800.0f }, { "amp_release", 6000.0f },
                { "s1_level", -4.4f }, { "s1_key_track", 30.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 8.0f }, { "s1_size", 900.0f }, { "s1_density", 9.0f },
                { "s1_spray", 30.0f }, { "s1_stereo_spread", 70.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 2800.0f }, { "s1_filter_res", 30.0f },
                { "s*_feedback", 50.0f }, { "s*_fb_diffusion", 60.0f }, { "s*_reverb_mix", 40.0f }, { "s*_reverb_size", 80.0f },
                { "lfo1_shape", SH }, { "lfo1_rate", 0.4f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 35.0f } },
              { { 0, "factory:drone", "Drone" } } },

            { "Drones \xc2\xb7 Deep Well",
              "The drone loops into a dark, diffused feedback well; the quiet noise bed stays out of it, lightly reverbed.",
              0x4F47490101000002ll,
              { { "polyphony", 6.0f }, { "amp_attack", 2500.0f }, { "amp_release", 8000.0f },
                { "s1_level", -5.2f }, { "s1_key_track", 20.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 5.0f }, { "s1_size", 1100.0f }, { "s1_density", 7.0f },
                { "s1_spray", 40.0f }, { "s1_pitch", -12.0f },
                { "s2_enable", 1.0f }, { "s2_level", -21.9f }, { "s2_key_track", 0.0f }, { "s2_size", 600.0f }, { "s2_density", 8.0f },
                { "s2_spray", 100.0f }, { "s2_filter_type", 1.0f }, { "s2_filter_cutoff", 900.0f },
                { "s1_feedback", 70.0f }, { "s1_fb_diffusion", 80.0f }, { "s1_fb_high_cut", 4000.0f },
                { "s1_reverb_mix", 45.0f }, { "s1_reverb_size", 85.0f }, { "s1_reverb_damp", 65.0f },
                { "s2_fb_enable", 0.0f }, { "s2_reverb_mix", 15.0f }, { "s2_reverb_size", 60.0f }, { "s2_reverb_damp", 65.0f } },
              { { 0, "factory:drone", "Drone" }, { 1, "factory:noise_bed", "Noise Bed" } } },

            { "Drones \xc2\xb7 Organ Loft",
              "Two organ streams an octave apart in a tall room, read past the key click; the lower one feeds its reverb back into its own loop.",
              0x4F47490101000003ll,
              { { "polyphony", 6.0f }, { "amp_attack", 1200.0f }, { "amp_release", 5000.0f },
                { "s1_level", -14.3f }, { "s1_key_track", 50.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 5.0f }, { "s1_size", 700.0f }, { "s1_density", 10.0f },
                { "s1_spray", 25.0f }, { "s1_loop_start", 2.0f },
                { "s2_enable", 1.0f }, { "s2_level", -13.5f }, { "s2_pitch", -12.0f }, { "s2_key_track", 50.0f }, { "s2_scan_mode", 2.0f },
                { "s2_scan_rate", 3.0f }, { "s2_size", 900.0f }, { "s2_density", 8.0f }, { "s2_spray", 35.0f }, { "s2_loop_start", 2.0f },
                { "s1_reverb_mix", 60.0f }, { "s1_reverb_size", 90.0f },
                { "s2_feedback", 30.0f }, { "s2_fb_diffusion", 40.0f }, { "s2_reverb_pre", 1.0f }, { "s2_reverb_mix", 60.0f }, { "s2_reverb_size", 90.0f } },
              { { 0, "factory:organ", "Organ" }, { 1, "factory:organ", "Organ" } } },

            { "Drones \xc2\xb7 Frozen Field",
              "Freeze holds the drone tape; long grains drift over it while the feedback stays parked.",
              0x4F47490101000004ll,
              { { "polyphony", 6.0f }, { "freeze", 1.0f }, { "amp_attack", 2000.0f }, { "amp_release", 7000.0f },
                { "s1_level", -6.9f }, { "s1_key_track", 40.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 4.0f }, { "s1_size", 1200.0f }, { "s1_density", 7.0f },
                { "s1_spray", 60.0f }, { "s1_stereo_spread", 90.0f },
                { "s*_feedback", 40.0f }, { "s*_fb_diffusion", 70.0f }, { "s*_reverb_mix", 50.0f }, { "s*_reverb_size", 85.0f } },
              { { 0, "factory:drone", "Drone" } } },

            { "Drones \xc2\xb7 Hollow Wind",
              "A resonant band-pass on the wind, stepped by sample-and-hold, over slow ping-pong reads.",
              0x4F47490101000005ll,
              { { "polyphony", 6.0f }, { "amp_attack", 1500.0f }, { "amp_release", 6000.0f },
                { "s1_level", 0.3f }, { "s1_key_track", 40.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 12.0f }, { "s1_size", 600.0f }, { "s1_density", 30.0f },
                { "s1_spray", 80.0f }, { "s1_stereo_spread", 80.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 650.0f }, { "s1_filter_res", 35.0f },
                { "s*_feedback", 45.0f }, { "s*_fb_diffusion", 55.0f }, { "s*_reverb_mix", 45.0f },
                { "lfo2_shape", SH }, { "lfo2_rate", 0.3f },
                { "mod1_source", Lfo2 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:wind", "Wind" } } },

            { "Drones \xc2\xb7 Low Orbit",
              "Drone and a harmonic fifth above, panned by a slow sine; the fifth moves at its own pace.",
              0x4F47490101000006ll,
              { { "polyphony", 6.0f }, { "amp_attack", 2000.0f }, { "amp_release", 6000.0f },
                { "s1_level", -7.0f }, { "s1_key_track", 25.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 3.0f }, { "s1_size", 1000.0f }, { "s1_density", 8.0f },
                { "s1_spray", 30.0f },
                { "s2_enable", 1.0f }, { "s2_level", -12.6f }, { "s2_pitch", 7.0f }, { "s2_key_track", 25.0f }, { "s2_size", 800.0f },
                { "s2_density", 8.0f }, { "s2_spray", 50.0f }, { "s2_scan_mode", 2.0f }, { "s2_scan_rate", 6.0f },
                { "s*_feedback", 35.0f }, { "s*_fb_diffusion", 50.0f }, { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 75.0f },
                { "lfo1_rate", 0.05f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_pan") }, { "mod1_depth", 60.0f } },
              { { 0, "factory:drone", "Drone" }, { 1, "factory:harmonic", "Harmonic" } } },

            { "Drones \xc2\xb7 Tape Halo",
              "The harmonic source circulating through heavy, saturated feedback — a warm halo that keeps growing.",
              0x4F47490101000007ll,
              { { "polyphony", 6.0f }, { "amp_attack", 1500.0f }, { "amp_release", 6000.0f },
                { "s1_key_track", 50.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 6.0f }, { "s1_size", 800.0f }, { "s1_density", 10.0f },
                { "s1_spray", 40.0f }, { "s1_level", -2.0f },
                { "s*_feedback", 80.0f }, { "s*_fb_saturation", 40.0f }, { "s*_fb_diffusion", 70.0f }, { "s*_fb_high_cut", 6000.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 70.0f } },
              { { 0, "factory:harmonic", "Harmonic" } } },

            { "Drones \xc2\xb7 Night Engine",
              "Ring-modulated drone feeding back dry over a noise bed left in deep reverb — a machine hum in the dark.",
              0x4F47490101000008ll,
              { { "polyphony", 6.0f }, { "amp_attack", 1500.0f }, { "amp_release", 5000.0f },
                { "s1_level", -1.8f }, { "s1_key_track", 10.0f }, { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 7.0f }, { "s1_size", 900.0f }, { "s1_density", 9.0f },
                { "s1_ring_freq", 55.0f }, { "s1_ring_mix", 25.0f }, { "s1_spray", 30.0f },
                { "s2_enable", 1.0f }, { "s2_level", -22.5f }, { "s2_key_track", 0.0f }, { "s2_size", 400.0f }, { "s2_density", 10.0f },
                { "s2_spray", 100.0f }, { "s2_filter_type", 2.0f }, { "s2_filter_cutoff", 300.0f }, { "s2_filter_res", 40.0f },
                { "s1_feedback", 55.0f }, { "s1_fb_diffusion", 45.0f },
                { "s2_fb_enable", 0.0f }, { "s2_reverb_mix", 70.0f }, { "s2_reverb_size", 75.0f }, { "s2_reverb_damp", 60.0f } },
              { { 0, "factory:drone", "Drone" }, { 1, "factory:noise_bed", "Noise Bed" } } },

            //==================================================================
            // Expressive — 4 × MPE (Pressure / Slide / Bend), 2 × Mono or
            // Legato + glide, 2 × velocity / key / note random / mod wheel.
            //==================================================================
            { "Expressive \xc2\xb7 MPE Breath",
              "MPE: pressure swells the level and opens the filter, slide walks the vowel tape.",
              0x4F47490201000001ll,
              { { "mpe_enable", 1.0f }, { "pitch_bend_range", 48.0f }, { "amp_attack", 60.0f }, { "amp_release", 900.0f },
                { "s1_size", 180.0f }, { "s1_density", 30.0f }, { "s1_spray", 15.0f }, { "s1_position", 30.0f }, { "s1_level", -1.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 1200.0f }, { "s1_filter_res", 20.0f },
                { "s*_reverb_mix", 25.0f },
                { "mod1_source", Press }, { "mod1_dest", modDest ("s1_level") }, { "mod1_depth", 40.0f },
                { "mod2_source", Press }, { "mod2_dest", modDest ("s1_filter_cutoff") }, { "mod2_depth", 50.0f },
                { "mod3_source", Slide }, { "mod3_dest", modDest ("s1_position") }, { "mod3_depth", 45.0f } },
              { { 0, "factory:vowel", "Vowel" } } },

            { "Expressive \xc2\xb7 Slide Glass",
              "MPE bell grains: slide opens the cutoff, pressure brings the level up, bend over two octaves.",
              0x4F47490201000002ll,
              { { "mpe_enable", 1.0f }, { "pitch_bend_range", 24.0f }, { "amp_attack", 10.0f }, { "amp_decay", 2000.0f },
                { "amp_sustain", 50.0f }, { "amp_release", 1500.0f },
                { "s1_scan_rate", 60.0f }, { "s1_size", 120.0f }, { "s1_density", 40.0f }, { "s1_spray", 8.0f }, { "s1_level", 2.9f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 1500.0f }, { "s1_filter_res", 30.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 70.0f },
                { "mod1_source", Slide }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 60.0f },
                { "mod2_source", Press }, { "mod2_dest", modDest ("s1_level") }, { "mod2_depth", 35.0f } },
              { { 0, "factory:bell", "Bell" } } },

            { "Expressive \xc2\xb7 Press Vowel",
              "MPE: pressure morphs the vowel (a, o, i) by moving the read position; slide brightens.",
              0x4F47490201000003ll,
              { { "mpe_enable", 1.0f }, { "pitch_bend_range", 48.0f }, { "amp_attack", 40.0f }, { "amp_release", 700.0f },
                { "s1_size", 160.0f }, { "s1_density", 32.0f }, { "s1_spray", 5.0f }, { "s1_position", 10.0f }, { "s1_level", -0.7f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 2500.0f },
                { "s*_reverb_mix", 20.0f },
                { "mod1_source", Press }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 70.0f },
                { "mod2_source", Slide }, { "mod2_dest", modDest ("s1_filter_cutoff") }, { "mod2_depth", 40.0f } },
              { { 0, "factory:vowel", "Vowel" } } },

            { "Expressive \xc2\xb7 Bend Choir",
              "MPE choir: a close vowel under a far harmonic halo an octave up; bend over four octaves pans the voice.",
              0x4F47490201000004ll,
              { { "mpe_enable", 1.0f }, { "pitch_bend_range", 48.0f }, { "amp_attack", 250.0f }, { "amp_release", 1800.0f },
                { "s1_size", 300.0f }, { "s1_density", 18.0f }, { "s1_spray", 25.0f }, { "s1_position", 40.0f }, { "s1_level", -5.0f },
                { "s2_enable", 1.0f }, { "s2_level", -10.0f }, { "s2_pitch", 12.0f }, { "s2_size", 350.0f }, { "s2_density", 12.0f },
                { "s2_spray", 30.0f },
                { "s1_reverb_mix", 15.0f }, { "s1_reverb_size", 45.0f },
                { "s2_reverb_mix", 65.0f }, { "s2_reverb_size", 85.0f },
                { "mod1_source", Bend }, { "mod1_dest", modDest ("s1_pan") }, { "mod1_depth", 50.0f },
                { "mod2_source", Press }, { "mod2_dest", modDest ("s2_level") }, { "mod2_depth", 40.0f },
                { "mod3_source", Slide }, { "mod3_dest", modDest ("s1_position") }, { "mod3_depth", 30.0f } },
              { { 0, "factory:vowel", "Vowel" }, { 1, "factory:harmonic", "Harmonic" } } },

            { "Expressive \xc2\xb7 Mono Lead",
              "Monophonic harmonic lead with a soft glide; the mod wheel adds vibrato-free brightness.",
              0x4F47490201000005ll,
              { { "voice_mode", 1.0f }, { "glide", 120.0f }, { "amp_attack", 15.0f }, { "amp_release", 400.0f },
                { "s1_size", 90.0f }, { "s1_density", 60.0f }, { "s1_spray", 3.0f }, { "s1_jitter", 5.0f }, { "s1_level", 4.1f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 2000.0f }, { "s1_filter_res", 35.0f }, { "s1_drive", 20.0f },
                { "s*_reverb_mix", 18.0f },
                { "mod1_source", Wheel }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 50.0f } },
              { { 0, "factory:harmonic", "Harmonic" } } },

            { "Expressive \xc2\xb7 Glide Bass",
              "Legato bass an octave down; overlapping notes glide without retriggering the envelope.",
              0x4F47490201000006ll,
              { { "voice_mode", 2.0f }, { "glide", 250.0f }, { "amp_attack", 8.0f }, { "amp_release", 300.0f },
                { "s1_pitch", -12.0f }, { "s1_size", 120.0f }, { "s1_density", 45.0f }, { "s1_spray", 2.0f }, { "s1_jitter", 0.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 900.0f }, { "s1_filter_res", 30.0f }, { "s1_drive", 35.0f },
                { "s1_stereo_spread", 0.0f },
                { "mod1_source", Vel }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:harmonic", "Harmonic" } } },

            { "Expressive \xc2\xb7 Wheel Morph",
              "Mod wheel walks the vowel, velocity opens the filter, the key sets the grain size.",
              0x4F47490201000007ll,
              { { "amp_attack", 30.0f }, { "amp_release", 900.0f },
                { "s1_size", 200.0f }, { "s1_density", 26.0f }, { "s1_spray", 10.0f }, { "s1_level", -3.1f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 1500.0f },
                { "s*_reverb_mix", 25.0f },
                { "mod1_source", Wheel }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 80.0f },
                { "mod2_source", Vel }, { "mod2_dest", modDest ("s1_filter_cutoff") }, { "mod2_depth", 45.0f },
                { "mod3_source", Key }, { "mod3_dest", modDest ("s1_size") }, { "mod3_depth", -30.0f } },
              { { 0, "factory:vowel", "Vowel" } } },

            { "Expressive \xc2\xb7 Velo Shards",
              "Bell shards: velocity sets the density and crush, each note lands at its own random pan.",
              0x4F47490201000008ll,
              { { "vel_to_amp", 45.0f }, { "amp_attack", 2.0f }, { "amp_decay", 2500.0f }, { "amp_sustain", 45.0f }, { "amp_release", 1200.0f },
                { "s1_scan_rate", 80.0f }, { "s1_size", 70.0f }, { "s1_density", 70.0f }, { "s1_spray", 20.0f }, { "s1_level", 6.0f },
                { "s1_crush", 30.0f }, { "s1_crush_prob", 40.0f },
                { "s*_reverb_mix", 30.0f },
                { "mod1_source", Vel }, { "mod1_dest", modDest ("s1_density") }, { "mod1_depth", 40.0f },
                { "mod2_source", NoteRnd }, { "mod2_dest", modDest ("s1_pan") }, { "mod2_depth", 70.0f },
                { "mod3_source", Vel }, { "mod3_dest", modDest ("s1_crush") }, { "mod3_depth", 50.0f } },
              { { 0, "factory:bell", "Bell" } } },

            //==================================================================
            // Keys — Poly, key track 100 %, short envelopes, velocity, a
            // percussive window, small spray. No pitch random, no pitch LFO.
            //==================================================================
            { "Keys \xc2\xb7 Grain Piano",
              "The pluck played through like a sample, in short percussive grains, velocity-sensitive.",
              0x4F47490301000001ll,
              { { "amp_attack", 2.0f }, { "amp_decay", 1800.0f }, { "amp_sustain", 50.0f }, { "amp_release", 600.0f }, { "vel_to_amp", 50.0f },
                { "s1_scan_rate", 100.0f }, { "s1_size", 60.0f }, { "s1_density", 140.0f }, { "s1_spray", 2.0f }, { "s1_jitter", 10.0f },
                { "s1_window", 0.5f }, { "s1_window_skew", -40.0f }, { "s1_level", 6.0f },
                { "s*_reverb_mix", 18.0f } },
              { { 0, "factory:pluck", "Pluck" } } },

            { "Keys \xc2\xb7 Celesta",
              "Bell an octave up in tight percussive grains; a short room.",
              0x4F47490301000002ll,
              { { "amp_attack", 1.0f }, { "amp_decay", 2500.0f }, { "amp_sustain", 45.0f }, { "amp_release", 800.0f }, { "vel_to_amp", 50.0f },
                { "s1_pitch", 12.0f }, { "s1_scan_rate", 100.0f }, { "s1_size", 50.0f }, { "s1_density", 150.0f }, { "s1_spray", 1.0f },
                { "s1_jitter", 5.0f }, { "s1_window", 0.6f }, { "s1_window_skew", -50.0f }, { "s1_level", 6.0f },
                { "s*_reverb_mix", 25.0f }, { "s*_reverb_size", 45.0f } },
              { { 0, "factory:bell", "Bell" } } },

            { "Keys \xc2\xb7 Bell",
              "The bell at its own pitch, long decay, generous room.",
              0x4F47490301000003ll,
              { { "amp_attack", 1.0f }, { "amp_decay", 4000.0f }, { "amp_sustain", 20.0f }, { "amp_release", 2000.0f }, { "vel_to_amp", 50.0f },
                { "s1_scan_rate", 100.0f }, { "s1_size", 80.0f }, { "s1_density", 60.0f }, { "s1_spray", 2.0f }, { "s1_jitter", 5.0f },
                { "s1_window", 0.6f }, { "s1_window_skew", -30.0f }, { "s1_level", 6.0f },
                { "s*_reverb_mix", 30.0f }, { "s*_reverb_size", 65.0f } },
              { { 0, "factory:bell", "Bell" } } },

            { "Keys \xc2\xb7 Glass Mallet",
              "Struck glass, low-passed, read back and forth over its ring so a held note never re-strikes; a bright pluck an octave above.",
              0x4F47490301000004ll,
              { { "amp_attack", 1.0f }, { "amp_decay", 5000.0f }, { "amp_sustain", 20.0f }, { "amp_release", 700.0f }, { "vel_to_amp", 40.0f },
                { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 25.0f }, { "s1_loop_end", 25.0f }, { "s1_size", 60.0f }, { "s1_density", 130.0f },
                { "s1_spray", 3.0f }, { "s1_jitter", 8.0f },
                { "s1_window", 0.8f }, { "s1_window_skew", -40.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 6000.0f },
                { "s2_enable", 1.0f }, { "s2_level", -6.0f }, { "s2_pitch", 12.0f }, { "s2_scan_rate", 100.0f }, { "s2_size", 40.0f },
                { "s2_density", 100.0f }, { "s2_spray", 3.0f }, { "s2_window", 0.9f },
                { "s*_reverb_mix", 32.0f } },
              { { 0, "factory:glass", "Glass" }, { 1, "factory:pluck", "Pluck" } } },

            { "Keys \xc2\xb7 Soft Rhodes",
              "Harmonic grains through a warm low-pass and a touch of drive; velocity opens it.",
              0x4F47490301000005ll,
              { { "amp_attack", 4.0f }, { "amp_decay", 1500.0f }, { "amp_sustain", 40.0f }, { "amp_release", 900.0f }, { "vel_to_amp", 90.0f },
                { "s1_scan_rate", 100.0f }, { "s1_size", 120.0f }, { "s1_density", 40.0f }, { "s1_spray", 3.0f }, { "s1_jitter", 10.0f },
                { "s1_window", 0.5f }, { "s1_level", 5.5f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 2500.0f }, { "s1_filter_res", 10.0f }, { "s1_drive", 15.0f },
                { "s*_reverb_mix", 20.0f },
                { "mod1_source", Vel }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 35.0f } },
              { { 0, "factory:harmonic", "Harmonic" } } },

            { "Keys \xc2\xb7 Kalimba",
              "Short kalimba-tine grains under a soft low-pass; a quick, bright decay.",
              0x4F47490301000006ll,
              { { "amp_attack", 0.0f }, { "amp_decay", 1200.0f }, { "amp_sustain", 55.0f }, { "amp_release", 900.0f }, { "vel_to_amp", 45.0f },
                { "s1_scan_rate", 120.0f }, { "s1_size", 60.0f }, { "s1_density", 200.0f }, { "s1_spray", 2.0f }, { "s1_jitter", 6.0f },
                { "s1_window", 0.6f }, { "s1_window_skew", -60.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 4000.0f }, { "s1_filter_res", 10.0f },
                { "s*_reverb_mix", 15.0f } },
              { { 0, "factory:kalimba", "Kalimba" } } },

            { "Keys \xc2\xb7 Harp Cascade",
              "Pluck plus its octave, a little sprayed, into a long hall.",
              0x4F47490301000007ll,
              { { "amp_attack", 2.0f }, { "amp_decay", 2500.0f }, { "amp_sustain", 35.0f }, { "amp_release", 1500.0f }, { "vel_to_amp", 50.0f },
                { "s1_scan_rate", 100.0f }, { "s1_size", 70.0f }, { "s1_density", 110.0f }, { "s1_spray", 6.0f }, { "s1_window", 0.6f },
                { "s1_level", 6.0f },
                { "s2_enable", 1.0f }, { "s2_level", -10.0f }, { "s2_pitch", 12.0f }, { "s2_scan_rate", 100.0f }, { "s2_size", 60.0f },
                { "s2_density", 50.0f }, { "s2_spray", 6.0f }, { "s2_window", 0.7f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 75.0f } },
              { { 0, "factory:pluck", "Pluck" }, { 1, "factory:pluck", "Pluck" } } },

            { "Keys \xc2\xb7 Toy Box",
              "Bell an octave up, lightly crushed on some grains — a music-box toy.",
              0x4F47490301000008ll,
              { { "amp_attack", 0.0f }, { "amp_decay", 2000.0f }, { "amp_sustain", 50.0f }, { "amp_release", 800.0f }, { "vel_to_amp", 50.0f },
                { "s1_pitch", 12.0f }, { "s1_scan_rate", 100.0f }, { "s1_size", 70.0f }, { "s1_density", 130.0f }, { "s1_spray", 2.0f },
                { "s1_window", 0.6f }, { "s1_window_skew", -50.0f }, { "s1_level", 6.0f }, { "s1_stereo_spread", 60.0f },
                { "s1_crush", 30.0f }, { "s1_crush_prob", 50.0f },
                { "s*_reverb_mix", 20.0f } },
              { { 0, "factory:bell", "Bell" } } },

            // 1.3.0 (preset-bank-expansion T3): 12 more Keys, one lead source
            // each from the Keys allocation row. Same rule (key track 100,
            // attack <= 5 ms, release <= 2 s, velocity, window >= 0.5, spray
            // <= 10, nothing that bends pitch). Off-C3 sources write their
            // root pair (crystal 73 / +17, guitar_drift 38 / -10,
            // guitar_overtones 72 / -32). Short sources decay to silence
            // before the tape wraps; sustained ones scan Ping-pong.
            { "Keys \xc2\xb7 Marimba",
              "The marimba bar, dry and woody: read back and forth over its strike so a held key never re-strikes, the envelope sets the decay; velocity shapes the stroke.",
              0x4F47490301000009ll,
              { { "amp_attack", 1.0f }, { "amp_decay", 2500.0f }, { "amp_sustain", 40.0f }, { "amp_release", 500.0f }, { "vel_to_amp", 40.0f },
                { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 15.0f }, { "s1_loop_end", 12.0f }, { "s1_size", 70.0f }, { "s1_density", 200.0f }, { "s1_spray", 2.0f }, { "s1_jitter", 6.0f },
                { "s1_window", 0.6f }, { "s1_window_skew", -50.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 5000.0f }, { "s1_filter_res", 10.0f },
                { "s*_reverb_mix", 14.0f }, { "s*_reverb_size", 40.0f } },
              { { 0, "factory:marimba_bar", "Marimba Bar" } } },

            { "Keys \xc2\xb7 Kalimba Hall",
              "Kalimba with a quiet octave tine above it in a long, open hall: the ambient thumb piano, ringing out after the key lifts.",
              0x4F47490301000010ll,
              { { "amp_attack", 0.0f }, { "amp_decay", 2400.0f }, { "amp_sustain", 10.0f }, { "amp_release", 1800.0f }, { "vel_to_amp", 50.0f },
                { "s1_scan_rate", 40.0f }, { "s1_size", 80.0f }, { "s1_density", 120.0f }, { "s1_spray", 3.0f }, { "s1_jitter", 6.0f },
                { "s1_window", 0.6f }, { "s1_window_skew", -50.0f }, { "s1_level", 6.0f },
                { "s2_enable", 1.0f }, { "s2_level", -1.5f }, { "s2_pitch", 12.0f }, { "s2_scan_rate", 40.0f }, { "s2_size", 50.0f },
                { "s2_density", 80.0f }, { "s2_spray", 3.0f }, { "s2_jitter", 6.0f }, { "s2_window", 0.7f }, { "s2_window_skew", -50.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 85.0f }, { "s*_reverb_damp", 40.0f } },
              { { 0, "factory:kalimba", "Kalimba" }, { 1, "factory:kalimba", "Kalimba" } } },

            { "Keys \xc2\xb7 FM Tines",
              "The FM bell as an electric-piano tine: low-passed, lightly driven, velocity opens the top; decays to a soft held tine.",
              0x4F47490301000011ll,
              { { "amp_attack", 2.0f }, { "amp_decay", 3500.0f }, { "amp_sustain", 35.0f }, { "amp_release", 900.0f }, { "vel_to_amp", 50.0f },
                { "s1_scan_rate", 60.0f }, { "s1_size", 90.0f }, { "s1_density", 180.0f }, { "s1_spray", 2.0f }, { "s1_jitter", 8.0f },
                { "s1_window", 0.5f }, { "s1_window_skew", -30.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 3000.0f }, { "s1_filter_res", 15.0f }, { "s1_drive", 10.0f },
                { "s*_reverb_mix", 18.0f },
                { "mod1_source", Vel }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 40.0f } },
              { { 0, "factory:fm_bell", "FM Bell" } } },

            { "Keys \xc2\xb7 Ice Bells",
              "Glass an octave up, high-passed and spread wide, over a soft FM bell: an icy top-octave sparkle that settles to a soft ring.",
              0x4F47490301000012ll,
              { { "amp_attack", 0.0f }, { "amp_decay", 1800.0f }, { "amp_sustain", 35.0f }, { "amp_release", 1200.0f }, { "vel_to_amp", 50.0f },
                { "s1_pitch", 12.0f }, { "s1_scan_rate", 25.0f }, { "s1_size", 45.0f }, { "s1_density", 200.0f }, { "s1_spray", 4.0f },
                { "s1_jitter", 6.0f }, { "s1_window", 0.7f }, { "s1_window_skew", -60.0f }, { "s1_level", 6.0f }, { "s1_stereo_spread", 70.0f },
                { "s1_filter_type", 3.0f }, { "s1_filter_cutoff", 300.0f },
                { "s2_enable", 1.0f }, { "s2_level", 5.2f }, { "s2_scan_rate", 60.0f }, { "s2_size", 70.0f }, { "s2_density", 160.0f },
                { "s2_spray", 3.0f }, { "s2_jitter", 6.0f }, { "s2_window", 0.6f }, { "s2_window_skew", -40.0f },
                { "s*_reverb_mix", 30.0f }, { "s*_reverb_size", 60.0f } },
              { { 0, "factory:glass", "Glass" }, { 1, "factory:fm_bell", "FM Bell" } } },

            { "Keys \xc2\xb7 Crystal Bowl",
              "The singing crystal read back and forth over its sustained ring: the held, glowing key of the set, in tune at its own root.",
              0x4F47490301000013ll,
              { { "amp_attack", 4.0f }, { "amp_decay", 3000.0f }, { "amp_sustain", 70.0f }, { "amp_release", 2000.0f }, { "vel_to_amp", 40.0f },
                { "s1_root_note", 73.0f }, { "s1_root_fine", 17.0f },
                { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 30.0f }, { "s1_loop_start", 50.0f }, { "s1_loop_end", 60.0f },
                { "s1_size", 150.0f }, { "s1_density", 40.0f }, { "s1_spray", 4.0f }, { "s1_jitter", 15.0f },
                { "s1_window", 0.8f }, { "s1_level", -8.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 70.0f } },
              { { 0, "factory:crystal", "Crystal" } } },

            { "Keys \xc2\xb7 Chapel Organ",
              "Organ with a quiet octave above, at full sustain in a stone room; read back and forth past the key click so a long chord never re-speaks.",
              0x4F47490301000014ll,
              { { "amp_attack", 3.0f }, { "amp_release", 350.0f }, { "vel_to_amp", 30.0f },
                { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 40.0f }, { "s1_loop_start", 2.0f }, { "s1_loop_end", 98.0f },
                { "s1_size", 120.0f }, { "s1_density", 50.0f }, { "s1_spray", 3.0f }, { "s1_jitter", 10.0f }, { "s1_window", 0.6f },
                { "s1_level", -10.6f },
                { "s2_enable", 1.0f }, { "s2_level", -20.6f }, { "s2_pitch", 12.0f }, { "s2_scan_mode", 2.0f }, { "s2_scan_rate", 40.0f },
                { "s2_loop_start", 2.0f }, { "s2_loop_end", 98.0f }, { "s2_size", 100.0f }, { "s2_density", 40.0f }, { "s2_spray", 3.0f },
                { "s2_jitter", 10.0f }, { "s2_window", 0.6f },
                { "s*_reverb_mix", 40.0f }, { "s*_reverb_size", 90.0f } },
              { { 0, "factory:organ", "Organ" }, { 1, "factory:organ", "Organ" } } },

            { "Keys \xc2\xb7 Gamelan Plate",
              "The struck metal plate an octave down: a long, inharmonic gong-bar ring for gamelan-like lines.",
              0x4F47490301000015ll,
              { { "amp_attack", 0.0f }, { "amp_decay", 4500.0f }, { "amp_sustain", 40.0f }, { "amp_release", 1500.0f }, { "vel_to_amp", 40.0f },
                { "s1_pitch", -12.0f }, { "s1_scan_rate", 75.0f }, { "s1_size", 110.0f }, { "s1_density", 150.0f }, { "s1_spray", 3.0f },
                { "s1_jitter", 8.0f }, { "s1_window", 0.6f }, { "s1_window_skew", -40.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 6000.0f },
                { "s*_reverb_mix", 30.0f }, { "s*_reverb_size", 70.0f } },
              { { 0, "factory:metal_plate", "Metal Plate" } } },

            { "Keys \xc2\xb7 Drift Guitar",
              "A picked guitar note played through from its attack like a sample, warm low-pass, a medium room; in tune at its own root.",
              0x4F47490301000016ll,
              { { "amp_attack", 2.0f }, { "amp_decay", 3000.0f }, { "amp_sustain", 25.0f }, { "amp_release", 1200.0f }, { "vel_to_amp", 60.0f },
                { "s1_root_note", 38.0f }, { "s1_root_fine", -10.0f },
                { "s1_position", 0.6f }, { "s1_scan_rate", 100.0f }, { "s1_size", 90.0f }, { "s1_density", 90.0f }, { "s1_spray", 2.0f },
                { "s1_jitter", 6.0f }, { "s1_window", 0.55f }, { "s1_window_skew", -30.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 5000.0f },
                { "s*_reverb_mix", 22.0f } },
              { { 0, "factory:guitar_drift", "Guitar Drift" } } },

            { "Keys \xc2\xb7 Guitar Harmonics",
              "Natural guitar harmonics read from the string's chime: glassy, pure and bright, in tune at their own root.",
              0x4F47490301000017ll,
              { { "amp_attack", 1.0f }, { "amp_decay", 3000.0f }, { "amp_sustain", 15.0f }, { "amp_release", 1500.0f }, { "vel_to_amp", 50.0f },
                { "s1_root_note", 72.0f }, { "s1_root_fine", -32.0f },
                { "s1_position", 3.4f }, { "s1_scan_rate", 55.0f }, { "s1_size", 70.0f }, { "s1_density", 110.0f }, { "s1_spray", 2.0f },
                { "s1_jitter", 5.0f }, { "s1_window", 0.6f }, { "s1_window_skew", -50.0f }, { "s1_level", 2.7f }, { "s1_stereo_spread", 50.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 70.0f } },
              { { 0, "factory:guitar_overtones", "Guitar Overtones" } } },

            { "Keys \xc2\xb7 Sus Chords",
              "One key plays a whole sus chord: a bright front that settles to a held chord, velocity brightening it, for one-finger comping.",
              0x4F47490301000018ll,
              { { "amp_attack", 1.0f }, { "amp_decay", 1200.0f }, { "amp_sustain", 50.0f }, { "amp_release", 250.0f }, { "vel_to_amp", 50.0f },
                { "s1_position", 1.5f }, { "s1_scan_rate", 100.0f }, { "s1_size", 60.0f }, { "s1_density", 200.0f }, { "s1_spray", 2.0f },
                { "s1_jitter", 5.0f }, { "s1_window", 0.5f }, { "s1_window_skew", -40.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 7000.0f }, { "s1_filter_res", 15.0f },
                { "s*_reverb_mix", 20.0f },
                { "mod1_source", Vel }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:chord_sus", "Chord Sus" } } },

            { "Keys \xc2\xb7 Poly Saw",
              "The saw stack as a synth key: a resonant low-pass that a mod envelope plucks open and velocity pushes further.",
              0x4F47490301000019ll,
              { { "amp_attack", 2.0f }, { "amp_decay", 1200.0f }, { "amp_sustain", 45.0f }, { "amp_release", 600.0f }, { "vel_to_amp", 70.0f },
                { "s1_scan_mode", 2.0f }, { "s1_scan_rate", 40.0f }, { "s1_loop_start", 5.0f }, { "s1_loop_end", 95.0f },
                { "s1_size", 100.0f }, { "s1_density", 60.0f }, { "s1_spray", 3.0f }, { "s1_jitter", 10.0f }, { "s1_window", 0.6f },
                { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 1500.0f }, { "s1_filter_res", 25.0f },
                { "s*_reverb_mix", 15.0f },
                { "menv1_attack", 0.0f }, { "menv1_decay", 600.0f }, { "menv1_release", 300.0f },
                { "mod1_source", Env1 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 45.0f },
                { "mod2_source", Vel }, { "mod2_dest", modDest ("s1_filter_cutoff") }, { "mod2_depth", 25.0f } },
              { { 0, "factory:saw_stack", "Saw Stack" } } },

            { "Keys \xc2\xb7 Bowed Stroke",
              "The bowed string read from past the bow's onset: an accented stroke that settles to a soft held bow, for string figures on the keys.",
              0x4F47490301000020ll,
              { { "amp_attack", 3.0f }, { "amp_decay", 900.0f }, { "amp_sustain", 35.0f }, { "amp_release", 300.0f }, { "vel_to_amp", 50.0f },
                { "s1_position", 2.5f }, { "s1_scan_rate", 100.0f }, { "s1_size", 60.0f }, { "s1_density", 200.0f }, { "s1_spray", 2.0f },
                { "s1_jitter", 6.0f }, { "s1_window", 0.6f }, { "s1_window_skew", -30.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 6000.0f },
                { "s*_reverb_mix", 22.0f }, { "s*_reverb_size", 50.0f } },
              { { 0, "factory:bowed", "Bowed" } } },

            //==================================================================
            // Live — Record-in templates. No `slots` key: the user's lanes stay
            // (D45). Manual / Threshold / Next Note, free and tempo lengths,
            // every target, gain, feedback, Freeze. "Live · Init" resets every
            // parameter and the scale and keeps the lanes.
            //==================================================================
            { "Live \xc2\xb7 Init",
              "Every parameter and the scale back to default; your lanes and takes stay.",
              0x4F47490401000001ll,
              {},
              {}, true },

            { "Live \xc2\xb7 Manual Loop",
              "Record 4 s into slot 1 with the Record button; the take plays back as a sliding grain loop.",
              0x4F47490401000002ll,
              { { "rec_target", 0.0f }, { "rec_trigger", 0.0f }, { "rec_length", 4.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 100.0f }, { "s1_size", 200.0f }, { "s1_density", 20.0f }, { "s1_spray", 5.0f },
                { "s*_reverb_mix", 15.0f } },
              {}, true },

            { "Live \xc2\xb7 Threshold Catch",
              "Armed, the first sound over -36 dB starts a 2 s take in slot 1; grains scatter through it.",
              0x4F47490401000003ll,
              { { "s1_level", -1.0f }, { "rec_target", 0.0f }, { "rec_trigger", 1.0f }, { "rec_threshold", -36.0f }, { "rec_length", 2.0f },
                { "s1_size", 150.0f }, { "s1_density", 20.0f }, { "s1_spray", 60.0f }, { "s1_stereo_spread", 70.0f },
                { "s*_reverb_mix", 25.0f } },
              {}, true },

            { "Live \xc2\xb7 Next Note Grab",
              "Armed, the next note you play starts a 3 s take into slot 2, played back by stream 2.",
              0x4F47490401000004ll,
              { { "s2_level", -1.0f }, { "rec_target", 1.0f }, { "rec_trigger", 2.0f }, { "rec_length", 3.0f },
                { "s1_enable", 0.0f },
                { "s2_enable", 1.0f }, { "s2_scan_rate", 100.0f }, { "s2_size", 180.0f }, { "s2_density", 22.0f }, { "s2_spray", 10.0f },
                { "s*_reverb_mix", 20.0f } },
              {}, true },

            { "Live \xc2\xb7 Tempo Bars",
              "Tempo mode: a 2-bar take into slot 1 on the Record button, grains locked to 1/16.",
              0x4F47490401000005ll,
              { { "s1_level", -1.5f }, { "sync_mode", 1.0f }, { "rec_target", 0.0f }, { "rec_trigger", 0.0f }, { "rec_length_sync", 1.0f },
                { "s1_density_sync", 7.0f }, { "s1_size_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_scan_rate", 100.0f } },
              {}, true },

            { "Live \xc2\xb7 Feedback Loop",
              "Record into slot 2; stream 2 reads it while 60 % feedback keeps rewriting the take.",
              0x4F47490401000006ll,
              { { "s2_level", -2.5f }, { "rec_target", 1.0f }, { "rec_trigger", 0.0f }, { "rec_length", 6.0f },
                { "s1_enable", 0.0f },
                { "s2_enable", 1.0f }, { "s2_scan_mode", 0.0f }, { "s2_scan_rate", 50.0f }, { "s2_size", 300.0f }, { "s2_density", 15.0f },
                { "s2_spray", 40.0f },
                { "s*_feedback", 60.0f }, { "s*_fb_diffusion", 50.0f }, { "s*_reverb_mix", 30.0f } },
              {}, true },

            { "Live \xc2\xb7 Freeze Cloud",
              "Record 8 s into slot 3 at +6 dB; Freeze holds it while a long-grain cloud plays it.",
              0x4F47490401000007ll,
              { { "rec_target", 2.0f }, { "rec_trigger", 0.0f }, { "rec_length", 8.0f }, { "rec_gain", 6.0f }, { "freeze", 1.0f },
                { "s1_enable", 0.0f },
                { "s3_enable", 1.0f }, { "s3_size", 700.0f }, { "s3_density", 10.0f }, { "s3_spray", 100.0f }, { "s3_stereo_spread", 90.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 80.0f } },
              {}, true },

            { "Live \xc2\xb7 Four Lanes",
              "Record into slot 4 at -6 dB; four lanes: dry, a small room, diffused feedback and a reverb wash inside the loop.",
              0x4F47490401000008ll,
              { { "s1_level", -3.3f }, { "rec_target", 3.0f }, { "rec_trigger", 0.0f }, { "rec_length", 4.0f }, { "rec_gain", -6.0f },
                { "s1_size", 200.0f }, { "s1_density", 12.0f },
                { "s2_enable", 1.0f }, { "s2_level", -9.0f }, { "s2_pitch", 7.0f }, { "s2_size", 200.0f }, { "s2_density", 10.0f },
                { "s3_enable", 1.0f }, { "s3_level", -11.0f }, { "s3_pitch", 12.0f }, { "s3_size", 200.0f }, { "s3_density", 8.0f },
                { "s4_enable", 1.0f }, { "s4_level", -9.0f }, { "s4_scan_rate", 100.0f }, { "s4_size", 250.0f }, { "s4_density", 12.0f },
                { "s2_reverb_mix", 25.0f }, { "s2_reverb_size", 25.0f },
                { "s3_feedback", 40.0f }, { "s3_fb_diffusion", 50.0f },
                { "s4_feedback", 25.0f }, { "s4_fb_diffusion", 30.0f }, { "s4_reverb_pre", 1.0f }, { "s4_reverb_mix", 60.0f }, { "s4_reverb_size", 85.0f } },
              {}, true },

            //==================================================================
            // Motion — Free-run shared playheads, negative scan, a narrow loop,
            // >= 6 routes each, per-voice LFOs, ring + crush, macros 1–4.
            //==================================================================
            { "Motion \xc2\xb7 Orbit Lines",
              "Free-run playheads on harmonic and vowel; a per-voice LFO pans, a slow global LFO shifts the vowel read; Macro 1 grows the grains.",
              0x4F47490501000001ll,
              { { "amp_attack", 200.0f }, { "amp_release", 1500.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 25.0f }, { "s1_size", 220.0f }, { "s1_density", 20.0f }, { "s1_spray", 15.0f },
                { "s1_level", -13.2f }, { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 4000.0f },
                { "s2_enable", 1.0f }, { "s2_level", -8.0f }, { "s2_scan_mode", 0.0f }, { "s2_scan_rate", 35.0f }, { "s2_size", 250.0f },
                { "s2_density", 15.0f }, { "s2_pitch", 12.0f },
                { "s*_reverb_mix", 30.0f },
                { "lfo1_mode", 1.0f }, { "lfo1_rate", 0.7f }, { "lfo2_rate", 0.11f }, { "lfo2_shape", Tri },
                { "menv1_attack", 800.0f }, { "menv1_sustain", 60.0f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_pan") }, { "mod1_depth", 60.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s2_position") }, { "mod2_depth", 40.0f },
                { "mod3_source", Env1 }, { "mod3_dest", modDest ("s1_filter_cutoff") }, { "mod3_depth", 30.0f },
                { "mod4_source", Vel }, { "mod4_dest", modDest ("s1_level") }, { "mod4_depth", 20.0f },
                { "mod5_source", Mac1 }, { "mod5_dest", modDest ("s1_size") }, { "mod5_depth", 50.0f },
                { "mod6_source", Wheel }, { "mod6_dest", modDest ("s2_level") }, { "mod6_depth", 30.0f } },
              { { 0, "factory:harmonic", "Harmonic" }, { 1, "factory:vowel", "Vowel" } } },

            { "Motion \xc2\xb7 Reverse River",
              "The drone read backwards in a free-running stream; per-voice LFOs bend the read; Macro 2 sprays it.",
              0x4F47490501000002ll,
              { { "amp_attack", 400.0f }, { "amp_release", 2500.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", -60.0f }, { "s1_size", 300.0f }, { "s1_density", 18.0f }, { "s1_spray", 20.0f },
                { "s1_level", -4.0f }, { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 5000.0f },
                { "s*_reverb_mix", 35.0f },
                { "lfo1_rate", 0.09f }, { "lfo2_mode", 1.0f }, { "lfo2_rate", 0.5f }, { "lfo2_shape", SmoothRnd },
                { "menv2_attack", 1500.0f }, { "menv2_sustain", 80.0f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 30.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s1_pitch_rand") }, { "mod2_depth", 4.0f },
                { "mod3_source", Mac2 }, { "mod3_dest", modDest ("s1_spray") }, { "mod3_depth", 60.0f },
                { "mod4_source", Env2 }, { "mod4_dest", modDest ("s1_density") }, { "mod4_depth", 25.0f },
                { "mod5_source", NoteRnd }, { "mod5_dest", modDest ("s1_pan") }, { "mod5_depth", 50.0f },
                { "mod6_source", Key }, { "mod6_dest", modDest ("s1_filter_cutoff") }, { "mod6_depth", 25.0f } },
              { { 0, "factory:drone", "Drone" } } },

            { "Motion \xc2\xb7 Loop Weaver",
              "A narrow loop in the vowel tape, its start walked by an LFO and Macro 3; per-voice pan.",
              0x4F47490501000003ll,
              { { "amp_attack", 80.0f }, { "amp_release", 1200.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 50.0f }, { "s1_loop_start", 40.0f }, { "s1_loop_end", 52.0f },
                { "s1_size", 160.0f }, { "s1_density", 28.0f }, { "s1_spray", 5.0f }, { "s1_level", -1.2f },
                { "s*_reverb_mix", 25.0f },
                { "lfo1_rate", 0.13f }, { "lfo1_shape", Tri }, { "lfo2_mode", 1.0f }, { "lfo2_rate", 1.1f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_loop_start") }, { "mod1_depth", 20.0f },
                { "mod2_source", Mac3 }, { "mod2_dest", modDest ("s1_loop_start") }, { "mod2_depth", 30.0f },
                { "mod3_source", Lfo2 }, { "mod3_dest", modDest ("s1_pan") }, { "mod3_depth", 55.0f },
                { "mod4_source", Vel }, { "mod4_dest", modDest ("s1_size") }, { "mod4_depth", 20.0f },
                { "mod5_source", Env1 }, { "mod5_dest", modDest ("s1_scan_rate") }, { "mod5_depth", 15.0f },
                { "mod6_source", Wheel }, { "mod6_dest", modDest ("s1_loop_end") }, { "mod6_depth", 25.0f } },
              { { 0, "factory:vowel", "Vowel" } } },

            { "Motion \xc2\xb7 Ring Spinner",
              "Bell grains through a ring modulator whose frequency an LFO spins; Macro 4 sets the ring mix.",
              0x4F47490501000004ll,
              { { "amp_attack", 5.0f }, { "amp_decay", 1500.0f }, { "amp_sustain", 70.0f }, { "amp_release", 1200.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 40.0f }, { "s1_size", 120.0f }, { "s1_density", 80.0f }, { "s1_spray", 10.0f },
                { "s1_level", 6.0f }, { "s1_ring_freq", 300.0f }, { "s1_ring_rand", 30.0f }, { "s1_ring_mix", 20.0f },
                { "s*_reverb_mix", 30.0f },
                { "lfo1_rate", 0.25f }, { "lfo2_mode", 1.0f }, { "lfo2_rate", 2.0f }, { "lfo2_shape", Tri },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_ring_freq") }, { "mod1_depth", 40.0f },
                { "mod2_source", Mac4 }, { "mod2_dest", modDest ("s1_ring_mix") }, { "mod2_depth", 50.0f },
                { "mod3_source", Lfo2 }, { "mod3_dest", modDest ("s1_pan") }, { "mod3_depth", 40.0f },
                { "mod4_source", Vel }, { "mod4_dest", modDest ("s1_ring_freq") }, { "mod4_depth", 20.0f },
                { "mod5_source", Key }, { "mod5_dest", modDest ("s1_size") }, { "mod5_depth", -20.0f },
                { "mod6_source", Env1 }, { "mod6_dest", modDest ("s1_density") }, { "mod6_depth", 20.0f } },
              { { 0, "factory:bell", "Bell" } } },

            { "Motion \xc2\xb7 Crush Tides",
              "Noise bed and pluck in tides of crushed grains; a per-voice LFO moves the crush, a slow global LFO the read.",
              0x4F47490501000005ll,
              { { "polyphony", 6.0f }, { "amp_attack", 150.0f }, { "amp_release", 1500.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 20.0f }, { "s1_size", 140.0f }, { "s1_density", 30.0f }, { "s1_spray", 60.0f },
                { "s1_level", -12.1f }, { "s1_crush", 45.0f }, { "s1_crush_prob", 60.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 1800.0f }, { "s1_filter_res", 30.0f },
                { "s2_enable", 1.0f }, { "s2_level", -6.0f }, { "s2_scan_rate", 100.0f }, { "s2_size", 80.0f }, { "s2_density", 25.0f },
                { "s2_spray", 5.0f }, { "s2_crush", 25.0f },
                { "s*_reverb_mix", 25.0f },
                { "lfo1_mode", 1.0f }, { "lfo1_rate", 0.6f }, { "lfo2_rate", 0.07f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_crush") }, { "mod1_depth", 35.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s1_position") }, { "mod2_depth", 40.0f },
                { "mod3_source", Mac1 }, { "mod3_dest", modDest ("s2_crush") }, { "mod3_depth", 60.0f },
                { "mod4_source", Vel }, { "mod4_dest", modDest ("s2_level") }, { "mod4_depth", 25.0f },
                { "mod5_source", NoteRnd }, { "mod5_dest", modDest ("s1_filter_cutoff") }, { "mod5_depth", 30.0f },
                { "mod6_source", Wheel }, { "mod6_dest", modDest ("s1_crush_prob") }, { "mod6_depth", 40.0f } },
              { { 0, "factory:noise_bed", "Noise Bed" }, { 1, "factory:pluck", "Pluck" } } },

            { "Motion \xc2\xb7 Swarm",
              "Dust and harmonic grains swarming: free-run reads, per-voice LFOs on density and pan.",
              0x4F47490501000006ll,
              { { "polyphony", 6.0f }, { "amp_attack", 300.0f }, { "amp_release", 2000.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 30.0f }, { "s1_size", 80.0f }, { "s1_density", 45.0f }, { "s1_spray", 100.0f },
                { "s1_level", -7.8f }, { "s1_gain_rand", 40.0f },
                { "s2_enable", 1.0f }, { "s2_level", -6.0f }, { "s2_scan_mode", 0.0f }, { "s2_scan_rate", -20.0f }, { "s2_size", 200.0f },
                { "s2_density", 18.0f }, { "s2_spray", 40.0f },
                { "s*_reverb_mix", 35.0f },
                { "lfo1_mode", 1.0f }, { "lfo1_rate", 0.9f }, { "lfo1_shape", SmoothRnd }, { "lfo2_mode", 1.0f }, { "lfo2_rate", 0.4f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_density") }, { "mod1_depth", 30.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s2_pan") }, { "mod2_depth", 60.0f },
                { "mod3_source", Mac2 }, { "mod3_dest", modDest ("s2_spray") }, { "mod3_depth", 50.0f },
                { "mod4_source", Env1 }, { "mod4_dest", modDest ("s1_level") }, { "mod4_depth", 20.0f },
                { "mod5_source", Key }, { "mod5_dest", modDest ("s2_size") }, { "mod5_depth", -25.0f },
                { "mod6_source", Vel }, { "mod6_dest", modDest ("s2_density") }, { "mod6_depth", 25.0f } },
              { { 0, "factory:dust", "Dust" }, { 1, "factory:harmonic", "Harmonic" } } },

            { "Motion \xc2\xb7 Phase Garden",
              "Two harmonic streams a fifth apart drifting in and out of phase; a slow LFO swells the upper one's reverb.",
              0x4F47490501000007ll,
              { { "amp_attack", 400.0f }, { "amp_release", 2500.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 15.0f }, { "s1_size", 260.0f }, { "s1_density", 16.0f }, { "s1_spray", 10.0f },
                { "s1_level", -8.1f },
                { "s2_enable", 1.0f }, { "s2_level", -9.6f }, { "s2_pitch", 7.0f }, { "s2_scan_mode", 0.0f }, { "s2_scan_rate", 17.0f },
                { "s2_size", 260.0f }, { "s2_density", 14.0f }, { "s2_spray", 10.0f },
                { "s1_reverb_mix", 40.0f }, { "s1_reverb_size", 70.0f },
                { "s2_reverb_mix", 40.0f }, { "s2_reverb_size", 70.0f },
                { "lfo1_mode", 1.0f }, { "lfo1_rate", 0.2f }, { "lfo2_mode", 1.0f }, { "lfo2_rate", 0.23f }, { "lfo2_phase", 90.0f },
                { "lfo3_rate", 0.08f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 25.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s2_position") }, { "mod2_depth", 25.0f },
                { "mod3_source", Lfo1 }, { "mod3_dest", modDest ("s1_pan") }, { "mod3_depth", 40.0f },
                { "mod4_source", Lfo2 }, { "mod4_dest", modDest ("s2_pan") }, { "mod4_depth", -40.0f },
                { "mod5_source", Mac3 }, { "mod5_dest", modDest ("s2_level") }, { "mod5_depth", 30.0f },
                { "mod6_source", Vel }, { "mod6_dest", modDest ("s1_size") }, { "mod6_depth", 20.0f },
                { "mod7_source", Lfo3 }, { "mod7_dest", modDest ("s2_reverb_mix") }, { "mod7_depth", 35.0f } },
              { { 0, "factory:harmonic", "Harmonic" }, { 1, "factory:harmonic", "Harmonic" } } },

            { "Motion \xc2\xb7 Clockwork",
              "The pulse loop in a free-running read, ring-modulated; LFOs and Macro 4 rearrange its grains.",
              0x4F47490501000008ll,
              { { "amp_attack", 5.0f }, { "amp_release", 600.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 100.0f }, { "s1_size", 110.0f }, { "s1_density", 24.0f }, { "s1_spray", 8.0f },
                { "s1_key_track", 50.0f }, { "s1_level", -7.9f }, { "s1_ring_freq", 800.0f }, { "s1_ring_mix", 20.0f },
                { "s*_reverb_mix", 20.0f },
                { "lfo1_mode", 1.0f }, { "lfo1_rate", 1.5f }, { "lfo1_shape", Square }, { "lfo2_rate", 0.17f }, { "lfo2_shape", Saw },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_reverse_prob") }, { "mod1_depth", 40.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s1_position") }, { "mod2_depth", 35.0f },
                { "mod3_source", Mac4 }, { "mod3_dest", modDest ("s1_ring_mix") }, { "mod3_depth", 60.0f },
                { "mod4_source", Vel }, { "mod4_dest", modDest ("s1_level") }, { "mod4_depth", 15.0f },
                { "mod5_source", NoteRnd }, { "mod5_dest", modDest ("s1_spray") }, { "mod5_depth", 30.0f },
                { "mod6_source", Env1 }, { "mod6_dest", modDest ("s1_ring_freq") }, { "mod6_depth", 25.0f } },
              { { 0, "factory:pulse", "Pulse" } } },

            //==================================================================
            // Pads — 2–4 streams with octave / fifth offsets, long envelopes,
            // LFO -> position, reverb 30–60 %, scale-quantised pitch random;
            // two with a custom scale + masks.
            //==================================================================
            { "Pads \xc2\xb7 Warm Choir",
              "The choir tape and its octave as slow overlapping grains; an LFO walks the vowel.",
              0x4F47490601000001ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 900.0f }, { "amp_decay", 2000.0f }, { "amp_sustain", 80.0f }, { "amp_release", 3000.0f },
                { "s1_position", 30.0f }, { "s1_size", 400.0f }, { "s1_density", 16.0f }, { "s1_spray", 40.0f }, { "s1_level", -0.5f },
                { "s2_enable", 1.0f }, { "s2_level", -5.5f }, { "s2_pitch", 12.0f }, { "s2_size", 500.0f }, { "s2_density", 10.0f },
                { "s2_spray", 50.0f }, { "s2_position", 50.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 75.0f },
                { "lfo1_rate", 0.08f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 25.0f } },
              { { 0, "factory:choir", "Choir" }, { 1, "factory:choir", "Choir" } } },

            { "Pads \xc2\xb7 Fifth Bloom",
              "Root, fifth and octave-down set near, mid and far in a wide hall; the lowest washes back into its own loop.",
              0x4F47490601000002ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1500.0f }, { "amp_sustain", 90.0f }, { "amp_release", 4000.0f },
                { "s1_size", 600.0f }, { "s1_density", 12.0f }, { "s1_spray", 30.0f }, { "s1_level", -8.1f },
                { "s2_enable", 1.0f }, { "s2_level", -10.0f }, { "s2_pitch", 7.0f }, { "s2_size", 700.0f }, { "s2_density", 9.0f },
                { "s2_spray", 40.0f },
                { "s3_enable", 1.0f }, { "s3_level", -10.5f }, { "s3_pitch", -12.0f }, { "s3_size", 900.0f }, { "s3_density", 7.0f },
                { "s3_spray", 50.0f },
                { "s1_reverb_mix", 25.0f }, { "s1_reverb_size", 85.0f },
                { "s2_reverb_mix", 50.0f }, { "s2_reverb_size", 85.0f },
                { "s3_feedback", 25.0f }, { "s3_fb_diffusion", 30.0f }, { "s3_reverb_pre", 1.0f }, { "s3_reverb_mix", 80.0f }, { "s3_reverb_size", 85.0f },
                { "lfo1_rate", 0.06f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:harmonic", "Harmonic" }, { 1, "factory:harmonic", "Harmonic" }, { 2, "factory:drone", "Drone" } } },

            { "Pads \xc2\xb7 Pentatonic Mist",
              "Harmonic and vowel grains scattered over a pentatonic, a slow LFO moving the vowel.",
              0x4F47490601000003ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1000.0f }, { "amp_release", 3500.0f },
                { "s1_size", 300.0f }, { "s1_density", 22.0f }, { "s1_spray", 60.0f }, { "s1_pitch_rand", 12.0f }, { "s1_scale", 4.0f },
                { "s1_level", -9.4f },
                { "s2_enable", 1.0f }, { "s2_level", -11.0f }, { "s2_pitch", 12.0f }, { "s2_size", 300.0f }, { "s2_density", 14.0f },
                { "s2_spray", 50.0f }, { "s2_pitch_rand", 7.0f }, { "s2_scale", 4.0f },
                { "s*_reverb_mix", 55.0f }, { "s*_reverb_size", 80.0f },
                { "lfo2_rate", 0.1f },
                { "mod1_source", Lfo2 }, { "mod1_dest", modDest ("s2_position") }, { "mod1_depth", 35.0f } },
              { { 0, "factory:harmonic", "Harmonic" }, { 1, "factory:vowel", "Vowel" } } },

            { "Pads \xc2\xb7 Just Glow",
              "Drone under harmonic grains scattered over 5-limit just intervals.",
              0x4F47490601000004ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1200.0f }, { "amp_release", 4000.0f },
                { "s1_size", 800.0f }, { "s1_density", 9.0f }, { "s1_spray", 30.0f }, { "s1_level", -7.0f },
                { "s2_enable", 1.0f }, { "s2_level", -9.0f }, { "s2_pitch", 12.0f }, { "s2_size", 400.0f }, { "s2_density", 14.0f },
                { "s2_spray", 40.0f }, { "s2_pitch_rand", 7.0f }, { "s2_scale", 24.0f },
                { "s*_reverb_mix", 40.0f }, { "s*_reverb_size", 75.0f },
                { "lfo1_rate", 0.07f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s2_position") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:drone", "Drone" }, { 1, "factory:harmonic", "Harmonic" } } },

            { "Pads \xc2\xb7 Nineteen",
              "A 19-EDO custom scale with a 7-note mask: harmonic grains scattered over it, vowel above.",
              0x4F47490601000005ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 900.0f }, { "amp_release", 3000.0f },
                { "s1_size", 350.0f }, { "s1_density", 18.0f }, { "s1_spray", 50.0f }, { "s1_pitch_rand", 12.0f }, { "s1_scale", 31.0f },
                { "s1_level", -7.0f },
                { "s2_enable", 1.0f }, { "s2_level", -12.0f }, { "s2_pitch", 12.0f }, { "s2_size", 400.0f }, { "s2_density", 10.0f },
                { "s2_spray", 50.0f }, { "s2_pitch_rand", 5.0f }, { "s2_scale", 31.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 75.0f },
                { "lfo1_rate", 0.09f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:harmonic", "Harmonic" }, { 1, "factory:vowel", "Vowel" } }, false,
              R"({"custom":{"name":"19-EDO","text":"edo 19"},"masks":[{"stream":1,"collection":31,"bits":"0000000000024949"},{"stream":2,"collection":31,"bits":"0000000000024949"}]})" },

            { "Pads \xc2\xb7 Seventeen Glass",
              "A 17-EDO custom scale masked to seven steps; bright harmonic glass over a drone.",
              0x4F47490601000006ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1100.0f }, { "amp_release", 3500.0f },
                { "s1_size", 300.0f }, { "s1_density", 18.0f }, { "s1_spray", 45.0f }, { "s1_pitch_rand", 12.0f }, { "s1_scale", 31.0f },
                { "s1_pitch", 12.0f }, { "s1_level", -11.4f },
                { "s2_enable", 1.0f }, { "s2_level", -8.0f }, { "s2_size", 900.0f }, { "s2_density", 7.0f }, { "s2_spray", 30.0f },
                { "s*_reverb_mix", 50.0f }, { "s*_reverb_size", 80.0f },
                { "lfo1_rate", 0.05f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s2_position") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:harmonic", "Harmonic" }, { 1, "factory:drone", "Drone" } }, false,
              R"({"custom":{"name":"17-EDO","text":"edo 17"},"masks":[{"stream":1,"collection":31,"bits":"00000000000124c9"}]})" },

            { "Pads \xc2\xb7 Slow Tide",
              "Drone and an octave-down vowel, very slow in and out; the vowel's reverb feeds back into its own loop.",
              0x4F47490601000007ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 2500.0f }, { "amp_release", 5000.0f },
                { "s1_scan_mode", 0.0f }, { "s1_scan_rate", 10.0f }, { "s1_size", 700.0f }, { "s1_density", 10.0f }, { "s1_spray", 40.0f },
                { "s1_level", -14.3f },
                { "s2_enable", 1.0f }, { "s2_level", -9.7f }, { "s2_pitch", -12.0f }, { "s2_size", 600.0f }, { "s2_density", 9.0f },
                { "s2_spray", 50.0f },
                { "s1_reverb_mix", 60.0f }, { "s1_reverb_size", 85.0f },
                { "s2_feedback", 30.0f }, { "s2_fb_diffusion", 40.0f }, { "s2_reverb_pre", 1.0f }, { "s2_reverb_mix", 60.0f }, { "s2_reverb_size", 85.0f },
                { "lfo1_shape", SmoothRnd }, { "lfo1_rate", 0.15f }, { "lfo2_rate", 0.04f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_pan") }, { "mod1_depth", 50.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s2_position") }, { "mod2_depth", 35.0f } },
              { { 0, "factory:drone", "Drone" }, { 1, "factory:vowel", "Vowel" } } },

            { "Pads \xc2\xb7 Velvet Strings",
              "Two bowed streams an octave apart under a soft low-pass, read past the bow's start.",
              0x4F47490601000008ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 700.0f }, { "amp_release", 2500.0f },
                { "s1_position", 10.0f }, { "s1_size", 250.0f }, { "s1_density", 26.0f }, { "s1_spray", 20.0f }, { "s1_level", -5.1f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 4000.0f },
                { "s2_enable", 1.0f }, { "s2_level", -6.0f }, { "s2_pitch", 12.0f }, { "s2_position", 10.0f }, { "s2_size", 250.0f },
                { "s2_density", 22.0f }, { "s2_spray", 20.0f }, { "s2_filter_type", 1.0f }, { "s2_filter_cutoff", 5000.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 70.0f },
                { "lfo1_rate", 0.12f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 20.0f } },
              { { 0, "factory:bowed", "Bowed" }, { 1, "factory:bowed", "Bowed" } } },

            // 1.3.0 (preset-bank-expansion T4): 12 Pads, one Pads-row lead source
            // each; realism titles (Choir, Strings, Organ) add no pitch wobble.
            { "Pads \xc2\xb7 Cathedral Choir",
              "The choir on root, fifth and octave below in a long stone hall: the big, still choral bed of the set.",
              0x4F47490601000009ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1400.0f }, { "amp_sustain", 90.0f }, { "amp_release", 4500.0f },
                { "s1_position", 35.0f }, { "s1_size", 450.0f }, { "s1_density", 14.0f }, { "s1_spray", 35.0f }, { "s1_level", -4.7f },
                { "s2_enable", 1.0f }, { "s2_level", -8.7f }, { "s2_pitch", 7.0f }, { "s2_position", 45.0f }, { "s2_size", 500.0f },
                { "s2_density", 10.0f }, { "s2_spray", 40.0f }, { "s2_pan", -30.0f },
                { "s3_enable", 1.0f }, { "s3_level", -8.7f }, { "s3_pitch", -12.0f }, { "s3_position", 55.0f }, { "s3_size", 600.0f },
                { "s3_density", 9.0f }, { "s3_spray", 40.0f }, { "s3_pan", 30.0f },
                { "s*_reverb_mix", 55.0f }, { "s*_reverb_size", 90.0f }, { "s*_reverb_damp", 60.0f },
                { "lfo1_rate", 0.05f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 20.0f } },
              { { 0, "factory:choir", "Choir" }, { 1, "factory:choir", "Choir" }, { 2, "factory:choir", "Choir" } } },

            { "Pads \xc2\xb7 Low Strings",
              "Bowed strings with a section an octave below under a dark low-pass: the cello-and-bass floor, read past the bow's start.",
              0x4F47490601000010ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1000.0f }, { "amp_sustain", 85.0f }, { "amp_release", 3000.0f },
                { "s1_position", 12.0f }, { "s1_size", 300.0f }, { "s1_density", 22.0f }, { "s1_spray", 20.0f }, { "s1_level", -5.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 3000.0f },
                { "s2_enable", 1.0f }, { "s2_level", -5.0f }, { "s2_pitch", -12.0f }, { "s2_position", 15.0f }, { "s2_size", 350.0f },
                { "s2_density", 18.0f }, { "s2_spray", 20.0f }, { "s2_filter_type", 1.0f }, { "s2_filter_cutoff", 2000.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 75.0f },
                { "lfo1_rate", 0.1f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s2_position") }, { "mod1_depth", 15.0f } },
              { { 0, "factory:bowed", "Bowed" }, { 1, "factory:bowed", "Bowed" } } },

            { "Pads \xc2\xb7 Organ Swell",
              "Organ 8' and 4' with a reed stop an octave down, swelling in slowly past the key click: the sustained church-organ pad.",
              0x4F47490601000011ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1800.0f }, { "amp_sustain", 100.0f }, { "amp_release", 3000.0f },
                { "s1_position", 25.0f }, { "s1_size", 250.0f }, { "s1_density", 20.0f }, { "s1_spray", 30.0f }, { "s1_window", 0.6f },
                { "s1_level", -9.7f },
                { "s2_enable", 1.0f }, { "s2_level", -15.7f }, { "s2_pitch", 12.0f }, { "s2_position", 30.0f }, { "s2_size", 200.0f },
                { "s2_density", 18.0f }, { "s2_spray", 30.0f }, { "s2_window", 0.6f },
                { "s3_enable", 1.0f }, { "s3_level", -14.7f }, { "s3_pitch", -12.0f }, { "s3_position", 30.0f }, { "s3_size", 300.0f },
                { "s3_density", 14.0f }, { "s3_spray", 30.0f }, { "s3_filter_type", 1.0f }, { "s3_filter_cutoff", 2500.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 85.0f },
                { "lfo1_rate", 0.07f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 15.0f } },
              { { 0, "factory:organ", "Organ" }, { 1, "factory:organ", "Organ" }, { 2, "factory:reed", "Reed" } } },

            { "Pads \xc2\xb7 Minor Haze",
              "A minor chord and its octave below under a soft low-pass, high-passed breath air on top: the dark, sombre pad.",
              0x4F47490601000012ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1200.0f }, { "amp_release", 4000.0f },
                { "s1_position", 40.0f }, { "s1_size", 400.0f }, { "s1_density", 14.0f }, { "s1_spray", 50.0f }, { "s1_level", -5.9f },
                { "s2_enable", 1.0f }, { "s2_level", -8.9f }, { "s2_pitch", -12.0f }, { "s2_position", 50.0f }, { "s2_size", 500.0f },
                { "s2_density", 10.0f }, { "s2_spray", 50.0f },
                { "s2_filter_type", 1.0f }, { "s2_filter_cutoff", 2500.0f },
                { "s3_enable", 1.0f }, { "s3_level", -16.9f }, { "s3_position", 30.0f }, { "s3_size", 300.0f }, { "s3_density", 16.0f },
                { "s3_spray", 80.0f }, { "s3_filter_type", 3.0f }, { "s3_filter_cutoff", 3000.0f },
                { "s*_reverb_mix", 50.0f }, { "s*_reverb_size", 80.0f },
                { "lfo1_shape", SmoothRnd }, { "lfo1_rate", 0.08f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 30.0f } },
              { { 0, "factory:chord_minor", "Chord Minor" }, { 1, "factory:chord_minor", "Chord Minor" }, { 2, "factory:breath", "Breath" } } },

            { "Pads \xc2\xb7 Sus Horizon",
              "A suspended chord with its octave below, wide and open, neither major nor minor: the floating, unresolved-bright pad.",
              0x4F47490601000013ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1500.0f }, { "amp_release", 4000.0f },
                { "s1_position", 35.0f }, { "s1_size", 450.0f }, { "s1_density", 14.0f }, { "s1_spray", 40.0f }, { "s1_level", -3.3f },
                { "s1_stereo_spread", 70.0f },
                { "s2_enable", 1.0f }, { "s2_level", -5.3f }, { "s2_pitch", -12.0f }, { "s2_position", 45.0f }, { "s2_size", 550.0f },
                { "s2_density", 10.0f }, { "s2_spray", 40.0f }, { "s2_stereo_spread", 70.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 85.0f },
                { "lfo2_rate", 0.06f },
                { "mod1_source", Lfo2 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 25.0f } },
              { { 0, "factory:chord_sus", "Chord Sus" }, { 1, "factory:chord_sus", "Chord Sus" } } },

            { "Pads \xc2\xb7 Analog Sweep",
              "A saw stack and its octave over a sub, a slow LFO sweeping the low-pass: the classic filtered synth pad.",
              0x4F47490601000014ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 900.0f }, { "amp_sustain", 85.0f }, { "amp_release", 3000.0f },
                { "s1_position", 40.0f }, { "s1_size", 300.0f }, { "s1_density", 18.0f }, { "s1_spray", 30.0f }, { "s1_level", -6.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 1500.0f }, { "s1_filter_res", 35.0f },
                { "s2_enable", 1.0f }, { "s2_level", -10.0f }, { "s2_pitch", 12.0f }, { "s2_position", 50.0f }, { "s2_size", 250.0f },
                { "s2_density", 14.0f }, { "s2_spray", 30.0f }, { "s2_filter_type", 1.0f }, { "s2_filter_cutoff", 2500.0f },
                { "s3_enable", 1.0f }, { "s3_level", -10.0f }, { "s3_root_note", 24.0f }, { "s3_root_fine", 0.0f }, { "s3_pitch", -12.0f },
                { "s3_position", 40.0f }, { "s3_size", 400.0f }, { "s3_density", 10.0f }, { "s3_spray", 20.0f },
                { "s*_reverb_mix", 35.0f }, { "s*_reverb_size", 70.0f },
                { "lfo1_shape", Tri }, { "lfo1_rate", 0.12f }, { "lfo2_rate", 0.05f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 40.0f },
                { "mod2_source", Lfo1 }, { "mod2_dest", modDest ("s2_filter_cutoff") }, { "mod2_depth", 30.0f },
                { "mod3_source", Lfo2 }, { "mod3_dest", modDest ("s1_position") }, { "mod3_depth", 20.0f } },
              { { 0, "factory:saw_stack", "Saw Stack" }, { 1, "factory:saw_stack", "Saw Stack" }, { 2, "factory:sub", "Sub" } } },

            { "Pads \xc2\xb7 Shimmer Veil",
              "The shimmer tape and its octave above, high-passed and wide: a thin, glittering top layer to sit over other parts.",
              0x4F47490601000015ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1500.0f }, { "amp_release", 5000.0f },
                { "s1_position", 30.0f }, { "s1_size", 350.0f }, { "s1_density", 18.0f }, { "s1_spray", 60.0f }, { "s1_level", -2.7f },
                { "s1_stereo_spread", 80.0f },
                { "s2_enable", 1.0f }, { "s2_level", -6.7f }, { "s2_pitch", 12.0f }, { "s2_position", 60.0f }, { "s2_size", 300.0f },
                { "s2_density", 14.0f }, { "s2_spray", 60.0f }, { "s2_stereo_spread", 90.0f }, { "s2_reverse_prob", 30.0f },
                { "s2_filter_type", 3.0f }, { "s2_filter_cutoff", 800.0f },
                { "s*_reverb_mix", 60.0f }, { "s*_reverb_size", 90.0f }, { "s*_reverb_damp", 30.0f },
                { "lfo1_rate", 0.09f }, { "lfo2_shape", SmoothRnd }, { "lfo2_rate", 0.2f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 30.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s2_pan") }, { "mod2_depth", 40.0f } },
              { { 0, "factory:shimmer", "Shimmer" }, { 1, "factory:shimmer", "Shimmer" } } },

            { "Pads \xc2\xb7 Reed Consort",
              "Reed on root, fifth and octave below, soft-edged and close: the woody wind-ensemble chord.",
              0x4F47490601000016ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 800.0f }, { "amp_sustain", 85.0f }, { "amp_release", 2800.0f },
                { "s1_position", 30.0f }, { "s1_size", 250.0f }, { "s1_density", 20.0f }, { "s1_spray", 25.0f }, { "s1_level", -5.4f },
                { "s1_window", 0.6f }, { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 4000.0f },
                { "s2_enable", 1.0f }, { "s2_level", -9.4f }, { "s2_pitch", 7.0f }, { "s2_position", 40.0f }, { "s2_size", 250.0f },
                { "s2_density", 16.0f }, { "s2_spray", 25.0f }, { "s2_window", 0.6f }, { "s2_pan", -35.0f },
                { "s3_enable", 1.0f }, { "s3_level", -7.4f }, { "s3_pitch", -12.0f }, { "s3_position", 35.0f }, { "s3_size", 300.0f },
                { "s3_density", 14.0f }, { "s3_spray", 25.0f }, { "s3_pan", 35.0f }, { "s3_filter_type", 1.0f }, { "s3_filter_cutoff", 2500.0f },
                { "s*_reverb_mix", 30.0f }, { "s*_reverb_size", 60.0f },
                { "lfo1_rate", 0.11f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 20.0f } },
              { { 0, "factory:reed", "Reed" }, { 1, "factory:reed", "Reed" }, { 2, "factory:reed", "Reed" } } },

            { "Pads \xc2\xb7 Crystal Halo",
              "The singing crystal's ring and its octave above, wide in a bright hall: the glassy, bell-tone pad, in tune at its own root.",
              0x4F47490601000017ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1200.0f }, { "amp_release", 4500.0f },
                { "s1_root_note", 73.0f }, { "s1_root_fine", 17.0f },
                { "s1_position", 55.0f }, { "s1_size", 400.0f }, { "s1_density", 14.0f }, { "s1_spray", 15.0f }, { "s1_level", -8.2f },
                { "s1_stereo_spread", 70.0f },
                { "s2_enable", 1.0f }, { "s2_root_note", 73.0f }, { "s2_root_fine", 17.0f }, { "s2_level", -13.2f }, { "s2_pitch", 12.0f },
                { "s2_position", 60.0f }, { "s2_size", 300.0f }, { "s2_density", 10.0f }, { "s2_spray", 15.0f }, { "s2_stereo_spread", 90.0f },
                { "s*_reverb_mix", 50.0f }, { "s*_reverb_size", 85.0f }, { "s*_reverb_damp", 35.0f },
                { "lfo1_rate", 0.06f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 10.0f } },
              { { 0, "factory:crystal", "Crystal" }, { 1, "factory:crystal", "Crystal" } } },

            { "Pads \xc2\xb7 Breath Cloud",
              "Breath grains band-passed into soft air with the shimmer an octave above: the airy, near-noise pad for space rather than harmony.",
              0x4F47490601000018ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 2000.0f }, { "amp_release", 5000.0f },
                { "s1_position", 40.0f }, { "s1_size", 400.0f }, { "s1_density", 150.0f }, { "s1_spray", 80.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 2000.0f }, { "s1_filter_res", 0.0f }, { "s1_stereo_spread", 80.0f },
                { "s2_enable", 1.0f }, { "s2_level", 3.5f }, { "s2_pitch", 12.0f }, { "s2_position", 40.0f }, { "s2_size", 350.0f },
                { "s2_density", 20.0f }, { "s2_spray", 60.0f }, { "s2_stereo_spread", 80.0f },
                { "s*_reverb_mix", 55.0f }, { "s*_reverb_size", 85.0f },
                { "lfo1_shape", SmoothRnd }, { "lfo1_rate", 0.1f }, { "lfo2_rate", 0.07f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 30.0f },
                { "mod2_source", Lfo2 }, { "mod2_dest", modDest ("s1_position") }, { "mod2_depth", 30.0f } },
              { { 0, "factory:breath", "Breath" }, { 1, "factory:shimmer", "Shimmer" } } },

            { "Pads \xc2\xb7 Overtone Glow",
              "A guitar's natural harmonic held as a cloud with its fifth above, read over the ring past the pluck: the pale, glassy guitar pad, in tune at its own root.",
              0x4F47490601000019ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1000.0f }, { "amp_release", 3500.0f },
                { "s1_root_note", 72.0f }, { "s1_root_fine", -32.0f },
                { "s1_position", 35.0f }, { "s1_size", 300.0f }, { "s1_density", 16.0f }, { "s1_spray", 20.0f }, { "s1_level", -6.2f },
                { "s2_enable", 1.0f }, { "s2_root_note", 72.0f }, { "s2_root_fine", -32.0f }, { "s2_level", -11.2f }, { "s2_pitch", 7.0f },
                { "s2_position", 45.0f }, { "s2_size", 300.0f }, { "s2_density", 12.0f }, { "s2_spray", 20.0f }, { "s2_pan", 30.0f },
                { "s*_reverb_mix", 45.0f }, { "s*_reverb_size", 80.0f },
                { "lfo1_rate", 0.08f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 12.0f } },
              { { 0, "factory:guitar_overtones", "Guitar Overtones" }, { 1, "factory:guitar_overtones", "Guitar Overtones" } } },

            { "Pads \xc2\xb7 Deep Floor",
              "The sub an octave below a soft minor chord, slow and dark: the low foundation pad that carries a mix's bottom.",
              0x4F47490601000020ll,
              { { "grain_limit", 2.0f }, { "amp_attack", 1500.0f }, { "amp_release", 4000.0f },
                { "s1_root_note", 24.0f }, { "s1_root_fine", 0.0f }, { "s1_pitch", -12.0f },
                { "s1_position", 40.0f }, { "s1_size", 500.0f }, { "s1_density", 12.0f }, { "s1_spray", 20.0f }, { "s1_level", -8.5f },
                { "s2_enable", 1.0f }, { "s2_level", -16.5f }, { "s2_position", 45.0f }, { "s2_size", 450.0f }, { "s2_density", 10.0f },
                { "s2_spray", 40.0f }, { "s2_filter_type", 1.0f }, { "s2_filter_cutoff", 1800.0f },
                { "s*_reverb_mix", 30.0f }, { "s*_reverb_size", 70.0f },
                { "lfo1_rate", 0.05f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s2_position") }, { "mod1_depth", 20.0f } },
              { { 0, "factory:sub", "Sub" }, { 1, "factory:chord_minor", "Chord Minor" } } },

            //==================================================================
            // Rhythmic — Tempo sync, density / size sync, jitter 0, Retrigger
            // scan, synced LFOs.
            //==================================================================
            { "Rhythmic \xc2\xb7 Pulse Grid",
              "The 120-bpm pulse loop read in real time, 1/16 grains locked to the host tempo.",
              0x4F47490701000001ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 2.0f }, { "amp_release", 200.0f },
                { "s1_scan_mode", 1.0f }, { "s1_scan_rate", 100.0f }, { "s1_key_track", 0.0f }, { "s1_density_sync", 7.0f },
                { "s1_size_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f }, { "s1_level", -6.2f },
                { "s*_reverb_mix", 10.0f } },
              { { 0, "factory:pulse", "Pulse" } } },

            { "Rhythmic \xc2\xb7 Pluck Arp",
              "Pluck grains on every 1/8 at the note's pitch; a synced triangle sweeps the filter.",
              0x4F47490701000002ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 2.0f }, { "amp_release", 400.0f },
                { "s1_scan_mode", 1.0f }, { "s1_density_sync", 10.0f }, { "s1_size_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f },
                { "s1_window", 0.8f }, { "s1_level", 1.4f }, { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 2500.0f }, { "s1_filter_res", 40.0f },
                { "s*_reverb_mix", 18.0f },
                { "lfo1_shape", Tri }, { "lfo1_rate_sync", 16.0f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 40.0f } },
              { { 0, "factory:pluck", "Pluck" } } },

            { "Rhythmic \xc2\xb7 Stutter Kit",
              "1/32 grains of the pulse loop; a synced sample-and-hold jumps the read point every 1/8.",
              0x4F47490701000003ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 1.0f }, { "amp_release", 150.0f },
                { "s1_scan_mode", 1.0f }, { "s1_scan_rate", 100.0f }, { "s1_key_track", 0.0f }, { "s1_density_sync", 4.0f },
                { "s1_size_sync", 4.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f }, { "s1_reverse_prob", 25.0f }, { "s1_level", -6.4f },
                { "lfo1_shape", SH }, { "lfo1_rate_sync", 10.0f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 50.0f } },
              { { 0, "factory:pulse", "Pulse" } } },

            { "Rhythmic \xc2\xb7 Dotted Echo",
              "Pluck grains on dotted eighths with feedback trailing behind them.",
              0x4F47490701000004ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 2.0f }, { "amp_release", 600.0f },
                { "s1_scan_mode", 1.0f }, { "s1_density_sync", 11.0f }, { "s1_size_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f },
                { "s1_window", 0.7f }, { "s1_level", 6.0f },
                { "s*_feedback", 40.0f }, { "s*_fb_diffusion", 20.0f }, { "s*_reverb_mix", 20.0f } },
              { { 0, "factory:pluck", "Pluck" } } },

            { "Rhythmic \xc2\xb7 Triplet Rain",
              "Pluck on 1/8 triplets, tight and dry; the pulse on 1/16 triplets underneath, washed in reverb.",
              0x4F47490701000005ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 2.0f }, { "amp_release", 400.0f },
                { "s1_scan_mode", 1.0f }, { "s1_density_sync", 9.0f }, { "s1_size_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f },
                { "s1_window", 0.7f }, { "s1_level", -6.6f },
                { "s2_enable", 1.0f }, { "s2_level", -10.4f }, { "s2_scan_mode", 1.0f }, { "s2_scan_rate", 100.0f }, { "s2_key_track", 0.0f },
                { "s2_density_sync", 6.0f }, { "s2_size_sync", 6.0f }, { "s2_jitter", 0.0f }, { "s2_spray", 0.0f },
                { "s1_reverb_mix", 5.0f },
                { "s2_reverb_mix", 45.0f }, { "s2_reverb_size", 65.0f } },
              { { 0, "factory:pluck", "Pluck" }, { 1, "factory:pulse", "Pulse" } } },

            { "Rhythmic \xc2\xb7 Gated Pad",
              "Pluck grains on 1/8, gated by a synced square LFO on the level.",
              0x4F47490701000006ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 20.0f }, { "amp_release", 800.0f },
                { "s1_scan_mode", 1.0f }, { "s1_density_sync", 10.0f }, { "s1_size_sync", 8.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f },
                { "s1_level", -8.5f }, { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 3000.0f },
                { "s*_reverb_mix", 25.0f },
                { "lfo1_shape", Square }, { "lfo1_rate_sync", 13.0f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_level") }, { "mod1_depth", 15.0f } },
              { { 0, "factory:pluck", "Pluck" } } },

            { "Rhythmic \xc2\xb7 Broken Beat",
              "The pulse loop in 1/16 grains, its read point swept by a synced 2-bar saw and lightly crushed.",
              0x4F47490701000007ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 1.0f }, { "amp_release", 200.0f },
                { "s1_scan_mode", 1.0f }, { "s1_scan_rate", 100.0f }, { "s1_key_track", 0.0f }, { "s1_density_sync", 7.0f },
                { "s1_size_sync", 7.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 30.0f }, { "s1_crush", 25.0f }, { "s1_level", -3.0f },
                { "lfo1_shape", Saw }, { "lfo1_rate_sync", 19.0f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 45.0f } },
              { { 0, "factory:pulse", "Pulse" } } },

            { "Rhythmic \xc2\xb7 Ratchet",
              "1/32 triplet ratchets of the pulse with the pluck an octave above on 1/16.",
              0x4F47490701000008ll,
              { { "sync_mode", 1.0f }, { "amp_attack", 1.0f }, { "amp_release", 250.0f },
                { "s1_scan_mode", 1.0f }, { "s1_scan_rate", 100.0f }, { "s1_key_track", 0.0f }, { "s1_density_sync", 3.0f },
                { "s1_size_sync", 4.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 0.0f }, { "s1_gain_rand", 40.0f }, { "s1_level", -4.0f },
                { "s2_enable", 1.0f }, { "s2_level", -9.0f }, { "s2_pitch", 12.0f }, { "s2_scan_mode", 1.0f }, { "s2_density_sync", 7.0f },
                { "s2_size_sync", 7.0f }, { "s2_jitter", 0.0f }, { "s2_spray", 0.0f },
                { "s*_reverb_mix", 12.0f } },
              { { 0, "factory:pulse", "Pulse" }, { 1, "factory:pluck", "Pluck" } } },

            //==================================================================
            // Textures — high density, spray 100, band-pass + filter random,
            // gain random, reverse, crush + crush probability, drive.
            //==================================================================
            { "Textures \xc2\xb7 Rain Glass",
              "Dense rain grains through a randomised band-pass, a noise bed under them.",
              0x4F47490801000001ll,
              { { "polyphony", 4.0f }, { "amp_attack", 300.0f }, { "amp_release", 2000.0f },
                { "s1_size", 60.0f }, { "s1_density", 120.0f }, { "s1_spray", 100.0f }, { "s1_gain_rand", 50.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 3500.0f }, { "s1_filter_res", 15.0f }, { "s1_filter_rand", 60.0f },
                { "s2_enable", 1.0f }, { "s2_level", 0.0f }, { "s2_size", 200.0f }, { "s2_density", 30.0f }, { "s2_spray", 100.0f },
                { "s2_filter_type", 2.0f }, { "s2_filter_cutoff", 1200.0f },
                { "s*_reverb_mix", 40.0f }, { "s*_reverb_size", 70.0f } },
              { { 0, "factory:rain_synth", "Rain" }, { 1, "factory:noise_bed", "Noise Bed" } } },

            { "Textures \xc2\xb7 Static Choir",
              "The vowel in dense, half-reversed, partly crushed grains through a wandering band-pass.",
              0x4F47490801000002ll,
              { { "polyphony", 4.0f }, { "amp_attack", 400.0f }, { "amp_release", 2000.0f },
                { "s1_size", 90.0f }, { "s1_density", 100.0f }, { "s1_spray", 100.0f }, { "s1_reverse_prob", 40.0f }, { "s1_level", 2.0f },
                { "s1_crush", 35.0f }, { "s1_crush_prob", 40.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 1500.0f }, { "s1_filter_rand", 40.0f }, { "s1_gain_rand", 30.0f },
                { "s*_reverb_mix", 30.0f } },
              { { 0, "factory:vowel", "Vowel" } } },

            { "Textures \xc2\xb7 Sand Drift",
              "The noise bed in dense high-passed grains, an LFO sliding the cutoff.",
              0x4F47490801000003ll,
              { { "polyphony", 4.0f }, { "amp_attack", 500.0f }, { "amp_release", 2500.0f },
                { "s1_size", 150.0f }, { "s1_density", 80.0f }, { "s1_spray", 100.0f }, { "s1_gain_rand", 60.0f }, { "s1_level", 6.0f },
                { "s1_filter_type", 3.0f }, { "s1_filter_cutoff", 800.0f }, { "s1_filter_rand", 30.0f },
                { "s*_reverb_mix", 25.0f },
                { "lfo1_rate", 0.1f },
                { "mod1_source", Lfo1 }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 35.0f } },
              { { 0, "factory:noise_bed", "Noise Bed" } } },

            { "Textures \xc2\xb7 Broken Radio",
              "Driven, crushed vowel grains in a band-pass, feeding back saturated and band-limited; dry dust on top.",
              0x4F47490801000004ll,
              { { "polyphony", 4.0f }, { "amp_attack", 100.0f }, { "amp_release", 1200.0f },
                { "s1_size", 80.0f }, { "s1_density", 90.0f }, { "s1_spray", 100.0f }, { "s1_level", 0.8f },
                { "s1_drive", 60.0f }, { "s1_crush", 50.0f }, { "s1_crush_prob", 70.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 2000.0f }, { "s1_filter_res", 50.0f }, { "s1_filter_rand", 30.0f },
                { "s1_ring_freq", 600.0f }, { "s1_ring_mix", 20.0f },
                { "s2_enable", 1.0f }, { "s2_level", -13.9f }, { "s2_size", 100.0f }, { "s2_density", 30.0f }, { "s2_spray", 100.0f },
                { "s1_feedback", 50.0f }, { "s1_fb_low_cut", 300.0f }, { "s1_fb_high_cut", 3000.0f }, { "s1_fb_saturation", 60.0f },
                { "s1_reverb_mix", 15.0f } },
              { { 0, "factory:vowel", "Vowel" }, { 1, "factory:dust", "Dust" } } },

            { "Textures \xc2\xb7 Ember Field",
              "Fire-crackle grains, half reversed, wildly varying in level, under a randomised low-pass.",
              0x4F47490801000005ll,
              { { "polyphony", 4.0f }, { "amp_attack", 300.0f }, { "amp_release", 2500.0f },
                { "s1_size", 110.0f }, { "s1_density", 190.0f }, { "s1_spray", 100.0f }, { "s1_reverse_prob", 50.0f }, { "s1_gain_rand", 40.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 8000.0f }, { "s1_filter_rand", 50.0f }, { "s1_level", -3.5f },
                { "s*_reverb_mix", 50.0f }, { "s*_reverb_size", 75.0f } },
              { { 0, "factory:fire", "Fire" } } },

            { "Textures \xc2\xb7 Wind Harp",
              "A resonant band-pass on the noise bed tracks the keys; the vowel an octave up breathes over it.",
              0x4F47490801000006ll,
              { { "polyphony", 4.0f }, { "amp_attack", 700.0f }, { "amp_release", 3000.0f },
                { "s1_size", 140.0f }, { "s1_density", 70.0f }, { "s1_spray", 100.0f }, { "s1_level", 2.5f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 900.0f }, { "s1_filter_res", 70.0f }, { "s1_filter_rand", 30.0f },
                { "s2_enable", 1.0f }, { "s2_level", -12.0f }, { "s2_pitch", 12.0f }, { "s2_size", 250.0f }, { "s2_density", 25.0f },
                { "s2_spray", 60.0f },
                { "s*_reverb_mix", 45.0f },
                { "mod1_source", Key }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 40.0f } },
              { { 0, "factory:noise_bed", "Noise Bed" }, { 1, "factory:vowel", "Vowel" } } },

            { "Textures \xc2\xb7 Glitter Haze",
              "The vowel two octaves up in tiny, dense, partly reversed grains through a bright band-pass.",
              0x4F47490801000007ll,
              { { "polyphony", 4.0f }, { "amp_attack", 400.0f }, { "amp_release", 2500.0f },
                { "s1_pitch", 24.0f }, { "s1_size", 35.0f }, { "s1_density", 140.0f }, { "s1_spray", 100.0f }, { "s1_reverse_prob", 30.0f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 6000.0f }, { "s1_filter_rand", 50.0f }, { "s1_level", 2.7f },
                { "s1_crush", 20.0f }, { "s1_crush_prob", 30.0f }, { "s1_gain_rand", 40.0f },
                { "s*_reverb_mix", 55.0f }, { "s*_reverb_size", 80.0f } },
              { { 0, "factory:vowel", "Vowel" } } },

            { "Textures \xc2\xb7 Rust Cloud",
              "Noise and dust through a wandering low-pass: the noise driven hard and crushed on every grain, the dust driven.",
              0x4F47490801000008ll,
              { { "polyphony", 4.0f }, { "amp_attack", 300.0f }, { "amp_release", 2000.0f },
                { "s1_size", 120.0f }, { "s1_density", 70.0f }, { "s1_spray", 100.0f }, { "s1_level", -9.3f },
                { "s1_drive", 70.0f }, { "s1_crush", 40.0f }, { "s1_crush_prob", 100.0f }, { "s1_reverse_prob", 20.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 2500.0f }, { "s1_filter_rand", 60.0f }, { "s1_gain_rand", 40.0f },
                { "s2_enable", 1.0f }, { "s2_level", -10.0f }, { "s2_size", 80.0f }, { "s2_density", 40.0f }, { "s2_spray", 100.0f },
                { "s2_drive", 40.0f },
                { "s*_reverb_mix", 25.0f } },
              { { 0, "factory:noise_bed", "Noise Bed" }, { 1, "factory:dust", "Dust" } } },
        };
    }

    /** PLAN-R1 PD27: every "s*_<suffix>" pair becomes the four "sN_<suffix>"
        pairs, in place and in stream order. The suffix must be a row of
        GiParamIds::streamDefs(); any other wildcard is left as written (and
        validateSpec then refuses it as an unknown key). The expanded ids live
        in a table built once (a spec holds const char*). */
    inline void expandSpecValues (std::vector<std::pair<const char*, float>>& values)
    {
        static const std::vector<std::array<std::string, 4>> table = []
        {
            std::vector<std::array<std::string, 4>> t;
            for (const auto& d : GiParamIds::streamDefs())
            {
                std::array<std::string, 4> row;
                for (int n = 1; n <= 4; ++n)
                    row[(size_t) (n - 1)] = "s" + std::to_string (n) + "_" + d.id;
                t.push_back (std::move (row));
            }
            return t;
        }();

        std::vector<std::pair<const char*, float>> out;
        out.reserve (values.size() + 24);
        for (const auto& [id, v] : values)
        {
            const std::array<std::string, 4>* row = nullptr;
            if (std::strncmp (id, "s*_", 3) == 0)
            {
                size_t k = 0;
                for (const auto& d : GiParamIds::streamDefs())
                {
                    if (std::strcmp (d.id, id + 3) == 0)
                        row = &table[k];
                    ++k;
                }
            }
            if (row == nullptr)
                out.emplace_back (id, v);
            else
                for (const auto& full : *row)
                    out.emplace_back (full.c_str(), v);
        }
        values = std::move (out);
    }

    /** Validation (PD2): no output_gain / rec_arm, every key a real parameter,
        every value inside its range, a seed, every slot a known factory id at
        index 0–3 (unique), every enabled stream with a listed slot (Live
        templates exempt). Returns the first problem, or an empty string. */
    inline juce::String validateSpec (const FactoryPresetSpec& spec, juce::AudioProcessorValueTreeState& apvts)
    {
        auto s = spec;
        expandSpecValues (s.values);   // PLAN-R1 PD27
        const auto who = juce::String::fromUTF8 (s.name) + ": ";
        if (s.seed == 0)
            return who + "no seed";
        float enabled[4] = { 1.0f, 0.0f, 0.0f, 0.0f };   // s1_enable defaults On
        for (const auto& [id, v] : s.values)
        {
            const juce::String key (id);
            if (key == "output_gain" || key == "rec_arm")
                return who + key;
            auto* rp = apvts.getParameter (key);
            if (rp == nullptr)
                return who + "unknown " + key;
            const auto& r = rp->getNormalisableRange();
            if (! (v >= r.start && v <= r.end))
                return who + key + " out of range";
            for (int n = 1; n <= 4; ++n)
                if (key == "s" + juce::String (n) + "_enable")
                    enabled[n - 1] = v;
        }
        bool listed[4] = { false, false, false, false };
        for (const auto& sl : s.slots)
        {
            if (s.live)
                return who + "a Live template lists a slot";
            if (sl.index < 0 || sl.index > 3 || listed[sl.index])
                return who + "bad slot index";
            listed[sl.index] = true;
            const juce::String src (sl.source);
            ogi::FactoryEntry e;
            if (! src.startsWith ("factory:") || ! ogi::findFactory (src.substring (8), e) || juce::String (sl.name).isEmpty())
                return who + "unknown source " + src;
        }
        if (! s.live)
            for (int n = 0; n < 4; ++n)
                if (enabled[n] > 0.5f && ! listed[n])
                    return who + "stream " + juce::String (n + 1) + " enabled without a listed slot";
        return {};
    }

    /** The customState for a spec: {"seed": "<int64>", custom?, masks?, slots?}.
        Slots carry index / source / name only (no contentHash, no blob); a
        Live template has no `slots` key at all. */
    inline juce::var customStateOf (const FactoryPresetSpec& s)
    {
        auto* o = new juce::DynamicObject();
        // Keep the parsed var alive while its object is read (a pointer taken
        // from the temporary parse() result would dangle in the loop below).
        const auto extraVar = s.customJson.isNotEmpty() ? juce::JSON::parse (s.customJson) : juce::var();
        if (auto* extra = extraVar.getDynamicObject())
            for (const auto& p : extra->getProperties())
                o->setProperty (p.name, p.value);
        o->setProperty ("seed", juce::String (s.seed));
        if (! s.live)
        {
            juce::Array<juce::var> list;
            for (const auto& sl : s.slots)
            {
                auto* e = new juce::DynamicObject();
                e->setProperty ("index", sl.index);
                e->setProperty ("source", juce::String (sl.source));
                e->setProperty ("name", juce::String::fromUTF8 (sl.name));
                list.add (juce::var (e));
            }
            o->setProperty ("slots", list);
        }
        return juce::var (o);
    }

    /** Engineering units -> the manager's normalised defs (one convertTo0to1 per value). */
    inline std::vector<OuariconPresetManager::FactoryPresetDef> makeFactoryPresets (juce::AudioProcessorValueTreeState& apvts)
    {
        std::vector<OuariconPresetManager::FactoryPresetDef> defs;
        for (auto& s : factoryPresetSpecs())
        {
            expandSpecValues (s.values);   // PLAN-R1 PD27
            jassert (validateSpec (s, apvts).isEmpty());
            OuariconPresetManager::FactoryPresetDef d;
            d.name = juce::String::fromUTF8 (s.name);
            for (const auto& [id, v] : s.values)
                if (auto* rp = apvts.getParameter (id))
                    d.parameters[id] = rp->convertTo0to1 (v);
            d.customState = customStateOf (s);
            defs.push_back (std::move (d));
        }
        return defs;
    }

    /** The preset JSON as the manager writes it (minus the version / plugin /
        factory metadata, which apply ignores). Harness + --dump-factory-presets. */
    inline juce::var presetJsonOf (const OuariconPresetManager::FactoryPresetDef& d)
    {
        auto* root = new juce::DynamicObject();
        auto* params = new juce::DynamicObject();
        for (const auto& [id, v] : d.parameters)
            params->setProperty (id, v);
        root->setProperty ("name", d.name);
        root->setProperty ("parameters", juce::var (params));
        root->setProperty ("customState", d.customState);
        return juce::var (root);
    }

    /** PD3: the bank stamp — XXH64 over the canonical serialisation of the 64
        defs (name, parameters in id order with the float's bits, customState
        JSON text), 16 lowercase hex digits. Any edit to the bank moves it. */
    inline juce::String bankHash (const std::vector<OuariconPresetManager::FactoryPresetDef>& defs)
    {
        juce::MemoryOutputStream mo;
        for (const auto& d : defs)
        {
            mo << d.name << "\n";
            std::vector<std::pair<juce::String, float>> ps (d.parameters.begin(), d.parameters.end());
            std::sort (ps.begin(), ps.end(), [] (const auto& a, const auto& b) { return a.first < b.first; });
            for (const auto& [id, v] : ps)
            {
                std::uint32_t bits = 0;
                std::memcpy (&bits, &v, sizeof (bits));
                mo << id << "=" << juce::String::toHexString ((int) bits) << ";";
            }
            mo << "\n" << juce::JSON::toString (d.customState, true) << "\n";
        }
        return ogi::gihash::toHex (ogi::gihash::xxh64 (mo.getData(), mo.getDataSize(), 0));
    }
} // namespace ogi::factory
