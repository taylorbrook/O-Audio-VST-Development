/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
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
    FactoryPresets.h — the factory bank (Stage 4, PLAN-4.1 P1).

    WHY THE DEFS ARE IN ENGINEERING UNITS. bNN_freq and bNN_q are pow-skewed,
    and a choice normalises through its own range. A def written as a
    normalised number by hand ignores both (pattern_factory_preset_normalized_
    ignores_skew). So each def holds Hz, dB, Q, a choice INDEX or a bool, and
    makeFactoryPresets() converts every value ONCE through the live
    parameter's convertTo0to1 (the O-GranulatorInstrument shape).

    WHAT A DEF NAMES. Only the bands it uses, from b01 in ascending frequency
    so the chips read left to right. Everything else comes from the preset
    manager's reset-to-defaults pass. A def never names output_gain or bypass
    (feedback_presets_never_set_output_gain; the manager also excludes both),
    and no def carries a customState: presets hold no UI state.

    NAMES are file stems and the identity key of load / isFactoryPreset, so
    they stay English and never contain a path separator (the manager turns
    "/" into "_", and the bar would show it).

    bankHash() is O-GranulatorInstrument's, as-is: XXH64 over the canonical
    serialisation; PluginProcessor::initFactoryBank() compares it with
    Factory/.bank-hash so a re-voice or a rename at an unchanged version still
    reaches ~/Library.
*/

#pragma once

#include <JuceHeader.h>
#include "OuariconPresetManager.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>

namespace opeq::factory
{
    // bNN_type choice indices (PluginProcessor.cpp kTypeChoices order).
    enum Type { bell = 0, lowShelf, highShelf, lowCut, highCut, notch, bandPass, tiltShelf };

    // bNN_slope choice indices (kSlopeChoices order).
    enum Slope { s6 = 0, s12, s18, s24, s36, s48, s72, s96 };

    // bNN_placement choice indices (kPlacementChoices order).
    enum Placement { stereo = 0, left, right, mid, side };

    struct Value
    {
        juce::String id;
        float value;   // engineering units: Hz, dB, Q, a choice index, 0 / 1
    };

    struct PresetSpec
    {
        const char* name;
        std::vector<Value> values;
    };

    /** The parameters one band of a def names. Gain only on gain-bearing
        types, slope only on types that read it, placement only off Stereo. */
    inline void addBand (std::vector<Value>& out, int n, Type type, float freq,
                         float gain, float q, Slope slope, Placement placement = stereo)
    {
        const auto p = "b" + juce::String (n).paddedLeft ('0', 2) + "_";
        const bool hasGain  = type == bell || type == lowShelf || type == highShelf || type == tiltShelf;
        const bool hasSlope = type == lowShelf || type == highShelf || type == lowCut
                           || type == highCut  || type == tiltShelf;

        out.push_back ({ p + "enabled", 1.0f });
        out.push_back ({ p + "type", (float) type });
        out.push_back ({ p + "freq", freq });
        if (hasGain)            out.push_back ({ p + "gain", gain });
        out.push_back ({ p + "q", q });
        if (hasSlope)           out.push_back ({ p + "slope", (float) slope });
        if (placement != stereo) out.push_back ({ p + "placement", (float) placement });
    }

   #if OPEQ_TEST_HOOKS
    /** Render harness only (FP2 (c) / (d)): edits the spec table after it is
        built, to stand for a re-voiced or renamed bank at the same version.
        The define is set on the harness target alone; no plugin target sees it. */
    inline std::function<void (std::vector<PresetSpec>&)>& testSpecEdit()
    {
        static std::function<void (std::vector<PresetSpec>&)> edit;
        return edit;
    }
   #endif

    /** RESEARCH-4 Q1's table. |gain| <= 5 dB everywhere, so every node is on
        screen at every display range. Q 1.0 on cuts and shelves (Butterworth)
        unless stated. The values are paper starting points; the 4.2
        listening pass re-voices them, and the bank hash carries a re-voice. */
    inline std::vector<PresetSpec> factoryPresetSpecs()
    {
        std::vector<PresetSpec> specs;

        auto preset = [&specs] (const char* name) -> std::vector<Value>&
        {
            specs.push_back ({ name, {} });
            return specs.back().values;
        };

        // 1. The flat state: no values at all, the reset pass alone.
        preset ("Default");

        { auto& v = preset ("Vocal Presence");
          addBand (v, 1, lowCut,    90.0f,    0.0f, 1.0f, s18);
          addBand (v, 2, bell,      280.0f,  -2.5f, 1.2f, s12);
          addBand (v, 3, bell,      3500.0f,  3.0f, 0.9f, s12);
          addBand (v, 4, highShelf, 11000.0f, 2.0f, 1.0f, s12); }

        { auto& v = preset ("Kick Tighten");
          addBand (v, 1, lowCut, 30.0f,    0.0f, 1.0f, s24);
          addBand (v, 2, bell,   62.0f,    3.0f, 1.4f, s12);
          addBand (v, 3, bell,   380.0f,  -5.0f, 1.8f, s12);
          addBand (v, 4, bell,   3800.0f,  4.0f, 1.6f, s12); }

        { auto& v = preset ("De-Mud");
          addBand (v, 1, bell, 220.0f, -3.0f, 1.0f, s12);
          addBand (v, 2, bell, 450.0f, -2.0f, 1.6f, s12); }

        { auto& v = preset ("Air Shelf");
          addBand (v, 1, highShelf, 12000.0f, 3.0f, 1.0f, s12); }

        { auto& v = preset ("Rumble Cut");
          addBand (v, 1, lowCut, 28.0f, 0.0f, 1.0f, s96); }

        { auto& v = preset ("Telephone");
          addBand (v, 1, lowCut,   300.0f,  0.0f, 1.0f, s48);
          addBand (v, 2, bandPass, 1200.0f, 0.0f, 0.8f, s12);
          addBand (v, 3, highCut,  3400.0f, 0.0f, 1.0f, s48);
          v.push_back ({ "auto_gain", 1.0f }); }

        { auto& v = preset ("Hum Notch 50 Hz");
          addBand (v, 1, notch, 50.0f,  0.0f, 30.0f, s12);
          addBand (v, 2, notch, 100.0f, 0.0f, 30.0f, s12);
          addBand (v, 3, notch, 150.0f, 0.0f, 30.0f, s12);
          addBand (v, 4, notch, 200.0f, 0.0f, 25.0f, s12); }

        { auto& v = preset ("Hum Notch 60 Hz");
          addBand (v, 1, notch, 60.0f,  0.0f, 30.0f, s12);
          addBand (v, 2, notch, 120.0f, 0.0f, 30.0f, s12);
          addBand (v, 3, notch, 180.0f, 0.0f, 30.0f, s12);
          addBand (v, 4, notch, 240.0f, 0.0f, 25.0f, s12); }

        // Tilt gain is the full span (ARCHITECTURE): lows -1 dB, highs +1 dB.
        { auto& v = preset ("Gentle Master Tilt");
          addBand (v, 1, lowCut,    20.0f,   0.0f, 1.0f, s12);
          addBand (v, 2, tiltShelf, 1000.0f, 2.0f, 1.0f, s6); }

        // C2: on a mono instance every placement processes the channel, so
        // this is a whole-signal 120 Hz cut there (FP5i prints it).
        { auto& v = preset ("Mono Low End");
          addBand (v, 1, lowCut, 120.0f, 0.0f, 1.0f, s24, side); }

        { auto& v = preset ("Side Air");
          addBand (v, 1, bell,      250.0f,  -1.5f, 1.0f, s12, mid);
          addBand (v, 2, highShelf, 9000.0f,  2.5f, 1.0f, s12, side); }

        // RESEARCH named it "L/R Pair Match"; a "/" is a path separator the
        // manager rewrites to "_" in the file stem, which is the name the bar
        // shows. Renamed in execute 4.1.
        { auto& v = preset ("L-R Pair Match");
          addBand (v, 1, bell,      180.0f,  -1.5f, 1.2f, s12, left);
          addBand (v, 2, highShelf, 6000.0f,  1.5f, 1.0f, s12, right); }

       #if OPEQ_TEST_HOOKS
        if (auto& edit = testSpecEdit())
            edit (specs);
       #endif

        return specs;
    }

    /** True when no def names a parameter a preset must never carry. */
    inline bool specsNameNoForbiddenKey (const std::vector<PresetSpec>& specs)
    {
        for (const auto& s : specs)
            for (const auto& v : s.values)
                if (v.id == "output_gain" || v.id == "bypass" || v.id == "customState")
                    return false;
        return true;
    }

    /** Engineering units -> the manager's normalised defs, one convertTo0to1
        per value through the live parameter (a choice through its own range). */
    inline std::vector<OuariconPresetManager::FactoryPresetDef> makeFactoryPresets (juce::AudioProcessorValueTreeState& apvts)
    {
        const auto specs = factoryPresetSpecs();
        jassert (specsNameNoForbiddenKey (specs));

        std::vector<OuariconPresetManager::FactoryPresetDef> defs;

        for (const auto& s : specs)
        {
            OuariconPresetManager::FactoryPresetDef d;
            d.name = s.name;

            for (const auto& v : s.values)
            {
                auto* rp = apvts.getParameter (v.id);
                jassert (rp != nullptr);   // every id a def names exists

                if (rp != nullptr)
                    d.parameters[v.id] = rp->convertTo0to1 (v.value);
            }

            defs.push_back (std::move (d));
        }

        return defs;
    }

    //==========================================================================
    // XXH64, written from the xxHash specification (copied from
    // O-GranulatorInstrument Source/dsp/GiHash.h, the one-shot function only).
    //==========================================================================
    namespace hash
    {
        inline constexpr std::uint64_t kP1 = 0x9E3779B185EBCA87ull;
        inline constexpr std::uint64_t kP2 = 0xC2B2AE3D27D4EB4Full;
        inline constexpr std::uint64_t kP3 = 0x165667B19E3779F9ull;
        inline constexpr std::uint64_t kP4 = 0x85EBCA77C2B2AE63ull;
        inline constexpr std::uint64_t kP5 = 0x27D4EB2F165667C5ull;

        inline std::uint64_t rotl (std::uint64_t x, int r) noexcept { return (x << r) | (x >> (64 - r)); }

        inline std::uint64_t read64 (const std::uint8_t* p) noexcept
        {
            std::uint64_t v = 0;
            for (int i = 7; i >= 0; --i)
                v = (v << 8) | p[i];
            return v;
        }

        inline std::uint32_t read32 (const std::uint8_t* p) noexcept
        {
            return static_cast<std::uint32_t> (p[0]) | (static_cast<std::uint32_t> (p[1]) << 8)
                 | (static_cast<std::uint32_t> (p[2]) << 16) | (static_cast<std::uint32_t> (p[3]) << 24);
        }

        inline std::uint64_t round (std::uint64_t acc, std::uint64_t lane) noexcept
        {
            acc += lane * kP2;
            acc = rotl (acc, 31);
            return acc * kP1;
        }

        inline std::uint64_t merge (std::uint64_t acc, std::uint64_t v) noexcept
        {
            acc ^= round (0, v);
            return acc * kP1 + kP4;
        }

        inline std::uint64_t xxh64 (const void* data, std::size_t len, std::uint64_t seed = 0) noexcept
        {
            const auto* p = static_cast<const std::uint8_t*> (data);
            const std::uint8_t* const end = p + len;
            std::uint64_t h;

            if (len >= 32)
            {
                std::uint64_t v1 = seed + kP1 + kP2, v2 = seed + kP2, v3 = seed, v4 = seed - kP1;
                const std::uint8_t* const limit = end - 32;
                do
                {
                    v1 = round (v1, read64 (p));      p += 8;
                    v2 = round (v2, read64 (p));      p += 8;
                    v3 = round (v3, read64 (p));      p += 8;
                    v4 = round (v4, read64 (p));      p += 8;
                }
                while (p <= limit);
                h = rotl (v1, 1) + rotl (v2, 7) + rotl (v3, 12) + rotl (v4, 18);
                h = merge (h, v1);
                h = merge (h, v2);
                h = merge (h, v3);
                h = merge (h, v4);
            }
            else
            {
                h = seed + kP5;
            }

            h += static_cast<std::uint64_t> (len);

            while (p + 8 <= end)
            {
                h ^= round (0, read64 (p));
                h = rotl (h, 27) * kP1 + kP4;
                p += 8;
            }
            if (p + 4 <= end)
            {
                h ^= static_cast<std::uint64_t> (read32 (p)) * kP1;
                h = rotl (h, 23) * kP2 + kP3;
                p += 4;
            }
            while (p < end)
            {
                h ^= static_cast<std::uint64_t> (*p) * kP5;
                h = rotl (h, 11) * kP1;
                ++p;
            }

            h ^= h >> 33;
            h *= kP2;
            h ^= h >> 29;
            h *= kP3;
            h ^= h >> 32;
            return h;
        }
    } // namespace hash

    /** The bank stamp: XXH64 over the canonical serialisation of the defs
        (name, parameters in id order with the float's bits, customState JSON
        text), 16 lowercase hex digits. Any edit to the bank moves it. */
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

        return juce::String::toHexString (static_cast<juce::int64> (hash::xxh64 (mo.getData(), mo.getDataSize(), 0)))
                   .paddedLeft ('0', 16);
    }
} // namespace opeq::factory
