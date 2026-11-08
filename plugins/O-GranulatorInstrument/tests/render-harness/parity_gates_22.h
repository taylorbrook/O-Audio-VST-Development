#pragma once
// ============================================================================
//   O-GranulatorInstrument — DSP-01 / FUNC-09 / D16 parity gates, OUR side
//   (PLAN-2.2 T9 parity rows; T11 / T12 run them)
//
//   O-GranulatorInstrument-render-harness --parity <dir> [--rows <prefix>]
//
//   <dir> = tests/parity/run.sh's WORK/out (parent @ 82406fce renders).
//   Rows (ParityRows.h is shared with the parent driver):
//     PN01–PN26  sample null ≤ −90 dB after least-squares gain alignment over
//                n ≥ attack + 2 cells; equal grain counts; our p0 = closed form
//     PNc1–PNc3  controls: each must be > −90 dB
//     PS1–PS4    statistical arm (KS two-sample + analytic, moments, range,
//                Bernoulli, jitter displacement + even-k χ², scale χ²); ×0.5
//                controls harness-side, PS3 also engine-side (jitterScale 0.5)
//     SC1 / SC3  ActiveSet + custom-table parse bitwise vs the parent dumps
//     GV0        D16 measurement (prints the target); GV1 (only with --rows GV)
//                verifies the shipped kVoiceGainMeasuredDb
//   Included by main.cpp after engine_gates_21.h (uses eg21::Session).
// ============================================================================

#include "../parity/ParityRows.h"

#include <map>
#include <numeric>
#include <set>

namespace { namespace pg22
{
    using eg21::Session;
    using eg21::P;
    using eg21::f2;
    using eg21::ctrl;
    using eg21::dB;

    juce::File gDir;

    /** main.cpp's row() (shadowed by the loop variables named row below). */
    void prow (bool ok, const juce::String& n, const juce::String& d = {}) { row (ok, n, d); }

    juce::String sid (int s, const std::string& x) { return juce::String (Ids::stream (s, x.c_str())); }

    //==========================================================================
    // Parent artefacts
    std::vector<float> readF32 (const juce::String& name)
    {
        juce::MemoryBlock mb;
        if (! gDir.getChildFile (name).loadFileAsData (mb))
            return {};
        std::vector<float> v (mb.getSize() / sizeof (float));
        std::memcpy (v.data(), mb.getData(), v.size() * sizeof (float));
        return v;
    }

    struct PGrain
    {
        juce::int64 onsetSample = 0, writeHead = 0;
        int stream = 0, dir = 1, level = 0, windowStep = 0;
        double delay = 0, anchor = 0, frac = 0, rate = 0, len = 0;
        float gainL = 1, gainR = 1, tiltT = 0.5f, pitchCents = 0;
    };

    std::vector<PGrain> readGrains (const juce::String& row)
    {
        std::vector<PGrain> out;
        juce::StringArray lines;
        gDir.getChildFile (row + ".grains.tsv").readLines (lines);
        for (const auto& l : lines)
        {
            if (l.isEmpty() || l.startsWith ("#")) continue;
            juce::StringArray t;
            t.addTokens (l, "\t", "");
            if (t.size() < 15) continue;
            PGrain g;
            g.onsetSample = t[0].getLargeIntValue(); g.writeHead = t[1].getLargeIntValue();
            g.stream = t[2].getIntValue(); g.dir = t[3].getIntValue(); g.level = t[4].getIntValue();
            g.delay = std::strtod (t[5].toRawUTF8(), nullptr); g.anchor = std::strtod (t[6].toRawUTF8(), nullptr);
            g.frac = std::strtod (t[7].toRawUTF8(), nullptr); g.rate = std::strtod (t[8].toRawUTF8(), nullptr);
            g.len = std::strtod (t[9].toRawUTF8(), nullptr);
            g.gainL = std::strtof (t[10].toRawUTF8(), nullptr); g.gainR = std::strtof (t[11].toRawUTF8(), nullptr);
            g.windowStep = t[12].getIntValue(); g.tiltT = std::strtof (t[13].toRawUTF8(), nullptr);
            g.pitchCents = std::strtof (t[14].toRawUTF8(), nullptr);
            out.push_back (g);
        }
        return out;
    }

    struct Manifest { float sizeRaw = 0, densityRaw = 0; int landed = 0, total = 0, grains = 0; juce::int64 A = 0, delta = 0; };

    std::map<juce::String, Manifest> readManifest()
    {
        std::map<juce::String, Manifest> m;
        juce::StringArray lines;
        gDir.getChildFile ("manifest.tsv").readLines (lines);
        for (const auto& l : lines)
        {
            if (l.isEmpty() || l.startsWith ("#")) continue;
            juce::StringArray t;
            t.addTokens (l, "\t", "");
            if (t.size() < 14) continue;
            Manifest x;
            x.A = t[2].getLargeIntValue(); x.delta = t[3].getLargeIntValue();
            x.landed = t[7].getIntValue(); x.total = t[8].getIntValue();
            x.sizeRaw = std::strtof (t[9].toRawUTF8(), nullptr); x.densityRaw = std::strtof (t[10].toRawUTF8(), nullptr);
            x.grains = t[12].getIntValue();
            m[t[0]] = x;
        }
        return m;
    }

    //==========================================================================
    // Our side
    /** Every stream parameter our engine reads, for stream n, at the null geometry. */
    P streamParams (int n, const std::vector<parity::KV>& sp)
    {
        P p { { sid (n, "enable"), 1.0f }, { sid (n, "mute"), 0.0f }, { sid (n, "solo"), 0.0f },
              { sid (n, "scan_rate"), 0.0f }, { sid (n, "scan_mode"), 1.0f },
              { sid (n, "loop_start"), 0.0f }, { sid (n, "loop_end"), 100.0f },
              { sid (n, "root_note"), (float) parity::kOurRoot }, { sid (n, "root_fine"), 0.0f },
              { sid (n, "key_track"), 100.0f } };
        for (const auto& kv : sp)
            p.push_back ({ sid (n, kv.id), kv.v });
        return p;
    }

    P globalsNull()
    {
        P p { { "polyphony", 16.0f }, { "grain_limit", 3.0f }, { "pitch_bend_range", 2.0f }, { "output_gain", 0.0f },
              { "amp_attack", 0.0f }, { "amp_decay", 0.0f }, { "amp_sustain", 100.0f }, { "amp_release", 50.0f },
              { "vel_to_amp", 0.0f } };
        for (int s = 1; s <= 4; ++s)
            p.push_back ({ sid (s, "enable"), 0.0f });
        return p;
    }

    juce::AudioBuffer<float> materialBuf()
    {
        juce::AudioBuffer<float> b (2, (int) parity::kF);
        for (int c = 0; c < 2; ++c)
        {
            float* d = b.getWritePointer (c);
            for (int i = 0; i < (int) parity::kF; ++i)
                d[i] = parity::material (i, c);
        }
        return b;
    }

    struct Ours
    {
        std::vector<float> L, R;
        std::vector<ogi::GrainLogEntry> log;
        float sizeRaw = 0, densityRaw = 0;
    };

    Ours renderOurs (const parity::Row& row, const std::function<void (Session&)>& tweak = {})
    {
        P params = globalsNull();
        const bool stats = row.kind == parity::Kind::stats;
        const int s0 = stats ? 1 : row.stream, s1 = stats ? row.streams : row.stream;
        for (int s = s0; s <= s1; ++s)
            for (auto& kv : streamParams (s, parity::over (row.sp, row.oursOnly)))
                params.push_back (kv);
        Session ses (parity::kFs, params, row.seed, 512);
        ses.hk().coherenceOff = false;   // shipping law (the LS gain absorbs the law ratio, R3)
        if (tweak) tweak (ses);
        const auto mat = materialBuf();
        for (int s = s0; s <= s1; ++s)
            ses.load (s - 1, mat, parity::kFs, "mat");
        const juce::int64 ton = parity::kOurTon + row.ourOnsetShift;
        ses.noteOn (ton, parity::kOurRoot, 127);
        const juce::int64 len = (juce::int64) std::llround (row.seconds * parity::kFs);
        ses.run (parity::kOurTon + len, 512);
        Ours o;
        o.L.assign (ses.L.begin() + parity::kOurTon, ses.L.end());
        o.R.assign (ses.R.begin() + parity::kOurTon, ses.R.end());
        o.log = ses.hk().logInOrder();
        o.sizeRaw = eg21::rawOf (*ses.p, sid (s0, "size"));
        o.densityRaw = eg21::rawOf (*ses.p, sid (s0, "density"));
        return o;
    }

    /** Residual after least-squares gain alignment, from sample n0. */
    struct Null { double resDb = 0, gain = 0, refDb = -400; size_t n = 0; };

    Null nullOf (const std::vector<float>& par, const Ours& o, size_t n0)
    {
        Null r;
        const size_t n = std::min (par.size() / 2, o.L.size());
        double po = 0, oo = 0, pp = 0;
        for (size_t i = n0; i < n; ++i)
        {
            const double pl = par[2 * i], pr = par[2 * i + 1], ol = o.L[i], orr = o.R[i];
            po += pl * ol + pr * orr; oo += ol * ol + orr * orr; pp += pl * pl + pr * pr;
        }
        r.n = n > n0 ? n - n0 : 0;
        if (oo <= 0 || pp <= 0) { r.resDb = 0; return r; }
        r.gain = po / oo;
        double e = 0;
        for (size_t i = n0; i < n; ++i)
        {
            const double dl = par[2 * i] - r.gain * o.L[i], dr = par[2 * i + 1] - r.gain * o.R[i];
            e += dl * dl + dr * dr;
        }
        r.resDb = 10.0 * std::log10 (std::max (1.0e-300, e / pp));
        r.refDb = 10.0 * std::log10 (pp / (2.0 * (double) r.n));
        return r;
    }

    //==========================================================================
    // PN rows
    void runNulls (const juce::String& prefix)
    {
        const auto man = readManifest();
        const size_t n0 = 256;   // attack (2 ms min = 96) + 2 cells, rounded up
        std::map<std::string, parity::Row> byId;
        for (const auto& r : parity::rows()) byId[r.id] = r;

        for (const auto& row : parity::rows())
        {
            if (row.kind != parity::Kind::null_) continue;
            if (prefix.isNotEmpty() && ! juce::String (row.id).startsWith (prefix)) continue;
            const bool isCtrl = juce::String (row.id).startsWith ("PNc");
            const juce::String parId = row.parentOf.empty() ? juce::String (row.id) : juce::String (row.parentOf);
            const auto par = readF32 (parId + ".f32");
            const auto pg  = readGrains (parId);
            const auto mit = man.find (parId);
            if (par.empty() || mit == man.end())
            {
                prow (false, juce::String (row.id) + " parent render present", "missing " + parId + ".f32 / manifest");
                continue;
            }
            // Our side of a Δ+1 control is the plain row it shifts.
            parity::Row ourRow = row;
            if (row.deltaAdj != 0) ourRow.deltaAdj = 0;
            const auto o = renderOurs (ourRow);

            const auto g = parity::solve (ourRow, o.sizeRaw);
            const auto nl = nullOf (par, o, n0);
            const juce::int64 len = (juce::int64) std::llround (row.seconds * parity::kFs);
            int ourCount = 0, p0Bad = 0;
            for (const auto& e : o.log)
                if (e.onsetSample >= parity::kOurTon && e.onsetSample < parity::kOurTon + len)
                {
                    ++ourCount;
                    p0Bad += eg21::sameD (e.startPos, (double) g.p0) ? 0 : 1;
                }
            const bool rawSame = juce::exactlyEqual (o.sizeRaw, mit->second.sizeRaw) && juce::exactlyEqual (o.densityRaw, mit->second.densityRaw);
            const juce::String meas = "residual " + f2 (nl.resDb, 1) + " dB (gain " + f2 (nl.gain, 5) + ", ref " + f2 (nl.refDb, 1)
                                      + " dBFS, n " + juce::String ((juce::int64) nl.n) + ") | grains ours " + juce::String (ourCount)
                                      + " parent " + juce::String ((int) pg.size()) + " | p0 " + juce::String (g.p0) + " off on "
                                      + juce::String (p0Bad) + " | raw size/density " + (rawSame ? "same" : "DIFFER");
            if (! isCtrl)
            {
                const bool ok = nl.resDb <= -90.0 && ourCount == (int) pg.size() && p0Bad == 0 && rawSame && nl.refDb >= -70.0 && g.ok;
                prow (ok, juce::String (row.id) + " DSP-01 null vs parent @82406fce",
                           "meas " + meas + " | thr <= -90 dB | margin " + f2 (-90.0 - nl.resDb, 1) + " dB");
            }
            else
            {
                const bool fired = nl.resDb > -90.0;
                prow (fired, juce::String (row.id) + " DSP-01 null control (must exceed -90 dB)",
                           "meas " + meas + " | " + ctrl (fired, row.id == "PNc1" ? "our onset + 1 sample"
                                                                 : row.id == "PNc2" ? "our window step + 1" : "parent delta + 1 (mip phase)"));
            }
        }
    }

    //==========================================================================
    // Statistics
    double ks2 (std::vector<double> a, std::vector<double> b)
    {
        std::sort (a.begin(), a.end()); std::sort (b.begin(), b.end());
        size_t i = 0, j = 0; double d = 0;
        while (i < a.size() && j < b.size())
        {
            const double x = std::min (a[i], b[j]);
            while (i < a.size() && a[i] <= x) ++i;
            while (j < b.size() && b[j] <= x) ++j;
            d = std::max (d, std::abs ((double) i / a.size() - (double) j / b.size()));
        }
        return d;
    }

    double ks1 (std::vector<double> a, const std::function<double (double)>& cdf)
    {
        std::sort (a.begin(), a.end());
        double d = 0; const double n = (double) a.size();
        for (size_t i = 0; i < a.size(); ++i)
        {
            const double F = cdf (a[i]);
            d = std::max ({ d, std::abs ((double) (i + 1) / n - F), std::abs (F - (double) i / n) });
        }
        return d;
    }

    struct Mom { double mean = 0, var = 0, mn = 1e300, mx = -1e300; };
    Mom mom (const std::vector<double>& v)
    {
        Mom m;
        for (double x : v) { m.mean += x; m.mn = std::min (m.mn, x); m.mx = std::max (m.mx, x); }
        m.mean /= std::max<size_t> (1, v.size());
        for (double x : v) m.var += (x - m.mean) * (x - m.mean);
        m.var /= std::max<size_t> (2, v.size()) - 1;
        return m;
    }

    /** Uniform-law test of ours vs parent vs U(a, b). Returns pass; fills detail. */
    bool uniformTest (const std::vector<double>& o, const std::vector<double>& p, double a, double b, juce::String& det)
    {
        const double w = b - a, sig = w / std::sqrt (12.0);
        auto cdf = [a, w] (double x) { return std::clamp ((x - a) / w, 0.0, 1.0); };
        const double d2 = ks2 (o, p), d1o = ks1 (o, cdf), d1p = ks1 (p, cdf);
        const Mom mo = mom (o), mp = mom (p);
        const double dm = std::abs (mo.mean - mp.mean) / sig, vr = mo.var / std::max (1e-300, mp.var);
        const bool rangeOk = mo.mn >= a - 1e-6 && mo.mx <= b + 1e-6 && (mo.mx - mo.mn) >= 0.98 * w;
        const bool ok = o.size() >= 4000 && p.size() >= 4000 && d2 <= 0.060 && d1o <= 0.043 && d1p <= 0.043 && dm <= 0.134
                        && vr >= 0.887 && vr <= 1.127 && rangeOk;
        det = "n " + juce::String ((int) o.size()) + "/" + juce::String ((int) p.size()) + " D2 " + f2 (d2) + " D1 " + f2 (d1o) + "/" + f2 (d1p)
              + " dmean " + f2 (dm) + "s var " + f2 (vr) + " range " + f2 (mo.mn) + ".." + f2 (mo.mx) + (rangeOk ? "" : " (RANGE)");
        return ok;
    }

    std::vector<double> scaled (std::vector<double> v, double k, double about = 0.0) { for (auto& x : v) x = about + (x - about) * k; return v; }

    void statsRow (const juce::String& name, const std::vector<double>& o, const std::vector<double>& p, double a, double b, double about)
    {
        juce::String d, dc;
        const bool ok = uniformTest (o, p, a, b, d);
        const bool ctrlFail = ! uniformTest (scaled (o, 0.5, about), p, a, b, dc);
        prow (ok && ctrlFail, name, "meas " + d + " | thr D2 <= 0.060, D1 <= 0.043, dmean <= 0.134s, var [0.887, 1.127], range >= 0.98w | "
                                         + ctrl (ctrlFail, "x0.5 on ours -> " + dc));
    }

    void runStats (const juce::String& prefix)
    {
        const auto man = readManifest();
        for (const auto& row : parity::rows())
        {
            if (row.kind != parity::Kind::stats) continue;
            if (prefix.isNotEmpty() && ! juce::String (row.id).startsWith (prefix)) continue;
            const auto pg = readGrains (row.id);
            const auto mit = man.find (row.id);
            if (pg.empty() || mit == man.end()) { prow (false, juce::String (row.id) + " parent grains present"); continue; }
            const auto o = renderOurs (row);
            const double sizeS = (double) mit->second.sizeRaw * 0.001 * parity::kFs;
            const double posPar = parity::findV (row.sp, "position", 0.0f) / 100.0;
            std::vector<ogi::GrainLogEntry> ol;
            for (const auto& e : o.log) if (e.onsetSample < parity::kOurTon + (juce::int64) (row.seconds * parity::kFs)) ol.push_back (e);

            auto collect = [&] (auto fo, auto fp, std::vector<double>& vo, std::vector<double>& vp)
            {
                for (const auto& e : ol) vo.push_back (fo (e));
                for (const auto& g : pg) vp.push_back (fp (g));
            };
            const juce::String id (row.id);
            if (id == "PS1")
            {
                const double s = parity::findV (row.sp, "spray", 0) / 100.0, z = parity::findV (row.sp, "size_rand", 0) / 100.0;
                const double sg = parity::findV (row.sp, "stereo_spread", 0) / 100.0, rho = parity::findV (row.sp, "pitch_rand", 0);
                const double rp = parity::findV (row.sp, "reverse_prob", 0) / 100.0;
                std::vector<double> o1, p1, o2, p2, o3, p3, o4, p4;
                collect ([] (const ogi::GrainLogEntry& e) { return (e.startPos - e.centre) / e.regionW; },
                         [&] (const PGrain& g) { return (g.delay - (double) parity::margin (g.level) - posPar * (double) parity::kS) / (double) parity::kS; }, o1, p1);
                statsRow ("PS1a spray (s 30 %)", o1, p1, -s / 2, s / 2, 0.0);
                collect ([&] (const ogi::GrainLogEntry& e) { return e.len / sizeS - 1.0; }, [&] (const PGrain& g) { return g.len / sizeS - 1.0; }, o2, p2);
                statsRow ("PS1b size_rand (50 %)", o2, p2, -z / 2, z / 2, 0.0);
                auto px = [] (float gl, float gr) { return 4.0 * std::atan2 ((double) gr, (double) gl) / eg21::kPi - 1.0; };
                collect ([&] (const ogi::GrainLogEntry& e) { return px (e.gainL, e.gainR); }, [&] (const PGrain& g) { return px (g.gainL, g.gainR); }, o3, p3);
                statsRow ("PS1c stereo spread (70 %)", o3, p3, -sg, sg, 0.0);
                collect ([] (const ogi::GrainLogEntry& e) { return (double) e.pitchCents / 100.0; }, [] (const PGrain& g) { return (double) g.pitchCents / 100.0; }, o4, p4);
                statsRow ("PS1d pitch_rand Off (7 st)", o4, p4, -rho, rho, 0.0);
                double ro = 0, rpp = 0;
                for (const auto& e : ol) ro += e.dir < 0 ? 1 : 0;
                for (const auto& g : pg) rpp += g.dir < 0 ? 1 : 0;
                ro /= std::max<size_t> (1, ol.size()); rpp /= std::max<size_t> (1, pg.size());
                const double ctrlP = ro * 0.5;
                const bool ok = std::abs (ro - rpp) <= 0.061 && std::abs (ro - rp) <= 0.061;
                const bool fired = std::abs (ctrlP - rpp) > 0.061;
                prow (ok && fired, "PS1e reverse Bernoulli (30 %)",
                           "meas p ours " + f2 (ro) + " parent " + f2 (rpp) + " | thr |dp| <= 0.061 | margin " + f2 (0.061 - std::abs (ro - rpp))
                               + " | " + ctrl (fired, "x0.5 -> " + f2 (ctrlP)));
            }
            else if (id == "PS2")
            {
                const double gm = parity::findV (row.sp, "gain_rand", 0) / 100.0;
                std::vector<double> vo, vp;
                collect ([] (const ogi::GrainLogEntry& e) { return 20.0 * std::log10 ((double) e.gainL); },
                         [] (const PGrain& g) { return 20.0 * std::log10 ((double) g.gainL); }, vo, vp);
                statsRow ("PS2 gain_rand dB (50 %, spread 0)", vo, vp, -24.0 * gm, 0.0, 0.0);
            }
            else if (id == "PS3")
            {
                const double J = parity::findV (row.sp, "jitter", 0) / 100.0;
                const double I = parity::kFs / (double) mit->second.densityRaw;
                // Onset time t = onsetSample − frac (frac = the fraction BEFORE the
                // sample the onset lands on; RESEARCH-2.2 §3.1). I = fs / density.
                auto disp = [I] (std::map<juce::int64, std::vector<double>>& seqs, std::vector<double>& e, std::vector<double>& iv)
                {
                    for (auto& kv : seqs)
                    {
                        auto& t = kv.second;
                        std::sort (t.begin(), t.end());
                        for (size_t k = 1; k < t.size(); ++k)
                            e.push_back (((t[k] - t[0]) - (double) k * I) / I);
                        for (size_t k = 0; k + 1 < t.size(); k += 2)
                            iv.push_back ((t[k + 1] - t[k]) / I);
                    }
                };
                auto oursSeqs = [] (const std::vector<ogi::GrainLogEntry>& lg)
                {
                    std::map<juce::int64, std::vector<double>> m;
                    for (const auto& e : lg) m[(juce::int64) (e.serial * 8 + (std::uint64_t) e.stream)].push_back ((double) e.onsetSample - e.frac);
                    return m;
                };
                std::map<juce::int64, std::vector<double>> ps;
                for (const auto& g : pg) ps[g.stream].push_back ((double) g.onsetSample - g.frac);
                auto os = oursSeqs (ol);
                std::vector<double> eo, ep, io, ip;
                disp (os, eo, io); disp (ps, ep, ip);
                // Displacement e_k/I ~ U(−J/2, J/2) (u₀ = ½).
                statsRow ("PS3a jitter displacement (J 50 %)", eo, ep, -J / 2, J / 2, 0.0);
                // Even-k intervals: 1 + J (u − u') ~ triangular on [1 − J, 1 + J]; χ² 20 equiprobable bins.
                auto triInv = [J] (double q) { return q < 0.5 ? 1.0 - J + J * std::sqrt (2.0 * q) : 1.0 + J - J * std::sqrt (2.0 * (1.0 - q)); };
                auto chi2 = [&] (const std::vector<double>& v)
                {
                    int cnt[20] {};
                    for (double x : v)
                    {
                        int b = 19;
                        for (int k = 1; k < 20; ++k) if (x < triInv (k / 20.0)) { b = k - 1; break; }
                        ++cnt[b];
                    }
                    const double e = (double) v.size() / 20.0;
                    double c = 0;
                    for (int k = 0; k < 20; ++k) c += (cnt[k] - e) * (cnt[k] - e) / e;
                    return c;
                };
                const double co = chi2 (io), cp = chi2 (ip), cc = chi2 (scaled (io, 0.5, 1.0));
                // Engine-side control: jitterScale 0.5 through the audio path.
                const auto oh = renderOurs (row, [] (Session& s) { s.hk().jitterScale = 0.5; });
                std::vector<ogi::GrainLogEntry> olh;
                for (const auto& e : oh.log) if (e.onsetSample < parity::kOurTon + (juce::int64) (row.seconds * parity::kFs)) olh.push_back (e);
                auto osh = oursSeqs (olh);
                std::vector<double> eh, ih;
                disp (osh, eh, ih);
                juce::String dh;
                const bool engFired = ! uniformTest (eh, ep, -J / 2, J / 2, dh);
                const bool ok = co <= 63.7 && cp <= 63.7 && io.size() >= 1500;
                prow (ok && cc > 63.7 && engFired, "PS3b jitter even-k interval chi2 (20 bins, df 19)",
                           "meas chi2 ours " + f2 (co, 1) + " parent " + f2 (cp, 1) + " (n " + juce::String ((int) io.size()) + "/" + juce::String ((int) ip.size())
                               + ") | thr <= 63.7 | margin " + f2 (63.7 - std::max (co, cp), 1) + " | " + ctrl (cc > 63.7, "x0.5 -> chi2 " + f2 (cc, 1))
                               + " | " + ctrl (engFired, "engine jitterScale 0.5 -> " + dh));
            }
            else if (id == "PS4")
            {
                std::map<float, int> co, cp;
                for (const auto& e : ol) ++co[e.pitchCents];
                for (const auto& g : pg) ++cp[g.pitchCents];
                std::set<float> keys;
                for (auto& kv : co) keys.insert (kv.first);
                for (auto& kv : cp) keys.insert (kv.first);
                const int k = (int) keys.size();
                auto uni = [k] (const std::map<float, int>& m, int n)
                {
                    const double e = (double) n / k; double c = 0;
                    for (auto& kv : m) c += (kv.second - e) * (kv.second - e) / e;
                    c += (double) (k - (int) m.size()) * e;
                    return c;
                };
                const int no = (int) ol.size(), np = (int) pg.size();
                double c2 = 0;   // two-sample (2 × k contingency)
                for (float key : keys)
                {
                    const double a = co.count (key) ? co.at (key) : 0, b = cp.count (key) ? cp.at (key) : 0, tot = a + b;
                    const double ea = tot * no / (double) (no + np), eb = tot * np / (double) (no + np);
                    if (ea > 0) c2 += (a - ea) * (a - ea) / ea;
                    if (eb > 0) c2 += (b - eb) * (b - eb) / eb;
                }
                // χ² critical at α 1e-6 for df = k − 1 (Wilson–Hilferty).
                const double df = k - 1, z = 4.753;
                const double crit = df * std::pow (1.0 - 2.0 / (9.0 * df) + z * std::sqrt (2.0 / (9.0 * df)), 3.0);
                // Control: our picks limited to the lower half of the members.
                std::map<float, int> ch;
                std::vector<float> ks (keys.begin(), keys.end());
                for (auto& kv : co)
                {
                    const auto it = std::find (ks.begin(), ks.end(), kv.first);
                    const int idx = (int) (it - ks.begin());
                    ch[ks[(size_t) (idx % std::max (1, k / 2))]] += kv.second;
                }
                const double cu = uni (co, no), cpu = uni (cp, np), cc = uni (ch, no);
                const bool ok = k >= 10 && cu <= crit && cpu <= crit && c2 <= crit;
                prow (ok && cc > crit, "PS4 scale-on picks (Major, pitch_rand 12) uniform + ours == parent",
                           "meas members " + juce::String (k) + ", chi2 uniform ours " + f2 (cu, 1) + " parent " + f2 (cpu, 1) + ", two-sample " + f2 (c2, 1)
                               + " | thr <= " + f2 (crit, 1) + " (df " + juce::String ((int) df) + ") | margin " + f2 (crit - std::max ({ cu, cpu, c2 }), 1)
                               + " | " + ctrl (cc > crit, "lower half only -> " + f2 (cc, 1)));
            }
        }
    }

    //==========================================================================
    // SC1 / SC3
    juce::String hexF (float v) { std::uint32_t b; std::memcpy (&b, &v, 4); return juce::String::toHexString ((int) b).paddedLeft ('0', 8); }
    juce::String hex64 (std::uint64_t v) { return juce::String::toHexString ((juce::int64) v).paddedLeft ('0', 16); }

    void runScale()
    {
        juce::StringArray lines;
        gDir.getChildFile ("activesets.tsv").readLines (lines);
        std::map<juce::String, juce::String> parent;
        for (const auto& l : lines)
            if (l.isNotEmpty() && ! l.startsWith ("#"))
                parent[l.upToFirstOccurrenceOf ("\t", false, false)] = l;

        int cases = 0, diff = 0, ctrlDiff = 0;
        juce::String firstDiff;
        for (int flip = 0; flip < 2; ++flip)
            for (int c = 1; c <= 31; ++c)
                for (int variant = 0; variant < (c == 31 ? 2 : 1); ++variant)
                {
                    auto p = make();
                    juce::String tag (c);
                    if (c == 31)
                    {
                        juce::String bad;
                        p->setCustomScale ("sc", variant == 0 ? "edo 19" : "edo 64", bad);
                        tag << (variant == 0 ? "a" : "b");
                    }
                    const auto* table = p->getCollectionTable (c);
                    if (table == nullptr) { ++diff; continue; }
                    for (int mk = 0; mk < 3; ++mk)
                    {
                        const std::uint64_t want = mk == 0 ? ~std::uint64_t (0) : (mk == 1 ? parity::kScFixedMask : parity::kScBit63);
                        p->setDegreeMask (1, c, want);
                        std::uint64_t mask = p->getDegreeMask (1, c);
                        if (flip) mask ^= 1u;
                        for (float span : parity::scSpans())
                            for (double pitch : parity::scPitches())
                            {
                                auto as = std::make_unique<ogi::ActiveSet>();
                                as->rebuildIfNeeded (c, mask, span, 0u, *table);
                                int lo = 0, hi = 0;
                                as->window (pitch, (double) span, lo, hi);
                                juce::String key;
                                key << tag << "_" << mk << "_" << (int) span << "_" << (int) pitch;
                                juce::String line;
                                line << key << "\t" << c << "\t" << mk << "\t" << hex64 (flip ? mask ^ 1u : mask) << "\t" << hexF (span) << "\t"
                                     << juce::String (pitch, 1) << "\t" << lo << "\t" << hi << "\t" << as->size() << "\t" << (as->wasTruncated() ? 1 : 0) << "\t";
                                for (int i = 0; i < as->size(); ++i)
                                    line << (i ? "," : "") << hexF (as->data()[i]);
                                const bool same = parent.count (key) && parent[key] == line;
                                if (flip) ctrlDiff += same ? 0 : 1;
                                else
                                {
                                    ++cases;
                                    if (! same) { ++diff; if (firstDiff.isEmpty()) firstDiff = key; }
                                }
                            }
                    }
                }
        const bool fired = ctrlDiff > 0;
        prow (diff == 0 && cases == (int) parent.size() && cases > 0 && fired, "SC1 FUNC-09 ActiveSet members + [lo, hi) == parent (bitwise)",
                   "meas " + juce::String (diff) + "/" + juce::String (cases) + " differ (parent " + juce::String ((int) parent.size()) + " cases)"
                       + (firstDiff.isNotEmpty() ? ", first " + firstDiff : juce::String()) + " | thr 0 | margin " + juce::String (-diff)
                       + " | " + ctrl (fired, "our mask bit 0 flipped -> " + juce::String (ctrlDiff) + " differ"));

        // SC3: custom parse.
        juce::StringArray tl;
        gDir.getChildFile ("customtables.tsv").readLines (tl);
        int k = 0, bad = 0, n = 0, failKept = 0;
        for (const auto& text : parity::scCustomTexts())
        {
            auto p = make();
            juce::String badTok;
            const auto err = p->setCustomScale ("t", juce::String (text), badTok);
            const auto t = p->getCustomTable();
            juce::String line;
            line << k << "\t" << (err.isEmpty() ? "-" : err) << "\t" << (badTok.isEmpty() ? "-" : badTok) << "\t" << t.count << "\t" << hexF (t.period) << "\t";
            for (int i = 0; i < t.count; ++i)
                line << (i ? "," : "") << hexF (t.cents[(size_t) i]);
            juce::String want;
            for (const auto& l : tl)
                if (l.startsWith (juce::String (k) + "\t")) want = l;
            bad += line == want ? 0 : 1;
            failKept += (err.isNotEmpty() && t.count == 12) ? 1 : 0;
            ++k; ++n;
        }
        prow (bad == 0 && n == 5 && failKept == 1, "SC3 custom-scale parse == parent (table, error, badToken; failure keeps 12-EDO)",
                   "meas " + juce::String (bad) + "/" + juce::String (n) + " differ, failures keeping the default " + juce::String (failKept) + " | thr 0 | ctrl --");
    }

    //==========================================================================
    // GV0 / GV1 (D16)
    double gvRms (const std::vector<float>& L, const std::vector<float>& R, size_t a)
    {
        double e = 0; size_t n = 0;
        for (size_t i = a; i < std::min (L.size(), R.size()); ++i) { e += (double) L[i] * L[i] + (double) R[i] * R[i]; ++n; }
        return std::sqrt (e / std::max<size_t> (1, 2 * n));
    }

    double gvParentRms (int s)
    {
        const auto v = readF32 ("GV" + juce::String (s) + ".f32");
        double e = 0; size_t n = 0;
        for (size_t i = (size_t) (parity::kGvSkipS * parity::kFs); i < v.size() / 2; ++i) { e += (double) v[2 * i] * v[2 * i] + (double) v[2 * i + 1] * v[2 * i + 1]; ++n; }
        return std::sqrt (e / std::max<size_t> (1, 2 * n));
    }

    double gvOurs (std::uint64_t seed, double overrideDb)
    {
        Session s (parity::kFs, {}, seed, 512);   // spec defaults
        s.hk().coherenceOff = false;
        s.hk().voiceGainDbOverride = overrideDb;
        juce::AudioBuffer<float> b (2, (int) (parity::kGvMaterialS * parity::kFs));
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < b.getNumSamples(); ++i)
                b.setSample (c, i, parity::gvNoise (i, c));
        s.load (0, b, parity::kFs, "gv");
        s.noteOn (0, 48, 127);
        s.run ((juce::int64) (parity::kGvSeconds * parity::kFs), 512);
        return gvRms (s.L, s.R, (size_t) (parity::kGvSkipS * parity::kFs));
    }

    void runGv (bool verify)
    {
        double t[4], tc[4];
        for (int k = 0; k < 4; ++k)
        {
            const std::uint64_t seed = 0x6A1D0000ull + (std::uint64_t) (k + 1);
            const double rp = gvParentRms (k + 1);
            t[k] = dB (rp / std::max (1e-300, gvOurs (seed, verify ? std::numeric_limits<double>::quiet_NaN() : 0.0)));
            if (verify) tc[k] = dB (rp / std::max (1e-300, gvOurs (seed, -6.0)));
        }
        const double mean = (t[0] + t[1] + t[2] + t[3]) / 4.0;
        const double spread = *std::max_element (t, t + 4) - *std::min_element (t, t + 4);
        const double oldOld = dB (gvOurs (0x6A1D0001ull, 0.0) / gvOurs (0x6A1D0001ull, 0.0));
        if (! verify)
        {
            std::printf ("  GV0 TARGET kVoiceGainMeasuredDb = %+.4f dB (seeds %+.3f %+.3f %+.3f %+.3f; spread %.3f dB)\n", mean, t[0], t[1], t[2], t[3], spread);
            prow (spread <= 0.2 && std::abs (oldOld) < 1e-12, "GV0 D16 measure vs parent mix 100 % (4 seeds)",
                       "meas target " + f2 (mean, 3) + " dB, seed spread " + f2 (spread, 3) + " dB, old-vs-old " + f2 (oldOld, 6)
                           + " dB | thr spread <= 0.2 | margin " + f2 (0.2 - spread, 3) + (std::abs (mean) > 1.0 ? " | STOP: |target| > 1 dB (PD1)" : "") + " | ctrl --");
        }
        else
        {
            const double cm = (tc[0] + tc[1] + tc[2] + tc[3]) / 4.0;
            const bool fired = std::abs (cm) > 0.5;
            prow (std::abs (mean) <= 0.5 && std::abs (oldOld) < 1e-12 && fired, "GV1 D16 shipped kVoiceGainMeasuredDb vs parent",
                       "meas error " + f2 (mean, 3) + " dB (spread " + f2 (spread, 3) + ") | thr |e| <= 0.5 | margin " + f2 (0.5 - std::abs (mean), 3)
                           + " | " + ctrl (fired, "override -6 dB -> " + f2 (cm, 3) + " dB"));
        }
    }

    //==========================================================================
    int run (const juce::File& dir, const juce::String& prefix)
    {
        gDir = dir;
        if (! gDir.getChildFile ("manifest.tsv").existsAsFile())
        {
            std::printf ("--parity: %s has no manifest.tsv (run tests/parity/run.sh first)\n", gDir.getFullPathName().toRawUTF8());
            return 255;
        }
        std::printf ("\n== Phase 2.2 parity gates vs O-Granulator @ 82406fce (%s)\n", gDir.getFullPathName().toRawUTF8());
        auto want = [&prefix] (const char* p) { return prefix.isEmpty() || juce::String (p).startsWith (prefix) || prefix.startsWith (p); };
        if (want ("PN")) runNulls (prefix.startsWith ("PN") ? prefix : juce::String());
        if (want ("PS")) runStats (prefix.startsWith ("PS") ? prefix : juce::String());
        if (want ("SC")) runScale();
        if (prefix.isEmpty() || prefix.startsWith ("GV0")) runGv (false);
        if (prefix == "GV") { runGv (false); runGv (true); }
        return 0;
    }
}} // namespace ::pg22
