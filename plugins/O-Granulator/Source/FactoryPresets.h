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
#include <cstdint>
#include <utility>
#include <vector>
#include "GranulatorParamIds.h"
#include "OuariconPresetManager.h"

//==============================================================================
// Factory presets (Stage 4, PLAN-4 P153 / P154, CONTEXT-4 D42).
//
// 16 presets, 4 per category, named "<Category> · <Name>" (≤ 24 characters;
// the preset manager has no categories, so the case-insensitive name sort
// groups them). Values are ENGINEERING units — Hz, ms, %, dB, choice INDEX,
// bool 0/1 — converted once through each parameter's own range
// (convertTo0to1), so skewed ranges and choices land exactly
// (pattern_factory_preset_normalized_ignores_skew). Absent parameters read as
// default (the manager resets before it applies). output_gain is never part of
// a preset (feedback_presets_never_set_output_gain). Every preset carries a
// fixed seed: an absent seed would keep the live instance seed (R142).
// Descriptions are documentation only (CHANGELOG / README); the UI does not
// render them (sign-off 2), and they are not written to the preset JSON.
//==============================================================================
namespace ogran::factory
{
    struct FactoryPresetSpec
    {
        const char* name;
        const char* description;
        std::int64_t seed;
        std::vector<std::pair<const char*, float>> values;
        juce::String customJson;   // optional {"custom":{…},"masks":[…]} merged into customState
    };

    /** modM_dest choice index of a destination parameter id (0 = None). */
    inline float modDest (const char* id)
    {
        for (int k = 1; k < GranulatorParamIds::kModDests; ++k)
            if (juce::String (GranulatorParamIds::kModDestIds[k]) == id)
                return static_cast<float> (k);
        jassertfalse;
        return 0.0f;
    }

    // Mod sources (choice index): 0 None, 1–4 LFO 1–4, 5 Env, 6 Pitch, 7 Onset, 8–11 Macro 1–4.
    // LFO shapes: 0 Sine, 1 Triangle, 2 Saw, 3 Square, 4 S&H, 5 Smooth Random.
    // Filter types: 0 Off, 1 LP, 2 BP, 3 HP.
    // Tempo divisions (index): 4 1/32, 6 1/16 T, 7 1/16, 8 1/16 D, 9 1/8 T, 10 1/8, 13 1/4, 16 1/2, 18 1 bar, 19 2 bars, 20 4 bars.
    // Scales (index): 2 Major, 3 Minor, 4 Pentatonic, 6 Dorian, 17 Fifths + Octaves, 24 5-Limit Just, 31 Custom.

    inline std::vector<FactoryPresetSpec> factoryPresetSpecs()
    {
        return {
            //==================================================================
            // Ambient
            //==================================================================
            { "Ambient \xc2\xb7 Cloud Drift",
              "Long, sprayed Hann grains with an octave-up second stream; a slow LFO walks the read position through the buffer.",
              0x0A3B1C2D4E5F6071ll,
              { { "mix", 60.0f }, { "buffer_length", 12.0f },
                { "s1_density", 25.0f }, { "s1_size", 600.0f }, { "s1_spray", 60.0f }, { "s1_jitter", 40.0f }, { "s1_stereo_spread", 80.0f },
                { "s2_enable", 1.0f }, { "s2_level", -9.0f }, { "s2_pitch", 12.0f }, { "s2_density", 12.0f }, { "s2_size", 900.0f },
                { "s2_spray", 80.0f }, { "s2_reverse_prob", 30.0f }, { "s2_stereo_spread", 90.0f },
                { "feedback", 35.0f }, { "fb_diffusion", 60.0f }, { "fb_high_cut", 6000.0f },
                { "reverb_mix", 40.0f }, { "reverb_size", 80.0f },
                { "lfo1_rate", 0.07f },
                { "mod1_source", 1.0f }, { "mod1_dest", modDest ("s1_position") }, { "mod1_depth", 20.0f } },
              {} },

            { "Ambient \xc2\xb7 Frozen Halo",
              "A three-voice root / fifth / octave halo of very long grains into a large diffused room.",
              0x1B2C3D4E5F607182ll,
              { { "mix", 70.0f }, { "buffer_length", 16.0f },
                { "s1_level", -3.0f }, { "s1_density", 20.0f }, { "s1_size", 1200.0f }, { "s1_spray", 100.0f }, { "s1_stereo_spread", 70.0f },
                { "s2_enable", 1.0f }, { "s2_level", -11.0f }, { "s2_pitch", 7.0f }, { "s2_density", 14.0f }, { "s2_size", 1400.0f },
                { "s2_spray", 100.0f }, { "s2_stereo_spread", 90.0f },
                { "s3_enable", 1.0f }, { "s3_level", -15.0f }, { "s3_pitch", 12.0f }, { "s3_density", 10.0f }, { "s3_size", 1600.0f },
                { "s3_spray", 100.0f }, { "s3_stereo_spread", 100.0f },
                { "feedback", 40.0f }, { "fb_diffusion", 80.0f }, { "fb_high_cut", 5000.0f },
                { "reverb_mix", 60.0f }, { "reverb_size", 90.0f }, { "reverb_damp", 60.0f } },
              {} },

            { "Ambient \xc2\xb7 Slow Shimmer",
              "Octave and two-octave grains recirculate through a bright, diffused loop; smooth random panning keeps it moving.",
              0x2C3D4E5F60718293ll,
              { { "mix", 55.0f },
                { "s1_pitch", 12.0f }, { "s1_density", 15.0f }, { "s1_size", 800.0f }, { "s1_spray", 50.0f }, { "s1_reverse_prob", 20.0f },
                { "s1_stereo_spread", 60.0f },
                { "s2_enable", 1.0f }, { "s2_level", -15.0f }, { "s2_pitch", 24.0f }, { "s2_density", 8.0f }, { "s2_size", 600.0f },
                { "s2_spray", 70.0f }, { "s2_stereo_spread", 80.0f },
                { "feedback", 55.0f }, { "fb_low_cut", 300.0f }, { "fb_diffusion", 70.0f }, { "fb_high_cut", 9000.0f },
                { "reverb_mix", 50.0f }, { "reverb_size", 75.0f },
                { "lfo2_shape", 5.0f }, { "lfo2_rate", 0.2f },
                { "mod1_source", 2.0f }, { "mod1_dest", modDest ("s1_pan") }, { "mod1_depth", 40.0f } },
              {} },

            { "Ambient \xc2\xb7 Dust Bloom",
              "Dense, jittered grains through a randomised low-pass with a reversed octave-down bed.",
              0x3D4E5F6071829304ll,
              { { "mix", 55.0f },
                { "s1_density", 40.0f }, { "s1_size", 250.0f }, { "s1_spray", 90.0f }, { "s1_jitter", 80.0f }, { "s1_gain_rand", 40.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 3000.0f }, { "s1_filter_rand", 40.0f }, { "s1_stereo_spread", 70.0f },
                { "s2_enable", 1.0f }, { "s2_level", -10.0f }, { "s2_pitch", -12.0f }, { "s2_reverse_prob", 100.0f }, { "s2_density", 10.0f },
                { "s2_size", 500.0f }, { "s2_spray", 60.0f },
                { "feedback", 25.0f },
                { "reverb_mix", 45.0f }, { "reverb_size", 70.0f } },
              {} },

            //==================================================================
            // Harmoniser (Pitch Follow on, scale from Scale Root)
            //==================================================================
            { "Harmoniser \xc2\xb7 Major Choir",
              "Three followed voices a third, a fifth and an octave above the tracked note, snapped to C major.",
              0x4E5F607182930415ll,
              { { "mix", 50.0f }, { "scale_root", 0.0f },
                { "s1_pitch", 4.0f }, { "s1_scale", 2.0f }, { "s1_density", 30.0f }, { "s1_size", 150.0f }, { "s1_spray", 5.0f }, { "s1_pan", -40.0f },
                { "s2_enable", 1.0f }, { "s2_pitch", 7.0f }, { "s2_scale", 2.0f }, { "s2_density", 30.0f }, { "s2_size", 150.0f }, { "s2_spray", 5.0f },
                { "s2_pan", 40.0f }, { "s2_level", -2.0f },
                { "s3_enable", 1.0f }, { "s3_pitch", 12.0f }, { "s3_scale", 2.0f }, { "s3_density", 20.0f }, { "s3_size", 200.0f }, { "s3_spray", 5.0f },
                { "s3_level", -8.0f },
                { "pitch_follow_s1", 1.0f }, { "pitch_follow_s2", 1.0f }, { "pitch_follow_s3", 1.0f },
                { "reverb_mix", 20.0f } },
              {} },

            { "Harmoniser \xc2\xb7 Fifths",
              "Power-chord harmoniser: fifth and octave voices snapped to Fifths + Octaves in D.",
              0x5F60718293041526ll,
              { { "mix", 50.0f }, { "scale_root", 2.0f },
                { "s1_pitch", 7.0f }, { "s1_scale", 17.0f }, { "s1_density", 30.0f }, { "s1_size", 120.0f }, { "s1_spray", 5.0f },
                { "s2_enable", 1.0f }, { "s2_pitch", -12.0f }, { "s2_scale", 17.0f }, { "s2_density", 25.0f }, { "s2_size", 180.0f }, { "s2_spray", 5.0f },
                { "s2_level", -4.0f },
                { "pitch_follow_s1", 1.0f }, { "pitch_follow_s2", 1.0f },
                { "s1_drive", 20.0f }, { "s2_drive", 20.0f },
                { "reverb_mix", 15.0f } },
              {} },

            { "Harmoniser \xc2\xb7 Just Halo",
              "Pure-interval harmony: thirds and fifths snapped to 5-limit just intonation in A, with a soft halo.",
              0x6071829304152637ll,
              { { "mix", 55.0f }, { "scale_root", 9.0f },
                { "s1_pitch", 4.0f }, { "s1_scale", 24.0f }, { "s1_density", 25.0f }, { "s1_size", 250.0f }, { "s1_spray", 10.0f }, { "s1_pan", -50.0f },
                { "s2_enable", 1.0f }, { "s2_pitch", 7.0f }, { "s2_scale", 24.0f }, { "s2_density", 25.0f }, { "s2_size", 250.0f }, { "s2_spray", 10.0f },
                { "s2_pan", 50.0f }, { "s2_level", -2.0f },
                { "pitch_follow_s1", 1.0f }, { "pitch_follow_s2", 1.0f },
                { "feedback", 20.0f }, { "fb_diffusion", 50.0f },
                { "reverb_mix", 35.0f }, { "reverb_size", 65.0f } },
              {} },

            { "Harmoniser \xc2\xb7 19-EDO Haze",
              "Followed voices snapped to a masked 19-EDO custom scale in E, with random pitch scatter on the same degrees.",
              0x7182930415263748ll,
              { { "mix", 55.0f }, { "scale_root", 4.0f },
                { "s1_pitch", 0.0f }, { "s1_scale", 31.0f }, { "s1_density", 30.0f }, { "s1_size", 200.0f }, { "s1_spray", 20.0f },
                { "s1_pitch_rand", 7.0f },
                { "s2_enable", 1.0f }, { "s2_pitch", 12.0f }, { "s2_scale", 31.0f }, { "s2_density", 15.0f }, { "s2_size", 300.0f },
                { "s2_spray", 30.0f }, { "s2_level", -6.0f }, { "s2_stereo_spread", 80.0f },
                { "pitch_follow_s1", 1.0f }, { "pitch_follow_s2", 1.0f },
                { "feedback", 25.0f }, { "fb_diffusion", 40.0f },
                { "reverb_mix", 30.0f } },
              // 19-EDO, degrees 0 3 6 8 11 14 17 kept on stream 1 (a 7-note subset); stream 2 all on.
              R"({"custom":{"name":"19-EDO","text":"edo 19"},"masks":[{"stream":1,"collection":31,"bits":"0000000000024949"}]})" },

            //==================================================================
            // Rhythmic (Tempo sync)
            //==================================================================
            { "Rhythmic \xc2\xb7 Stutter 16",
              "Sixteenth-note grains locked to the host tempo; a square LFO at 1/8 gates the level.",
              0x1293041526374859ll,
              { { "mix", 60.0f }, { "sync_mode", 1.0f },
                { "s1_size_sync", 7.0f }, { "s1_density_sync", 7.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 0.0f }, { "s1_window", 0.67f },
                { "s1_stereo_spread", 20.0f },
                { "lfo1_shape", 3.0f }, { "lfo1_rate_sync", 10.0f },
                { "mod1_source", 1.0f }, { "mod1_dest", modDest ("s1_level") }, { "mod1_depth", -40.0f } },
              {} },

            { "Rhythmic \xc2\xb7 Triplet Dust",
              "Eighth-note triplet bursts with random reverse and crushed accents, panned by a stepped LFO.",
              0x130415263748596All,
              { { "mix", 55.0f }, { "sync_mode", 1.0f },
                { "s1_size_sync", 6.0f }, { "s1_density_sync", 9.0f }, { "s1_spray", 15.0f }, { "s1_jitter", 0.0f }, { "s1_reverse_prob", 40.0f },
                { "s1_crush", 40.0f }, { "s1_crush_prob", 30.0f },
                { "lfo1_shape", 4.0f }, { "lfo1_rate_sync", 9.0f },
                { "mod1_source", 1.0f }, { "mod1_dest", modDest ("s1_pan") }, { "mod1_depth", 70.0f },
                { "reverb_mix", 15.0f } },
              {} },

            { "Rhythmic \xc2\xb7 Gate Pulse",
              "Quarter-note pulses of two streams an octave apart; a saw LFO at one bar sweeps the band-pass.",
              0x04152637485960A1ll,
              { { "mix", 60.0f }, { "sync_mode", 1.0f },
                { "s1_level", 4.0f }, { "s1_size_sync", 10.0f }, { "s1_density_sync", 13.0f }, { "s1_spray", 5.0f }, { "s1_jitter", 0.0f }, { "s1_window", 0.33f },
                { "s1_filter_type", 2.0f }, { "s1_filter_cutoff", 1200.0f }, { "s1_filter_res", 50.0f },
                { "s2_enable", 1.0f }, { "s2_pitch", -12.0f }, { "s2_size_sync", 13.0f }, { "s2_density_sync", 16.0f }, { "s2_level", -2.0f },
                { "s2_jitter", 0.0f },
                { "lfo1_shape", 2.0f }, { "lfo1_rate_sync", 18.0f },
                { "mod1_source", 1.0f }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 50.0f },
                { "feedback", 20.0f } },
              {} },

            { "Rhythmic \xc2\xb7 Onset Spray",
              "Grain bursts fired by the input's transients, with tempo-synced sixteenth-note tails.",
              0x152637485960A1B2ll,
              { { "mix", 55.0f }, { "sync_mode", 1.0f }, { "onset_threshold", -24.0f },
                { "s1_onset_trigger", 1.0f }, { "s1_size_sync", 7.0f }, { "s1_density_sync", 4.0f }, { "s1_spray", 30.0f },
                { "s1_pitch_rand", 5.0f }, { "s1_scale", 4.0f }, { "s1_stereo_spread", 70.0f },
                { "lfo1_rate_sync", 13.0f },
                { "mod1_source", 7.0f }, { "mod1_dest", modDest ("s1_filter_cutoff") }, { "mod1_depth", 30.0f },
                { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 4000.0f },
                { "feedback", 30.0f }, { "reverb_mix", 20.0f } },
              {} },

            //==================================================================
            // Cascade (feedback ≥ 70 %)
            //==================================================================
            { "Cascade \xc2\xb7 Infinite",
              "Near-unity feedback through a gentle diffuser: an endless, slowly evolving tail held by the loop AGC.",
              0x2637485960A1B2C3ll,
              { { "mix", 60.0f },
                { "s1_density", 15.0f }, { "s1_size", 400.0f }, { "s1_spray", 40.0f }, { "s1_stereo_spread", 60.0f },
                { "feedback", 95.0f }, { "fb_saturation", 0.0f }, { "fb_diffusion", 60.0f }, { "fb_low_cut", 120.0f }, { "fb_high_cut", 8000.0f },
                { "reverb_mix", 30.0f }, { "reverb_size", 70.0f } },
              {} },

            { "Cascade \xc2\xb7 Rust Spiral",
              "Hot, saturated feedback with driven grains; the AGC holds the spiral at the saturation ceiling.",
              0x37485960A1B2C3D4ll,
              { { "mix", 50.0f },
                { "s1_density", 20.0f }, { "s1_size", 150.0f }, { "s1_spray", 20.0f }, { "s1_drive", 40.0f },
                { "feedback", 90.0f }, { "fb_saturation", 60.0f }, { "fb_diffusion", 30.0f }, { "fb_low_cut", 150.0f }, { "fb_high_cut", 6000.0f },
                { "reverb_mix", 15.0f } },
              {} },

            { "Cascade \xc2\xb7 Pitch Ladder",
              "Every generation climbs a fifth: pitched grains in a long feedback loop build a rising ladder.",
              0x485960A1B2C3D4E5ll,
              { { "mix", 55.0f },
                { "s1_pitch", 7.0f }, { "s1_density", 12.0f }, { "s1_size", 300.0f }, { "s1_spray", 30.0f }, { "s1_position", 5.0f },
                { "s1_stereo_spread", 50.0f },
                { "feedback", 80.0f }, { "fb_saturation", 30.0f }, { "fb_diffusion", 40.0f }, { "fb_low_cut", 200.0f }, { "fb_high_cut", 7000.0f },
                { "reverb_mix", 25.0f } },
              {} },

            { "Cascade \xc2\xb7 Dark Bloom",
              "Octave-down grains in a dark, heavily diffused loop; a slow LFO breathes the feedback amount.",
              0x5960A1B2C3D4E5F6ll,
              { { "mix", 60.0f },
                { "s1_pitch", -12.0f }, { "s1_density", 18.0f }, { "s1_size", 500.0f }, { "s1_spray", 60.0f }, { "s1_reverse_prob", 30.0f },
                { "feedback", 85.0f }, { "fb_saturation", 20.0f }, { "fb_diffusion", 90.0f }, { "fb_high_cut", 2500.0f },
                { "reverb_mix", 35.0f }, { "reverb_size", 85.0f }, { "reverb_damp", 70.0f },
                { "lfo1_rate", 0.1f },
                { "mod1_source", 1.0f }, { "mod1_dest", modDest ("feedback") }, { "mod1_depth", -10.0f } },
              {} },
        };
    }

    /** Validation (P153): every key a real parameter, no output_gain, every value
        inside its range. Returns the first problem, or an empty string. */
    inline juce::String validateSpec (const FactoryPresetSpec& s, juce::AudioProcessorValueTreeState& apvts)
    {
        for (const auto& [id, v] : s.values)
        {
            if (juce::String (id) == "output_gain")
                return juce::String::fromUTF8 (s.name) + ": output_gain";
            auto* rp = apvts.getParameter (id);
            if (rp == nullptr)
                return juce::String::fromUTF8 (s.name) + ": unknown " + id;
            const auto& r = rp->getNormalisableRange();
            if (! (v >= r.start && v <= r.end))
                return juce::String::fromUTF8 (s.name) + ": " + id + " out of range";
        }
        return {};
    }

    /** The customState for a spec: {"seed": "<int64>"} merged with customJson. */
    inline juce::var customStateOf (const FactoryPresetSpec& s)
    {
        auto* o = new juce::DynamicObject();
        if (s.customJson.isNotEmpty())
            if (auto* extra = juce::JSON::parse (s.customJson).getDynamicObject())
                for (const auto& p : extra->getProperties())
                    o->setProperty (p.name, p.value);
        o->setProperty ("seed", juce::String (s.seed));
        return juce::var (o);
    }

    /** Engineering units → the manager's normalised defs (one convertTo0to1 per value). */
    inline std::vector<OuariconPresetManager::FactoryPresetDef> makeFactoryPresets (juce::AudioProcessorValueTreeState& apvts)
    {
        std::vector<OuariconPresetManager::FactoryPresetDef> defs;
        for (const auto& s : factoryPresetSpecs())
        {
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

    /** The preset JSON exactly as initializeFactoryPresets writes it (minus the
        version / plugin / factory metadata, which apply ignores). Harness use. */
    inline juce::var presetJsonOf (const OuariconPresetManager::FactoryPresetDef& d)
    {
        auto* root = new juce::DynamicObject();
        auto* params = new juce::DynamicObject();
        for (const auto& [id, v] : d.parameters)
            params->setProperty (id, v);
        root->setProperty ("parameters", juce::var (params));
        if (! d.customState.isVoid())
            root->setProperty ("customState", d.customState);
        return juce::var (root);
    }
}
