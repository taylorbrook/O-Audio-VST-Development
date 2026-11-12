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
        (never localized). `encodedBytes` (PLAN-3.2 PD20) is
        SlotInfo::encodedBytes: the FLAC bytes of the cached blob, 0 without
        one, 16 MiB + 1 when the encode was refused (so the page's 16 MB
        comparison fires) — no blob copy on a state pull. `dirty` (PD7) is the
        processor's cached "the working copy differs from the pristine" flag
        (false for a fixture built without a processor).
        No absolute path is emitted: the page reads none, and the editor takes
        paths from SlotInfo directly. */
   #if OGI_TEST_HOOKS
    inline bool testOmitWavFailed = false;   // PLAN-4 UWF1n control
   #endif

    inline juce::var slotInfoToVar (const ogi::SlotInfo& info, bool dirty = false)
    {
        const auto scheme = schemeOf (info.source);
        const int encodedBytes = static_cast<int> (juce::jlimit (static_cast<juce::int64> (0),
                                                                 ogi::SlotLoader::kEmbedMaxBytes + 1, info.encodedBytes));

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
        o->setProperty ("dirty", dirty);
        o->setProperty ("loading", info.loading);
        o->setProperty ("error", info.lastError);   // load / relink errors reach the page on the next state pull
        // PLAN-4 PD10 (D55a): the take's WAV could not be written (the take still
        // plays). The page edge-toasts it on the slotRev pull, so it works while
        // the host idles the audio callback. UWF1n omits it (test builds only).
       #if OGI_TEST_HOOKS
        if (! testOmitWavFailed)
       #endif
        o->setProperty ("wavFailed", info.wavFailed);
        return juce::var (o);
    }

    /** getSlotState: { revision, slots[4] }. `dirty` = the per-slot flags
        (PLAN-3.2 PD7); omitted → all false. */
    inline juce::var slotStateToVar (std::uint32_t revision, const std::array<ogi::SlotInfo, ogi::kTapeSlots>& infos,
                                     const std::array<bool, ogi::kTapeSlots>& dirty = {})
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("revision", revToInt (revision));
        juce::Array<juce::var> slots;
        for (std::size_t s = 0; s < infos.size(); ++s)
            slots.add (slotInfoToVar (infos[s], dirty[s]));
        root->setProperty ("slots", slots);
        return juce::var (root);
    }

    inline juce::var slotStateToVar (GranulatorInstrumentProcessor& p)
    {
        // The revision FIRST: an info that changes after it is caught by the next poll.
        const auto revision = p.getSlotStateRevision();
        std::array<ogi::SlotInfo, ogi::kTapeSlots> infos;
        std::array<bool, ogi::kTapeSlots> dirty {};
        for (int s = 0; s < ogi::kTapeSlots; ++s)
        {
            infos[static_cast<std::size_t> (s)] = p.getSlotInfo (s);
            dirty[static_cast<std::size_t> (s)] = p.getSlotDirty (s);   // the flag the revision was hashed with
        }
        return slotStateToVar (revision, infos, dirty);
    }

    //==========================================================================
    /** getTelemetry, the 3.1 PLACEHOLDER shape (PLAN-3.1 PD8, exact). Kept
        for the 3.1 editor call site and row UTL31 until the editor switches to
        the 3.2 form below (PLAN-3.2 T8); it reads no consuming feed. ALWAYS
        the full object — the page replaces its telemetry wholesale on each answer:

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
    /** getTelemetry, the 3.3 shape (PLAN-3.3 PD13; 3.2 PD13 extended) from the
        processor's ONE telemetry builder (GranulatorInstrumentProcessor::
        buildUiTelemetry — the editor passes Time::getMillisecondCounterHiRes(),
        the harness a scripted clock). ALWAYS the full object, 14 keys:

          { scaleRev, slotRev,
            lanes: [ { ovRev, fb: 0…1 | null, rec: 0…1 | null[, lo, hi] } × 4 ],
            voices: [ [state 0…3, note, amp, scan1, scan2, scan3, scan4, wantSt] × 16 ],
            newest: row | -1,
            menv: [ [e1Level, e1Stage, e2Level, e2Stage, ampStage] × 16 ],
            lfoPh: [ [global, row0 … row15] × 4 ],
            grains: [ [stream 1…4, normPos, pan, env, gain, lengthSec, voice row, flags] … ],
            counts: [n1, n2, n3, n4], limit: 32 << grain_limit, out: [L, R],
            rec: { bus: 0 | 1, signal, run: 0…5, slot: 0…4, pos, len, capped,
                   peak: [L, R] },
            sources: [17 values], modNorm: { "d": v } (routed d only) }

        Absent stays absent / null, never 0: fb / rec null; lo / hi only
        while a loop destination of the lane is routed; modNorm keys only for
        routed destinations. scan = −1 → no playhead; lfoPh −1 → no dot.
        flags: bit 0 reversed, bit 1 the voice's wanted pitch beyond ±48 st on
        that stream. frozen, modRouted and stale are not emitted. Floats are
        rounded to 4 decimals here (in double, so the JSON stays short); a
        non-finite value is sent as 0. */
    inline juce::var telemetryToVar (const GranulatorInstrumentProcessor::UiTelemetry& t)
    {
        const auto r4 = [] (double v) { return std::isfinite (v) ? std::round (v * 10000.0) / 10000.0 : 0.0; };

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("scaleRev", revToInt (t.scaleRev));
        obj->setProperty ("slotRev",  revToInt (t.slotRev));

        juce::Array<juce::var> lanes;
        for (const auto& lane : t.lanes)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("ovRev", revToInt (lane.ovRev));
            o->setProperty ("fb",  lane.hasFb  ? juce::var (r4 (static_cast<double> (lane.fb)))  : juce::var());
            o->setProperty ("rec", lane.hasRec ? juce::var (r4 (static_cast<double> (lane.rec))) : juce::var());
            if (lane.hasLoHi)
            {
                o->setProperty ("lo", r4 (static_cast<double> (lane.lo)));
                o->setProperty ("hi", r4 (static_cast<double> (lane.hi)));
            }
            lanes.add (juce::var (o));
        }
        obj->setProperty ("lanes", lanes);

        juce::Array<juce::var> voices;
        voices.ensureStorageAllocated (static_cast<int> (t.voices.size()));
        for (const auto& v : t.voices)
        {
            juce::Array<juce::var> row;
            row.ensureStorageAllocated (8);
            row.add (v.state);
            row.add (v.note);
            row.add (r4 (static_cast<double> (v.amp)));
            for (const auto sc : v.scan)
                row.add (r4 (static_cast<double> (sc)));
            row.add (r4 (static_cast<double> (v.wantSt)));
            voices.add (juce::var (row));
        }
        obj->setProperty ("voices", voices);
        obj->setProperty ("newest", t.newest);

        juce::Array<juce::var> menv;
        menv.ensureStorageAllocated (static_cast<int> (t.menv.size()));
        for (const auto& m : t.menv)
        {
            juce::Array<juce::var> row;
            row.ensureStorageAllocated (5);
            row.add (r4 (static_cast<double> (m.e1)));
            row.add (m.st1);
            row.add (r4 (static_cast<double> (m.e2)));
            row.add (m.st2);
            row.add (m.ampSt);
            menv.add (juce::var (row));
        }
        obj->setProperty ("menv", menv);

        juce::Array<juce::var> lfoPh;
        for (const auto& lr : t.lfoPh)
        {
            juce::Array<juce::var> row;
            row.ensureStorageAllocated (static_cast<int> (lr.size()));
            for (const auto ph : lr)
                row.add (r4 (static_cast<double> (ph)));
            lfoPh.add (juce::var (row));
        }
        obj->setProperty ("lfoPh", lfoPh);

        juce::Array<juce::var> grains;
        const int numGrains = juce::jlimit (0, static_cast<int> (t.grains.size()), t.grainCount);
        grains.ensureStorageAllocated (numGrains);
        for (int i = 0; i < numGrains; ++i)
        {
            const auto& g = t.grains[static_cast<std::size_t> (i)];
            juce::Array<juce::var> row;
            row.ensureStorageAllocated (8);
            row.add (g.stream);
            row.add (r4 (static_cast<double> (g.normPos)));
            row.add (r4 (static_cast<double> (g.pan)));
            row.add (r4 (static_cast<double> (g.env)));
            row.add (r4 (static_cast<double> (g.gain)));
            row.add (r4 (static_cast<double> (g.lengthSec)));
            row.add (g.voice);
            row.add (g.flags);
            grains.add (juce::var (row));
        }
        obj->setProperty ("grains", grains);

        juce::Array<juce::var> counts;
        for (const auto c : t.counts)
            counts.add (c);
        obj->setProperty ("counts", counts);
        obj->setProperty ("limit", t.limit);

        juce::Array<juce::var> out;
        out.add (r4 (static_cast<double> (t.outL)));
        out.add (r4 (static_cast<double> (t.outR)));
        obj->setProperty ("out", out);

        auto* rec = new juce::DynamicObject();
        rec->setProperty ("bus", t.rec.bus > 0 ? 1 : 0);
        rec->setProperty ("signal", t.rec.signal);
        rec->setProperty ("run", t.rec.run);
        rec->setProperty ("slot", t.rec.slot);
        rec->setProperty ("pos", r4 (static_cast<double> (t.rec.pos)));
        rec->setProperty ("len", r4 (static_cast<double> (t.rec.len)));
        rec->setProperty ("capped", t.rec.capped);
        juce::Array<juce::var> peak;
        peak.add (r4 (static_cast<double> (t.rec.peakL)));
        peak.add (r4 (static_cast<double> (t.rec.peakR)));
        rec->setProperty ("peak", peak);
        obj->setProperty ("rec", juce::var (rec));

        juce::Array<juce::var> sources;
        sources.ensureStorageAllocated (static_cast<int> (t.sources.size()));
        for (const auto sv : t.sources)
            sources.add (r4 (static_cast<double> (sv)));
        obj->setProperty ("sources", sources);

        auto* modNorm = new juce::DynamicObject();
        for (std::size_t d = 1; d < t.modRouted.size(); ++d)
            if (t.modRouted[d])
                modNorm->setProperty (juce::Identifier (juce::String (static_cast<int> (d))), r4 (static_cast<double> (t.modNorm[d])));
        obj->setProperty ("modNorm", juce::var (modNorm));
        return juce::var (obj);
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
        empty state). Peaks (pristine with the live bins over them, PLAN-3.2
        PD7) are rounded to 3 decimals (in double, so the JSON stays short);
        dirty is 0…255 (GranulatorInstrumentProcessor::kDirtyFullScale). */
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
