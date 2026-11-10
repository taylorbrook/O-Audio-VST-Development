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

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>
#include <cstdint>
#include "PluginProcessor.h"

// ============================================================================
// GiUiJson — the ONE encoder of what the page pulls (PLAN-3.1 PD8 / PD10,
// RESEARCH-3.1 §2 / §3). juce_core + processor types only: no WebView type, so
// the render harness (JUCE_WEB_BROWSER=0, no editor) compiles it too. The
// editor's native functions and the harness's --dump-ui-overrides both call
// these, so the stub fixture is produced by the code the page talks to.
//
// Message thread. Each *ToVar (processor) form has a pure form underneath that
// takes plain values, for fixtures that need no loaded file.
//
// Conventions:
//   * revisions cross as non-negative ints (& 0x3fffffff);
//   * uint64 masks cross as "0x" + 16 hex digits (a JS number cannot hold 64
//     bits);
//   * slots are 0…3 here; the page's 1…4 appears only in the `slot` key of the
//     overview answer.
// ============================================================================
namespace GiUiJson
{
    inline constexpr int kOverviewPeaks     = GranulatorInstrumentProcessor::kOverviewPeaks;   // 656
    inline constexpr int kWindowCurvePoints = 64;

    /** A revision as the page compares it. */
    inline int revToInt (std::uint32_t r) noexcept { return static_cast<int> (r & 0x3fffffffu); }

    inline juce::String maskToHex (std::uint64_t m)
    {
        return "0x" + juce::String::toHexString (static_cast<juce::int64> (m)).paddedLeft ('0', 16);
    }

    inline juce::var tableToVar (const PitchCollections::Table& t)
    {
        auto* obj = new juce::DynamicObject();
        juce::Array<juce::var> cents;
        cents.ensureStorageAllocated (t.count);
        for (int i = 0; i < t.count; ++i)
            cents.add (static_cast<double> (t.cents[static_cast<std::size_t> (i)]));
        obj->setProperty ("cents", cents);
        obj->setProperty ("period", static_cast<double> (t.period));
        return juce::var (obj);
    }

    //==========================================================================
    /** getScaleState: { revision, tables[32] (0 = null (Off), 1…30 the
        built-ins, 31 the shared custom table), custom { name, text, period,
        version }, masks[4][32] }. The page draws chips and the pitch circle
        from THESE numbers; it carries no copy of the collections. */
    inline juce::var scaleStateToVar (const GranulatorInstrumentProcessor& p)
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("revision", revToInt (p.getScaleStateRevision()));

        juce::Array<juce::var> tables;
        for (int c = 0; c < GiParamIds::kNumCollections; ++c)
        {
            if (const auto* t = p.getCollectionTable (c))
                tables.add (tableToVar (*t));
            else
                tables.add (juce::var());
        }
        root->setProperty ("tables", tables);

        const auto custom = p.getCustomScale();
        auto* cs = new juce::DynamicObject();
        cs->setProperty ("name", custom.name);
        cs->setProperty ("text", custom.text);
        cs->setProperty ("period", static_cast<double> (custom.period));
        cs->setProperty ("version", revToInt (custom.version));
        root->setProperty ("custom", juce::var (cs));

        juce::Array<juce::var> masks;
        for (int s = 1; s <= GiParamIds::kStreams; ++s)
        {
            juce::Array<juce::var> row;
            for (int c = 0; c < GiParamIds::kNumCollections; ++c)
                row.add (maskToHex (p.getDegreeMask (s, c)));
            masks.add (row);
        }
        root->setProperty ("masks", masks);
        return juce::var (root);
    }

    //==========================================================================
    /** "factory" | "file" | "rec" | "" from the prefix of SlotInfo::source. */
    inline juce::String schemeOf (const juce::String& source)
    {
        if (source.startsWith ("factory:")) return "factory";
        if (source.startsWith ("file:"))    return "file";
        if (source.startsWith ("rec:"))     return "rec";
        return {};
    }

    /** One slot as the page reads it (RESEARCH-3.1 §2). `name` is user text
        (never localized). 3.1 values: `encodedBytes` is 0, or 16 MiB + 1 when
        the encode was refused (so the page's 16 MB comparison fires; the exact
        figure is 3.2); `dirty` is false (needs the record-head epoch, 3.2).
        No absolute path is emitted: the page reads none, and the editor takes
        paths from SlotInfo directly. */
    inline juce::var slotInfoToVar (const ogi::SlotInfo& info)
    {
        const auto scheme = schemeOf (info.source);
        const int encodedBytes = info.tooLarge ? static_cast<int> (ogi::SlotLoader::kEmbedMaxBytes) + 1 : 0;

        auto* o = new juce::DynamicObject();
        o->setProperty ("scheme", scheme);
        o->setProperty ("id", scheme == "factory" ? info.source.substring (8) : juce::String());   // factory:<id>
        o->setProperty ("name", info.name);
        o->setProperty ("seconds", info.loaded && info.tapeRate > 0.0
                                       ? static_cast<double> (info.frames) / info.tapeRate : 0.0);
        o->setProperty ("sourceSeconds", info.loaded && info.nativeRate > 0.0
                                             ? static_cast<double> (info.nativeFrames) / info.nativeRate : 0.0);
        o->setProperty ("trimmed", info.trimmed);
        o->setProperty ("missing", info.missing);
        o->setProperty ("embed", info.embed);
        o->setProperty ("encodedBytes", encodedBytes);
        o->setProperty ("embeddable", encodedBytes <= static_cast<int> (ogi::SlotLoader::kEmbedMaxBytes) && scheme != "factory");
        o->setProperty ("dirty", false);
        o->setProperty ("loading", info.loading);
        o->setProperty ("error", info.lastError);   // load / relink errors reach the page on the next state pull
        return juce::var (o);
    }

    /** getSlotState: { revision, slots[4] }. */
    inline juce::var slotStateToVar (std::uint32_t revision, const std::array<ogi::SlotInfo, ogi::kTapeSlots>& infos)
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("revision", revToInt (revision));
        juce::Array<juce::var> slots;
        for (const auto& info : infos)
            slots.add (slotInfoToVar (info));
        root->setProperty ("slots", slots);
        return juce::var (root);
    }

    inline juce::var slotStateToVar (GranulatorInstrumentProcessor& p)
    {
        // The revision FIRST: an info that changes after it is caught by the next poll.
        const auto revision = p.getSlotStateRevision();
        std::array<ogi::SlotInfo, ogi::kTapeSlots> infos;
        for (int s = 0; s < ogi::kTapeSlots; ++s)
            infos[static_cast<std::size_t> (s)] = p.getSlotInfo (s);
        return slotStateToVar (revision, infos);
    }

    //==========================================================================
    /** getTelemetry, the 3.1 shape (PLAN-3.1 PD8, exact). ALWAYS the full
        object — the page replaces its telemetry wholesale on each answer:

          { scaleRev, slotRev,
            lanes: [ { ovRev, fb: null, rec: null } × 4 ],     (no lo / hi)
            voices: [], newest: -1, menv: [], grains: [],
            counts: [0, 0, 0, 0], limit: 0, out: [0, 0],
            rec: { bus: 0 | 1, signal: false, run: 0, slot: 0, pos: 0, len: 0,
                   capped: false, peak: [0, 0] },
            sources: [], modNorm: {} }

        lfoPh, frozen and modRouted are omitted. fb / rec MUST be null (any
        number draws a head / marks the lane recording); lo / hi MUST be absent
        (the page then uses the loop handles); counts MUST be four numbers;
        rec.bus is 0 or 1, never a channel count. The consuming feeds (peaks,
        the grain frame) are not read in 3.1. */
    inline juce::var telemetryToVar (std::uint32_t scaleRev, std::uint32_t slotRev,
                                     const std::array<std::uint32_t, ogi::kTapeSlots>& ovRevs, int sidechainChannels)
    {
        const auto emptyArray = [] { return juce::var (juce::Array<juce::var>()); };
        const auto zeros = [] (int n)
        {
            juce::Array<juce::var> a;
            for (int i = 0; i < n; ++i)
                a.add (0);
            return juce::var (a);
        };

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("scaleRev", revToInt (scaleRev));
        obj->setProperty ("slotRev",  revToInt (slotRev));

        juce::Array<juce::var> lanes;
        for (const auto ovRev : ovRevs)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("ovRev", revToInt (ovRev));
            o->setProperty ("fb", juce::var());
            o->setProperty ("rec", juce::var());
            lanes.add (juce::var (o));
        }
        obj->setProperty ("lanes", lanes);

        obj->setProperty ("voices", emptyArray());
        obj->setProperty ("newest", -1);
        obj->setProperty ("menv", emptyArray());
        obj->setProperty ("grains", emptyArray());
        obj->setProperty ("counts", zeros (ogi::kTapeSlots));
        obj->setProperty ("limit", 0);
        obj->setProperty ("out", zeros (2));

        auto* rec = new juce::DynamicObject();
        rec->setProperty ("bus", sidechainChannels > 0 ? 1 : 0);
        rec->setProperty ("signal", false);
        rec->setProperty ("run", 0);
        rec->setProperty ("slot", 0);
        rec->setProperty ("pos", 0);
        rec->setProperty ("len", 0);
        rec->setProperty ("capped", false);
        rec->setProperty ("peak", zeros (2));
        obj->setProperty ("rec", juce::var (rec));

        obj->setProperty ("sources", emptyArray());
        obj->setProperty ("modNorm", juce::var (new juce::DynamicObject()));
        return juce::var (obj);
    }

    inline juce::var telemetryToVar (GranulatorInstrumentProcessor& p)
    {
        std::array<std::uint32_t, ogi::kTapeSlots> ovRevs {};
        for (int s = 0; s < ogi::kTapeSlots; ++s)
            ovRevs[static_cast<std::size_t> (s)] = p.getOverviewRev (s);
        const auto scaleRev = p.getScaleStateRevision();
        const auto slotRev  = p.getSlotStateRevision();
        return telemetryToVar (scaleRev, slotRev, ovRevs, p.getSidechainChannelCount());
    }

    //==========================================================================
    /** getParameterDefaults (dblclick reset): every Float in ENGINEERING units
        and every Choice as its INDEX, keyed by parameter ID, in registration
        order. Read from the parameter objects, so it cannot drift from the
        NormalisableRange. Bools are not reset through it and are omitted. */
    inline juce::var parameterDefaultsToVar (const GranulatorInstrumentProcessor& p)
    {
        using Kind = GiParamIds::Kind;
        auto* obj = new juce::DynamicObject();
        for (const auto& e : GiParamIds::registrationOrder())
        {
            const auto* param = p.parameters.getParameter (e.id);
            if (param == nullptr)
                continue;
            const float v = param->convertFrom0to1 (param->getDefaultValue());
            switch (e.def->kind)
            {
                case Kind::Float:  obj->setProperty (e.id, static_cast<double> (v)); break;
                case Kind::Choice: obj->setProperty (e.id, juce::roundToInt (v));    break;
                case Kind::Bool:   break;
            }
        }
        return juce::var (obj);
    }

    //==========================================================================
    /** getSlotOverview for slot 0…3: { slot (1…4), rev, peaks[656], dirty[656] },
        or { slot } alone when the slot holds no tape (the lane then draws its
        empty state). Peaks are rounded to 3 decimals (in double, so the JSON
        stays short). */
    inline juce::var slotOverviewToVar (const GranulatorInstrumentProcessor& p, int slot0)
    {
        std::array<float, static_cast<std::size_t> (kOverviewPeaks)> peaks {};
        std::array<std::uint8_t, static_cast<std::size_t> (kOverviewPeaks)> dirty {};
        std::uint32_t rev = 0;

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("slot", slot0 + 1);
        if (! p.getSlotOverview (slot0, peaks.data(), dirty.data(), kOverviewPeaks, rev))
            return juce::var (obj);

        juce::Array<juce::var> pk, dt;
        pk.ensureStorageAllocated (kOverviewPeaks);
        dt.ensureStorageAllocated (kOverviewPeaks);
        for (std::size_t i = 0; i < peaks.size(); ++i)
        {
            pk.add (std::round (static_cast<double> (peaks[i]) * 1000.0) / 1000.0);
            dt.add (static_cast<int> (dirty[i]));
        }
        obj->setProperty ("rev", revToInt (rev));
        obj->setProperty ("peaks", pk);
        obj->setProperty ("dirty", dt);
        return juce::var (obj);
    }

    /** getWindowCurve for stream 0…3: 64 points of the engine's own curve. */
    inline juce::var windowCurveToVar (const GranulatorInstrumentProcessor& p, int stream0)
    {
        std::array<float, static_cast<std::size_t> (kWindowCurvePoints)> curve {};
        p.getWindowCurve (stream0, curve.data(), kWindowCurvePoints);
        juce::Array<juce::var> pts;
        pts.ensureStorageAllocated (kWindowCurvePoints);
        for (const auto v : curve)
            pts.add (static_cast<double> (v));
        return juce::var (pts);
    }
} // namespace GiUiJson
