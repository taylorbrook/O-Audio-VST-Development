/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
#include "PitchCollections.h"   // pitch-collections 1.0.0 (modules/tuning/pitch-collections/cpp)

namespace PitchCollections = ouaricon::tuning::PitchCollections;

// ============================================================================
// O-Granulator parameter contract — .planning/parameter-spec.md (LOCKED).
//
// 220 parameters (140 Float / 59 Choice / 21 Bool), registered in the spec's
// "Full ID list" order. IDs are generated from the FROZEN suffix tables below;
// the 113-entry modulation-destination table is a LITERAL on purpose (a diff
// shows an append; a generated table would hide a re-order).
//
// Goldens: tests/golden/{params,mod-dests}.tsv are extracted from the SPEC by
// tests/golden/extract_spec.py, never from this file; O-Granulator-render-harness
// checks this file against them (rows G1-G6, D1-D4, P1).
// ============================================================================

namespace GranulatorParamIds
{
    inline constexpr int kStreams = 4, kLfos = 4, kMacros = 4, kModSlots = 16;
    inline constexpr int kModDests = 113;
    inline constexpr int kModDestTableVersion = 1;
    inline constexpr int kStateVersion = 1;
    inline constexpr int kNumCollections = 32;   // sN_scale choices: 0 Off … 31 Custom

    enum class Kind { Float, Choice, Bool };

    /** One row of the contract. Float: start/end/def/skew (or centre > 0 →
        setSkewForCentre). Choice: choices + def (index). Bool: def (0/1). */
    struct Def
    {
        const char* id;      // full ID, or the suffix for templated groups
        const char* name;    // host name, or the suffix part ("Level" → "S1 · Level")
        Kind kind;
        float start = 0.0f, end = 1.0f, def = 0.0f, skew = 1.0f, centre = 0.0f;
        const juce::StringArray* choices = nullptr;
    };

    // ── Choice lists (exact strings, exact order) ───────────────────────────
    inline const juce::StringArray& syncModes()   { static const juce::StringArray a { "Free", "Tempo" }; return a; }
    inline const juce::StringArray& noteNames()   { static const juce::StringArray a { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return a; }
    inline const juce::StringArray& grainLimits() { static const juce::StringArray a { "32", "64", "96", "128" }; return a; }
    inline const juce::StringArray& filterTypes() { static const juce::StringArray a { "Off", "LP", "BP", "HP" }; return a; }
    inline const juce::StringArray& lfoShapes()   { static const juce::StringArray a { "Sine", "Triangle", "Saw", "Square", "S&H", "Smooth Random" }; return a; }
    inline const juce::StringArray& modSources()
    {
        static const juce::StringArray a { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Env", "Pitch", "Onset",
                                           "Macro 1", "Macro 2", "Macro 3", "Macro 4" };
        return a;
    }

    /** Tempo divisions, first n of the 22-entry list (size_sync 20, density_sync 19, rate_sync 22). */
    inline juce::StringArray tempoDivisions (int n)
    {
        const juce::StringArray all { "1/64 T", "1/64", "1/64 D", "1/32 T", "1/32", "1/32 D", "1/16 T", "1/16", "1/16 D",
                                      "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2 T", "1/2", "1/2 D",
                                      "1 bar", "2 bars", "4 bars", "8 bars" };
        juce::StringArray a;
        for (int i = 0; i < n; ++i) a.add (all[i]);
        return a;
    }
    inline const juce::StringArray& sizeSyncs()    { static const juce::StringArray a = tempoDivisions (20); return a; }
    inline const juce::StringArray& densitySyncs() { static const juce::StringArray a = tempoDivisions (19); return a; }
    inline const juce::StringArray& lfoRateSyncs() { static const juce::StringArray a = tempoDivisions (22); return a; }

    /** sN_scale: 0 Off, then PitchCollections without GS's "Free" (Id 5), Custom last. */
    inline const juce::StringArray& scaleNames()
    {
        static const juce::StringArray a {
            "Off", "Chromatic", "Major", "Minor", "Pentatonic", "Whole Tone",
            "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian",
            "Harmonic Minor", "Melodic Minor", "Minor Pentatonic", "Blues",
            "Octatonic", "Augmented", "Fifths + Octaves", "Octaves",
            "19-EDO", "22-EDO", "24-EDO", "31-EDO", "53-EDO",
            "5-Limit Just", "7-Limit Just", "Harmonics 8-16",
            "Bohlen-Pierce", "Slendro", "Pelog", "Maqam Rast", "Custom" };
        return a;
    }

    /** Choice index → PitchCollections::Id at the parameter boundary. −1 = Off. */
    static_assert (PitchCollections::WholeTone == 4 && PitchCollections::Free == 5
                   && PitchCollections::Dorian == 6 && PitchCollections::Custom == 31
                   && PitchCollections::kCount == kNumCollections,
                   "pitch-collections Id order is frozen; the Granulator scale map depends on it");
    inline constexpr int scaleChoiceToId (int c) noexcept { return c <= 0 ? -1 : (c <= 5 ? c - 1 : c); }
    static_assert (scaleChoiceToId (0) == -1 && scaleChoiceToId (1) == PitchCollections::Chromatic
                   && scaleChoiceToId (5) == PitchCollections::WholeTone && scaleChoiceToId (6) == PitchCollections::Dorian
                   && scaleChoiceToId (31) == PitchCollections::Custom, "scale map");

    // ── Global / buffer (8) ─────────────────────────────────────────────────
    inline const std::array<Def, 8>& globalDefs()
    {
        static const std::array<Def, 8> d {{
            { "input_gain",    "Input Gain",    Kind::Float, -24.0f, 12.0f, 0.0f },
            { "buffer_length", "Buffer Length", Kind::Float,   1.0f, 30.0f, 8.0f, 0.5f },
            { "freeze",        "Freeze",        Kind::Bool },
            { "sync_mode",     "Sync Mode",     Kind::Choice, 0, 1, 0, 1, 0, &syncModes() },
            { "mix",           "Mix",           Kind::Float,   0.0f, 100.0f, 50.0f },
            { "output_gain",   "Output Gain",   Kind::Float, -24.0f, 12.0f, 0.0f },
            { "scale_root",    "Scale Root",    Kind::Choice, 0, 1, 0, 1, 0, &noteNames() },
            { "grain_limit",   "Grain Limit",   Kind::Choice, 0, 1, 3, 1, 0, &grainLimits() },
        }};
        return d;
    }

    // ── Per stream: sN_ + suffix, 32 × 4. FROZEN order. ─────────────────────
    // s1_enable defaults On, s2…s4 Off (special-cased in the layout).
    inline const std::array<Def, 32>& streamDefs()
    {
        static const std::array<Def, 32> d {{
            { "enable",         "Enable",         Kind::Bool },
            { "mute",           "Mute",           Kind::Bool },
            { "solo",           "Solo",           Kind::Bool },
            { "level",          "Level",          Kind::Float,  -60.0f,     6.0f,     0.0f },
            { "pan",            "Pan",            Kind::Float, -100.0f,   100.0f,     0.0f },
            { "position",       "Position",       Kind::Float,    0.0f,   100.0f,     0.0f },
            { "spray",          "Spray",          Kind::Float,    0.0f,   100.0f,    10.0f },
            { "size",           "Size",           Kind::Float,    5.0f,  2000.0f,   100.0f, 0.3f },
            { "size_sync",      "Size Sync",      Kind::Choice, 0, 1, 7, 1, 0, &sizeSyncs() },
            { "size_rand",      "Size Random",    Kind::Float,    0.0f,   100.0f,     0.0f },
            { "density",        "Density",        Kind::Float,    0.5f,   200.0f,    10.0f, 0.3f },
            { "density_sync",   "Density Sync",   Kind::Choice, 0, 1, 7, 1, 0, &densitySyncs() },
            { "jitter",         "Jitter",         Kind::Float,    0.0f,   100.0f,    20.0f },
            { "pitch",          "Pitch",          Kind::Float,  -24.0f,    24.0f,     0.0f },
            { "pitch_rand",     "Pitch Random",   Kind::Float,    0.0f,    24.0f,     0.0f },
            { "scale",          "Scale",          Kind::Choice, 0, 1, 0, 1, 0, &scaleNames() },
            { "reverse_prob",   "Reverse",        Kind::Float,    0.0f,   100.0f,     0.0f },
            { "window",         "Window",         Kind::Float,    0.0f,     1.0f,     0.0f },
            { "window_skew",    "Window Skew",    Kind::Float, -100.0f,   100.0f,     0.0f },
            { "stereo_spread",  "Stereo Spread",  Kind::Float,    0.0f,   100.0f,    30.0f },
            { "gain_rand",      "Gain Random",    Kind::Float,    0.0f,   100.0f,     0.0f },
            { "onset_trigger",  "Onset Trigger",  Kind::Bool },
            { "filter_type",    "Filter Type",    Kind::Choice, 0, 1, 0, 1, 0, &filterTypes() },
            { "filter_cutoff",  "Filter Cutoff",  Kind::Float,   20.0f, 20000.0f, 20000.0f, 1.0f, 632.4555f },
            { "filter_rand",    "Filter Random",  Kind::Float,    0.0f,   100.0f,     0.0f },
            { "filter_res",     "Filter Res",     Kind::Float,    0.0f,   100.0f,    20.0f },
            { "drive",          "Drive",          Kind::Float,    0.0f,   100.0f,     0.0f },
            { "crush",          "Crush",          Kind::Float,    0.0f,   100.0f,     0.0f },
            { "crush_prob",     "Crush Chance",   Kind::Float,    0.0f,   100.0f,   100.0f },
            { "ring_freq",      "Ring Freq",      Kind::Float,   20.0f,  5000.0f,   200.0f, 1.0f, 316.2278f },
            { "ring_rand",      "Ring Random",    Kind::Float,    0.0f,   100.0f,     0.0f },
            { "ring_mix",       "Ring Mix",       Kind::Float,    0.0f,   100.0f,     0.0f },
        }};
        return d;
    }

    // ── Feedback network (5) + output reverb (3) ────────────────────────────
    inline const std::array<Def, 8>& feedbackReverbDefs()
    {
        static const std::array<Def, 8> d {{
            { "feedback",      "Feedback",      Kind::Float,    0.0f,   100.0f,     0.0f },
            { "fb_low_cut",    "FB Low Cut",    Kind::Float,   20.0f,  2000.0f,    80.0f, 1.0f, 200.0f },
            { "fb_high_cut",   "FB High Cut",   Kind::Float, 1000.0f, 20000.0f, 12000.0f, 1.0f, 4472.136f },
            { "fb_saturation", "FB Saturation", Kind::Float,    0.0f,   100.0f,    20.0f },
            { "fb_diffusion",  "FB Diffusion",  Kind::Float,    0.0f,   100.0f,     0.0f },
            { "reverb_mix",    "Reverb Mix",    Kind::Float,    0.0f,   100.0f,     0.0f },
            { "reverb_size",   "Reverb Size",   Kind::Float,    0.0f,   100.0f,    50.0f },
            { "reverb_damp",   "Reverb Damp",   Kind::Float,    0.0f,   100.0f,    50.0f },
        }};
        return d;
    }

    // ── LFOs: lfoN_ + suffix, 4 × 4 ─────────────────────────────────────────
    inline const std::array<Def, 4>& lfoDefs()
    {
        static const std::array<Def, 4> d {{
            { "shape",     "Shape",     Kind::Choice, 0, 1, 0, 1, 0, &lfoShapes() },
            { "rate",      "Rate",      Kind::Float,  0.01f,  20.0f, 0.5f, 0.3f },
            { "rate_sync", "Rate Sync", Kind::Choice, 0, 1, 18, 1, 0, &lfoRateSyncs() },
            { "phase",     "Phase",     Kind::Float,  0.0f,  360.0f, 0.0f },
        }};
        return d;
    }

    // ── Envelope follower (3), pitch follow (×4), onset (1), macros (×4) ────
    inline const std::array<Def, 3>& envDefs()
    {
        static const std::array<Def, 3> d {{
            { "env_attack",  "Env Attack",  Kind::Float,   1.0f,  500.0f,  10.0f, 0.4f },
            { "env_release", "Env Release", Kind::Float,  10.0f, 2000.0f, 200.0f, 0.4f },
            { "env_gain",    "Env Gain",    Kind::Float, -24.0f,   24.0f,   0.0f },
        }};
        return d;
    }
    inline const Def& onsetThresholdDef() { static const Def d { "onset_threshold", "Onset Threshold", Kind::Float, -60.0f, 0.0f, -30.0f }; return d; }
    inline const Def& macroDef()          { static const Def d { "macro", "Macro", Kind::Float, 0.0f, 100.0f, 0.0f }; return d; }

    // ── Modulation matrix: modM_ + suffix, 3 × 16 (dest choices built below) ─
    inline const std::array<Def, 3>& modDefs()
    {
        static const std::array<Def, 3> d {{
            { "source", "Source", Kind::Choice, 0, 1, 0, 1, 0, &modSources() },
            { "dest",   "Dest",   Kind::Choice, 0, 1, 0, 1, 0, nullptr },   // modDestChoices()
            { "depth",  "Depth",  Kind::Float, -100.0f, 100.0f, 0.0f },
        }};
        return d;
    }

    // ── ID helpers ──────────────────────────────────────────────────────────
    inline juce::String stream (int n, const char* s) { return "s" + juce::String (n) + "_" + s; }
    inline juce::String lfo (int n, const char* s)    { return "lfo" + juce::String (n) + "_" + s; }
    inline juce::String mod (int m, const char* s)    { return "mod" + juce::String (m) + "_" + s; }
    inline juce::String macro (int n)                 { return "macro" + juce::String (n); }
    inline juce::String pitchFollow (int n)           { return "pitch_follow_s" + juce::String (n); }

    /** One registration entry: full ID, host name, and its contract row. */
    struct Entry
    {
        juce::String id, name;
        const Def* def;
        int instance;   // stream / LFO / macro / slot number, 0 for globals
    };

    /** Every parameter in registration order (= parameter-spec.md "Full ID list"). */
    inline const std::vector<Entry>& registrationOrder()
    {
        static const std::vector<Entry> entries = []
        {
            static const Def pitchFollowDef { "pitch_follow", "Pitch Follow", Kind::Bool };
            std::vector<Entry> e;
            for (auto& d : globalDefs()) e.push_back ({ d.id, d.name, &d, 0 });
            for (int n = 1; n <= kStreams; ++n)
                for (auto& d : streamDefs())
                    e.push_back ({ stream (n, d.id), "S" + juce::String (n) + " · " + d.name, &d, n });
            for (auto& d : feedbackReverbDefs()) e.push_back ({ d.id, d.name, &d, 0 });
            for (int n = 1; n <= kLfos; ++n)
                for (auto& d : lfoDefs())
                    e.push_back ({ lfo (n, d.id), "LFO " + juce::String (n) + " · " + d.name, &d, n });
            for (auto& d : envDefs()) e.push_back ({ d.id, d.name, &d, 0 });
            for (int n = 1; n <= kStreams; ++n)
                e.push_back ({ pitchFollow (n), "Pitch Follow S" + juce::String (n), &pitchFollowDef, n });
            e.push_back ({ onsetThresholdDef().id, onsetThresholdDef().name, &onsetThresholdDef(), 0 });
            for (int n = 1; n <= kMacros; ++n)
                e.push_back ({ macro (n), "Macro " + juce::String (n), &macroDef(), n });
            for (int m = 1; m <= kModSlots; ++m)
                for (auto& d : modDefs())
                    e.push_back ({ mod (m, d.id), "Mod " + juce::String (m) + " · " + d.name, &d, m });
            jassert (e.size() == 220);
            return e;
        }();
        return entries;
    }

    inline const juce::StringArray& allIdsInOrder()
    {
        static const juce::StringArray ids = []
        {
            juce::StringArray a;
            for (auto& e : registrationOrder()) a.add (e.id);
            return a;
        }();
        return ids;
    }

    // ── Modulation destination table — APPEND-ONLY. modM_dest stores the index.
    // Never re-order or remove; a new destination gets index 113, 114, …, and
    // kModDestTableVersion is bumped. Every entry ≠ None is a Float parameter.
    inline constexpr const char* kModDestIds[kModDests] = {
        "None",
        "input_gain", "mix", "output_gain", "buffer_length",
        // s1 (5…28)
        "s1_level", "s1_pan", "s1_position", "s1_spray", "s1_size", "s1_size_rand", "s1_density", "s1_jitter",
        "s1_pitch", "s1_pitch_rand", "s1_reverse_prob", "s1_window", "s1_window_skew", "s1_stereo_spread",
        "s1_gain_rand", "s1_filter_cutoff", "s1_filter_rand", "s1_filter_res", "s1_drive", "s1_crush",
        "s1_crush_prob", "s1_ring_freq", "s1_ring_rand", "s1_ring_mix",
        // s2 (29…52)
        "s2_level", "s2_pan", "s2_position", "s2_spray", "s2_size", "s2_size_rand", "s2_density", "s2_jitter",
        "s2_pitch", "s2_pitch_rand", "s2_reverse_prob", "s2_window", "s2_window_skew", "s2_stereo_spread",
        "s2_gain_rand", "s2_filter_cutoff", "s2_filter_rand", "s2_filter_res", "s2_drive", "s2_crush",
        "s2_crush_prob", "s2_ring_freq", "s2_ring_rand", "s2_ring_mix",
        // s3 (53…76)
        "s3_level", "s3_pan", "s3_position", "s3_spray", "s3_size", "s3_size_rand", "s3_density", "s3_jitter",
        "s3_pitch", "s3_pitch_rand", "s3_reverse_prob", "s3_window", "s3_window_skew", "s3_stereo_spread",
        "s3_gain_rand", "s3_filter_cutoff", "s3_filter_rand", "s3_filter_res", "s3_drive", "s3_crush",
        "s3_crush_prob", "s3_ring_freq", "s3_ring_rand", "s3_ring_mix",
        // s4 (77…100)
        "s4_level", "s4_pan", "s4_position", "s4_spray", "s4_size", "s4_size_rand", "s4_density", "s4_jitter",
        "s4_pitch", "s4_pitch_rand", "s4_reverse_prob", "s4_window", "s4_window_skew", "s4_stereo_spread",
        "s4_gain_rand", "s4_filter_cutoff", "s4_filter_rand", "s4_filter_res", "s4_drive", "s4_crush",
        "s4_crush_prob", "s4_ring_freq", "s4_ring_rand", "s4_ring_mix",
        // feedback + reverb (101…108)
        "feedback", "fb_low_cut", "fb_high_cut", "fb_saturation", "fb_diffusion",
        "reverb_mix", "reverb_size", "reverb_damp",
        // LFO rates (109…112)
        "lfo1_rate", "lfo2_rate", "lfo3_rate", "lfo4_rate",
    };

    /** modM_dest choices: "None", then each destination's host name. */
    inline const juce::StringArray& modDestChoices()
    {
        static const juce::StringArray a = []
        {
            juce::StringArray c { "None" };
            const auto& order = registrationOrder();
            for (int k = 1; k < kModDests; ++k)
            {
                const juce::String id (kModDestIds[k]);
                juce::String name;
                for (auto& e : order)
                    if (e.id == id) { name = e.name; break; }
                jassert (name.isNotEmpty());
                c.add (name);
            }
            return c;
        }();
        return a;
    }
}
