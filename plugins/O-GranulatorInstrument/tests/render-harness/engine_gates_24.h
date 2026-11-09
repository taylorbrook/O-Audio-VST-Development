/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.4 engine gates
    (PLAN-2.4 T3 / T12: working-copy feedback, Freeze, Reset, reverb + knee).
    Included by main.cpp AFTER engine_gates_23.h (uses eg21 / eg22 / eg23
    helpers and rh::).

    Every row prints: ID, measured value, threshold, margin and its negative
    control ("ctrl FIRED: ..." / "ctrl NOT FIRED (vacuous): ..."); a row whose
    control does not fire FAILS.

      TB0  shipping renders == renders-2.4 (14 jobs)        ctrl N1-default + legacyWetTanh
      TB2  knee identity <= kKneeT; |out| <= |pre| <= ...;   ctrl legacyWetTanh -> identity-half renders differ
           |out| <= float (A) above T (FZ1 48 k)

  ==============================================================================
*/
#pragma once

namespace { namespace eg24
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::setP;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::sameF;
    using eg21::noiseBuf;
    using eg21::sineBuf;
    using eg21::kSeed;
    using eg22::base4;
    using eg22::all4;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;

    constexpr double kFs = 48000.0;
    using Proc = GranulatorInstrumentProcessor;

    void section (const char* t) { std::printf ("\n-- %s\n", t); }

    /** Dev aid: OGI_ROWS=ID1,ID2 runs only those row groups (unset = all). */
    bool want (const char* id)
    {
        const char* e = std::getenv ("OGI_ROWS");
        if (e == nullptr || *e == 0) return true;
        juce::StringArray ids;
        ids.addTokens (juce::String (e), ",", "");
        return ids.contains (juce::String (id));
    }

    /** "<job> <hex>" golden table → map. */
    std::map<juce::String, juce::String> readTable (const juce::String& file)
    {
        std::map<juce::String, juce::String> m;
        juce::StringArray g;
        g.addLines (gGoldenDir.getChildFile (file).loadFileAsString());
        for (const auto& l : g)
            if (l.trim().isNotEmpty())
                m[l.upToFirstOccurrenceOf (" ", false, false)] = l.fromLastOccurrenceOf (" ", false, false).trim();
        return m;
    }

    std::map<juce::String, juce::String> toMap (const juce::StringArray& lines)
    {
        std::map<juce::String, juce::String> m;
        for (const auto& l : lines)
            m[l.upToFirstOccurrenceOf (" ", false, false)] = l.fromLastOccurrenceOf (" ", false, false).trim();
        return m;
    }

    /** Lines of `now` equal to `gold` (same key, same hash). */
    int sameLines (const std::map<juce::String, juce::String>& now, const std::map<juce::String, juce::String>& gold)
    {
        int n = 0;
        for (const auto& kv : now)
        {
            const auto it = gold.find (kv.first);
            n += (it != gold.end() && it->second == kv.second) ? 1 : 0;
        }
        return n;
    }

    double peakOf (const rh::JobOut& o)
    {
        double p = 0.0;
        for (float x : o.L) p = std::max (p, (double) std::abs (x));
        for (float x : o.R) p = std::max (p, (double) std::abs (x));
        return p;
    }

    //==========================================================================
    // TB0 — the true-bypass invariant from PLAN-2.4 T4 on: shipping == renders-2.4
    void rowTB0()
    {
        const auto gold = readTable ("renders-2.4.sha256");
        const auto now  = toMap (rh::computeAll());
        const int same = sameLines (now, gold);

        // Control: N1-default under legacyWetTanh hashes differently from renders-2.4.
        juce::String hC;
        {
            rh::Overrides o = rh::overrides;
            o.legacy = true;
            rh::ScopedOverrides so (o);
            hC = rh::sha (rh::jobN1 (false, kSeed));
        }
        const auto it = gold.find ("N1-default");
        const bool fired = it != gold.end() && hC.isNotEmpty() && hC != it->second;
        row (gold.size() == 14 && now.size() == 14 && same == 14 && fired,
             "TB0 shipping renders == renders-2.4.sha256 (14 jobs, PLAN-2.4 PD1)",
             "meas " + juce::String (same) + "/14 lines identical (golden " + juce::String ((int) gold.size()) + " lines) | thr 14 | margin "
                 + juce::String (same - 14) + " | " + ctrl (fired, "N1-default + legacyWetTanh -> hash differs"));
    }

    //==========================================================================
    // TB1 — true bypass with every 2.4 control moved away from its default
    // (feedback 0 and reverb_mix 0 keep the path off; freeze on at 0.5 s, off at 1.0 s)
    void rowTB1()
    {
        rh::Overrides o;
        o.extra = { { "feedback", 0.0f }, { "reverb_mix", 0.0f }, { "fb_low_cut", 2000.0f }, { "fb_high_cut", 1000.0f },
                    { "fb_saturation", 100.0f }, { "fb_diffusion", 100.0f }, { "reverb_size", 100.0f }, { "reverb_damp", 0.0f } };
        o.events = { { 24000, { { "freeze", 1.0f } } }, { 48000, { { "freeze", 0.0f } } } };
        juce::StringArray ship, legacy;
        {
            rh::ScopedOverrides so (o);
            ship = rh::computeAll();
            legacy = rh::computeAllLegacy();
        }
        const int sameShip = sameLines (toMap (ship), readTable ("renders-2.4.sha256"));
        const int sameLeg  = sameLines (toMap (legacy), readTable ("renders-2.2.sha256"));
        const auto gold = readTable ("renders-2.4.sha256");
        auto n1With = [] (const P& extra)
        {
            rh::Overrides c;
            c.extra = extra;
            rh::ScopedOverrides so (c);
            return rh::sha (rh::jobN1 (false, kSeed));
        };
        const auto it = gold.find ("N1-default");
        const juce::String g = it != gold.end() ? it->second : juce::String();
        const bool fFb  = n1With ({ { "feedback", 0.1f } }) != g;
        const bool fRev = n1With ({ { "reverb_mix", 1.0f } }) != g;
        row (sameShip == 14 && sameLeg == 14 && fFb && fRev && g.isNotEmpty(),
             "TB1 true bypass at moved 2.4 params (fb 0, reverb 0, cutoffs / sat / diffusion / size / damp moved, freeze toggled)",
             "meas shipping " + juce::String (sameShip) + "/14 == renders-2.4, legacy " + juce::String (sameLeg) + "/14 == renders-2.2 | thr 14 / 14 | "
                 + ctrl (fFb && fRev, "N1-default + feedback 0.1 % -> " + juce::String (fFb ? "differs" : "SAME") + "; + reverb_mix 1 % -> "
                                          + juce::String (fRev ? "differs" : "SAME")));
    }

    //==========================================================================
    // TB2 — the knee is identity at or below kKneeT (R§6.2)
    struct StressOut { std::vector<float> L, R; };

    StressOut stressRender (double fs, double secs, bool wetOff, bool legacy)
    {
        Session s (fs, eg23::stressParams(), kSeed, 512);
        shipping (s);
        s.hk().wetSafetyOff  = wetOff;
        s.hk().legacyWetTanh = legacy;
        const int n = (int) (2.0 * fs);
        s.load (0, noiseBuf (2, n, 1.0f, 41), fs);
        s.load (1, noiseBuf (2, n, 1.0f, 42), fs);
        s.load (2, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        s.load (3, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 84 + k);
        s.run ((juce::int64) (secs * fs), 512);
        return { s.L, s.R };
    }

    void rowTB2()
    {
        // Identity half: N1-default and P2-default, shipping vs wetSafetyOff (bitwise).
        // The wetSafetyOff render's peak must sit at or below kKneeT (else one
        // recorded step: s1_level −6 dB on every render; still above → INVALID).
        auto renderPair = [] (const P& extra, bool legacy, rh::JobOut (&ship)[2], rh::JobOut (&off)[2])
        {
            for (int w = 0; w < 2; ++w)
            {
                rh::Overrides o = rh::overrides;
                o.extra = extra;
                o.legacy = legacy;
                o.wetOff = (w == 1);
                rh::ScopedOverrides so (o);
                auto& dst = (w == 0) ? ship : off;
                dst[0] = rh::jobN1 (false, kSeed);
                dst[1] = rh::jobP2 (false);
            }
        };

        P extra;
        rh::JobOut ship[2], off[2];
        renderPair (extra, false, ship, off);
        double pk = std::max (peakOf (off[0]), peakOf (off[1]));
        juce::String step = "none";
        if (pk > (double) ogi::kKneeT)
        {
            extra = { { eg21::s1 ("level"), -6.0f } };
            renderPair (extra, false, ship, off);
            pk = std::max (peakOf (off[0]), peakOf (off[1]));
            step = "s1_level -6 dB (recorded)";
        }
        const bool valid = pk <= (double) ogi::kKneeT && pk > 0.0;
        const bool idN1 = rh::sha (ship[0]) == rh::sha (off[0]);
        const bool idP2 = rh::sha (ship[1]) == rh::sha (off[1]);

        // Control: the identity-half renders under legacyWetTanh differ from wetSafetyOff.
        rh::JobOut lship[2], loff[2];
        renderPair (extra, true, lship, loff);
        const bool fired = rh::sha (lship[0]) != rh::sha (off[0]) && rh::sha (lship[1]) != rh::sha (off[1]);

        // Knee half: the FZ1 stimulus at 48 k, 2 s: knee render vs wetSafetyOff (pre).
        const auto pre = stressRender (kFs, 2.0, true, false);
        const auto out = stressRender (kFs, 2.0, false, false);
        std::uint64_t idBad = 0, bound = 0, notBelow = 0, nId = 0, nKnee = 0, nEqual = 0;
        double prePk = 0.0, outPk = 0.0;
        const size_t N = std::min (pre.L.size(), out.L.size());
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto& a = ch == 0 ? pre.L : pre.R;
            const auto& b = ch == 0 ? out.L : out.R;
            for (size_t i = 0; i < N; ++i)
            {
                const float x = a[i], y = b[i];
                prePk = std::max (prePk, (double) std::abs (x));
                outPk = std::max (outPk, (double) std::abs (y));
                if (std::abs (y) > ogi::kKneeA) ++bound;
                if (std::abs (x) <= ogi::kKneeT)
                {
                    ++nId;
                    if (! sameF (x, y)) ++idBad;
                }
                else
                {
                    ++nKnee;
                    // The knee property is y <= x (OutputLaw.h): just above T,
                    // tanh (u) ~ u rounds back to x in float (PLAN-2.4 deviation:
                    // the plan's strict < is false there; equal count is INFO).
                    if (! (std::abs (y) <= std::abs (x))) ++notBelow;
                    if (std::abs (y) == std::abs (x)) ++nEqual;
                }
            }
        }
        const bool kneeValid = prePk > 0.6 && nKnee > 0;
        const bool ok = valid && idN1 && idP2 && fired && kneeValid && idBad == 0 && bound == 0 && notBelow == 0;
        row (ok, "TB2 knee identity <= kKneeT (N1/P2 default bitwise == wetSafetyOff) + knee bound (FZ1 48 k)",
             "meas identity N1 " + juce::String (idN1 ? "==" : "!=") + ", P2 " + juce::String (idP2 ? "==" : "!=")
                 + " (wetOff peak " + f2 (pk, 4) + ", step " + step + (valid ? "" : ", INVALID") + ")"
                 + "; knee half: " + i64 (nId) + " samples |pre|<=T, " + i64 (idBad) + " differ; " + i64 (nKnee) + " above T, "
                 + i64 (notBelow) + " with |out| > |pre| (INFO " + i64 (nEqual) + " equal, rounding near T); |out| > A: " + i64 (bound) + " (pre peak " + f2 (prePk, 3) + ", out peak "
                 + f2 (outPk, 6) + (kneeValid ? "" : ", INVALID pre <= 0.6") + ") | thr 0 / 0 / 0, out <= " + f2 (ogi::kKneeA, 8)
                 + " | margin " + f2 ((double) ogi::kKneeA - outPk, 6)
                 + " | " + ctrl (fired, "legacyWetTanh -> N1 / P2 identity-half renders differ"));
    }

    //==========================================================================
    // Shared 2.4 helpers
    using Hashes = std::array<std::uint64_t, 4>;

    Hashes tapeHashes (Session& s)
    {
        Hashes h {};
        for (int k = 0; k < 4; ++k)
            if (const auto* t = s.eng().getCurrentTape (k))
                h[(size_t) k] = t->tape.hashForTest();
        return h;
    }

    std::uint64_t writesAll (Session& s)
    {
        std::uint64_t w = 0;
        for (const auto& c : s.hk().head)
            w += c.writes;
        return w;
    }

    /** The hash of a fresh build of `pcm` (a second processor's loader, no notes). */
    std::uint64_t pristineHash (const juce::AudioBuffer<float>& pcm, double rate, double fs)
    {
        Session s (fs, eg21::base(), kSeed, 512);
        s.load (0, pcm, rate);
        s.run (64, 32);
        const auto* t = s.eng().getCurrentTape (0);
        return t != nullptr ? t->tape.hashForTest() : 0;
    }

    struct Classify { std::uint64_t nonFinite = 0, subnormal = 0; };
    inline void classify (float x, Classify& c)
    {
        if (! std::isfinite (x)) ++c.nonFinite;
        else if (std::fpclassify (x) == FP_SUBNORMAL) ++c.subnormal;
    }

    void chainScan (const ogi::FeedbackChain& ch, Classify& c)
    {
        for (int k = 0; k < 2; ++k)
        {
            classify (ch.hpIc1[k], c); classify (ch.hpIc2[k], c);
            classify (ch.lpIc1[k], c); classify (ch.lpIc2[k], c);
            for (int s = 0; s < 4; ++s)
                for (int i = 0; i < ogi::kDiffuserMaxLen; ++i)
                    classify (ch.line[k][s][i], c);
        }
        classify (ch.env, c);
    }

    /** Slots whose written maximum exceeds the bound (FL1 / FL1s). */
    int boundViolations (const std::array<float, 4>& written, const std::array<float, 4>& bound)
    {
        int n = 0;
        for (int k = 0; k < 4; ++k)
            n += written[(size_t) k] > bound[(size_t) k] ? 1 : 0;
        return n;
    }

    juce::AudioBuffer<float> constBuf (int n, float v)
    {
        juce::AudioBuffer<float> b (2, n);
        for (int c = 0; c < 2; ++c)
            std::fill (b.getWritePointer (c), b.getWritePointer (c) + n, v);
        return b;
    }

    void setFreezeAt (Session& s, juce::int64 t, bool on) { eg22::setAt (s, t, "freeze", on ? 1.0f : 0.0f); }

    //==========================================================================
    // FL1 / FL1s / CL1 — the Algorithm §3 stress
    P flParams (float sat, float fb = 100.0f)
    {
        P p = cat ({ base4 (4), { { "polyphony", 16.0f }, { "grain_limit", 3.0f }, { "feedback", fb }, { "fb_saturation", sat },
                                  { "reverb_mix", 100.0f }, { "reverb_size", 100.0f } },
                     all4 ("density", 200.0f), all4 ("size", 500.0f), all4 ("pitch", 12.0f), all4 ("level", 6.0f),
                     all4 ("filter_res", 100.0f), all4 ("filter_cutoff", 1000.0f) });
        const float types[4] { 1.0f, 2.0f, 3.0f, 1.0f };
        for (int n = 1; n <= 4; ++n)
            p = with (p, { { sn (n, "filter_type"), types[n - 1] } });
        return p;
    }

    struct FlOut
    {
        std::array<float, 4> pristinePk {}, written {}, bound {};
        float levelPk[4][5] {};
        Classify out, tape, chain;
        std::uint64_t writes[4] {}, rClamps = 0, writeScrubs = 0, forced = 0;
        float pre = 0, knee = 0;
        double outPk = 0;
        Hashes hash {};
    };

    FlOut flRender (const P& params, double seconds, float tapeAmp, const std::function<void (Session&)>& tweak = {})
    {
        Session s (kFs, params, kSeed, 512);
        shipping (s);
        if (tweak) tweak (s);
        for (int k = 0; k < 4; ++k)
            s.load (k, noiseBuf (2, (int) (4.0 * kFs), tapeAmp, 21 + k), kFs);
        for (int n = 40; n < 56; ++n)
            s.noteOn (0, n);
        FlOut o;
        s.run (32, 32);   // the tick takes the tapes; the notes land after it (no write yet)
        for (int k = 0; k < 4; ++k)
            if (const auto* t = s.eng().getCurrentTape (k))
                o.pristinePk[(size_t) k] = t->tape.levelPeak (0);
        s.run ((juce::int64) (seconds * kFs) - 32, 512);
        auto& h = s.hk();
        for (int k = 0; k < 4; ++k)
        {
            const auto* t = s.eng().getCurrentTape (k);
            float fin = 0.0f;
            if (t != nullptr)
            {
                fin = t->tape.levelPeak (0);
                for (int l = 0; l < 5; ++l) o.levelPk[k][l] = t->tape.levelPeak (l);
                t->tape.scanForTest (o.tape.nonFinite, o.tape.subnormal);
            }
            o.written[(size_t) k] = std::max (h.head[k].maxWritten, fin);
            o.bound[(size_t) k] = std::max (o.pristinePk[(size_t) k], ogi::kAgcCeil);
            o.writes[k] = h.head[k].writes;
            o.rClamps += h.head[k].rClamps;
            o.writeScrubs += h.head[k].writeScrubs;
            chainScan (s.eng().getChainForTest (k), o.chain);
        }
        for (size_t i = 0; i < s.L.size(); ++i)
        {
            classify (s.L[i], o.out); classify (s.R[i], o.out);
            o.outPk = std::max ({ o.outPk, (double) std::abs (s.L[i]), (double) std::abs (s.R[i]) });
        }
        o.forced = h.forcedCrossings;
        o.pre = h.preKneePeak;
        o.knee = h.kneePeak;
        o.hash = tapeHashes (s);
        return o;
    }

    juce::String peaksInfo (const FlOut& o)
    {
        juce::String d;
        for (int k = 0; k < 4; ++k)
        {
            d << " s" << (k + 1) << " w " << f2 (o.written[(size_t) k], 4) << "/" << f2 (o.bound[(size_t) k], 4) << " lv[";
            for (int l = 0; l < 5; ++l) d << (l ? "," : "") << f2 (o.levelPk[k][l], 3);
            d << "]";
        }
        return d;
    }

    void rowsFL1CL1()
    {
        const auto o = flRender (flParams (20.0f), 60.0, 1.0f);
        const int viol = boundViolations (o.written, o.bound);
        bool allW = true;
        for (auto w : o.writes) allW = allW && w > 0;
        // Scanner / bound-checker liveness controls.
        Classify cDen, cNan;
        classify (std::numeric_limits<float>::denorm_min(), cDen);
        classify (std::numeric_limits<float>::quiet_NaN(), cNan);
        std::array<float, 4> b1 = o.bound, w1 = o.bound;
        w1[0] = std::nextafter (b1[0], 2.0f);
        const bool fired = cDen.subnormal == 1 && cNan.nonFinite == 1 && boundViolations (w1, b1) == 1;
        const bool ok = viol == 0 && o.out.nonFinite == 0 && o.tape.nonFinite == 0 && o.tape.subnormal == 0
                     && o.chain.nonFinite == 0 && o.chain.subnormal == 0 && o.out.subnormal == 0 && allW;
        row (ok && fired, "FL1 DSP-04 60 s stress, fb 100 %, 16 voices x 4 streams: tape peak <= max (pristine, 0.7) exactly, all finite",
             "meas bound violations " + juce::String (viol) + " |" + peaksInfo (o) + " | out non-finite " + i64 (o.out.nonFinite)
                 + " subnormal " + i64 (o.out.subnormal) + ", tape (all levels) " + i64 (o.tape.nonFinite) + "/" + i64 (o.tape.subnormal)
                 + ", chains " + i64 (o.chain.nonFinite) + "/" + i64 (o.chain.subnormal) + ", writes " + i64 (o.writes[0]) + "/" + i64 (o.writes[1])
                 + "/" + i64 (o.writes[2]) + "/" + i64 (o.writes[3]) + " | INFO rClamps " + i64 (o.rClamps) + ", writeScrubs " + i64 (o.writeScrubs)
                 + ", forcedCrossings " + i64 (o.forced) + " | thr 0 | " + ctrl (fired, "scanners: denorm_min -> 1, NaN -> 1; bound + 1 ulp -> 1"));

        // CL1: the knee output of the same render; control wetSafetyOff (5 s).
        const bool valid = o.pre > ogi::kKneeA;
        const auto c = flRender (flParams (20.0f), 5.0, 1.0f, [] (Session& s) { s.hk().wetSafetyOff = true; });
        const bool cFired = c.outPk > (double) ogi::kKneeA;
        row (valid && o.knee <= ogi::kKneeA && cFired, "CL1 D23 knee output <= +6 dBFS before output_gain (FL1 render, reverb 100 / size 100)",
             "meas knee peak " + juce::String (o.knee, 8) + " (pre-knee " + f2 (o.pre, 4) + (valid ? "" : ", INVALID: pre-knee <= A")
                 + ") | thr <= " + juce::String (ogi::kKneeA, 8) + " | margin " + juce::String ((double) ogi::kKneeA - o.knee, 8)
                 + " | " + ctrl (cFired, "wetSafetyOff (5 s) -> output peak " + f2 (c.outPk, 4)));
    }

    void rowFL1s()
    {
        const auto o = flRender (flParams (0.0f), 60.0, 0.5f);
        const int viol = boundViolations (o.written, o.bound);
        bool exact = true;
        for (auto b : o.bound) exact = exact && sameF (b, ogi::kAgcCeil);
        const auto c = flRender (flParams (0.0f), 10.1, 0.5f, [] (Session& s)
        {
            s.hk().rClampOff = true;
            s.hk().injectReturn = { 0, 480000, 0.95f, false };
        });
        const bool fired = c.written[0] > ogi::kAgcCeil;
        row (viol == 0 && exact && o.tape.nonFinite == 0, "FL1s DSP-04 sat 0: tape peak <= 0.7f exactly (writer clamp, PLAN-2.4 PD2)",
             "meas bound violations " + juce::String (viol) + " |" + peaksInfo (o) + " | INFO rClamps " + i64 (o.rClamps)
                 + " | thr 0, bound 0.7f | " + ctrl (fired, "rClampOff + injectReturn {s1, 10 s, 0.95} -> written " + f2 (c.written[0], 4)));
    }

    //==========================================================================
    // FL2 — runaway guard
    double fl2TapRms (int voices, bool sumTap)
    {
        P p = cat ({ base4 (1), { { "polyphony", 16.0f }, { "grain_limit", 3.0f }, { "feedback", 100.0f } },
                     eg23::s1p ({ { "density", 200.0f }, { "size", 500.0f }, { "key_track", 0.0f }, { "level", 6.0f },
                                  { "filter_type", 1.0f }, { "filter_cutoff", 1000.0f }, { "filter_res", 100.0f },
                                  { "spray", 30.0f }, { "jitter", 50.0f } }) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().sumTap = sumTap;
        s.record = false;
        s.load (0, noiseBuf (2, (int) (4.0 * kFs), 1.0f, 31), kFs);
        const juce::int64 n = (juce::int64) (20.0 * kFs);
        std::vector<float> tap ((size_t) (2 * n), 0.0f);
        s.hk().tapProbe = { tap.data(), n, 0 };
        for (int k = 0; k < voices; ++k)
            s.noteOn (0, 40 + k);
        s.run (n, 512);
        s.hk().tapProbe = {};
        double e = 0;
        const size_t a = (size_t) (10.0 * kFs);
        for (size_t i = 2 * a; i < tap.size(); ++i) e += (double) tap[i] * tap[i];
        return 10.0 * std::log10 (std::max (1e-300, e / (double) (tap.size() - 2 * a)));
    }

    void rowFL2()
    {
        const double r1 = fl2TapRms (1, false), r16 = fl2TapRms (16, false);
        const double c1 = fl2TapRms (1, true), c16 = fl2TapRms (16, true);
        const bool fired = c16 - c1 >= 6.0;
        row (r16 <= r1 + 1.0 && fired, "FL2 DSP-04 runaway guard: tap RMS 16 voices <= 1 voice + 1 dB (unison, fb 100 %)",
             "meas tap RMS 1 voice " + f2 (r1, 2) + " dB, 16 voices " + f2 (r16, 2) + " dB (delta " + f2 (r16 - r1, 2) + ") | thr <= +1 dB | margin "
                 + f2 (1.0 - (r16 - r1), 2) + " | " + ctrl (fired, "sumTap -> delta " + f2 (c16 - c1, 2) + " dB (>= +6)"));
    }

    //==========================================================================
    // FL3 — freeze hash
    P loopParams (float fb)
    {
        return cat ({ base4 (4), { { "polyphony", 16.0f }, { "feedback", fb } }, all4 ("density", 50.0f), all4 ("size", 200.0f) });
    }

    void loadLoop4 (Session& s, double seconds, float amp, std::vector<juce::AudioBuffer<float>>* keep = nullptr)
    {
        for (int k = 0; k < 4; ++k)
        {
            auto b = noiseBuf (2, (int) (seconds * kFs), amp, 51 + k);
            s.load (k, b, kFs);
            if (keep != nullptr) keep->push_back (b);
        }
    }

    void rowFL3()
    {
        Session s (kFs, loopParams (100.0f), kSeed, 512);
        shipping (s);
        s.record = false;
        loadLoop4 (s, 3.0, 0.5f);
        for (int n : { 48, 50, 52, 53, 55, 57, 59, 60 })
            s.noteOn (0, n);
        setFreezeAt (s, 240000, true);
        s.run (240000, 512);
        const auto hEdge = tapeHashes (s);
        s.run (4800, 512);                    // past the 20 ms ramp, the park and its settle
        const auto hA = tapeHashes (s);
        const auto wA = writesAll (s);
        std::uint64_t parks = 0;
        for (const auto& c : s.hk().head) parks += c.parks;
        s.run ((juce::int64) (10.0 * kFs), 512);
        const auto hB = tapeHashes (s);
        const auto wB = writesAll (s);
        const bool fired = hEdge != hB;
        row (hA == hB && wA == wB && parks >= 4 && wA > 0, "FL3 DSP-04 freeze: tape hashes after the ramp + settle == 10 s later; writes frozen",
             "meas hashes " + juce::String (hA == hB ? "equal" : "DIFFER") + ", writes " + i64 (wA) + " -> " + i64 (wB) + ", parks " + i64 (parks)
                 + " | thr bitwise | " + ctrl (fired, "hash at the freeze edge (before the ramp) differs"));
    }

    //==========================================================================
    // FL4 — feedback 0 == pristine
    void rowFL4()
    {
        std::array<std::uint64_t, 4> pr {};
        for (int k = 0; k < 4; ++k)
            pr[(size_t) k] = pristineHash (noiseBuf (2, (int) (4.0 * kFs), 1.0f, 21 + k), kFs, kFs);
        const auto o = flRender (flParams (20.0f, 0.0f), 60.0, 1.0f);
        int same = 0;
        for (int k = 0; k < 4; ++k) same += o.hash[(size_t) k] == pr[(size_t) k] ? 1 : 0;
        std::uint64_t w = 0;
        for (auto x : o.writes) w += x;
        // settles: re-run counters are in the render; a parked head never settles.
        const auto c = flRender (flParams (20.0f, 0.1f), 5.0, 1.0f);
        int cDiff = 0;
        for (int k = 0; k < 4; ++k) cDiff += c.hash[(size_t) k] != pr[(size_t) k] ? 1 : 0;
        row (same == 4 && w == 0, "FL4 DSP-04 feedback 0: every tape hash == its pristine build after 60 s (bitwise), 0 writes",
             "meas " + juce::String (same) + "/4 hashes == pristine, writes " + i64 (w) + " | thr 4/4, 0 | "
                 + ctrl (cDiff > 0, "feedback 0.1 % (5 s) -> " + juce::String (cDiff) + "/4 differ"));
    }

    //==========================================================================
    // FL5 — block-size null with the loop active
    struct BlockOut { std::vector<float> out; Hashes hash {}; };

    BlockOut fl5Render (int block, std::uint64_t seed)
    {
        P p = cat ({ eg23::fn2Params(), eg22::randomised(), { { "polyphony", 8.0f }, { "feedback", 80.0f }, { "reverb_mix", 50.0f } } });
        Session s (kFs, p, seed, 512);
        shipping (s);
        eg22::loadNoise4 (s, 3.0, 0.3f);
        int k = 0;
        for (int n : { 48, 52, 55, 60, 64, 67, 71, 72, 74, 76 })
            s.noteOn (1003 * (k++), n);   // > 8 counted: steals
        s.noteOn (150011, 43);
        s.noteOn (150011, 79);
        s.noteOff (300007, 52);
        s.noteOff (500009, 64);
        s.addMidi (100003, juce::MidiMessage::pitchWheel (1, 12000));
        s.addMidi (400019, juce::MidiMessage::pitchWheel (1, 4000));
        s.addMidi (700001, juce::MidiMessage::pitchWheel (1, 8192));
        for (juce::int64 t : { 96000, 192000, 384000, 480000 })   // freeze toggles x 4 (on / off / on / off)
            setFreezeAt (s, t, t == 96000 || t == 384000);
        s.at (288000, [] (Session& ss) { ss.p->resetSlotSync (0); });
        s.at (576000, [] (Session& ss) { ss.p->resetSlotSync (2); });
        eg22::setAt (s, 432000, "reverb_mix", 0.0f);
        eg22::setAt (s, 672000, "reverb_mix", 50.0f);
        s.run ((juce::int64) (20.0 * kFs), block);
        return { eg22::lr (s), tapeHashes (s) };
    }

    void rowFL5()
    {
        const auto ref = fl5Render (32, kSeed);
        Tally t;
        juce::String d;
        int worst = 0;
        for (int b : { 441, 512, 4096, 0 })
        {
            const auto r = fl5Render (b, kSeed);
            const int n = eg21::diffCount (ref.out, r.out);
            const bool hs = r.hash == ref.hash;
            worst = std::max (worst, n);
            t.check (n == 0 && hs, (b == 0 ? juce::String ("var") : juce::String (b)) + " " + juce::String (n) + (hs ? "" : " hash"));
            d << " " << (b == 0 ? juce::String ("var") : juce::String (b)) << ":" << n << (hs ? "" : "(hash!)");
        }
        const auto c = fl5Render (32, kSeed + 1);
        const bool fired = eg21::diffCount (ref.out, c.out) > 0;
        row (t.bad == 0 && fired, "FL5 block-size null, fb 80 %, freeze x 4, 2 Resets, reverb 50 -> 0 -> 50, steals, bend (20 s)",
             "meas differing samples / tape hashes vs 32:" + d + " | thr 0, bitwise | margin " + juce::String (-worst)
                 + " | " + ctrl (fired, "seed + 1 -> output differs"));
    }

    //==========================================================================
    // FL6 — NaN scrub
    struct Fl6Out { Hashes hash {}; std::int64_t scrubs = 0; std::uint64_t writeScrubs = 0; Classify tape; };

    Fl6Out fl6Render (const std::function<void (Session&)>& tweak)
    {
        Session s (kFs, loopParams (100.0f), kSeed, 512);
        shipping (s);
        s.record = false;
        if (tweak) tweak (s);
        loadLoop4 (s, 3.0, 0.5f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.run ((juce::int64) (3.0 * kFs), 512);
        Fl6Out o;
        o.hash = tapeHashes (s);
        o.scrubs = s.eng().getChainForTest (1).scrubs;
        for (const auto& c : s.hk().head) o.writeScrubs += c.writeScrubs;
        for (int k = 0; k < 4; ++k)
            if (const auto* t = s.eng().getCurrentTape (k)) t->tape.scanForTest (o.tape.nonFinite, o.tape.subnormal);
        return o;
    }

    void rowFL6()
    {
        const auto ref = fl6Render ({});
        const auto o = fl6Render ([] (Session& s) { s.hk().injectFbNan = { 1, 96000, false }; });
        const bool others = o.hash[0] == ref.hash[0] && o.hash[2] == ref.hash[2] && o.hash[3] == ref.hash[3];
        const auto c = fl6Render ([] (Session& s)
        {
            s.hk().writeScrubOff = true;
            s.hk().injectReturn = { 1, 96000, std::numeric_limits<float>::quiet_NaN(), false };
        });
        const bool fired = c.tape.nonFinite >= 1;
        row (o.scrubs == 1 && o.tape.nonFinite == 0 && others && o.writeScrubs == 0,
             "FL6 ARCH §17 NaN into chain s2: scrubbed once, every tape finite, s1 / s3 / s4 == no-inject",
             "meas chain s2 scrubs " + juce::String ((juce::int64) o.scrubs) + ", tape non-finite " + i64 (o.tape.nonFinite) + ", s1/s3/s4 "
                 + (others ? "equal" : "DIFFER") + ", writeScrubs " + i64 (o.writeScrubs) + " | thr 1 / 0 / equal / 0 | "
                 + ctrl (fired, "writeScrubOff + injectReturn {s2, 2 s, NaN} -> " + i64 (c.tape.nonFinite) + " non-finite tape samples"));
    }

    //==========================================================================
    // FL7 — unread params with the loop active
    struct Fl7Out { std::vector<float> out; Hashes hash {}; };

    Fl7Out fl7Render (const std::function<void (Proc&)>& tweak)
    {
        P p = cat ({ loopParams (80.0f), eg22::randomised(), { { "reverb_mix", 50.0f } } });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        if (tweak) tweak (*s.p);
        loadLoop4 (s, 3.0, 0.3f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        setFreezeAt (s, 48000, true);
        setFreezeAt (s, 72000, false);
        s.run ((juce::int64) (3.0 * kFs), 512);
        return { eg22::lr (s), tapeHashes (s) };
    }

    void rowFL7()
    {
        const auto readSet = eg21::readSet25b();   // PLAN-2.5b S8 user decision 2026-11-09 (2.5a: readSet25a 251 / 10, S8; 2.4: readSet24 162 / 99)
        int bound = 0;
        {
            auto probe = make();
            for (const auto& id : readSet)
                bound += param (*probe, id) != nullptr ? 1 : 0;
        }
        int touched = 0;
        auto unread = [&readSet, &touched] (Proc& p)
        {
            juce::Random r ((juce::int64) 0x4E240007);
            touched = 0;
            for (auto* ap : p.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
                    if (! readSet.contains (rp->getParameterID()))
                    {
                        rp->setValueNotifyingHost (r.nextFloat());
                        ++touched;
                    }
        };
        const auto ref = fl7Render ({});
        const auto o = fl7Render (unread);
        const int n = eg21::diffCount (ref.out, o.out);
        const auto cD = fl7Render ([] (Proc& p) { setP (p, "fb_diffusion", 51.0f); });
        const auto cR = fl7Render ([] (Proc& p) { setP (p, "reverb_size", 51.0f); });
        const int nD = eg21::diffCount (ref.out, cD.out), nR = eg21::diffCount (ref.out, cR.out);
        const bool fired = (nD > 0 || cD.hash != ref.hash) && nR > 0;
        row (n == 0 && o.hash == ref.hash && bound == 254 && touched == 7 && fired,
             "FL7 7 unread params bit-identical with the loop active (fb 80 %, reverb 50 %, freeze x 2; readSet25b)",
             "meas " + juce::String (n) + " samples differ, tape hashes " + (o.hash == ref.hash ? "equal" : "DIFFER") + " (" + juce::String (touched)
                 + " randomised, " + juce::String (bound) + "/254 bound) | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (fired, "fb_diffusion 51 -> " + juce::String (nD) + ", reverb_size 51 -> " + juce::String (nR) + " differ"));
    }

    //==========================================================================
    // FL8 — tap law: one cloud never exceeds the tape peak
    double fl8TapPeak (float density, float size, float window, bool arch)
    {
        P p = cat ({ base4 (1), { { "polyphony", 16.0f }, { "grain_limit", 3.0f } },
                     eg23::s1p ({ { "density", density }, { "size", size }, { "window", window }, { "level", 6.0f } }) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().fbArchLaw = arch;
        s.record = false;
        s.load (0, constBuf ((int) (6.0 * kFs), 0.5f), kFs);
        const juce::int64 n = (juce::int64) (4.0 * kFs);
        std::vector<float> tap ((size_t) (2 * n), 0.0f);
        s.hk().tapProbe = { tap.data(), n, 0 };
        s.noteOn (0, 48);
        s.run (n, 512);
        s.hk().tapProbe = {};
        double pk = 0;
        for (float x : tap) pk = std::max (pk, (double) std::abs (x));
        return pk;
    }

    void rowFL8()
    {
        double worst = 0, worstArch = 0;
        juce::String d;
        for (float dn : { 10.0f, 75.0f, 200.0f })
            for (float sz : { 20.0f, 500.0f })
                for (float w : { 0.0f, 0.5f })
                {
                    const double pk = fl8TapPeak (dn, sz, w, false);
                    worst = std::max (worst, pk);
                    worstArch = std::max (worstArch, fl8TapPeak (dn, sz, w, true));
                }
        const double thr = 0.5 * (1.0 + 1.0e-6);
        const bool fired = worstArch > 0.5;
        row (worst <= thr && worst > 0.1, "FL8 tap law: one cloud's tap <= the tape peak (DC 0.5, 12 density x size x window configs)",
             "meas worst tap peak " + juce::String (worst, 9) + " | thr <= " + juce::String (thr, 9) + " | margin " + juce::String (thr - worst, 9)
                 + " | " + ctrl (fired, "fbArchLaw -> worst " + juce::String (worstArch, 6)));
    }

    //==========================================================================
    // CL2 — the FZ1 stimulus at 44.1 / 48 / 96 k
    struct ClOut { float knee = 0, pre = 0; double outPk = 0; };

    ClOut clRender (double fs, bool wetOff, bool inject)
    {
        Session s (fs, eg23::stressParams(), kSeed, 512);
        shipping (s);
        s.hk().wetSafetyOff = wetOff;
        if (inject) s.hk().injectPreKnee = { (std::int64_t) (0.5 * fs), 8.0f, false };
        const int n = (int) (2.0 * fs);
        s.load (0, noiseBuf (2, n, 1.0f, 41), fs);
        s.load (1, noiseBuf (2, n, 1.0f, 42), fs);
        s.load (2, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        s.load (3, sineBuf (2, n, 1000.0, fs, 1.0), fs);
        for (int k = 0; k < 16; ++k)
            s.noteOn (0, 84 + k);
        s.run ((juce::int64) (1.5 * fs), 512);
        ClOut o;
        o.knee = s.hk().kneePeak;
        o.pre = s.hk().preKneePeak;
        for (size_t i = 0; i < s.L.size(); ++i)
            o.outPk = std::max ({ o.outPk, (double) std::abs (s.L[i]), (double) std::abs (s.R[i]) });
        return o;
    }

    void rowCL2()
    {
        Tally t;
        juce::String d, dc;
        bool fired = true;
        for (double fs : { 44100.0, 48000.0, 96000.0 })
        {
            const auto o = clRender (fs, false, false);
            t.check (o.knee <= ogi::kKneeA, f2 (fs / 1000.0, 1) + " k " + juce::String (o.knee, 8));
            d << " " << f2 (fs / 1000.0, 1) << " k knee " << juce::String (o.knee, 6) << " (pre " << f2 (o.pre, 3) << ")";
            const auto c = clRender (fs, true, true);
            const auto k = clRender (fs, false, true);
            fired = fired && c.outPk > (double) ogi::kKneeA && k.knee <= ogi::kKneeA;
            dc << " " << f2 (fs / 1000.0, 1) << " k off " << f2 (c.outPk, 3) << " / on " << juce::String (k.knee, 6);
        }
        row (t.bad == 0 && fired, "CL2 D23 knee output <= +6 dBFS on the FZ1 stimulus (carried), 44.1 / 48 / 96 k",
             "meas" + d + " | thr <= " + juce::String (ogi::kKneeA, 8) + " | " + ctrl (fired, "injectPreKnee 8.0 + wetSafetyOff -> > A; knee on stays <= A:" + dc));
    }

    //==========================================================================
    // MS1 — mip settle: every level == an offline build of the final level 0
    int ms1Mismatch (bool settleOff, juce::String& info)
    {
        Session s (kFs, loopParams (80.0f), kSeed, 512);
        shipping (s);
        s.hk().settleOff = settleOff;
        s.record = false;
        loadLoop4 (s, 2.0, 0.5f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        for (int n : { 48, 52, 55, 60 })
            s.noteOff ((juce::int64) (21.0 * kFs), n);
        s.run ((juce::int64) (21.0 * kFs), 512);
        int guard = 0;
        auto allParked = [&s]
        {
            for (int k = 0; k < 4; ++k) if (! s.eng().getHeadForTest (k).parked) return false;
            return true;
        };
        while (! allParked() && guard++ < 400)
            s.run (512, 512);
        std::uint64_t settles = 0, parks = 0, writes = writesAll (s);
        for (const auto& c : s.hk().head) { settles += c.settles; parks += c.parks; }
        int bad = 0;
        juce::String where;
        for (int k = 0; k < 4; ++k)
        {
            const auto* t = s.eng().getCurrentTape (k);
            if (t == nullptr) { ++bad; continue; }
            const auto& hd = s.eng().getHeadForTest (k);
            const int F = t->tape.getFrames();
            ogi::MipTape<4> ref;
            ref.prepare (F);
            std::vector<float> l (4096), r (4096);
            for (int p = 0; p < F; p += 4096)
            {
                const int n = std::min (4096, F - p);
                for (int i = 0; i < n; ++i) { l[(size_t) i] = t->tape.levelSample (0, 0, p + i); r[(size_t) i] = t->tape.levelSample (0, 1, p + i); }
                ref.writeRun (p, n, l.data(), r.data());
            }
            ref.flushTail();
            for (int lv = 0; lv <= 4; ++lv)
                for (int ch = 0; ch < 2; ++ch)
                    for (int j = 0; j < t->tape.levelSize (lv); ++j)
                        if (! sameF (t->tape.levelSample (lv, ch, j), ref.levelSample (lv, ch, j)))
                        {
                            if (bad < 6)
                                where << " [s" << (k + 1) << " l" << lv << " j" << j << " (frame " << (j << lv) << ") h " << (juce::int64) hd.h
                                      << " lo " << (juce::int64) hd.lo << " hi " << (juce::int64) hd.hi << " F " << F << "]";
                            ++bad;
                        }
        }
        if (where.isNotEmpty())
            info << " | first:" << where << " |";
        info = "parked " + juce::String (allParked() ? "yes" : "NO") + ", writes " + i64 (writes) + ", settles " + i64 (settles) + ", parks " + i64 (parks) + info;
        return allParked() && writes > 0 ? bad : -1;
    }

    void rowMS1()
    {
        juce::String info, infoC;
        const int bad = ms1Mismatch (false, info);
        const int badC = ms1Mismatch (true, infoC);
        const bool fired = badC > 0;
        row (bad == 0, "MS1 mip settle: every level of every slot == an offline build of the final level 0 (>= 10 laps, park)",
             "meas mismatching samples " + juce::String (bad) + " (" + info + ") | thr 0 (bitwise) | "
                 + ctrl (fired, "settleOff -> " + juce::String (badC) + " mismatching"));
    }

    //==========================================================================
    // RS1 / RS2 — Reset == pristine
    void rowRS1 (bool doReset, Hashes& after, Hashes& later, std::uint64_t& wA, std::uint64_t& wB, Hashes& pr)
    {
        Session s (kFs, loopParams (100.0f), kSeed, 512);
        shipping (s);
        s.record = false;
        std::vector<juce::AudioBuffer<float>> keep;
        loadLoop4 (s, 3.0, 0.5f, &keep);
        for (int k = 0; k < 4; ++k) pr[(size_t) k] = pristineHash (keep[(size_t) k], kFs, kFs);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        setFreezeAt (s, 240000, true);
        if (doReset)
            s.at (244800, [] (Session& ss) { ss.p->resetSlotSync (-1); });
        s.run (249600, 512);   // 5.2 s: the take + the 20 ms crossfade done
        after = tapeHashes (s);
        wA = writesAll (s);
        s.run ((juce::int64) (2.0 * kFs), 512);
        later = tapeHashes (s);
        wB = writesAll (s);
    }

    void rowsRS12()
    {
        Hashes a {}, l {}, pr {}, ca {}, cl {};
        std::uint64_t wA = 0, wB = 0, cwA = 0, cwB = 0;
        rowRS1 (true, a, l, wA, wB, pr);
        rowRS1 (false, ca, cl, cwA, cwB, pr);
        const bool fired = ca != pr;
        row (a == pr && l == a && wA == wB, "RS1 DSP-04 Reset (all slots) while frozen == pristine; still frozen 2 s later",
             "meas hashes " + juce::String (a == pr ? "== pristine" : "!= pristine") + ", 2 s later " + (l == a ? "unchanged" : "CHANGED")
                 + ", writes " + i64 (wA) + " -> " + i64 (wB) + " | thr bitwise | " + ctrl (fired, "no Reset -> hashes != pristine"));

        auto rs2 = [] (bool doReset, Hashes& before, Hashes& after, Hashes& pr2) -> bool
        {
            Session s (kFs, loopParams (100.0f), kSeed, 512);
            shipping (s);
            s.record = false;
            std::vector<juce::AudioBuffer<float>> keep;
            loadLoop4 (s, 3.0, 0.5f, &keep);
            for (int k = 0; k < 4; ++k) pr2[(size_t) k] = pristineHash (keep[(size_t) k], kFs, kFs);
            for (int n : { 48, 52, 55, 60 })
            {
                s.noteOn (0, n);
                s.noteOff (144000, n);
            }
            s.run (144000, 512);
            int guard = 0;
            auto parked = [&s] { for (int k = 0; k < 4; ++k) if (! s.eng().getHeadForTest (k).parked) return false; return true; };
            while (! parked() && guard++ < 1000)
                s.run (512, 512);
            const bool ok = parked();
            before = tapeHashes (s);
            if (doReset)
                s.p->resetSlotSync (0);
            s.run (4800, 512);
            after = tapeHashes (s);
            return ok;
        };
        Hashes b {}, af {}, p2 {}, cb {}, caf {};
        const bool parked = rs2 (true, b, af, p2);
        rs2 (false, cb, caf, p2);
        const bool othersSame = af[1] == b[1] && af[2] == b[2] && af[3] == b[3];
        const bool fired2 = caf[0] != p2[0];
        row (parked && af[0] == p2[0] && othersSame, "RS2 DSP-04 Reset s1 while parked (notes off, return decayed) == pristine; s2-s4 unchanged",
             "meas parked " + juce::String (parked ? "yes" : "NO") + ", s1 " + (af[0] == p2[0] ? "== pristine" : "!= pristine") + ", s2-s4 "
                 + (othersSame ? "unchanged" : "CHANGED") + " | thr bitwise | " + ctrl (fired2, "no Reset -> s1 != pristine"));
    }

    //==========================================================================
    // RS3 — host idle: 100 Reset requests, no processBlock
    void rowRS3()
    {
        const int base0 = ogi::SlotTape::liveCount.load();
        int maxPending = 0, livePost = -1, pendingSlots = 0;
        std::uint64_t published = 0, freed = 0;
        int deferred = -1;
        bool idleOk = true;
        {
            auto p = make();
            p->prepareToPlay (48000.0, 512);
            juce::String e;
            for (int k = 0; k < 4; ++k)
                p->loadSlotSync (k, noiseBuf (2, 9600, 0.2f, 61 + k), 48000.0, "rs3", e);
            auto& h = p->getHandoff();
            const auto pub0 = h.getPublishedCount();
            for (int i = 0; i < 50; ++i)
            {
                p->requestReset (-1);
                idleOk = eg21::waitIdle (*p, 3000) && idleOk;
                eg21::pump (5);
                for (int k = 0; k < 4; ++k) maxPending = std::max (maxPending, h.pendingDepth (k));
            }
            for (int i = 0; i < 50; ++i)
            {
                p->requestReset (-1);
                for (int k = 0; k < 4; ++k) maxPending = std::max (maxPending, h.pendingDepth (k));
            }
            juce::Thread::sleep (500);
            eg21::pump (300);
            for (int k = 0; k < 4; ++k)
            {
                maxPending = std::max (maxPending, h.pendingDepth (k));
                pendingSlots += h.pendingDepth (k);
            }
            published = h.getPublishedCount() - pub0;
            freed = h.getSupersededFreed();
            deferred = h.deferredCount();
            livePost = ogi::SlotTape::liveCount.load() - base0;
        }
        int deferredK = -1;
        {
            auto q = make();
            q->prepareToPlay (48000.0, 512);
            juce::String e;
            q->loadSlotSync (0, noiseBuf (2, 9600, 0.2f, 61), 48000.0, "rs3", e);
            q->getHandoff().testNoEntryCounter.store (true);
            for (int i = 0; i < 20; ++i)
            {
                q->requestReset (0);
                juce::Thread::sleep (20);
                eg21::pump (5);
            }
            eg21::pump (300);
            deferredK = q->getHandoff().deferredCount();
            q->getHandoff().testNoEntryCounter.store (false);
            q->getHandoff().sweep();
        }
        const bool fired = deferredK >= 10;
        row (idleOk && maxPending <= 1 && livePost <= 4 && pendingSlots == 4 && published >= 50 && deferred == 0 && fired,
             "RS3 PERF-03 host idle: 100 requestReset (-1), 0 processBlock -> pending <= 1 per slot, bounded memory",
             "meas pending max " + juce::String (maxPending) + ", resets published " + i64 (published) + ", superseded freed " + i64 (freed)
                 + ", deferred " + juce::String (deferred) + ", live tapes " + juce::String (livePost) + ", slots pending " + juce::String (pendingSlots)
                 + " | thr <= 1, live <= 4, 4 pending | " + ctrl (fired, "no entry counter -> " + juce::String (deferredK) + " deferred (leak)"));
    }

    //==========================================================================
    // RS4 — a Reset requested mid-crossfade queues
    struct Rs4Out { std::vector<juce::int64> takes; int maxPending = 0; std::uint64_t swapWaits = 0; };

    Rs4Out rs4Render (juce::int64 bDelay)
    {
        Session s (kFs, loopParams (100.0f), kSeed, 32);
        shipping (s);
        s.record = false;
        loadLoop4 (s, 3.0, 0.5f);
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (0, n);
        s.at (96000, [] (Session& ss) { ss.p->resetSlotSync (0); });
        Rs4Out o;
        std::uint64_t lastX = 0;
        bool bScheduled = false;
        s.afterBlock = [&o, &lastX, &bScheduled, bDelay] (Session& ss)
        {
            const auto x = ss.hk().xfadesStarted;
            if (x != lastX)
            {
                o.takes.push_back (ss.pos - 32);   // the tick at this block's start
                lastX = x;
                if (! bScheduled)
                {
                    bScheduled = true;
                    ss.at (ss.pos - 32 + bDelay, [] (Session& s2) { s2.p->resetSlotSync (0); });
                }
            }
            o.maxPending = std::max (o.maxPending, ss.p->getHandoff().pendingDepth (0));
        };
        s.run ((juce::int64) (2.6 * kFs), 32);
        o.swapWaits = s.hk().swapWaits;
        return o;
    }

    void rowRS4()
    {
        const auto o = rs4Render (480);       // B 10 ms after A's take (inside the 20 ms fade)
        const juce::int64 xlen = 960;
        const bool queued = o.takes.size() == 2 && o.takes[1] >= o.takes[0] + xlen && o.maxPending <= 1;
        const auto c = rs4Render (4800);      // B 100 ms after: the head runs again → waits for its park
        const bool fired = c.takes.size() == 2 && c.takes[1] > c.takes[0] + 4800 && c.takes[1] <= c.takes[0] + 4800 + 960 + 64 && c.swapWaits > 0;
        row (queued, "RS4 PERF-03 Reset B mid-crossfade queues behind A (taken only after A's 20 ms fade)",
             "meas takes " + juce::String (o.takes.size()) + (o.takes.size() == 2 ? " at A " + juce::String (o.takes[0]) + ", B +" + juce::String (o.takes[1] - o.takes[0]) : juce::String())
                 + " smp, pending max " + juce::String (o.maxPending) + " | thr B >= A + 960, pending <= 1 | "
                 + ctrl (fired, "B at A + 100 ms -> taken at +" + (c.takes.size() == 2 ? juce::String (c.takes[1] - c.takes[0]) : juce::String ("?"))
                                     + " after the head parks (swapWaits " + i64 (c.swapWaits) + ")"));
    }

    //==========================================================================
    // SR1 — the SR-rebuild path parks the heads
    struct Sr1Out { std::uint64_t writesBefore = 0, writesAfter = 0; bool hashSame = false, landed = false, headAtLo = false; double cents = 1e9; };

    Sr1Out sr1Render (bool ignoreRate)
    {
        Session s (44100.0, loopParams (100.0f), kSeed, 512);
        shipping (s);
        s.hk().runIgnoreRate = ignoreRate;
        s.record = false;
        s.load (0, sineBuf (2, (int) (3.0 * 44100.0), 440.0, 44100.0, 0.5), 44100.0);
        for (int k = 1; k < 4; ++k) s.load (k, noiseBuf (2, (int) (3.0 * 44100.0), 0.3f, 71 + k), 44100.0);
        s.run (64, 32);
        const auto h0 = tapeHashes (s);
        s.p->prepareToPlay (96000.0, 512);   // the rebuild is queued (callAsync), not pumped yet
        for (int n : { 48, 52, 55, 60 })
            s.noteOn (s.pos, n);
        s.run ((juce::int64) (0.3 * 96000.0), 512);
        Sr1Out o;
        o.writesBefore = writesAll (s);
        o.hashSame = tapeHashes (s) == h0;
        {
            const double tr = s.hk().tapeRate[0];
            const double e = eg21::expectRate (*s.p, s.hk().voiceNote[0], tr, 96000.0);
            o.cents = std::abs (1200.0 * std::log2 (s.hk().voiceRate[0] / e));
        }
        const auto taken0 = s.hk().tapesTaken;
        for (int i = 0; i < 200 && s.hk().tapesTaken < taken0 + 4; ++i)
        {
            eg21::pump (10);
            s.afterBlock = [&o, taken0] (Session& ss)
            {
                if (! o.landed && ss.hk().tapesTaken > taken0)
                {
                    const auto& hd = ss.eng().getHeadForTest (0);
                    o.landed = true;
                    o.headAtLo = hd.h == hd.lo && hd.parked;
                }
            };
            s.run (512, 512);
        }
        s.afterBlock = {};
        s.run ((juce::int64) (0.3 * 96000.0), 512);
        o.writesAfter = writesAll (s) - o.writesBefore;
        return o;
    }

    void rowSR1()
    {
        const auto o = sr1Render (false);
        const auto c = sr1Render (true);
        const bool fired = c.writesBefore > 0;
        row (o.writesBefore == 0 && o.hashSame && o.cents <= 1.0 && o.landed && o.headAtLo && o.writesAfter > 0,
             "SR1 DSP-07 44.1 k tape in a 96 k session: heads parked until the rebuild lands; pitch right; resume at region lo",
             "meas writes before the land " + i64 (o.writesBefore) + ", hashes " + (o.hashSame ? "unchanged" : "CHANGED") + ", root rate err "
                 + juce::String (o.cents, 6) + " cents, landed " + (o.landed ? "yes" : "NO") + ", head at lo " + (o.headAtLo ? "yes" : "NO")
                 + ", writes after " + i64 (o.writesAfter) + " | thr 0 / <= 1 cent / > 0 | "
                 + ctrl (fired, "runIgnoreRate -> " + i64 (c.writesBefore) + " writes before the land"));
    }

    //==========================================================================
    // FA4 — 0 audio-thread allocations across the 2.4 events
    void rowFA4()
    {
       #if JUCE_MAC
        using namespace eg21;
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;
        allocTrace = true;

        P fx;
        const float types[4] { 1.0f, 2.0f, 3.0f, 0.0f };
        for (int n = 1; n <= 4; ++n)
            fx = with (fx, { { sn (n, "filter_type"), types[n - 1] }, { sn (n, "filter_cutoff"), 1500.0f }, { sn (n, "filter_res"), 80.0f },
                             { sn (n, "drive"), 50.0f }, { sn (n, "crush"), 40.0f }, { sn (n, "crush_prob"), 50.0f },
                             { sn (n, "ring_freq"), 300.0f }, { sn (n, "ring_mix"), 50.0f } });
        Session s (kFs, cat ({ base4 (4), eg22::randomised(), all4 ("density", 100.0f), all4 ("size", 200.0f), all4 ("loop_end", 10.0f),
                               { { "polyphony", 8.0f }, { "amp_attack", 5.0f }, { "amp_release", 100.0f },
                                 { "feedback", 100.0f }, { "reverb_mix", 50.0f } }, fx }));
        shipping (s);
        s.record = false;
        loadLoop4 (s, 3.0, 0.3f);
        s.arm = true;
        s.noteOn (4800, 60);
        for (int n = 40; n < 60; ++n) s.noteOn (14400, n);
        for (int n = 40; n < 60; ++n) s.noteOff (24000, n);
        for (int n = 48; n < 54; ++n) s.noteOn (28800, n);
        s.at (19200, [] (Session& ss) { ss.p->resetSlotSync (0); });
        s.at (38400, [] (Session& ss) { ss.p->resetSlotSync (-1); });
        setFreezeAt (s, 48000, true);
        setFreezeAt (s, 52800, false);
        eg22::setAt (s, 57600, "feedback", 0.0f);
        eg22::setAt (s, 62400, "feedback", 100.0f);
        s.at (67200, [] (Session& ss)
        {
            // An SR-rebuild take: slot 2 rebuilt at 44.1 k (same generation, rebuild flag).
            ogi::SlotLoader::Pristine pr;
            juce::String err;
            if (ogi::SlotLoader::pristineFromPcm (noiseBuf (2, (int) (3.0 * kFs), 0.3f, 53), kFs, "fa4", 44100.0, pr, err))
            {
                std::vector<float> ov;
                auto t = ogi::SlotLoader::buildTape (pr, 44100.0, ov, [] { return false; }, err);
                if (t != nullptr)
                {
                    const auto* cur = ss.eng().getCurrentTape (2);
                    t->slot = 2;
                    t->generation = cur != nullptr ? cur->generation : 0;
                    t->rebuild = true;
                    ss.p->getHandoff().publish (2, t.release());
                }
            }
        });
        eg22::setAt (s, 72000, "reverb_mix", 0.0f);
        eg22::setAt (s, 81600, "reverb_mix", 50.0f);   // re-enable after the 50 ms fade: clearTank
        for (int n = 48; n < 54; ++n) s.noteOff (96000, n);
        s.noteOn (4800, 72);
        s.noteOff (96000, 72);
        s.noteOff (96000, 60);
        s.run (168000, 512);
        s.arm = false;
        malloc_logger = nullptr;
        allocTrace = false;

        const int n = allocCount;
        auto& h = s.hk();
        std::uint64_t parks = 0, settles = 0, writes = 0;
        for (const auto& c : h.head) { parks += c.parks; settles += c.settles; writes += c.writes; }
        const bool exercised = h.tapesTaken > 4 && h.xfadesStarted >= 5 && parks > 0 && settles > 0 && h.voiceSteals > 0 && writes > 0;
        row (liveness == 1 && n == 0 && exercised,
             "FA4 PERF-01 0 audio-thread allocations: Reset (slot + all), freeze, fb 0 <-> 100, SR take, steals, reverb re-enable, wrap + park",
             "meas " + juce::String (n) + " allocs over " + i64 ((std::uint64_t) s.blocksDone) + " blocks (taken " + i64 (h.tapesTaken)
                 + ", xfades " + i64 (h.xfadesStarted) + ", parks " + i64 (parks) + ", settles " + i64 (settles) + ", steals " + i64 (h.voiceSteals)
                 + ", writes " + i64 (writes) + ") | thr 0 | margin " + juce::String (-n)
                 + " | " + ctrl (liveness == 1, "liveness malloc(64) armed -> " + juce::String (liveness)));
       #else
        row (false, "FA4 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // --cpu25 (PLAN-2.4 PD17): the final PERF-02 worst case + typical proxy.
    struct Cpu25 : eg23::CpuCfg { bool writesOk = true; std::uint64_t minWrites = 0; std::uint64_t xfades = 0; };

    Cpu25 measure25 (const std::function<std::unique_ptr<Session>()>& mk, const std::vector<int>& notes, double fs, int block,
                     double refNs, bool checkWrites, int resetEveryMs = 0, int reps = 7)
    {
        Cpu25 c;
        auto s = mk();
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer mb;
            for (int n : notes)
                mb.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
            buf.clear();
            s->p->processBlock (buf, mb);
        }
        std::int64_t rendered = 0, nextReset = resetEveryMs > 0 ? (std::int64_t) (resetEveryMs * 0.001 * fs) : -1;
        auto run = [&] (double seconds, bool timeIt, double& cpu, double& duty, double& live)
        {
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer mb;
            const int blocks = (int) (seconds * fs / block);
            cpu = 0; live = 0;
            const double w0 = eg22::wallNs(), t0 = eg22::threadNs();
            for (int b = 0; b < blocks; ++b)
            {
                if (nextReset >= 0 && rendered >= nextReset)
                {
                    s->p->requestReset (0);
                    nextReset += (std::int64_t) (resetEveryMs * 0.001 * fs);
                }
                buf.clear();
                mb.clear();
                const double a = timeIt ? eg22::threadNs() : 0.0;
                s->p->processBlock (buf, mb);
                if (timeIt) cpu += eg22::threadNs() - a;
                live += (double) s->hk().liveNow;
                rendered += block;
            }
            const double wall = eg22::wallNs() - w0, thr = eg22::threadNs() - t0;
            duty = wall > 0 ? thr / wall : 0;
            live /= std::max (1, blocks);
        };
        double cpu, duty, live;
        c.calB = cal::calibNs();
        run (5.0, false, cpu, duty, live);   // warm-up
        std::vector<double> pct;
        double minDuty = 1e9, lv = 0;
        c.minWrites = ~std::uint64_t (0);
        for (int i = 0; i < reps; ++i)
        {
            std::uint64_t w0[4];
            for (int k = 0; k < 4; ++k) w0[k] = s->hk().head[k].writes;
            run (10.0, true, cpu, duty, live);
            for (int k = 0; k < 4; ++k)
            {
                const auto dw = s->hk().head[k].writes - w0[k];
                c.minWrites = std::min (c.minWrites, dw);
                if (checkWrites && dw == 0) c.writesOk = false;
            }
            pct.push_back (100.0 * cpu / (10.0 * 1.0e9));
            minDuty = std::min (minDuty, duty);
            lv += live / reps;
        }
        c.calA = cal::calibNs();
        std::sort (pct.begin(), pct.end());
        c.pct = pct[pct.size() / 2]; c.mn = pct.front(); c.mx = pct.back();
        c.duty = minDuty; c.live = lv;
        c.contended = minDuty < 0.9;
        c.slowed = ! std::isfinite (refNs) || c.calB > 1.15 * refNs || c.calA / c.calB > 1.10;
        c.xfades = s->hk().xfadesStarted;
        return c;
    }

    std::unique_ptr<Session> c25Session (double fs, int block, bool streamsOn, const P& fx, const P& extra)
    {
        auto s = eg23::c22Session (fs, block, streamsOn, fx);
        eg21::setAll (*s->p, extra);
        return s;
    }

    P c25Extra()
    {
        return { { "feedback", 100.0f }, { "fb_diffusion", 100.0f }, { "fb_saturation", 20.0f }, { "reverb_mix", 100.0f },
                 { "reverb_size", 100.0f }, { "freeze", 0.0f } };
    }

    int cpu25()
    {
        constexpr int kBlock = 128;
        const double refNs = cal::readRef (gGoldenDir);
        // T2 (this phase, 2026-11-08): C23w on the UNMODIFIED 2.3 engine, the same harness recipe.
        constexpr double c23wAt23 = 26.64, c23wAt23CalB = 41211083.0;
        std::printf ("== --cpu25 (PLAN-2.4 T17): ref_ns %.0f (tests/golden/cpu-calib-2.3.txt)\n", refNs);
        std::vector<int> n16;
        for (int k = 0; k < 16; ++k) n16.push_back (40 + k);

        const auto by = measure25 ([] { return c25Session (kFs, kBlock, false, {}, {}); }, n16, kFs, kBlock, refNs, false, 0, 1);
        std::printf ("  INFO bypass (streams disabled): %s\n", eg23::cpuLine ("bypass", by, refNs).toRawUTF8());
        std::fflush (stdout);

        const auto r = measure25 ([] { return c25Session (kFs, kBlock, true, eg23::worstFx(), {}); }, n16, kFs, kBlock, refNs, false);
        std::printf ("  INFO %s\n", eg23::cpuLine ("C23w@2.4 (fb 0, reverb 0, same session)", r, refNs).toRawUTF8());
        std::fflush (stdout);

        const auto w = measure25 ([] { return c25Session (kFs, kBlock, true, eg23::worstFx(), c25Extra()); }, n16, kFs, kBlock, refNs, true);
        const bool wValid = ! w.slowed && ! w.contended && w.live >= 240.0 && w.writesOk;
        const bool wPass = wValid && w.pct <= 40.0;
        row (wPass, "C25w D25 final worst case: C23w + fb 100 % (diffusion 100) on 4 chains + reverb 100 % / size 100 <= 40 %",
             "meas " + eg23::cpuLine ("C25w", w, refNs) + ", min writes / slot / window " + i64 (w.minWrites)
                 + " | thr <= 40 %, live >= 240, writes > 0 on 4 slots, witness valid | margin " + f2 (40.0 - w.pct, 2)
                 + " points | delta vs C23w@2.4 " + f2 (w.pct - r.pct, 2) + " points" + (wValid ? "" : " | NOVERDICT / INVALID = FAIL"));
        std::fflush (stdout);

        auto tSession = [] ()
        {
            auto p = cat ({ base4 (2), { { sn (1, "density"), 40.0f }, { sn (2, "density"), 40.0f }, { sn (1, "size"), 150.0f }, { sn (2, "size"), 150.0f },
                                         { sn (1, "spray"), 10.0f }, { sn (2, "spray"), 10.0f }, { sn (1, "jitter"), 20.0f }, { sn (2, "jitter"), 20.0f },
                                         { sn (1, "stereo_spread"), 30.0f }, { sn (2, "stereo_spread"), 30.0f },
                                         { "grain_limit", 3.0f }, { "polyphony", 8.0f },
                                         { sn (1, "filter_type"), 1.0f }, { sn (1, "filter_cutoff"), 1200.0f }, { sn (1, "filter_res"), 40.0f },
                                         { sn (1, "drive"), 30.0f } } });
            auto s = std::make_unique<Session> (kFs, p, kSeed, 128);
            shipping (*s);
            for (int k = 0; k < 4; ++k)
                s->load (k, noiseBuf (2, (int) (10.0 * kFs), 0.1f, 11 + k), kFs);
            s->record = false;
            return s;
        };
        const auto t = measure25 (tSession, { 48, 52, 55, 59, 62, 64 }, kFs, kBlock, refNs, false);
        const bool tValid = ! t.slowed && ! t.contended && t.live >= 60.0 && t.live <= 90.0;
        const bool tPass = tValid && t.pct <= 12.0;
        row (tPass, "C25t D25 typical proxy (the 2.3 C23t recipe, fb 0, reverb at its default 0) <= 12 %",
             "meas " + eg23::cpuLine ("C25t", t, refNs) + " | thr <= 12 %, live in [60, 90], witness valid | margin " + f2 (12.0 - t.pct, 2) + " points"
                 + (tValid ? "" : " | INVALID / NOVERDICT = FAIL"));
        std::fflush (stdout);

        const auto rr = measure25 ([] { return c25Session (kFs, kBlock, true, eg23::worstFx(), c25Extra()); }, n16, kFs, kBlock, refNs, false, 200);
        std::printf ("  INFO C25r %s, xfadesStarted %llu, delta vs C25w %+.2f points (the dual-read fade)\n",
                     eg23::cpuLine ("C25w + requestReset (0) every 200 ms", rr, refNs).toRawUTF8(), (unsigned long long) rr.xfades, rr.pct - w.pct);
        std::fflush (stdout);

        const double rs = r.pct * refNs / r.calB, ps = c23wAt23 * refNs / c23wAt23CalB;
        std::printf ("  INFO C25s C23w@2.4 %.2f %% (scaled %.2f) vs C23w@2.3 %.2f %% (scaled %.2f, T2): %+.2f points%s\n", r.pct, rs, c23wAt23, ps, rs - ps,
                     rs - ps > 0.6 ? "  FLAG > +0.6 (fb-0 restructure cost)" : "");
        std::printf ("  INFO delta C25w - C23w@2.4 = %+.2f points (projection +2..4, C25w ~ 28-30 %%)\n", w.pct - r.pct);

        const auto w96 = measure25 ([] { return c25Session (96000.0, 128, true, eg23::worstFx(), c25Extra()); }, n16, 96000.0, 128, refNs, false);
        std::printf ("  INFO C25w96 %s\n", eg23::cpuLine ("C25w @ 96 k / 128", w96, refNs).toRawUTF8());
        std::fflush (stdout);
        return (wPass ? 0 : 1) + (tPass ? 0 : 1);
    }

    void runAll()
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("\n== Phase 2.4 engine gates (feedback, freeze, reset, reverb + knee)\n");
        section ("TB. true bypass + knee");
        if (want ("TB0")) rowTB0();
        if (want ("TB1")) rowTB1();
        if (want ("TB2")) rowTB2();
        section ("FL / CL. feedback law arm + ceiling");
        if (want ("FL1CL1")) rowsFL1CL1();
        if (want ("FL1s")) rowFL1s();
        if (want ("FL2")) rowFL2();
        if (want ("FL3")) rowFL3();
        if (want ("FL4")) rowFL4();
        if (want ("FL5")) rowFL5();
        if (want ("FL6")) rowFL6();
        if (want ("FL7")) rowFL7();
        if (want ("FL8")) rowFL8();
        if (want ("CL2")) rowCL2();
        section ("MS / RS / SR / FA. settle, Reset, SR rebuild, allocations");
        if (want ("MS1")) rowMS1();
        if (want ("RS12")) rowsRS12();
        if (want ("RS3")) rowRS3();
        if (want ("RS4")) rowRS4();
        if (want ("SR1")) rowSR1();
        if (want ("FA4")) rowFA4();
        std::printf ("\n-- Phase 2.4 engine gates done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }
}} // namespace ::eg24
