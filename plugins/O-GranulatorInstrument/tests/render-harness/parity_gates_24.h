/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.4 parity gates
    vs O-Granulator @ 82406fce (PLAN-2.4 T14, PD12). Included by main.cpp
    AFTER parity_gates_23.h (reuses pg22 / pg23 helpers in the same TU).

      PB0      tap null <= -130 dB (precondition; sets g_PB)
      PB1-12   FeedbackChain return null <= -90 dB, |g / g_PB - 1| <= 1e-4
      PBd      xcorr (our ret, parent ret) peak at lag 0; our written layer vs
               our return at lag 32 (the z^-32)
      PBc1-5   controls: low_cut x 2^(1/12), unity knob (band), diffusion 51,
               testAgcLegacy, return read at lag 31
      PR1-12   reverb + knee output null <= -90 dB, |g / g0 - 1| <= 1e-4
      PRk      knee row (parent peak > 0.6)
      PRc1-2   controls: size 51; legacyWetTanh

    Our side renders with a silent pre-roll to the parent's T_e (note-on at
    T_e), so the reverb LFOs / smoothers and the chains follow the same
    absolute-time history as the parent (PLAN-2.4 deviation, recorded).

  ==============================================================================
*/
#pragma once

namespace { namespace pg24
{
    using eg21::Session;
    using eg21::P;
    using eg21::f2;
    using eg21::ctrl;
    using pg22::prow;
    using pg22::sid;

    //==========================================================================
    struct Ours
    {
        std::vector<float> L, R, tap, ret;   // tap / ret interleaved L, R
        float glRaw[8] {};
        float envMax = 0.0f;
        std::uint64_t writes = 0;
    };

    /** Our side of a PB / PR row: PN01 geometry, note-on at T_e (pre-roll),
        the row's globals (+ our-only overrides), hooks, probes on slot 0. */
    Ours renderOurs (const parity::Row& row, bool freezeOn, bool probes, bool withHooks = true,
                     const std::function<void (Session&)>& afterBlock = {})
    {
        P params = pg22::globalsNull();
        for (auto& kv : pg22::streamParams (row.stream, parity::over (row.sp, row.oursOnly)))
            params.push_back (kv);
        for (const auto& kv : parity::over (row.gl, row.oursGl))
            params.push_back ({ juce::String (kv.id), kv.v });
        params.push_back ({ "freeze", freezeOn ? 1.0f : 0.0f });
        // PB rows: the stream is enabled AT T_e, as on the parent side. The
        // parent's tap carries its 10 ms fb-enable ramp from the enable edge
        // (fbEnRamp, mixerRampCells); ours folds enable into the 10 ms
        // audibility ramp, so enabling at T_e gives the same i/480 ramp
        // (diagnosed at PB0: −57 dB confined to samples [0, 480), bitwise after).
        const bool enableAtTe = parity::isFbKind (row.kind);
        if (enableAtTe)
            for (auto& kv : params)
                if (kv.first == sid (row.stream, "enable")) kv.second = 0.0f;

        Session ses (parity::kFs, params, row.seed, row.blockSize);
        if (enableAtTe)
            eg22::setAt (ses, parity::kTe, sid (row.stream, "enable"), 1.0f);
        ses.hk().coherenceOff = false;   // shipping law
        bool lag31 = false;
        if (withHooks)
            for (const auto& h : row.oursHooks)
            {
                if (h == "chainKnobParity") ses.hk().chainKnobParity = true;
                if (h == "legacyWetTanh")   ses.hk().legacyWetTanh = true;
                if (h == "testAgcLegacy")   ses.eng().chainForTest (0).testAgcLegacy = true;
                if (h == "retLag31")        lag31 = true;
            }
        const auto mat = pg23::materialBuf (row.matPeak);
        ses.load (row.stream - 1, mat, parity::kFs, "mat");

        const juce::int64 ton = parity::kTe + row.ourOnsetShift;
        const juce::int64 len = (juce::int64) std::llround (row.seconds * parity::kFs);
        const juce::int64 end = parity::kTe + len;
        std::vector<float> tapBuf, retBuf;
        if (probes)
        {
            tapBuf.assign ((size_t) (2 * end), 0.0f);
            retBuf.assign ((size_t) (2 * end), 0.0f);
            ses.hk().tapProbe = { tapBuf.data(), end, row.stream - 1 };
            ses.hk().retProbe = { retBuf.data(), end, row.stream - 1 };
        }
        Ours o;
        ses.afterBlock = [&o, &afterBlock, s = row.stream - 1] (Session& ss)
        {
            o.envMax = std::max (o.envMax, ss.eng().getChainForTest (s).env);
            if (afterBlock) afterBlock (ss);
        };
        ses.noteOn (ton, parity::kOurRoot, 127);
        ses.run (end, row.blockSize);
        ses.hk().tapProbe = {};
        ses.hk().retProbe = {};

        o.L.assign (ses.L.begin() + parity::kTe, ses.L.end());
        o.R.assign (ses.R.begin() + parity::kTe, ses.R.end());
        if (probes)
        {
            o.tap.assign (tapBuf.begin() + 2 * parity::kTe, tapBuf.end());
            o.ret.assign (retBuf.begin() + 2 * parity::kTe, retBuf.end());
            if (lag31)
            {
                // PBc5: the return read one sample early (lag 31 instead of 32).
                std::vector<float> sh (o.ret.size(), 0.0f);
                for (size_t i = 2; i < o.ret.size(); ++i)
                    sh[i] = o.ret[i - 2];
                o.ret.swap (sh);
            }
        }
        for (int k = 0; k < 8; ++k)
            o.glRaw[k] = eg21::rawOf (*ses.p, juce::String (parity::fbRevIds()[(size_t) k]));
        o.writes = ses.hk().head[row.stream - 1].writes;
        return o;
    }

    /** LS-aligned residual of two interleaved stereo arrays from frame n0. */
    pg22::Null nullIL (const std::vector<float>& par, const std::vector<float>& ours, size_t n0)
    {
        pg22::Null r;
        const size_t n = std::min (par.size(), ours.size()) / 2;
        double po = 0, oo = 0, pp = 0;
        for (size_t i = n0; i < n; ++i)
            for (int c = 0; c < 2; ++c)
            {
                const double p = par[2 * i + (size_t) c], q = ours[2 * i + (size_t) c];
                po += p * q; oo += q * q; pp += p * p;
            }
        r.n = n > n0 ? n - n0 : 0;
        if (oo <= 0 || pp <= 0) { r.resDb = 0; return r; }
        r.gain = po / oo;
        double e = 0;
        for (size_t i = n0; i < n; ++i)
            for (int c = 0; c < 2; ++c)
            {
                const double d = par[2 * i + (size_t) c] - r.gain * ours[2 * i + (size_t) c];
                e += d * d;
            }
        r.resDb = 10.0 * std::log10 (std::max (1.0e-300, e / pp));
        r.refDb = 10.0 * std::log10 (pp / (2.0 * (double) r.n));
        return r;
    }

    std::vector<float> interleave (const std::vector<float>& L, const std::vector<float>& R)
    {
        std::vector<float> v (2 * std::min (L.size(), R.size()));
        for (size_t i = 0; i < v.size() / 2; ++i) { v[2 * i] = L[i]; v[2 * i + 1] = R[i]; }
        return v;
    }

    /** Lag (in [−maxLag, maxLag]) of the cross-correlation peak of b against a:
        argmax_k Σ a[n]·b[n − k] (L + R), from frame n0. */
    int xcorrPeak (const std::vector<float>& a, const std::vector<float>& b, size_t n0, int maxLag)
    {
        const size_t n = std::min (a.size(), b.size()) / 2;
        int best = 0;
        double bestV = -1e300;
        for (int k = -maxLag; k <= maxLag; ++k)
        {
            double acc = 0;
            for (size_t i = n0; i < n; ++i)
            {
                const std::int64_t j = (std::int64_t) i - k;
                if (j < 0 || j >= (std::int64_t) n) continue;
                acc += (double) a[2 * i] * b[2 * (size_t) j] + (double) a[2 * i + 1] * b[2 * (size_t) j + 1];
            }
            if (acc > bestV) { bestV = acc; best = k; }
        }
        return best;
    }

    struct GlMan { bool ok = false; float raw[8] {}; double peak = 0; };

    std::map<juce::String, GlMan> readGlManifest()
    {
        std::map<juce::String, GlMan> m;
        juce::StringArray lines;
        pg22::gDir.getChildFile ("manifest.tsv").readLines (lines);
        for (const auto& l : lines)
        {
            if (l.isEmpty() || l.startsWith ("#")) continue;
            juce::StringArray t;
            t.addTokens (l, "\t", "");
            if (t.size() < 33) continue;
            GlMan x;
            x.ok = true;
            x.peak = std::strtod (t[24].toRawUTF8(), nullptr);
            for (int k = 0; k < 8; ++k)
                x.raw[k] = std::strtof (t[25 + k].toRawUTF8(), nullptr);
            m[t[0]] = x;
        }
        return m;
    }

    bool rawsSame (const GlMan& m, const Ours& o)
    {
        for (int k = 0; k < 8; ++k)
            if (! juce::exactlyEqual (m.raw[k], o.glRaw[k])) return false;
        return true;
    }

    const parity::Row* rowById (const std::vector<parity::Row>& rs, const std::string& id)
    {
        for (const auto& r : rs) if (r.id == id) return &r;
        return nullptr;
    }

    //==========================================================================
    // PB — FeedbackChain leaf nulls
    void runPB (const juce::String& prefix)
    {
        const auto rs  = parity::rows();
        const auto man = readGlManifest();
        const size_t n0 = 256;
        auto want = [&prefix] (const juce::String& id) { return prefix.isEmpty() || id.startsWith (prefix) || prefix == "PB"; };

        // PB0 first: the tap precondition and g_PB.
        double gPB = 0.0;
        bool pb0Ok = false;
        {
            const auto* row = rowById (rs, "PB0");
            const auto parTap = pg22::readF32 ("tap_PB5.f32");
            if (row == nullptr || parTap.empty())
                prow (false, "PB0 parent tap present", "missing tap_PB5.f32");
            else
            {
                const auto o = renderOurs (*row, true, true);
                const auto nl = nullIL (parTap, o.tap, n0);
                gPB = nl.gain;
                pb0Ok = nl.resDb <= -130.0 && nl.refDb >= -70.0;
                prow (pb0Ok, "PB0 tap precondition vs parent (frozen, chainKnobParity, peak 0.25)",
                      "meas tap residual " + f2 (nl.resDb, 1) + " dB (g_PB " + juce::String (gPB, 7) + ", ref " + f2 (nl.refDb, 1)
                          + " dBFS, n " + juce::String ((juce::int64) nl.n) + ") | thr <= -130 dB | margin " + f2 (-130.0 - nl.resDb, 1)
                          + " | ctrl -- (gates PB1-12)" + (pb0Ok ? "" : " | S1: PB0 precondition failed"));
            }
        }
        if (prefix == "PB0")
            return;

        std::printf ("  PB band table (g / g_PB - 1):\n");
        for (const auto& row : rs)
        {
            if (row.kind != parity::Kind::fbNull || row.id == "PB0" || row.id == "PBd") continue;
            const juce::String id (row.id);
            if (! want (id)) continue;
            const bool isCtrl = id.startsWith ("PBc");
            const juce::String parId = row.parentOf.empty() ? id : juce::String (row.parentOf);
            const auto parRet = pg22::readF32 ("ret_" + parId + ".f32");
            const auto mit = man.find (parId);
            if (parRet.empty() || mit == man.end())
            {
                prow (false, id + " parent return present", "missing ret_" + parId + ".f32 / manifest");
                continue;
            }
            const auto o = renderOurs (row, true, true);
            const auto nl = nullIL (parRet, o.ret, n0);
            const double band = gPB != 0.0 ? nl.gain / gPB - 1.0 : 1e9;
            std::printf ("    %-5s g %.9f  g/g_PB-1 %+.3e  residual %.1f dB  env max %.4f\n", row.id.c_str(), nl.gain, band, nl.resDb, (double) o.envMax);
            const juce::String meas = "return residual " + f2 (nl.resDb, 1) + " dB (g/g_PB - 1 " + juce::String (band, 3) + ", ref "
                                      + f2 (nl.refDb, 1) + " dBFS, n " + juce::String ((juce::int64) nl.n) + ", env max "
                                      + f2 (o.envMax, 4) + ")";
            if (! isCtrl)
            {
                const bool same = rawsSame (mit->second, o);
                const bool agc = id != "PB9" || o.envMax > 0.7f;
                const bool ok = pb0Ok && nl.resDb <= -90.0 && std::abs (band) <= 1.0e-4 && nl.refDb >= -70.0 && same && agc;
                prow (ok, id + " DSP-04 FeedbackChain return null vs parent @82406fce",
                      "meas " + meas + " | landed fb raws " + (same ? "same" : "DIFFER") + (id == "PB9" ? (agc ? " | AGC engaged" : " | AGC NOT engaged") : "")
                          + " | thr <= -90 dB, |band| <= 1e-4 | margin " + f2 (-90.0 - nl.resDb, 1) + " dB"
                          + (pb0Ok ? "" : " | BLOCKED (PB0)"));
            }
            else
            {
                const bool fired = id == "PBc2" ? std::abs (band) > 1.0e-4 : nl.resDb > -90.0;
                const juce::String what = id == "PBc1" ? "PB5 + low_cut x 2^(1/12)"
                                        : id == "PBc2" ? "PB1 with the unity knob (band must fail; null " + f2 (nl.resDb, 1) + " dB recorded)"
                                        : id == "PBc3" ? "PB11 + diffusion 51"
                                        : id == "PBc4" ? "PB9 + testAgcLegacy" : "PB5 return read at lag 31";
                prow (fired, id + " DSP-04 chain null control", "meas " + meas + " | " + ctrl (fired, what));
            }
        }

        // PBd: the return arrays align at lag 0; our written layer is the return z^-32.
        if (want ("PBd"))
        {
            const auto* row = rowById (rs, "PBd");
            const auto parRet = pg22::readF32 ("ret_PB5.f32");
            if (row == nullptr || parRet.empty())
                prow (false, "PBd parent return present", "missing ret_PB5.f32");
            else
            {
                const auto o = renderOurs (*row, true, true);
                const int lagRet = xcorrPeak (o.ret, parRet, n0, 64);

                // Written layer: an unfrozen PB5 render (shipping unity knob,
                // fb 100 → λ = 0.95f once the 20 ms ramps land), 32-sample
                // blocks. After each block the head's cell [cellStart, +32) was
                // written from r[n − 32]; layer = (after − (1 − λ)·before) / λ.
                const parity::Row& r5 = *row;
                P params = pg22::globalsNull();
                for (auto& kv : pg22::streamParams (r5.stream, r5.sp)) params.push_back (kv);
                for (const auto& kv : r5.gl) params.push_back ({ juce::String (kv.id), kv.v });
                params.push_back ({ "freeze", 0.0f });
                for (auto& kv : params)
                    if (kv.first == sid (r5.stream, "enable")) kv.second = 0.0f;
                Session ses (parity::kFs, params, r5.seed, 32);
                eg22::setAt (ses, parity::kTe, sid (r5.stream, "enable"), 1.0f);
                ses.hk().coherenceOff = false;
                ses.load (0, pg23::materialBuf (r5.matPeak), parity::kFs, "mat");
                const juce::int64 len = (juce::int64) std::llround (r5.seconds * parity::kFs);
                const juce::int64 end = parity::kTe + len;
                std::vector<float> retBuf ((size_t) (2 * end), 0.0f), tapBuf ((size_t) (2 * end), 0.0f);
                ses.hk().retProbe = { retBuf.data(), end, 0 };
                ses.hk().tapProbe = { tapBuf.data(), end, 0 };
                std::vector<std::pair<juce::int64, juce::int64>> cells;   // (sample from T_e, tape frame)
                ses.afterBlock = [&cells] (Session& ss)
                {
                    const auto& hd = ss.eng().getHeadForTest (0);
                    const juce::int64 blockStart = ss.pos - 32;
                    if (! hd.parked && hd.wrapOff < 0 && blockStart >= parity::kTe)
                        cells.push_back ({ blockStart - parity::kTe, hd.cellStart });
                };
                ses.noteOn (parity::kTe, parity::kOurRoot, 127);
                ses.run (end, 32);
                ses.hk().retProbe = {};
                ses.hk().tapProbe = {};
                const std::vector<float> ouRet (retBuf.begin() + 2 * parity::kTe, retBuf.end());
                const std::vector<float> ouTap (tapBuf.begin() + 2 * parity::kTe, tapBuf.end());
                const auto* tp = ses.eng().getCurrentTape (0);
                std::vector<float> layer (ouRet.size(), 0.0f);
                const double lam = (double) ogi::kFbKnob;
                int used = 0;
                const size_t skipCells = 40;   // past the 20 ms run / λ ramps (30 cells @ 48 k)
                for (size_t k = skipCells; k < cells.size() && tp != nullptr; ++k)
                    for (int i = 0; i < 32; ++i)
                    {
                        const juce::int64 n = cells[k].first + i, f = cells[k].second + i;
                        if (n < 0 || 2 * (size_t) n + 1 >= layer.size()) continue;
                        for (int c = 0; c < 2; ++c)
                        {
                            const double after  = tp->tape.levelSample (0, c, f);
                            const double before = parity::material (f, c, r5.matPeak);
                            layer[2 * (size_t) n + (size_t) c] = (float) ((after - (1.0 - lam) * before) / lam);
                        }
                        ++used;
                    }
                const std::uint64_t writes = ses.hk().head[0].writes;
                const int lagLayer = xcorrPeak (layer, ouRet, n0, 64);
                const int lagTap   = xcorrPeak (layer, ouTap, n0, 64);
                // Control (PBc5 shape): the lag-31 read peaks at lag 1 against the parent.
                parity::Row c5 = *row;
                c5.oursHooks = { "chainKnobParity", "retLag31" };
                const int lagCtrl = xcorrPeak (renderOurs (c5, true, true).ret, parRet, n0, 64);
                const bool ok = lagRet == 0 && lagLayer == 32 && used > 1000 && writes > 0;
                prow (ok && lagCtrl != 0, "PBd return alignment: ours vs parent lag 0; written layer vs return lag 32 (z^-32)",
                      "meas xcorr peak ret/parent " + juce::String (lagRet) + ", layer/ret " + juce::String (lagLayer)
                          + " (INFO layer/tap " + juce::String (lagTap) + "), " + juce::String (used) + " frames, writes "
                          + eg21::i64 (writes) + " | thr 0 / 32 | " + ctrl (lagCtrl != 0, "lag-31 read -> ret/parent peak at " + juce::String (lagCtrl)));
            }
        }
    }

    //==========================================================================
    // PR — reverb + knee
    void runPR (const juce::String& prefix)
    {
        const auto rs  = parity::rows();
        const auto man = readGlManifest();
        const size_t n0 = 256;
        auto want = [&prefix] (const juce::String& id) { return prefix.isEmpty() || id.startsWith (prefix) || prefix == "PR"; };

        // g0 from PF0 (the 2.3 precondition: our wetSafetyOff vs the parent).
        double g0 = 0.0;
        {
            const auto* pf0 = rowById (rs, "PF0");
            const auto par = pg22::readF32 ("PF0.f32");
            if (pf0 != nullptr && ! par.empty())
                g0 = pg22::nullOf (par, pg23::renderOurs (*pf0, true), n0).gain;
        }
        std::printf ("  PR band table (g / g0 - 1), g0 %.7f:\n", g0);
        for (const auto& row : rs)
        {
            if (row.kind != parity::Kind::revNull) continue;
            const juce::String id (row.id);
            if (! want (id)) continue;
            const bool isCtrl = id.startsWith ("PRc");
            const juce::String parId = row.parentOf.empty() ? id : juce::String (row.parentOf);
            const auto par = pg22::readF32 (parId + ".f32");
            const auto mit = man.find (parId);
            if (par.empty() || mit == man.end())
            {
                prow (false, id + " parent render present", "missing " + parId + ".f32 / manifest");
                continue;
            }
            const auto o = renderOurs (row, false, false);
            const auto nl = nullIL (par, interleave (o.L, o.R), n0);
            const double band = g0 != 0.0 ? nl.gain / g0 - 1.0 : 1e9;
            std::printf ("    %-5s g %.9f  g/g0-1 %+.3e  residual %.1f dB  parent peak %.4f\n", row.id.c_str(), nl.gain, band, nl.resDb, mit->second.peak);
            const juce::String meas = "residual " + f2 (nl.resDb, 1) + " dB (g/g0 - 1 " + juce::String (band, 3) + ", ref "
                                      + f2 (nl.refDb, 1) + " dBFS, n " + juce::String ((juce::int64) nl.n) + ", parent peak "
                                      + f2 (mit->second.peak, 4) + ")";
            if (! isCtrl)
            {
                const bool same = rawsSame (mit->second, o);
                const bool kneeValid = id != "PRk" || mit->second.peak > 0.6;
                const bool ok = nl.resDb <= -90.0 && std::abs (band) <= 1.0e-4 && nl.refDb >= -70.0 && same && kneeValid;
                prow (ok, id + (id == "PRk" ? " DSP-05 reverb + knee null (knee engaged)" : " DSP-05 reverb + knee null vs parent @82406fce"),
                      "meas " + meas + " | landed rev raws " + (same ? "same" : "DIFFER") + (kneeValid ? "" : " | INVALID: parent peak <= 0.6")
                          + " | thr <= -90 dB, |band| <= 1e-4 | margin " + f2 (-90.0 - nl.resDb, 1) + " dB");
            }
            else
            {
                const bool fired = id == "PRc2" ? (nl.resDb > -90.0 || std::abs (band) > 1.0e-4) : nl.resDb > -90.0;
                prow (fired, id + " DSP-05 reverb null control",
                      "meas " + meas + " | " + ctrl (fired, id == "PRc1" ? "PR9 + our size 51" : "PRk + legacyWetTanh (null or band fails)"));
            }
        }
    }

    void run (const juce::File& dir, const juce::String& prefix)
    {
        pg22::gDir = dir;
        std::printf ("\n== Phase 2.4 parity gates (PB FeedbackChain, PR reverb + knee) vs O-Granulator @ 82406fce\n");
        if (prefix.isEmpty() || prefix.startsWith ("PB")) runPB (prefix);
        if (prefix.isEmpty() || prefix.startsWith ("PR")) runPR (prefix);
    }
}} // namespace ::pg24
