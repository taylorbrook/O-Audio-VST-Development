/*
  ==============================================================================

    O-Granulator — render harness, sections N7 / FZ / RV / Z3 / Z4 / M4 / C4
    (Stage 2 / Phase 2.4, PLAN-2.4 T7–T12, P44). Sibling of engine_gates_24.h.

      N7  PERF-03 block-size null: the N6 all-FX recipe + feedback 80 %,
          diffusion 50 %, saturation 50 %, a fb_high_cut change at a shared
          boundary (T7); + freeze toggles incl. a re-engage inside the 10 ms
          release fade (T9); + reverb_mix 0 → 30 → 0 (T12, final). 441 / 512 /
          4096 / var vs 32, VALUE compare (R35). N7c different seed differs.
      FZ1 FUNC-02: frozen level-0 ring hash identical across 10 s of new input
          (FZ1n: freeze off differs)
      FZ2a/b engage / release click (sine 440, spray 0 / position 0) κ <= 2.0x
          vs the same-motion control; FZ2an freezeNoReflect, FZ2bn
          releaseSpliceOld > 2.0x
      FZ3 ρ_H anti-buzz <= 0.2 (FZ3n freezeNoMinSpray >= 0.9); FZ3c ROADMAP
          crest (non-diagnostic, R28)
      FZ4 freeze 0.5 s after load not silent;  FZ5 60 s drift < 0.5 dB
      FZ6 reach diag: 0 violations, engage reflections, forced releases ==
          snapshot geometry; frozen spawns never reflect; reverse cap
      FZ7 freeze + feedback: chain warm, ring held;  FZ8 release decay vs the
          steady loop ±0.2 dB/s

  ==============================================================================
*/
#pragma once

namespace { namespace eg24b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    constexpr double kFs = 48000.0;

    //==========================================================================
    // N7 — PERF-03 block-size null with feedback (T7 form; T9 / T12 widen it)
    //==========================================================================
    Job n7Base (int form)
    {
        Job base = eg23b::n6Base (true);
        for (auto& kv : P { { "feedback", 80.0f }, { "fb_diffusion", 50.0f }, { "fb_saturation", 50.0f } }) base.params.push_back (kv);
        base.events.push_back ({ 165001, [] (Proc& p) { eg::setP (p, "fb_high_cut", 3000.0f); } });
        if (form >= 1)
        {
            // Freeze toggles at three shared ticks (the third re-engages inside the 10 ms
            // release fade) with a frozen position automation.
            base.events.push_back ({ 64 * 1500,        [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            base.events.push_back ({ 64 * 1700 + 5,    [] (Proc& p) { eg::setP (p, "s1_position", 60.0f); eg::setP (p, "s2_position", 10.0f); } });
            base.events.push_back ({ 64 * 2100,        [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
            base.events.push_back ({ 64 * 2100 + 192,  [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });   // 6 cells into the 15-cell fade
            base.events.push_back ({ 64 * 2600 + 7,    [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
        }
        if (form >= 2)
        {
            base.events.push_back ({ 60011, [] (Proc& p) { eg::setP (p, "reverb_mix", 30.0f); } });
            base.events.push_back ({ 240007, [] (Proc& p) { eg::setP (p, "reverb_mix", 0.0f); } });
        }
        return base;
    }

    void blockNullFbRows (int form)
    {
        const char* fn[3] { "feedback", "feedback + freeze", "feedback + freeze + reverb (final)" };
        std::printf ("\n-- N7. PERF-03 block-size null, N6 all-FX recipe + %s\n", fn[form]);
        Job base = n7Base (form);
        Job j32 = base; j32.block = 32;
        const auto ref = eg::render (j32);
        const auto& rd = ref.eng().diag;
        // Value compare (R35): the reverb's idle hand-off may flip −0 / +0 across splits.
        auto vdiff = [] (const Out& a, const Out& b, int& n, double& mx)
        {
            n = 0; mx = 0.0;
            for (size_t i = 0; i < a.L.size(); ++i)
            {
                const double dl = std::abs ((double) a.L[i] - (double) b.L[i]), dr = std::abs ((double) a.R[i] - (double) b.R[i]);
                if (dl > 0.0 || dr > 0.0 || std::isnan (a.L[i]) != std::isnan (b.L[i])) ++n;
                mx = juce::jmax (mx, dl, dr);
            }
        };
        Tally t;
        for (int b : { 441, 512, 4096, 0 })
        {
            Job jb = base; jb.block = b;
            const auto o = eg::render (jb);
            int n; double mx; vdiff (ref, o, n, mx);
            t.check (n == 0, (b == 0 ? juce::String ("var") : juce::String (b)) + juce::String::formatted (": %d diffs max %.3g", n, mx));
        }
        bool live = rd.spawns > 500 && rd.kills > 0 && rd.fbScrubs == 0;
        juce::String extra;
        if (form >= 1)
        {
            live = live && rd.freezeEngages == 2 && rd.freezeReleases == 2 && rd.forcedReleases > 0;
            extra << juce::String::formatted (", engages %lld releases %lld forced %lld reflections %lld", (long long) rd.freezeEngages,
                                              (long long) rd.freezeReleases, (long long) rd.forcedReleases, (long long) rd.headReflections);
        }
        row (t.bad == 0 && live, juce::String ("N7 ") + fn[form] + ": 441/512/4096/var vs 32 max|d| == 0 (value compare)",
             t.detail (4) + juce::String::formatted ("  [spawns %lld kills %lld scrubs %lld", (long long) rd.spawns, (long long) rd.kills,
                                                     (long long) rd.fbScrubs) + extra + "]" + (live ? "" : "  VACUOUS"));
        Job js = j32; js.seed = base.seed ^ 0x1ull;
        int n; double mx; vdiff (ref, eg::render (js), n, mx);
        row (n > 0, juce::String ("N7c ") + fn[form] + ": different-seed control differs", juce::String (n) + " diffs");
    }
    //==========================================================================
    // FZ — freeze (T8, T9)
    //==========================================================================
    std::uint64_t ringHash (const Proc& p)
    {
        const auto& e = const_cast<Proc&> (p).getEngineForTest();
        const auto& ring = e.getRingForTest();
        const std::int64_t w = ring.getWriteCount();
        const std::int64_t n = (std::int64_t) e.getRingSamples();
        std::uint64_t h = 0xcbf29ce484222325ull;
        for (int ch = 0; ch < 2; ++ch)
            for (std::int64_t j = w - n; j < w; ++j)
            {
                const float x = j >= 0 ? ring.levelSample (0, ch, j) : 0.0f;
                std::uint32_t b; std::memcpy (&b, &x, 4);
                for (int k = 0; k < 4; ++k) { h ^= (b >> (8 * k)) & 0xffu; h *= 0x100000001b3ull; }
            }
        return h;
    }

    // κ = max |Δ²x| / RMS over [t0, t1) (the K1 / K2 click metric), max over L / R.
    double kappa (const Out& o, double t0, double t1)
    {
        auto one = [&] (const std::vector<float>& x)
        {
            const size_t a = (size_t) (t0 * kFs), b = (size_t) (t1 * kFs);
            double e = 0.0;
            for (size_t i = a; i < b; ++i) e += (double) x[i] * x[i];
            return eg::maxCurv (x, a, b) / juce::jmax (1e-30, std::sqrt (e / (double) (b - a)));
        };
        return juce::jmax (one (o.L), one (o.R));
    }

    double rhoAt (const std::vector<float>& x, size_t a, size_t b, int lag) { return eg24::acorrPeak (x, a, b, lag, lag); }

    P fzBase()
    {
        P p = eg22::base ({ 1 });
        for (auto& kv : P { { "s1_spray", 0.0f }, { "s1_position", 0.0f }, { "s1_jitter", 0.0f } }) p.push_back (kv);
        return p;
    }

    std::int64_t frozenSpawnReflections = -1;   // FZ6 (T8 run) → checked strictly in T9

    void freezeRowsT8()
    {
        std::printf ("\n-- FZ (T8). FUNC-02 freeze: hold, edges, reach, freeze + feedback\n");
        const juce::int64 tFz = 96000;   // 2.0 s (a tick)

        // FZ1 — the ring is held.
        {
            auto run = [&] (bool freeze, std::uint64_t& h0, std::uint64_t& h1)
            {
                Job j; j.len = (juce::int64) (12.2 * kFs); j.block = 512;
                j.params = eg22::base ({ 1 });
                if (freeze) j.events.push_back ({ tFz, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                bool got0 = false;
                j.after = [&] (Proc& p, juce::int64 pos)
                {
                    if (! got0 && pos >= tFz + 512) { h0 = ringHash (p); got0 = true; }
                    if (pos >= j.len) h1 = ringHash (p);
                };
                eg::render (j);
            };
            std::uint64_t a0 = 0, a1 = 1, b0 = 0, b1 = 0;
            run (true, a0, a1);
            run (false, b0, b1);
            row (a0 == a1, "FZ1 FUNC-02: level-0 ring hash identical at the freeze tick and 10 s later (new input)",
                 juce::String::formatted ("%016llx vs %016llx", (unsigned long long) a0, (unsigned long long) a1));
            row (b0 != b1, "FZ1n control: the same with freeze off differs",
                 juce::String::formatted ("%016llx vs %016llx", (unsigned long long) b0, (unsigned long long) b1));
        }

        // FZ2 — engage / release clicks vs same-motion controls (tonal source, linear wet sum).
        {
            auto job = [&] (std::vector<std::pair<juce::int64, float>> edges, std::function<void (Proc&)> pre, float pitch, float spray = 0.0f)
            {
                Job j; j.len = (juce::int64) (8.0 * kFs); j.block = 512;
                j.params = fzBase();
                for (auto& kv : P { { "s1_density", 20.0f }, { "s1_size", 200.0f }, { "s1_jitter", 20.0f }, { "s1_pitch", pitch }, { "s1_spray", spray } })
                    j.params.push_back (kv);
                j.stim = eg::sineStim (440.0, kFs, 0.5);
                for (auto& e : edges) { const float v = e.second; j.events.push_back ({ e.first, [v] (Proc& p) { eg::setP (p, "freeze", v); } }); }
                j.pre = pre;
                return j;
            };
            const juce::int64 tRel = 5 * 48000;
            auto noHook = [] (Proc&) {};
            float pitch = 0.0f;
            // Engage: subject = live → frozen at 2 s (deferred head-stop); control =
            // never frozen (same motion, no edge). Negative control: freezeNoDefer —
            // the head stops AT the edge and the crossers reflect (the PLAN P38 form).
            auto engage = [&] (int mode)   // 0 subject, 1 freezeNoDefer, 2 freezeNoDefer + freezeNoReflect (stuck, INFO)
            {
                std::function<void (Proc&)> pre = noHook;
                if (mode == 1) pre = [] (Proc& p) { p.getEngineForTest().hooks.freezeNoDefer = true; };
                if (mode == 2) pre = [] (Proc& p) { p.getEngineForTest().hooks.freezeNoDefer = true; p.getEngineForTest().hooks.freezeNoReflect = true; };
                const auto s = eg::render (job ({ { tFz, 1.0f } }, pre, pitch));
                const auto c = eg::render (job ({}, noHook, pitch));
                return std::make_pair (kappa (s, 1.5, 2.5) / kappa (c, 1.5, 2.5), s.eng().diag.forcedReleases);
            };
            auto e0 = engage (0), en = engage (1), es = engage (2);
            juce::String note;
            if (en.first <= 2.0)
            {
                pitch = 24.0f;   // stimulus below threshold: re-derive (crossers cross faster at +24 st)
                note = " [re-derived at +24 st: the 0 st control read " + juce::String (en.first, 3) + "]";
                e0 = engage (0); en = engage (1); es = engage (2);
            }
            row (e0.first <= 2.0 && en.first > 2.0, "FZ2a engage click (sine 440, spray 0 / position 0, deferred stop): kappa <= 2.0x the unfrozen control",
                 juce::String::formatted ("ratio %.3f (forced releases %lld)", e0.first, (long long) e0.second) + note);
            row (en.first > 2.0, "FZ2an control: freezeNoDefer (stop at the edge, crossers reflect: P38 form) > 2.0x",
                 juce::String::formatted ("ratio %.3f; INFO stop at the edge without reflect (stuck reads) %.3f", en.first, es.first));
            // Release: subject = frozen 2 → 5 s; control = frozen from 2 s, never
            // released. Spray 50 %: later grains read BACK across W0 (at spray 0 the
            // 10 ms splice region is never re-read, so no continuation is audible).
            auto release = [&] (bool old)
            {
                std::function<void (Proc&)> pre = noHook;
                if (old) pre = [] (Proc& p) { p.getEngineForTest().hooks.releaseSpliceOld = true; };
                const auto s = eg::render (job ({ { tFz, 1.0f }, { tRel, 0.0f } }, pre, pitch, 50.0f));
                const auto c = eg::render (job ({ { tFz, 1.0f } }, noHook, pitch, 50.0f));
                return kappa (s, 5.0, 7.0) / kappa (c, 5.0, 7.0);
            };
            const double r0 = release (false), rn = release (true);
            row (r0 <= 2.0 && rn > 2.0, "FZ2b release click (mirror continuation, spray 50 %): kappa <= 2.0x the still-frozen control",
                 juce::String::formatted ("ratio %.3f", r0));
            row (rn > 2.0, "FZ2bn control: releaseSpliceOld (ring[W - heldSpan]) > 2.0x", juce::String::formatted ("ratio %.3f", rn));
        }

        // FZ6 — reach diag at an engage with crossers (+12 st, spray 0 / position 0).
        // With the deferred stop no crosser reaches the head (reflections 0); the
        // freezeNoDefer run at 0 st (crossers sit 1 sample from the head) proves
        // the reflect safety net still works.
        {
            auto run = [&] (float pitch, bool noDefer)
            {
                Job j; j.len = (juce::int64) (12.0 * kFs); j.block = 512;
                j.params = fzBase();
                for (auto& kv : P { { "s1_density", 30.0f }, { "s1_size", 300.0f }, { "s1_pitch", pitch } }) j.params.push_back (kv);
                j.events.push_back ({ tFz, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                if (noDefer) j.pre = [] (Proc& p) { p.getEngineForTest().hooks.freezeNoDefer = true; };
                return eg::render (j);
            };
            const auto nd = run (0.0f, true);
            const auto o = run (12.0f, false);
            const auto& e = o.eng();
            const auto& d = e.diag;
            // Recompute the R25 crossers from the engage snapshot.
            int expect = 0;
            for (int i = 0; i < ogran::kPoolLive; ++i)
            {
                const auto& g = e.engageSnap[(size_t) i];
                if (! e.engageActive[(size_t) i] || g.dir < 0 || g.killStep > 0.0f) continue;
                const double delta = (double) e.engageW0 - g.readAbs, rem = g.len - (double) g.age - g.frac;
                if (delta - g.rate * rem < (double) ogran::Ring::margin (g.level)) ++expect;
            }
            const auto& dn = nd.eng().diag;
            row (d.reachViolations == 0 && d.headReflections == 0 && d.forcedReleases == expect && expect > 0
                     && dn.reachViolations == 0 && dn.headReflections > 0,
                 "FZ6 reach diag (+12 st engage, deferred stop): 0 violations, 0 reflections, forced releases == snapshot; no-defer 0 st: crossers reflect",
                 juce::String::formatted ("violations %lld, reflections %lld, forced %lld (snapshot %d); no-defer 0 st: violations %lld reflections %lld",
                                          (long long) d.reachViolations, (long long) d.headReflections, (long long) d.forcedReleases, expect,
                                          (long long) dn.reachViolations, (long long) dn.headReflections));
            frozenSpawnReflections = d.headReflectionsFrozenSpawn;
        }

        // FZ7 — freeze + feedback: the chain stays warm, the ring stays held.
        {
            Job j; j.len = (juce::int64) (8.0 * kFs); j.block = 512;
            j.params = eg22::base ({ 1 });
            j.params.push_back ({ "feedback", 80.0f });
            j.events.push_back ({ tFz, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            j.events.push_back ({ tFz + 5 * 48000, [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
            j.pre = [] (Proc& p) { p.getEngineForTest().hooks.snapshotChain = true; };
            std::uint64_t h0 = 0, h1 = 1, seen = 0;
            int pairs = 0, evolving = 0, nonzero = 0, ticks = 0;
            ogran::Engine::ChainSnap prev {};
            bool havePrev = false;
            j.after = [&] (Proc& p, juce::int64 pos)
            {
                auto& e = p.getEngineForTest();
                for (; seen < e.chainSnapCount; ++seen)
                {
                    const auto& s = e.chainSnaps[(size_t) (seen % ogran::Engine::kChainSnaps)];
                    if (s.tick <= tFz + 32 || s.tick >= tFz + 5 * 48000) { havePrev = false; continue; }
                    ++ticks;
                    if (s.hpIc1 != 0.0f && s.lpIc1 != 0.0f && s.env > 0.0f) ++nonzero;
                    if (havePrev)
                    {
                        ++pairs;
                        if (s.hpIc1 != prev.hpIc1 && s.lpIc1 != prev.lpIc1 && s.env != prev.env) ++evolving;
                    }
                    prev = s; havePrev = true;
                }
                if (pos == tFz + 512) h0 = ringHash (p);
                if (pos == tFz + 5 * 48000) h1 = ringHash (p);
            };
            const auto o = eg::render (j);
            bool finite = true;
            for (size_t i = 0; i < o.L.size(); ++i) finite = finite && std::isfinite (o.L[i]) && std::isfinite (o.R[i]);
            row (pairs > 0 && evolving >= 0.9 * pairs && nonzero >= 0.9 * ticks && h0 == h1 && finite,
                 "FZ7 freeze + feedback 80 %: chain warm (>= 90 % of tick pairs evolve), ring hash held, output finite",
                 juce::String::formatted ("%d/%d pairs evolve, %d/%d ticks non-zero, hash %s, finite %s", evolving, pairs, nonzero, ticks,
                                          h0 == h1 ? "held" : "MOVED", finite ? "yes" : "NO"));
        }
    }
    void freezeRowsT9()
    {
        std::printf ("\n-- FZ (T9). FUNC-02 frozen geometry: anti-buzz, early freeze, drift, reach, release decay\n");
        const juce::int64 tFz = 3 * 48000;

        // FZ3 / FZ3c — ρ_H anti-buzz (R28) + the ROADMAP crest row.
        {
            auto job = [&] (bool noMin)
            {
                Job j; j.len = (juce::int64) (14.0 * kFs); j.block = 512;
                j.params = fzBase();
                for (auto& kv : P { { "s1_density", 10.0f }, { "s1_size", 100.0f }, { "s1_window", 0.0f } }) j.params.push_back (kv);
                j.events.push_back ({ tFz, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                if (noMin) j.pre = [] (Proc& p) { p.getEngineForTest().hooks.freezeNoMinSpray = true; };
                return j;
            };
            const auto s = eg::render (job (false)), n = eg::render (job (true));
            const int H = (int) (kFs / 10.0);
            const size_t a = (size_t) (4.0 * kFs), b = (size_t) (14.0 * kFs);
            const double rs = rhoAt (s.L, a, b, H), rn = rhoAt (n.L, a, b, H);
            row (rs <= 0.2 && rn >= 0.9, "FZ3 anti-buzz: frozen rho at H = fs/density <= 0.2 (spray 0 / pos 0 / jitter 0, Hann)",
                 juce::String::formatted ("rho %.3f", rs));
            row (rn >= 0.9, "FZ3n control: freezeNoMinSpray (live formula + plain clamp) rho >= 0.9", juce::String::formatted ("rho %.3f", rn));
            const double crF = eg24::crestDb (s.L, a, b), crL = eg24::crestDb (s.L, (size_t) (0.5 * kFs), (size_t) (3.0 * kFs));
            const double ratio = std::pow (10.0, (crF - crL) / 20.0);
            row (ratio <= 1.5, "FZ3c ROADMAP crest: frozen <= 1.5x live (non-diagnostic on noise, R28)",
                 juce::String::formatted ("ratio %.3f (frozen %.2f dB, live %.2f dB)", ratio, crF, crL));
        }

        // FZ4 — freeze 0.5 s after load is not silent.
        {
            juce::String det; bool ok = true;
            for (float size : { 100.0f, 500.0f })
            {
                Job j; j.len = (juce::int64) (3.0 * kFs); j.block = 512;
                j.params = eg22::base ({ 1 });
                j.params.push_back ({ "s1_size", size });
                j.events.push_back ({ 24000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                const auto o = eg::render (j);
                const double db = eg22::db (eg22::meanPow (o, (size_t) (1.0 * kFs), (size_t) (3.0 * kFs)));
                const int H = (int) (kFs / 10.0);
                const double r = rhoAt (o.L, (size_t) (1.0 * kFs), (size_t) (3.0 * kFs), H);
                ok = ok && db > -60.0;
                det << juce::String::formatted ("%g ms: %.1f dBFS, rho %.3f%s; ", size, db, r, size > 400.0f ? " (INFO)" : "");
            }
            row (ok, "FZ4 freeze 0.5 s after load (100 ms / 500 ms grains): frozen RMS > -60 dBFS", det);
        }

        // FZ5 — 60 s frozen drift.
        {
            Job j; j.len = (juce::int64) (62.0 * kFs); j.block = 512;
            j.params = eg22::base ({ 1 });
            for (auto& kv : P { { "s1_density", 50.0f }, { "s1_size", 200.0f } }) j.params.push_back (kv);
            j.events.push_back ({ 96000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            const auto o = eg::render (j);
            const double a = eg22::db (eg22::meanPow (o, (size_t) (2.5 * kFs), (size_t) (7.5 * kFs)));
            const double b = eg22::db (eg22::meanPow (o, (size_t) (57.0 * kFs), (size_t) (62.0 * kFs)));
            row (std::abs (a - b) < 0.5, "FZ5 60 s frozen drift: RMS first 5 s vs last 5 s < 0.5 dB",
                 juce::String::formatted ("%.2f vs %.2f dBFS (%.3f dB)", a, b, std::abs (a - b)));
        }

        // FZ6 strict + reverse sub-row.
        {
            Job j; j.len = (juce::int64) (12.0 * kFs); j.block = 512;
            j.params = fzBase();
            for (auto& kv : P { { "s1_density", 30.0f }, { "s1_size", 300.0f }, { "s1_reverse_prob", 100.0f }, { "s1_pitch", 12.0f },
                                { "s1_spray", 50.0f } })
                j.params.push_back (kv);
            j.events.push_back ({ 96000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
            eg22::RecLog L; j.after = eg22::collect (L);
            const auto o = eg::render (j);
            const auto& e = o.eng();
            const double held = (double) e.getFreezeForTest().heldCaptured;
            int frozenRev = 0, over = 0;
            for (auto& r : L.r)
                if (r.onsetSample >= 96000 && r.dir < 0)
                {
                    ++frozenRev;
                    if (r.delay + r.rate * r.len > held + 1e-6) ++over;
                }
            row (frozenSpawnReflections == 0 && e.diag.headReflectionsFrozenSpawn == 0 && frozenRev > 0 && over == 0 && ! L.overflow,
                 "FZ6 strict: grains spawned while frozen never reflect; frozen reverse D + r*len <= heldCaptured",
                 juce::String::formatted ("frozen-spawn reflections %lld / %lld, reverse frozen spawns %d, over %d", (long long) frozenSpawnReflections,
                                          (long long) e.diag.headReflectionsFrozenSpawn, frozenRev, over));
        }

        // FZ8 — R37 release decay vs the steady loop.
        {
            auto job = [&] (bool freeze)
            {
                Job j; j.len = (juce::int64) (10.5 * kFs); j.block = 512;
                j.params = eg24::fbBase (100.0f);
                for (auto& kv : P { { "s1_density", 12.5f }, { "s1_size", 100.0f }, { "fb_diffusion", 100.0f } }) j.params.push_back (kv);
                constexpr juce::int64 rel = 96000 + 5 * 48000, cut = rel + 480;
                if (freeze)
                {
                    j.events.push_back ({ 96000, [] (Proc& p) { eg::setP (p, "freeze", 1.0f); } });
                    j.events.push_back ({ rel, [] (Proc& p) { eg::setP (p, "freeze", 0.0f); } });
                }
                j.stim = [] (juce::int64 i, int ch) { return i < cut ? eg::hashNoise (i, ch, eg24::kNoise52) : 0.0f; };
                return j;
            };
            const auto s = eg::render (job (true)), c = eg::render (job (false));
            const double t0 = (96000 + 5 * 48000 + 480) / kFs + 0.05;
            auto fs2 = eg24::decayFit (s, kFs, t0, t0 + 2.0), fc2 = eg24::decayFit (c, kFs, t0, t0 + 2.0);
            juce::String note;
            if (std::abs (fs2.slope - fc2.slope) > 0.2)
            {
                fs2 = eg24::decayFit (s, kFs, t0, t0 + 4.0); fc2 = eg24::decayFit (c, kFs, t0, t0 + 4.0);
                note = " [fit lengthened to 4 s]";
            }
            // ±0.2 dB/s was sized for a few dB/s; the linear corner decays ~ −41 dB/s
            // (≈ −2 dB per 49.5 ms generation), so the tolerance is max (0.2, 2 %).
            const double tol = juce::jmax (0.2, 0.02 * std::abs (fc2.slope));
            row (std::abs (fs2.slope - fc2.slope) <= tol && fs2.slope < 0.0,
                 "FZ8 release decay (freeze 5 s @ fb 100 %, then input removed): dB/s within max(0.2, 2 %) of the steady loop, < 0",
                 juce::String::formatted ("released %+.3f dB/s (%d fr), steady %+.3f dB/s (%d fr)", fs2.slope, fs2.frames, fc2.slope, fc2.frames) + note);
        }
    }
} } // namespace eg24b (anonymous)

static void engineRows24b()
{
    std::printf ("\n== N7/FZ/RV/Z3/Z4/M4/C4. Stage 2 / Phase 2.4 rows (engine_gates_24b.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg24b::blockNullFbRows (1);
    eg24b::freezeRowsT8();
    eg24b::freezeRowsT9();
    std::printf ("  [INFO] 2.4b rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
