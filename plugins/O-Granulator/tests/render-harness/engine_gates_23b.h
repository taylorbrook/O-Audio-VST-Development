/*
  ==============================================================================

    O-Granulator — render harness, sections N6 / Z / K2 / M3 / C3 (Stage 2 /
    Phase 2.3, PLAN-2.3 T4, T11, T12). Sibling of engine_gates_23.h.

      N6  PERF-03 block-size null: the N5 everything-on recipe (4 streams,
          cap 32, toggles + custom swap at shared boundaries) PLUS per-grain FX
          on every stream (T4 SVF form: LP / BP / HP / LP, 1.5 kHz, res 60 %,
          filter_rand 50 %; s1 LP → HP at a shared boundary). 441 / 512 / 4096
          / var vs 32 max|Δ| == 0. N6c different seed differs. T12 widens it to
          every stage (drive 50 %, crush 40 % @ 50 %, ring 300 Hz rand 50 % mix
          50 %; s4 Off → BP and a ring_freq change at shared boundaries).
      Z1a QUAL-02 max everything (4 × 200 g/s × 2 s, cap 128, res 100 %, rand
          100 %, drive / crush / ring 100 %, pitch_rand 12): noise 4 s → sine at
          fc 4 s → silence 2 s; LP/BP/HP @ 48 k + LP @ 44.1 / 192 k: every
          sample finite, no subnormal float in the 144 slots at each section
          end, nanKills 0
      Z1b hottest path (drive 0, crush 0, LP res 100 %, ring 100 %, sine at
          fc, spray 0, level +6 dB): same; wet peak INFO (no <= 1 assertion)
      Z2  NaN into ic1[0] / ic1[1] / ic2[0] / ic2[1] of a live SVF grain, +Inf
          into a fade-slot grain after a steal: finite, nanKills == 1, the
          grain retired, max|Δ| == 0 vs the forceRetire reference
      K2a cutoff sweep 200 Hz → 8 kHz [3, 7) s (K1 source, LP res 50 %):
          κ ratio vs static {200, 632, 2000, 8000} <= 2.0
      K2b ring sweep 50 → 2000 Hz: vs static {50, 316, 2000} <= 2.0
      K2c cutoff step 200 → 8 kHz at 5.0 s: vs static {200, 8000} <= 2.0
      K2n control: K2c with fxLiveCoeffs > 2.0 (a grain alive at the step)
      K2i (INFO) K2a with fxLiveCoeffs
      M3  0 allocations: M2 coverage + filter type / cutoff / res / rand,
          drive across the guard, crush / prob, ring sweeps, a NaN kill;
          liveness 1
      C3  CPU (INFO in the exit code; P26 phase gate): C3b bypass, C2′, C3w
          worst (C2 + every FX at max), C3t typical; median of 3 × 10 s

  ==============================================================================
*/
#pragma once

namespace { namespace eg23b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    constexpr double kFs = 48000.0;

    //==========================================================================
    // N6 — block-size null with FX (the N5 base + FX)
    //==========================================================================
    Job n6Base (bool allFx)
    {
        Job base; base.len = (juce::int64) (6.0 * kFs); base.prepBlock = 512;
        base.params = { { "mix", 70.0f }, { "buffer_length", 4.0f }, { "grain_limit", 0.0f } };
        const float dens[4] { 100, 60, 40, 30 }, size[4] { 500, 300, 800, 200 }, scale[4] { 3, 31, 0, 24 }, prand[4] { 12, 7, 5, 9 };
        const float ftype[4] { 1, 2, 3, 1 };
        for (int s = 1; s <= 4; ++s)
        {
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), dens[s - 1] }, { sp (s, "size"), size[s - 1] },
                                { sp (s, "size_rand"), 50.0f }, { sp (s, "spray"), 50.0f }, { sp (s, "jitter"), 50.0f },
                                { sp (s, "position"), 30.0f }, { sp (s, "pitch_rand"), prand[s - 1] }, { sp (s, "reverse_prob"), 50.0f },
                                { sp (s, "window"), 0.6f }, { sp (s, "window_skew"), -40.0f }, { sp (s, "stereo_spread"), 70.0f },
                                { sp (s, "gain_rand"), 50.0f }, { sp (s, "scale"), scale[s - 1] }, { sp (s, "pan"), -60.0f + 40.0f * s },
                                { sp (s, "filter_type"), allFx && s == 4 ? 0.0f : ftype[s - 1] }, { sp (s, "filter_cutoff"), 1500.0f },
                                { sp (s, "filter_res"), 60.0f }, { sp (s, "filter_rand"), 50.0f } })
                base.params.push_back (kv);
            if (allFx)
                for (auto& kv : P { { sp (s, "drive"), 50.0f }, { sp (s, "crush"), 40.0f }, { sp (s, "crush_prob"), 50.0f },
                                    { sp (s, "ring_freq"), 300.0f }, { sp (s, "ring_rand"), 50.0f }, { sp (s, "ring_mix"), 50.0f } })
                    base.params.push_back (kv);
        }
        base.params.push_back ({ "s3_pitch", 5.0f });
        base.pre = [] (Proc& p)
        {
            juce::String bad; p.setCustomScale ("n5a", "edo 7", bad);
            p.setDegreeMask (1, 3, 0b10110101ull);
            p.setDegreeMask (4, 24, 0b1011011ull);
        };
        auto ev = [&base] (juce::int64 at, std::function<void (Proc&)> f) { base.events.push_back ({ at, std::move (f) }); };
        ev (100003, [] (Proc& p) { eg::setP (p, "s1_pitch", 5.0f); eg::setP (p, "mix", 40.0f); eg::setP (p, "s1_level", -4.0f); eg::setP (p, "s2_pan", 40.0f); });
        ev (120001, [] (Proc& p) { eg::setP (p, "s2_mute", 1.0f); });
        ev (140009, [] (Proc& p) { eg::setP (p, "s3_solo", 1.0f); });
        ev (150011, [] (Proc& p) { eg::setP (p, "s4_enable", 0.0f); });
        ev (160007, [] (Proc& p) { eg::setP (p, "s3_solo", 0.0f); });
        ev (170017, [] (Proc& p) { eg::setP (p, "s2_mute", 0.0f); });
        ev (190013, [] (Proc& p) { eg::setP (p, "s4_enable", 1.0f); });
        ev (200003, [] (Proc& p) { juce::String bad; p.setCustomScale ("n5b", "edo 5", bad); });
        ev (210001, [] (Proc& p) { eg::setP (p, "s1_window", 0.2f); eg::setP (p, "s1_window_skew", 30.0f); eg::setP (p, "s1_scale", 9.0f); });
        ev (230003, [] (Proc& p) { p.setDegreeMask (1, 9, 0b1101ull); eg::setP (p, "s2_pitch_rand", 3.0f); });
        // FX changes at shared boundaries.
        ev (130007, [] (Proc& p) { eg::setP (p, "s1_filter_type", 3.0f); });   // s1 LP → HP
        if (allFx)
        {
            ev (180001, [] (Proc& p) { eg::setP (p, "s4_filter_type", 2.0f); });   // s4 Off → BP
            ev (220009, [] (Proc& p) { eg::setP (p, "s2_ring_freq", 900.0f); });
        }
        return base;
    }

    void blockNullFxRows (bool allFx)
    {
        std::printf ("\n-- N6. PERF-03 block-size null, N5 recipe + per-grain FX (%s)\n", allFx ? "all four stages" : "SVF form");
        Job base = n6Base (allFx);
        Job j32 = base; j32.block = 32;
        eg22::RecLog L32; j32.after = eg22::collect (L32);
        const auto ref = eg::render (j32);
        const auto& rd = ref.eng().diag;
        std::uint32_t bitsSeen = 0; int crushed = 0, uncrushedCrushOn = 0, modes[4] {};
        for (auto& r : L32.r)
        {
            bitsSeen |= r.fxActive;
            modes[juce::jlimit (0, 3, r.svfMode)]++;
            if (r.crushed) ++crushed; else ++uncrushedCrushOn;
        }
        const std::uint32_t want = allFx ? (ogran::kFxSvf | ogran::kFxDrive | ogran::kFxCrush | ogran::kFxRing) : ogran::kFxSvf;
        const bool live = rd.kills > 0 && rd.spawns > 500 && (bitsSeen & want) == want && modes[1] > 0 && modes[2] > 0 && modes[3] > 0
                          && (! allFx || (crushed > 0 && uncrushedCrushOn > 0)) && ! L32.overflow;
        Tally t;
        for (int b : { 441, 512, 4096, 0 })
        {
            Job jb = base; jb.block = b;
            const auto o = eg::render (jb);
            const auto d = eg::diff (ref, o);
            const bool ok = d.count == 0 && o.eng().diag.spawns == rd.spawns && o.eng().diag.reachViolations == 0;
            t.check (ok, (b == 0 ? juce::String ("var") : juce::String (b)) + juce::String::formatted (": %d diffs max %.3g @%lld", d.count, d.maxAbs, (long long) d.first));
        }
        row (t.bad == 0 && live && rd.reachViolations == 0 && rd.nanKills == 0,
             juce::String ("N6 4 streams, everything on + FX (") + (allFx ? "all FX" : "SVF") + "): 441/512/4096/var vs 32 max|d| == 0",
             t.detail (4) + juce::String::formatted ("  [spawns %lld kills %lld, fx bits 0x%x, LP/BP/HP %d/%d/%d, crushed %d/%d]",
                                                     (long long) rd.spawns, (long long) rd.kills, (unsigned) bitsSeen, modes[1], modes[2], modes[3],
                                                     crushed, crushed + uncrushedCrushOn)
                 + (live ? "" : "  VACUOUS"));
        Job js = j32; js.seed = base.seed ^ 0x1ull; js.after = {};
        const auto d = eg::diff (ref, eg::render (js));
        row (d.count > 0, "N6c different-seed control differs", juce::String (d.count) + " diffs");
    }

    //==========================================================================
    // Z1 / Z2 — QUAL-02
    //==========================================================================
    bool anySubnormal (const ogran::Grain& g)
    {
        auto sub = [] (float x) { return std::fpclassify (x) == FP_SUBNORMAL; };
        auto subd = [] (double x) { return std::fpclassify (x) == FP_SUBNORMAL; };
        return sub (g.gainL) || sub (g.gainR) || sub (g.killGain) || sub (g.killStep) || sub (g.windowSkew) || sub (g.tiltT)
            || sub (g.tiltA) || sub (g.tiltB) || sub (g.pitchCents) || sub (g.svfK) || sub (g.svfA1) || sub (g.svfA2) || sub (g.svfA3)
            || sub (g.svfIc1[0]) || sub (g.svfIc1[1]) || sub (g.svfIc2[0]) || sub (g.svfIc2[1]) || sub (g.driveD) || sub (g.driveNorm)
            || sub (g.crushHoldInc) || sub (g.crushHoldPhase) || sub (g.crushScale) || sub (g.crushHeld[0]) || sub (g.crushHeld[1])
            || sub (g.ringRe) || sub (g.ringIm) || sub (g.ringRotRe) || sub (g.ringRotIm) || sub (g.ringMix)
            || subd (g.rate) || subd (g.step) || subd (g.readAbs) || subd (g.frac) || subd (g.len) || subd (g.invLen) || subd (g.startDelay);
    }

    struct StressResult { bool finite = true; int subnormalScans = 0, scans = 0; long long nanKills = 0, spawns = 0, kills = 0, steals = 0; double peak = 0.0; };

    StressResult stress (double fs, int mode, bool hot)
    {
        Job j; j.fs = fs; j.len = (juce::int64) (10.0 * fs); j.block = 512;
        j.params = eg22::base ({ 1, 2, 3, 4 });
        j.params.push_back ({ "grain_limit", 3.0f });
        for (int s = 1; s <= 4; ++s)
        {
            for (auto& kv : P { { sp (s, "density"), 200.0f }, { sp (s, "size"), 2000.0f }, { sp (s, "filter_type"), (float) mode },
                                { sp (s, "filter_res"), 100.0f }, { sp (s, "filter_cutoff"), 2000.0f }, { sp (s, "filter_rand"), 100.0f },
                                { sp (s, "drive"), hot ? 0.0f : 100.0f }, { sp (s, "crush"), hot ? 0.0f : 100.0f }, { sp (s, "crush_prob"), 100.0f },
                                { sp (s, "ring_mix"), 100.0f }, { sp (s, "ring_freq"), 5000.0f }, { sp (s, "ring_rand"), 100.0f },
                                { sp (s, "pitch_rand"), 12.0f } })
                j.params.push_back (kv);
            if (hot)
                for (auto& kv : P { { sp (s, "spray"), 0.0f }, { sp (s, "level"), 6.0f } }) j.params.push_back (kv);
        }
        const juce::int64 a = (juce::int64) (4.0 * fs), b = (juce::int64) (8.0 * fs);
        j.stim = [a, b, fs] (juce::int64 i, int ch) -> float
        {
            if (i < a) return eg::hashNoise (i, ch, 1.0f);
            if (i < b) return (float) std::sin (2.0 * eg::kPi * 2000.0 * (double) i / fs);
            return 0.0f;
        };
        StressResult r;
        int nextCheck = 0;
        const juce::int64 ends[3] { a, b, j.len };
        j.after = [&] (Proc& p, juce::int64 pos)
        {
            if (nextCheck < 3 && pos >= ends[nextCheck] - 512)
            {
                const auto& pool = p.getEngineForTest().getPool();
                for (int k = 0; k < ogran::kPoolLive + ogran::kPoolFade; ++k)
                {
                    ++r.scans;
                    if (anySubnormal (pool.getGrains()[(size_t) k])) ++r.subnormalScans;
                }
                ++nextCheck;
            }
        };
        const auto o = eg::render (j);
        for (size_t i = 0; i < o.L.size(); ++i)
        {
            if (! std::isfinite (o.L[i]) || ! std::isfinite (o.R[i])) r.finite = false;
            else r.peak = juce::jmax (r.peak, (double) std::abs (o.L[i]), (double) std::abs (o.R[i]));
        }
        const auto& d = o.eng().diag;
        r.nanKills = d.nanKills; r.spawns = d.spawns; r.kills = d.kills; r.steals = d.steals;
        r.scans = nextCheck == 3 ? r.scans : -r.scans;
        return r;
    }

    void stressRows()
    {
        std::printf ("\n-- Z. QUAL-02 max-everything stress + NaN kill\n");
        {
            Tally t; juce::String det;
            struct C { double fs; int mode; };
            for (auto c : { C { 48000.0, 1 }, C { 48000.0, 2 }, C { 48000.0, 3 }, C { 44100.0, 1 }, C { 192000.0, 1 } })
            {
                const auto r = stress (c.fs, c.mode, false);
                t.check (r.finite && r.subnormalScans == 0 && r.scans == 432 && r.nanKills == 0 && r.spawns > 1000,
                         juce::String::formatted ("fs %g mode %d: finite %d sub %d/%d nanKills %lld", c.fs, c.mode, (int) r.finite, r.subnormalScans, r.scans, r.nanKills));
                det << juce::String::formatted ("%g/%s pk %.2f; ", c.fs / 1000.0, c.mode == 1 ? "LP" : c.mode == 2 ? "BP" : "HP", r.peak);
                if (c.fs == 48000.0 && c.mode == 1) det << juce::String::formatted ("[spawns %lld kills %lld steals %lld] ", r.spawns, r.kills, r.steals);
            }
            row (t.bad == 0, "Z1a max everything (LP/BP/HP @ 48 k, LP @ 44.1/192 k): finite, no subnormal, nanKills 0", t.detail (5) + "  " + det);
        }
        {
            const auto r = stress (48000.0, 1, true);
            row (r.finite && r.subnormalScans == 0 && r.scans == 432 && r.nanKills == 0,
                 "Z1b hottest path (LP res 100 %, ring 100 %, sine at fc, +6 dB): finite, no subnormal, nanKills 0",
                 juce::String::formatted ("sub %d/%d, nanKills %lld", r.subnormalScans, r.scans, r.nanKills));
            std::printf ("  [INFO] Z1b wet peak %.2f (%+.1f dBFS; R19 expects up to ~+38 dBFS pre-safety, 2.4 item)\n", r.peak, 20.0 * std::log10 (juce::jmax (1e-30, r.peak)));
        }
        // Z2
        {
            auto job = [] (bool steal)
            {
                Job j; j.len = (juce::int64) (3.0 * kFs); j.block = 512;
                j.params = eg22::base ({ 1 });
                for (auto& kv : P { { "s1_filter_type", 1.0f }, { "s1_filter_cutoff", 3000.0f }, { "s1_filter_res", 60.0f },
                                    { "s1_density", steal ? 200.0f : 20.0f }, { "s1_size", steal ? 1000.0f : 300.0f }, { "grain_limit", 3.0f } })
                    j.params.push_back (kv);
                return j;
            };
            // the fade-slot target: the first steal's fade copy lands in slot kPoolLive.
            juce::int64 firstSteal = -1;
            {
                Job j = job (true); eg22::RecLog L; j.after = eg22::collect (L);
                eg::render (j);
                for (auto& r : L.r) if (r.stolen) { firstSteal = r.onsetSample; break; }
            }
            struct Case { const char* name; int slot; juce::int64 at; int field; float v; bool steal; };
            const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
            const std::vector<Case> cases {
                { "ic1[0] NaN", 0, (juce::int64) (1.0 * kFs), 0, nan, false },
                { "ic1[1] NaN", 0, (juce::int64) (1.3 * kFs), 1, nan, false },
                { "ic2[0] NaN", 0, (juce::int64) (1.6 * kFs), 2, nan, false },
                { "ic2[1] NaN", 0, (juce::int64) (1.9 * kFs), 3, nan, false },
                { "fade ic1[0] +Inf", ogran::kPoolLive, firstSteal + 1, 0, inf, true },
            };
            Tally t;
            for (auto& c : cases)
            {
                auto run = [&] (bool inject, bool& retired)
                {
                    Job j = job (c.steal);
                    juce::int64 doneAt = -1;
                    double idLen = -1.0, idDelay = -1.0;   // identity of the target, seen before the kill
                    j.pre = [&] (Proc& p)
                    {
                        auto& h = p.getEngineForTest().hooks;
                        if (inject) h.injectNan = { c.slot, c.at, c.field, c.v, false };
                        else        h.forceRetire = { c.slot, c.at, false };
                    };
                    // "Retired after that segment" is checked inside the engine, right
                    // after pool.render and before any spawn can reuse the slot
                    // (Diag::nanKillNotRetired).
                    juce::ignoreUnused (doneAt, idLen, idDelay);
                    auto o = eg::render (j);
                    retired = o.eng().diag.nanKillNotRetired == 0;
                    return o;
                };
                bool rI = false, rR = false;
                const auto oi = run (true, rI), orf = run (false, rR);
                bool finite = true;
                for (size_t i = 0; i < oi.L.size(); ++i) finite = finite && std::isfinite (oi.L[i]) && std::isfinite (oi.R[i]);
                const auto d = eg::diff (oi, orf);
                t.check (finite && oi.eng().diag.nanKills == 1 && orf.eng().diag.nanKills == 1 && rI && rR && d.count == 0
                             && oi.eng().hooks.injectNan.done && (! c.steal || firstSteal > 0),
                         juce::String (c.name) + juce::String::formatted (": finite %d kills %lld/%lld retired %d/%d diffs %d",
                             (int) finite, (long long) oi.eng().diag.nanKills, (long long) orf.eng().diag.nanKills, (int) rI, (int) rR, d.count));
            }
            row (t.bad == 0, "Z2 NaN/Inf into each SVF integrator + a fade slot: finite, 1 kill, retired, == forceRetire ref",
                 t.detail ((int) cases.size()) + juce::String::formatted ("  [first steal @%lld]", (long long) firstSteal));
        }
    }

    //==========================================================================
    // K2 — QUAL-01 FX sweeps (latched → steps only between grains)
    //==========================================================================
    void setNorm (Proc& p, const char* id, float v)
    {
        if (auto* rp = param (p, id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    }

    void sweepRows()
    {
        std::printf ("\n-- K2. QUAL-01 cutoff / ring sweeps and a cutoff step (kappa vs static)\n");
        auto job = [] (P extra)
        {
            Job j; j.len = (juce::int64) (7.0 * kFs); j.block = 512; j.stim = eg::sineStim (440.0, kFs, 0.001);
            j.params = eg22::base ({ 1 });
            for (auto& kv : P { { "s1_density", 20.0f }, { "s1_size", 200.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 20.0f } }) j.params.push_back (kv);
            for (auto& kv : extra) j.params.push_back (kv);
            return j;
        };
        auto kappa = [] (const Out& o, double t0, double t1)
        {
            auto k1 = [&] (const std::vector<float>& x)
            {
                const auto a = (juce::int64) (t0 * kFs), b = (juce::int64) (t1 * kFs);
                double e = 0.0;
                for (auto i = a; i < b; ++i) e += (double) x[(size_t) i] * x[(size_t) i];
                return eg::maxCurv (x, (size_t) a, (size_t) b) / juce::jmax (1e-30, std::sqrt (e / (double) (b - a)));
            };
            return juce::jmax (k1 (o.L), k1 (o.R));
        };
        auto sweep = [] (Job& j, const char* id, double v0, double v1)
        {
            for (juce::int64 at = (juce::int64) (3.0 * kFs); at < (juce::int64) (7.0 * kFs); at += 256)
            {
                const double t = (double) (at - (juce::int64) (3.0 * kFs)) / (4.0 * kFs);
                const float v = (float) (v0 * std::pow (v1 / v0, t));
                j.events.push_back ({ at, [id, v] (Proc& p) { setNorm (p, id, v); } });
            }
        };
        const P lp { { "s1_filter_type", 1.0f }, { "s1_filter_res", 50.0f } };
        auto statics = [&] (const P& basep, const char* id, std::initializer_list<float> vals, double t0, double t1)
        {
            double mx = 0.0;
            for (float v : vals)
            {
                P ps = basep; ps.push_back ({ id, v });
                mx = juce::jmax (mx, kappa (eg::render (job (ps)), t0, t1));
            }
            return mx;
        };
        // K2a
        {
            const double ref = statics (lp, "s1_filter_cutoff", { 200.0f, 632.0f, 2000.0f, 8000.0f }, 3.0, 7.0);
            P ps = lp; ps.push_back ({ "s1_filter_cutoff", 200.0f });
            Job j = job (ps); sweep (j, "s1_filter_cutoff", 200.0, 8000.0);
            const double k = kappa (eg::render (j), 3.0, 7.0);
            row (k / ref <= 2.0, "K2a cutoff sweep 200 Hz -> 8 kHz (LP res 50 %): kappa <= 2.0x static max",
                 juce::String::formatted ("ratio %.3f (kappa %.4g, static ref %.4g)", k / ref, k, ref));
            Job jl = job (ps); sweep (jl, "s1_filter_cutoff", 200.0, 8000.0);
            jl.pre = [] (Proc& p) { p.getEngineForTest().hooks.fxLiveCoeffs = true; };
            std::printf ("  [INFO] K2i K2a with fxLiveCoeffs (RESEARCH sweep control): ratio %.3f\n", kappa (eg::render (jl), 3.0, 7.0) / ref);
        }
        // K2b
        {
            const P rm { { "s1_ring_mix", 100.0f } };
            const double ref = statics (rm, "s1_ring_freq", { 50.0f, 316.0f, 2000.0f }, 3.0, 7.0);
            P ps = rm; ps.push_back ({ "s1_ring_freq", 50.0f });
            Job j = job (ps); sweep (j, "s1_ring_freq", 50.0, 2000.0);
            const double k = kappa (eg::render (j), 3.0, 7.0);
            row (k / ref <= 2.0, "K2b ring sweep 50 -> 2000 Hz (mix 100 %): kappa <= 2.0x static max",
                 juce::String::formatted ("ratio %.3f (kappa %.4g, static ref %.4g)", k / ref, k, ref));
        }
        // K2c / K2n
        {
            const double ref = statics (lp, "s1_filter_cutoff", { 200.0f, 8000.0f }, 4.5, 5.5);
            const juce::int64 stepAt = (juce::int64) (5.0 * kFs);
            auto stepJob = [&] (bool live, bool& alive)
            {
                P ps = lp; ps.push_back ({ "s1_filter_cutoff", 200.0f });
                Job j = job (ps);
                j.events.push_back ({ stepAt, [] (Proc& p) { setNorm (p, "s1_filter_cutoff", 8000.0f); } });
                j.pre = [live] (Proc& p) { p.getEngineForTest().hooks.fxLiveCoeffs = live; };
                eg22::RecLog L; j.after = eg22::collect (L);
                const auto o = eg::render (j);
                alive = false;
                for (auto& r : L.r)
                    alive = alive || ((r.fxActive & ogran::kFxSvf) && r.onsetSample < stepAt && (double) r.onsetSample + r.len > (double) stepAt + 64.0);
                return kappa (o, 4.5, 5.5);
            };
            bool a1 = false, a2 = false;
            const double kc = stepJob (false, a1), kn = stepJob (true, a2);
            row (kc / ref <= 2.0, "K2c cutoff step 200 -> 8 kHz at 5.0 s (latched): kappa <= 2.0x static",
                 juce::String::formatted ("ratio %.3f (static ref %.4g)", kc / ref, ref));
            row (a2 && kn / ref > 2.0, "K2n control: the step with fxLiveCoeffs reads > 2.0x",
                 juce::String::formatted ("ratio %.3f; grain alive across the step: %s", kn / ref, a2 ? "yes" : "NO"));
        }
    }

    //==========================================================================
    // M3 — allocations with FX
    //==========================================================================
    void allocFxRows()
    {
        std::printf ("\n-- M3. PERF-01 allocations with per-grain FX\n");
       #if JUCE_MAC
        using namespace eg;
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocTrace = false;
        allocArmed = true;
        void* volatile probe = std::malloc (64);
        allocArmed = false;
        std::free (probe);
        const int liveness = allocCount;
        allocCount = 0;
        allocTrace = true;

        Job j; j.len = (juce::int64) (12.0 * kFs); j.block = 512;
        j.params = { { "mix", 60.0f }, { "grain_limit", 3.0f } };
        for (int s = 1; s <= 4; ++s)
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 60.0f }, { sp (s, "size"), 1000.0f },
                                { sp (s, "size_rand"), 40.0f }, { sp (s, "spray"), 50.0f }, { sp (s, "jitter"), 50.0f },
                                { sp (s, "pitch_rand"), 7.0f }, { sp (s, "reverse_prob"), 50.0f }, { sp (s, "scale"), (float) (s * 7) },
                                { sp (s, "window"), 0.3f * (float) s }, { sp (s, "window_skew"), 20.0f * (float) s - 50.0f },
                                { sp (s, "stereo_spread"), 50.0f }, { sp (s, "gain_rand"), 50.0f },
                                { sp (s, "filter_type"), (float) (s % 4) }, { sp (s, "filter_res"), 70.0f }, { sp (s, "filter_rand"), 50.0f },
                                { sp (s, "drive"), 30.0f }, { sp (s, "crush"), 30.0f }, { sp (s, "crush_prob"), 50.0f },
                                { sp (s, "ring_mix"), 40.0f }, { sp (s, "ring_rand"), 50.0f } })
                j.params.push_back (kv);
        j.pre = [] (Proc& p)
        {
            juce::String bad; p.setCustomScale ("m3a", "edo 9", bad);
            p.getEngineForTest().hooks.injectNan = { 5, (juce::int64) (7.0 * kFs), 0, std::numeric_limits<float>::quiet_NaN(), false };
        };
        j.events.push_back ({ (juce::int64) (3.0 * kFs), [] (Proc& p) { setP (p, "grain_limit", 0.0f); } });
        j.events.push_back ({ (juce::int64) (9.0 * kFs), [] (Proc& p) { setP (p, "grain_limit", 1.0f); } });
        for (int k = 1; k < 115; ++k)
        {
            const juce::int64 at = (juce::int64) (0.1 * k * kFs) + 7;
            j.events.push_back ({ at, [k] (Proc& p)
            {
                const int s = 1 + (k % 4);
                switch (k % 8)
                {
                    case 0: setP (p, sp (s, "filter_type"), (float) ((k / 8) % 4)); break;
                    case 1: setNorm (p, sp (s, "filter_cutoff").toRawUTF8(), (float) (50.0 * std::pow (2.0, (k % 9)))); setP (p, sp (s, "filter_res"), (float) ((k * 13) % 101)); break;
                    case 2: setP (p, sp (s, "filter_rand"), (float) ((k * 17) % 101)); break;
                    case 3: setP (p, sp (s, "drive"), k % 3 == 0 ? 0.2f : k % 3 == 1 ? 0.3f : (float) ((k * 7) % 101)); break;
                    case 4: setP (p, sp (s, "crush"), (float) ((k * 11) % 101)); setP (p, sp (s, "crush_prob"), (float) ((k * 29) % 101)); break;
                    case 5: setNorm (p, sp (s, "ring_freq").toRawUTF8(), (float) (20.0 * std::pow (1.8, (k % 10)))); break;
                    case 6: setP (p, sp (s, "ring_rand"), (float) ((k * 19) % 101)); setP (p, sp (s, "ring_mix"), (float) ((k * 23) % 101)); break;
                    default: setP (p, sp (s, "scale"), (float) (1 + (k * 5) % 31)); setP (p, sp (s, "window"), (float) ((k * 7) % 65) / 64.0f); break;
                }
            } });
        }
        armDuringProcess = false;
        j.after = [] (Proc&, juce::int64) { armDuringProcess = true; };
        const auto o = render (j);
        armDuringProcess = false;
        malloc_logger = nullptr;
        allocTrace = false;
        const auto& dg = o.eng().diag;
        const int n = allocCount;
        const bool live = dg.steals > 0 && dg.kills > 0 && dg.nanKills == 1 && dg.activeSetRebuilds > 5;
        row (liveness == 1, "M3 liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && n == 0 && live, "M3 0 allocations: FX type/cutoff/res/rand, drive across guard, crush, ring sweeps, NaN kill, steals/kills",
             juce::String::formatted ("%d allocs; spawns %lld steals %lld kills %lld nanKills %lld rebuilds %lld", n, (long long) dg.spawns,
                                      (long long) dg.steals, (long long) dg.kills, (long long) dg.nanKills, (long long) dg.activeSetRebuilds)
                 + (live ? "" : "  VACUOUS"));
       #else
        row (false, "M3 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // C3 — CPU (INFO; P26 phase gate on the printed medians)
    //==========================================================================
    double timeConfig (const P& ps, int& alive)
    {
        auto p = make();
        eg::setAll (*p, ps);
        p->setSeed (0xC2C2ull);
        p->prepareToPlay (kFs, 128);
        juce::AudioBuffer<float> buf (2, 128);
        juce::MidiBuffer midi;
        juce::int64 pos = 0;
        auto runFor = [&] (double secs)
        {
            const juce::int64 end = pos + (juce::int64) (secs * kFs);
            for (; pos < end; pos += 128)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 128; ++i) buf.setSample (ch, i, eg::hashNoise (pos + i, ch));
                p->processBlock (buf, midi);
            }
        };
        runFor (4.5);
        alive = 0;
        for (int s = 0; s < ogran::kPoolLive; ++s) alive += p->getEngineForTest().getPool().isActive (s) ? 1 : 0;
        double t[3];
        for (double& x : t)
        {
            const auto t0 = std::chrono::steady_clock::now();
            runFor (10.0);
            x = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / 10.0;
        }
        std::sort (t, t + 3);
        return t[1] * 100.0;
    }

    void cpuRows()
    {
        std::printf ("\n-- C3. CPU with per-grain FX (INFO; P26 phase gate: C3w <= 30 %%, C3t <= 10 %%)\n");
        P c2 { { "mix", 100.0f }, { "grain_limit", 3.0f } };
        for (int s = 1; s <= 4; ++s)
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 16.0f }, { sp (s, "size"), 2000.0f },
                                { sp (s, "window"), 0.6f }, { sp (s, "scale"), 1.0f }, { sp (s, "pitch_rand"), 2.0f } })
                c2.push_back (kv);
        P byp { { "mix", 100.0f }, { "grain_limit", 3.0f } };
        for (int s = 1; s <= 4; ++s) byp.push_back ({ sp (s, "enable"), 0.0f });
        P worst = c2;
        for (int s = 1; s <= 4; ++s)
            for (auto& kv : P { { sp (s, "filter_type"), 1.0f }, { sp (s, "filter_res"), 100.0f }, { sp (s, "filter_rand"), 100.0f },
                                { sp (s, "drive"), 100.0f }, { sp (s, "crush"), 100.0f }, { sp (s, "crush_prob"), 100.0f },
                                { sp (s, "ring_mix"), 100.0f }, { sp (s, "ring_rand"), 100.0f } })
                worst.push_back (kv);
        P typ { { "mix", 100.0f }, { "grain_limit", 3.0f } };
        for (int s = 1; s <= 4; ++s)
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 8.0f }, { sp (s, "size"), 2000.0f },
                                { sp (s, "window"), 0.0f }, { sp (s, "scale"), 0.0f }, { sp (s, "filter_type"), 1.0f },
                                { sp (s, "filter_res"), 20.0f }, { sp (s, "drive"), 30.0f } })
                typ.push_back (kv);
        int aB, a2, aW, aT;
        const double b = timeConfig (byp, aB), c2p = timeConfig (c2, a2), w = timeConfig (worst, aW), t = timeConfig (typ, aT);
        std::printf ("  [INFO] C3b bypass (all streams off): %.2f %% of one core (live %d)\n", b, aB);
        std::printf ("  [INFO] C2' C2 config re-run: %.2f %% (live %d; 2.2 verify 8.92 %%)\n", c2p, a2);
        std::printf ("  [INFO] C3w worst (C2 + LP res 100 %% rand 100 %%, drive/crush/ring 100 %%): %.2f %% (live %d), +%.2f over C3b, %.2fx C2'  -> budget 30 %%: %s\n",
                     w, aW, w - b, w / juce::jmax (1e-9, c2p), w <= 30.0 ? "WITHIN" : "OVER");
        std::printf ("  [INFO] C3t typical (4 x 8 g/s x 2 s, LP res 20 %% + drive 30 %%): %.2f %% (live %d), +%.2f over C3b  -> budget 10 %%: %s\n",
                     t, aT, t - b, t <= 10.0 ? "WITHIN" : "OVER");
    }
} } // namespace eg23b (anonymous)

static void engineRows23b()
{
    std::printf ("\n== N6/Z/K2/M3/C3. Stage 2 / Phase 2.3 PERF + QUAL rows (engine_gates_23b.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg23b::stressRows();
    eg23b::sweepRows();
    eg23b::blockNullFxRows (true);
    eg23b::allocFxRows();
    eg23b::cpuRows();
    std::printf ("  [INFO] 2.3b rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
