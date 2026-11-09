/*
  ==============================================================================

    O-GranulatorInstrument: render harness, Stage 2 / Phase 2.4 engine gates,
    part b (PLAN-2.4 T13): record-head clamp, click gates, reverb transitions.
    Included by main.cpp AFTER engine_gates_24.h (uses eg24 helpers).

      HC1  clamp telemetry + exact read scan (fb 80 %, levels 0-3, reverse 50 %)   ctrl headClampOff -> violations
      HC2  forced crossings counted (region 30 ms < reach)                          ctrl the HC1 region -> 0
      HC3  clampToHead boundary sweep (unit): W+1 feasible, W forced               ctrl g_a = 16 -> p0 inside the PD9 set
      CK1  freeze edge                       kappa <= 2.0x static                    ctrl freezeHardStop > 2.0x
      CK2  feedback 0 -> 100 % step          INFO (user 2026-11-08: hard control cannot fire; HANDS-ON-2.4 row 3)
      CK3  head stop / start (park, new note) INFO (user 2026-11-08: hard control cannot fire; HANDS-ON-2.4 row 4)
      CK4  Reset swap                        kappa <= 2.0x the same-motion freeze pulse  ctrl resetHardSwap > 2.0x
      RV1  reverb audible + tail (parent RV1)                                         ctrl mix 0 -> fails
      RV2  mix 50 -> 0: kappa <= 2.0x; gone within 60 ms; then bitwise mix-0          ctrl reverbHardCut > 2.0x
      RV3  mix 0 -> 50 after 2 s (cold tank): kappa <= 2.0x static-on                 ctrl kappa fed a +0.1 step > 2.0x

    kappa = LOCAL: max over 5 ms frames of max |d2 x| / the frame's RMS, max
    over L / R (user decision at S6, 2026-11-08; amends the parent
    engine_gates_24b.h:137-138 whole-window form, which level changes mask).

  ==============================================================================
*/
#pragma once

namespace { namespace eg24b
{
    using eg21::Session;
    using eg21::P;
    using eg21::with;
    using eg21::f2;
    using eg21::i64;
    using eg21::ctrl;
    using eg21::noiseBuf;
    using eg21::sineBuf;
    using eg21::kSeed;
    using eg22::base4;
    using eg22::all4;
    using eg22::cat;
    using eg22::sn;
    using eg22::shipping;
    using eg22::setAt;

    constexpr double kFs = 48000.0;
    constexpr double kRatio = 2.0;

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

    double maxCurv (const std::vector<float>& x, size_t from, size_t to)
    {
        double m = 0.0;
        for (size_t n = std::max ((size_t) 1, from); n + 1 < to && n + 1 < x.size(); ++n)
            m = std::max (m, std::abs ((double) x[n + 1] - 2.0 * (double) x[n] + (double) x[n - 1]));
        return m;
    }

    struct Out { std::vector<float> L, R; };

    /** LOCAL kappa (user decision 2026-11-08 at S6, amends PLAN-2.4 PD13):
        max over 5 ms frames in [t0, t1) of max |d2 x| / the frame's own RMS,
        max over L / R. A whole-window RMS lets a level change mask a click (a
        Reset brings the pristine copy back at ~30x the erased level, so the loud
        audio afterwards outweighs the step). Frames below −80 dBFS are skipped
        (no audio to normalise by). The 2.0x threshold and the hard controls are
        unchanged. */
    double kappa (const Out& o, double t0, double t1)
    {
        constexpr size_t kFrame = 240;           // 5 ms @ 48 k
        constexpr double kFloor = 1.0e-4;         // −80 dBFS
        auto one = [&] (const std::vector<float>& x)
        {
            const size_t a = (size_t) (t0 * kFs), b = std::min (x.size(), (size_t) (t1 * kFs));
            double best = 0.0;
            for (size_t f = a; f + kFrame <= b; f += kFrame)
            {
                double e = 0.0;
                for (size_t i = f; i < f + kFrame; ++i) e += (double) x[i] * x[i];
                const double rms = std::sqrt (e / (double) kFrame);
                if (rms < kFloor) continue;
                best = std::max (best, maxCurv (x, f, f + kFrame) / rms);
            }
            return best;
        };
        return std::max (one (o.L), one (o.R));
    }

    double diffDb (const Out& a, const Out& b, double t0, double t1)
    {
        double e = 0.0;
        const size_t x = (size_t) (t0 * kFs), y = std::min ({ a.L.size(), b.L.size(), (size_t) (t1 * kFs) });
        for (size_t i = x; i < y; ++i)
        {
            const double dl = (double) a.L[i] - b.L[i], dr = (double) a.R[i] - b.R[i];
            e += dl * dl + dr * dr;
        }
        e /= 2.0 * (double) std::max<size_t> (1, y - x);
        return e > 0.0 ? 10.0 * std::log10 (e) : -400.0;
    }

    //==========================================================================
    // HC — the spawner head clamp
    struct HcOut { std::uint64_t forced = 0, viol = 0, parkedX = 0, segs = 0, writes = 0, moves = 0; };

    HcOut hcRender (float loopEnd, float size, bool clampOff, double seconds = 10.0)
    {
        P p = cat ({ base4 (4), { { "polyphony", 16.0f }, { "grain_limit", 3.0f }, { "feedback", 80.0f } },
                     all4 ("density", 200.0f), all4 ("size", size), all4 ("reverse_prob", 50.0f), all4 ("loop_end", loopEnd),
                     { { sn (1, "pitch"), 24.0f }, { sn (2, "pitch"), -24.0f }, { sn (3, "pitch"), 0.0f }, { sn (4, "pitch"), 12.0f } } });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.record = false;
        s.hk().headReadScan = true;
        s.hk().headClampOff = clampOff;
        for (int k = 0; k < 4; ++k)
            s.load (k, noiseBuf (2, (int) (3.0 * kFs), 0.3f, 81 + k), kFs);
        for (int n : { 48, 50, 52, 55, 57, 59, 60, 62 })
            s.noteOn (0, n);
        s.run ((juce::int64) (seconds * kFs), 512);
        HcOut o;
        auto& h = s.hk();
        o.forced = h.forcedCrossings; o.viol = h.headScanViolations; o.parkedX = h.parkedSpawnCrossings;
        o.segs = h.headScanSegments; o.moves = h.clampMoves;
        o.writes = eg24::writesAll (s);
        return o;
    }

    HcOut hc1Cache;

    void rowHC1()
    {
        const float loopEnd = 100.0f * 2.0f / 3.0f;   // region 2 s of a 3 s tape
        hc1Cache = hcRender (loopEnd, 100.0f, false);
        const auto c = hcRender (loopEnd, 100.0f, true);
        const auto& o = hc1Cache;
        const bool fired = c.viol > 0;
        row (o.forced == 0 && o.viol == 0 && o.writes > 0 && o.segs > 0,
             "HC1 D22 head clamp: 0 forced crossings, 0 reads inside [-margin(level), 40] of a running head (fb 80 %, levels 0-3, reverse 50 %)",
             "meas forced " + i64 (o.forced) + ", scan violations " + i64 (o.viol) + " over " + i64 (o.segs) + " grain segments, clamp moves "
                 + i64 (o.moves) + ", writes " + i64 (o.writes) + " | INFO parkedSpawnCrossings " + i64 (o.parkedX) + " | thr 0 / 0 | "
                 + ctrl (fired, "headClampOff -> " + i64 (c.viol) + " violations"));
    }

    void rowHC2()
    {
        const auto o = hcRender (1.0f, 100.0f, false, 2.0);   // region 30 ms, grains 100 ms: reach >= W
        const bool fired = hc1Cache.forced == 0;
        row (o.forced > 0, "HC2 D22 forced crossings are counted (region 30 ms, size 100 ms: the reach >= W branch)",
             "meas forced " + i64 (o.forced) + " | thr > 0 | " + ctrl (fired, "the HC1 region (2 s) -> " + i64 (hc1Cache.forced) + " forced"));
    }

    /** p0 inside the PD9 forbidden set (g_a 40), modulo W — evaluated in
        ABSOLUTE tape coordinates exactly as clampToHead forms it (PD9); the
        relative form p0 − head rounds a one-ulp-outside result back onto the
        boundary. */
    bool insidePd9 (double p0, double head, int dir, double rate, double len, int level, double W)
    {
        double a = 0.0, b = 0.0;
        ogi::headForbidden (dir, rate, len, level, a, b);
        for (int k = -1; k <= 1; ++k)
            if (p0 >= head + a + (double) k * W && p0 <= head + b + (double) k * W)
                return true;
        return false;
    }

    void rowHC3()
    {
        const double len = 2400.0, lo = 1000.0;
        std::uint64_t cases = 0, notFeasible = 0, badResult = 0, notForced = 0, ctrlInside = 0;
        double fracSum = 0.0;
        int fracN = 0;
        for (int level = 0; level <= 4; ++level)
            for (int dir : { 1, -1 })
                for (double rate : { 0.5, 1.0, 2.0, 4.0 })
                {
                    const double reach = rate * len;
                    const double delta = std::abs ((dir * rate - 1.0) * len);
                    const double margin = (double) ogi::headBehind (level);
                    double a = 0.0, b = 0.0;
                    ogi::headForbidden (dir, rate, len, level, a, b);
                    auto sweep = [&] (double W, int gA, std::uint64_t& infeasible, std::uint64_t& bad, std::uint64_t& insideCtrl, double& frac)
                    {
                        const double hi = lo + W;
                        const double fitLo = dir > 0 ? lo : lo + reach, fitHi = dir > 0 ? hi - reach : hi;
                        const int steps = W <= 4096.0 ? (int) W : 4096;
                        int feasible = 0, total = 0;
                        for (int i = 0; i < steps; ++i)
                        {
                            const double h = lo + std::floor ((double) i * W / steps);
                            for (double off : { a, 0.0, b - 1.0 })
                            {
                                double d = h + off;
                                while (d >= hi) d -= W;
                                while (d < lo) d += W;
                                const double p0 = std::clamp (d, fitLo, fitHi);
                                const auto r = ogi::clampToHead (p0, dir, rate, len, level, fitLo, fitHi, W, true, h, gA);
                                ++total;
                                ++cases;
                                if (r.forced) { ++infeasible; continue; }
                                ++feasible;
                                const bool inFit = r.p0 >= fitLo && r.p0 <= fitHi;
                                const bool inSet = insidePd9 (r.p0, h, dir, rate, len, level, W);
                                if (gA == ogi::kHeadAhead && (! inFit || inSet)) ++bad;
                                if (gA != ogi::kHeadAhead && inSet) ++insideCtrl;
                            }
                        }
                        frac = total > 0 ? (double) feasible / total : 0.0;
                    };
                    double f = 0.0;
                    std::uint64_t inf1 = 0, bad1 = 0, dummy = 0;
                    sweep (delta + 40.0 + margin + reach + 1.0, ogi::kHeadAhead, inf1, bad1, dummy, f);
                    notFeasible += inf1;
                    badResult += bad1;
                    std::uint64_t inf2 = 0, bad2 = 0;
                    sweep (delta + 40.0 + margin, ogi::kHeadAhead, inf2, bad2, dummy, f);
                    std::uint64_t tot2 = 0;
                    {
                        const double W = delta + 40.0 + margin;
                        tot2 = (std::uint64_t) (3 * (W <= 4096.0 ? (int) W : 4096));
                    }
                    notForced += tot2 - inf2;
                    double fm = 0.0;
                    std::uint64_t i3 = 0, b3 = 0;
                    sweep (delta + 40.0 + margin + 0.5 * reach, ogi::kHeadAhead, i3, b3, dummy, fm);
                    badResult += b3;
                    fracSum += fm;
                    ++fracN;
                    std::uint64_t i4 = 0, b4 = 0, c4 = 0;
                    double fc = 0.0;
                    sweep (delta + 40.0 + margin + reach + 1.0, 16, i4, b4, c4, fc);
                    ctrlInside += c4;
                }
        const bool fired = ctrlInside > 0;
        row (notFeasible == 0 && badResult == 0 && notForced == 0,
             "HC3 clampToHead boundary sweep: W = |D| + 40 + margin + reach + 1 feasible everywhere (result outside the set, inside the fit); W = |D| + 40 + margin forced",
             "meas " + i64 (cases) + " calls (40 configs), infeasible at W+1 " + i64 (notFeasible) + ", bad results " + i64 (badResult)
                 + ", not forced at W " + i64 (notForced) + " | INFO feasible fraction at W + reach/2 " + f2 (fracSum / std::max (1, fracN), 3)
                 + " | thr 0 / 0 / 0 | " + ctrl (fired, "g_a 16 (ARCH Algorithm §4) -> " + i64 (ctrlInside) + " results inside the PD9 set"));
    }

    //==========================================================================
    // CK — click gates (D24, QUAL-01). Material: a 440 Hz sine tape (parent FZ2).
    /** PLAN-2.4 PD13 stimulus fix (the one allowed, recorded). At the default
        chain the return r tracks the copy wc, so the splice a head stop leaves,
        |r − wc|, sits far below the sine's own curvature: the hard controls read
        1.0x (diagnosed: max |d2 x| identical in the subject, hard and static
        renders). An EMPTY chain passband (fb_low_cut 2000 Hz > fb_high_cut
        1000 Hz, the PB3 / PB6 setting) makes r ≈ 0: the writer erases at λ, and
        every head stop / start / tape swap leaves the worst-case splice 0.95·wc,
        which only the 20 ms ramps / fades can smooth. */
    P ckChain() { return { { "fb_low_cut", 2000.0f }, { "fb_high_cut", 1000.0f } }; }
    /** CK3: a clean tonal return (sat 0, HP 20 Hz, LP 1 kHz) — the park splice is
        r(t_park) vs wc, and the content stays a 440 Hz tone. */
    P ckChainClean() { return { { "fb_saturation", 0.0f }, { "fb_low_cut", 20.0f }, { "fb_high_cut", 1000.0f } }; }

    Out ckRender (const P& extra, double seconds, const std::function<void (Session&)>& setup, int streams = 1, double regionPct = 66.0f)
    {
        P p = cat ({ base4 (streams), { { "polyphony", 16.0f }, { "grain_limit", 3.0f } }, ckChain(),
                     eg23::s1p ({ { "density", 20.0f }, { "size", 200.0f }, { "jitter", 20.0f }, { "spray", 100.0f },
                                  { "position", 50.0f },   // centre: spray 100 % covers the whole region (at 0 it reflects into [0, W/2])
                                  { "loop_end", (float) regionPct } }),
                     extra });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.load (0, sineBuf (2, (int) (3.0 * kFs), 440.0, kFs, 0.5), kFs);
        if (setup) setup (s);
        s.run ((juce::int64) (seconds * kFs), 512);
        return { s.L, s.R };
    }

    void ckRow (const char* id, const char* title, const Out& subj, const Out& stat, const Out& hard, double t0, double t1, const char* hardName)
    {
        const double ks = kappa (subj, t0, t1), kc = kappa (stat, t0, t1), kh = kappa (hard, t0, t1);
        const double r = ks / std::max (1e-30, kc), rh = kh / std::max (1e-30, kc);
        const bool fired = rh > kRatio;
        row (r <= kRatio && fired, juce::String (id) + " " + title,
             "meas kappa ratio " + f2 (r, 3) + " (subject " + f2 (ks, 4) + ", static " + f2 (kc, 4) + ", window " + f2 (t0, 2) + "-" + f2 (t1, 2)
                 + " s) | thr <= " + f2 (kRatio, 1) + "x | margin " + f2 (kRatio - r, 3) + " | " + ctrl (fired, juce::String (hardName) + " -> " + f2 (rh, 3) + "x"));
    }

    /** CK2 / CK3 (user decision 2026-11-08): INFO, not gates. Their hard
        controls cannot fire by construction (SUMMARY-2.4 § click gates); the
        verdict is HANDS-ON-2.4 rows 3–4. */
    void ckInfo (const char* id, const char* title, const Out& subj, const Out& stat, const Out& hard, double t0, double t1, const char* hardName)
    {
        const double ks = kappa (subj, t0, t1), kc = kappa (stat, t0, t1), kh = kappa (hard, t0, t1);
        std::printf ("  INFO %s %s: kappa ratio %.3f (subject %.4f, static %.4f, window %.2f-%.2f s); %s -> %.3fx (not a gate: HANDS-ON-2.4)\n",
                     id, title, ks / std::max (1e-30, kc), ks, kc, t0, t1, hardName, kh / std::max (1e-30, kc));
    }

    void rowCK1()
    {
        const P fb { { "feedback", 100.0f } };
        auto notes = [] (Session& s) { for (int n : { 48, 55, 60 }) s.noteOn (0, n); };
        // Freeze at 1 s: the head stops mid-lap (region 2 s), erased behind / pristine ahead.
        const auto subj = ckRender (fb, 2.0, [&] (Session& s) { notes (s); eg24::setFreezeAt (s, 48000, true); });
        const auto stat = ckRender (fb, 2.0, notes);
        const auto hard = ckRender (fb, 2.0, [&] (Session& s) { notes (s); s.hk().freezeHardStop = true; eg24::setFreezeAt (s, 48000, true); });
        ckRow ("CK1", "QUAL-01 / D24 freeze edge (fb 100 %, heads stop over 20 ms) vs the same render unfrozen", subj, stat, hard, 1.0, 2.0, "freezeHardStop");
    }

    void rowCK2()
    {
        auto notes = [] (Session& s) { for (int n : { 48, 55, 60 }) s.noteOn (0, n); };
        // A parked head resumes where it parked. Freeze parks it mid-region at
        // 0.5 s (run ramp); fb drops to 0 while frozen (no writes); unfreeze at
        // 0.8 s (the head stays parked by fb 0); the 0 → 100 step at 1 s is the
        // only λ edge that writes, starting where grains read.
        auto pulse = [&] (Session& s, bool hardStep)
        {
            notes (s);
            s.hk().lambdaRampZero = hardStep;
            eg24::setFreezeAt (s, 24000, true);
            setAt (s, 28800, "feedback", 0.0f);
            eg24::setFreezeAt (s, 38400, false);
            setAt (s, 48000, "feedback", 100.0f);
        };
        const auto subj = ckRender ({ { "feedback", 100.0f } }, 2.0, [&] (Session& s) { pulse (s, false); });
        const auto stat = ckRender ({ { "feedback", 100.0f } }, 2.0, notes);
        const auto hard = ckRender ({ { "feedback", 100.0f } }, 2.0, [&] (Session& s) { pulse (s, true); });
        ckInfo ("CK2", "QUAL-01 / D24 feedback 0 -> 100 % at a tick (20 ms lambda ramp) vs fb 100 % held", subj, stat, hard, 1.0, 2.0, "lambdaRampZero");
    }

    void rowCK3()
    {
        // Notes off at 1 s → the return decays → the heads park (~0.5 s); a new
        // note at 2.5 s reads across the park point for 4 s.
        auto script = [] (Session& s)
        {
            for (int n : { 48, 55, 60 }) { s.noteOn (0, n); s.noteOff (48000, n); }
            s.noteOn (120000, 48);
        };
        const auto subj = ckRender (cat ({ { { "feedback", 100.0f } }, ckChainClean() }), 4.5, script);
        const auto stat = ckRender (cat ({ { { "feedback", 0.0f } }, ckChainClean() }), 4.5, script);
        const auto hard = ckRender (cat ({ { { "feedback", 100.0f } }, ckChainClean() }), 4.5, [&] (Session& s) { script (s); s.hk().runRampZero = true; });
        ckInfo ("CK3", "QUAL-01 / D24 head stop (park) / start: grains sweep the park point after a new note vs fb 0", subj, stat, hard, 2.5, 4.5, "runRampZero");
    }

    void rowCK4()
    {
        // Reset at 1.5 s after 1.5 s at fb 100 %; the static control is a freeze pulse
        // with the same head motion (park → take → 20 ms fade → resume) and no swap.
        auto notes = [] (Session& s) { for (int n : { 48, 55, 60 }) s.noteOn (0, n); };
        const juce::int64 tR = 72000;   // 1.5 s
        const auto subj = ckRender ({ { "feedback", 100.0f } }, 2.5, [&] (Session& s) { notes (s); s.at (tR, [] (Session& ss) { ss.p->resetSlotSync (0); }); });
        const auto stat = ckRender ({ { "feedback", 100.0f } }, 2.5, [&] (Session& s)
        {
            notes (s);
            eg24::setFreezeAt (s, tR, true);
            eg24::setFreezeAt (s, tR + 32 * 32 + 960 + 64, false);   // park (31 cells) + take + the 20 ms fade
        });
        const auto hard = ckRender ({ { "feedback", 100.0f } }, 2.5, [&] (Session& s)
        {
            notes (s);
            s.hk().resetHardSwap = true;
            s.at (tR, [] (Session& ss) { ss.p->resetSlotSync (0); });
        });
        ckRow ("CK4", "QUAL-01 / D24 Reset swap (20 ms dual-read fade) vs a same-motion freeze pulse", subj, stat, hard, 1.5, 2.5, "resetHardSwap");
    }

    //==========================================================================
    // RV — reverb (parent RV1-RV3)
    Out rvRender (float mix0, double seconds, bool noise, std::vector<std::pair<juce::int64, float>> edges = {}, bool hardCut = false,
                  float size = 50.0f, double noteOffS = -1.0)
    {
        P p = cat ({ base4 (1), { { "reverb_mix", mix0 }, { "reverb_size", size } },
                     eg23::s1p ({ { "density", 50.0f }, { "size", 100.0f }, { "spray", 0.0f }, { "position", 0.0f } }) });
        Session s (kFs, p, kSeed, 512);
        shipping (s);
        s.hk().reverbHardCut = hardCut;
        s.load (0, noise ? noiseBuf (2, (int) (4.0 * kFs), 0.3f, 91) : sineBuf (2, (int) (4.0 * kFs), 440.0, kFs, 0.5), kFs);
        s.noteOn (0, 48);
        if (noteOffS > 0.0) s.noteOff ((juce::int64) (noteOffS * kFs), 48);
        for (auto& e : edges) setAt (s, e.first, "reverb_mix", e.second);
        s.run ((juce::int64) (seconds * kFs), 512);
        return { s.L, s.R };
    }

    void rowRV1()
    {
        const auto on = rvRender (50.0f, 3.5, true, {}, false, 0.0f, 2.0), off = rvRender (0.0f, 3.5, true, {}, false, 0.0f, 2.0);
        const double during = diffDb (on, off, 0.5, 2.0);
        // Tail: notes off at 2.0 s (release 50 ms, grains 100 ms): fit the decay 2.2-3.5 s.
        std::vector<double> t, db;
        for (double a = 2.2; a + 0.05 <= 3.5; a += 0.05)
        {
            double e = 0.0;
            const size_t x = (size_t) (a * kFs), y = (size_t) ((a + 0.05) * kFs);
            for (size_t i = x; i < y; ++i) e += (double) on.L[i] * on.L[i] + (double) on.R[i] * on.R[i];
            e /= 2.0 * (double) (y - x);
            const double d = e > 0.0 ? 10.0 * std::log10 (e) : -400.0;
            if (d > -120.0) { t.push_back (a); db.push_back (d); }
        }
        double slope = 0.0;
        if (t.size() >= 3)
        {
            double mt = 0, md = 0;
            for (size_t i = 0; i < t.size(); ++i) { mt += t[i]; md += db[i]; }
            mt /= (double) t.size(); md /= (double) t.size();
            double num = 0, den = 0;
            for (size_t i = 0; i < t.size(); ++i) { num += (t[i] - mt) * (db[i] - md); den += (t[i] - mt) * (t[i] - mt); }
            slope = den > 0 ? num / den : 0.0;
        }
        const double rt60 = slope < 0.0 ? 60.0 / -slope : 1.0e9;
        const auto c = rvRender (0.0f, 3.5, true, {}, false, 0.0f, 2.0);
        const bool fired = ! (diffDb (c, off, 0.5, 2.0) > -40.0);
        row (during > -40.0 && rt60 > 0.2 && t.size() >= 3, "RV1 DSP-05 reverb_mix 50 %, size 0: wet - (mix 0) > -40 dBFS during input; tail RT60 > 0.2 s",
             "meas diff " + f2 (during, 1) + " dBFS, tail " + f2 (slope, 1) + " dB/s -> RT60 " + f2 (rt60, 3) + " s (" + juce::String ((int) t.size())
                 + " frames) | thr > -40 dBFS, > 0.2 s | " + ctrl (fired, "reverb_mix 0 -> diff " + f2 (diffDb (c, off, 0.5, 2.0), 1) + " dBFS (fails > -40)"));
    }

    void rowRV2()
    {
        const juce::int64 t = 96000;
        const auto s  = rvRender (50.0f, 4.0, false, { { t, 0.0f } });
        const auto st = rvRender (50.0f, 4.0, false);
        const auto off = rvRender (0.0f, 4.0, false);
        const auto hc = rvRender (50.0f, 4.0, false, { { t, 0.0f } }, true);
        const double k = kappa (s, 1.5, 2.5) / std::max (1e-30, kappa (st, 1.5, 2.5));
        const double kh = kappa (hc, 1.5, 2.5) / std::max (1e-30, kappa (st, 1.5, 2.5));
        const double after = diffDb (s, off, 2.0 + 0.060, 2.5);
        // After the fade the unit stops: the output is bitwise the mix-0 render (from 2.1 s).
        int diffs = 0;
        for (size_t i = (size_t) (2.1 * kFs); i < s.L.size() && i < off.L.size(); ++i)
            diffs += (eg21::sameF (s.L[i], off.L[i]) && eg21::sameF (s.R[i], off.R[i])) ? 0 : 1;
        const bool fired = kh > kRatio;
        row (k <= kRatio && after < -90.0 && diffs == 0 && fired,
             "RV2 DSP-05 reverb_mix 50 -> 0 at a tick: kappa <= 2.0x static; gone within 60 ms; then bitwise the mix-0 render",
             "meas kappa ratio " + f2 (k, 3) + ", residual " + f2 (after, 1) + " dBFS after 60 ms, " + juce::String (diffs) + " samples != mix-0 after 2.1 s"
                 + " | thr <= 2.0x, < -90 dBFS, 0 | " + ctrl (fired, "reverbHardCut -> " + f2 (kh, 3) + "x"));
    }

    void rowRV3()
    {
        const juce::int64 t = 96000;
        const auto s  = rvRender (0.0f, 4.0, false, { { t, 50.0f } });
        const auto st = rvRender (50.0f, 4.0, false);
        const double kc = kappa (st, 1.5, 2.5);
        const double k = kappa (s, 1.5, 2.5) / std::max (1e-30, kc);
        Out stepped = s;
        stepped.L[(size_t) t] += 0.1f;
        stepped.R[(size_t) t] += 0.1f;
        const double kh = kappa (stepped, 1.5, 2.5) / std::max (1e-30, kc);
        const bool fired = kh > kRatio;
        row (k <= kRatio && fired, "RV3 DSP-05 reverb_mix 0 -> 50 after 2 s off (cold tank): kappa <= 2.0x static-on",
             "meas kappa ratio " + f2 (k, 3) + " | thr <= 2.0x | margin " + f2 (kRatio - k, 3)
                 + " | " + ctrl (fired, "the same render + a 1-sample +0.1 step -> " + f2 (kh, 3) + "x"));
    }

    void runAll()
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("\n== Phase 2.4 engine gates, part b (head clamp, clicks, reverb)\n");
        section ("HC. record-head clamp");
        if (want ("HC1")) rowHC1();
        if (want ("HC2")) rowHC2();
        if (want ("HC3")) rowHC3();
        section ("CK. click gates (D24)");
        if (want ("CK1")) rowCK1();
        if (want ("CK2")) rowCK2();
        if (want ("CK3")) rowCK3();
        if (want ("CK4")) rowCK4();
        section ("RV. reverb (DSP-05)");
        if (want ("RV1")) rowRV1();
        if (want ("RV2")) rowRV2();
        if (want ("RV3")) rowRV3();
        std::printf ("\n-- Phase 2.4 engine gates part b done (%.1f s)\n", (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001);
    }
}} // namespace ::eg24b
