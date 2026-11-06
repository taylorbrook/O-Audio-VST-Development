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
#include <cmath>
#include "PluginProcessor.h"

// ============================================================================
// The page's two JSON payloads, built in ONE place (PLAN-3.1 P107):
//
//   buildScaleState (processor)       → the getScaleState native's answer
//   telemetryToVar  (telemetry, rev)  → the getTelemetry native's answer
//
// The editor (PluginEditor.cpp) and the render harness (--dump-ui-overrides,
// which writes tests/ui-stub/generic-overrides.json) both include this header,
// so the stub fixture the UI gates measure is produced by the same code the
// page talks to in the plugin (pattern_ui_fixture_state_must_be_producible_by_cpp).
// juce_core types only — no WebBrowserComponent — so the headless consoles
// (JUCE_WEB_BROWSER=0) compile it.
// ============================================================================
namespace ogran::ui
{
    inline constexpr int kNumStreams       = GranulatorParamIds::kStreams;          // 4
    inline constexpr int kNumCollections   = GranulatorParamIds::kNumCollections;   // 32: 0 Off … 31 Custom
    inline constexpr int kCustomCollection = kNumCollections - 1;                   // 31

    // uint64 masks cross the bridge as 16-hex-digit strings: a JS number cannot
    // hold 64 bits, and getLargeIntValue clamps uint64 > INT64_MAX
    // (critical_juce_getlargeintvalue_clamps_unsigned_64).
    inline juce::String maskToHex (juce::uint64 m)
    {
        return "0x" + juce::String::toHexString ((juce::int64) m).paddedLeft ('0', 16);
    }

    inline juce::var tableToVar (const PitchCollections::Table& t)
    {
        auto* obj = new juce::DynamicObject();
        juce::Array<juce::var> cents;
        cents.ensureStorageAllocated (t.count);
        for (int i = 0; i < t.count; ++i)
            cents.add (t.cents[(size_t) i]);
        obj->setProperty ("cents", cents);
        obj->setProperty ("period", t.period);
        return juce::var (obj);
    }

    /** { revision, tables[32], custom{ name, text, period, version }, masks[4][32] }.
        Message thread. Collection 31 is read through getCustomTable() — a copy
        taken under the custom-scale lock — never through getCollectionTable (31),
        which hands out the live table unlocked (RESEARCH-3.1 R109). */
    inline juce::var buildScaleState (const GranulatorProcessor& proc)
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("revision", (int) (proc.getScaleStateRevision() & 0x3fffffffu));

        // tables[0] = null (Off); tables[1..30] = the built-ins; tables[31] = the
        // shared custom table. The page draws chips and the pitch circle from THESE
        // numbers — it carries no copy of the collections.
        juce::Array<juce::var> tables;
        for (int c = 0; c < kNumCollections; ++c)
        {
            if (c == kCustomCollection)
                tables.add (tableToVar (proc.getCustomTable()));
            else if (auto* t = proc.getCollectionTable (c))
                tables.add (tableToVar (*t));
            else
                tables.add (juce::var());
        }
        root->setProperty ("tables", tables);

        const auto custom = proc.getCustomScale();
        auto* cs = new juce::DynamicObject();
        cs->setProperty ("name", custom.name);
        cs->setProperty ("text", custom.text);
        cs->setProperty ("period", custom.period);
        cs->setProperty ("version", (int) (custom.version & 0x3fffffffu));
        root->setProperty ("custom", juce::var (cs));

        juce::Array<juce::var> masks;
        for (int s = 1; s <= kNumStreams; ++s)
        {
            juce::Array<juce::var> row;
            for (int c = 0; c < kNumCollections; ++c)
                row.add (maskToHex (proc.getDegreeMask (s, c)));
            masks.add (row);
        }
        root->setProperty ("masks", masks);
        return juce::var (root);
    }

    /** The getTelemetry payload. A key is emitted only when its field group is
        live (Telemetry::has*); an omitted key reads as "default" in the page,
        whose every read is guarded (RESEARCH-3.1 R106). 3.1 has sources, lfo,
        lfoPhase (zeros), env, pitch, modNorm and scaleRev; 3.2 / 3.3 set the
        remaining flags. `torn` = readTelemetry() returned false: only scaleRev
        goes out and the page keeps its previous frame. */
    inline juce::var telemetryToVar (const GranulatorProcessor::Telemetry& t, bool torn = false)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("scaleRev", (int) (t.scaleRev & 0x3fffffffu));
        if (torn)
            return juce::var (obj);

        if (t.hasBuffer)   // 3.2: wave ring + grain snapshot
        {
            obj->setProperty ("frozen", t.frozen);
            obj->setProperty ("spanSec", t.spanSec);

            juce::Array<juce::var> wave;
            wave.ensureStorageAllocated (t.waveCount);
            for (int i = 0; i < t.waveCount; ++i)
                wave.add (t.wave[(size_t) i]);
            obj->setProperty ("wave", wave);

            // [stream, delaySec, pitchSt, pan, env, gain, lengthSec] per sounding grain
            juce::Array<juce::var> grains;
            grains.ensureStorageAllocated (t.grainCount);
            for (int i = 0; i < t.grainCount; ++i)
            {
                const auto& g = t.grains[(size_t) i];
                grains.add (juce::Array<juce::var> { (int) g.stream, g.delaySec, g.pitchSt, g.pan,
                                                     g.env, g.gain, g.lengthSec });
            }
            obj->setProperty ("grains", grains);
        }

        if (t.hasCounts)   // 3.3
        {
            obj->setProperty ("counts", juce::Array<juce::var> { t.counts[0], t.counts[1], t.counts[2], t.counts[3] });
            obj->setProperty ("limit", t.limit);
        }

        if (t.hasMeters)   // 3.3: consuming peaks
        {
            obj->setProperty ("out", juce::Array<juce::var> { t.outPeakL, t.outPeakR });
            obj->setProperty ("inPeak", t.inPeak);
        }

        // 11 modulation sources in dock order: LFO 1-4 (-1..1), Env, Pitch, Onset, Macro 1-4 (0..1)
        juce::Array<juce::var> sources;
        for (auto v : t.sources)
            sources.add (v);
        obj->setProperty ("sources", sources);
        obj->setProperty ("lfo", juce::Array<juce::var> { t.sources[0], t.sources[1], t.sources[2], t.sources[3] });
        obj->setProperty ("lfoPhase", juce::Array<juce::var> { t.lfoPhase[0], t.lfoPhase[1], t.lfoPhase[2], t.lfoPhase[3] });
        obj->setProperty ("env", t.sources[4]);

        if (t.hasOnset)    // 3.3: the pulse and the since-last-read flag
        {
            obj->setProperty ("onset", t.onsetPulse);
            obj->setProperty ("onsetFired", t.onsetFired);
        }

        if (t.pitchMidi > 0.0f)
        {
            auto* p = new juce::DynamicObject();
            p->setProperty ("midi", t.pitchMidi);
            p->setProperty ("cents", t.pitchCents);
            p->setProperty ("clarity", t.pitchClarity);
            obj->setProperty ("pitch", juce::var (p));
        }

        // absent = loop open (feedback 0, or not live yet); otherwise dB, always < 0
        if (t.hasLoopGain && std::isfinite (t.loopGainDb))
            obj->setProperty ("loopGainDb", t.loopGainDb);

        // Modulated NORMALISED value for every destination that has a route, keyed
        // by destination index (the mod-ring live dot). Values are never written
        // back to the APVTS (ARCHITECTURE "Destinations append-only").
        auto* mod = new juce::DynamicObject();
        for (int d = 1; d < (int) t.modNorm.size(); ++d)
            if (t.modRouted[(size_t) d])
                mod->setProperty (juce::Identifier (juce::String (d)), t.modNorm[(size_t) d]);
        obj->setProperty ("modNorm", juce::var (mod));

        return juce::var (obj);
    }
}
