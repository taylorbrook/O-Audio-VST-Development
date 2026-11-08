#pragma once
// ============================================================================
//   O-GranulatorInstrument — DSP-03 parity gates, OUR side (PLAN-2.3 T7 / T9)
//
//   --parity <dir> runs pg22::run (DSP-01, unchanged) then pg23::run:
//     PF0        precondition (FX off, wetSafetyOff, peak 0.25): residual
//                ≤ −130 dB; its fitted gain is g₀ (PD6). PF0 > −130 dB →
//                every PFC* / PFX* row reports BLOCKED (PF0) = FAIL (S1).
//     PF rows    48 rows (PFL / PFB / PFH 27, PFD 4, PFC 8, PFR 6, PFX 3):
//                residual ≤ −90 dB after LS alignment over n ≥ 256;
//                |g / g₀ − 1| ≤ 1e-4 (S4); parent peak ≤ 0.45 (INVALID);
//                ref ≥ −70 dBFS; equal grain counts; FX raws landed identical;
//                fxActive equal per grain; PFC1–4 ours bitwise ≡ PF0 ours.
//     PFc1–PFc5  controls: each must exceed −90 dB.
//     PFc6       make-down control (P1): fires iff ≤ −90 dB AND band fails.
//     PS5–PS7    statistical arm (filter_rand, ring_rand, crush_prob) with
//                wrong-distribution controls (ours re-rendered).
//   Rows come from ../parity/ParityRows.h (Kind::fxNull / fxStats; pg22 never
//   sees them). Included by main.cpp after parity_gates_22.h (same TU: the
//   pg22 helpers are reused).
// ============================================================================

namespace { namespace pg23
{
    using eg21::Session;
    using eg21::P;
    using eg21::f2;
    using eg21::ctrl;
    using pg22::prow;
    using pg22::sid;
    using pg22::readF32;
    using pg22::Ours;
    using pg22::Null;
    using pg22::nullOf;

    //==========================================================================
    // Parent artefacts (the 2.3 columns are appended)
    struct PFx { std::uint32_t fxActive = 0; int svfMode = 0; double fcHz = 0, ringHz = 0, driveD = 1; bool crushed = false; };

    std::vector<PFx> readFx (const juce::String& row)
    {
        std::vector<PFx> out;
        juce::StringArray lines;
        pg22::gDir.getChildFile (row + ".grains.tsv").readLines (lines);
        for (const auto& l : lines)
        {
            if (l.isEmpty() || l.startsWith ("#")) continue;
            juce::StringArray t;
            t.addTokens (l, "\t", "");
            if (t.size() < 22) continue;
            PFx g;
            g.fxActive = (std::uint32_t) t[16].getLargeIntValue();
            g.svfMode  = t[17].getIntValue();
            g.fcHz     = std::strtod (t[18].toRawUTF8(), nullptr);
            g.ringHz   = std::strtod (t[19].toRawUTF8(), nullptr);
            g.crushed  = t[20].getIntValue() != 0;
            g.driveD   = std::strtod (t[21].toRawUTF8(), nullptr);
            out.push_back (g);
        }
        return out;
    }

    struct FxManifest { bool ok = false; int grains = 0; float raw[10] {}; double peak = 0; };

    std::map<juce::String, FxManifest> readFxManifest()
    {
        std::map<juce::String, FxManifest> m;
        juce::StringArray lines;
        pg22::gDir.getChildFile ("manifest.tsv").readLines (lines);
        for (const auto& l : lines)
        {
            if (l.isEmpty() || l.startsWith ("#")) continue;
            juce::StringArray t;
            t.addTokens (l, "\t", "");
            if (t.size() < 25) continue;
            FxManifest x;
            x.ok = true;
            x.grains = t[12].getIntValue();
            for (int k = 0; k < 10; ++k)
                x.raw[k] = std::strtof (t[14 + k].toRawUTF8(), nullptr);
            x.peak = std::strtod (t[24].toRawUTF8(), nullptr);
            m[t[0]] = x;
        }
        return m;
    }

    //==========================================================================
    // Our side: pg22::renderOurs with the row's material peak, wetSafetyOff
    // (PF rows) and the row's our-side hooks.
    juce::AudioBuffer<float> materialBuf (float peak)
    {
        juce::AudioBuffer<float> b (2, (int) parity::kF);
        for (int c = 0; c < 2; ++c)
        {
            float* d = b.getWritePointer (c);
            for (int i = 0; i < (int) parity::kF; ++i)
                d[i] = parity::material (i, c, peak);
        }
        return b;
    }

    struct OursFx : Ours { float fxRaw[10] {}; };

    OursFx renderOurs (const parity::Row& row, bool wetOff)
    {
        P params = pg22::globalsNull();
        const bool stats = parity::isStatsKind (row.kind);
        const int s0 = stats ? 1 : row.stream, s1 = stats ? row.streams : row.stream;
        for (int s = s0; s <= s1; ++s)
            for (auto& kv : pg22::streamParams (s, parity::over (row.sp, row.oursOnly)))
                params.push_back (kv);
        Session ses (parity::kFs, params, row.seed, 512);
        ses.hk().coherenceOff = false;   // shipping law (the LS gain absorbs the law ratio)
        ses.hk().wetSafetyOff = wetOff;
        for (const auto& h : row.oursHooks)
        {
            if (h == "legacySvfMake") ses.hk().legacySvfMake = true;
            if (h == "fxOrderSwap")   ses.hk().fxOrderSwap = true;
        }
        const auto mat = materialBuf (row.matPeak);
        for (int s = s0; s <= s1; ++s)
            ses.load (s - 1, mat, parity::kFs, "mat");
        const juce::int64 ton = parity::kOurTon + row.ourOnsetShift;
        ses.noteOn (ton, parity::kOurRoot, 127);
        const juce::int64 len = (juce::int64) std::llround (row.seconds * parity::kFs);
        ses.run (parity::kOurTon + len, 512);
        OursFx o;
        o.L.assign (ses.L.begin() + parity::kOurTon, ses.L.end());
        o.R.assign (ses.R.begin() + parity::kOurTon, ses.R.end());
        o.log = ses.hk().logInOrder();
        o.sizeRaw = eg21::rawOf (*ses.p, sid (s0, "size"));
        o.densityRaw = eg21::rawOf (*ses.p, sid (s0, "density"));
        for (int k = 0; k < 10; ++k)
            o.fxRaw[k] = eg21::rawOf (*ses.p, sid (s0, parity::fxIds()[(size_t) k]));
        return o;
    }

    bool bitwiseSame (const Ours& a, const Ours& b)
    {
        return a.L.size() == b.L.size() && std::memcmp (a.L.data(), b.L.data(), a.L.size() * sizeof (float)) == 0
            && std::memcmp (a.R.data(), b.R.data(), a.R.size() * sizeof (float)) == 0;
    }

    //==========================================================================
    // PF rows
    void runNulls (const juce::String& prefix)
    {
        const auto man  = pg22::readManifest();
        const auto fman = readFxManifest();
        const size_t n0 = 256;

        std::vector<parity::Row> pfRows;
        for (const auto& r : parity::rows())
            if (r.kind == parity::Kind::fxNull)
                pfRows.push_back (r);

        // PF0 first (PD6): the precondition and g₀.
        double g0 = 0.0, pf0Res = 0.0;
        bool pf0Ok = false;
        OursFx pf0Ours;
        {
            const auto& row = pfRows.front();   // PF0
            const auto par = readF32 ("PF0.f32");
            const auto mit = man.find ("PF0");
            if (par.empty() || mit == man.end())
                prow (false, "PF0 parent render present", "missing PF0.f32 / manifest");
            else
            {
                pf0Ours = renderOurs (row, true);
                const auto nl = nullOf (par, pf0Ours, n0);
                g0 = nl.gain;
                pf0Res = nl.resDb;
                pf0Ok = nl.resDb <= -130.0 && nl.refDb >= -70.0;
                if (prefix.isEmpty() || juce::String ("PF0").startsWith (prefix) || prefix.startsWith ("PF0"))
                    prow (pf0Ok, "PF0 FX-off precondition vs parent (wetSafetyOff, peak 0.25)",
                          "meas residual " + f2 (nl.resDb, 1) + " dB (g0 " + juce::String (g0, 7) + ", ref " + f2 (nl.refDb, 1)
                              + " dBFS) | thr <= -130 dB | margin " + f2 (-130.0 - nl.resDb, 1) + " | ctrl -- (gates PFC / PFX)"
                              + (pf0Ok ? "" : " | S1: PF0 precondition failed"));
            }
        }
        if (prefix == "PF0")
            return;

        std::printf ("  PF band table (g / g0 - 1):\n");
        for (const auto& row : parity::rows())
        {
            if (row.kind != parity::Kind::fxNull || row.id == "PF0") continue;
            const juce::String id (row.id);
            if (prefix.isNotEmpty() && ! id.startsWith (prefix)) continue;
            const bool isCtrl = parity::isPfControl (row.id);
            const juce::String parId = row.parentOf.empty() ? id : juce::String (row.parentOf);
            const auto par = readF32 (parId + ".f32");
            const auto pg  = pg22::readGrains (parId);
            const auto pfx = readFx (parId);
            const auto mit = man.find (parId);
            const auto fit = fman.find (parId);
            if (par.empty() || mit == man.end() || fit == fman.end())
            {
                prow (false, id + " parent render present", "missing " + parId + ".f32 / manifest");
                continue;
            }
            const bool crushOrChain = id.startsWith ("PFC") || id.startsWith ("PFX");
            if (! pf0Ok && crushOrChain && ! isCtrl)
            {
                prow (false, id + " DSP-03 null vs parent", "BLOCKED (PF0 " + f2 (pf0Res, 1) + " dB > -130 dB) | S1");
                continue;
            }

            const auto o  = renderOurs (row, true);
            const auto g  = parity::solve (row, o.sizeRaw);
            const auto nl = nullOf (par, o, n0);
            const double band = g0 != 0.0 ? nl.gain / g0 - 1.0 : 1.0;
            const bool bandOk = std::abs (band) <= 1.0e-4;
            std::printf ("    %-5s g %.7f  g/g0-1 %+.3e%s\n", id.toRawUTF8(), nl.gain, band, bandOk ? "" : "  (outside 1e-4)");

            const juce::int64 len = (juce::int64) std::llround (row.seconds * parity::kFs);
            int ourCount = 0, bitsBad = 0;
            std::vector<std::uint32_t> ourBits;
            for (const auto& e : o.log)
                if (e.onsetSample >= parity::kOurTon && e.onsetSample < parity::kOurTon + len)
                {
                    ++ourCount;
                    ourBits.push_back (e.fxActive);
                }
            for (size_t k = 0; k < std::min (ourBits.size(), pfx.size()); ++k)
                bitsBad += ourBits[k] == pfx[k].fxActive ? 0 : 1;
            bool rawsSame = juce::exactlyEqual (o.sizeRaw, mit->second.sizeRaw) && juce::exactlyEqual (o.densityRaw, mit->second.densityRaw);
            if (! isCtrl)
                for (int k = 0; k < 10; ++k)
                    rawsSame = rawsSame && juce::exactlyEqual (o.fxRaw[k], fit->second.raw[k]);
            const double peak = fit->second.peak;
            const bool valid = peak <= 0.45 && nl.refDb >= -70.0;

            juce::String meas = "residual " + f2 (nl.resDb, 1) + " dB (g/g0-1 " + juce::String (band, 7) + ", ref " + f2 (nl.refDb, 1)
                                + " dBFS, parent peak " + f2 (peak, 3) + ", n " + juce::String ((juce::int64) nl.n) + ") | grains ours "
                                + juce::String (ourCount) + " parent " + juce::String ((int) pg.size()) + " | fxActive bad " + juce::String (bitsBad)
                                + " | raws " + (rawsSame ? "same" : "DIFFER");

            if (! isCtrl)
            {
                bool bitwise = true;
                if (id == "PFC1" || id == "PFC2" || id == "PFC3" || id == "PFC4")
                {
                    bitwise = bitwiseSame (o, pf0Ours);
                    meas << " | ours bitwise == PF0: " << (bitwise ? "yes" : "NO");
                }
                const bool ok = valid && nl.resDb <= -90.0 && bandOk && ourCount == (int) pg.size() && bitsBad == 0
                                && (int) pfx.size() == (int) pg.size() && rawsSame && g.ok && bitwise;
                prow (ok, id + " DSP-03 null vs parent @82406fce",
                      "meas " + meas + " | thr <= -90 dB, |g/g0-1| <= 1e-4 | margin " + f2 (-90.0 - nl.resDb, 1) + " dB"
                          + (valid ? "" : " | INVALID (peak > 0.45 or ref < -70 dBFS)") + (bandOk ? "" : " | S4: band"));
            }
            else if (id == "PFc6")
            {
                const bool fired = nl.resDb <= -90.0 && ! bandOk;
                prow (fired, "PFc6 make-down control (PFL6 + legacySvfMake): null passes, band fails",
                      "meas " + meas + " | " + ctrl (fired, "legacySvfMake -> residual " + f2 (nl.resDb, 1) + " dB, band " + juce::String (band, 7)));
            }
            else
            {
                const bool fired = nl.resDb > -90.0;
                const juce::String what = id == "PFc1" ? "PFB5 our cutoff x 2^(1/12)" : id == "PFc2" ? "PFD3 our drive 51"
                                        : id == "PFc3" ? "PFC6 our crush 41" : id == "PFc4" ? "PFR4 our ring_freq x 2^(1/12)"
                                                       : "PFX1 + fxOrderSwap (crush before drive)";
                prow (fired, id + " DSP-03 null control (must exceed -90 dB)", "meas " + meas + " | " + ctrl (fired, what));
            }
        }
    }

    //==========================================================================
    // PS5–PS7
    std::vector<ogi::GrainLogEntry> inWindow (const Ours& o, double seconds)
    {
        std::vector<ogi::GrainLogEntry> ol;
        for (const auto& e : o.log)
            if (e.onsetSample < parity::kOurTon + (juce::int64) (seconds * parity::kFs))
                ol.push_back (e);
        return ol;
    }

    void runStats (const juce::String& prefix)
    {
        for (const auto& row : parity::rows())
        {
            if (row.kind != parity::Kind::fxStats) continue;
            const juce::String id (row.id);
            if (prefix.isNotEmpty() && ! id.startsWith (prefix)) continue;
            const auto pfx = readFx (id);
            if (pfx.empty()) { prow (false, id + " parent grains present"); continue; }
            const auto o  = renderOurs (row, false);
            const auto ol = inWindow (o, row.seconds);

            auto oursWith = [&row] (const char* key, float v)
            {
                parity::Row c = row;
                c.oursOnly = { { key, v } };
                return inWindow (renderOurs (c, false), c.seconds);
            };

            if (id == "PS5" || id == "PS6")
            {
                const bool ring = id == "PS6";
                const double base = ring ? parity::findV (row.sp, "ring_freq", 200.0f) : parity::findV (row.sp, "filter_cutoff", 1000.0f);
                const double half = 2.0 * (ring ? parity::findV (row.sp, "ring_rand", 0.0f) : parity::findV (row.sp, "filter_rand", 0.0f)) / 100.0;   // ±rand·2 oct
                const std::uint32_t bit = ring ? 8u : 1u;
                auto ofOurs = [&] (const std::vector<ogi::GrainLogEntry>& lg, int& bitOn)
                {
                    std::vector<double> v;
                    bitOn = 0;
                    for (const auto& e : lg)
                    {
                        if ((e.fxActive & bit) == 0) continue;
                        ++bitOn;
                        v.push_back (std::log2 ((double) (ring ? e.ringHz : e.fcHz) / base));
                    }
                    return v;
                };
                int bo = 0, bc = 0, bp = 0;
                const auto vo = ofOurs (ol, bo);
                std::vector<double> vp;
                for (const auto& g : pfx)
                    if ((g.fxActive & bit) != 0) { ++bp; vp.push_back (std::log2 ((ring ? g.ringHz : g.fcHz) / base)); }
                juce::String d, dc;
                const bool ok = pg22::uniformTest (vo, vp, -half, half, d);
                const auto ctl = oursWith (ring ? "ring_rand" : "filter_rand", 35.0f);
                const bool ctrlFail = ! pg22::uniformTest (ofOurs (ctl, bc), vp, -half, half, dc);
                const double fo = (double) bo / std::max<size_t> (1, ol.size()), fp = (double) bp / std::max<size_t> (1, pfx.size());
                const bool alive = fo >= 0.99 && fp >= 0.99;
                prow (ok && alive && ctrlFail, id + (ring ? " ring_rand census (200 Hz, 70 %) vs parent" : " filter_rand census (LP 1 kHz, 70 %) vs parent"),
                      "meas " + d + " | stage bit ours " + f2 (fo, 4) + " parent " + f2 (fp, 4)
                          + " | thr D2 <= 0.060, D1 <= 0.043, dmean <= 0.134s, var [0.887, 1.127], range +-" + f2 (half, 2) + " >= 0.98w, bit >= 0.99 | "
                          + ctrl (ctrlFail, "ours rand 35 % -> " + dc));
            }
            else if (id == "PS7")
            {
                const double pTrue = parity::findV (row.sp, "crush_prob", 0.0f) / 100.0;
                auto pHat = [] (const std::vector<ogi::GrainLogEntry>& lg, int& bad)
                {
                    double c = 0;
                    bad = 0;
                    for (const auto& e : lg)
                    {
                        c += e.crushed ? 1 : 0;
                        bad += (((e.fxActive & 4u) != 0) == e.crushed) ? 0 : 1;
                    }
                    return c / std::max<size_t> (1, lg.size());
                };
                int badO = 0, badC = 0, badP = 0;
                const double po = pHat (ol, badO);
                double pp = 0;
                for (const auto& g : pfx) { pp += g.crushed ? 1 : 0; badP += (((g.fxActive & 4u) != 0) == g.crushed) ? 0 : 1; }
                pp /= (double) pfx.size();
                const double pc = pHat (oursWith ("crush_prob", 15.0f), badC);
                const bool ok = ol.size() >= 4000 && pfx.size() >= 4000 && std::abs (po - pp) <= 0.050 && std::abs (po - pTrue) <= 0.0354
                                && std::abs (pp - pTrue) <= 0.0354 && badO == 0 && badP == 0;
                const bool fired = ! (std::abs (pc - pp) <= 0.050 && std::abs (pc - pTrue) <= 0.0354);
                prow (ok && fired, "PS7 crush_prob Bernoulli (crush 50, prob 30 %) vs parent",
                      "meas n " + juce::String ((int) ol.size()) + "/" + juce::String ((int) pfx.size()) + " p ours " + f2 (po, 4) + " parent " + f2 (pp, 4)
                          + " | crush bit != crushed ours " + juce::String (badO) + " parent " + juce::String (badP)
                          + " | thr |dp| <= 0.050, |p - 0.3| <= 0.0354 | margin " + f2 (0.050 - std::abs (po - pp), 4)
                          + " | " + ctrl (fired, "ours prob 15 % -> p " + f2 (pc, 4)));
            }
        }
    }

    //==========================================================================
    int run (const juce::File& dir, const juce::String& prefix)
    {
        pg22::gDir = dir;
        if (! pg22::gDir.getChildFile ("PF0.f32").existsAsFile())
        {
            std::printf ("--parity: %s has no PF0.f32 (re-run tests/parity/run.sh with the 2.3 rows)\n", pg22::gDir.getFullPathName().toRawUTF8());
            prow (false, "PF parent renders present", "missing PF0.f32");
            return 0;
        }
        std::printf ("\n== Phase 2.3 parity gates (DSP-03) vs O-Granulator @ 82406fce\n");
        auto want = [&prefix] (const char* p) { return prefix.isEmpty() || juce::String (p).startsWith (prefix) || prefix.startsWith (p); };
        if (want ("PF")) runNulls (prefix.startsWith ("PF") ? prefix : juce::String());
        if (prefix.isEmpty() || prefix == "PS" || prefix.startsWith ("PS5") || prefix.startsWith ("PS6") || prefix.startsWith ("PS7"))
            runStats (prefix == "PS" ? juce::String() : prefix);
        return 0;
    }
}} // namespace ::pg23
