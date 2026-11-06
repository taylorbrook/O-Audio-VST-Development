/*
  ==============================================================================

    O-Granulator — render harness, section F/Q/D (Stage 2 / Phase 2.2 rows,
    PLAN-2.2 P16, Wave 5: T12, T13, T15). T14 / T16 rows: engine_gates_22b.h.

    Included by main.cpp AFTER engine_gates.h (reuses eg::Job / render / setP /
    diff / rmsDb / fft / hashNoise and main.cpp's row / Tally / make). Every row
    prints PASS/FAIL and counts toward the exit code. Seeded; stimuli are
    POSITION-deterministic. Negative controls that pass anyway FAIL as vacuous.

    Level probes (F4, F5): stationary band-limited noise (127-tap Blackman sinc
    FIR at 0.2·fs over hashNoise, a fixed table indexed by absolute sample), c = 0
    (spray 10 %), spread 0, Hann. Warm-up 4.5 s (>= 2 × the largest size, 2 s,
    plus half a 1 g/s interval so no small-stream grain straddles a window
    edge); window 16 s. Per-stream power through P15 solo renders. TARGET =
    σ²_in · g_out² · P_s from the quota law — waterFill (demands, Q) → q_s →
    streamPower (N_s, q_s, Hann, c 0, spread 0) → outputGainFromPower — never
    from a render.

      F1  each stream alone audible (> −40 dBFS), log only that stream, 0 plan
          calls on the others
      F2  s2–s4 disabled across 4 s with s1 enable toggles: 0 plan / 0 scheduler
          calls / 0 onsets; s3 enabled at a grid point fires its first onset on
          that sample (and before it: 0 calls); an off-grid enable fires at the
          next grid point
      F3  4 streams 200 g/s × 2 s, grain_limit 32/64/96/128: maxAlive <= cap
          (engine per spawn + harness scan per 32-sample block), == cap; kills > 0
          below 128, steals > 0 at 128 (= kPoolLive: no free slot ever, so the
          cap is the native steal — PLAN "kills > 0" erratum)
      F4  RESEARCH §3 "400, 50, 10, 1 @ 64": each stream ±1.5 dB of its quota-law
          target; F4n nativeProgress → smallest stream < −10 dB
      F5  the six RESEARCH §3 cases ±1.0 dB; "60, 60, 0.1, 0.1 @ 96" with the
          noQuotaCeil control (pooled small streams < −0.5 dB)
      F6  spawn logs (slot, onset, delay, …) identical: no solo / solo s2 /
          mute s1 (capped, stealing); solo / mute really change the output
      F7  unit: one stream — Spawner::acquire victim / slot == the 2.1 acquire
          (verbatim reference) over random pools, limits, quotas, both R8 forms
      Q1  31 collections (BP tritave, a non-default Custom with period 1300):
          every record's offset within 1 ¢ of an active member in the D4 window
          [max (−span, −2400 − 100p), min (span, 2400 − 100p)], p ∈ {0, +5.3,
          −20, +20} (one per stream); active sets == independent tiling
      Q2  empty mask / no degree in span / no degree in window: rate ==
          exp2 (p/12) EXACTLY, and the pitch_rand 7 render == pitch_rand 0
          render bitwise (control: a non-empty mask differs)
      Q3  7-Limit Just: source unsorted (vacuity), active array sorted; mask
          only 7/4 → every pick is 7/4 (± period) exactly, never 12/7
      Q4  spec index ≠ gs-Id, 1-based stream: s3 Chromatic {1, 3}, s4 Major
          {2, 4}, decoy masks elsewhere — picks only from the own set
      Q5  1 kHz sine, 5-Limit Just only 5/4, pitch_rand 5 st, spray 10 %:
          isolated grains (2 g/s × 200 ms, jitter 20 %), summed power spectra,
          2^17 zero-padded FFT, parabolic (log) peak → 1250 Hz ± 1 ¢; control
          only 6/5 → 1200 Hz ± 1 ¢. (A nonnegative-envelope tone's magnitude
          spectrum is symmetric about f0, so the peak is exact per grain and
          the incoherent sum cannot speckle — R11.)
      Q6  Custom swap mid-render (edo 7 → edo 5): picks from onset >= c + 2
          grids all new-table, every pick before c old-table; 1 copy, 0 misses
      Q7  pitch +0.37 st, pitch_rand 0, scale on: rate == exp2 (0.37f/12)
      D1  reverse 0 % none, 100 % all, 50 % within 4σ (n >= 2000)
      D2  spray 0: delay == margin + pos·span exactly (floor not binding);
          spread 0 / gain_rand 0: gainL == gainR == 1.0f (control spray 10 %)
      D3  spread 100 %: gL² + gR² == 2 (1e-6); mean gL² vs gR² within 0.2 dB
          over >= 8000 grains at stream pan 0 / +1 / −1; audio L/R at pan 0
          within 0.2 dB, at pan +0.5 == the bus balance within 0.2 dB
      D4  gain_rand 0: gains 1.0f; gain_rand 1e-6 % (a draw whose product rounds
          to 1.0f) renders bitwise == 0 (draws do not perturb other slots);
          100 %: every gain in [10^(−24/20), 1], range exercised
      D5  draw-before-obtain: 4 streams, everything random, cap 32 (steals /
          kills) vs cap 128 (steal-free): every record (stream, globalIndex)
          has identical draw-derived fields, incl. those after the first kill

  ==============================================================================
*/
#pragma once

#include <atomic>
#include <bit>
#include <map>
#include <set>
#include <thread>

namespace { namespace eg22
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using Rec = ogran::Engine::SpawnRecord;
    constexpr double kFs = 48000.0;

    juce::String sp (int s, const char* name) { return "s" + juce::String (s) + "_" + name; }

    //==========================================================================
    // Band-limited stationary noise: a fixed table, indexed by absolute sample.
    struct BlNoise
    {
        static constexpr int kLen = 42 * 48000;
        std::vector<float> t[2];
        double power[2] {};
    };
    BlNoise& blNoise()
    {
        static BlNoise b = []
        {
            BlNoise n;
            constexpr int taps = 127, half = taps / 2;
            constexpr double fc = 0.2;   // × fs
            double h[taps];
            for (int k = 0; k < taps; ++k)
            {
                const double m = k - half;
                const double sinc = m == 0 ? 2.0 * fc : std::sin (2.0 * eg::kPi * fc * m) / (eg::kPi * m);
                const double w = 0.42 - 0.5 * std::cos (2.0 * eg::kPi * k / (taps - 1)) + 0.08 * std::cos (4.0 * eg::kPi * k / (taps - 1));
                h[k] = sinc * w;
            }
            for (int ch = 0; ch < 2; ++ch)
            {
                n.t[ch].resize ((size_t) BlNoise::kLen);
                double acc2 = 0.0;
                for (int i = 0; i < BlNoise::kLen; ++i)
                {
                    double y = 0.0;
                    for (int k = 0; k < taps; ++k)
                        y += h[k] * (double) eg::hashNoise ((juce::int64) i - k + 1000000, ch, 0.9f);
                    n.t[ch][(size_t) i] = (float) y;
                    acc2 += y * y;
                }
                n.power[ch] = acc2 / BlNoise::kLen;
            }
            return n;
        }();
        return b;
    }
    std::function<float (juce::int64, int)> blStim()
    {
        auto* b = &blNoise();
        return [b] (juce::int64 i, int ch) { return b->t[ch != 0 ? 1 : 0][(size_t) (i % BlNoise::kLen)]; };
    }

    //==========================================================================
    // Spawn-record collector (drains the 4096 ring after every block).
    struct RecLog
    {
        std::vector<Rec> r;
        std::uint64_t seen = 0;
        bool overflow = false;
    };
    std::function<void (Proc&, juce::int64)> collect (RecLog& L, std::function<void (Proc&, juce::int64)> also = {})
    {
        return [&L, also] (Proc& p, juce::int64 pos)
        {
            const auto& d = p.getEngineForTest().diag;
            if (d.logCount - L.seen > (std::uint64_t) ogran::Engine::kSpawnLogSize) L.overflow = true;
            for (std::uint64_t k = L.seen; k < d.logCount; ++k) L.r.push_back (d.record (k));
            L.seen = d.logCount;
            if (also) also (p, pos);
        };
    }

    //==========================================================================
    // Parallel renders: processors are built / prepared on THIS thread (APVTS,
    // parameters), only processBlock runs on workers (each job owns its
    // processor — the audio-thread contract). Jobs here carry no events.
    Out setupOut (const Job& j)
    {
        using CS = juce::AudioChannelSet;
        Out o;
        o.p = make();
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (j.inCh == 1 ? CS::mono() : CS::stereo());
        l.outputBuses.add (j.outCh == 1 ? CS::mono() : CS::stereo());
        o.p->setBusesLayout (l);
        eg::setAll (*o.p, j.params);
        o.p->setSeed (j.seed);
        if (j.pre) j.pre (*o.p);
        o.p->prepareToPlay (j.fs, j.prepBlock);
        return o;
    }
    void runLoop (const Job& j, Out& o)
    {
        const int numCh = juce::jmax (j.inCh, j.outCh);
        o.L.assign ((size_t) j.len, 0.0f);
        o.R.assign ((size_t) j.len, 0.0f);
        juce::AudioBuffer<float> blk (numCh, 8192);
        juce::MidiBuffer midi;
        juce::int64 pos = 0;
        while (pos < j.len)
        {
            const int ni = (int) juce::jmin ((juce::int64) j.block, j.len - pos);
            blk.setSize (numCh, ni, false, false, true);
            for (int ch = 0; ch < numCh; ++ch)
                for (int i = 0; i < ni; ++i)
                    blk.setSample (ch, i, ch < j.inCh ? j.stim (pos + i, ch) : 0.0f);
            o.p->processBlock (blk, midi);
            const float* l0 = blk.getReadPointer (0);
            const float* r0 = blk.getReadPointer (j.outCh > 1 ? 1 : 0);
            std::copy (l0, l0 + ni, o.L.begin() + pos);
            std::copy (r0, r0 + ni, o.R.begin() + pos);
            pos += ni;
            if (j.after) j.after (*o.p, pos);
        }
    }
    void renderBatch (const std::vector<Job>& jobs, const std::function<void (size_t, Out&)>& consume)
    {
        const size_t hw = juce::jmax (1u, std::thread::hardware_concurrency());
        for (size_t b0 = 0; b0 < jobs.size(); b0 += hw)
        {
            const size_t b1 = juce::jmin (jobs.size(), b0 + hw);
            std::vector<Out> outs;
            for (size_t i = b0; i < b1; ++i) { jassert (jobs[i].events.empty()); outs.push_back (setupOut (jobs[i])); }
            std::vector<std::thread> th;
            for (size_t i = b0; i < b1; ++i)
                th.emplace_back ([&jobs, &outs, i, b0] { runLoop (jobs[i], outs[i - b0]); });
            for (auto& t : th) t.join();
            for (size_t i = b0; i < b1; ++i) consume (i, outs[i - b0]);
        }
    }

    double meanPow (const Out& o, size_t from, size_t to)
    {
        double a = 0.0;
        for (size_t i = from; i < to && i < o.L.size(); ++i) a += (double) o.L[i] * o.L[i] + (double) o.R[i] * o.R[i];
        return a / (2.0 * (double) (to - from));
    }
    double db (double p) { return p > 0.0 ? 10.0 * std::log10 (p) : -400.0; }

    // Base for 2.2 rows: wet only, spread 0 on every stream (unity grain gains),
    // only the listed streams enabled.
    P base (std::initializer_list<int> enabled)
    {
        P p { { "mix", 100.0f } };
        for (int s = 1; s <= 4; ++s)
        {
            bool on = false;
            for (int e : enabled) on = on || e == s;
            p.push_back ({ sp (s, "enable"), on ? 1.0f : 0.0f });
            p.push_back ({ sp (s, "stereo_spread"), 0.0f });
        }
        return p;
    }

    //==========================================================================
    // T12 — FUNC-03
    //==========================================================================
    void streamsAloneRows()
    {
        std::printf ("\n-- F1-F2. FUNC-03 streams alone / disabled = nothing\n");
        Tally t;
        juce::String det;
        for (int k = 1; k <= 4; ++k)
        {
            Job j; j.len = (juce::int64) (2.0 * kFs);
            j.params = base ({ k });
            j.params.push_back ({ sp (k, "density"), 40.0f });
            RecLog L; j.after = collect (L);
            const auto o = eg::render (j);
            const auto& d = o.eng().diag;
            int other = 0;
            for (auto& r : L.r) other += r.stream != k - 1 ? 1 : 0;
            std::int64_t otherPlans = 0;
            for (int s = 0; s < 4; ++s) if (s != k - 1) otherPlans += d.planCalls[s] + d.schedCalls[s];
            const double lvl = 0.5 * (eg::rmsDb (o.L, (size_t) (0.5 * kFs), o.L.size()) + eg::rmsDb (o.R, (size_t) (0.5 * kFs), o.R.size()));
            t.check (lvl > -40.0 && other == 0 && otherPlans == 0 && L.r.size() > 50 && ! L.overflow,
                     "s" + juce::String (k) + juce::String::formatted (" %.1f dB other %d", lvl, other));
            det << juce::String::formatted ("s%d %.1f dB %d rec; ", k, lvl, (int) L.r.size());
        }
        row (t.bad == 0, "F1 each stream alone: audible > -40 dBFS, log only that stream", t.detail (4) + "  " + det);

        // F2a: s2-s4 disabled, s1 toggled.
        {
            Job j; j.len = (juce::int64) (4.0 * kFs); j.block = 512;
            j.params = base ({ 1 });
            j.params.push_back ({ "s1_density", 50.0f });
            for (auto [at, v] : { std::pair<double, float> { 1.0, 0.0f }, { 1.5, 1.0f }, { 2.5, 0.0f }, { 3.0, 1.0f } })
                j.events.push_back ({ (juce::int64) (at * kFs), [v] (Proc& p) { eg::setP (p, "s1_enable", v); } });
            RecLog L; j.after = collect (L);
            const auto o = eg::render (j);
            const auto& d = o.eng().diag;
            int other = 0;
            for (auto& r : L.r) other += r.stream != 0 ? 1 : 0;
            std::int64_t calls = 0;
            for (int s = 1; s < 4; ++s) calls += d.planCalls[s] + d.schedCalls[s];
            const bool live = d.planCalls[0] > 100 && o.eng().getEpoch (0) > 0;   // toggles really happened
            row (calls == 0 && other == 0 && live,
                 "F2 s2-s4 disabled (s1 toggled x4): 0 plan, 0 scheduler calls, 0 onsets",
                 juce::String::formatted ("s2-4 calls %lld onsets %d; s1 plans %lld epoch %llu", (long long) calls, other,
                                          (long long) d.planCalls[0], (unsigned long long) o.eng().getEpoch (0))
                     + (live ? "" : "  VACUOUS"));
        }
        // F2b: s3 enable edge — grid-aligned and off-grid.
        {
            juce::String det2;
            bool ok = true;
            for (juce::int64 c : { (juce::int64) 96000, (juce::int64) 96007 })
            {
                Job j; j.len = (juce::int64) (3.0 * kFs); j.block = 32;
                j.params = base ({ 1 });
                j.params.push_back ({ "s3_density", 20.0f });
                j.events.push_back ({ c, [] (Proc& p) { eg::setP (p, "s3_enable", 1.0f); } });
                std::int64_t callsBefore = -1;
                RecLog L;
                j.after = collect (L, [&callsBefore, c] (Proc& p, juce::int64 pos)
                {
                    if (pos == c) { const auto& d = p.getEngineForTest().diag; callsBefore = d.planCalls[2] + d.schedCalls[2]; }
                });
                const auto o = eg::render (j);
                juce::int64 first = -1;
                for (auto& r : L.r) if (r.stream == 2) { first = r.onsetSample; break; }
                const juce::int64 grid = ((c + ogran::kCell - 1) / ogran::kCell) * ogran::kCell;
                ok = ok && first == grid && callsBefore == 0;
                det2 << juce::String::formatted ("enable @%lld -> first onset @%lld (grid %lld), calls before %lld; ",
                                                 (long long) c, (long long) first, (long long) grid, (long long) callsBefore);
            }
            row (ok, "F2 s3 enable edge: first onset on the enabling grid sample", det2);
        }
    }

    void capRows()
    {
        std::printf ("\n-- F3. FUNC-03 total grains <= cap (4 streams x 200 g/s x 2 s)\n");
        for (int idx = 0; idx < 4; ++idx)
        {
            const int cap = 32 * (idx + 1);
            Job j; j.len = (juce::int64) (3.0 * kFs); j.block = 32;
            j.params = base ({ 1, 2, 3, 4 });
            j.params.push_back ({ "grain_limit", (float) idx });
            for (int s = 1; s <= 4; ++s) { j.params.push_back ({ sp (s, "density"), 200.0f }); j.params.push_back ({ sp (s, "size"), 2000.0f }); }
            int scanMax = 0;
            j.after = [&scanMax] (Proc& p, juce::int64)
            {
                const auto& pool = p.getEngineForTest().getPool();
                int alive = 0;
                for (int s = 0; s < ogran::kPoolLive; ++s)
                    if (pool.isActive (s) && ! (pool.getGrains()[(size_t) s].killStep > 0.0f)) ++alive;
                scanMax = juce::jmax (scanMax, alive);
            };
            const auto o = eg::render (j);
            const auto& d = o.eng().diag;
            // Liveness: below the pool size the cap is the R3 kill; at 128 (= kPoolLive)
            // there is never a free live slot, so it is the native steal (erratum vs
            // the plan's "kills > 0", which is false by construction at 128).
            const bool engaged = cap < ogran::kPoolLive ? d.kills > 0 : d.steals > 0;
            row (d.maxAlive <= cap && scanMax <= cap && d.maxAlive == cap && engaged,
                 "F3 grain_limit " + juce::String (cap) + ": maxAlive <= cap (== cap), " + (cap < ogran::kPoolLive ? "kills > 0" : "steals > 0"),
                 juce::String::formatted ("engine maxAlive %lld, scan %d, spawns %lld kills %lld steals %lld", (long long) d.maxAlive,
                                          scanMax, (long long) d.spawns, (long long) d.kills, (long long) d.steals));
        }
    }

    //--------------------------------------------------------------------------
    // F4 / F5 — fairness vs the quota law.
    struct FairCase { const char* name; float dens[4]; float sizeMs[4]; int limIdx; };
    enum class Pol { Fair, Native, NoCeil };

    struct FairResult
    {
        double measDb[4] {}, targetDb[4] {}, measPow[4] {}, targetPow[4] {};
        double q[4] {}, n[4] {};
        bool lawOk = true;            // engine quotas == waterFill, c == 0, closed-form P agrees
        std::int64_t kills = 0, steals = 0;
        bool logsEqual = true;         // the 4 solo runs had identical spawn counts (P15 sanity)
    };

    Job fairJob (const FairCase& fc, int soloStream, Pol pol)
    {
        Job j; j.len = (juce::int64) (20.5 * kFs); j.block = 512; j.stim = blStim();
        j.seed = 0xFA1Bull;
        j.params = base ({ 1, 2, 3, 4 });
        j.params.push_back ({ "grain_limit", (float) fc.limIdx });
        for (int s = 1; s <= 4; ++s)
        {
            j.params.push_back ({ sp (s, "density"), fc.dens[s - 1] });
            j.params.push_back ({ sp (s, "size"), fc.sizeMs[s - 1] });
            j.params.push_back ({ sp (s, "spray"), 10.0f });
            j.params.push_back ({ sp (s, "solo"), s == soloStream ? 1.0f : 0.0f });
        }
        j.pre = [pol] (Proc& p)
        {
            p.getEngineForTest().hooks.nativeProgress = pol == Pol::Native;
            p.getEngineForTest().hooks.noQuotaCeil    = pol == Pol::NoCeil;
        };
        return j;
    }

    std::vector<FairResult> runFair (const std::vector<std::pair<FairCase, Pol>>& cfgs)
    {
        std::vector<Job> jobs;
        for (auto& c : cfgs)
            for (int s = 1; s <= 4; ++s) jobs.push_back (fairJob (c.first, s, c.second));
        std::vector<FairResult> res (cfgs.size());
        std::vector<std::int64_t> spawns (jobs.size());
        const size_t from = (size_t) (4.5 * kFs), to = (size_t) (20.5 * kFs);
        const auto& bl = blNoise();
        const double sigma2 = 0.5 * (bl.power[0] + bl.power[1]);
        renderBatch (jobs, [&] (size_t i, Out& o)
        {
            auto& r = res[i / 4];
            const int s = (int) (i % 4);
            const auto& e = o.eng();
            const auto& gp = e.getGridParams();
            spawns[i] = e.diag.spawns;
            r.measPow[s] = meanPow (o, from, to);
            r.measDb[s] = db (r.measPow[s]);
            if (s == 0) { r.kills = e.diag.kills; r.steals = e.diag.steals; }

            // Quota-law target from the LATCHED parameters.
            double dem[4] {};
            for (int k = 0; k < 4; ++k) dem[k] = ogran::streamDemand ((double) gp.s[k].density, (double) gp.s[k].sizeMs, kFs);
            const auto qs = ogran::waterFill (dem, gp.grainLimit);
            const double n = dem[s], q = qs.q[s];
            const double pw = ogran::streamPower (n, q, ogran::WindowSel {}, 0.0, 0.0, e.getBank());
            const double pIndep = n <= q ? n * 0.375 : q * ogran::hannCumW2 (q / n) / (q / n);
            const float g = ogran::outputGainFromPower (pw);
            r.lawOk = r.lawOk && juce::exactlyEqual (e.getQuotas().q[s], q) && juce::exactlyEqual (e.getCoherence (s), 0.0)
                      && std::abs (pw / pIndep - 1.0) < 1e-12;
            r.n[s] = n; r.q[s] = q;
            r.targetPow[s] = sigma2 * (double) g * (double) g * pw;
            r.targetDb[s] = db (r.targetPow[s]);
        });
        for (size_t c = 0; c < cfgs.size(); ++c)
            for (int s = 1; s < 4; ++s) res[c].logsEqual = res[c].logsEqual && spawns[c * 4 + (size_t) s] == spawns[c * 4];
        return res;
    }

    juce::String fairDetail (const FairResult& r)
    {
        juce::String d;
        for (int s = 0; s < 4; ++s)
            d << juce::String::formatted ("s%d N %.1f q %.2f %+.2f dB; ", s + 1, r.n[s], r.q[s], r.measDb[s] - r.targetDb[s]);
        d << juce::String::formatted ("kills %lld steals %lld", (long long) r.kills, (long long) r.steals);
        return d;
    }

    void fairnessRows()
    {
        std::printf ("\n-- F4-F5. FUNC-03 / PERF-02 starvation + fairness vs the quota law (solo probes, P15)\n");
        const FairCase cases[6] = {
            { "200,1,1,1 @128",     { 200, 1, 1, 1 },  { 1000, 1000, 1000, 1000 }, 3 },
            { "200,100,1,1 @128",   { 200, 100, 1, 1 }, { 1000, 1000, 1000, 1000 }, 3 },
            { "50x4 @128",          { 50, 50, 50, 50 }, { 1000, 1000, 1000, 1000 }, 3 },
            { "400,50,10,1 @64",    { 200, 50, 10, 1 }, { 2000, 1000, 1000, 1000 }, 1 },
            { "200,1,1,1 @32",      { 200, 1, 1, 1 },  { 1000, 1000, 1000, 1000 }, 0 },
            { "60,60,0.1,0.1 @96",  { 60, 60, 1, 1 },  { 1000, 1000, 100, 100 },   2 },
        };
        std::vector<std::pair<FairCase, Pol>> cfgs;
        for (auto& c : cases) cfgs.push_back ({ c, Pol::Fair });
        cfgs.push_back ({ cases[3], Pol::Native });   // F4n
        cfgs.push_back ({ cases[5], Pol::NoCeil });   // F5 R8 control
        for (int c : { 0, 1, 4 }) cfgs.push_back ({ cases[c], Pol::Native });   // INFO: native on the starving cases
        const auto t0 = std::chrono::steady_clock::now();
        const auto res = runFair (cfgs);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();

        auto worst = [] (const FairResult& r) { double w = 0; for (int s = 0; s < 4; ++s) w = juce::jmax (w, std::abs (r.measDb[s] - r.targetDb[s])); return w; };
        auto sane = [] (const FairResult& r) { return r.lawOk && r.logsEqual && r.kills + r.steals > 0; };

        {
            const auto& r = res[3];
            row (sane (r) && worst (r) <= 1.5, "F4 400,50,10,1 @64: every stream within +-1.5 dB of its quota-law target", fairDetail (r));
            const auto& n = res[6];
            const double small = n.measDb[3] - n.targetDb[3];
            row (sane (n) && small < -10.0, "F4n nativeProgress control: smallest stream (N 1) < -10 dB", fairDetail (n)
                 + (small < -10.0 ? "" : "  CONTROL DID NOT FAIL"));
        }
        for (int c = 0; c < 6; ++c)
        {
            const auto& r = res[(size_t) c];
            row (sane (r) && worst (r) <= 1.0, juce::String ("F5 ") + cases[c].name + ": every stream within +-1.0 dB", fairDetail (r)
                 + (sane (r) ? "" : "  (law/solo/steal sanity failed)"));
        }
        {
            const auto& f = res[5];
            const auto& n = res[7];
            const double fairSmall = db (f.measPow[2] + f.measPow[3]) - db (f.targetPow[2] + f.targetPow[3]);
            const double nc = db (n.measPow[2] + n.measPow[3]) - db (n.targetPow[2] + n.targetPow[3]);
            row (sane (n) && nc < -0.5 && fairSmall > -0.5,
                 "F5 R8 control (noQuotaCeil) 60,60,0.1,0.1 @96: pooled small streams < -0.5 dB",
                 juce::String::formatted ("no-ceil %+.2f dB vs ceil %+.2f dB; ", nc, fairSmall) + fairDetail (n));
        }
        for (size_t k = 8; k < res.size(); ++k)
            std::printf ("  [INFO] native steal-oldest %s: %s\n", cfgs[k].first.name, fairDetail (res[k]).toRawUTF8());
        std::printf ("  [INFO] F4/F5: %d renders x 20.5 s in %.1f s wall (parallel)\n", (int) cfgs.size() * 4, secs);
    }

    //--------------------------------------------------------------------------
    void soloIsolationRow()
    {
        std::printf ("\n-- F6. solo / mute leave the pool untouched\n");
        auto job = [] (int variant)
        {
            Job j; j.len = (juce::int64) (6.0 * kFs); j.block = 512;
            j.params = base ({ 1, 2, 3, 4 });
            j.params.push_back ({ "grain_limit", 1.0f });
            const float dens[4] { 200, 50, 10, 1 }, size[4] { 2000, 1000, 1000, 1000 };
            for (int s = 1; s <= 4; ++s)
            {
                j.params.push_back ({ sp (s, "density"), dens[s - 1] });
                j.params.push_back ({ sp (s, "size"), size[s - 1] });
                j.params.push_back ({ sp (s, "pitch_rand"), 2.0f });
                j.params.push_back ({ sp (s, "stereo_spread"), 40.0f });
            }
            if (variant == 1) j.params.push_back ({ "s2_solo", 1.0f });
            if (variant == 2) j.params.push_back ({ "s1_mute", 1.0f });
            return j;
        };
        RecLog L[3];
        std::vector<Out> o;
        for (int v = 0; v < 3; ++v) { auto j = job (v); j.after = collect (L[v]); o.push_back (eg::render (j)); }
        int mism = 0;
        for (int v = 1; v < 3; ++v)
        {
            if (L[v].r.size() != L[0].r.size()) { ++mism; continue; }
            for (size_t k = 0; k < L[0].r.size(); ++k)
            {
                const auto& a = L[0].r[k]; const auto& b = L[v].r[k];
                if (a.slot != b.slot || a.onsetSample != b.onsetSample || a.stream != b.stream || a.killed != b.killed
                    || a.stolen != b.stolen || std::memcmp (&a.delay, &b.delay, sizeof (double)) != 0
                    || std::memcmp (&a.rate, &b.rate, sizeof (double)) != 0)
                    ++mism;
            }
        }
        const auto d1 = eg::diff (o[0], o[1]), d2 = eg::diff (o[0], o[2]);
        const double soloLvl = eg::rmsDb (o[1].L, (size_t) kFs, o[1].L.size());
        const auto& dg = o[0].eng().diag;
        const bool live = dg.kills > 0 && d1.count > 0 && d2.count > 0 && soloLvl > -40.0 && ! L[0].overflow && L[0].r.size() > 1000;
        row (mism == 0 && live, "F6 spawn logs identical: no solo / solo s2 / mute s1 (cap 64, stealing)",
             juce::String::formatted ("%d mismatches over %d records; kills %lld; solo/mute change output (%d / %d diffs); solo s2 %.1f dB",
                                      mism, (int) L[0].r.size(), (long long) dg.kills, d1.count, d2.count, soloLvl)
                 + (live ? "" : "  VACUOUS"));
    }

    //--------------------------------------------------------------------------
    // F7 — one stream: Spawner::acquire == the 2.1 acquire (verbatim reference).
    void victimEquivalenceRow()
    {
        std::printf ("\n-- F7. single-stream victim equivalence (T8 proof)\n");
        using ogran::kPoolLive;
        using PoolT = ogran::Spawner::Pool;
        juce::Random rnd (0xF7F7F7ull);
        auto pool = std::make_unique<PoolT>();
        auto ref  = std::make_unique<PoolT>();
        int steps = 0, mism = 0, kills = 0, steals = 0, overQ = 0, plain = 0;
        juce::String first;

        auto randomise = [&rnd] (ogran::Grain& g, int tieWith, const ogran::Grain* base)
        {
            g.stream = 0;
            if (tieWith >= 0 && base != nullptr) { g.len = base[tieWith].len; g.invLen = base[tieWith].invLen; g.age = base[tieWith].age; return; }
            g.len = 100.0 + rnd.nextDouble() * 96000.0;
            g.invLen = 1.0 / g.len;
            g.age = rnd.nextInt ((int) g.len);
        };

        for (int trial = 0; trial < 300; ++trial)
        {
            pool->prepare (kFs, ogran::kPoolFadeMs);
            ogran::Spawner spn;
            spn.prepare (kFs);
            const int limit = 32 * (1 + rnd.nextInt (4));
            const int n0 = rnd.nextInt (kPoolLive + 1);
            for (int k = 0; k < n0; ++k)
                if (auto* g = pool->tryObtain()) randomise (*g, -1, nullptr);

            for (int step = 0; step < 60; ++step)
            {
                // Time passes: ages advance, some grains retire, some start a kill.
                const ogran::Grain* base = pool->getGrains().data();
                pool->render ([&] (ogran::Grain& g, PoolT::Fader&) -> bool
                {
                    const auto i = &g - base;
                    if (i >= kPoolLive) return rnd.nextInt (4) != 0;
                    g.age += rnd.nextInt (2000);
                    if (rnd.nextInt (40) == 0) g.killStep = 0.01f;
                    if (rnd.nextInt (25) == 0 && i > 0) { const int t = rnd.nextInt ((int) i); if (pool->isActive (t)) randomise (g, t, base); }
                    return (double) g.age < g.len && rnd.nextInt (30) != 0;
                });
                // Fill: to full (steal branch), a few, or none — every branch exercised.
                const int fillMode = rnd.nextInt (3);
                const int fillN = fillMode == 0 ? kPoolLive : (fillMode == 1 ? rnd.nextInt (8) : 0);
                for (int k = 0; k < fillN; ++k) if (auto* g = pool->tryObtain()) randomise (*g, -1, nullptr); else break;

                ogran::Quotas q;
                for (int s = 0; s < 4; ++s) { q.q[s] = rnd.nextDouble() * 160.0; q.qCeil[s] = (int) std::ceil (q.q[s]); }
                ogran::AcquirePolicy pol;
                pol.noQuotaCeil = rnd.nextBool();

                // 2.1 reference (verbatim logic) on a copy; nothing pending (kills applied below).
                *ref = *pool;
                const ogran::Grain* rb = ref->getGrains().data();
                int alive = 0, freeSlot = -1, victim = -1;
                double best = -1.0;
                for (int i = 0; i < kPoolLive; ++i)
                {
                    if (! ref->isActive (i)) { if (freeSlot < 0) freeSlot = i; continue; }
                    if (rb[i].killStep > 0.0f) continue;
                    ++alive;
                    const double pr = (double) rb[i].age * rb[i].invLen;
                    if (pr > best) { best = pr; victim = i; }
                }
                int expSlot = -1, expVictim = -1; bool expKilled = false, expStolen = false;
                if (alive >= limit && freeSlot >= 0 && victim >= 0) { expKilled = true; expVictim = victim; expSlot = (int) (ref->tryObtain() - rb); }
                else
                {
                    ogran::Grain* g = alive >= limit ? nullptr : ref->tryObtain();
                    if (g == nullptr)
                    {
                        g = &ref->obtain ([] (const ogran::Grain& x) { return (x.killStep > 0.0f ? 3.0 : 0.0) + (double) x.age * x.invLen; });
                        expStolen = true;
                    }
                    expSlot = (int) (g - rb);
                }

                int liveCount = 0;
                for (int i = 0; i < kPoolLive; ++i) if (pool->isActive (i) && ! (pool->getGrains()[(size_t) i].killStep > 0.0f)) ++liveCount;
                overQ += (pol.noQuotaCeil ? liveCount > q.q[0] : liveCount > q.qCeil[0]) ? 1 : 0;

                // 2.6 (Taylor sign-off at execute, PLAN-2.6 D25): F7 proves the 2.1 victim / slot
                // equivalence; the over-cap convergence kill (Spawner::acquire, alive > cap) is
                // outside that reference and is gated by GL3 / GL3n — so F7 runs without it.
                ogran::AcquirePolicy pol7 = pol;
                pol7.noConverge = true;
                const auto a = spn.acquire (*pool, limit, q, 0, pol7);
                int gotVictim = -1;
                const ogran::Grain* pb = pool->getGrains().data();
                pool->render ([&] (ogran::Grain& g, PoolT::Fader&) -> bool
                {
                    const bool was = g.killStep > 0.0f;
                    spn.applyPendingKill (g, pb);
                    if (! was && g.killStep > 0.0f) gotVictim = (int) (&g - pb);
                    return true;
                });
                spn.pendingKillsApplied();
                if (a.grain != nullptr) randomise (*a.grain, -1, nullptr);

                ++steps;
                kills += expKilled; steals += expStolen; plain += (! expKilled && ! expStolen);
                if (a.slot != expSlot || a.killed != expKilled || a.stolen != expStolen || gotVictim != expVictim)
                {
                    if (mism++ == 0)
                        first = juce::String::formatted ("trial %d step %d: slot %d/%d victim %d/%d killed %d/%d stolen %d/%d",
                                                         trial, step, a.slot, expSlot, gotVictim, expVictim, a.killed, expKilled, a.stolen, expStolen);
                }
            }
        }
        const bool live = kills > 100 && steals > 100 && plain > 100 && overQ > 100;
        row (mism == 0 && live, "F7 one stream: acquire victim/slot == 2.1 acquire (random pools, quotas, R8 forms)",
             juce::String::formatted ("%d steps, %d mismatches; kill %d steal %d plain %d, over-quota flagged %d", steps, mism, kills, steals, plain, overQ)
                 + (first.isNotEmpty() ? "  first: " + first : juce::String()) + (live ? "" : "  VACUOUS"));
    }

    //==========================================================================
    // T13 — DSP-01 scale quantiser
    //==========================================================================
    namespace PC = ouaricon::tuning::PitchCollections;

    std::uint64_t usedBits (int count) { return count >= 64 ? ~std::uint64_t (0) : ((std::uint64_t (1) << count) - 1); }

    /** Independent tiling of the active members inside [lo, hi] (cents). */
    std::vector<double> members (const PC::Table& t, std::uint64_t mask, double lo, double hi)
    {
        std::vector<double> m;
        mask &= usedBits (t.count);
        for (int d = 0; d < t.count; ++d)
        {
            if (((mask >> d) & 1u) == 0) continue;
            for (int k = -6; k <= 6; ++k)
            {
                const double c = (double) t.cents[(size_t) d] + k * (double) t.period;
                if (c >= lo - 1e-3 && c <= hi + 1e-3) m.push_back (c);
            }
        }
        std::sort (m.begin(), m.end());
        return m;
    }
    double nearest (const std::vector<double>& m, double x)
    {
        double b = 1e30;
        for (double v : m) b = juce::jmin (b, std::abs (v - x));
        return b;
    }
    const PC::Table& tableFor (Proc& p, int choice) { return *p.getCollectionTable (choice); }
    double recOffsetCents (const Rec& r, float pitch) { return 1200.0 * std::log2 (r.rate) - 100.0 * (double) pitch; }

    const char* kCustomQ1 = "130 290 515 640 860 1080 1300";   // 7 degrees, period 1300 (non-default)

    void quantiserRecordRows()
    {
        std::printf ("\n-- Q1-Q4, Q7. DSP-01 scale quantiser (records)\n");
        // Q1
        {
            const float pitches[4] { 0.0f, 5.3f, -20.0f, 20.0f };
            Tally t;
            int records = 0, setsChecked = 0;
            double worst = 0.0;
            for (int c = 1; c <= 31; ++c)
            {
                juce::Random rnd ((juce::int64) 0x51000 + c);
                Job j; j.len = (juce::int64) (1.0 * kFs); j.block = 512;
                j.params = base ({ 1, 2, 3, 4 });
                for (int s = 1; s <= 4; ++s)
                {
                    j.params.push_back ({ sp (s, "scale"), (float) c });
                    j.params.push_back ({ sp (s, "pitch_rand"), 7.0f });
                    j.params.push_back ({ sp (s, "pitch"), pitches[s - 1] });
                    j.params.push_back ({ sp (s, "density"), 100.0f });
                    j.params.push_back ({ sp (s, "size"), 30.0f });
                }
                std::uint64_t masks[4] {};
                j.pre = [&masks, &rnd, c] (Proc& p)
                {
                    if (c == 31) { juce::String bad; p.setCustomScale ("q1", kCustomQ1, bad); }
                    const int n = tableFor (p, c).count;
                    for (int s = 0; s < 4; ++s)
                    {
                        std::uint64_t m = ~std::uint64_t (0);
                        if (n >= 3)
                            for (;;)
                            {
                                m = (((std::uint64_t) (juce::uint32) rnd.nextInt()) << 32 | (juce::uint32) rnd.nextInt()) & usedBits (n);
                                const bool want0 = (s % 2) == 0;   // s1/s3 with unison, s2/s4 without
                                m = want0 ? (m | 1u) : (m & ~std::uint64_t (1));
                                if (std::popcount (m) >= 3) break;
                            }
                        masks[s] = m;
                        p.setDegreeMask (s + 1, c, m);
                    }
                };
                RecLog L; j.after = collect (L);
                const auto o = eg::render (j);
                auto& pr = *o.p;
                const auto& tab = tableFor (pr, c);
                const auto& gp = o.eng().getGridParams();
                if (c == 31 && ! (tab.count == 7 && juce::exactlyEqual (tab.period, 1300.0f))) t.check (false, "custom not installed");
                for (int s = 0; s < 4; ++s)
                {
                    const double p = (double) gp.s[s].pitch, span = 700.0;
                    const double lo = juce::jmax (-span, -2400.0 - 100.0 * p), hi = juce::jmin (span, 2400.0 - 100.0 * p);
                    const auto win = members (tab, masks[s], lo, hi);
                    // Active set (engine) == independent tiling over [−span, span].
                    const auto all = members (tab, masks[s], -span, span);
                    const auto& as = o.eng().getActiveSet (s);
                    bool same = as.size() == (int) all.size();
                    for (int k = 0; same && k < as.size(); ++k) same = std::abs ((double) as.data()[k] - all[(size_t) k]) < 1e-3;
                    t.check (same, juce::String (c) + "/s" + juce::String (s + 1) + " active set");
                    ++setsChecked;
                    std::set<int> distinct;
                    int n = 0;
                    for (auto& r : L.r)
                    {
                        if (r.stream != s) continue;
                        ++n;
                        const double off = recOffsetCents (r, gp.s[s].pitch);
                        const double e = win.empty() ? std::abs (off) : nearest (win, off);
                        worst = juce::jmax (worst, e);
                        distinct.insert ((int) std::lround (off * 10.0));
                        if (e > 1.0 && t.notes.size() < 4)
                            t.notes.add (juce::String (c) + "/s" + juce::String (s + 1) + juce::String::formatted (" off %.2f", off));
                        if (e > 1.0) ++t.bad;
                    }
                    records += n;
                    t.check (n > 50 && (int) distinct.size() >= juce::jmin (3, (int) win.size()) && ! L.overflow,
                             juce::String (c) + "/s" + juce::String (s + 1) + " vacuous");
                }
            }
            row (t.bad == 0, "Q1 31 collections x 4 pitches: every pick within 1 c of an active member in the D4 window",
                 juce::String::formatted ("%d records, %d active sets, worst %.2e c; ", records, setsChecked, worst) + t.detail (records + setsChecked));
        }

        // Q2
        {
            struct Case { const char* name; int choice; std::uint64_t mask; float pitch, prand; };
            const Case cases[3] = {
                { "empty mask",          1, 0,                      3.7f,  7.0f },
                { "no degree in span",   1, std::uint64_t (1) << 6, 3.7f,  3.0f },   // 600 / −600 outside ±300
                { "no degree in window", 1, std::uint64_t (1) << 4, 22.0f, 7.0f },   // {400}; window hi = 200
            };
            Tally t;
            juce::String det;
            auto job = [] (const Case& cs, float prand, std::uint64_t mask)
            {
                Job j; j.len = (juce::int64) (1.0 * kFs); j.block = 512;
                j.params = base ({ 1 });
                j.params.push_back ({ "s1_scale", (float) cs.choice });
                j.params.push_back ({ "s1_pitch", cs.pitch });
                j.params.push_back ({ "s1_pitch_rand", prand });
                j.params.push_back ({ "s1_density", 100.0f });
                j.params.push_back ({ "s1_size", 30.0f });
                j.pre = [cs, mask] (Proc& p) { p.setDegreeMask (1, cs.choice, mask); };
                return j;
            };
            for (auto& cs : cases)
            {
                RecLog L; auto j = job (cs, cs.prand, cs.mask); j.after = collect (L);
                const auto a = eg::render (j);
                const auto b = eg::render (job (cs, 0.0f, cs.mask));
                const auto& gp = a.eng().getGridParams();
                const double exact = std::exp2 ((double) gp.s[0].pitch / 12.0);
                int bad = 0;
                for (auto& r : L.r) bad += (std::memcmp (&r.rate, &exact, sizeof (double)) != 0 || ! juce::exactlyEqual (r.pitchCents, 0.0f)) ? 1 : 0;
                const auto d = eg::diff (a, b);
                const auto& ps = a.eng().getPitchSel (0);
                t.check (bad == 0 && d.count == 0 && L.r.size() > 50 && ps.set != nullptr && ps.hi == ps.lo, cs.name);
                det << juce::String::formatted ("%s: %d/%d inexact, prand7 vs 0 %d diffs; ", cs.name, bad, (int) L.r.size(), d.count);
            }
            // Control: a non-empty mask with pitch_rand 7 differs from pitch_rand 0.
            const Case live { "ctrl", 1, std::uint64_t (0b1010), 3.7f, 7.0f };
            const auto dc = eg::diff (eg::render (job (live, 7.0f, live.mask)), eg::render (job (live, 0.0f, live.mask)));
            t.check (dc.count > 0, "control");
            row (t.bad == 0, "Q2 empty mask / no degree in span / window: rate == exp2(p/12) exactly, no draw (bitwise)",
                 det + juce::String::formatted ("ctrl non-empty mask %d diffs", dc.count));
        }

        // Q3 — 7-Limit Just: sorted active array, mask-bit alignment.
        {
            const int choice = 25;   // spec 25 -> gs Id 25 JustSevenLimit (6…31 map 1:1)
            auto p0 = make();
            const auto& tab = *p0->getCollectionTable (choice);
            const bool isSeven = GranulatorParamIds::scaleChoiceToId (choice) == PC::JustSevenLimit;
            int i74 = -1, i127 = -1;
            for (int d = 0; d < tab.count; ++d)
            {
                if (std::abs (tab.cents[(size_t) d] - PC::ratioToCents (7, 4)) < 1e-3f) i74 = d;
                if (std::abs (tab.cents[(size_t) d] - PC::ratioToCents (12, 7)) < 1e-3f) i127 = d;
            }
            const bool unsortedSrc = i74 >= 0 && i127 > i74 && tab.cents[(size_t) i74] > tab.cents[(size_t) i127];

            auto job = [] (std::uint64_t mask, float prand)
            {
                Job j; j.len = (juce::int64) (2.0 * kFs); j.block = 512;
                j.params = base ({ 1 });
                j.params.push_back ({ "s1_scale", (float) choice });
                j.params.push_back ({ "s1_pitch_rand", prand });
                j.params.push_back ({ "s1_density", 150.0f });
                j.params.push_back ({ "s1_size", 30.0f });
                j.pre = [mask] (Proc& p) { p.setDegreeMask (1, choice, mask); };
                return j;
            };
            // Full mask, span 24 st: the active array is sorted and equals the tiling.
            const auto full = eg::render (job (~std::uint64_t (0), 24.0f));
            const auto& as = full.eng().getActiveSet (0);
            bool sorted = as.size() > 2 * tab.count;
            for (int k = 1; k < as.size(); ++k) sorted = sorted && as.data()[k - 1] < as.data()[k];

            // Only 7/4: every pick is 7/4 (+ k·1200) exactly; never 12/7.
            RecLog L; auto j = job (std::uint64_t (1) << i74, 10.0f); j.after = collect (L);
            const auto o = eg::render (j);
            const float a0 = (float) ((double) tab.cents[(size_t) i74]), a1 = (float) ((double) tab.cents[(size_t) i74] - 1200.0);
            const float b0 = (float) ((double) tab.cents[(size_t) i127]), b1 = (float) ((double) tab.cents[(size_t) i127] - 1200.0);
            int bad = 0, n0 = 0, n1 = 0, wrong = 0;
            for (auto& r : L.r)
            {
                if (juce::exactlyEqual (r.pitchCents, a0)) ++n0;
                else if (juce::exactlyEqual (r.pitchCents, a1)) ++n1;
                else ++bad;
                wrong += (juce::exactlyEqual (r.pitchCents, b0) || juce::exactlyEqual (r.pitchCents, b1)) ? 1 : 0;
                bad += std::abs (recOffsetCents (r, 0.0f) - (double) r.pitchCents) > 1e-3 ? 1 : 0;
            }
            row (isSeven && unsortedSrc && sorted && bad == 0 && wrong == 0 && n0 > 20 && n1 > 20,
                 "Q3 7-Limit Just: active array sorted; only 7/4 -> picks 7/4 (+-1200) exactly, never 12/7",
                 juce::String::formatted ("src 7/4 idx %d (%.2f c) before 12/7 idx %d (%.2f c); sorted %d (%d members); picks %.2f x%d, %.2f x%d, other %d, 12/7 %d",
                                          i74, tab.cents[(size_t) juce::jmax (0, i74)], i127, tab.cents[(size_t) juce::jmax (0, i127)], (int) sorted,
                                          as.size(), a0, n0, a1, n1, bad, wrong));
        }

        // Q4 — spec index ≠ gs-Id; 1-based stream.
        {
            Job j; j.len = (juce::int64) (2.0 * kFs); j.block = 512;
            j.params = base ({ 3, 4 });
            for (int s : { 3, 4 })
            {
                j.params.push_back ({ sp (s, "scale"), s == 3 ? 1.0f : 2.0f });   // s3 Chromatic (spec 1), s4 Major (spec 2 = gs-Id 1)
                j.params.push_back ({ sp (s, "pitch_rand"), 7.0f });
                j.params.push_back ({ sp (s, "density"), 150.0f });
                j.params.push_back ({ sp (s, "size"), 30.0f });
            }
            auto bits = [] (std::initializer_list<int> b) { std::uint64_t m = 0; for (int x : b) m |= std::uint64_t (1) << x; return m; };
            const std::uint64_t mChrom = bits ({ 1, 3 }), mMajor = bits ({ 2, 4 });
            j.pre = [&] (Proc& p)
            {
                for (int s = 1; s <= 4; ++s) { p.setDegreeMask (s, 1, bits ({ 7 })); p.setDegreeMask (s, 2, bits ({ 6 })); }   // decoys
                p.setDegreeMask (3, 1, mChrom);
                p.setDegreeMask (4, 2, mMajor);
            };
            RecLog L; j.after = collect (L);
            const auto o = eg::render (j);
            auto& pr = *o.p;
            const auto setC = members (*pr.getCollectionTable (1), mChrom, -700, 700);   // {100, 300}
            const auto setM = members (*pr.getCollectionTable (2), mMajor, -700, 700);   // {-500, 400, 700}
            const bool masksDiffer = pr.getDegreeMask (3, 1) != pr.getDegreeMask (3, 2) && pr.getDegreeMask (4, 1) != pr.getDegreeMask (4, 2);
            int bad = 0, n3 = 0, n4 = 0;
            std::set<int> d3, d4;
            for (auto& r : L.r)
            {
                const double off = recOffsetCents (r, 0.0f);
                if (r.stream == 2) { ++n3; d3.insert ((int) std::lround (off)); bad += nearest (setC, off) > 1.0 ? 1 : 0; }
                else if (r.stream == 3) { ++n4; d4.insert ((int) std::lround (off)); bad += nearest (setM, off) > 1.0 ? 1 : 0; }
                else ++bad;
            }
            row (masksDiffer && bad == 0 && setC.size() == 2 && setM.size() == 3 && d3.size() == 2 && d4.size() == 3 && n3 > 50 && n4 > 50,
                 "Q4 spec index != gs-Id, 1-based stream: s3 Chromatic {100,300}, s4 Major {-500,400,700} only",
                 juce::String::formatted ("s3 %d picks (%d distinct), s4 %d picks (%d distinct), %d outside; decoy masks elsewhere", n3, (int) d3.size(), n4, (int) d4.size(), bad));
        }

        // Q7 — base pitch unsnapped.
        {
            Job j; j.len = (juce::int64) (1.0 * kFs); j.block = 512;
            j.params = base ({ 1 });
            j.params.push_back ({ "s1_scale", 1.0f });
            j.params.push_back ({ "s1_pitch", 0.37f });
            j.params.push_back ({ "s1_pitch_rand", 0.0f });
            j.params.push_back ({ "s1_density", 100.0f });
            j.params.push_back ({ "s1_size", 30.0f });
            RecLog L; j.after = collect (L);
            const auto o = eg::render (j);
            const float p = o.eng().getGridParams().s[0].pitch;
            const double exact = std::exp2 ((double) p / 12.0);
            int bad = 0;
            for (auto& r : L.r) bad += std::memcmp (&r.rate, &exact, sizeof (double)) != 0 ? 1 : 0;
            const bool latched = std::abs (p - 0.37f) < 1e-5f;
            row (latched && bad == 0 && L.r.size() > 50 && o.eng().getPitchSel (0).set != nullptr,
                 "Q7 pitch +0.37 st, pitch_rand 0, scale Chromatic: rate == exp2(p/12) exactly (unsnapped)",
                 juce::String::formatted ("%d/%d exact, latched p %.9g, rate %.9f, scale set %d", (int) L.r.size() - bad, (int) L.r.size(),
                                          (double) p, exact, (int) (o.eng().getPitchSel (0).set != nullptr)));
        }
    }

    //--------------------------------------------------------------------------
    // Q5 — audio: isolated-grain spectrum peak.
    double grainPeakHz (const Out& o, const std::vector<Rec>& recs, int& used)
    {
        constexpr size_t N = 1u << 17;
        std::vector<double> acc (N / 2, 0.0);
        used = 0;
        for (size_t k = 0; k < recs.size(); ++k)
        {
            const auto& r = recs[k];
            const juce::int64 a = r.onsetSample, b = r.onsetSample + (juce::int64) std::ceil (r.len) + 2;
            const bool isolated = (k == 0 || recs[k - 1].onsetSample + (juce::int64) std::ceil (recs[k - 1].len) + 2 < a)
                               && (k + 1 >= recs.size() || recs[k + 1].onsetSample > b);
            if (a < (juce::int64) (0.5 * kFs) || b >= (juce::int64) o.L.size() || ! isolated) continue;
            std::vector<std::complex<double>> x (N);
            for (juce::int64 i = a; i < b; ++i) x[(size_t) (i - a)] = (double) o.L[(size_t) i];
            eg::fft (x);
            for (size_t f = 0; f < N / 2; ++f) acc[f] += std::norm (x[f]);
            ++used;
        }
        const double binHz = kFs / (double) N;
        size_t kb = (size_t) (1000.0 / binHz);
        for (size_t f = (size_t) (1000.0 / binHz); f < (size_t) (1500.0 / binHz); ++f) if (acc[f] > acc[kb]) kb = f;
        const double y0 = std::log (acc[kb - 1]), y1 = std::log (acc[kb]), y2 = std::log (acc[kb + 1]);
        const double delta = 0.5 * (y0 - y2) / (y0 - 2.0 * y1 + y2);
        return ((double) kb + delta) * binHz;
    }

    void quantiserAudioRows()
    {
        std::printf ("\n-- Q5-Q6. DSP-01 audio peak / custom swap\n");
        {
            const int choice = 24;   // spec 24 -> gs Id 24 JustFiveLimit
            auto probe = [] (int degree, int& used, double& hz)
            {
                Job j; j.len = (juce::int64) (8.0 * kFs); j.block = 512;
                j.stim = eg::sineStim (1000.0, kFs);
                j.params = base ({ 1 });
                j.params.push_back ({ "s1_scale", (float) choice });
                j.params.push_back ({ "s1_pitch_rand", 5.0f });
                j.params.push_back ({ "s1_spray", 10.0f });
                j.params.push_back ({ "s1_jitter", 20.0f });
                j.params.push_back ({ "s1_density", 2.0f });
                j.params.push_back ({ "s1_size", 200.0f });
                j.pre = [degree] (Proc& p) { p.setDegreeMask (1, choice, std::uint64_t (1) << degree); };
                RecLog L; j.after = collect (L);
                const auto o = eg::render (j);
                hz = grainPeakHz (o, L.r, used);
                return o.eng().getGridParams().s[0].jitter;
            };
            int u4 = 0, u3 = 0; double h4 = 0, h3 = 0;
            const float jit = probe (4, u4, h4);   // 5/4
            probe (3, u3, h3);                     // 6/5 (control)
            const double c4 = 1200.0 * std::log2 (h4 / 1250.0), c3 = 1200.0 * std::log2 (h3 / 1200.0);
            const bool fiveLimit = GranulatorParamIds::scaleChoiceToId (choice) == PC::JustFiveLimit;
            row (fiveLimit && jit > 0.0f && u4 >= 8 && u3 >= 8 && std::abs (c4) <= 1.0 && std::abs (c3) <= 1.0,
                 "Q5 1 kHz sine, 5-Limit only 5/4: FFT peak 1250 Hz +-1 c (ctrl 6/5 -> 1200 Hz)",
                 juce::String::formatted ("5/4: %.3f Hz (%+.3f c, %d grains); 6/5: %.3f Hz (%+.3f c, %d grains)", h4, c4, u4, h3, c3, u3));
        }
        // Q6 — custom swap.
        {
            const juce::int64 c = 24000;   // grid-aligned (750 × 32)
            Job j; j.len = (juce::int64) (1.0 * kFs); j.block = 64;
            j.params = base ({ 1 });
            j.params.push_back ({ "s1_scale", 31.0f });
            j.params.push_back ({ "s1_pitch_rand", 12.0f });
            j.params.push_back ({ "s1_density", 200.0f });
            j.params.push_back ({ "s1_size", 20.0f });
            j.pre = [] (Proc& p) { juce::String bad; p.setCustomScale ("A", "edo 7", bad); };
            j.events.push_back ({ c, [] (Proc& p) { juce::String bad; p.setCustomScale ("B", "edo 5", bad); } });
            RecLog L; j.after = collect (L);
            const auto o = eg::render (j);
            const auto A = members (PC::equalDivision (7), ~std::uint64_t (0), -1200, 1200);
            const auto B = members (PC::equalDivision (5), ~std::uint64_t (0), -1200, 1200);
            int preBad = 0, postBad = 0, preU = 0, postU = 0, mid = 0;
            juce::int64 firstNew = -1;
            for (auto& r : L.r)
            {
                const double off = (double) r.pitchCents;
                const bool inA = nearest (A, off) < 1e-3, inB = nearest (B, off) < 1e-3;
                if (inB && ! inA && firstNew < 0) firstNew = r.onsetSample;
                if (r.onsetSample < c) { preBad += inA ? 0 : 1; preU += (inA && ! inB) ? 1 : 0; }
                else if (r.onsetSample >= c + 2 * ogran::kCell) { postBad += inB ? 0 : 1; postU += (inB && ! inA) ? 1 : 0; }
                else ++mid;
            }
            const auto& e = o.eng();
            row (preBad == 0 && postBad == 0 && preU > 50 && postU > 50 && e.diag.customCopies == 1 && e.diag.customTryFails == 0
                     && e.getCustomCopy().count == 5,
                 "Q6 custom swap (edo 7 -> 5): new table within 2 grids, every earlier pick old-table",
                 juce::String::formatted ("pre %d bad (%d unambiguous), post %d bad (%d unambiguous), %d in the 2-grid gap; first new pick @%+lld smp; copies %lld misses %lld",
                                          preBad, preU, postBad, postU, mid, (long long) (firstNew - c), (long long) e.diag.customCopies,
                                          (long long) e.diag.customTryFails));
        }
    }

    //==========================================================================
    // T15 — DSP-04 randomisation
    //==========================================================================
    void randomisationRows()
    {
        std::printf ("\n-- D1-D5. DSP-04 randomisation\n");
        auto recJob = [] (P extra, double secs, float density = 200.0f)
        {
            Job j; j.len = (juce::int64) (secs * kFs); j.block = 512;
            j.params = base ({ 1 });
            j.params.push_back ({ "s1_density", density });
            j.params.push_back ({ "s1_size", 20.0f });
            for (auto& kv : extra) j.params.push_back (kv);
            return j;
        };
        // D1
        {
            int nRev[3] {}, nTot[3] {};
            const float probs[3] { 0.0f, 100.0f, 50.0f };
            for (int k = 0; k < 3; ++k)
            {
                RecLog L; auto j = recJob ({ { "s1_reverse_prob", probs[k] } }, 12.0); j.after = collect (L);
                eg::render (j);
                nTot[k] = (int) L.r.size();
                for (auto& r : L.r) nRev[k] += r.dir < 0 ? 1 : 0;
            }
            const double mu = 0.5 * nTot[2], sd = std::sqrt (0.25 * nTot[2]);
            const double z = (nRev[2] - mu) / sd;
            row (nRev[0] == 0 && nRev[1] == nTot[1] && nTot[2] >= 2000 && std::abs (z) <= 4.0 && nTot[0] > 2000,
                 "D1 reverse 0 % none, 100 % all, 50 % within 4 sigma (n >= 2000)",
                 juce::String::formatted ("0 %%: %d/%d; 100 %%: %d/%d; 50 %%: %d/%d (z %+.2f)", nRev[0], nTot[0], nRev[1], nTot[1], nRev[2], nTot[2], z));
        }
        // D2
        {
            RecLog L; auto j = recJob ({ { "s1_spray", 0.0f }, { "s1_position", 25.0f }, { "s1_pitch_rand", 24.0f }, { "s1_reverse_prob", 50.0f } }, 3.0);
            j.after = collect (L);
            const auto o = eg::render (j);
            const auto& gp = o.eng().getGridParams();
            const double span = (double) gp.bufferLengthS * kFs;
            int checked = 0, badD = 0, badG = 0;
            std::set<int> levels;
            for (auto& r : L.r)
            {
                const double margin = (double) ogran::Ring::margin (r.level);
                const double d0 = margin + (double) gp.s[0].position * span;
                levels.insert (r.level);
                badG += (juce::exactlyEqual (r.gainL, 1.0f) && juce::exactlyEqual (r.gainR, 1.0f)) ? 0 : 1;
                if (d0 < eg::floorFor (r)) continue;
                ++checked;
                badD += std::memcmp (&r.delay, &d0, sizeof (double)) != 0 ? 1 : 0;
            }
            RecLog C; auto jc = recJob ({ { "s1_spray", 10.0f }, { "s1_position", 25.0f }, { "s1_stereo_spread", 30.0f } }, 1.0);
            jc.after = collect (C);
            const auto oc = eg::render (jc);
            const double margin0 = (double) ogran::Ring::margin (0);
            const double d00 = margin0 + (double) oc.eng().getGridParams().s[0].position * span;
            int ctrlD = 0, ctrlG = 0;
            for (auto& r : C.r) { ctrlD += (r.level == 0 && ! juce::exactlyEqual (r.delay, d00)) ? 1 : 0; ctrlG += juce::exactlyEqual (r.gainL, 1.0f) ? 0 : 1; }
            row (checked == (int) L.r.size() && checked > 500 && badD == 0 && badG == 0 && levels.size() >= 2 && ctrlD > 0 && ctrlG > 0,
                 "D2 spray 0: delay == margin + pos*span exactly; spread 0: gains == 1.0f",
                 juce::String::formatted ("%d/%d checked (levels %d), %d delay / %d gain mismatches; ctrl spray 10 %% %d delays move, spread 30 %% %d gains != 1",
                                          checked, (int) L.r.size(), (int) levels.size(), badD, badG, ctrlD, ctrlG));
        }
        // D3
        {
            Tally t;
            juce::String det;
            for (float pan : { 0.0f, 100.0f, -100.0f })
            {
                RecLog L; auto j = recJob ({ { "s1_stereo_spread", 100.0f }, { "s1_pan", pan } }, 41.0); j.after = collect (L);
                eg::render (j);
                double sL = 0, sR = 0, worst = 0;
                for (auto& r : L.r)
                {
                    const double a = (double) r.gainL * r.gainL, b = (double) r.gainR * r.gainR;
                    sL += a; sR += b;
                    worst = juce::jmax (worst, std::abs (a + b - 2.0));
                }
                const double bal = db (sL / sR);
                t.check (L.r.size() >= 8000 && worst <= 1e-6 && std::abs (bal) <= 0.2 && ! L.overflow, juce::String (pan));
                det << juce::String::formatted ("pan %+.0f: %d grains, |gL2+gR2-2| %.1e, L/R %+.3f dB; ", pan, (int) L.r.size(), worst, bal);
            }
            // Audio: noise L/R independent; normalise by each input channel's power.
            const auto& bl = blNoise();
            for (float pan : { 0.0f, 50.0f })
            {
                auto j = recJob ({ { "s1_stereo_spread", 100.0f }, { "s1_pan", pan } }, 41.0);
                j.stim = blStim();   // 41 s: the table is 42 s
                const auto o = eg::render (j);
                double pl = 0, pr = 0;
                for (size_t i = (size_t) kFs; i < o.L.size(); ++i) { pl += (double) o.L[i] * o.L[i]; pr += (double) o.R[i] * o.R[i]; }
                const double meas = db ((pl / bl.power[0]) / (pr / bl.power[1]));
                const auto g = ogran::panGains (pan / 100.0f);
                const double expect = db (((double) g.a * g.a) / ((double) g.b * g.b));
                t.check (std::abs (meas - expect) <= 0.2, "audio pan " + juce::String (pan));
                det << juce::String::formatted ("audio pan %+.0f: L/R %+.3f dB (bus %+.3f); ", pan, meas, expect);
            }
            row (t.bad == 0, "D3 spread 100 %: constant power per grain, L = R (0.2 dB) at pan 0 / +-1, audio == bus balance", det);
        }
        // D4
        {
            auto job = [&recJob] (float gr) { return recJob ({ { "s1_gain_rand", gr }, { "s1_pitch_rand", 3.0f }, { "s1_size_rand", 30.0f } }, 3.0, 100.0f); };
            RecLog L0; auto j0 = job (0.0f); j0.after = collect (L0);
            const auto o0 = eg::render (j0);
            auto jt = job (1.0e-6f);
            const auto ot = eg::render (jt);
            const bool tinyLatched = ot.eng().getGridParams().s[0].gainRand > 0.0f;
            int bad0 = 0;
            for (auto& r : L0.r) bad0 += (juce::exactlyEqual (r.gainL, 1.0f) && juce::exactlyEqual (r.gainR, 1.0f)) ? 0 : 1;
            const auto dt = eg::diff (o0, ot);
            RecLog L1; auto j1 = job (100.0f); j1.after = collect (L1);
            const auto o1 = eg::render (j1);
            const float lo = (float) std::pow (10.0, -24.0 / 20.0);
            int bad1 = 0; float gmin = 2.0f, gmax = 0.0f;
            for (auto& r : L1.r)
            {
                bad1 += (r.gainL >= lo && r.gainL <= 1.0f && juce::exactlyEqual (r.gainL, r.gainR)) ? 0 : 1;
                gmin = juce::jmin (gmin, r.gainL); gmax = juce::jmax (gmax, r.gainL);
            }
            const auto d1 = eg::diff (o0, o1);
            row (bad0 == 0 && L0.r.size() > 200 && tinyLatched && dt.count == 0 && bad1 == 0 && L1.r.size() > 200
                     && db ((double) gmin * gmin) < -20.0 && db ((double) gmax * gmax) > -1.0 && d1.count > 0,
                 "D4 gain_rand 0: gains 1.0f, 1e-6 % draw renders bitwise == 0; 100 %: every gain in [-24, 0] dB",
                 juce::String::formatted ("0: %d/%d not unity; 1e-6 %% vs 0: %d diffs; 100 %%: %d/%d out of range, span [%.2f, %.2f] dB; 100 %% vs 0 %d diffs",
                                          bad0, (int) L0.r.size(), dt.count, bad1, (int) L1.r.size(), db ((double) gmin * gmin),
                                          db ((double) gmax * gmax), d1.count));
        }
        // D5
        {
            auto job = [] (int limIdx)
            {
                Job j; j.len = (juce::int64) (4.0 * kFs); j.block = 512;
                j.params = base ({ 1, 2, 3, 4 });
                j.params.push_back ({ "grain_limit", (float) limIdx });
                for (int s = 1; s <= 4; ++s)
                {
                    for (auto& kv : P { { sp (s, "density"), 50.0f }, { sp (s, "size"), 500.0f }, { sp (s, "size_rand"), 40.0f },
                                        { sp (s, "spray"), 30.0f }, { sp (s, "pitch_rand"), 5.0f }, { sp (s, "reverse_prob"), 50.0f },
                                        { sp (s, "stereo_spread"), 70.0f }, { sp (s, "gain_rand"), 50.0f },
                                        { sp (s, "window"), 0.6f }, { sp (s, "window_skew"), -40.0f } })
                        j.params.push_back (kv);
                }
                j.params.push_back ({ "s2_scale", 3.0f });
                return j;
            };
            RecLog A, B;
            auto ja = job (0); ja.after = collect (A);
            auto jb = job (3); jb.after = collect (B);
            const auto oa = eg::render (ja);
            const auto ob = eg::render (jb);
            std::map<std::pair<int, std::uint64_t>, const Rec*> idx;
            for (auto& r : B.r) idx[{ r.stream, r.globalIndex }] = &r;
            int bad = 0, matched = 0, afterKill = 0;
            bool seenKill = false;
            for (auto& r : A.r)
            {
                seenKill = seenKill || r.killed || r.stolen;
                auto it = idx.find ({ r.stream, r.globalIndex });
                if (it == idx.end()) { ++bad; continue; }
                const auto& b = *it->second;
                ++matched;
                afterKill += seenKill ? 1 : 0;
                if (std::memcmp (&r.rate, &b.rate, sizeof (double)) != 0 || r.dir != b.dir || ! juce::exactlyEqual (r.gainL, b.gainL)
                    || ! juce::exactlyEqual (r.gainR, b.gainR) || r.windowStep != b.windowStep || ! juce::exactlyEqual (r.tiltT, b.tiltT)
                    || ! juce::exactlyEqual (r.pitchCents, b.pitchCents) || std::memcmp (&r.delay, &b.delay, sizeof (double)) != 0
                    || std::memcmp (&r.len, &b.len, sizeof (double)) != 0 || r.onsetSample != b.onsetSample)
                    ++bad;
            }
            const auto& da = oa.eng().diag; const auto& dbb = ob.eng().diag;
            const bool live = da.kills + da.steals > 100 && dbb.kills + dbb.steals == 0 && afterKill > 500 && matched == (int) B.r.size()
                              && ! A.overflow && eg::diff (oa, ob).count > 0;
            row (bad == 0 && live, "D5 draw-before-obtain: cap 32 (stealing) vs 128 records identical on draw-derived fields",
                 juce::String::formatted ("%d matched, %d after the first kill, %d mismatches; cap 32 kills %lld steals %lld, cap 128 kills %lld steals %lld",
                                          matched, afterKill, bad, (long long) da.kills, (long long) da.steals, (long long) dbb.kills, (long long) dbb.steals)
                     + (live ? "" : "  VACUOUS"));
        }
    }
} } // namespace eg22 (anonymous)

static void engineRows22()
{
    std::printf ("\n== F/Q/D. Stage 2 / Phase 2.2 engine rows (engine_gates_22.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg22::blNoise();   // build the stimulus table on this thread before any parallel render
    eg22::streamsAloneRows();
    eg22::capRows();
    eg22::fairnessRows();
    eg22::soloIsolationRow();
    eg22::victimEquivalenceRow();
    eg22::quantiserRecordRows();
    eg22::quantiserAudioRows();
    eg22::randomisationRows();
    std::printf ("  [INFO] 2.2 rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
