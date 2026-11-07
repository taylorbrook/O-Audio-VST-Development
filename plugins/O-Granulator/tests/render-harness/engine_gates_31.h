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

//==============================================================================
// Stage 3 / Phase 3.1 rows (PLAN-3.1 P104). The harness compiles the processor,
// never the editor, so the processor surface the editor talks to is gated here:
//
//   PM1  preset round trip (all params but output_gain, custom scale, masks, seed)
//   PM2  output_gain excluded: a load never resets it; the file never carries it
//   PM3  preset with no customState: scale back to 12-EDO, masks all-on, seed KEPT
//   PM4  two reopens after a preset load: final == the changed state; no stale
//        <CustomState> / duplicate custom child in the live tree
//   PM5  seed 0xFEDC'BA98'7654'3211 (> INT64_MAX) exact through a preset
//   WC1  windowCurve (0, 0) == closed-form Hann; WC2 skew ±100 / window 1 live
//   (TL31 was rewritten as TL32 in engine_gates_32.h at 3.2 — R126)
//   NCs  PM2n (exclusion off), PM3n (seed reset on absent), PM5n (seed written
//        as a JSON number) — each row PASSes only when the variant FAILS the
//        positive row's predicate.
//
// Preset files go to a fresh temp directory (removed at the end); nothing is
// written under ~/Library.
//==============================================================================
namespace { namespace eg31
{
    struct TempDir
    {
        juce::File dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                             .getNonexistentChildFile ("ogran-31-presets", "", false);
        TempDir()  { dir.createDirectory(); }
        ~TempDir() { dir.deleteRecursively(); }
        juce::File file (const juce::String& name) const { return dir.getChildFile (name + ".json"); }
    };

    // A deterministic non-default normalised value for every parameter.
    inline float pattern (int i) { const double v = std::fmod (0.6180339887 * (i + 1) + 0.1, 1.0); return (float) v; }

    inline void setAllPattern (juce::AudioProcessor& p)
    {
        int i = 0;
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                rp->setValueNotifyingHost (pattern (i++));
    }

    // Every parameter but `skip` equal (normalised, 1e-6). Returns the mismatch count.
    inline int paramDiffs (juce::AudioProcessor& a, juce::AudioProcessor& b, const juce::String& skip, juce::String& first)
    {
        int bad = 0;
        auto& pa = a.getParameters(); auto& pb = b.getParameters();
        for (int i = 0; i < pa.size(); ++i)
        {
            auto* ra = dynamic_cast<juce::RangedAudioParameter*> (pa[i]);
            auto* rb = dynamic_cast<juce::RangedAudioParameter*> (pb[i]);
            if (ra == nullptr || rb == nullptr || ra->getParameterID() == skip) continue;
            if (std::abs (ra->getValue() - rb->getValue()) > 1e-6f)
            {
                if (bad++ == 0) first = ra->getParameterID() + " " + juce::String (ra->getValue(), 6) + " vs " + juce::String (rb->getValue(), 6);
            }
        }
        return bad;
    }

    // Seed + custom (name, text, table) + all 4 x 31 masks. uiLanguage is not preset state.
    inline bool sameScaleAndSeed (GranulatorProcessor& a, GranulatorProcessor& b, juce::String& why)
    {
        if (a.getSeed() != b.getSeed()) { why = "seed"; return false; }
        const auto ca = a.getCustomScale(), cb = b.getCustomScale();
        if (ca.text != cb.text || ca.name != cb.name) { why = "custom text/name"; return false; }
        const auto ta = a.getCustomTable(), tb = b.getCustomTable();
        if (ta.count != tb.count || std::memcmp (ta.cents.data(), tb.cents.data(), sizeof (float) * (size_t) ta.count) != 0) { why = "custom table"; return false; }
        for (int s = 1; s <= 4; ++s)
            for (int c = 1; c < 32; ++c)
                if (a.getDegreeMask (s, c) != b.getDegreeMask (s, c)) { why = "mask s" + juce::String (s) + " c" + juce::String (c); return false; }
        return true;
    }

    inline bool allMasksOn (GranulatorProcessor& p)
    {
        for (int s = 1; s <= 4; ++s)
            for (int c = 1; c < 32; ++c)
                if (p.getDegreeMask (s, c) != GranulatorProcessor::kAllOn) return false;
        return true;
    }

    inline float eng (juce::AudioProcessor& p, const juce::String& id)
    {
        auto* rp = param (p, id);
        return rp != nullptr ? rp->convertFrom0to1 (rp->getValue()) : std::numeric_limits<float>::quiet_NaN();
    }

    inline juce::var readJson (const juce::File& f) { return juce::JSON::parse (f.loadFileAsString()); }
    inline void writeJson (const juce::File& f, const juce::var& v) { f.replaceWithText (juce::JSON::toString (v, true), false, false, "\n"); }

    // Live-tree hygiene (critical_preset_manager_stale_customstate_child): no
    // CustomState child, and none of the codec's own children, ever in parameters.state.
    inline int liveCustomChildren (GranulatorProcessor& p)
    {
        int n = 0;
        for (int i = 0; i < p.parameters.state.getNumChildren(); ++i)
        {
            const auto t = p.parameters.state.getChild (i).getType().toString();
            n += (t == "CustomState" || t == "Engine" || t == "CustomScale" || t == "DegreeMasks") ? 1 : 0;
        }
        return n;
    }

    constexpr uint64_t kPmSeed  = 0x0123456789ABCDEFull;
    constexpr uint64_t kBigSeed = 0xFEDCBA9876543211ull;   // > INT64_MAX as uint64

    // The PM1 source instance: pattern params, output_gain −6 dB, custom scale,
    // three non-default masks (one on Custom, set AFTER the scale), the seed.
    inline std::unique_ptr<GranulatorProcessor> pmSource()
    {
        auto a = make();
        setAllPattern (*a);
        setParam (*a, "output_gain", -6.0f);
        a->setSeed (kPmSeed);
        juce::String bad;
        a->setCustomScale ("Pelog-ish", "120 270 540 670 785 950 1200", bad);
        a->setDegreeMask (1, 19, 0x7ull);
        a->setDegreeMask (2, 23, 0x8000000000000001ull);
        a->setDegreeMask (4, 31, 0x5ull);
        return a;
    }
} }

static void engineRows31()
{
    using namespace eg31;
    std::printf ("\n-- PM. preset manager (Stage 3 / 3.1, P98-P100)\n");
    TempDir tmp;

    // PM1 — round trip.
    {
        auto a = pmSource();
        const bool saved = a->getPresetManager().savePresetToFile (tmp.file ("pm1"));
        auto b = make();
        b->setSeed (0x42ull);
        const float ogBefore = eng (*b, "output_gain");
        const auto rev0 = b->getScaleStateRevision();
        const bool loaded = b->getPresetManager().loadPresetFromFile (tmp.file ("pm1"));
        juce::String first, why;
        const int diffs = paramDiffs (*a, *b, "output_gain", first);
        const bool same = sameScaleAndSeed (*a, *b, why);
        row (saved && loaded && diffs == 0 && same && b->getScaleStateRevision() != rev0
             && eng (*b, "output_gain") == ogBefore,
             "PM1 preset round trip: 219 params, custom scale, 3 masks, seed",
             "param diffs " + juce::String (diffs) + (first.isNotEmpty() ? " (" + first + ")" : juce::String())
             + "; scale/seed " + (same ? "equal" : why) + "; rev " + juce::String ((int) rev0) + " -> " + juce::String ((int) b->getScaleStateRevision())
             + "; masks 1/19 " + juce::String::toHexString ((juce::int64) b->getDegreeMask (1, 19)));
    }

    // PM2 / PM2n — output_gain excluded.
    {
        auto a = make();                                  // saved at 0 dB
        a->getPresetManager().savePresetToFile (tmp.file ("pm2"));
        const auto j = readJson (tmp.file ("pm2"));
        const bool keyAbsent = j.getProperty ("parameters", {}).getDynamicObject() != nullptr
                               && ! j.getProperty ("parameters", {}).getDynamicObject()->hasProperty ("output_gain")
                               && j.getProperty ("parameters", {}).getDynamicObject()->hasProperty ("input_gain");
        auto b = make();
        setParam (*b, "output_gain", -12.0f);
        b->getPresetManager().loadPresetFromFile (tmp.file ("pm2"));
        const float og = eng (*b, "output_gain");
        const bool pred = std::abs (og - (-12.0f)) < 1e-4f;
        row (pred && keyAbsent, "PM2 output_gain -12 dB survives a load; the file has no output_gain key",
             "after load " + juce::String (og, 3) + " dB; key " + (keyAbsent ? "absent" : "PRESENT"));

        // NC: exclusion off on both sides — the preset carries 0 dB and the load applies it.
        auto a2 = make();
        a2->getPresetManager().setExcludedParameters ({});
        a2->getPresetManager().savePresetToFile (tmp.file ("pm2n"));
        auto b2 = make();
        b2->getPresetManager().setExcludedParameters ({});
        setParam (*b2, "output_gain", -12.0f);
        b2->getPresetManager().loadPresetFromFile (tmp.file ("pm2n"));
        const float og2 = eng (*b2, "output_gain");
        const bool predN = std::abs (og2 - (-12.0f)) < 1e-4f;
        row (! predN, "PM2n NC exclusion off: -12 dB is lost (PM2's predicate FAILS)", "after load " + juce::String (og2, 3) + " dB");
    }

    // PM3 / PM3n — no customState: scale + masks reset, seed kept.
    {
        auto a = pmSource();
        a->getPresetManager().savePresetToFile (tmp.file ("pm3"));
        auto j = readJson (tmp.file ("pm3"));
        if (auto* o = j.getDynamicObject()) o->removeProperty ("customState");
        writeJson (tmp.file ("pm3"), j);

        auto prime = [] (GranulatorProcessor& b)
        {
            b.setSeed (0x1234ull);
            juce::String bad;
            b.setCustomScale ("x", "5/4 3/2 2/1", bad);
            b.setDegreeMask (3, 19, 0x5ull);
        };
        auto b = make();
        prime (*b);
        const bool loaded = b->getPresetManager().loadPresetFromFile (tmp.file ("pm3"));
        const auto cs = b->getCustomScale();
        const bool reset = cs.text == GranulatorProcessor::defaultCustomText() && cs.name.isEmpty()
                           && b->getCustomTable().count == 12 && allMasksOn (*b);
        const bool pred = b->getSeed() == 0x1234ull;
        row (loaded && reset && pred, "PM3 no customState: custom 12-EDO, masks all-on, seed unchanged",
             juce::String ("reset ") + (reset ? "yes" : "NO") + "; seed " + juce::String::toHexString ((juce::int64) b->getSeed()));

        // NC: a load path that resets the seed when the preset has none.
        auto b2 = make();
        prime (*b2);
        auto* raw = b2.get();
        b2->getPresetManager().setCustomStateCallbacks ([raw] { return raw->presetCustomState(); },
            [raw] (const juce::var& v)
            {
                if (! v.getDynamicObject() || ! v.getDynamicObject()->hasProperty ("seed")) raw->setSeed (0);
                raw->applyPresetCustomState (v);
            });
        b2->getPresetManager().loadPresetFromFile (tmp.file ("pm3"));
        const bool predN = b2->getSeed() == 0x1234ull;
        row (! predN, "PM3n NC seed reset on absent: the seed differs (PM3's predicate FAILS)",
             "seed " + juce::String::toHexString ((juce::int64) b2->getSeed()));
    }

    // PM4 — two reopens after a preset load.
    {
        auto a = pmSource();
        a->getPresetManager().savePresetToFile (tmp.file ("pm4"));
        auto b = make();
        b->getPresetManager().loadPresetFromFile (tmp.file ("pm4"));
        const int liveB = liveCustomChildren (*b);
        const auto blob1 = stateOf (*b);
        auto c = make();
        load (*c, blob1);
        juce::String why1;
        const bool reopen1 = sameScaleAndSeed (*b, *c, why1);
        c->setDegreeMask (3, 5, 0x3ull);
        const auto blob2 = stateOf (*c);
        auto d = make();
        load (*d, blob2);
        juce::String why, first;
        const bool same = sameNonParamState (*c, *d, why);
        const int diffs = paramDiffs (*c, *d, {}, first);
        const auto x = xmlOf (blob2);
        const bool oneEach = x != nullptr && countChildren (*x, "Engine") == 1 && countChildren (*x, "CustomScale") == 1
                             && countChildren (*x, "DegreeMasks") == 1 && countChildren (*x, "CustomState") == 0;
        const int live = liveB + liveCustomChildren (*c) + liveCustomChildren (*d);
        row (reopen1 && same && diffs == 0 && oneEach && live == 0
             && d->getDegreeMask (3, 5) == GranulatorProcessor::canonicalMask (0x3ull, d->getCollectionTable (5)->count),
             "PM4 preset load -> reopen -> change -> reopen: final == changed; no stale child",
             juce::String ("reopen1 ") + (reopen1 ? "equal" : why1) + "; final " + (same ? "equal" : why)
             + "; param diffs " + juce::String (diffs) + "; XML one each " + (oneEach ? "yes" : "NO")
             + "; live custom children " + juce::String (live));
    }

    // PM5 / PM5n — seed > INT64_MAX.
    {
        auto a = make();
        a->setSeed (kBigSeed);
        a->getPresetManager().savePresetToFile (tmp.file ("pm5"));
        const auto j = readJson (tmp.file ("pm5"));
        const auto sv = j.getProperty ("customState", {}).getProperty ("seed", {});
        auto b = make();
        b->setSeed (0x77ull);
        b->getPresetManager().loadPresetFromFile (tmp.file ("pm5"));
        row (b->getSeed() == kBigSeed && sv.isString() && sv.toString() == juce::String ((juce::int64) kBigSeed),
             "PM5 seed 0xFEDCBA9876543211 exact through a preset (int64 decimal string)",
             "file " + sv.toString() + "; loaded " + juce::String::toHexString ((juce::int64) b->getSeed()));

        // NC: the same preset with the seed written as a JSON number (a double).
        auto jn = readJson (tmp.file ("pm5"));
        if (auto* cs = jn.getProperty ("customState", {}).getDynamicObject())
            cs->setProperty ("seed", (double) (juce::int64) kBigSeed);
        writeJson (tmp.file ("pm5n"), jn);
        auto b2 = make();
        b2->setSeed (0x77ull);
        b2->getPresetManager().loadPresetFromFile (tmp.file ("pm5n"));
        row (b2->getSeed() != kBigSeed, "PM5n NC seed as a JSON number: mismatch (PM5's predicate FAILS)",
             "loaded " + juce::String::toHexString ((juce::int64) b2->getSeed()));
    }

    std::printf ("\n-- WC. window figure (Stage 3 / 3.1, P101)\n");
    {
        auto p = make();
        const auto& e = p->getEngineForTest();
        constexpr int N = 64;
        float hann[N], c0[N];
        e.windowCurve (0.0f, 0.0f, c0, N);
        double worst = 0.0;
        for (int i = 0; i < N; ++i)
        {
            hann[i] = (float) (0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * i / (N - 1)));
            worst = juce::jmax (worst, (double) std::abs (c0[i] - hann[i]));
        }
        row (worst <= 1e-6, "WC1 windowCurve (0, 0, 64) == closed-form Hann (1e-6)", "worst " + juce::String (worst, 3, true));

        Tally t;
        struct Case { float w, sk; const char* name; } cases[] = { { 0.0f, 100.0f, "skew +100" }, { 0.0f, -100.0f, "skew -100" },
                                                                   { 1.0f, 0.0f, "window 1" }, { 0.5f, 37.0f, "window 0.5 skew 37" } };
        for (auto& cse : cases)
        {
            float c[N];
            e.windowCurve (cse.w, cse.sk, c, N);
            bool ok = std::abs (c[0]) <= 1e-6f && std::abs (c[N - 1]) <= 1e-6f;
            double diff = 0.0;
            for (int i = 0; i < N; ++i)
            {
                ok = ok && std::isfinite (c[i]) && c[i] >= 0.0f && c[i] <= 1.0f;
                diff = juce::jmax (diff, (double) std::abs (c[i] - c0[i]));
            }
            t.check (ok && diff > 1e-3, juce::String (cse.name) + " diff " + juce::String (diff, 4));
        }
        // The processor reads the BASE parameters of the stream asked for.
        setParam (*p, "s2_window", 0.7f);
        setParam (*p, "s2_window_skew", -40.0f);
        float viaProc[N], direct[N];
        p->getWindowCurve (2, viaProc, N);
        const float rw = p->parameters.getRawParameterValue ("s2_window")->load();
        const float rs = p->parameters.getRawParameterValue ("s2_window_skew")->load();
        e.windowCurve (rw, rs, direct, N);
        float other[N];
        e.windowCurve (0.0f, 0.0f, other, N);
        t.check (std::memcmp (viaProc, direct, sizeof (viaProc)) == 0 && std::memcmp (viaProc, other, sizeof (viaProc)) != 0,
                 "getWindowCurve (2) != windowCurve (raw s2_window, raw s2_window_skew)");
        row (t.bad == 0, "WC2 skew +-100 / window 1 / 0.5: finite, in [0, 1], endpoints 0, differ from WC1; processor reads base params",
             t.detail (5));
    }

    // TL31 → TL32 (PLAN-3.2 P120, RESEARCH-3.2 R126): the telemetry row lives in
    // engine_gates_32.h now — the 3.2 fields are live.
}
