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

#include "FactoryPresets.h"

//==============================================================================
// Stage 4 rows, part b (PLAN-4 P155–P157): presets, state round trip, layouts.
//
//   PR0    table: 64, 8 per prefix (8 categories), unique, <= 24 chars, no / \ :, no
//          output_gain, every value in range, a seed on every preset
//   PR1    apply (fresh processor, no disk): engineering read-back == table,
//          absent == default, output_gain -7 dB kept;  PR1n linear-fraction
//          conversion of a skewed param fails the read-back
//   PR2    20 s per preset on the standard stimulus: finite, <= the output
//          bound, RMS (2–20 s) > -60 dBFS; loudness spread <= 9 dB
//   PR3    use case: Harmoniser follow hits on scale degrees; Rhythmic tempo
//          lock at 120 / 93 bpm + grid onsets; Cascade min g_agc < 1 or loop
//          gain >= -3 dB; Ambient / Drone >= 2 s of tail > -60 dBFS;
//          Texture / Pitch FX / Glitch: PR2 only (no use-case criterion)
//   PR4    load A -> load B == fresh load of B (10 s, bitwise), 320 ordered
//          pairs: A = B + k (mod 64), k in {1, 8, 13, 32, 63} (1.1.0; all
//          4032 pairs would be ~17x the 1.0.0 cost for the same reset check)
//   PR5    the real path under a temp HOME: 64 files, loadPreset read-back,
//          save as user -> load == factory apply (10 s, bitwise)
//   ST1–3  state round trip on fresh instances (codec, two reopens, preset data)
//   ST2n   the codec without the custom-child strip shows the stale value
//   LY1    mono->mono / mono->stereo / stereo->stereo x 44.1 / 48 / 96 k
//   LY1n   rejected layouts refused
//==============================================================================
namespace { namespace eg4b
{
    using namespace eg;
    using eg25b::operator+;
    using eg22::sp;
    namespace F = ogran::factory;
    constexpr double kFs = 48000.0;
    inline void info (const juce::String& s) { std::printf ("  [INFO] %s\n", s.toRawUTF8()); }

    inline bool sameBits (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size() && std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0;
    }

    /** The standard preset stimulus: a 6-harmonic tone, notes A3 C4 E4 G3 (1 s
        each) x 4 = 16 s, then silence; 10 ms attack, 0.6 s decay, -6 dBFS peak. */
    inline float stdStim (juce::int64 n, int ch, double fs = kFs)
    {
        static const double midi[4] { 57.0, 60.0, 64.0, 55.0 };
        const double t = (double) n / fs;
        if (t >= 16.0) return 0.0f;
        const int k = (int) std::floor (t);
        const double tn = t - (double) k, f = 440.0 * std::exp2 ((midi[k % 4] - 69.0) / 12.0);
        double v = 0.0;
        for (int h = 1; h <= 6; ++h) v += std::sin (2.0 * eg::kPi * h * f * t) / (double) h;
        const double env = std::min (1.0, tn / 0.010) * std::exp (-tn / 0.6);
        return (float) (0.27 * env * v * (ch ? 0.98 : 1.0));
    }
    inline int stdNoteAt (double t) { static const int m[4] { 57, 60, 64, 55 }; return m[((int) std::floor (t)) % 4]; }

    struct HeadRender
    {
        std::vector<float> L, R;
    };
    /** Block loop with a playing MockPlayHead (prepared by the caller). */
    inline HeadRender renderHead (GranulatorProcessor& p, juce::int64 len, double bpm, int block,
                                  const std::function<float (juce::int64, int)>& stim,
                                  const std::function<void (GranulatorProcessor&, juce::int64)>& after = {})
    {
        MockPlayHead head;
        head.sampleRate = p.getSampleRate() > 0.0 ? p.getSampleRate() : kFs;
        head.bpm = bpm; head.playing = true;
        p.setPlayHead (&head);
        HeadRender r;
        r.L.assign ((size_t) len, 0.0f); r.R.assign ((size_t) len, 0.0f);
        juce::AudioBuffer<float> b (2, block);
        juce::MidiBuffer m;
        for (juce::int64 pos = 0; pos < len; pos += block)
        {
            const int n = (int) std::min<juce::int64> (block, len - pos);
            b.setSize (2, n, false, false, true);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i) b.setSample (c, i, stim (pos + i, c));
            head.timeInSamples = pos;
            p.processBlock (b, m);
            std::copy (b.getReadPointer (0), b.getReadPointer (0) + n, r.L.begin() + pos);
            std::copy (b.getReadPointer (1), b.getReadPointer (1) + n, r.R.begin() + pos);
            if (after) after (p, pos + n);
        }
        p.setPlayHead (nullptr);
        return r;
    }

    inline double rmsDb (const HeadRender& r, double t0, double t1, double fs = kFs)
    {
        double e = 0.0; size_t n = 0;
        for (size_t i = (size_t) (t0 * fs); i < (size_t) (t1 * fs) && i < r.L.size(); ++i, ++n)
            e += 0.5 * ((double) r.L[i] * r.L[i] + (double) r.R[i] * r.R[i]);
        return n ? 10.0 * std::log10 (std::max (1e-30, e / (double) n)) : -300.0;
    }

    inline std::vector<OuariconPresetManager::FactoryPresetDef> defsFor (GranulatorProcessor& p) { return F::makeFactoryPresets (p.parameters); }

    /** Fresh processor with def applied through the manager (no disk). */
    inline std::unique_ptr<GranulatorProcessor> withPreset (const OuariconPresetManager::FactoryPresetDef& d)
    {
        auto p = make();
        p->getPresetManager().applyPresetData (F::presetJsonOf (d));
        return p;
    }

    /** Read-back check of one spec against a processor. Returns the bad count. */
    inline int readBack (GranulatorProcessor& p, const F::FactoryPresetSpec& s, juce::String& why, float outGainDb)
    {
        int bad = 0;
        std::map<juce::String, float> want;
        for (const auto& [id, v] : s.values) want[id] = v;
        for (auto* ap : p.getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap);
            if (rp == nullptr) continue;
            const auto id = rp->getParameterID();
            const float eng = rp->convertFrom0to1 (rp->getValue());
            bool ok;
            if (id == "output_gain")
                ok = std::abs (eng - outGainDb) <= 1.0e-3f;
            else if (auto it = want.find (id); it != want.end())
            {
                const auto& r = rp->getNormalisableRange();
                const float tol = std::max (r.interval > 0.0f ? 0.5f * r.interval : 0.0f, 1.0e-4f * (r.end - r.start));
                ok = std::abs (eng - it->second) <= tol;
            }
            else   // decoded (raw) values; the manager's reset lands a skewed default ~2e-7 off the declared one
            {
                const float def = rp->convertFrom0to1 (rp->getDefaultValue());
                ok = std::abs (eng - def) <= 1.0e-6f * std::max (1.0f, std::abs (def));
            }
            if (! ok) { ++bad; if (why.isEmpty()) why = juce::String::fromUTF8 (s.name) + " " + id + juce::String::formatted (" = %g", (double) eng); }
        }
        return bad;
    }

    inline double ppqOf (int idx)   // tempo-division index → quarter notes
    {
        if (idx >= 18) return 4.0 * std::exp2 ((double) (idx - 18));
        const double base = std::exp2 ((double) (idx / 3)) / 16.0;
        const int v = idx % 3;
        return base * (v == 0 ? 2.0 / 3.0 : v == 1 ? 1.0 : 1.5);
    }
    inline float specValue (const F::FactoryPresetSpec& s, const char* id, float def)
    {
        for (const auto& kv : s.values) if (juce::String (kv.first) == id) return kv.second;
        return def;
    }

    //==========================================================================
    // PR — factory presets (P153–P155)
    //==========================================================================
    void prRows()
    {
        std::printf ("\n-- PR. FUNC-09 factory presets: 64 x 8 categories, engineering-unit table, fixed seeds (D42, 1.1.0)\n");
        const auto specs = F::factoryPresetSpecs();
        const int N = (int) specs.size();
        auto probe = make();
        const auto defs = defsFor (*probe);

        // PR0
        {
            const char* cats[8] { "Ambient \xc2\xb7 ", "Harmoniser \xc2\xb7 ", "Rhythmic \xc2\xb7 ", "Cascade \xc2\xb7 ",
                                  "Texture \xc2\xb7 ", "Pitch FX \xc2\xb7 ", "Glitch \xc2\xb7 ", "Drone \xc2\xb7 " };
            int perCat[8] {}; juce::StringArray names; juce::String why; int maxLen = 0; juce::int64 seeds[128] {}; int nSeeds = 0;
            for (const auto& s : specs)
            {
                const auto n = juce::String::fromUTF8 (s.name);
                for (int c = 0; c < 8; ++c) if (n.startsWith (juce::String::fromUTF8 (cats[c]))) ++perCat[c];
                if (names.contains (n) && why.isEmpty()) why = "duplicate " + n;
                names.add (n);
                maxLen = std::max (maxLen, n.length());
                if (n.length() > 24 && why.isEmpty()) why = n + " > 24";
                if (n.containsAnyOf ("/\\:") && why.isEmpty()) why = n + " path char";
                const auto v = F::validateSpec (s, probe->parameters);
                if (v.isNotEmpty() && why.isEmpty()) why = v;
                if (s.seed == 0 && why.isEmpty()) why = n + " seed 0";
                for (int k = 0; k < nSeeds; ++k) if (seeds[k] == s.seed && why.isEmpty()) why = n + " duplicate seed";
                if (nSeeds < 128) seeds[nSeeds++] = s.seed;
            }
            bool cats8 = true;
            for (int c = 0; c < 8; ++c) cats8 = cats8 && perCat[c] == 8;
            const bool ok = N == 64 && cats8 && why.isEmpty() && (int) defs.size() == 64;
            row (ok, "PR0 table: 64, 8 per category prefix (8), unique names + seeds, <= 24 chars, no / \\ :, no output_gain, in range, seeded",
                 juce::String::formatted ("%d presets, %d/%d/%d/%d/%d/%d/%d/%d, longest %d chars", N, perCat[0], perCat[1], perCat[2], perCat[3],
                                          perCat[4], perCat[5], perCat[6], perCat[7], maxLen)
                     + (why.isEmpty() ? juce::String() : " BAD " + why));
        }

        // PR1 / PR1n
        {
            int bad = 0; juce::String why;
            for (size_t i = 0; i < specs.size(); ++i)
            {
                auto p = make();
                eg::setP (*p, "output_gain", -7.0f);
                p->getPresetManager().applyPresetData (F::presetJsonOf (defs[i]));
                bad += readBack (*p, specs[i], why, -7.0f);
            }
            row (bad == 0, "PR1 apply: engineering read-back == table (interval / snap tol), absent == default (decoded, 1e-6 rel), output_gain -7 dB kept",
                 bad == 0 ? juce::String (N) + "/" + juce::String (N) + ", 220 params each" : juce::String (bad) + " bad, e.g. " + why);
            // PR1n: s1_size (skewed) written as a linear fraction.
            auto d = defs[0];
            auto* rp = probe->parameters.getParameter ("s1_size");
            const auto& r = rp->getNormalisableRange();
            const float v = specValue (specs[0], "s1_size", 100.0f);
            d.parameters["s1_size"] = (v - r.start) / (r.end - r.start);
            auto p = make();
            p->getPresetManager().applyPresetData (F::presetJsonOf (d));
            juce::String w;
            const int nb = readBack (*p, specs[0], w, 0.0f);
            row (nb > 0, "PR1n control: s1_size as a linear fraction fails the read-back", w);
        }

        // PR2 / PR3 — one 20 s render per preset at 120 bpm (+ 93 bpm for Rhythmic).
        struct Res { double rms = -300.0, peak = 0.0, peakIn = 0.0; bool finite = true; float gmin = 1.0f; double lgMax = -300.0;
                     long long hits = 0, lookups = 0, offScale = 0, rateBad = 0, gridBad = 0, gridN = 0; bool lock120 = false, lock93 = false;
                     double tailS = 0.0; };
        std::vector<Res> rs (specs.size());
        {
            std::vector<std::thread> th;
            const size_t hw = std::max<size_t> (1, std::thread::hardware_concurrency());
            std::atomic<size_t> next { 0 };
            for (size_t w = 0; w < hw; ++w)
                th.emplace_back ([&]
                {
                    for (size_t i = next++; i < specs.size(); i = next++)
                    {
                        auto& r = rs[i];
                        const auto& s = specs[i];
                        const juce::String name = juce::String::fromUTF8 (s.name);
                        auto p = withPreset (defs[i]);
                        p->prepareToPlay (kFs, 512);
                        auto& e = p->getEngineForTest();
                        const bool harm = name.startsWith ("Harmoniser"), rhy = name.startsWith ("Rhythmic");
                        const int root = (int) specValue (s, "scale_root", 0.0f);
                        const juce::int64 len = (juce::int64) (20.0 * kFs);
                        std::uint64_t seen = 0;
                        auto after = [&] (GranulatorProcessor& pp, juce::int64)
                        {
                            const auto& en = pp.getEngineForTest();
                            r.gmin = std::min (r.gmin, en.loopAgc());
                            if (en.loopK() > 0.0f) r.lgMax = std::max (r.lgMax, 20.0 * std::log10 ((double) en.loopK() * (double) en.loopAgc()));
                            const auto& d = en.diag;
                            const std::uint64_t first = d.logCount > (std::uint64_t) ogran::Engine::kSpawnLogSize ? d.logCount - (std::uint64_t) ogran::Engine::kSpawnLogSize : 0;
                            for (std::uint64_t k = std::max (seen, first); k < d.logCount; ++k)
                            {
                                const auto& rec = d.record (k);
                                if (harm && rec.followHit)
                                {
                                    const int st = rec.stream + 1;
                                    const int choice = (int) specValue (s, (juce::String ("s") + juce::String (st) + "_scale").toRawUTF8(), 0.0f);
                                    const auto* tab = pp.getCollectionTable (choice);
                                    const uint64_t mask = pp.getDegreeMask (st, choice);
                                    bool on = false;
                                    if (tab != nullptr)
                                    {
                                        const double R = ogran::kAbsRefCents + 100.0 * root;
                                        double q = std::fmod (100.0 * rec.target - R, (double) tab->period);
                                        if (q < 0.0) q += tab->period;
                                        for (int dd = 0; dd < tab->count; ++dd)
                                            if (((mask >> dd) & 1u) != 0 && (std::abs (q - tab->cents[(size_t) dd]) < 0.5
                                                                             || std::abs (q - tab->cents[(size_t) dd] - tab->period) < 0.5))
                                                on = true;
                                    }
                                    if (! on) ++r.offScale;
                                    // the grain's rate realises target − f0 (pitch offset folded into the target)
                                    if (std::abs (12.0 * std::log2 (rec.rate) - (rec.target - (double) rec.f0)) > 0.01) ++r.rateBad;
                                }
                                if (rhy && rec.stream == 0 && specValue (s, "s1_jitter", 20.0f) == 0.0f && specValue (s, "s1_onset_trigger", 0.0f) == 0.0f)
                                {
                                    const double q = ppqOf ((int) specValue (s, "s1_density_sync", 7.0f));
                                    const double ppq = en.getTracker().ppqAt (rec.onsetSample);
                                    const double fr = ppq / q - std::round (ppq / q);
                                    const double errSmp = std::abs (fr) * q / en.getTracker().anchor().pps;
                                    ++r.gridN;
                                    if (errSmp > 1.0) ++r.gridBad;
                                }
                            }
                            seen = d.logCount;
                        };
                        const auto out = renderHead (*p, len, 120.0, 512, [] (juce::int64 n, int c) { return stdStim (n, c); }, after);
                        const auto& tr = e.getTracker();
                        r.lock120 = tr.isPlaying() && std::abs (tr.anchor().pps * 60.0 * kFs - 120.0) < 1e-6;
                        for (size_t k = 0; k < out.L.size(); ++k)
                        {
                            if (! std::isfinite (out.L[k]) || ! std::isfinite (out.R[k])) r.finite = false;
                            r.peak = std::max ({ r.peak, (double) std::abs (out.L[k]), (double) std::abs (out.R[k]) });
                            r.peakIn = std::max ({ r.peakIn, (double) std::abs (stdStim ((juce::int64) k, 0)), (double) std::abs (stdStim ((juce::int64) k, 1)) });
                        }
                        r.rms = rmsDb (out, 2.0, 20.0);
                        r.hits = e.diag.followHits; r.lookups = e.diag.followLookups;
                        // tail: 50 ms frames after the input stops (16 s) above -60 dBFS
                        for (double t = 16.0; t + 0.05 <= 20.0; t += 0.05)
                        {
                            if (rmsDb (out, t, t + 0.05) <= -60.0) break;
                            r.tailS = t + 0.05 - 16.0;
                        }
                        if (rhy)
                        {
                            auto q = withPreset (defs[i]);
                            q->prepareToPlay (kFs, 512);
                            renderHead (*q, (juce::int64) (4.0 * kFs), 93.0, 512, [] (juce::int64 n, int c) { return stdStim (n, c); });
                            const auto& t9 = q->getEngineForTest().getTracker();
                            r.lock93 = t9.isPlaying() && std::abs (t9.anchor().pps * 60.0 * kFs - 93.0) < 1e-6;
                        }
                    }
                });
            for (auto& t : th) t.join();
        }
        {
            const double A = (double) ogran::kKneeA;
            bool ok = true; double lo = 1e9, hi = -1e9; juce::String tab;
            for (size_t i = 0; i < specs.size(); ++i)
            {
                const auto& r = rs[i];
                const double bound = std::sqrt (r.peakIn * r.peakIn + A * A) * (1.0 + 1e-6);
                const bool good = r.finite && r.peak <= bound && r.rms > -60.0;
                ok = ok && good;
                lo = std::min (lo, r.rms); hi = std::max (hi, r.rms);
                tab << juce::String::fromUTF8 (specs[i].name).fromFirstOccurrenceOf (" \xc2\xb7 ", false, false).removeCharacters (" ")
                    << juce::String::formatted (" %.1f;", r.rms);
            }
            row (ok, "PR2 20 s per preset (standard stimulus, 120 bpm): finite, peak <= sqrt (in^2 + A^2), RMS 2-20 s > -60 dBFS",
                 juce::String::formatted ("RMS %.1f .. %.1f dBFS", lo, hi));
            info ("PR2 RMS (dBFS, 2-20 s): " + tab);
            row (hi - lo <= 9.0, "PR2 loudness spread across the 64 <= 9 dB", juce::String::formatted ("%.2f dB", hi - lo));
        }
        {
            Tally t; juce::String det;
            for (size_t i = 0; i < specs.size(); ++i)
            {
                const auto& r = rs[i];
                const juce::String n = juce::String::fromUTF8 (specs[i].name);
                const juce::String sh = n.fromFirstOccurrenceOf (" \xc2\xb7 ", false, false);
                if (n.startsWith ("Harmoniser"))
                {
                    t.check (r.hits > 50 && r.offScale == 0 && r.rateBad == 0,
                             sh + juce::String::formatted (": hits %lld/%lld off-scale %lld rate %lld", r.hits, r.lookups, r.offScale, r.rateBad));
                    det << sh << juce::String::formatted (" hits %lld off %lld; ", r.hits, r.offScale);
                }
                else if (n.startsWith ("Rhythmic"))
                {
                    t.check (r.lock120 && r.lock93 && r.gridBad == 0, sh + juce::String::formatted (": lock %d/%d grid bad %lld/%lld", (int) r.lock120, (int) r.lock93, r.gridBad, r.gridN));
                    det << sh << juce::String::formatted (" lock %d/%d grid %lld/%lld; ", (int) r.lock120, (int) r.lock93, r.gridN - r.gridBad, r.gridN);
                }
                else if (n.startsWith ("Cascade"))
                {
                    t.check (r.gmin < 1.0f || r.lgMax >= -3.0, sh + juce::String::formatted (": g %.3f LG %.2f", (double) r.gmin, r.lgMax));
                    det << sh << juce::String::formatted (" g %.3f LG %.2f dB; ", (double) r.gmin, r.lgMax);
                }
                else if (n.startsWith ("Ambient") || n.startsWith ("Drone"))
                {
                    t.check (r.tailS >= 2.0, sh + juce::String::formatted (": tail %.2f s", r.tailS));
                    det << sh << juce::String::formatted (" tail %.2f s; ", r.tailS);
                }
                // Texture / Pitch FX / Glitch: PR2 (finite, bounded, audible, in the spread) only.
            }
            row (t.bad == 0, "PR3 use case: Harmoniser on-scale follow hits; Rhythmic lock 120/93 + grid; Cascade g < 1 or LG >= -3 dB; Ambient / Drone tail >= 2 s",
                 t.bad ? t.detail (16) : det.trim());
        }

        // PR4 — load A -> load B == fresh B (10 s, bitwise), 320 ordered pairs A = B + k.
        {
            const juce::int64 len = (juce::int64) (10.0 * kFs);
            auto job = [&] (int a, int b)
            {
                Job j; j.len = len; j.block = 512;
                j.stim = [] (juce::int64 n, int c) { return stdStim (n, c); };
                j.pre = [&defs, a, b] (Proc& p)
                {
                    if (a >= 0) p.getPresetManager().applyPresetData (F::presetJsonOf (defs[(size_t) a]));
                    p.getPresetManager().applyPresetData (F::presetJsonOf (defs[(size_t) b]));
                };
                return j;
            };
            std::vector<Job> fresh; std::vector<double> c1;
            for (int b = 0; b < N; ++b) { fresh.push_back (job (-1, b)); c1.push_back (1.0); }
            std::vector<std::vector<float>> fL ((size_t) N), fR ((size_t) N);
            eg22b::renderPool (fresh, c1, [&] (size_t i, Out& o) { fL[i] = std::move (o.L); fR[i] = std::move (o.R); });
            std::vector<Job> pairs; std::vector<std::pair<int, int>> ab; std::vector<double> c2;
            for (int k : { 1, 8, 13, 32, 63 })
                for (int b = 0; b < N; ++b)
                {
                    const int a = (b + k) % N;
                    pairs.push_back (job (a, b)); ab.push_back ({ a, b }); c2.push_back (1.0);
                }
            const int nPairs = (int) pairs.size();
            int bad = 0; juce::String why;
            eg22b::renderPool (pairs, c2, [&] (size_t i, Out& o)
            {
                const int b = ab[i].second;
                if (! sameBits (o.L, fL[(size_t) b]) || ! sameBits (o.R, fR[(size_t) b]))
                {
                    ++bad;
                    if (why.isEmpty()) why = juce::String (ab[i].first) + " -> " + juce::String (b);
                }
            });
            // non-vacuity: the 16 fresh renders are pairwise different
            int same = 0;
            for (int a = 0; a < N; ++a) for (int b = a + 1; b < N; ++b) same += sameBits (fL[(size_t) a], fL[(size_t) b]) ? 1 : 0;
            row (bad == 0 && same == 0 && nPairs == 320, "PR4 load A -> load B == fresh load of B, 10 s bitwise, 320 ordered pairs A = B + {1,8,13,32,63} (64 fresh renders distinct)",
                 juce::String::formatted ("%d/%d identical, %d equal fresh pairs", nPairs - bad, nPairs, same) + (why.isEmpty() ? juce::String() : " first diff " + why));
        }

        // PR5 — the real path, under the harness's temp HOME (main() sets it).
        {
            const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
            const bool tempHome = home.getFullPathName().contains ("ogran-harness-home");
            if (! tempHome)
            {
                row (false, "PR5 real path: HOME is not the harness temp home -- refusing to touch the real preset root", home.getFullPathName());
            }
            else
            {
                auto pm0 = make();
                auto factory = pm0->getPresetManager().getFactoryPresetsDirectory();
                pm0.reset();
                factory.deleteRecursively();
                auto p = make();   // the constructor writes the bank
                int files = 0; juce::StringArray missing;
                for (const auto& s : specs)
                {
                    const auto n = juce::String::fromUTF8 (s.name);
                    if (factory.getChildFile (n + ".json").existsAsFile()) ++files; else missing.add (n);
                }
                const int onDisk = factory.getNumberOfChildFiles (juce::File::findFiles, "*.json");
                int bad = 0; juce::String why;
                for (const auto& s : specs)
                {
                    auto q = make();
                    eg::setP (*q, "output_gain", -7.0f);
                    q->getPresetManager().loadPreset (juce::String::fromUTF8 (s.name));
                    bad += readBack (*q, s, why, -7.0f);
                }
                // save as user -> load == factory apply (10 s render, bitwise)
                size_t rust = 0;   // Cascade · Rust Spiral (fb 90 %, sat 60 %)
                for (size_t k = 0; k < specs.size(); ++k)
                    if (juce::String::fromUTF8 (specs[k].name) == juce::String::fromUTF8 ("Cascade \xc2\xb7 Rust Spiral")) rust = k;
                const auto& s0 = specs[rust];
                auto a = make();
                a->getPresetManager().applyPresetData (F::presetJsonOf (defs[rust]));
                a->getPresetManager().savePreset ("PR5 user copy");
                auto b = make();
                const bool loaded = b->getPresetManager().loadPreset ("PR5 user copy");
                auto c = withPreset (defs[rust]);
                auto rend = [] (GranulatorProcessor& g)
                {
                    g.prepareToPlay (kFs, 512);
                    return renderHead (g, (juce::int64) (10.0 * kFs), 120.0, 512, [] (juce::int64 n, int ch) { return stdStim (n, ch); });
                };
                const auto rb = rend (*b), rc = rend (*c);
                const bool same = loaded && sameBits (rb.L, rc.L) && sameBits (rb.R, rc.R);
                b->getPresetManager().deletePreset ("PR5 user copy");
                const bool sentinel = factory.getChildFile (".factory-version").existsAsFile();
                row (files == N && onDisk == N && sentinel && bad == 0 && same,
                     "PR5 real path (temp HOME): fresh bank 64 files + sentinel; loadPreset read-back == table; user copy == factory apply (10 s bitwise)",
                     juce::String::formatted ("%d/%d named files (%d .json), sentinel %d, read-back bad %d, user copy %s (", files, N, onDisk, (int) sentinel, bad,
                                              same ? "bitwise" : "DIFFERS") + juce::String::fromUTF8 (s0.name) + ")"
                         + (missing.isEmpty() ? juce::String() : " missing " + missing.joinIntoString (", ")) + (why.isEmpty() ? juce::String() : " " + why));
                info ("PR5 factory dir " + factory.getFullPathName() + "; names keep U+00B7 on disk (sanitiser maps only / \\ :)");
            }
        }
    }

    //==========================================================================
    // ST — COMPAT-02 state round trip (P156)
    //==========================================================================
    /** A full non-default parameter set: Float random (snapped), Choice random or
        last index, Bool random or inverted; all 16 mod slots routed. */
    inline void randomise (GranulatorProcessor& p, juce::Random& rng, bool lastChoices)
    {
        for (auto* ap : p.getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap);
            if (rp == nullptr) continue;
            const auto id = rp->getParameterID();
            if (id.startsWith ("mod")) continue;
            const auto& r = rp->getNormalisableRange();
            float v;
            if (dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr)
                v = lastChoices ? (rp->getDefaultValue() >= 0.5f ? 0.0f : 1.0f) : (rng.nextBool() ? 1.0f : 0.0f);
            else if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (rp))
                v = lastChoices ? (float) (c->choices.size() - 1) : (float) rng.nextInt (c->choices.size());
            else
                v = r.snapToLegalValue (r.convertFrom0to1 (rng.nextFloat()));
            if (id == "freeze") v = 0.0f;   // a frozen head makes the 10 s render input-independent; kept off
            eg::setP (p, id, v);
        }
        for (int m = 1; m <= 16; ++m)
        {
            eg::setP (p, "mod" + juce::String (m) + "_source", (float) (1 + rng.nextInt (11)));
            eg::setP (p, "mod" + juce::String (m) + "_dest", (float) (1 + rng.nextInt (GranulatorParamIds::kModDests - 1)));
            float d = (float) (rng.nextInt (199) - 99);
            if (d == 0.0f) d = 37.0f;
            eg::setP (p, "mod" + juce::String (m) + "_depth", d);
        }
    }
    inline double gMaxRel = 0.0;   // worst decoded relative difference seen by sameParams (INFO)
    /** Decoded (raw) values within 1e-6 relative: the XML / JSON float text keeps
        ~7 significant digits, so a skewed value can come back an ulp or two away
        (the 10 s bitwise render is the strict arm). */
    inline int sameParams (juce::AudioProcessor& a, juce::AudioProcessor& b, juce::String& why, const juce::String& skip = {})
    {
        int bad = 0;
        const auto& pa = a.getParameters(); const auto& pb = b.getParameters();
        for (int i = 0; i < pa.size(); ++i)
        {
            auto* ra = dynamic_cast<juce::RangedAudioParameter*> (pa[i]);
            auto* rb = dynamic_cast<juce::RangedAudioParameter*> (pb[i]);
            if (ra == nullptr || rb == nullptr || ra->getParameterID() == skip) continue;
            // decoded (raw) values — what the engine reads; the normalised value of a
            // skewed parameter can re-normalise an ulp away after XML (same raw value)
            const float va = ra->convertFrom0to1 (ra->getValue()), vb = rb->convertFrom0to1 (rb->getValue());
            const double rel = std::abs ((double) va - (double) vb) / std::max (1.0, std::abs ((double) va));
            gMaxRel = std::max (gMaxRel, rel);
            if (rel > 1.0e-6) { ++bad; if (why.isEmpty()) why = ra->getParameterID() + juce::String::formatted (" %.9g vs %.9g", (double) va, (double) vb); }
        }
        return bad;
    }
    inline void setNonParam (GranulatorProcessor& p, uint64_t seed)
    {
        p.setSeed (seed);
        p.uiLanguage = 2;
        juce::String bad;
        p.setCustomScale ("Nineteen", "edo 19", bad);
        p.setDegreeMask (1, 31, 0x24949ull);
        p.setDegreeMask (3, 2, 0x55ull);
    }
    inline HeadRender render10 (GranulatorProcessor& p)
    {
        p.prepareToPlay (kFs, 512);
        return renderHead (p, (juce::int64) (10.0 * kFs), 120.0, 512, [] (juce::int64 n, int c) { return stdStim (n, c); });
    }

    void stRows()
    {
        std::printf ("\n-- ST. COMPAT-02 state round trip on fresh instances (codec, two reopens, preset data)\n");
        const uint64_t seeds[3] { (1ull << 53) + 1ull, 0xF00DFACECAFEBEEFull, 0x0123456789ABCDEFull };
        // ST1 / ST3
        {
            Tally t1, t3;
            for (int k = 0; k < 3; ++k)
            {
                juce::Random rng ((juce::int64) (0x57A7E0 + k));
                auto a = make();
                randomise (*a, rng, k == 1);
                setNonParam (*a, seeds[k]);
                const auto blob = stateOf (*a);
                auto b = make();
                load (*b, blob);
                juce::String why;
                const int pb = sameParams (*a, *b, why);
                const bool np = sameNonParamState (*a, *b, why) && b->getSeed() == seeds[k] && b->uiLanguage.load() == 2;
                const auto ra = render10 (*a), rb = render10 (*b);
                const bool same = sameBits (ra.L, rb.L) && sameBits (ra.R, rb.R);
                t1.check (pb == 0 && np && same, juce::String::formatted ("set %d: %d params differ, nonparam %d, render %s ", k, pb, (int) np, same ? "==" : "differs") + why);

                // ST3: preset data (output_gain excluded by design)
                auto c = make();
                const float og = c->parameters.getParameter ("output_gain")->getValue();
                c->getPresetManager().applyPresetData (a->getPresetManager().capturePresetData());
                c->uiLanguage = a->uiLanguage.load();   // the UI language is session state, not preset content
                juce::String w3;
                const int pc = sameParams (*a, *c, w3, "output_gain");
                const bool ogKept = juce::exactlyEqual (c->parameters.getParameter ("output_gain")->getValue(), og);
                const bool np3 = sameNonParamState (*a, *c, w3) && c->getSeed() == seeds[k];
                t3.check (pc == 0 && ogKept && np3, juce::String::formatted ("set %d: %d params differ, og kept %d, nonparam %d ", k, pc, (int) ogKept, (int) np3) + w3);
            }
            row (t1.bad == 0, "ST1 3 full random sets (16 routes, seed 2^53+1 / > INT64_MAX, edo 19, masks, zh-Hans) -> fresh instance: decoded params (1e-6 rel), state, 10 s render bitwise",
                 t1.detail (3));
            info (juce::String::formatted ("ST1 / ST3 worst decoded relative difference %.3g (float text precision)", gMaxRel));
            row (t3.bad == 0, "ST3 the same sets through capturePresetData -> applyPresetData (fresh): decoded params (1e-6 rel, excl. output_gain), seed / scale / masks; output_gain kept",
                 t3.detail (3));
        }
        // ST2 / ST2n — two reopens with a change between.
        auto twoReopen = [] (bool noStrip, juce::String& got)
        {
            auto a = make();
            setNonParam (*a, 0x1111ull);
            auto b = make();
            b->testNoStateStrip = noStrip;
            load (*b, stateOf (*a));
            juce::String bad;
            b->setCustomScale ("Changed", "edo 22", bad);
            b->setSeed (0x2222ull);
            const auto blob = stateOf (*b);
            auto c = make();
            c->testNoStateStrip = noStrip;
            load (*c, blob);
            got = c->getCustomScale().text + juce::String::formatted (" seed %llx", (unsigned long long) c->getSeed());
            return c->getCustomScale().text == "edo 22" && c->getSeed() == 0x2222ull && c->getCustomTable().count == 22;
        };
        juce::String g2, g2n;
        const bool ok2 = twoReopen (false, g2), ok2n = twoReopen (true, g2n);
        row (ok2, "ST2 save -> B -> change scale + seed -> save -> C: C holds the change", g2);
        row (! ok2n, "ST2n control: without the custom-child strip, C reads the stale child (fails ST2)", g2n);
    }

    //==========================================================================
    // LY — COMPAT-03 layouts (P157)
    //==========================================================================
    void lyRows()
    {
        std::printf ("\n-- LY. COMPAT-03 I/O layouts\n");
        using CS = juce::AudioChannelSet;
        struct L { const char* name; int in, out; };
        const L ls[3] { { "mono->mono", 1, 1 }, { "mono->stereo", 1, 2 }, { "stereo->stereo", 2, 2 } };
        Tally t; juce::String det;
        for (double fs : { 44100.0, 48000.0, 96000.0 })
            for (const auto& l : ls)
            {
                auto p = make();
                juce::AudioProcessor::BusesLayout lay;
                lay.inputBuses.add (l.in == 1 ? CS::mono() : CS::stereo());
                lay.outputBuses.add (l.out == 1 ? CS::mono() : CS::stereo());
                const bool acc = p->setBusesLayout (lay);
                eg::setAll (*p, P { { "s1_stereo_spread", 80.0f }, { "feedback", 40.0f }, { "reverb_mix", 20.0f } });
                p->setSeed (0x1A7u);
                p->prepareToPlay (fs, 512);
                const int nch = std::max (l.in, l.out);
                juce::AudioBuffer<float> b (nch, 512);
                juce::MidiBuffer m;
                const juce::int64 len = (juce::int64) (10.0 * fs);
                double e[2] {}, diff = 0.0; bool fin = true;
                for (juce::int64 pos = 0; pos < len; pos += 512)
                {
                    for (int c = 0; c < nch; ++c)
                        for (int i = 0; i < 512; ++i) b.setSample (c, i, c < l.in ? stdStim (pos + i, c, fs) : 0.0f);
                    p->processBlock (b, m);
                    for (int i = 0; i < 512; ++i)
                    {
                        const float x0 = b.getSample (0, i), x1 = l.out > 1 ? b.getSample (1, i) : x0;
                        if (! std::isfinite (x0) || ! std::isfinite (x1)) fin = false;
                        e[0] += (double) x0 * x0; e[1] += (double) x1 * x1; diff += std::abs ((double) x0 - x1);
                    }
                }
                const auto& d = p->getEngineForTest().diag;
                const long long scr = d.fbScrubs + d.hostScrubs + d.wetScrubs + d.envScrubs + d.analysisScrubs;
                const double db0 = 10.0 * std::log10 (std::max (1e-30, e[0] / (double) len)), db1 = 10.0 * std::log10 (std::max (1e-30, e[1] / (double) len));
                bool ok = acc && fin && scr == 0 && db0 > -60.0;
                if (l.out == 2) ok = ok && db1 > -60.0 && diff > 0.0;
                t.check (ok, juce::String::formatted ("%g k %s: accepted %d finite %d scrubs %lld L %.1f R %.1f dBFS |L-R| %.3g", fs / 1000.0, l.name, (int) acc, (int) fin, scr, db0, db1, diff));
                if (fs == 48000.0) det << l.name << juce::String::formatted (" %.1f/%.1f dB; ", db0, db1);
            }
        row (t.bad == 0, "LY1 mono->mono / mono->stereo / stereo->stereo x 44.1 / 48 / 96 k, 10 s: finite, non-silent, L != R (spread), 0 scrubs",
             t.bad ? t.detail (9) : det.trim());
        {
            auto p = make();
            auto sup = [&p] (const CS& in, const CS& out)
            {
                juce::AudioProcessor::BusesLayout l;
                l.inputBuses.add (in); l.outputBuses.add (out);
                return p->checkBusesLayoutSupported (l);
            };
            const bool s2m = sup (CS::stereo(), CS::mono()), s51 = sup (CS::create5point1(), CS::create5point1()),
                       dis = sup (CS::disabled(), CS::stereo());
            row (! s2m && ! s51 && ! dis, "LY1n refused: stereo->mono, 5.1->5.1, disabled input",
                 juce::String::formatted ("stereo->mono %d, 5.1 %d, disabled-in %d", (int) s2m, (int) s51, (int) dis));
        }
    }
} }

static void engineRows4b()
{
    eg4b::prRows();
    eg4b::stRows();
    eg4b::lyRows();
}
