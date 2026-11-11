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
#include <array>
#include <cstring>
#include <vector>
#include "PitchCollections.h"   // pitch-collections 1.0.0 (modules/tuning/pitch-collections/cpp)

namespace PitchCollections = ouaricon::tuning::PitchCollections;

// ============================================================================
// O-GranulatorInstrument parameter contract: .planning/parameter-spec.md (LOCKED).
//
// 261 parameters (176 Float / 70 Choice / 15 Bool), registered in the spec's
// "Full ID list" order. IDs are generated from the FROZEN suffix tables below
// (sN_, menvN_, lfoN_, macroN, modM_). The 126-entry modulation destination
// table and the 18-entry source table are LITERALS on purpose: a diff shows an
// append, a generated table would hide a re-order.
//
// Float ranges are start / end / interval / skew only. A row with centre > 0
// gets setSkewForCentre (centre) in the layout loop. Never a lambda-mapped
// range: the WebView slider frontend sees only start / end / skew / interval.
// Interval is 1 on polyphony, pitch_bend_range and sN_root_note (6 rows, all
// with start >= 0) and 0 everywhere else, including every negative-start row
// (a stepped range with a negative start snaps a 0 default off zero).
// PLAN-4 PD8 (D55): six 0 dB-default rows (output_gain, rec_gain, s1-s4_level)
// get a SNAP-ONLY range (exactZeroDbRange): no interval, skew 1, null convert
// functions (so the WebView still sees plain start / end / skew / interval);
// the snap clamps to [start, end], then maps |v| < 1e-5 to exactly 0. Without
// it, arm64 fuses start + (end - start) * p into an FMA and the 0 dB default
// reads 7.15e-07 (output_gain, rec_gain) / 1.8e-06 (levels). The audio was
// already exact (ControlGrid::snapDb). The APVTS raw atomic is still seeded
// UNSNAPPED at construction (JUCE: "should not be snapped"), so it reads the
// FMA value until the first set; get() and every setter path are exact.
//
// Host names that contain the middle dot are built by concatenation with
// dot() (UTF-8 explicitly); a raw char literal holding it would go through
// the ASCII-only String constructor.
//
// Goldens: tests/golden/{params,mod-dests,mod-sources}.tsv are extracted from
// the SPEC by tests/golden/extract_spec.py, never from this file; the render
// harness checks this file against them (rows G1-G6, D1-D5, SRC1).
// ============================================================================

namespace GiParamIds
{
    inline constexpr int kStreams = 4, kModEnvs = 2, kLfos = 4, kMacros = 4, kModSlots = 16;
    inline constexpr int kModDests = 126;
    inline constexpr int kNumModSources = 18;
    inline constexpr int kModDestTableVersion = 1;
    inline constexpr int kStateVersion = 1;
    inline constexpr int kNumCollections = 32;   // sN_scale choices: 0 Off ... 31 Custom
    inline constexpr int kSlots = 4;              // Record-in / file slots (rec_target index 0...3)

    // Group sizes in contract order.
    inline constexpr int kGlobalCount = 9, kAmpCount = 5, kModEnvCount = 4, kStreamCount = 38, kRecCount = 7,
                         kFeedbackCount = 5, kReverbCount = 3, kLfoCount = 5, kModCount = 3;
    inline constexpr int kNumParams = kGlobalCount + kAmpCount + kModEnvCount * kModEnvs + kStreamCount * kStreams
                                    + kRecCount + kFeedbackCount + kReverbCount + kLfoCount * kLfos + kMacros
                                    + kModCount * kModSlots;
    static_assert (kNumParams == 261, "parameter-spec.md: 261 parameters");

    enum class Kind { Float, Choice, Bool };

    /** One row of the contract. Float: lo / hi (range start / end) / def / skew
        (or centre > 0 -> setSkewForCentre) / interval. Choice: choices + def
        (index). Bool: def (0 / 1). Built only through floatDef / choiceDef /
        boolDef below, so every field is always initialised. */
    struct Def
    {
        const char* id;      // full ID, or the suffix for templated groups
        const char* name;    // host name, or the suffix part ("Level" -> "S1 . Level")
        Kind kind;
        float lo, hi, def, skew, centre, interval;
        const juce::StringArray* choices;   // nullptr on modM_dest: modDestChoices()
    };

    inline Def floatDef (const char* id, const char* name, float lo, float hi, float def,
                         float skew = 1.0f, float centre = 0.0f, float interval = 0.0f)
    {
        return Def { id, name, Kind::Float, lo, hi, def, skew, centre, interval, nullptr };
    }
    inline Def choiceDef (const char* id, const char* name, const juce::StringArray* choices, int defIndex)
    {
        return Def { id, name, Kind::Choice, 0.0f, 1.0f, static_cast<float> (defIndex), 1.0f, 0.0f, 0.0f, choices };
    }
    inline Def boolDef (const char* id, const char* name, bool def)
    {
        return Def { id, name, Kind::Bool, 0.0f, 1.0f, def ? 1.0f : 0.0f, 1.0f, 0.0f, 0.0f, nullptr };
    }

    /** PLAN-4 PD8: the six rows whose 0 dB default must read exactly 0. */
    inline bool isExactZeroDbRow (const juce::String& fullId)
    {
        return fullId == "output_gain" || fullId == "rec_gain" || fullId == "s1_level" || fullId == "s2_level"
            || fullId == "s3_level" || fullId == "s4_level";
    }

    /** PLAN-4 PD8: start / end, skew 1, interval 0, null convert functions; the
        snap replaces JUCE's clamp, so it clamps first, then |v| < 1e-5 -> 0. */
    inline juce::NormalisableRange<float> exactZeroDbRange (float lo, float hi)
    {
        return juce::NormalisableRange<float> (lo, hi, nullptr, nullptr, [] (float start, float end, float v)
        {
            const float c = juce::jlimit (start, end, v);
            return std::abs (c) < 1.0e-5f ? 0.0f : c;
        });
    }

    /** " . " with the U+00B7 middle dot, decoded as UTF-8. */
    inline const juce::String& dot()
    {
        static const juce::String s (juce::CharPointer_UTF8 (" \xc2\xb7 "));
        return s;
    }

    // ── Choice lists (exact strings, exact order) ───────────────────────────
    inline const juce::StringArray& voiceModes()   { static const juce::StringArray a { "Poly", "Mono", "Legato" }; return a; }
    inline const juce::StringArray& grainLimits()  { static const juce::StringArray a { "32", "64", "128", "256" }; return a; }
    inline const juce::StringArray& syncModes()    { static const juce::StringArray a { "Free", "Tempo" }; return a; }
    inline const juce::StringArray& scanModes()    { static const juce::StringArray a { "Free-run", "Retrigger", "Ping-pong" }; return a; }
    inline const juce::StringArray& filterTypes()  { static const juce::StringArray a { "Off", "LP", "BP", "HP" }; return a; }
    inline const juce::StringArray& recTargets()   { static const juce::StringArray a { "Slot 1", "Slot 2", "Slot 3", "Slot 4" }; return a; }
    inline const juce::StringArray& recTriggers()  { static const juce::StringArray a { "Manual", "Threshold", "Next Note" }; return a; }
    inline const juce::StringArray& recLengthSyncs()
    {
        static const juce::StringArray a { "1 bar", "2 bars", "4 bars", "8 bars", "16 bars", "32 bars" };
        return a;
    }
    inline const juce::StringArray& lfoShapes()    { static const juce::StringArray a { "Sine", "Triangle", "Saw", "Square", "S&H", "Smooth Random" }; return a; }
    inline const juce::StringArray& lfoModes()     { static const juce::StringArray a { "Global", "Per-voice" }; return a; }

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

    /** sN_scale: 0 Off, then PitchCollections without GS's "Free" (Id 5), Custom last
        (unchanged from O-Granulator). */
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

    /** Choice index -> PitchCollections::Id at the parameter boundary. -1 = Off. */
    static_assert (PitchCollections::WholeTone == 4 && PitchCollections::Free == 5
                   && PitchCollections::Dorian == 6 && PitchCollections::Custom == 31
                   && PitchCollections::kCount == kNumCollections,
                   "pitch-collections Id order is frozen; the scale map depends on it");
    inline constexpr int scaleChoiceToId (int c) noexcept { return c <= 0 ? -1 : (c <= 5 ? c - 1 : c); }
    static_assert (scaleChoiceToId (0) == -1 && scaleChoiceToId (1) == PitchCollections::Chromatic
                   && scaleChoiceToId (5) == PitchCollections::WholeTone && scaleChoiceToId (6) == PitchCollections::Dorian
                   && scaleChoiceToId (31) == PitchCollections::Custom, "scale map");

    // ── Modulation sources: APPEND-ONLY, ORDER FROZEN (modM_source stores the index)
    enum class SourceClass { None, PerVoice, Global, LfoMode };   // LfoMode: per-voice or global per lfoN_mode

    struct ModSource
    {
        const char* name;
        SourceClass cls;
    };

    inline constexpr ModSource kModSources[kNumModSources] = {
        { "None",        SourceClass::None },
        { "LFO 1",       SourceClass::LfoMode },
        { "LFO 2",       SourceClass::LfoMode },
        { "LFO 3",       SourceClass::LfoMode },
        { "LFO 4",       SourceClass::LfoMode },
        { "Mod Env 1",   SourceClass::PerVoice },
        { "Mod Env 2",   SourceClass::PerVoice },
        { "Velocity",    SourceClass::PerVoice },
        { "Key",         SourceClass::PerVoice },
        { "Note Random", SourceClass::PerVoice },
        { "Mod Wheel",   SourceClass::Global },
        { "Pressure",    SourceClass::PerVoice },
        { "Slide",       SourceClass::PerVoice },
        { "Bend",        SourceClass::PerVoice },
        { "Macro 1",     SourceClass::Global },
        { "Macro 2",     SourceClass::Global },
        { "Macro 3",     SourceClass::Global },
        { "Macro 4",     SourceClass::Global },
    };
    static_assert (std::size (kModSources) == 18, "parameter-spec.md: 18 modulation sources");

    /** The golden's class token for a source (mod-sources.tsv column 3). */
    inline const char* sourceClassToken (SourceClass c) noexcept
    {
        switch (c)
        {
            case SourceClass::None:     return "none";
            case SourceClass::PerVoice: return "per-voice";
            case SourceClass::Global:   return "global";
            case SourceClass::LfoMode:  return "lfo-mode";
        }
        return "";
    }

    inline const juce::StringArray& modSources()
    {
        static const juce::StringArray a = []
        {
            juce::StringArray s;
            for (const auto& m : kModSources) s.add (m.name);
            return s;
        }();
        return a;
    }

    // ── Global / voice (9) ──────────────────────────────────────────────────
    inline const std::array<Def, kGlobalCount>& globalDefs()
    {
        static const std::array<Def, kGlobalCount> d {{
            floatDef  ("polyphony",        "Polyphony",        1.0f,  16.0f, 8.0f, 1.0f, 0.0f, 1.0f),
            choiceDef ("voice_mode",       "Voice Mode",       &voiceModes(), 0),
            floatDef  ("glide",            "Glide",            0.0f, 2000.0f, 0.0f, 0.3f),
            floatDef  ("pitch_bend_range", "Pitch Bend Range", 0.0f,  48.0f, 2.0f, 1.0f, 0.0f, 1.0f),
            boolDef   ("mpe_enable",       "MPE",              false),
            choiceDef ("grain_limit",      "Grain Limit",      &grainLimits(), 3),
            boolDef   ("freeze",           "Freeze",           false),
            choiceDef ("sync_mode",        "Sync Mode",        &syncModes(), 0),
            floatDef  ("output_gain",      "Output Gain",    -24.0f,  12.0f, 0.0f),
        }};
        return d;
    }

    // ── Amp envelope, per voice (5) ─────────────────────────────────────────
    inline const std::array<Def, kAmpCount>& ampDefs()
    {
        static const std::array<Def, kAmpCount> d {{
            floatDef ("amp_attack",  "Amp Attack",      0.0f, 10000.0f,   20.0f, 1.0f, 300.0f),
            floatDef ("amp_decay",   "Amp Decay",       0.0f, 10000.0f,  300.0f, 1.0f, 300.0f),
            floatDef ("amp_sustain", "Amp Sustain",     0.0f,   100.0f,  100.0f),
            floatDef ("amp_release", "Amp Release",     0.0f, 20000.0f, 1500.0f, 1.0f, 500.0f),
            floatDef ("vel_to_amp",  "Velocity to Amp", 0.0f,   100.0f,   70.0f),
        }};
        return d;
    }

    // ── Mod envelopes: menvN_ + suffix, 4 x 2 ───────────────────────────────
    inline const std::array<Def, kModEnvCount>& modEnvDefs()
    {
        static const std::array<Def, kModEnvCount> d {{
            floatDef ("attack",  "Attack",  0.0f, 10000.0f,  10.0f, 1.0f, 300.0f),
            floatDef ("decay",   "Decay",   0.0f, 10000.0f, 500.0f, 1.0f, 300.0f),
            floatDef ("sustain", "Sustain", 0.0f,   100.0f,   0.0f),
            floatDef ("release", "Release", 0.0f, 20000.0f, 500.0f, 1.0f, 500.0f),
        }};
        return d;
    }

    // ── Per stream: sN_ + suffix, 38 x 4. FROZEN order. ─────────────────────
    // s1_enable defaults On, s2...s4 Off (special-cased in the layout loop).
    inline const std::array<Def, kStreamCount>& streamDefs()
    {
        static const std::array<Def, kStreamCount> d {{
            boolDef   ("enable",        "Enable",        false),
            boolDef   ("mute",          "Mute",          false),
            boolDef   ("solo",          "Solo",          false),
            floatDef  ("level",         "Level",          -60.0f,     6.0f,     0.0f),
            floatDef  ("pan",           "Pan",           -100.0f,   100.0f,     0.0f),
            floatDef  ("position",      "Position",         0.0f,   100.0f,     0.0f),
            floatDef  ("spray",         "Spray",            0.0f,   100.0f,    10.0f),
            floatDef  ("scan_rate",     "Scan Rate",     -400.0f,   400.0f,     0.0f),
            choiceDef ("scan_mode",     "Scan Mode",     &scanModes(), 1),
            floatDef  ("loop_start",    "Loop Start",       0.0f,   100.0f,     0.0f),
            floatDef  ("loop_end",      "Loop End",         0.0f,   100.0f,   100.0f),
            floatDef  ("root_note",     "Root Note",        0.0f,   127.0f,    48.0f, 1.0f, 0.0f, 1.0f),
            floatDef  ("root_fine",     "Root Fine",     -100.0f,   100.0f,     0.0f),
            floatDef  ("key_track",     "Key Track",        0.0f,   100.0f,   100.0f),
            floatDef  ("size",          "Size",             5.0f,  2000.0f,   100.0f, 0.3f),
            choiceDef ("size_sync",     "Size Sync",     &sizeSyncs(), 7),
            floatDef  ("size_rand",     "Size Random",      0.0f,   100.0f,     0.0f),
            floatDef  ("density",       "Density",          0.5f,   200.0f,    10.0f, 0.3f),
            choiceDef ("density_sync",  "Density Sync",  &densitySyncs(), 7),
            floatDef  ("jitter",        "Jitter",           0.0f,   100.0f,    20.0f),
            floatDef  ("pitch",         "Pitch",          -24.0f,    24.0f,     0.0f),
            floatDef  ("pitch_rand",    "Pitch Random",     0.0f,    24.0f,     0.0f),
            choiceDef ("scale",         "Scale",         &scaleNames(), 0),
            floatDef  ("reverse_prob",  "Reverse",          0.0f,   100.0f,     0.0f),
            floatDef  ("window",        "Window",           0.0f,     1.0f,     0.0f),
            floatDef  ("window_skew",   "Window Skew",   -100.0f,   100.0f,     0.0f),
            floatDef  ("stereo_spread", "Stereo Spread",    0.0f,   100.0f,    30.0f),
            floatDef  ("gain_rand",     "Gain Random",      0.0f,   100.0f,     0.0f),
            choiceDef ("filter_type",   "Filter Type",   &filterTypes(), 0),
            floatDef  ("filter_cutoff", "Filter Cutoff",   20.0f, 20000.0f, 20000.0f, 1.0f, 632.4555f),
            floatDef  ("filter_rand",   "Filter Random",    0.0f,   100.0f,     0.0f),
            floatDef  ("filter_res",    "Filter Res",       0.0f,   100.0f,    20.0f),
            floatDef  ("drive",         "Drive",            0.0f,   100.0f,     0.0f),
            floatDef  ("crush",         "Crush",            0.0f,   100.0f,     0.0f),
            floatDef  ("crush_prob",    "Crush Chance",     0.0f,   100.0f,   100.0f),
            floatDef  ("ring_freq",     "Ring Freq",       20.0f,  5000.0f,   200.0f, 1.0f, 316.2278f),
            floatDef  ("ring_rand",     "Ring Random",      0.0f,   100.0f,     0.0f),
            floatDef  ("ring_mix",      "Ring Mix",         0.0f,   100.0f,     0.0f),
        }};
        return d;
    }

    // ── Record-in, sidechain (7) ────────────────────────────────────────────
    inline const std::array<Def, kRecCount>& recDefs()
    {
        static const std::array<Def, kRecCount> d {{
            choiceDef ("rec_target",      "Record Target",      &recTargets(), 0),
            boolDef   ("rec_arm",         "Record Arm",         false),
            choiceDef ("rec_trigger",     "Record Trigger",     &recTriggers(), 0),
            floatDef  ("rec_threshold",   "Record Threshold", -60.0f,  0.0f, -30.0f),
            floatDef  ("rec_length",      "Record Length",      0.5f, 30.0f,   4.0f, 0.5f),
            choiceDef ("rec_length_sync", "Record Length Sync", &recLengthSyncs(), 2),
            floatDef  ("rec_gain",        "Record Gain",      -24.0f, 12.0f,   0.0f),
        }};
        return d;
    }

    // ── Feedback network (5) ────────────────────────────────────────────────
    inline const std::array<Def, kFeedbackCount>& feedbackDefs()
    {
        static const std::array<Def, kFeedbackCount> d {{
            floatDef ("feedback",      "Feedback",         0.0f,   100.0f,     0.0f),
            floatDef ("fb_low_cut",    "FB Low Cut",      20.0f,  2000.0f,    80.0f, 1.0f, 200.0f),
            floatDef ("fb_high_cut",   "FB High Cut",   1000.0f, 20000.0f, 12000.0f, 1.0f, 4472.136f),
            floatDef ("fb_saturation", "FB Saturation",    0.0f,   100.0f,    20.0f),
            floatDef ("fb_diffusion",  "FB Diffusion",     0.0f,   100.0f,     0.0f),
        }};
        return d;
    }

    // ── Output reverb (3) ───────────────────────────────────────────────────
    inline const std::array<Def, kReverbCount>& reverbDefs()
    {
        static const std::array<Def, kReverbCount> d {{
            floatDef ("reverb_mix",  "Reverb Mix",  0.0f, 100.0f,  0.0f),
            floatDef ("reverb_size", "Reverb Size", 0.0f, 100.0f, 50.0f),
            floatDef ("reverb_damp", "Reverb Damp", 0.0f, 100.0f, 50.0f),
        }};
        return d;
    }

    // ── LFOs: lfoN_ + suffix, 5 x 4 ─────────────────────────────────────────
    inline const std::array<Def, kLfoCount>& lfoDefs()
    {
        static const std::array<Def, kLfoCount> d {{
            choiceDef ("shape",     "Shape",     &lfoShapes(), 0),
            floatDef  ("rate",      "Rate",      0.01f,  20.0f, 0.5f, 0.3f),
            choiceDef ("rate_sync", "Rate Sync", &lfoRateSyncs(), 18),
            floatDef  ("phase",     "Phase",     0.0f,  360.0f, 0.0f),
            choiceDef ("mode",      "Mode",      &lfoModes(), 0),
        }};
        return d;
    }

    // ── Macros: macroN ──────────────────────────────────────────────────────
    inline const Def& macroDef() { static const Def d = floatDef ("macro", "Macro", 0.0f, 100.0f, 0.0f); return d; }

    // ── Modulation matrix: modM_ + suffix, 3 x 16 (dest choices: modDestChoices())
    inline const std::array<Def, kModCount>& modDefs()
    {
        static const std::array<Def, kModCount> d {{
            choiceDef ("source", "Source",      &modSources(), 0),
            choiceDef ("dest",   "Destination", nullptr, 0),
            floatDef  ("depth",  "Depth",       -100.0f, 100.0f, 0.0f),
        }};
        return d;
    }

    // ── ID helpers ──────────────────────────────────────────────────────────
    inline juce::String stream (int n, const char* s) { return "s" + juce::String (n) + "_" + s; }
    inline juce::String modEnv (int n, const char* s) { return "menv" + juce::String (n) + "_" + s; }
    inline juce::String lfo (int n, const char* s)    { return "lfo" + juce::String (n) + "_" + s; }
    inline juce::String mod (int m, const char* s)    { return "mod" + juce::String (m) + "_" + s; }
    inline juce::String macro (int n)                 { return "macro" + juce::String (n); }

    /** One registration entry: full ID, host name, and its contract row. */
    struct Entry
    {
        juce::String id, name;
        const Def* def;
        int instance;   // stream / mod env / LFO / macro / slot number, 0 for globals
    };

    /** True for the one Bool that defaults On (s1_enable). */
    inline bool boolDefaultFor (const Entry& e) noexcept
    {
        if (std::strcmp (e.def->id, "enable") == 0 && e.instance > 0)
            return e.instance == 1;
        return e.def->def > 0.5f;
    }

    /** Every parameter in registration order (= parameter-spec.md "Full ID list"). */
    inline const std::vector<Entry>& registrationOrder()
    {
        static const std::vector<Entry> entries = []
        {
            std::vector<Entry> e;
            e.reserve (static_cast<size_t> (kNumParams));
            for (const auto& d : globalDefs()) e.push_back ({ d.id, d.name, &d, 0 });
            for (const auto& d : ampDefs())    e.push_back ({ d.id, d.name, &d, 0 });
            for (int n = 1; n <= kModEnvs; ++n)
                for (const auto& d : modEnvDefs())
                    e.push_back ({ modEnv (n, d.id), "Mod Env " + juce::String (n) + dot() + d.name, &d, n });
            for (int n = 1; n <= kStreams; ++n)
                for (const auto& d : streamDefs())
                    e.push_back ({ stream (n, d.id), "S" + juce::String (n) + dot() + d.name, &d, n });
            for (const auto& d : recDefs())      e.push_back ({ d.id, d.name, &d, 0 });
            for (const auto& d : feedbackDefs()) e.push_back ({ d.id, d.name, &d, 0 });
            for (const auto& d : reverbDefs())   e.push_back ({ d.id, d.name, &d, 0 });
            for (int n = 1; n <= kLfos; ++n)
                for (const auto& d : lfoDefs())
                    e.push_back ({ lfo (n, d.id), "LFO " + juce::String (n) + dot() + d.name, &d, n });
            for (int n = 1; n <= kMacros; ++n)
                e.push_back ({ macro (n), "Macro " + juce::String (n), &macroDef(), n });
            for (int m = 1; m <= kModSlots; ++m)
                for (const auto& d : modDefs())
                    e.push_back ({ mod (m, d.id), "Mod " + juce::String (m) + dot() + d.name, &d, m });
            jassert (e.size() == static_cast<size_t> (kNumParams));
            return e;
        }();
        return entries;
    }

    inline const juce::StringArray& allIdsInOrder()
    {
        static const juce::StringArray ids = []
        {
            juce::StringArray a;
            for (const auto& e : registrationOrder()) a.add (e.id);
            return a;
        }();
        return ids;
    }

    // ── Modulation destination table: APPEND-ONLY. modM_dest stores the index.
    // Never re-order or remove; a new destination gets index 126, 127, ... and
    // kModDestTableVersion is bumped. Every entry other than None is a Float.
    // Per stream base b = 2 + 28 * (N - 1): the parent's 24 suffixes in their
    // original order, then scan_rate, loop_start, loop_end, key_track.
    inline constexpr const char* kModDestIds[kModDests] = {
        "None",
        "output_gain",
        // s1 (2...29)
        "s1_level", "s1_pan", "s1_position", "s1_spray", "s1_size", "s1_size_rand", "s1_density", "s1_jitter",
        "s1_pitch", "s1_pitch_rand", "s1_reverse_prob", "s1_window", "s1_window_skew", "s1_stereo_spread",
        "s1_gain_rand", "s1_filter_cutoff", "s1_filter_rand", "s1_filter_res", "s1_drive", "s1_crush",
        "s1_crush_prob", "s1_ring_freq", "s1_ring_rand", "s1_ring_mix",
        "s1_scan_rate", "s1_loop_start", "s1_loop_end", "s1_key_track",
        // s2 (30...57)
        "s2_level", "s2_pan", "s2_position", "s2_spray", "s2_size", "s2_size_rand", "s2_density", "s2_jitter",
        "s2_pitch", "s2_pitch_rand", "s2_reverse_prob", "s2_window", "s2_window_skew", "s2_stereo_spread",
        "s2_gain_rand", "s2_filter_cutoff", "s2_filter_rand", "s2_filter_res", "s2_drive", "s2_crush",
        "s2_crush_prob", "s2_ring_freq", "s2_ring_rand", "s2_ring_mix",
        "s2_scan_rate", "s2_loop_start", "s2_loop_end", "s2_key_track",
        // s3 (58...85)
        "s3_level", "s3_pan", "s3_position", "s3_spray", "s3_size", "s3_size_rand", "s3_density", "s3_jitter",
        "s3_pitch", "s3_pitch_rand", "s3_reverse_prob", "s3_window", "s3_window_skew", "s3_stereo_spread",
        "s3_gain_rand", "s3_filter_cutoff", "s3_filter_rand", "s3_filter_res", "s3_drive", "s3_crush",
        "s3_crush_prob", "s3_ring_freq", "s3_ring_rand", "s3_ring_mix",
        "s3_scan_rate", "s3_loop_start", "s3_loop_end", "s3_key_track",
        // s4 (86...113)
        "s4_level", "s4_pan", "s4_position", "s4_spray", "s4_size", "s4_size_rand", "s4_density", "s4_jitter",
        "s4_pitch", "s4_pitch_rand", "s4_reverse_prob", "s4_window", "s4_window_skew", "s4_stereo_spread",
        "s4_gain_rand", "s4_filter_cutoff", "s4_filter_rand", "s4_filter_res", "s4_drive", "s4_crush",
        "s4_crush_prob", "s4_ring_freq", "s4_ring_rand", "s4_ring_mix",
        "s4_scan_rate", "s4_loop_start", "s4_loop_end", "s4_key_track",
        // feedback + reverb (114...121)
        "feedback", "fb_low_cut", "fb_high_cut", "fb_saturation", "fb_diffusion",
        "reverb_mix", "reverb_size", "reverb_damp",
        // LFO rates (122...125)
        "lfo1_rate", "lfo2_rate", "lfo3_rate", "lfo4_rate",
    };
    static_assert (std::size (kModDestIds) == 126, "parameter-spec.md: 126 modulation destinations");

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
                for (const auto& e : order)
                    if (e.id == id) { name = e.name; break; }
                jassert (name.isNotEmpty());
                c.add (name);
            }
            return c;
        }();
        return a;
    }
}
