/*
  ==============================================================================

    O-Granulator — render harness, Stage 2 / Phase 2.6 PERF / QUAL-01 rows
    (PLAN-2.6 P94). Sibling of engine_gates_26.h (uses its builders).

      C7 / C7L     PERF-02 CPU at the D22 argmax, 48 k / 128 (R83, R84, R86) (T4)
      GL1 / GL2 / GL2c   grain_limit cap + determinism (R98)              (T4)
      MX3a–g, MX3nR, MX3nD   trajectory full cross (R90a)               (T5)
      MF8a–f, MF8n (MF8i)    audio kappa on every application path (R90b) (T6)
      EN1a–c, EN1n           enable / mute / solo                         (T6)
      FZ9a / b, K3a / b, QB3a / b (+ NCs)   freeze, steal / kill, bursts  (T7)
      M7L / M7F / M7T / M7c  PERF-01 armed from block 0 (R97)            (T9)
      N10F / N10T / N10c / N10n, O2   PERF-03 (R99, R100)                (T9)

  ==============================================================================
*/
#pragma once

#include <ctime>
#if JUCE_MAC
 #include <pthread/qos.h>
#endif

namespace { namespace eg26b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg26::Stim;
    using eg26::c7Params;
    using eg26::c7Stim;
    using eg26::perStream;
    using eg26::info;
    using eg26::Census;
    using eg26::census;
    using eg22::sp;
    using eg25b::route;
    using eg25b::operator+;
    namespace D = ogran::ModDest;

    //==========================================================================
    // C7 — PERF-02 (P84)
    //==========================================================================
    inline double threadCpuS()
    {
        timespec ts {};
        clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts);
        return (double) ts.tv_sec + 1e-9 * (double) ts.tv_nsec;
    }

    /** PLAN-4 P150 (D47 c): fixed-work machine-speed calibration — a dependent
        FMA chain (each step needs the last; ~50 ms on the 3.3 machine), sunk
        into a volatile so it cannot be folded. Returns ns per step. A core that
        is uniformly slowed (thermal, E-core) shows up here, where the duty
        witness cannot see it. */
    inline double calibrateNs()
    {
        constexpr int kSteps = 40'000'000;
        static volatile double sink = 0.0;
        double x = 1.0 + 1e-9 * (double) (sink != 0.0 ? 1 : 0);
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < kSteps; ++i)
            x = std::fma (x, 0.999999999, 1.0e-9);
        const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        sink = x;
        return s * 1e9 / (double) kSteps;
    }
    inline double& sessionCalNs() { static double v = 0.0; return v; }   // the session's first calibration

    struct CpuRes
    {
        std::vector<double> pct, duty;      // valid runs only
        int contended = 0;
        int slowed = 0;                     // P150: runs whose calibration ratio vs the session's first > 1.10
        double calNs = 0.0, calMaxRatio = 0.0;
        std::vector<double> lastUs;         // per-block µs of the last valid run
        long long events = 0, hops = 0, confident = 0, steals = 0;
        long long maxAlive = 0; int aliveWarm = 0;
        double secs = 0.0;
        double median() const { auto v = pct; std::sort (v.begin(), v.end()); return v.empty() ? 0.0 : v[v.size() / 2]; }
    };

    /** One config on a dedicated USER_INTERACTIVE thread: prepare on that thread,
        a playing 120 bpm MockPlayHead, exact `block`-sample blocks, warm-up, then
        `runs` × runS (a contended run, duty < 0.9, is replaced up to `extra` times). */
    inline CpuRes timeC7 (const P& ps, double fs, int block, int runs, double runS, double warmS, int extra)
    {
        CpuRes res;
        std::thread th ([&]
        {
           #if JUCE_MAC
            pthread_set_qos_class_self_np (QOS_CLASS_USER_INTERACTIVE, 0);
           #endif
            auto p = make();
            eg::setAll (*p, ps);
            p->setSeed (0xC2C2ull);
            MockPlayHead head;
            head.sampleRate = fs; head.bpm = 120.0; head.playing = true;
            p->setPlayHead (&head);
            p->prepareToPlay (fs, block);
            const auto stim = eg26::c7Buffer (fs);
            const juce::int64 N = (juce::int64) stim->size();
            juce::AudioBuffer<float> buf (2, block);
            juce::MidiBuffer midi;
            juce::int64 pos = 0;
            auto runFor = [&] (double secs, double& wall, double& thr, std::vector<double>* us)
            {
                const juce::int64 end = pos + (juce::int64) (secs * fs);
                wall = 0.0;
                const double c0 = threadCpuS();
                for (; pos < end; pos += block)
                {
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        float* d = buf.getWritePointer (ch);
                        for (int i = 0; i < block; ++i) d[i] = (*stim)[(size_t) ((pos + i) % N)];
                    }
                    head.timeInSamples = pos;
                    const auto b0 = std::chrono::steady_clock::now();
                    p->processBlock (buf, midi);
                    const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - b0).count();
                    wall += s;
                    if (us != nullptr) us->push_back (s * 1e6);
                }
                thr = threadCpuS() - c0;
            };
            double w = 0.0, c = 0.0;
            runFor (warmS, w, c, nullptr);
            auto& e = p->getEngineForTest();
            for (int s = 0; s < ogran::kPoolLive + ogran::kPoolFade; ++s) res.aliveWarm += e.getPool().isActive (s) ? 1 : 0;
            const auto d0 = e.diag;
            if (sessionCalNs() <= 0.0) sessionCalNs() = calibrateNs();
            res.calNs = sessionCalNs();
            for (int r = 0; (int) res.pct.size() < runs && r < runs + extra; ++r)
            {
                std::vector<double> us;
                const double cb = calibrateNs();
                runFor (runS, w, c, &us);
                const double ca = calibrateNs();
                res.secs += runS;
                const double ratio = std::max (cb, ca) / sessionCalNs();
                res.calMaxRatio = std::max (res.calMaxRatio, ratio);
                const double duty = w > 0.0 ? c / w : 0.0;
                if (duty < 0.9) { ++res.contended; continue; }
                if (ratio > 1.10) { ++res.contended; ++res.slowed; continue; }   // P150: a slowed core counts as contended
                res.pct.push_back (w / runS * 100.0);
                res.duty.push_back (duty);
                res.lastUs = std::move (us);
            }
            const auto& d = e.diag;
            res.events = d.onsetEvents - d0.onsetEvents;
            res.hops = d.mpmHops - d0.mpmHops;
            res.confident = d.mpmConfident - d0.mpmConfident;
            res.steals = d.steals - d0.steals;
            res.maxAlive = d.maxAlive;
            p->setPlayHead (nullptr);
        });
        th.join();
        return res;
    }

    inline juce::String spread (const CpuRes& r)
    {
        if (r.pct.empty()) return "no valid run";
        auto us = r.lastUs;
        std::sort (us.begin(), us.end());
        const double mn = *std::min_element (r.pct.begin(), r.pct.end()), mx = *std::max_element (r.pct.begin(), r.pct.end());
        const double dmin = *std::min_element (r.duty.begin(), r.duty.end());
        return juce::String::formatted ("median %.2f %% (min %.2f max %.2f spread %.2f, N %d valid, %d contended (%d slowed), duty >= %.3f); last run per block p50 %.1f p99 %.1f max %.1f us; cal %.3f ns/op, max ratio %.3f",
                                        r.median(), mn, mx, mx - mn, (int) r.pct.size(), r.contended, r.slowed, dmin,
                                        us.empty() ? 0.0 : us[us.size() / 2], us.empty() ? 0.0 : us[(size_t) (0.99 * (double) us.size())], us.empty() ? 0.0 : us.back(),
                                        r.calNs, r.calMaxRatio);
    }

    void cpuRows()
    {
        std::printf ("\n-- C7. PERF-02 CPU at the D22 argmax (crush 1 %%, route 13 -> s1_level), 48 k / 128, N = 7 x 10 s (P84)\n");
        if (gNoC7) { std::printf ("  [SKIP] C7 / C7L (--no-c7 iteration aid)\n"); return; }
        const auto r = timeC7 (c7Params(), 48000.0, 128, 7, 10.0, 5.0, 3);
        if ((int) r.pct.size() < 7)
        {
            std::printf ("  [NOVERDICT] C7 contended -- re-run (%d valid of 7, %d contended)\n", (int) r.pct.size(), r.contended);
            gNoVerdict = true;
        }
        else
        {
            const double med = r.median();
            const double mx = *std::max_element (r.pct.begin(), r.pct.end());
            if (mx > 30.0 && med <= 30.0) std::printf ("  [FLAG] C7 a single run exceeded 30 %% (%.2f %%) with the median within budget\n", mx);
            row (med <= 30.0, "C7 CPU median of 7 x 10 s <= 30 % of one core @ 48 k / 128 (R83 expects 16.7-16.9 %)", spread (r));
        }
        const double T = r.secs;
        const double evS = (double) r.events / T, stS = (double) r.steals / T, conf = r.hops > 0 ? (double) r.confident / (double) r.hops : 0.0;
        row (evS >= 15.0 && conf >= 0.95 && stS >= 500.0 && r.maxAlive == 128 && r.aliveWarm >= 128,
             "C7L liveness: events >= 15/s, MPM confident >= 95 % of hops, steals >= 500/s, maxAlive 128, >= 128 active slots after warm-up",
             juce::String::formatted ("events %.1f/s, confident %lld/%lld (%.1f %%), steals %.0f/s, maxAlive %lld, active %d (R84: 19/s, 8437/8437, 834/s, 132)",
                                      evS, r.confident, r.hops, 100.0 * conf, stS, r.maxAlive, r.aliveWarm));
        if (gLong)
        {
            P lit = c7Params() + perStream ("crush", 100.0f) + P { { "mod13_dest", (float) D::InputGain } };
            info ("C7 (--long) literal D22 all-max (crush 100 %, route 13 -> input_gain), N 3: " + spread (timeC7 (lit, 48000.0, 128, 3, 10.0, 5.0, 2)));
            for (double fs : { 44100.0, 96000.0, 192000.0 })
                info (juce::String::formatted ("C7 (--long) @ %g k / 128, N 3 (budget %.1f us; R87 44.1 15.3 / 96 33.2 / 192 67.3 %%): ", fs / 1000.0, 128.0 / fs * 1e6)
                      + spread (timeC7 (c7Params(), fs, 128, 3, 10.0, 5.0, 2)));
            for (int cap = 0; cap < 3; ++cap)
                info (juce::String::formatted ("C7 (--long) grain_limit %d, N 3: ", 32 * (cap + 1))
                      + spread (timeC7 (c7Params() + P { { "grain_limit", (float) cap } }, 48000.0, 128, 3, 10.0, 5.0, 2)));
        }
    }

    //==========================================================================
    // GL — grain_limit cap + determinism (P85)
    //==========================================================================
    inline bool sameLog (const eg22::RecLog& a, const eg22::RecLog& b)
    {
        if (a.r.size() != b.r.size() || a.overflow || b.overflow) return false;
        for (size_t k = 0; k < a.r.size(); ++k)
        {
            const auto& x = a.r[k];
            const auto& y = b.r[k];
            if (x.onsetSample != y.onsetSample || x.slot != y.slot || x.stolen != y.stolen || x.killed != y.killed || x.globalIndex != y.globalIndex) return false;
        }
        return true;
    }

    void capRows()
    {
        std::printf ("\n-- GL. PERF-02 grain_limit 32 / 64 / 96 / 128 under the C7 config: cap + determinism (P85)\n");
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<eg22::RecLog>> logs;
        std::vector<std::shared_ptr<int>> scans;
        for (int cap = 0; cap < 4; ++cap)
            for (int v = 0; v < 3; ++v)   // 0, 1: the same seed; 2: seed + 1
            {
                Job j;
                j.fs = 48000.0; j.len = 480000; j.block = 128; j.prepBlock = 128;
                j.params = c7Params() + P { { "grain_limit", (float) cap } };
                j.stim = c7Stim (j.fs);
                j.seed = 0xC2C2ull + (v == 2 ? 1u : 0u);
                auto L = std::make_shared<eg22::RecLog>();
                auto sc = std::make_shared<int> (0);
                j.after = eg22::collect (*L, [sc] (Proc& p, juce::int64)
                {
                    const auto& pool = p.getEngineForTest().getPool();
                    int alive = 0;
                    for (int s = 0; s < ogran::kPoolLive; ++s)
                        if (pool.isActive (s) && ! (pool.getGrains()[(size_t) s].killStep > 0.0f)) ++alive;
                    *sc = juce::jmax (*sc, alive);
                });
                logs.push_back (L); scans.push_back (sc);
                jobs.push_back (j);
            }
        auto o = eg25::renderAll (jobs);
        Tally t1, t2, tc;
        for (int cap = 0; cap < 4; ++cap)
        {
            const size_t a = (size_t) (cap * 3), b = a + 1, c = a + 2;
            const int lim = 32 * (cap + 1);
            const auto& d = o[a].eng().diag;
            const bool path = cap < 3 ? d.kills > 0 : d.steals > 0;
            t1.check (d.maxAlive <= lim && *scans[a] <= lim && path,
                      juce::String::formatted ("cap %d: maxAlive %lld scan %d kills %lld steals %lld", lim, (long long) d.maxAlive, *scans[a], (long long) d.kills, (long long) d.steals));
            const auto dd = eg::diff (o[a], o[b]);
            const bool logs2 = sameLog (*logs[a], *logs[b]);
            t2.check (dd.count == 0 && logs2, juce::String::formatted ("cap %d: %d diffs, logs %s (%d spawns)", lim, dd.count, logs2 ? "same" : "DIFFER", (int) logs[a]->r.size()));
            const auto dc = eg::diff (o[a], o[c]);
            tc.check (dc.count > 0 && ! sameLog (*logs[a], *logs[c]), juce::String::formatted ("cap %d: seed + 1 %d diffs", lim, dc.count));
        }
        juce::String det;
        for (int cap = 0; cap < 4; ++cap)
        {
            const auto& d = o[(size_t) (cap * 3)].eng().diag;
            det << juce::String::formatted ("%d: maxAlive %lld scan %d kills %lld steals %lld spawns %lld; ", 32 * (cap + 1), (long long) d.maxAlive, *scans[(size_t) (cap * 3)],
                                            (long long) d.kills, (long long) d.steals, (long long) d.spawns);
        }
        row (t1.bad == 0, "GL1 grain_limit 32 / 64 / 96 / 128, C7 10 s: maxAlive and the per-block scan <= cap; kills > 0 below 128, steals > 0 at 128", t1.bad ? t1.detail (4) : det.trim());
        row (t2.bad == 0, "GL2 same seed twice per cap: spawn logs (onset, slot, stolen, killed, globalIndex) and audio bitwise", t2.detail (4));
        row (tc.bad == 0, "GL2c seed + 1 differs (logs and audio) at every cap", tc.detail (4));

        // GL3 — D25 class fix (execution deviation): a cap LOWERED live (128 -> 32 at 3 s) under
        // sustained over-demand converges (the M7 script found the pool held at 128 forever).
        {
            struct Scan { int at325 = -1, maxAfter = 0, last = 0; };
            auto mk = [] (bool noConv, Scan* sc)
            {
                Job j;
                j.fs = 48000.0; j.len = 6 * 48000; j.block = 128; j.prepBlock = 128; j.seed = 0x6C33ull;
                j.params = c7Params();
                j.stim = c7Stim (j.fs);
                j.events.push_back ({ 3 * 48000, [] (Proc& p) { eg::setP (p, "grain_limit", 0.0f); } });
                j.pre = [noConv] (Proc& p) { p.getEngineForTest().hooks.capNoConverge = noConv; };
                j.after = [sc] (Proc& p, juce::int64 pos)
                {
                    const auto& pool = p.getEngineForTest().getPool();
                    int alive = 0;
                    for (int s = 0; s < ogran::kPoolLive; ++s)
                        if (pool.isActive (s) && ! (pool.getGrains()[(size_t) s].killStep > 0.0f)) ++alive;
                    if (pos >= 156000 && sc->at325 < 0) sc->at325 = alive;           // 3.25 s
                    if (pos >= 156000) sc->maxAfter = juce::jmax (sc->maxAfter, alive);
                    sc->last = alive;
                };
                return j;
            };
            Scan a, n;
            auto og = eg25::renderAll ({ mk (false, &a), mk (true, &n) });
            const auto& d = og[0].eng().diag;
            row (a.at325 >= 0 && a.maxAfter <= 32 && d.capKills > 0,
                 "GL3 grain_limit lowered live 128 -> 32 under sustained over-demand (C7): live grains <= 32 from 3.25 s to the end",
                 juce::String::formatted ("live at 3.25 s %d, max after %d, end %d; capKills %lld, kills %lld", a.at325, a.maxAfter, a.last, (long long) d.capKills, (long long) d.kills));
            row (n.last > 64, "GL3n capNoConverge (the pre-2.6 acquire): the pool stays above the lowered cap",
                 juce::String::formatted ("live at 3.25 s %d, at the end %d", n.at325, n.last));
        }
    }

    //==========================================================================
    // MX3 — trajectory full cross: 11 sources × 112 destinations × 7 edit kinds (P87)
    //==========================================================================
    /** The composite MX3 input (P87 source drives on one signal): a 233 → 311 Hz
        stepped tone (Pitch) whose level swells +12 dB every 125 ms (Env), with
        2 ms 1 kHz bursts every 180 ms (Onset). */
    inline std::shared_ptr<std::vector<float>> mx3Input (double fs, double lenS)
    {
        auto x = std::make_shared<std::vector<float>> ((size_t) (lenS * fs));
        const double up = std::pow (10.0, 12.0 / 20.0);
        double ph = 0.0;
        for (size_t i = 0; i < x->size(); ++i)
        {
            const double t = (double) i / fs;
            const double hz = std::fmod (t, 0.5) < 0.25 ? 233.0 : 311.0;
            ph += 2.0 * eg::kPi * hz / fs;
            const double tc = std::fmod (t, 0.125);
            const double e = tc < 0.03 ? 1.0 + (up - 1.0) * 0.5 * (1.0 - std::cos (eg::kPi * tc / 0.03)) : (tc < 0.06 ? up - (up - 1.0) * (tc - 0.03) / 0.03 : 1.0);
            double v = 0.08 * e * std::sin (ph);
            const double tb = std::fmod (t, 0.18);
            if (tb < 0.002) v += 0.6 * std::sin (2.0 * eg::kPi * 1000.0 * tb);
            (*x)[i] = (float) v;
        }
        return x;
    }

    /** Every destination's base at 50 % normalised, except the LFO rates (2 Hz,
        the source drives) and s1_density (10 g/s: light audio); s1 only; LFO
        shapes sine / triangle / smooth random / S&H; macros 0 (host-ramped). */
    inline P mx3Base()
    {
        P p = eg22::base ({ 1 }) + P { { "onset_threshold", -40.0f }, { "pitch_follow_s1", 0.0f } };
        const int shapes[4] { 0, 1, 5, 4 };
        for (int n = 1; n <= 4; ++n)
            p = p + P { { "lfo" + juce::String (n) + "_shape", (float) shapes[n - 1] }, { "macro" + juce::String (n), 0.0f } };
        return p;
    }
    inline void setBase50 (Proc& p)
    {
        for (int d = 1; d < ogran::kModDests; ++d)
            if (auto* rp = param (p, GranulatorParamIds::kModDestIds[d]))
                rp->setValueNotifyingHost (0.5f);
        for (int n = 1; n <= 4; ++n) eg::setP (p, "lfo" + juce::String (n) + "_rate", 2.0f);
        eg::setP (p, "s1_density", 10.0f);
    }

    struct Mx3Trace
    {
        std::vector<std::array<float, ogran::kModDests>> dest;   // per cell
        std::vector<std::array<float, ogran::kModSources>> src;
    };
    inline std::shared_ptr<Mx3Trace> mx3Record (Job& j)
    {
        auto tr = std::make_shared<Mx3Trace>();
        auto prev = j.after;
        j.after = [tr, prev] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto& e = p.getEngineForTest();
            std::array<float, ogran::kModDests> d {};
            std::array<float, ogran::kModSources> s {};
            for (int k = 1; k < ogran::kModDests; ++k) d[(size_t) k] = e.getDestTelemetry (k);
            for (int k = 1; k < ogran::kModSources; ++k) s[(size_t) k] = e.getSrcTelemetry (k);
            tr->dest.push_back (d);
            tr->src.push_back (s);
            // macros 0 → 100 % over the 1 s render (host automation, per block)
            const float m = juce::jlimit (0.0f, 1.0f, (float) pos / 48000.0f);
            for (int n = 1; n <= 4; ++n)
                if (auto* rp = param (p, "macro" + juce::String (n))) rp->setValueNotifyingHost (m);
        };
        return tr;
    }

    inline Job mx3Job (const P& routes, std::vector<eg::Event> ev, std::function<void (ogran::Engine::TestHooks&)> hk = {})
    {
        Job j;
        j.fs = 48000.0; j.len = 48000; j.block = 32; j.prepBlock = 32;
        j.params = mx3Base() + routes;
        auto x = mx3Input (48000.0, 1.0);
        j.stim = eg25x::fromBuffer (x);
        j.events = std::move (ev);
        j.pre = [hk] (Proc& p) { setBase50 (p); if (hk) hk (p.getEngineForTest().hooks); };
        return j;
    }

    enum Mx3Kind { kAdd, kSrcNone, kDestNone, kDepth0, kDepthStep, kDestSwap, kSrcSwap, kNumKinds };
    inline const char* kindName (int k)
    {
        static const char* n[] { "add None -> src", "source -> None", "dest -> None", "depth -> 0", "depth +60 -> -60", "dest swap", "source swap" };
        return n[k];
    }

    /** One render: source S on the 16 destinations of group g, edit kind k at cell eCell. */
    inline Job mx3Render (int S, int g, int k, int eCell, std::function<void (ogran::Engine::TestHooks&)> hk = {})
    {
        P routes;
        for (int m = 0; m < 16; ++m)
        {
            const int d = 1 + 16 * g + m;
            const float dep = k == kDepthStep ? 60.0f : (m % 2 ? -60.0f : 60.0f);
            routes = routes + route (m + 1, k == kAdd ? 0 : S, d, dep);
        }
        const juce::int64 at = (juce::int64) eCell * ogran::kCell;
        const int S2 = S % 11 + 1;
        std::vector<eg::Event> ev { { at, [S, g, k, S2] (Proc& p)
        {
            for (int m = 0; m < 16; ++m)
            {
                const auto id = [m] (const char* f) { return "mod" + juce::String (m + 1) + "_" + f; };
                switch (k)
                {
                    case kAdd:       eg::setP (p, id ("source"), (float) S); break;
                    case kSrcNone:   eg::setP (p, id ("source"), 0.0f); break;
                    case kDestNone:  eg::setP (p, id ("dest"), 0.0f); break;
                    case kDepth0:    eg::setP (p, id ("depth"), 0.0f); break;
                    case kDepthStep: eg::setP (p, id ("depth"), -60.0f); break;
                    case kDestSwap:  eg::setP (p, id ("dest"), (float) (1 + 16 * g + (m + 1) % 16)); break;
                    case kSrcSwap:   eg::setP (p, id ("source"), (float) S2); break;
                    default: break;
                }
            }
        } } };
        return mx3Job (routes, std::move (ev), std::move (hk));
    }

    struct Mx3Verdict { double worst = 0.0; int violations = 0, crossTalk = 0; double dmax = 0.0; };
    /** Δv01 per cell vs 1.05 × the analytic bound (ModMatrix.h tick): Σ |depth|·max|Δsrc| over
        t−1…t+1 (two slots in the edit window of a swap) + the ramp terms (|Δdepth| + |residual|)
        / rampCells inside [edit, edit + rampCells + 2]; outside the group: constant. */
    inline Mx3Verdict mx3Check (const Mx3Trace& tr, int S, int g, int k, int eCell, int rampCells)
    {
        Mx3Verdict v;
        const int S2 = S % 11 + 1;
        const size_t n = tr.dest.size();
        auto dsrc = [&] (int s, size_t t) { return t >= 1 && t < n ? std::abs ((double) tr.src[t][(size_t) s] - (double) tr.src[t - 1][(size_t) s]) : 0.0; };
        const double K = (k == kDepthStep || k == kDestSwap || k == kSrcSwap) ? 1.2 : 0.6;
        for (int d = 1; d < ogran::kModDests; ++d)
        {
            const bool inGroup = d >= 1 + 16 * g && d < 1 + 16 * (g + 1);
            for (size_t t = 1; t < n; ++t)
            {
                const double dv = std::abs ((double) tr.dest[t][(size_t) d] - (double) tr.dest[t - 1][(size_t) d]);
                if (! inGroup)
                {
                    if (dv != 0.0) ++v.crossTalk;
                    continue;
                }
                const bool win = (int) t >= eCell && (int) t <= eCell + rampCells + 2;
                double ms = 0.0;
                for (size_t u = t - 1; u <= t + 1; ++u)
                {
                    ms = juce::jmax (ms, dsrc (S, u));
                    if (k == kSrcSwap) ms = juce::jmax (ms, dsrc (S2, u));
                }
                const double slots = win && (k == kDestSwap || k == kSrcSwap) ? 2.0 : 1.0;
                const double B = slots * 0.6 * ms + (win ? K / (double) rampCells : 0.0);
                const double r = dv / juce::jmax (B, 1e-12);
                v.dmax = juce::jmax (v.dmax, dv);
                if (dv > 1.05 * B + 1e-6) ++v.violations;
                if (dv > 2e-6) v.worst = juce::jmax (v.worst, B > 1e-9 ? r : 1e9);   // real moves only (float rounding sits in the 1e-6 slack)
            }
        }
        return v;
    }

    void mx3Rows()
    {
        std::printf ("\n-- MX3. QUAL-01 trajectory full cross: 11 sources x 112 destinations x 7 edit kinds (P87)\n");
        // Edit cell per source: argmax |src| in [0.4, 0.75] s of a route-free probe (an NC needs a large
        // contribution at the edit; the subject's bound holds at any cell).
        int eCell[12] {};
        int rampCells = 0;
        {
            Job pj = mx3Job ({}, {});
            auto tr = mx3Record (pj);
            auto o = eg25::renderAll ({ pj });
            rampCells = o[0].eng().getRampCells();
            juce::String det;
            for (int S = 1; S <= 11; ++S)
            {
                // the largest |src| whose neighbourhood (t − 3 … t + 3) is smooth (|Δsrc| < 0.02 per cell):
                // never the source's own attack / step cell, where the bound is wide anyway
                double best = -1.0;
                for (int c = 600; c <= 1125; ++c)
                {
                    double md = 0.0;
                    for (int u = c - 3; u <= c + 3; ++u)
                        md = juce::jmax (md, std::abs ((double) tr->src[(size_t) u][(size_t) S] - (double) tr->src[(size_t) u - 1][(size_t) S]));
                    const double v = std::abs (tr->src[(size_t) c][(size_t) S]);
                    if (md < 0.02 && v > best) { best = v; eCell[S] = c; }
                }
                if (best < 0.0) eCell[S] = 800;
                det << juce::String::formatted ("%d@%d (%.2f) ", S, eCell[S], best);
            }
            info ("MX3 edit cells (source@cell |src|), rampCells " + juce::String (rampCells) + ": " + det.trim());
        }
        for (int k = 0; k < kNumKinds; ++k)
        {
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<Mx3Trace>> trs;
            std::vector<std::pair<int, int>> sg;
            for (int S = 1; S <= 11; ++S)
                for (int g = 0; g < 7; ++g)
                {
                    jobs.push_back (mx3Render (S, g, k, eCell[S]));
                    trs.push_back (mx3Record (jobs.back()));
                    sg.push_back ({ S, g });
                }
            auto o = eg25::renderAll (jobs);
            Tally t;
            double worst = 0.0; int cross = 0;
            for (size_t i = 0; i < jobs.size(); ++i)
            {
                const auto v = mx3Check (*trs[i], sg[i].first, sg[i].second, k, eCell[sg[i].first], rampCells);
                worst = juce::jmax (worst, v.worst);
                cross += v.crossTalk;
                t.check (v.violations == 0 && v.crossTalk == 0,
                         juce::String::formatted ("src %d group %d: %d violations (worst %.2fx), cross-talk %d", sg[i].first, sg[i].second, v.violations, v.worst, v.crossTalk));
            }
            row (t.bad == 0, juce::String ("MX3") + (char) ('a' + k) + " " + kindName (k) + ": |dv01| per cell <= 1.05x the analytic bound, 11 x 112, no cross-talk",
                 t.detail (77) + juce::String::formatted ("  [worst %.3fx of B, cross-talk %d]", worst, cross));
        }
        // MX3nR — noResidual on (b); MX3nD — noDepthRamp on (a) and (e).
        auto ncRun = [&] (int k, std::function<void (ogran::Engine::TestHooks&)> hk, juce::String& det)
        {
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<Mx3Trace>> trs;
            std::vector<std::pair<int, int>> sg;
            for (int S = 1; S <= 11; ++S)
                for (int g = 0; g < 7; ++g)
                {
                    jobs.push_back (mx3Render (S, g, k, eCell[S], hk));
                    trs.push_back (mx3Record (jobs.back()));
                    sg.push_back ({ S, g });
                }
            auto o = eg25::renderAll (jobs);
            double perSrc[12] {};
            for (size_t i = 0; i < jobs.size(); ++i)
            {
                const auto v = mx3Check (*trs[i], sg[i].first, sg[i].second, k, eCell[sg[i].first], rampCells);
                perSrc[sg[i].first] = juce::jmax (perSrc[sg[i].first], v.worst);
            }
            bool all = true;
            for (int S = 1; S <= 11; ++S)
            {
                all = all && perSrc[S] > 2.0;
                det << juce::String::formatted ("%d:%.1fx ", S, perSrc[S]);
            }
            return all;
        };
        juce::String dR, dA, dE;
        const bool nR = ncRun (kSrcNone, [] (auto& h) { h.noResidual = true; }, dR);
        row (nR, "MX3nR noResidual on source -> None: > 2x the bound in >= 1 cell for every source", dR.trim());
        const bool nA = ncRun (kAdd, [] (auto& h) { h.noDepthRamp = true; }, dA);
        const bool nE = ncRun (kDepthStep, [] (auto& h) { h.noDepthRamp = true; }, dE);
        row (nA && nE, "MX3nD noDepthRamp on add and depth step: > 2x the bound in >= 1 cell for every source", "add " + dA.trim() + " | step " + dE.trim());
    }

    //==========================================================================
    // Shared κ helpers (P86): window κ, the linear-wet-sum precondition.
    //==========================================================================
    inline double kap (const Out& o, double t0, double t1) { return eg24b::kappa (o, t0, t1); }
    inline bool linearWet (const Out& o) { return o.eng().diag.maxWetPeak < 0.5f; }
    inline Stim sine440 (double amp = 0.001) { return eg::sineStim (440.0, 48000.0, amp); }
    /** The QB1 swell carrier rescaled to base `base` (440 Hz, +18 dB 1 ms-attack swells every 125 ms). */
    inline Stim swell (double lenS, double base)
    {
        auto x = eg25y::swellCarrier (48000.0, lenS);
        const float k = (float) (base / 0.03);
        for (auto& v : *x) v *= k;
        return eg25x::fromBuffer (x);
    }

    //==========================================================================
    // MF8 — audio κ on every application path (P88)
    //==========================================================================
    struct Mf8Dest { int d; const char* name; bool reverb; };
    inline std::vector<Mf8Dest> mf8Dests()
    {
        std::vector<Mf8Dest> v { { D::InputGain, "input_gain", false }, { D::OutputGain, "output_gain", false }, { D::Mix, "mix", false },
                                 { D::Feedback, "feedback", false }, { D::FbDiff, "fb_diffusion", false }, { D::FbSat, "fb_saturation", false } };
        static const char* lv[4] { "s1_level", "s2_level", "s3_level", "s4_level" };
        static const char* pn[4] { "s1_pan", "s2_pan", "s3_pan", "s4_pan" };
        for (int s = 0; s < 4; ++s) v.push_back ({ D::stream (s, D::Level), lv[s], false });
        for (int s = 0; s < 4; ++s) v.push_back ({ D::stream (s, D::Pan), pn[s], false });
        v.push_back ({ D::FbLowCut, "fb_low_cut", false });
        v.push_back ({ D::FbHighCut, "fb_high_cut", false });
        static const char* lr[4] { "lfo1_rate", "lfo2_rate", "lfo3_rate", "lfo4_rate" };
        for (int n = 0; n < 4; ++n) v.push_back ({ D::LfoRate1 + n, lr[n], false });
        v.push_back ({ D::RevMix, "reverb_mix", true });
        v.push_back ({ D::RevSize, "reverb_size", true });
        v.push_back ({ D::RevDamp, "reverb_damp", true });
        return v;
    }

    enum Mf8Edit { kMfAdd, kMfRemove, kMfStep };
    /** edit < 0: the held-route reference at `heldDepth`. */
    inline Job mf8Job (const Mf8Dest& md, bool onset, int edit, float heldDepth, std::function<void (ogran::Engine::TestHooks&)> hk = {})
    {
        Job j;
        j.fs = 48000.0; j.len = 3 * 48000; j.block = 128; j.prepBlock = 128;
        // mix 50 % (re-derived: at the 100 % base a Mix add clamps to a no-op — MF8n read 1.00x)
        j.params = eg22::base ({ 1, 2, 3, 4 }) + P { { "mix", 50.0f }, { "feedback", 50.0f }, { "reverb_mix", md.reverb ? 30.0f : 0.0f }, { "lfo1_shape", 0.0f },
                                                    { "lfo1_rate", 2.0f }, { "onset_threshold", -50.0f } };
        for (int s = 1; s <= 4; ++s)
            j.params = j.params + P { { sp (s, "density"), 20.0f }, { sp (s, "size"), 200.0f }, { sp (s, "spray"), 0.0f }, { sp (s, "jitter"), 20.0f },
                                      { sp (s, "window"), 0.0f }, { sp (s, "window_skew"), 0.0f } };
        if (md.d >= D::LfoRate1)
        {
            const int n = md.d - D::LfoRate1;   // carrier: LFO_N → s1_level at 30 %, held
            j.params = j.params + route (2, 1 + n, D::stream (0, D::Level), 30.0f) + P { { "lfo" + juce::String (n + 1) + "_rate", 2.0f } };
        }
        const int src = onset ? ogran::ModSrc::Onset : 1;
        const juce::int64 at = onset ? 99008 : 102016;   // Onset: >= 30 ms from every swell; LFO1 2 Hz: at its peak
        if (edit < 0)
            j.params = j.params + route (1, src, md.d, heldDepth);
        else if (edit == kMfAdd)
        {
            j.params = j.params + route (1, 0, md.d, 100.0f);
            j.events.push_back ({ at, [src] (Proc& p) { eg::setP (p, "mod1_source", (float) src); } });
        }
        else if (edit == kMfRemove)
        {
            j.params = j.params + route (1, src, md.d, 100.0f);
            j.events.push_back ({ at, [] (Proc& p) { eg::setP (p, "mod1_source", 0.0f); } });
        }
        else
        {
            j.params = j.params + route (1, src, md.d, 60.0f);
            j.events.push_back ({ at, [] (Proc& p) { eg::setP (p, "mod1_depth", -60.0f); } });
        }
        j.stim = onset ? swell (3.0, 0.001) : sine440();
        if (hk) j.pre = [hk] (Proc& p) { hk (p.getEngineForTest().hooks); };
        return j;
    }

    void mf8Rows()
    {
        std::printf ("\n-- MF8. QUAL-01 route edits on every application path: kappa vs the held-route reference (P88)\n");
        const auto ds = mf8Dests();
        juce::String info8;
        bool wetOk = true;
        for (int sk = 0; sk < 2; ++sk)
        {
            const bool onset = sk == 1;
            const double te = (onset ? 99008.0 : 102016.0) / 48000.0, t0 = te - 0.030, t1 = te + 0.050;
            // References: the route held at the pre-edit AND the post-edit state (the EN1 form):
            // add / remove → held 100 % and no route (depth 0); step → held +60 and held −60.
            // Measure: max curvature over the window vs the larger reference's (one common
            // normaliser). Execution deviation: per-render κ = maxCurv / RMS is biased by a
            // deliberate level change inside its own window (a 34 dB input-gain drop lowers
            // the subject's RMS, MF8c read 2.18x); plain κ stays in the detail as INFO.
            std::vector<Job> jobs;
            constexpr size_t kPer = 7;
            for (const auto& md : ds)
            {
                jobs.push_back (mf8Job (md, onset, -1, 100.0f));
                jobs.push_back (mf8Job (md, onset, -1, 60.0f));
                jobs.push_back (mf8Job (md, onset, -1, 0.0f));
                jobs.push_back (mf8Job (md, onset, -1, -60.0f));
                for (int e = 0; e < 3; ++e) jobs.push_back (mf8Job (md, onset, e, 0.0f));
            }
            auto o = eg25::renderAll (jobs);
            Tally t[3];
            double worst[3] {}, worstK[3] {};
            auto mc = [t0, t1] (const Out& x)
            {
                const size_t a = (size_t) (t0 * 48000.0), b = (size_t) (t1 * 48000.0);
                return juce::jmax (eg::maxCurv (x.L, a, b), eg::maxCurv (x.R, a, b));
            };
            for (size_t k = 0; k < ds.size(); ++k)
            {
                const auto& r100 = o[k * kPer], &r60 = o[k * kPer + 1], &r0 = o[k * kPer + 2], &rm60 = o[k * kPer + 3];
                for (int e = 0; e < 3; ++e)
                {
                    const auto& s = o[k * kPer + 4 + (size_t) e];
                    const auto& refA = e == kMfStep ? r60 : r100;
                    const auto& refB = e == kMfStep ? rm60 : r0;
                    const double ratio = mc (s) / juce::jmax (1e-30, juce::jmax (mc (refA), mc (refB)));
                    worstK[e] = juce::jmax (worstK[e], kap (s, t0, t1) / juce::jmax (1e-30, kap (refA, t0, t1)));
                    const bool lin = linearWet (s) && linearWet (refA) && linearWet (refB);
                    wetOk = wetOk && lin;
                    if (ds[k].reverb) { info8 << juce::String::formatted ("%s/%s/%s %.2fx; ", onset ? "Onset" : "LFO", ds[k].name, e == 0 ? "add" : e == 1 ? "remove" : "step", ratio); continue; }
                    worst[e] = juce::jmax (worst[e], ratio);
                    t[e].check (ratio <= 2.0 && lin, juce::String::formatted ("%s %.2fx%s", ds[k].name, ratio, lin ? "" : " (wet > 0.5)"));
                }
            }
            const char* en[3] { "add", "source -> None", "depth +60 -> -60" };
            for (int e = 0; e < 3; ++e)
                row (t[e].bad == 0, juce::String ("MF8") + (char) ('a' + 3 * sk + e) + " " + (onset ? "Onset" : "LFO1 2 Hz") + " " + en[e]
                                        + ": max curvature <= 2.0x the held pre / post-edit refs on the 20 gated paths (14 ramped + 6 tick)",
                     t[e].detail (20) + juce::String::formatted ("  [worst %.3fx; INFO plain kappa vs the pre-edit ref worst %.3fx]", worst[e], worstK[e]));
        }
        info ("MF8i reverb dests (kappa vs held ref, R92 tank confound; their gate is MX3): " + info8.trim());
        // MF8n — noDepthRamp add (LFO) on mix and s1_level.
        {
            const double te = 102016.0 / 48000.0, t0 = te - 0.030, t1 = te + 0.050;
            std::vector<Job> jobs;
            const Mf8Dest mx { D::Mix, "mix", false }, lv { D::stream (0, D::Level), "s1_level", false };
            for (const auto& md : { mx, lv })
            {
                jobs.push_back (mf8Job (md, false, -1, 100.0f));
                jobs.push_back (mf8Job (md, false, -1, 0.0f));
                jobs.push_back (mf8Job (md, false, kMfAdd, 0.0f, [] (auto& h) { h.noDepthRamp = true; }));
            }
            auto o = eg25::renderAll (jobs);
            auto mc = [t0, t1] (const Out& x)
            {
                const size_t ia = (size_t) (t0 * 48000.0), ib = (size_t) (t1 * 48000.0);
                return juce::jmax (eg::maxCurv (x.L, ia, ib), eg::maxCurv (x.R, ia, ib));
            };
            const double a = mc (o[2]) / juce::jmax (mc (o[0]), mc (o[1])), b = mc (o[5]) / juce::jmax (mc (o[3]), mc (o[4]));
            row (a > 2.0 && b > 2.0, "MF8n noDepthRamp add (LFO1 at its peak) on mix and s1_level: max curvature > 2.0x the held refs",
                 juce::String::formatted ("mix %.2fx, s1_level %.2fx", a, b));
        }
        if (! wetOk) std::printf ("  [NOTE] MF8 a render exceeded the linear-wet precondition (maxWetPeak >= 0.5)\n");
    }

    //==========================================================================
    // EN1 — stream enable / mute / solo under the Q set (P89)
    //==========================================================================
    void enableRows()
    {
        std::printf ("\n-- EN1. QUAL-01 stream enable / mute / solo under the Q set (2 streams), kappa vs max (held on, held off)\n");
        const juce::int64 t1 = 2 * 48000, t2 = 4 * 48000;
        struct Sw { const char* id; float on, off; bool startOff; };
        const Sw sws[3] { { "s2_enable", 1.0f, 0.0f, true }, { "s2_mute", 1.0f, 0.0f, true }, { "s1_solo", 1.0f, 0.0f, true } };
        auto job = [&] (const Sw& w, int mode)   // 0 edges (off → on at t1 → off at t2), 1 held on, 2 held off
        {
            Job j;
            j.fs = 48000.0; j.len = 5 * 48000; j.block = 128; j.prepBlock = 128;
            j.params = eg26::qSet() + P { { "s3_enable", 0.0f }, { "s4_enable", 0.0f } };
            j.stim = sine440();
            const juce::String id (w.id);
            j.params = j.params + P { { id, mode == 1 ? w.on : w.off } };
            if (mode == 0)
            {
                const float on = w.on, off = w.off;
                j.events.push_back ({ t1, [id, on] (Proc& p) { eg::setP (p, id, on); } });
                j.events.push_back ({ t2, [id, off] (Proc& p) { eg::setP (p, id, off); } });
            }
            j.seed = 0xE11ull;
            return j;
        };
        std::vector<Job> jobs;
        std::vector<std::shared_ptr<eg22::RecLog>> logs;
        for (const auto& w : sws)
            for (int m = 0; m < 3; ++m)
            {
                jobs.push_back (job (w, m));
                auto L = std::make_shared<eg22::RecLog>();
                jobs.back().after = eg22::collect (*L);
                logs.push_back (L);
            }
        auto o = eg25::renderAll (jobs);
        const char* nm[3] { "EN1a s2 enable", "EN1b s2 mute", "EN1c s1 solo" };
        double worstN = 1e30;
        juce::String nDet;
        for (int k = 0; k < 3; ++k)
        {
            const auto& s = o[(size_t) (3 * k)], &on = o[(size_t) (3 * k + 1)], &off = o[(size_t) (3 * k + 2)];
            Tally t;
            double worst = 0.0;
            // EN1n: the X1 hard-switch model from the held renders (off before t1, on in [t1, t2), off after).
            Out hard;
            hard.L = off.L; hard.R = off.R;
            for (juce::int64 i = t1; i < t2; ++i) { hard.L[(size_t) i] = on.L[(size_t) i]; hard.R[(size_t) i] = on.R[(size_t) i]; }
            for (juce::int64 te : { t1, t2 })
            {
                const double a = (double) te / 48000.0 - 0.030, b = (double) te / 48000.0 + 0.050;
                const double ref = juce::jmax (kap (on, a, b), kap (off, a, b));
                const double r = kap (s, a, b) / juce::jmax (1e-30, ref);
                // the model's κ (eg24b::kappa needs an engine-free Out: compute on the vectors)
                auto one = [&] (const std::vector<float>& x)
                {
                    const size_t ia = (size_t) (a * 48000.0), ib = (size_t) (b * 48000.0);
                    double e = 0.0;
                    for (size_t i = ia; i < ib; ++i) e += (double) x[i] * x[i];
                    return eg::maxCurv (x, ia, ib) / juce::jmax (1e-30, std::sqrt (e / (double) (ib - ia)));
                };
                const double rh = juce::jmax (one (hard.L), one (hard.R)) / juce::jmax (1e-30, ref);
                worst = juce::jmax (worst, r);
                worstN = juce::jmin (worstN, rh);
                nDet << juce::String::formatted ("%s @%.0f s %.1fx; ", nm[k] + 5, (double) te / 48000.0, rh);
                t.check (r <= 2.0, juce::String::formatted ("@%.0f s %.2fx", (double) te / 48000.0, r));
            }
            bool live = linearWet (s) && linearWet (on) && linearWet (off);
            int s2spawn = 0;
            if (k == 0)
                for (const auto& r : logs[(size_t) (3 * k)]->r) s2spawn += r.stream == 1 && r.onsetSample >= t1 && r.onsetSample < t1 + 960 ? 1 : 0;
            if (k == 0) live = live && s2spawn >= 1;
            row (t.bad == 0 && live, juce::String (nm[k]) + " off -> on @ 2 s -> off @ 4 s: kappa <= 2.0x max (held on, held off) in [t - 30, t + 50] ms",
                 t.detail (2) + juce::String::formatted ("  [worst %.3fx%s, wet %.3f]", worst, k == 0 ? (", s2 spawns within 20 ms of on: " + juce::String (s2spawn)).toRawUTF8() : "",
                                                        (double) s.eng().diag.maxWetPeak) + (live ? "" : "  VACUOUS / NONLINEAR"));
        }
        row (worstN > 2.0, "EN1n the X1 hard-switch model (held off / on spliced at the edges) > 2.0x at every edge", nDet.trim());
    }

    //==========================================================================
    // FZ9 — freeze engage / release under the Q set (P89; the FZ2 method)
    //==========================================================================
    void freezeRows()
    {
        std::printf ("\n-- FZ9. QUAL-01 freeze engage / release under the Q set (FZ2 method, same-motion controls)\n");
        const juce::int64 tFz = 2 * 48000, tRel = 5 * 48000;
        // FZ2's geometry inside the Q set (execution deviation, recorded): engage needs grains
        // crossing the head (spray 0, position 0; +24 st if the NC reads <= 2x — FZ2's own rule),
        // release needs grains reading back across W0 (spray 50 %). Q-set spray 100 % over the
        // 2 s span puts almost no grain near the head, so both NCs read ~1x there; reverse 100 %
        // means no grain reads forward across the head, and 4 x 200 g/s x 2 s clouds dilute one
        // grain's splice — FZ2's grain recipe (forward, 20 g/s, 200 ms, jitter 20 %) on the Q set's
        // FX, routes, feedback and analysis. Route 2 (LFO2 -> s2 pitch, +-14 st latched per grain)
        // alone still masked the engage NC (bisected: 1.99x with it, 7.1x without) -> retargeted
        // to s2 spray; the other 15 routes stay.
        // Engage renders end at 3 s (window 1.5–2.5 s): the never-frozen control's feedback-100 %
        // loop keeps building afterwards (wet 0.90 by 8 s), outside the window the precondition guards.
        auto job = [&] (std::vector<std::pair<juce::int64, float>> edges, std::function<void (ogran::Engine::TestHooks&)> hk, float spray, float pitch)
        {
            Job j;
            j.fs = 48000.0; j.len = (edges.size() == 2 || spray > 0.0f ? 8 : 3) * 48000; j.block = 512; j.prepBlock = 512;
            j.params = eg26::qSet() + perStream ("spray", spray) + perStream ("pitch", pitch) + perStream ("reverse_prob", 0.0f)
                     + perStream ("density", 20.0f) + perStream ("size", 200.0f) + perStream ("size_rand", 0.0f) + perStream ("jitter", 20.0f)
                     + P { { "mod2_dest", (float) D::stream (1, D::Spray) } };
            j.stim = sine440();
            j.seed = 0xF29ull;
            for (auto& e : edges) { const float v = e.second; j.events.push_back ({ e.first, [v] (Proc& p) { eg::setP (p, "freeze", v); } }); }
            if (hk) j.pre = [hk] (Proc& p) { hk (p.getEngineForTest().hooks); };
            return j;
        };
        float pitch = 0.0f;
        juce::String note;
        std::vector<Out> e;
        for (int pass = 0; pass < 2; ++pass)
        {
            e = eg25::renderAll ({ job ({ { tFz, 1.0f } }, {}, 0.0f, pitch), job ({}, {}, 0.0f, pitch),
                                   job ({ { tFz, 1.0f } }, [] (auto& h) { h.freezeNoDefer = true; }, 0.0f, pitch) });
            if (kap (e[2], 1.5, 2.5) / kap (e[1], 1.5, 2.5) > 2.0 || pass == 1) break;
            note = juce::String::formatted (" [re-derived at +24 st: the 0 st NC read %.3f]", kap (e[2], 1.5, 2.5) / kap (e[1], 1.5, 2.5));
            pitch = 24.0f;
        }
        auto r = eg25::renderAll ({ job ({ { tFz, 1.0f }, { tRel, 0.0f } }, {}, 50.0f, pitch), job ({ { tFz, 1.0f } }, {}, 50.0f, pitch),
                                    job ({ { tFz, 1.0f }, { tRel, 0.0f } }, [] (auto& h) { h.releaseSpliceOld = true; }, 50.0f, pitch) });
        const double ea = kap (e[0], 1.5, 2.5) / kap (e[1], 1.5, 2.5), en = kap (e[2], 1.5, 2.5) / kap (e[1], 1.5, 2.5);
        const double rb = kap (r[0], 5.0, 7.0) / kap (r[1], 5.0, 7.0), rn = kap (r[2], 5.0, 7.0) / kap (r[1], 5.0, 7.0);
        const auto& d0 = e[0].eng().diag; const auto& d3 = r[0].eng().diag;
        bool lin = true;
        for (auto& x : e) lin = lin && linearWet (x);
        for (auto& x : r) lin = lin && linearWet (x);
        row (ea <= 2.0 && d0.freezeEngages >= 1 && lin, "FZ9a freeze engage @ 2 s (Q set, spray 0 / position 0): kappa <= 2.0x the unfrozen control",
             juce::String::formatted ("ratio %.3f (engages %lld, forced releases %lld)", ea, (long long) d0.freezeEngages, (long long) d0.forcedReleases) + note);
        row (rb <= 2.0 && d3.freezeReleases >= 1 && lin, "FZ9b freeze release @ 5 s (Q set, spray 50 %): kappa <= 2.0x the still-frozen control",
             juce::String::formatted ("ratio %.3f (releases %lld)", rb, (long long) d3.freezeReleases));
        row (en > 2.0, "FZ9an freezeNoDefer > 2.0x", juce::String::formatted ("ratio %.3f", en));
        row (rn > 2.0, "FZ9bn releaseSpliceOld > 2.0x", juce::String::formatted ("ratio %.3f", rn));
        if (! lin)
        {
            std::printf ("  [NOTE] FZ9 a render exceeded the linear-wet precondition: wet peaks");
            for (auto& x : e) std::printf (" %.3f", (double) x.eng().diag.maxWetPeak);
            for (auto& x : r) std::printf (" %.3f", (double) x.eng().diag.maxWetPeak);
            std::printf ("\n");
        }
    }

    //==========================================================================
    // K3 — R3 kill / native steal under the Q set (P89; the K1 method)
    //==========================================================================
    /** The Q set with the demand-changing routes (Size, SizeRand, Density, BufferLength) retargeted to non-demand dests. */
    inline P qSetFixedDemand()
    {
        P p = eg26::qSet();
        for (int m = 0; m < 16; ++m)
        {
            const int d = eg26::kC7Dests[m];
            const bool demand = d == D::BufferLength || (d >= D::StreamBase && d < D::Feedback
                                && ((d - D::StreamBase) % D::StreamStride == D::Size || (d - D::StreamBase) % D::StreamStride == D::SizeRand
                                    || (d - D::StreamBase) % D::StreamStride == D::Density));
            if (demand)
            {
                const int s = (d - D::StreamBase) / D::StreamStride;
                p = p + P { { "mod" + juce::String (m + 1) + "_dest", (float) D::stream (s, D::FilterRes) } };
            }
        }
        return p;
    }

    inline double kapWin (const Out& o, juce::int64 from, juce::int64 to)
    {
        auto one = [&] (const std::vector<float>& x)
        {
            double e = 0.0;
            for (auto i = from; i < to; ++i) e += (double) x[(size_t) i] * x[(size_t) i];
            return eg::maxCurv (x, (size_t) from, (size_t) to) / juce::jmax (1e-30, std::sqrt (e / (double) (to - from)));
        };
        return juce::jmax (one (o.L), one (o.R));
    }

    struct CapJob { float density; int lim; double killMs = -1.0, stealMs = -1.0; };
    inline Job k3Job (const P& base, const CapJob& c, Stim stim, eg22::RecLog& L)
    {
        Job j;
        j.fs = 48000.0; j.len = 7 * 48000; j.block = 512; j.prepBlock = 512;
        j.params = base + P { { "grain_limit", (float) c.lim } };
        for (int s = 1; s <= 4; ++s)
            j.params = j.params + P { { sp (s, "density"), c.density }, { sp (s, "size"), 1000.0f }, { sp (s, "size_rand"), 0.0f } };
        j.stim = std::move (stim);
        j.seed = 0x6B33ull;
        const double km = c.killMs, sm = c.stealMs;
        j.pre = [km, sm] (Proc& p) { p.getEngineForTest().hooks.killFadeMs = km; p.getEngineForTest().hooks.stealFadeMs = sm; };
        j.after = eg22::collect (L);
        return j;
    }
    inline int inWin (const eg22::RecLog& L, juce::int64 from, juce::int64 to, bool stolen, bool burstOnly)
    {
        int n = 0;
        for (auto& r : L.r)
            if (r.onsetSample >= from && r.onsetSample < to && (stolen ? r.stolen : r.killed) && (! burstOnly || r.burst)) ++n;
        return n;
    }

    void stealKillRows()
    {
        std::printf ("\n-- K3 / QB3. QUAL-01 R3 kill / native steal (and burst kill / steal) under the Q set (K1 method, window 3-7 s)\n");
        constexpr juce::int64 from = 3 * 48000, to = 7 * 48000;
        // K3 — the sine carrier, triggers / follow on (no onsets on a steady sine).
        {
            const P base = qSetFixedDemand();
            eg22::RecLog Lr, La, Lb, Lan, Lbn;
            auto o = eg25::renderAll ({ k3Job (base, { 20.0f, 3 }, sine440(), Lr), k3Job (base, { 20.0f, 0 }, sine440(), La), k3Job (base, { 50.0f, 3 }, sine440(), Lb),
                                        k3Job (base, { 20.0f, 0, 0.0, -1.0 }, sine440(), Lan), k3Job (base, { 50.0f, 3, -1.0, 0.0 }, sine440(), Lbn) });
            const double kr = kapWin (o[0], from, to);
            const auto& dr = o[0].eng().diag;
            const bool refClean = dr.steals == 0 && dr.kills == 0 && dr.spawns > 400;
            const int ka = inWin (La, from, to, false, false), sb = inWin (Lb, from, to, true, false);
            const double ra = kapWin (o[1], from, to) / kr, rb = kapWin (o[2], from, to) / kr;
            const double ran = kapWin (o[3], from, to) / kr, rbn = kapWin (o[4], from, to) / kr;
            bool lin = true;
            for (auto& x : o) lin = lin && linearWet (x);
            row (refClean && ka >= 50 && ra <= 2.0 && lin && ! La.overflow, "K3a cap 32 (4 x 20 g/s x 1 s, R3 kills, Q set): kappa <= 2.0x the steal-free ref",
                 juce::String::formatted ("ratio %.3f; %d kills in window; ref %lld spawns, %lld steals, %lld kills", ra, ka, (long long) dr.spawns, (long long) dr.steals, (long long) dr.kills)
                     + (refClean && ka >= 50 ? "" : "  VACUOUS"));
            row (refClean && sb >= 50 && rb <= 2.0 && lin && ! Lb.overflow, "K3b cap 128 (4 x 50 g/s x 1 s, native steals, Q set): kappa <= 2.0x the ref",
                 juce::String::formatted ("ratio %.3f; %d steals in window", rb, sb) + (sb >= 50 ? "" : "  VACUOUS"));
            row (ran > 2.0, "K3an killFadeMs 0 (hard-cut kills) > 2.0x", juce::String::formatted ("ratio %.3f (%d kills)", ran, inWin (Lan, from, to, false, false)));
            row (rbn > 2.0, "K3bn stealFadeMs 0 (hard-cut steals) > 2.0x", juce::String::formatted ("ratio %.3f (%d steals)", rbn, inWin (Lbn, from, to, true, false)));
        }
        // QB3 — triggers on, the swell carrier, Hann.
        {
            const P base = qSetFixedDemand() + perStream ("window", 0.0f) + perStream ("window_skew", 0.0f) + P { { "onset_threshold", -50.0f } };
            eg22::RecLog Lr, La, Lb, Lan, Lbn;
            auto st = [] { return swell (7.0, 0.001); };
            auto o = eg25::renderAll ({ k3Job (base, { 20.0f, 3 }, st(), Lr), k3Job (base, { 20.0f, 0 }, st(), La), k3Job (base, { 50.0f, 3 }, st(), Lb),
                                        k3Job (base, { 20.0f, 0, 0.0, -1.0 }, st(), Lan), k3Job (base, { 50.0f, 3, -1.0, 0.0 }, st(), Lbn) });
            const double kr = kapWin (o[0], from, to);
            const auto& dr = o[0].eng().diag;
            const bool refClean = dr.steals == 0 && dr.kills == 0;
            const int ka = inWin (La, from, to, false, true), sb = inWin (Lb, from, to, true, true);
            const double ra = kapWin (o[1], from, to) / kr, rb = kapWin (o[2], from, to) / kr;
            const double ran = kapWin (o[3], from, to) / kr, rbn = kapWin (o[4], from, to) / kr;
            bool lin = true;
            for (auto& x : o) lin = lin && linearWet (x);
            const long long ev = o[1].eng().diag.onsetEvents;
            row (refClean && ka >= 50 && ra <= 2.0 && ev >= 20 && lin, "QB3a burst kills (triggers on, cap 32, 4 x 20 g/s, Hann, swell carrier): kappa <= 2.0x the cap-128 ref",
                 juce::String::formatted ("ratio %.3f; burst && killed %d in window; events %lld; ref steals %lld kills %lld bursts %lld", ra, ka, ev, (long long) dr.steals,
                                          (long long) dr.kills, (long long) dr.freeBursts) + (refClean && ka >= 50 ? "" : "  VACUOUS"));
            row (refClean && sb >= 50 && rb <= 2.0 && lin, "QB3b burst steals (cap 128, 4 x 50 g/s): kappa <= 2.0x the ref",
                 juce::String::formatted ("ratio %.3f; burst && stolen %d in window", rb, sb) + (sb >= 50 ? "" : "  VACUOUS"));
            row (ran > 2.0, "QB3an killFadeMs 0 > 2.0x", juce::String::formatted ("ratio %.3f", ran));
            row (rbn > 2.0, "QB3bn stealFadeMs 0 > 2.0x", juce::String::formatted ("ratio %.3f", rbn));
        }
    }

    //==========================================================================
    // M7 — PERF-01 whole-feature allocation gate, armed from block 0 (P91)
    //==========================================================================
    /** The P91 event script on job j (message side, between blocks). */
    inline void m7Script (Job& j, bool tempo)
    {
        auto ev = [&j] (juce::int64 at, std::function<void (Proc&)> f) { j.events.push_back ({ at, std::move (f) }); };
        const int major = eg25y::scaleChoice ("Major");
        // freeze ×4 (mid-cell)
        for (int k = 0; k < 4; ++k) { const float v = k % 2 == 0 ? 1.0f : 0.0f; ev (24000 + 72000 * k + 13, [v] (Proc& p) { eg::setP (p, "freeze", v); }); }
        // route edits from each of the 11 sources (7 kinds, rotating slots 1–16)
        juce::int64 t = 30011;
        for (int S = 1; S <= 11; ++S)
            for (int k = 0; k < 7; ++k)
            {
                const int m = 1 + (S * 7 + k) % 16;
                const juce::String id = "mod" + juce::String (m);
                std::function<void (Proc&)> f;
                switch (k)
                {
                    case 0: f = [id, S] (Proc& p) { eg::setP (p, id + "_source", (float) S); eg::setP (p, id + "_dest", (float) D::Mix); eg::setP (p, id + "_depth", 40.0f); }; break;
                    case 1: f = [id] (Proc& p) { eg::setP (p, id + "_source", 0.0f); }; break;
                    case 2: f = [id, S] (Proc& p) { eg::setP (p, id + "_source", (float) S); eg::setP (p, id + "_dest", 0.0f); }; break;
                    case 3: f = [id] (Proc& p) { eg::setP (p, id + "_dest", (float) D::RevSize); eg::setP (p, id + "_depth", 0.0f); }; break;
                    case 4: f = [id] (Proc& p) { eg::setP (p, id + "_depth", 60.0f); eg::setP (p, id + "_depth", -60.0f); }; break;
                    case 5: f = [id, S] (Proc& p) { eg::setP (p, id + "_dest", (float) D::stream (S % 4, D::Spray)); }; break;
                    default: f = [id, S] (Proc& p) { eg::setP (p, id + "_source", (float) (S % 11 + 1)); }; break;
                }
                ev (t, std::move (f));
                t += 5003;
            }
        ev (40003,  [] (Proc& p) { eg::setP (p, "s1_scale", 5.0f); });
        ev (60011,  [major] (Proc& p) { p.setDegreeMask (2, major, 0x95ull); });
        ev (80021,  [] (Proc& p) { juce::String b; p.setCustomScale ("", "edo 7", b); eg::setP (p, "s3_scale", (float) eg25y::scaleChoice ("Custom")); });
        ev (200001, [tempo] (Proc& p) { eg::setP (p, "sync_mode", tempo ? 0.0f : 1.0f); });
        ev (260003, [tempo] (Proc& p) { eg::setP (p, "sync_mode", tempo ? 1.0f : 0.0f); });
        ev (150007, [] (Proc& p) { p.setSeed (0x5EEDull); });
        ev (170009, [] (Proc& p) { eg::setP (p, "reverb_mix", 0.0f); });
        ev (220013, [] (Proc& p) { eg::setP (p, "reverb_mix", 50.0f); });
        ev (330017, [] (Proc& p) { eg::setP (p, "reverb_mix", 0.0f); });
        for (int k = 0; k < 6; ++k) { const float th = -60.0f + 10.0f * (float) k; ev (100009 + 9600 * k, [th] (Proc& p) { eg::setP (p, "onset_threshold", th); }); }
        ev (300013, [] (Proc& p) { eg::setP (p, "grain_limit", 0.0f); });   // 6.25 s: Tempo transport still playing (stop 7-8 s)
        // preset-like mass change: every parameter rewritten between two blocks
        ev (420001, [] (Proc& p)
        {
            int i = 0;
            for (auto* ap : p.getParameters()) ap->setValueNotifyingHost (eg26::fuzzU (0x77ull, 7, 0, i++));
            eg::setP (p, "mix", 100.0f); eg::setP (p, "s1_enable", 1.0f); eg::setP (p, "s2_enable", 1.0f);
        });
        ev (480007, [] (Proc& p) { eg::setAll (p, c7Params()); });
        // injections (hooks; message side before the block)
        ev (250001, [] (Proc& p) { p.getEngineForTest().hooks.injectFbNan = { 250100, false, false }; });
        ev (270001, [] (Proc& p) { p.getEngineForTest().hooks.injectEnvNanAt = 270100; });
        ev (290001, [] (Proc& p) { p.getEngineForTest().hooks.injectAnalysis = { 290100, std::numeric_limits<float>::quiet_NaN(), false }; });
        ev (310001, [] (Proc& p)
        {
            auto& e = p.getEngineForTest();
            int slot = 0;
            for (int s = 0; s < ogran::kPoolLive; ++s) if (e.getPool().isActive (s)) { slot = s; break; }
            e.hooks.injectNan = { slot, 310100, 0, std::numeric_limits<float>::quiet_NaN(), false };
        });
        juce::ignoreUnused (tempo);
    }

    void allocRows()
    {
        std::printf ("\n-- M7. PERF-01 allocations over the whole feature set, armed from block 0 (P91)\n");
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
        int totalF = 0, totalT = 0;
        juce::String det;
        bool countersOk = true;
        juce::String cdet;
        for (int form = 0; form < 2; ++form)
            for (int sched = 0; sched < 2; ++sched)
            {
                const bool tempo = form == 1;
                Job j;
                j.fs = 48000.0; j.len = 12 * 48000; j.seed = 0xA77Cull;
                j.block = sched == 0 ? 0 : 8192;
                j.prepBlock = 512;
                j.params = c7Params() + P { { "sync_mode", tempo ? 1.0f : 0.0f } };
                // host NaN in the stimulus at 2.5 s (ring-write guard)
                auto base = c7Stim (j.fs);
                j.stim = [base] (juce::int64 i, int ch) { return i == 120007 && ch == 0 ? std::numeric_limits<float>::quiet_NaN() : base (i, ch); };
                m7Script (j, tempo);
                if (tempo)
                {
                    Transport tr (48000.0, 120.0);
                    tr.tempo (48017, 97.3).loopWrap (192005, 4.0, 12.0).locate (288011, 21.5).stop (336001).start (384001).timeSig (432001, 7, 8);
                    eg25::withTransport (j, tr);
                }
                armDuringProcess = true;    // armed from block 0 (R97)
                const auto o = render (j);
                armDuringProcess = false;
                const auto& d = o.eng().diag;
                (tempo ? totalT : totalF) += allocCount;
                det << (tempo ? "Tempo" : "Free") << (sched == 0 ? " var" : " 8192") << juce::String::formatted (": %d allocs; ", (int) allocCount);
                allocCount = 0;
                const long long c[] { d.spawns, d.steals, d.kills, d.onsetEvents, d.mpmHops, d.followHits, d.freeBursts + d.tempoBursts, d.routeChanges,
                                      d.fbScrubs, d.envScrubs, d.analysisScrubs, d.hostScrubs, d.nanKills, d.freezeEngages, d.freezeReleases, d.customCopies,
                                      tempo ? d.tempoReanchors : 1, tempo ? d.tempoRelocks : 1 };
                const char* cn[] { "spawns", "steals", "kills", "events", "hops", "followHits", "bursts", "routeChanges", "fbScrubs", "envScrubs",
                                   "analysisScrubs", "hostScrubs", "nanKills", "engages", "releases", "customCopies", "reanchors", "relocks" };
                for (size_t k = 0; k < sizeof (c) / sizeof (c[0]); ++k)
                    if (c[k] < 1) { countersOk = false; cdet << (tempo ? "T" : "F") << (sched == 0 ? "var " : "8192 ") << cn[k] << " 0; "; }
                if (form == 1 && sched == 0)
                    cdet << juce::String::formatted ("[Tempo var: spawns %lld steals %lld kills %lld events %lld hops %lld hits %lld bursts %lld routes %lld scrubs %lld/%lld/%lld/%lld nanKills %lld "
                                                     "engages %lld releases %lld custom %lld reanchors %lld relocks %lld]",
                                                     d.spawns, d.steals, d.kills, d.onsetEvents, d.mpmHops, d.followHits, d.freeBursts + d.tempoBursts, d.routeChanges,
                                                     d.fbScrubs, d.envScrubs, d.analysisScrubs, d.hostScrubs, d.nanKills, d.freezeEngages, d.freezeReleases, d.customCopies,
                                                     d.tempoReanchors, d.tempoRelocks);
            }
        malloc_logger = nullptr;
        allocTrace = false;
        row (liveness == 1, "M7L liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && totalF == 0, "M7F Free, var + fresh 8192 (prep 512), armed from block 0: 0 allocations over the whole feature + event script", det.trim());
        row (liveness == 1 && totalT == 0, "M7T Tempo (step, loop, locate, stop / start, 7/8), var + fresh 8192: 0 allocations", det.trim());
        row (countersOk, "M7c every feature counter >= 1 in every render (spawns ... custom copies, re-anchors / re-locks in Tempo)", cdet.trim());
       #else
        row (false, "M7 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // N10 / O2 — PERF-03 (P92)
    //==========================================================================
    struct RevProbe { long long at65 = -1, at80 = -1, at95 = -1, at110 = -1; };
    inline Job n10Base (bool tempo, std::shared_ptr<RevProbe> rp)
    {
        const double fs = 48000.0;
        Job j = eg25y::n9Base (tempo);
        // the C7 feature set on top of N9's input, events and transport; fb 80 %, crush kept on at 50 %
        j.params = j.params + c7Params() + P { { "feedback", 80.0f } } + perStream ("crush", 50.0f) + P { { "sync_mode", tempo ? 1.0f : 0.0f } };
        if (tempo)   // N9's density_sync 1/16.. gives no over-demand in Tempo: the R82 "MAX Tempo 1/64T" sync (steals live)
            j.params = j.params + perStream ("density_sync", 0.0f) + perStream ("size_sync", 19.0f);
        auto ev = [&j] (juce::int64 at, std::function<void (Proc&)> f) { j.events.push_back ({ at, std::move (f) }); };
        ev (150011, [] (Proc& p) { eg::setP (p, "mod3_source", 0.0f); });
        ev (170023, [] (Proc& p) { eg::setP (p, "mod5_dest", (float) D::stream (1, D::Spray)); });
        ev (190027, [] (Proc& p) { eg::setP (p, "mod7_depth", -40.0f); });
        ev (300007, [] (Proc& p) { eg::setP (p, "reverb_mix", 0.0f); });     // R100: running → idle
        ev (460811, [] (Proc& p) { eg::setP (p, "reverb_mix", 60.0f); });    // re-enable (cold tank) at 9.6 s
        auto prev = j.after;
        j.after = [prev, rp] (Proc& p, juce::int64 pos)
        {
            if (prev) prev (p, pos);
            const auto rc = p.getEngineForTest().diag.reverbCells;
            if (rp->at65 < 0 && pos >= 312000) rp->at65 = rc;
            if (rp->at80 < 0 && pos >= 384000) rp->at80 = rc;
            if (rp->at95 < 0 && pos >= 456000) rp->at95 = rc;
            if (rp->at110 < 0 && pos >= 528000) rp->at110 = rc;
        };
        juce::ignoreUnused (fs);
        return j;
    }

    void blockNullRows()
    {
        std::printf ("\n-- N10. PERF-03 full-feature block null (C7 set + N9 elements, fb 80 %%, crush 50 %%, reverb edge) incl. the fresh 8192 host block\n");
        for (int form = 0; form < 2; ++form)
        {
            const bool tempo = form == 1;
            std::vector<Job> jobs;
            std::vector<std::shared_ptr<eg25x::EvLog>> ev;
            std::vector<std::shared_ptr<eg25x::HopLog>> hp;
            std::vector<std::shared_ptr<RevProbe>> rps;
            struct V { int block, prep; const char* name; };
            const V vs[] { { 32, 512, "32" }, { 441, 512, "441" }, { 512, 512, "512" }, { 4096, 512, "4096" }, { 0, 512, "var" },
                           { 8192, 512, "8192 (prep 512)" }, { 8192, 1, "8192 (prep 1)" } };
            for (const auto& v : vs)
            {
                auto rp = std::make_shared<RevProbe>();
                Job j = n10Base (tempo, rp);
                j.block = v.block; j.prepBlock = v.prep;
                ev.push_back (eg25x::evInto (j));
                hp.push_back (eg25x::hopsInto (j));
                rps.push_back (rp);
                jobs.push_back (j);
            }
            { auto rp = std::make_shared<RevProbe>(); Job s = n10Base (tempo, rp); s.block = 32; s.seed ^= 0x1ull; jobs.push_back (s); rps.push_back (rp); }
            if (tempo)
            {
                auto rp = std::make_shared<RevProbe>();
                Job n = n10Base (tempo, rp); n.block = 441;
                auto pre = n.pre;
                n.pre = [pre] (Proc& p) { if (pre) pre (p); p.getEngineForTest().hooks.perBlockOrigin = true; };
                jobs.push_back (n); rps.push_back (rp);
            }
            auto o = eg25::renderAll (jobs);
            Tally t;
            for (size_t k = 1; k < 7; ++k)
            {
                const int n = eg25y::vdiff (o[0], o[k]);
                const bool same = ev[0]->at == ev[k]->at && eg25y::sameHops (*hp[0], *hp[k]) && eg25y::sameRing (eg25x::an (o[0]).getRing(), eg25x::an (o[k]).getRing());
                t.check (n == 0 && same && o[k].eng().guardsIntact(), juce::String (vs[k].name) + juce::String::formatted (": %d diffs%s", n, same ? "" : ", analysis state differs"));
            }
            const auto& d = o[0].eng().diag;
            const auto& rp = *rps[0];
            const bool revEdge = rp.at65 >= 0 && rp.at80 == rp.at95 && rp.at110 > rp.at95 && rp.at65 > 0;
            bool live = d.onsetEvents > 0 && d.mpmConfident > 0 && d.followHits > 0 && d.steals > 0 && revEdge
                        && (tempo ? d.tempoBursts > 0 && d.tempoReanchors > 0 : d.freeBursts > 0);
            row (t.bad == 0 && live, juce::String (tempo ? "N10T Tempo" : "N10F Free") + ": 441/512/4096/var/8192 fresh (prep 512, prep 1) vs 32: max|d| == 0; events, hops, f0 ring; canaries",
                 t.detail (6) + juce::String::formatted ("  [events %lld bursts %lld confident %lld hits %lld steals %lld re-anchors %lld; reverbCells %lld -> %lld = %lld -> %lld]",
                                                         (long long) d.onsetEvents, (long long) (tempo ? d.tempoBursts : d.freeBursts), (long long) d.mpmConfident,
                                                         (long long) d.followHits, (long long) d.steals, (long long) d.tempoReanchors, rp.at65, rp.at80, rp.at95, rp.at110)
                     + (live ? "" : "  VACUOUS"));
            const int ds = eg25y::vdiff (o[0], o[7]);
            if (! tempo)
                row (ds > 0, "N10c seed + 1 differs (Free)", juce::String (ds) + " diffs");
            else
            {
                const int dn = eg25y::vdiff (o[0], o[8]);
                row (ds > 0 && dn > 0, "N10n perBlockOrigin (Tempo) breaks the 441 vs 32 null; N10c (Tempo) seed + 1 differs",
                     juce::String::formatted ("perBlockOrigin %d diffs; seed + 1 %d diffs", dn, ds));
            }
        }

        // O2 — the O1 poison test under the C7 feature set.
        {
            auto p = make();
            eg::setAll (*p, c7Params());
            p->setSeed (0x0B52ull);
            p->prepareToPlay (48000.0, 512);
            constexpr int kGuard = 256;
            constexpr float kPoison = -12345.678f;
            std::vector<float> mem[2];
            for (auto& m : mem) m.assign (8192 + 2 * kGuard, kPoison);
            bool finite = true, hostGuards = true;
            juce::MidiBuffer midi;
            juce::int64 pos = 0;
            const auto st = c7Stim (48000.0, 0.25f);
            for (int k = 0; k < 24; ++k)
            {
                const int n = (k % 2 == 0) ? 8192 : 4096;
                float* chans[2] = { mem[0].data() + kGuard, mem[1].data() + kGuard };
                for (int ch = 0; ch < 2; ++ch)
                {
                    std::fill (mem[ch].begin(), mem[ch].end(), kPoison);
                    for (int i = 0; i < n; ++i) chans[ch][i] = st (pos + i, ch);
                }
                juce::AudioBuffer<float> buf (chans, 2, n);
                p->processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch)
                {
                    for (int i = 0; i < n; ++i) finite = finite && std::isfinite (chans[ch][i]);
                    for (int i = 0; i < kGuard; ++i) hostGuards = hostGuards && juce::exactlyEqual (mem[ch][(size_t) i], kPoison);
                    for (int i = kGuard + n; i < (int) mem[ch].size(); ++i) hostGuards = hostGuards && juce::exactlyEqual (mem[ch][(size_t) i], kPoison);
                }
                pos += n;
            }
            auto& e = p->getEngineForTest();
            row (finite && hostGuards && e.guardsIntact() && e.diag.reachViolations == 0 && e.diag.spawns > 500,
                 "O2 oversize under the C7 feature set: prepare 512, blocks 8192 / 4096: finite, host guards, canaries, reach 0",
                 juce::String::formatted ("finite %d, host guards %d, canaries %d, reach %lld, spawns %lld", (int) finite, (int) hostGuards, (int) e.guardsIntact(),
                                          (long long) e.diag.reachViolations, (long long) e.diag.spawns));
        }
    }
} } // namespace eg26b

static void engineRows26b()
{
    std::printf ("\n== Stage 2 / 2.6 PERF-02 / QUAL-01 / PERF-01 / PERF-03 rows\n");
    if (sect ("cpuRows")) eg26b::cpuRows();
    if (sect ("capRows")) eg26b::capRows();
    if (sect ("mx3Rows")) eg26b::mx3Rows();
    if (sect ("mf8Rows")) eg26b::mf8Rows();
    if (sect ("enableRows")) eg26b::enableRows();
    if (sect ("freezeRows")) eg26b::freezeRows();
    if (sect ("stealKillRows")) eg26b::stealKillRows();
    if (sect ("allocRows")) eg26b::allocRows();
    if (sect ("blockNullRows")) eg26b::blockNullRows();
}
