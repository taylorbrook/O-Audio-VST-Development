/*
  ==============================================================================

    O-Granulator — render harness, sections E0+/W/K/X/N5/M2/C2 (Stage 2 /
    Phase 2.2, PLAN-2.2 Wave 5: T14, T16). Sibling of engine_gates_22.h
    (reuses eg22::base / blNoise / renderBatch / collect / meanPow / db and
    eg::Job / render / setP / diff / maxCurv / the M1 malloc_logger hook).

    T14 — DSP-03 / output law (R5, R6, R7, R11)
      E0b bank C(1) == μ (1e-6), k = 1, 2, all 65 steps
      E0c warped cumulative vs direct double integration of the heard window
          (WindowMorphBank::read, midpoint 2^17) <= 1e-5, 65 steps × 9 skews
      E0d R5 law at c = 0 == 2.1 streamOutputGain BITWISE (and P/μ₂ == 2.1
          effectiveOverlap bitwise) over (density, size, q, fs); c = 1 differs
      E0e R5 continuity at N = q (c 0 / 0.5 / 1 × 4 windows × 4 q) < 1e-6
      E0f worst capped P/μ₂H == 333 ± 0.5 % at step 22 / skew −1 / q 128 /
          N 400, and it IS the max over 65 × 9
      E0g water-fill: one stream, capped → q == Q, λ == Q EXACTLY; uncapped →
          q == N exactly
      W1  morph × skew ±1 dB: N 40 (200 g/s × 200 ms), cap 128, spray 10 %
          (c = 0), spread 0, band-limited noise, 4 seeds, warm-up 0.4 s
          (2 × size), 2 s window; 65 steps × skew {−1, −.5, 0, .5, 1} vs the
          step 0 / skew 0 render. Capped corner: N 400 (200 g/s × 2 s), q 128,
          step 22 / skew −1 vs step 0 / skew 0 at the same N, q (warm-up 4 s,
          4 s window)
      W2  density flatness ±1.5 dB vs the law target σ²·g_out²·P: size 1000 ms,
          density {1 … 200} (crosses cap 128), c = 1 (spray 0) / 0 (spray
          10 %) / 0.5 (reverse 50 %), noise (4 seeds) and sine 440 (16 seeds,
          R11 >= 8), jitter 20 %, pitch 0, spread 0; warm-up 2 s, 10 s window.
          W2o old-vs-old control: a second seed set, ±0.5 dB on every cell not
          R11-variance-limited (noise ×3, sine c = 1); incoherent sine spread INFO
      W2n c = 1, density 200: 2.1 law (harness reference: same render rescaled
          by (g21/g22)²) > +15 dB, 2.2 within ±1.5 dB
      W3  sub-floor: size 100 ms, density 1, step 64, skew −1: g_out == 1.0f,
          the unfloored gain would be > 1 (floor binding), output peak <=
          input peak × window peak × level (no boost)
      R11 guard: every W2 / band sine spec passes the builder's R11 predicate
          (no jitter 0 with pitch ≠ 0), and the predicate rejects one that does
      BAND (INFO, non-gating, RESEARCH §1.4): spray 0.03 %, N 20 — noise vs
          sine 110 Hz level vs the law target

    T16 — QUAL-01, mixer, PERF
      K1  steal click: sine 440 at 0.001 (linear wet sum), spray 0, 4 streams.
          κ = maxCurv / RMS over [3, 7) s; ratio vs the steal-free reference
          (4 × 20 g/s × 1 s @ cap 128, 0 steals / kills) <= 2.0 for
          (a) cap 32, R3 kills and (b) 4 × 50 g/s × 1 s @ cap 128, native
          steals. Non-vacuous: kills / steals with onset in the window.
          K1n: subject (a) with ONE grain hard-cut at the loudest in-window
          kill (the coherent cloud loses the fraction 0.9 / S1 at once) > 2.0
      X1  mute / solo / level / pan ramps (E2 same-motion method): subject vs
          the static render × the ExactRamp/CellLerp gain trajectory driven by
          the same events (15 cells) — per event, per channel κ ratio < 1.5;
          the same reference with a HARD switch at the event > 10×; mute
          reaches exact 0 at event + 15 cells; getMixerRampCells() == 15
      N5  block-size null, 4 streams, everything on (spray, size_rand, jitter,
          pitch_rand + scale + mask, Custom, reverse 50 %, window 0.6, skew
          −0.4, spread 70 %, gain_rand 50 %, cap 32), mute / solo / enable
          toggles + a custom swap at shared boundaries: 441 / 512 / 4096 / var
          vs 32 max|Δ| == 0. N5c different seed differs
      M2  0 allocations on the render thread (M1 coverage + 4 streams, scale /
          mask / pitch_rand sweeps → active-set rebuilds, custom swaps, window
          / skew sweeps, mute / solo); setCustomScale / setDegreeMask run in
          events, OUTSIDE processBlock (alloc flag off); liveness 1
      C2  CPU (non-blocking INFO): 4 streams × 32 grains (128), window 0.6,
          scale on, 48 k / 128 — vs the 2.1 C1 baseline 8.28 %

  ==============================================================================
*/
#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

namespace { namespace eg22b
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    using eg22::db;
    using Rec = ogran::Engine::SpawnRecord;
    using Tilt = ogran::WindowMorphBank::Tilt;
    constexpr double kFs = 48000.0;

    Tilt tiltFor (double skew) { return ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * (float) skew); }

    //==========================================================================
    // E0 additions — law / bank / water-fill units
    //==========================================================================
    void lawUnitRows()
    {
        std::printf ("\n-- E0+. R5 law, morph-bank cumulatives, water-fill (units)\n");
        static const ogran::WindowMorphBank bank;   // built once, message thread
        constexpr int S = ogran::WindowMorphBank::kSteps, Np = ogran::WindowMorphBank::kPoints;

        // E0b — C(1) == μ of the lerped float table.
        {
            double worst = 0.0;
            for (int k = 0; k < S; ++k)
            {
                const float* w = bank.table (k);
                const double h = 1.0 / (Np - 1);
                double m1 = 0.0, m2 = 0.0;
                for (int j = 0; j + 1 < Np; ++j)
                {
                    const double a = w[j], b = w[j + 1];
                    m1 += h * 0.5 * (a + b);
                    m2 += h * (a * a + a * b + b * b) / 3.0;
                }
                worst = juce::jmax (worst, std::abs (bank.mu1 (k) - m1), std::abs (bank.mu2 (k) - m2));
            }
            row (worst <= 1e-6, "E0b bank C(1) == mu (k = 1, 2) per step, 65 steps (1e-6)", juce::String::formatted ("worst %.3g", worst));
        }

        // E0c — warped cumulative vs direct integration of the HEARD window.
        {
            constexpr int M = 1 << 17;
            double worst = 0.0; int wk = -1; double ws = 0.0, wx = 0.0; int checks = 0;
            for (int k = 0; k < S; ++k)
            {
                const float* tab = bank.table (k);
                for (int si = 0; si < 9; ++si)
                {
                    const double skew = -1.0 + 0.25 * si;
                    const Tilt tl = tiltFor (skew);
                    std::vector<double> xs;
                    for (int j = 1; j <= 16; ++j) xs.push_back (j / 16.0);
                    xs.push_back ((double) tl.t);
                    xs.push_back (0.08);
                    std::sort (xs.begin(), xs.end());
                    double a1 = 0.0, a2 = 0.0;
                    size_t xi = 0;
                    for (int i = 0; i < M && xi < xs.size(); ++i)
                    {
                        const double lo = (double) i / M, hi = (double) (i + 1) / M;
                        // integrate [lo, hi) clipped at the next checkpoint (midpoint on each part)
                        double from = lo;
                        while (xi < xs.size() && xs[xi] <= hi)
                        {
                            const double to = xs[xi];
                            if (to > from)
                            {
                                const double v = ogran::WindowMorphBank::read (tab, tl, 0.5 * (from + to));
                                a1 += (to - from) * v; a2 += (to - from) * v * v;
                            }
                            for (int kk : { 1, 2 })
                            {
                                const double e = std::abs (bank.cum (kk, k, tl, to) - (kk == 1 ? a1 : a2));
                                if (e > worst) { worst = e; wk = k; ws = skew; wx = to; }
                                ++checks;
                            }
                            from = to;
                            ++xi;
                        }
                        if (hi > from)
                        {
                            const double v = ogran::WindowMorphBank::read (tab, tl, 0.5 * (from + hi));
                            a1 += (hi - from) * v; a2 += (hi - from) * v * v;
                        }
                    }
                }
            }
            row (worst <= 1e-5 && checks == S * 9 * 18 * 2, "E0c warped cumulative vs direct integration, 65 steps x 9 skews (1e-5)",
                 juce::String::formatted ("worst %.3g at step %d skew %+.2f x %.4f; %d checks", worst, wk, ws, wx, checks));
        }

        // E0d — c = 0 reduces bitwise to the 2.1 law.
        {
            int cells = 0, bad = 0, capped = 0, uncapped = 0, cDiffers = 0;
            juce::String first;
            for (double fs : { 44100.0, 48000.0, 96000.0 })
                for (double d : { 0.5, 1.0, 2.0, 3.7, 5.0, 10.0, 20.0, 37.5, 50.0, 64.0, 100.0, 128.0, 160.0, 200.0 })
                    for (double sz : { 5.0, 10.0, 33.0, 50.0, 100.0, 250.0, 500.0, 1000.0, 1500.0, 2000.0 })
                        for (int q : { 32, 64, 96, 128 })
                        {
                            const double n = ogran::streamDemand (d, sz, fs);
                            const double p = ogran::streamPower (n, (double) q, ogran::WindowSel {}, 0.0, 0.0, bank);
                            const float g22 = ogran::outputGainFromPower (p);
                            const float g21 = ogran::streamOutputGain (d, sz, fs, q);
                            const double eo22 = p / ogran::kHannMu2, eo21 = ogran::effectiveOverlap (d, sz, fs, q);
                            ++cells;
                            (n <= q ? uncapped : capped) += 1;
                            if (std::memcmp (&g22, &g21, sizeof (float)) != 0 || std::memcmp (&eo22, &eo21, sizeof (double)) != 0)
                            {
                                if (bad++ == 0) first = juce::String::formatted ("fs %g d %g size %g q %d: %.9g vs %.9g", fs, d, sz, q, g22, g21);
                            }
                            const float g1 = ogran::outputGainFromPower (ogran::streamPower (n, (double) q, ogran::WindowSel {}, 1.0, 0.0, bank));
                            cDiffers += juce::exactlyEqual (g1, g21) ? 0 : 1;
                        }
            row (bad == 0 && capped > 50 && uncapped > 50 && cDiffers > 50,
                 "E0d R5 law at c = 0 == 2.1 streamOutputGain BITWISE (P/mu2 too)",
                 juce::String::formatted ("%d cells (%d capped, %d uncapped), %d mismatches; ctrl c = 1 differs in %d", cells, capped,
                                          uncapped, bad, cDiffers) + (first.isNotEmpty() ? "  first: " + first : juce::String()));
        }

        // E0e — continuity at N = q.
        {
            double worst = 0.0;
            struct W { int step; double skew; };
            for (const W& w : { W { 0, 0.0 }, W { 22, -1.0 }, W { 64, 0.5 }, W { 40, 0.0 } })
            {
                ogran::WindowSel sel; sel.step = w.step; sel.tilt = tiltFor (w.skew);
                for (double c : { 0.0, 0.5, 1.0 })
                    for (int q : { 32, 64, 96, 128 })
                    {
                        const double at = ogran::streamPower (q, q, sel, c, 0.0, bank);
                        const double a  = ogran::streamPower (q * (1.0 - 1e-9), q, sel, c, 0.0, bank);
                        const double b  = ogran::streamPower (q * (1.0 + 1e-9), q, sel, c, 0.0, bank);
                        worst = juce::jmax (worst, std::abs (a - at) / at, std::abs (b - at) / at);
                    }
            }
            row (worst < 1e-6, "E0e R5 continuity at N = q (c 0/.5/1 x 4 windows x 4 q), rel jump < 1e-6", juce::String (worst, 3));
        }

        // E0f — worst capped overlap 333.
        {
            ogran::WindowSel sel; sel.step = 22; sel.tilt = tiltFor (-1.0);
            const double v = ogran::streamPower (400.0, 128.0, sel, 0.0, 0.0, bank) / ogran::kHannMu2;
            double mx = 0.0; int mk = -1; double ms = 0.0;
            for (int k = 0; k < S; ++k)
                for (int si = 0; si < 9; ++si)
                {
                    ogran::WindowSel w; w.step = k; w.tilt = tiltFor (-1.0 + 0.25 * si);
                    const double r = ogran::streamPower (400.0, 128.0, w, 0.0, 0.0, bank) / ogran::kHannMu2;
                    if (r > mx) { mx = r; mk = k; ms = -1.0 + 0.25 * si; }
                }
            const double hann = ogran::streamPower (400.0, 128.0, ogran::WindowSel {}, 0.0, 0.0, bank) / ogran::kHannMu2;
            row (std::abs (v / 333.0 - 1.0) <= 0.005 && mx <= 333.0 * 1.005 && std::abs (hann / 43.02 - 1.0) <= 0.005,
                 "E0f worst capped P/mu2H = 333 (+-0.5 %) at step 22 / skew -1 / q 128 / N 400",
                 juce::String::formatted ("%.2f (g_out %.2f dB); scan max %.2f at step %d skew %+.2f; Hann %.2f", v,
                                          20.0 * std::log10 ((double) ogran::outputGainFromPower (v * ogran::kHannMu2)), mx, mk, ms, hann));
        }

        // E0g — water-fill single-stream λ == Q exactly.
        {
            int checks = 0, bad = 0;
            for (int s = 0; s < 4; ++s)
                for (int Q : { 32, 64, 96, 128 })
                    for (double n : { 0.7, 13.25, 32.0, 33.3, 96.5, 128.5, 200.0, 400.0, 1.0e4 })
                    {
                        double dem[4] {}; dem[s] = n;
                        const auto r = ogran::waterFill (dem, Q);
                        const double expect = n > Q ? (double) Q : n;
                        bool ok = std::memcmp (&r.q[s], &expect, sizeof (double)) == 0 && r.qCeil[s] == (int) std::ceil (expect);
                        if (n > Q) ok = ok && std::memcmp (&r.lambda, &expect, sizeof (double)) == 0 && r.capped;
                        for (int o = 0; o < 4; ++o) if (o != s) ok = ok && juce::exactlyEqual (r.q[o], 0.0);
                        ++checks; bad += ok ? 0 : 1;
                    }
            row (bad == 0, "E0g water-fill one stream: capped q == lambda == Q, uncapped q == N (exact)",
                 juce::String::formatted ("%d/%d exact", checks - bad, checks));
        }
    }

    //==========================================================================
    // Streaming render pool: like eg22::renderBatch (processors built, consumed
    // and destroyed on THIS thread; only processBlock runs on workers), but a
    // bounded queue keeps every worker busy instead of waiting for the slowest
    // job of each batch. Jobs are dispatched most-expensive first.
    void renderPool (const std::vector<Job>& jobs, const std::vector<double>& cost,
                     const std::function<void (size_t, Out&)>& consume)
    {
        const size_t hw = juce::jmax (1u, std::thread::hardware_concurrency());
        std::vector<size_t> order (jobs.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::stable_sort (order.begin(), order.end(), [&cost] (size_t a, size_t b) { return cost[a] > cost[b]; });

        std::mutex m;
        std::condition_variable cvWork, cvDone;
        std::deque<std::pair<size_t, std::unique_ptr<Out>>> todo, done;
        bool stop = false;
        std::vector<std::thread> th;
        for (size_t w = 0; w < hw; ++w)
            th.emplace_back ([&]
            {
                for (;;)
                {
                    std::pair<size_t, std::unique_ptr<Out>> item;
                    {
                        std::unique_lock<std::mutex> lk (m);
                        cvWork.wait (lk, [&] { return stop || ! todo.empty(); });
                        if (todo.empty()) return;
                        item = std::move (todo.front()); todo.pop_front();
                    }
                    eg22::runLoop (jobs[item.first], *item.second);
                    { std::lock_guard<std::mutex> lk (m); done.push_back (std::move (item)); }
                    cvDone.notify_one();
                }
            });
        size_t next = 0, finished = 0, inFlight = 0;
        while (finished < jobs.size())
        {
            while (next < jobs.size() && inFlight < 2 * hw)
            {
                jassert (jobs[order[next]].events.empty());
                auto o = std::make_unique<Out> (eg22::setupOut (jobs[order[next]]));
                { std::lock_guard<std::mutex> lk (m); todo.emplace_back (order[next], std::move (o)); }
                cvWork.notify_one();
                ++next; ++inFlight;
            }
            std::pair<size_t, std::unique_ptr<Out>> item;
            {
                std::unique_lock<std::mutex> lk (m);
                cvDone.wait (lk, [&] { return ! done.empty(); });
                item = std::move (done.front()); done.pop_front();
            }
            consume (item.first, *item.second);
            item.second.reset();
            ++finished; --inFlight;
        }
        { std::lock_guard<std::mutex> lk (m); stop = true; }
        cvWork.notify_all();
        for (auto& t : th) t.join();
    }

    //==========================================================================
    // Level renders (W1 / W2 / W3 / band)
    //==========================================================================
    struct LvlSpec
    {
        int step = 0; float skewPct = 0.0f;
        float density = 200.0f, sizeMs = 200.0f; int limIdx = 3;
        float spray = 10.0f, reverse = 0.0f, jitter = 20.0f, pitch = 0.0f;
        bool sine = false; double sineHz = 440.0;
        std::uint64_t seed = 1;
        double warm = 0.4, meas = 2.0;
    };
    struct LvlRes
    {
        double meas = 0.0, target = 0.0, c = -1.0, n = 0.0, q = 0.0, pRaw = 0.0;
        float g = 0.0f, gEngine = 0.0f, g21 = 0.0f;
        bool lawOk = false, latchOk = false;
        std::int64_t steals = 0, kills = 0, spawns = 0;
    };

    /** R11 (RESEARCH-2.2): a sine row at jitter 0 with pitch != 0 comb-cancels. */
    bool r11Allowed (const LvlSpec& s) { return ! (s.sine && ! (s.jitter > 0.0f) && ! juce::exactlyEqual (s.pitch, 0.0f)); }

    Job lvlJob (const LvlSpec& s)
    {
        Job j; j.len = (juce::int64) ((s.warm + s.meas) * kFs); j.block = 512; j.seed = s.seed;
        j.stim = s.sine ? eg::sineStim (s.sineHz, kFs, 0.5) : eg22::blStim();
        j.params = eg22::base ({ 1 });
        for (auto& kv : P { { "grain_limit", (float) s.limIdx }, { "s1_density", s.density }, { "s1_size", s.sizeMs },
                            { "s1_spray", s.spray }, { "s1_reverse_prob", s.reverse }, { "s1_jitter", s.jitter },
                            { "s1_pitch", s.pitch }, { "s1_window", (float) s.step / 64.0f }, { "s1_window_skew", s.skewPct } })
            j.params.push_back (kv);
        return j;
    }

    std::vector<LvlRes> runLevels (const std::vector<LvlSpec>& specs)
    {
        std::vector<Job> jobs;
        for (auto& s : specs) jobs.push_back (lvlJob (s));
        std::vector<LvlRes> res (specs.size());
        const auto& bl = eg22::blNoise();
        std::vector<double> cost;
        for (auto& sp : specs) cost.push_back ((sp.warm + sp.meas) * juce::jmin (128.0, (double) sp.density * sp.sizeMs / 1000.0 + 4.0));
        renderPool (jobs, cost, [&] (size_t i, Out& o)
        {
            const auto& s = specs[i];
            auto& r = res[i];
            const auto& e = o.eng();
            const auto& st = e.getGridParams().s[0];
            r.meas = eg22::meanPow (o, (size_t) (s.warm * kFs), o.L.size());
            r.c = e.getCoherence (0);
            r.n = ogran::streamDemand ((double) st.density, (double) st.sizeMs, kFs);
            double dem[4] { r.n, 0, 0, 0 };
            const auto qs = ogran::waterFill (dem, e.getGridParams().grainLimit);
            r.q = qs.q[0];
            ogran::WindowSel win; win.step = ogran::WindowMorphBank::stepFor (st.window); win.tilt = tiltFor ((double) st.windowSkew);
            const double pw = ogran::streamPower (r.n, r.q, win, r.c, 0.0, e.getBank());
            r.pRaw = pw;
            r.g = ogran::outputGainFromPower (pw);
            r.gEngine = e.getGOutTarget (0);
            r.g21 = ogran::streamOutputGain ((double) st.density, (double) st.sizeMs, kFs, e.getGridParams().grainLimit);
            const double sigma2 = s.sine ? 0.125 : 0.5 * (bl.power[0] + bl.power[1]);
            r.target = sigma2 * (double) r.g * (double) r.g * pw;
            r.lawOk = juce::exactlyEqual (r.g, r.gEngine) && juce::exactlyEqual (e.getQuotas().q[0], r.q);
            const auto& d = e.diag;
            r.steals = d.steals; r.kills = d.kills; r.spawns = d.spawns;
            const auto& last = d.record (d.logCount - 1);
            r.latchOk = d.logCount > 0 && last.windowStep == s.step && juce::exactlyEqual (last.tiltT, tiltFor (s.skewPct / 100.0).t)
                        && juce::exactlyEqual (last.gainL, 1.0f) && juce::exactlyEqual (last.gainR, 1.0f);
        });
        return res;
    }

    double avgDb (const std::vector<LvlRes>& r, size_t from, size_t n, bool target = false)
    {
        double a = 0.0;
        for (size_t k = from; k < from + n; ++k) a += target ? r[k].target : r[k].meas;
        return db (a / (double) n);
    }

    //--------------------------------------------------------------------------
    void morphRows()
    {
        std::printf ("\n-- W1. DSP-03 morph x skew level (N 40, c = 0, noise, 4 seeds)\n");
        const float skews[5] { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f };
        constexpr int kSeeds = 4;
        std::vector<LvlSpec> specs;
        for (int k = 0; k < 65; ++k)
            for (float sk : skews)
                for (int s = 0; s < kSeeds; ++s)
                {
                    LvlSpec x; x.step = k; x.skewPct = sk; x.seed = 0x31000 + (std::uint64_t) s;
                    specs.push_back (x);
                }
        // Capped corner + its reference (same N 400, q 128).
        const size_t corner = specs.size();
        for (int which = 0; which < 2; ++which)
            for (int s = 0; s < kSeeds; ++s)
            {
                LvlSpec x; x.step = which == 0 ? 22 : 0; x.skewPct = which == 0 ? -100.0f : 0.0f;
                x.density = 200.0f; x.sizeMs = 2000.0f; x.warm = 4.0; x.meas = 4.0; x.seed = 0x32000 + (std::uint64_t) s;
                specs.push_back (x);
            }
        const auto t0 = std::chrono::steady_clock::now();
        const auto r = runLevels (specs);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();

        const size_t refIdx = (size_t) 2 * kSeeds;   // step 0, skew 0 (index 2 of skews)
        const double ref = avgDb (r, refIdx, kSeeds);
        double worst = 0.0, worstAbs = 0.0, rawLo = 1e9, rawHi = -1e9;
        int wk = -1; float wsk = 0; bool sane = true; int cellsBad = 0;
        static const ogran::WindowMorphBank bank;
        for (int k = 0; k < 65; ++k)
            for (int si = 0; si < 5; ++si)
            {
                const size_t i0 = ((size_t) k * 5 + (size_t) si) * kSeeds;
                const double d = avgDb (r, i0, kSeeds) - ref;
                const double dAbs = avgDb (r, i0, kSeeds) - avgDb (r, i0, kSeeds, true);
                if (std::abs (d) > std::abs (worst)) { worst = d; wk = k; wsk = skews[si]; }
                worstAbs = juce::jmax (worstAbs, std::abs (dAbs));
                cellsBad += std::abs (d) <= 1.0 ? 0 : 1;
                for (size_t s = i0; s < i0 + kSeeds; ++s)
                    sane = sane && r[s].lawOk && r[s].latchOk && juce::exactlyEqual (r[s].c, 0.0) && r[s].steals + r[s].kills == 0 && r[s].g < 1.0f;
                // What the level would do without g_out (the normalisation's work): μ₂(k, skew) vs Hann.
                ogran::WindowSel w; w.step = k; w.tilt = tiltFor (skews[si] / 100.0);
                const double raw = db (bank.cum (2, k, w.tilt, 1.0) / ogran::kHannMu2);
                rawLo = juce::jmin (rawLo, raw); rawHi = juce::jmax (rawHi, raw);
            }
        const bool live = sane && rawHi - rawLo > 5.0;
        row (cellsBad == 0 && live, "W1 65 steps x 5 skews, N 40: level within +-1 dB of step 0 / skew 0",
             juce::String::formatted ("worst %+.3f dB at step %d skew %+.0f %%; vs law target worst %.3f dB; un-normalised mu2 span %.2f dB (%+.2f..%+.2f); %d bad cells",
                                      worst, wk, wsk, worstAbs, rawHi - rawLo, rawLo, rawHi, cellsBad)
                 + (live ? "" : "  VACUOUS/INSANE"));

        const double dC = avgDb (r, corner, kSeeds) - avgDb (r, corner + kSeeds, kSeeds);
        const auto& rc = r[corner];
        const bool cLive = rc.steals > 0 && juce::exactlyEqual (rc.q, 128.0) && rc.pRaw / ogran::kHannMu2 > 300.0 && rc.lawOk && rc.latchOk;
        row (std::abs (dC) <= 1.0 && cLive, "W1 capped corner N 400 / q 128, step 22 skew -1 vs Hann: +-1 dB",
             juce::String::formatted ("%+.3f dB (vs law %+.3f / Hann vs law %+.3f); P/mu2H %.1f, g_out %.2f dB, steals %lld",
                                      dC, avgDb (r, corner, kSeeds) - avgDb (r, corner, kSeeds, true),
                                      avgDb (r, corner + kSeeds, kSeeds) - avgDb (r, corner + kSeeds, kSeeds, true),
                                      rc.pRaw / ogran::kHannMu2, 20.0 * std::log10 ((double) rc.g), (long long) rc.steals)
                 + (cLive ? "" : "  VACUOUS"));
        std::printf ("  [INFO] W1: %d renders in %.1f s wall (parallel)\n", (int) specs.size(), secs);
    }

    //--------------------------------------------------------------------------
    void densityRows()
    {
        std::printf ("\n-- W2. DSP-03 density flatness (size 1 s, crosses cap 128) + W2n / W3 / R11 / band\n");
        const float dens[10] { 1, 2, 5, 10, 20, 50, 100, 128, 160, 200 };
        struct CRow { const char* name; float spray, reverse; double c; };
        const CRow crow[3] { { "c=1 (spray 0)", 0.0f, 0.0f, 1.0 }, { "c=0 (spray 10 %)", 10.0f, 0.0f, 0.0 }, { "c=0.5 (reverse 50 %)", 0.0f, 50.0f, 0.5 } };
        struct Src { const char* name; bool sine; int seeds; };
        const Src src[2] { { "noise", false, 4 }, { "sine 440", true, 16 } };   // R11: >= 8 seeds

        std::vector<LvlSpec> specs;
        int r11Checked = 0, r11Bad = 0;
        auto add = [&] (LvlSpec x) { ++r11Checked; r11Bad += r11Allowed (x) ? 0 : 1; specs.push_back (x); };
        struct Cell { size_t at; int n; };
        Cell cells[2][3][2][10];   // [seedSet][c][src][density]
        for (int set = 0; set < 2; ++set)
            for (int ci = 0; ci < 3; ++ci)
                for (int si = 0; si < 2; ++si)
                    for (int di = 0; di < 10; ++di)
                    {
                        cells[set][ci][si][di] = { specs.size(), src[si].seeds };
                        for (int s = 0; s < src[si].seeds; ++s)
                        {
                            LvlSpec x; x.density = dens[di]; x.sizeMs = 1000.0f; x.spray = crow[ci].spray; x.reverse = crow[ci].reverse;
                            x.sine = src[si].sine; x.sineHz = 440.0; x.warm = 2.0; x.meas = 10.0;
                            x.seed = 0x4000000ull * (std::uint64_t) (set + 1) + 0x1000 * (std::uint64_t) (ci * 2 + si) + (std::uint64_t) (di * 16 + s);
                            add (x);
                        }
                    }
        // W3 sub-floor (sine — the peak is the input peak × window).
        const size_t w3 = specs.size();
        { LvlSpec x; x.step = 64; x.skewPct = -100.0f; x.density = 1.0f; x.sizeMs = 100.0f; x.spray = 10.0f; x.sine = true; x.sineHz = 440.0;
          x.warm = 0.0; x.meas = 6.0; x.seed = 0x5300; add (x); }
        // Band documentation (spray 0.03 %, N 20): noise ×4, sine 110 Hz ×8.
        const size_t band = specs.size();
        for (int s = 0; s < 12; ++s)
        {
            LvlSpec x; x.density = 20.0f; x.sizeMs = 1000.0f; x.spray = 0.03f; x.sine = s >= 4; x.sineHz = 110.0;
            x.warm = 2.0; x.meas = 10.0; x.seed = 0x5400 + (std::uint64_t) s; add (x);
        }

        const auto t0 = std::chrono::steady_clock::now();
        const auto r = runLevels (specs);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();

        // W2 rows (seed set 0) and the W2o control (set 0 vs set 1).
        // W2o gates every cell whose measurement is NOT R11-variance-limited (noise
        // c = 1/0/0.5, sine c = 1). The incoherent sine cells (c = 0 / 0.5) are a
        // random-phasor sum: ~1 independent configuration per grain length, so
        // +-0.5 dB set-to-set would need ~70 × 10 s renders per cell — their spread
        // is printed (INFO), and their W2 margin is read against it.
        double ctrlWorst = 0.0, sineSpread = 0.0; juce::String ctrlWhere, sineWhere;
        for (int ci = 0; ci < 3; ++ci)
            for (int si = 0; si < 2; ++si)
            {
                juce::String det;
                double worst = 0.0; bool sane = true; int bad = 0; bool crossed = false;
                for (int di = 0; di < 10; ++di)
                {
                    const auto& cl = cells[0][ci][si][di];
                    const double d = avgDb (r, cl.at, (size_t) cl.n) - avgDb (r, cl.at, (size_t) cl.n, true);
                    if (std::abs (d) > std::abs (worst)) worst = d;
                    bad += std::abs (d) <= 1.5 ? 0 : 1;
                    for (size_t k = cl.at; k < cl.at + (size_t) cl.n; ++k)
                        sane = sane && r[k].lawOk && std::abs (r[k].c - crow[ci].c) < 1e-12;
                    crossed = crossed || r[cl.at].n > r[cl.at].q;
                    det << juce::String::formatted ("%g:%+.2f ", (double) dens[di], d);
                    const auto& c2 = cells[1][ci][si][di];
                    const double o = avgDb (r, cl.at, (size_t) cl.n) - avgDb (r, c2.at, (size_t) c2.n);
                    const bool r11Limited = src[si].sine && crow[ci].c < 1.0;
                    auto& w = r11Limited ? sineSpread : ctrlWorst;
                    if (std::abs (o) > std::abs (w))
                    {
                        w = o;
                        (r11Limited ? sineWhere : ctrlWhere) = juce::String::formatted ("%s %s d %g", crow[ci].name, src[si].name, (double) dens[di]);
                    }
                }
                const bool live = sane && crossed;
                row (bad == 0 && live, juce::String ("W2 ") + crow[ci].name + ", " + src[si].name + ": density 1..200 within +-1.5 dB of law",
                     juce::String::formatted ("worst %+.2f dB; ", worst) + det + (live ? "" : "  VACUOUS/INSANE"));
            }
        row (std::abs (ctrlWorst) <= 0.5, "W2o old-vs-old control: 2nd seed set, noise x3 + sine c=1 (40 cells) +-0.5 dB",
             juce::String::formatted ("worst %+.3f dB (%s)", ctrlWorst, ctrlWhere.toRawUTF8()));
        std::printf ("  [INFO] W2o incoherent sine (c = 0 / 0.5, 20 cells, 16 seeds): seed-set spread worst %+.3f dB (%s) - R11 variance, not gated\n",
                     sineSpread, sineWhere.toRawUTF8());

        // W2n — the 2.1 hot spot (noise and sine, c = 1, density 200).
        {
            juce::String det; bool ok = true;
            for (int si = 0; si < 2; ++si)
            {
                const auto& cl = cells[0][0][si][9];
                const double d22 = avgDb (r, cl.at, (size_t) cl.n) - avgDb (r, cl.at, (size_t) cl.n, true);
                const auto& x = r[cl.at];
                const double d21 = d22 + 20.0 * std::log10 ((double) x.g21 / (double) x.g);
                ok = ok && d21 > 15.0 && std::abs (d22) <= 1.5;
                det << juce::String::formatted ("%s: 2.1 law %+.2f dB, 2.2 %+.2f dB (g21 %.4f g22 %.4f); ", src[si].name, d21, d22, x.g21, x.g);
            }
            row (ok, "W2n c = 1, density 200: 2.1 law > +15 dB (control), 2.2 within +-1.5 dB", det);
        }

        // W3 — sub-floor, no boost.
        {
            const auto& x = r[w3];
            Job j = lvlJob (specs[w3]);
            const auto o = eg::render (j);   // re-render (the batch drops the audio)
            double peak = 0.0;
            for (size_t i = 0; i < o.L.size(); ++i) peak = juce::jmax (peak, (double) std::abs (o.L[i]), (double) std::abs (o.R[i]));
            const float* tab = o.eng().getBank().table (64);
            double wPeak = 0.0;
            for (int i = 0; i < ogran::WindowMorphBank::kPoints; ++i) wPeak = juce::jmax (wPeak, (double) tab[i]);
            const double lim = 0.5 * wPeak * 1.0;   // input peak × window peak × level (0 dB)
            const double unfloored = 1.0 / std::sqrt (x.pRaw / ogran::kHannMu2);
            const bool live = x.spawns >= 4 && unfloored > 1.0 && peak > 0.8 * lim;
            row (juce::exactlyEqual (x.gEngine, 1.0f) && peak <= lim * 1.01 && live,
                 "W3 sub-floor (100 ms, 1 g/s, Expo skew -1): g_out == 1, peak <= input x window peak (no boost)",
                 juce::String::formatted ("g_out %.9g; peak %.5f vs limit %.5f (ratio %.5f); unfloored gain would be %+.1f dB (P/mu2H %.4f); %lld grains",
                                          x.gEngine, peak, lim, peak / lim, 20.0 * std::log10 (unfloored), x.pRaw / ogran::kHannMu2, (long long) x.spawns)
                     + (live ? "" : "  VACUOUS"));
        }

        // R11 guard.
        {
            LvlSpec bad; bad.sine = true; bad.jitter = 0.0f; bad.pitch = 12.0f;
            const bool rejects = ! r11Allowed (bad);
            row (r11Bad == 0 && rejects && r11Checked > 300, "R11 guard: no sine spec at jitter 0 with pitch != 0 (builder predicate)",
                 juce::String::formatted ("%d specs checked, %d violations; predicate rejects (sine, jitter 0, +12 st): %d", r11Checked, r11Bad, (int) rejects));
        }

        // Band documentation (INFO).
        {
            const double dn = avgDb (r, band, 4) - avgDb (r, band, 4, true);
            const double ds = avgDb (r, band + 4, 8) - avgDb (r, band + 4, 8, true);
            const auto& x = r[band];
            const double excess = 10.0 * std::log10 (x.n * 0.25 / 0.375);   // coherent excess N·μ₁²/μ₂ (Hann)
            std::printf ("  [INFO] BAND spray 0.03 %% (W %.0f smp < W0 %.0f), N %.0f, c %.4f: noise %+.2f dB, sine 110 Hz %+.2f dB vs law (coherent excess bound %.1f dB) - documented, not gated\n",
                         0.0003 * 8.0 * kFs, 0.005 * kFs, x.n, x.c, dn, ds, excess);
        }
        std::printf ("  [INFO] W2/W3/band: %d renders in %.1f s wall (parallel)\n", (int) specs.size(), secs);
    }

    //==========================================================================
    // K1 — steal click
    //==========================================================================
    void stealClickRows()
    {
        std::printf ("\n-- K1. QUAL-01 steal click (sine 440 @ 0.001, spray 0, linear wet sum)\n");
        constexpr juce::int64 from = (juce::int64) (3.0 * kFs), to = (juce::int64) (7.0 * kFs);
        auto job = [] (float density, int limIdx)
        {
            Job j; j.len = (juce::int64) (7.0 * kFs); j.block = 512; j.stim = eg::sineStim (440.0, kFs, 0.001);
            j.params = eg22::base ({ 1, 2, 3, 4 });
            j.params.push_back ({ "grain_limit", (float) limIdx });
            for (int s = 1; s <= 4; ++s)
                for (auto& kv : P { { sp (s, "density"), density }, { sp (s, "size"), 1000.0f }, { sp (s, "spray"), 0.0f }, { sp (s, "jitter"), 20.0f } })
                    j.params.push_back (kv);
            return j;
        };
        auto kappa = [] (const std::vector<float>& x)
        {
            double e = 0.0;
            for (auto i = from; i < to; ++i) e += (double) x[(size_t) i] * x[(size_t) i];
            const double rms = std::sqrt (e / (double) (to - from));
            return eg::maxCurv (x, (size_t) from, (size_t) to) / juce::jmax (1e-30, rms);
        };
        auto kap2 = [&] (const Out& o) { return juce::jmax (kappa (o.L), kappa (o.R)); };

        eg22::RecLog Lr, La, Lb;
        auto jr = job (20.0f, 3); jr.after = eg22::collect (Lr);
        auto ja = job (20.0f, 0); ja.after = eg22::collect (La);
        auto jb = job (50.0f, 3); jb.after = eg22::collect (Lb);
        const auto oref = eg::render (jr);
        const auto oa = eg::render (ja);
        const auto ob = eg::render (jb);
        auto inWin = [] (const eg22::RecLog& L, bool stolen)
        {
            int n = 0;
            for (auto& r : L.r) if (r.onsetSample >= from && r.onsetSample < to && (stolen ? r.stolen : r.killed)) ++n;
            return n;
        };
        const double kr = kap2 (oref);
        const auto& dr = oref.eng().diag;
        const bool refClean = dr.steals == 0 && dr.kills == 0 && dr.spawns > 400;
        const int ka = inWin (La, false), sb = inWin (Lb, true);
        const double ra = kap2 (oa) / kr, rb = kap2 (ob) / kr;
        row (refClean && ka > 50 && ra <= 2.0 && ! La.overflow,
             "K1a cap 32 (4 x 20 g/s x 1 s, R3 kills): click kappa <= 2.0x steal-free ref",
             juce::String::formatted ("ratio %.3f; %d kills in window (total %lld); ref kappa %.4g, %lld spawns 0 steals/kills",
                                      ra, ka, (long long) oa.eng().diag.kills, kr, (long long) dr.spawns) + (refClean && ka > 50 ? "" : "  VACUOUS"));
        row (refClean && sb > 50 && rb <= 2.0 && ! Lb.overflow,
             "K1b cap 128 (4 x 50 g/s x 1 s, native steals): click kappa <= 2.0x ref",
             juce::String::formatted ("ratio %.3f; %d steals in window (total %lld, kills %lld)", rb, sb,
                                      (long long) ob.eng().diag.steals, (long long) ob.eng().diag.kills) + (sb > 50 ? "" : "  VACUOUS"));

        // K1n — one grain hard-cut in subject (a): at the loudest in-window kill
        // onset, the coherent sum loses that grain's share 0.9/S1 at once.
        {
            const auto& gpS = oa.eng().getGridParams();
            double s1 = 0.0;
            for (int s = 0; s < 4; ++s)
            {
                const double n = ogran::streamDemand ((double) gpS.s[s].density, (double) gpS.s[s].sizeMs, kFs);
                const double q = oa.eng().getQuotas().q[s];
                const double x = juce::jmin (1.0, q / n);
                s1 += n <= q ? n * 0.5 : q * ogran::hannCumW (x) / x;
            }
            const double frac = 0.9 / s1;
            juce::int64 k = -1; double best = -1.0;
            for (auto& r : La.r)
                if (r.killed && r.onsetSample >= from + 64 && r.onsetSample < to - 64 && std::abs (oa.L[(size_t) r.onsetSample]) > best)
                { best = std::abs (oa.L[(size_t) r.onsetSample]); k = r.onsetSample; }
            auto L = oa.L, R = oa.R;
            for (juce::int64 i = juce::jmax ((juce::int64) 0, k); k >= 0 && i < (juce::int64) L.size(); ++i)
            { L[(size_t) i] *= (float) (1.0 - frac); R[(size_t) i] *= (float) (1.0 - frac); }
            const double rn = juce::jmax (kappa (L), kappa (R)) / kr;
            row (k >= 0 && rn > 2.0, "K1n control: one grain hard-cut at a kill in (a) reads > 2.0x",
                 juce::String::formatted ("ratio %.2f (cut %.4f of the cloud = 0.9/S1, S1 %.2f, @%lld)", rn, frac, s1, (long long) k)
                     + (rn > 2.0 ? "" : "  CONTROL DID NOT FAIL"));
        }
    }

    //==========================================================================
    // X1 — mixer ramps vs the same-motion reference
    //==========================================================================
    void mixerRampRows()
    {
        std::printf ("\n-- X1. FUNC-03 mute / solo / level / pan ramps (same-motion reference, 10 ms)\n");
        struct Ev { juce::int64 at; const char* what; const char* id; float v; };
        const juce::int64 g = ogran::kCell;
        const Ev evs[] = {
            { 1500 * g, "s1 mute on",   "s1_mute",  1.0f },
            { 2250 * g, "s1 mute off",  "s1_mute",  0.0f },
            { 3000 * g, "s2 solo on",   "s2_solo",  1.0f },
            { 3750 * g, "s2 solo off",  "s2_solo",  0.0f },
            { 4500 * g, "level -12 dB", "s1_level", -12.0f },
            { 5250 * g, "level +4 dB",  "s1_level", 4.0f },
            { 6000 * g, "pan +100",     "s1_pan",   100.0f },
            { 6750 * g, "pan -50",      "s1_pan",   -50.0f },
        };
        auto stim = [] (juce::int64 i, int ch)
        {
            const double f = ch == 0 ? 220.0 : 331.0;
            return (float) (0.5 * std::sin (2.0 * eg::kPi * f * (double) i / kFs));
        };
        auto job = [&] (bool withEvents)
        {
            Job j; j.len = 7500 * g; j.block = 512; j.stim = stim;
            j.params = eg22::base ({ 1, 2 });
            for (auto& kv : P { { "s1_density", 50.0f }, { "s1_size", 200.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 20.0f },
                                { "s2_density", 10.0f }, { "s2_size", 100.0f }, { "s2_level", -60.0f } })
                j.params.push_back (kv);
            if (withEvents)
                for (auto& e : evs)
                {
                    const juce::String id = e.id; const float v = e.v;
                    j.events.push_back ({ e.at, [id, v] (Proc& p) { eg::setP (p, id, v); } });
                }
            return j;
        };
        const auto subj = eg::render (job (true));
        const auto stat = eg::render (job (false));
        const int cells = subj.eng().getMixerRampCells();

        // Same-motion model: the engine's per-grid ExactRamp (15 cells) → CellLerp
        // trajectory of (level · audible) · panGains, driven by the same events;
        // HARD: the same targets switched at the event sample.
        const size_t n = subj.L.size();
        std::vector<float> mL (n), mR (n), hL (n), hR (n);
        {
            ogran::ExactRamp lv, pn, au;
            lv.reset (1.0f); pn.reset (0.0f); au.reset (1.0f);
            ogran::CellLerp cl, cr;
            cl.reset (1.0f); cr.reset (1.0f);
            float levelDb = 0.0f, pan = 0.0f; bool mute = false, solo2 = false;
            size_t ei = 0;
            for (juce::int64 c = 0; c * g < (juce::int64) n; ++c)
            {
                while (ei < std::size (evs) && evs[ei].at <= c * g)
                {
                    const juce::String id = evs[ei].id;
                    if (id == "s1_mute") mute = evs[ei].v > 0.5f;
                    else if (id == "s2_solo") solo2 = evs[ei].v > 0.5f;
                    else if (id == "s1_level") levelDb = evs[ei].v;
                    else pan = evs[ei].v / 100.0f;
                    ++ei;
                }
                lv.setTarget (ogran::levelDbToGain (levelDb), cells); lv.tick();
                pn.setTarget (pan, cells); pn.tick();
                au.setTarget ((! mute && ! solo2) ? 1.0f : 0.0f, cells); au.tick();
                const auto pg = ogran::panGains (pn.current());
                const float gg = lv.current() * au.current();
                cl.push (gg * pg.a); cr.push (gg * pg.b);
                const auto hp = ogran::panGains (pan);
                const float hg = ogran::levelDbToGain (levelDb) * ((! mute && ! solo2) ? 1.0f : 0.0f);
                for (int i = 0; i < g && (size_t) (c * g + i) < n; ++i)
                {
                    const size_t s = (size_t) (c * g + i);
                    mL[s] = stat.L[s] * cl.at (i); mR[s] = stat.R[s] * cr.at (i);
                    hL[s] = stat.L[s] * hg * hp.a; hR[s] = stat.R[s] * hg * hp.b;
                }
            }
        }
        Tally t;
        juce::String det;
        double worstRatio = 0.0, minHard = 1e30, worstStatic = 0.0;
        for (auto& e : evs)
        {
            const size_t a = (size_t) (e.at - 4 * g), b = (size_t) (e.at + (cells + 4) * g);
            double ratio = 0.0, hard = 0.0, vsStatic = 0.0, moved = 0.0;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto& s = ch ? subj.R : subj.L;
                const auto& m = ch ? mR : mL;
                const auto& h = ch ? hR : hL;
                const auto& st = ch ? stat.R : stat.L;
                const double cm = juce::jmax (1e-12, eg::maxCurv (m, a, b));
                ratio = juce::jmax (ratio, eg::maxCurv (s, a, b) / cm);
                hard = juce::jmax (hard, eg::maxCurv (h, a, b) / cm);
                vsStatic = juce::jmax (vsStatic, eg::maxCurv (s, a, b) / juce::jmax (1e-12, eg::maxCurv (st, a, b)));
                for (size_t i = a; i < b; ++i) moved = juce::jmax (moved, std::abs ((double) s[i] - (double) st[i]));
            }
            worstRatio = juce::jmax (worstRatio, ratio); minHard = juce::jmin (minHard, hard); worstStatic = juce::jmax (worstStatic, vsStatic);
            t.check (ratio < 1.5 && hard > 10.0 && moved > 0.01, e.what);
            det << juce::String::formatted ("%s %.3f/%.0fx; ", e.what, ratio, hard);
        }
        // Mute arrives at exactly 0 at event + 15 cells (and not before).
        juce::int64 zeroCell = -1;
        {
            const juce::int64 ev = evs[0].at, end = evs[1].at;
            for (juce::int64 c = ev / g; c * g < end; ++c)
            {
                bool allZero = true;
                for (juce::int64 i = c * g; i < end && allZero; ++i) allZero = juce::exactlyEqual (subj.L[(size_t) i], 0.0f) && juce::exactlyEqual (subj.R[(size_t) i], 0.0f);
                if (allZero) { zeroCell = c - ev / g; break; }
            }
        }
        row (t.bad == 0 && cells == 15 && zeroCell == 15,
             "X1 mute/solo/level/pan: kappa < 1.5x same-motion ref, hard switch > 10x, 15-cell ramp",
             juce::String::formatted ("worst %.4f, hard-switch min %.1fx; mute silent from event + %lld cells (ramp %d cells); ",
                                      worstRatio, minHard, (long long) zeroCell, cells) + det
                 + juce::String::formatted ("[INFO vs static: worst %.2fx - the ramp kink itself, not a defect]", worstStatic));
    }

    //==========================================================================
    // N5 — block-size null, 4 streams, everything on
    //==========================================================================
    void nullRows()
    {
        std::printf ("\n-- N5. PERF-03 block-size null, 4 streams, all randomisation + scale + toggles + custom swap\n");
        Job base; base.len = (juce::int64) (6.0 * kFs); base.prepBlock = 512;
        base.params = { { "mix", 70.0f }, { "buffer_length", 4.0f }, { "grain_limit", 0.0f } };
        const float dens[4] { 100, 60, 40, 30 }, size[4] { 500, 300, 800, 200 }, scale[4] { 3, 31, 0, 24 }, prand[4] { 12, 7, 5, 9 };
        for (int s = 1; s <= 4; ++s)
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), dens[s - 1] }, { sp (s, "size"), size[s - 1] },
                                { sp (s, "size_rand"), 50.0f }, { sp (s, "spray"), 50.0f }, { sp (s, "jitter"), 50.0f },
                                { sp (s, "position"), 30.0f }, { sp (s, "pitch_rand"), prand[s - 1] }, { sp (s, "reverse_prob"), 50.0f },
                                { sp (s, "window"), 0.6f }, { sp (s, "window_skew"), -40.0f }, { sp (s, "stereo_spread"), 70.0f },
                                { sp (s, "gain_rand"), 50.0f }, { sp (s, "scale"), scale[s - 1] }, { sp (s, "pan"), -60.0f + 40.0f * s } })
                base.params.push_back (kv);
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

        Job j32 = base; j32.block = 32;
        eg22::RecLog L32; j32.after = eg22::collect (L32);
        const auto ref = eg::render (j32);
        const auto& rd = ref.eng().diag;
        int streams = 0; { bool seen[4] {}; for (auto& r : L32.r) seen[r.stream] = true; for (bool b : seen) streams += b ? 1 : 0; }
        int nonHann = 0, scaled = 0;
        for (auto& r : L32.r) { nonHann += r.windowStep != 0 ? 1 : 0; scaled += juce::exactlyEqual (r.pitchCents, 0.0f) ? 0 : 1; }
        const bool live = rd.kills > 0 && rd.spawns > 500 && streams == 4 && rd.customCopies >= 1 && rd.activeSetRebuilds >= 3
                          && nonHann > 100 && scaled > 100 && ref.eng().getEpoch (3) > 0;
        Tally t;
        for (int b : { 441, 512, 4096, 0 })
        {
            Job jb = base; jb.block = b;
            const auto o = eg::render (jb);
            const auto d = eg::diff (ref, o);
            const bool ok = d.count == 0 && o.eng().diag.spawns == rd.spawns && o.eng().diag.reachViolations == 0;
            t.check (ok, (b == 0 ? juce::String ("var") : juce::String (b)) + juce::String::formatted (": %d diffs max %.3g @%lld", d.count, d.maxAbs, (long long) d.first));
        }
        row (t.bad == 0 && live && rd.reachViolations == 0, "N5 4 streams, everything on: 441/512/4096/var vs 32 max|d| == 0",
             t.detail (4) + juce::String::formatted ("  [spawns %lld kills %lld steals %lld, 4 streams %d, custom copies %lld, rebuilds %lld, non-Hann %d, picked %d]",
                                                     (long long) rd.spawns, (long long) rd.kills, (long long) rd.steals, streams,
                                                     (long long) rd.customCopies, (long long) rd.activeSetRebuilds, nonHann, scaled)
                 + (live ? "" : "  VACUOUS"));
        Job js = j32; js.seed = base.seed ^ 0x1ull; js.after = {};
        const auto d = eg::diff (ref, eg::render (js));
        row (d.count > 0, "N5c different-seed control differs", juce::String (d.count) + " diffs");
    }

    //==========================================================================
    // M2 — allocations
    //==========================================================================
    void allocRows()
    {
        std::printf ("\n-- M2. PERF-01 allocations, 4 streams + scale / custom / window / mute-solo sweeps\n");
       #if JUCE_MAC
        using namespace eg;
        audioThread = pthread_self();
        malloc_logger = countAllocation;
        allocCount = 0;
        allocTrace = false;   // M1 leaves it on; the liveness probe must not print a trace
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
                                { sp (s, "stereo_spread"), 50.0f }, { sp (s, "gain_rand"), 50.0f } })
                j.params.push_back (kv);
        j.pre = [] (Proc& p) { juce::String bad; p.setCustomScale ("m2a", "edo 9", bad); };
        // 0–3 s cap 128 (native steals, N 240); 3–6 s cap 32 (R3 kills); 9 s cap 64.
        j.events.push_back ({ (juce::int64) (3.0 * kFs), [] (Proc& p) { setP (p, "grain_limit", 0.0f); } });
        j.events.push_back ({ (juce::int64) (9.0 * kFs), [] (Proc& p) { setP (p, "grain_limit", 1.0f); } });
        for (int k = 1; k < 115; ++k)
        {
            const juce::int64 at = (juce::int64) (0.1 * k * kFs) + 7;
            j.events.push_back ({ at, [k] (Proc& p)
            {
                const int s = 1 + (k % 4);
                switch (k % 6)
                {
                    case 0: setP (p, sp (s, "scale"), (float) (1 + (k * 5) % 31)); break;
                    case 1: p.setDegreeMask (s, 1 + (k * 5) % 31, (std::uint64_t) 0x9E3779B97F4A7C15ull * (std::uint64_t) k); break;
                    case 2: setP (p, sp (s, "pitch_rand"), (float) ((k * 3) % 24)); break;
                    case 3: setP (p, sp (s, "window"), (float) ((k * 7) % 65) / 64.0f); setP (p, sp (s, "window_skew"), (float) ((k * 37) % 201) - 100.0f); break;
                    case 4: setP (p, sp (s, "mute"), (k / 6) % 2 ? 1.0f : 0.0f); setP (p, sp (1 + (k + 1) % 4, "solo"), (k / 6) % 3 == 0 ? 1.0f : 0.0f); break;
                    default: { juce::String bad; p.setCustomScale ("m2", "edo " + juce::String (5 + k % 13), bad); setP (p, sp (s, "scale"), 31.0f); } break;
                }
            } });
        }
        for (int k = 0; k < 6; ++k)
            j.events.push_back ({ (juce::int64) ((6.0 + 0.5 * k) * kFs) + 13, [k] (Proc& p) { setP (p, "s3_enable", (k % 2) ? 1.0f : 0.0f); } });

        armDuringProcess = false;
        j.after = [] (Proc&, juce::int64) { armDuringProcess = true; };   // first block unarmed (warm-up)
        const auto o = render (j);
        armDuringProcess = false;
        malloc_logger = nullptr;
        allocTrace = false;
        const auto& dg = o.eng().diag;
        const int n = allocCount;
        const bool live = dg.steals > 0 && dg.kills > 0 && dg.activeSetRebuilds > 20 && dg.customCopies >= 10 && o.eng().getEpoch (2) > 0;
        row (liveness == 1, "M2 liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
        row (liveness == 1 && n == 0 && live, "M2 0 allocations: 4 streams, rebuilds, custom swaps, window/skew, mute/solo, steals/kills",
             juce::String::formatted ("%d allocs; spawns %lld steals %lld kills %lld, rebuilds %lld, custom copies %lld (misses %lld)", n,
                                      (long long) dg.spawns, (long long) dg.steals, (long long) dg.kills, (long long) dg.activeSetRebuilds,
                                      (long long) dg.customCopies, (long long) dg.customTryFails) + (live ? "" : "  VACUOUS"));
       #else
        row (false, "M2 alloc gate is macOS-only");
       #endif
    }

    //==========================================================================
    // C2 — CPU (non-blocking)
    //==========================================================================
    void cpuRow()
    {
        std::printf ("\n-- C2. CPU, 4 streams (non-blocking, recorded)\n");
        auto p = make();
        P ps { { "mix", 100.0f }, { "grain_limit", 3.0f } };
        for (int s = 1; s <= 4; ++s)
            for (auto& kv : P { { sp (s, "enable"), 1.0f }, { sp (s, "density"), 16.0f }, { sp (s, "size"), 2000.0f },
                                { sp (s, "window"), 0.6f }, { sp (s, "scale"), 1.0f }, { sp (s, "pitch_rand"), 2.0f } })
                ps.push_back (kv);
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
        int alive = 0;
        for (int s = 0; s < ogran::kPoolLive; ++s) alive += p->getEngineForTest().getPool().isActive (s) ? 1 : 0;
        const auto t0 = std::chrono::steady_clock::now();
        runFor (10.0);
        const double frac = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / 10.0;
        std::printf ("  [INFO] C2 4 streams x 32 grains (live %d), window 0.6, scale Chromatic + pitch_rand, 48 k / 128: %.2f %% of one core (2.1 C1 baseline 8.28 %%: %.2fx)\n",
                     alive, frac * 100.0, frac * 100.0 / 8.28);
    }
} } // namespace eg22b (anonymous)

static void engineRows22b()
{
    std::printf ("\n== E0+/W/K/X/N5/M2/C2. Stage 2 / Phase 2.2 law + T16 rows (engine_gates_22b.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg22b::lawUnitRows();
    eg22b::morphRows();
    eg22b::densityRows();
    eg22b::stealClickRows();
    eg22b::mixerRampRows();
    eg22b::nullRows();
    eg22b::allocRows();
    eg22b::cpuRow();
    std::printf ("  [INFO] 2.2b rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
