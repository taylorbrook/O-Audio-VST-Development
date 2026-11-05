/*
  ==============================================================================

    O-Granulator — render harness, section E (Stage 2 / Phase 2.1 engine rows)

    Included by main.cpp (uses its row / Tally / make / param helpers). Every row
    prints PASS/FAIL and counts toward the exit code. Seeds via setSeed. Stimuli
    are POSITION-deterministic (a hash or closed form of the absolute sample
    index), never a sequential RNG. Negative controls that pass anyway FAIL as
    "vacuous gate".

      E0  output law units (T3): ∫₀¹ w² = 0.375; continuity at N = q;
          200 g/s × 2 s, q 128 → P/μ₂ = 43.02 (−4.73 dB vs naive min(N, q) = 128).
          PLAN T3/T14 says ≈ 61 / −3.2 dB — WRONG (M(0.32) = 0.12604, not ≈ 0.18).
          g_out exactly 1.0f at N <= 1 and 0.5f at N = 4.
      E1  FUNC-01 audible at position 0 (energy within interval + len) + control
      E2  FUNC-01 buffer_length change: (a) span bound on post-change spawns,
          (b) 0 reach violations, (c) step-click verdict vs same-motion reference
      E3  FUNC-07 mix 100 %: |corr (out, dry)| < 0.05 (control: mix 50 % > 0.5)
      E4  R2: output/input gain & s1_level default after a normalised round trip
          → gain exactly 1.0f (and the unsnapped gain is not)
      A1  DSP-02 alias, processor end-to-end, 44.1 k, +24 st: out-of-band tones +
          out-of-band sweep ≥ 60 dB below the in-band reference; forceLevel0 FAILS
      L1  DSP-01 long buffer: periodic-corpus null (grain early vs grain ≥ 25 s;
          and a 25.5 s-reach pair) for rates 0.5 / ~1.5 / 4
      L2  DSP-01 double reference: readAbs vs anchor + dir·rate·(age + frac) at
          25 s, |err| <= 1e-6
      N1  PERF-03 block-size null 32 / 441 / 512 / 4096 / variable (1, 8192),
          every s1 randomisation ON, grain_limit 32 (R3 kills inside), param
          change at a shared boundary, s1_enable off→on; max|Δ| == 0
      N2  different-seed control differs
      N3  R1 negative control: disableFracQuant, 20 s, 128 grains, 441 vs 32 → ≥ 1
          diff (and with R1 on → 0 diffs)
      N4  R4: second-burst start D set ≠ first; globalIndex continues the epoch
      O1  oversize: prepare 512, process 8192 / 4096 at position 0 — finite,
          0 reach violations, engine canaries + host-buffer guards intact
      M1  PERF-01: 0 heap allocations on the render thread across spawn / native
          steal / R3 kill / enable toggles / buffer_length sweeps; liveness = 1
      R3  PERF-02 grain_limit 32 / 64: maxAlive <= cap (engine + harness scan),
          kills > 0
      S0  PERF-02 steal-free bit-identity (PLAN-2.2 P9/T1): grain_limit 128, s1,
          N = 100 (density 100 × 1 s, jitter 20 %), 4 s — engine acquire vs
          TestHooks::nativeAcquire (bare tryObtain() ?: obtain (age·invLen));
          steals == kills == 0 on both runs (else vacuous); max|Δ| == 0
      S0n negative control: N = 300 (density 150 × 2 s) with the R3 cap engaged
          (grain_limit 64) — the pair must DIFFER (the hook really bypasses
          Spawner::acquire). INFO: the same N = 300 pair at grain_limit 128 is
          identical BY CONSTRUCTION on the 2.1 tree (acquire reduces to the
          native policy at cap 128 in every regime), so it cannot be the control
      C1  CPU baseline (non-blocking INFO): 128 grains, 48 k / 128

  ==============================================================================
*/
#pragma once

#include <algorithm>
#include <chrono>
#include <complex>
#include <functional>
#include <limits>
#include <vector>

#if JUCE_MAC
 #include <execinfo.h>
 #include <pthread.h>
extern "C"
{
    typedef void (malloc_logger_t) (uint32_t type, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                                    uintptr_t result, uint32_t numHotFramesToSkip);
    extern malloc_logger_t* malloc_logger;
}
#endif

namespace { namespace eg
{
    constexpr double kPi = 3.14159265358979323846;
    using Proc = GranulatorProcessor;
    using P    = std::vector<std::pair<juce::String, float>>;

    //==========================================================================
    // Allocation hook (O-Bells --alloc-check pattern; memory: malloc_logger two
    // traps). volatile flag + counter (clang folds a plain flag around malloc),
    // scoped to the render thread (JUCE's TimerThread mallocs at will).
   #if JUCE_MAC
    volatile bool allocArmed = false;
    volatile int  allocCount = 0;
    volatile bool allocTrace = false;
    pthread_t     audioThread;

    void countAllocation (uint32_t type, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t)
    {
        if (allocArmed && (type & 2u) != 0 && pthread_equal (pthread_self(), audioThread))   // MALLOC_LOG_TYPE_ALLOCATE
        {
            allocCount = allocCount + 1;
            if (allocTrace && allocCount == 1)
            {
                void* frames[32];
                backtrace_symbols_fd (frames, backtrace (frames, 32), 2);   // does not malloc
            }
        }
    }
   #endif
    bool armDuringProcess = false;   // render() arms the hook around processBlock only

    //==========================================================================
    // Host-normalised setter that lands EXACTLY on v when some normalised value
    // maps there (skewed ranges: search a few ulps around convertTo0to1 (v)).
    // Lands on the APVTS RAW value the engine reads (= convertFrom0to1 of the
    // parameter's getValue(), which re-normalises the stored value — NOT
    // AudioParameterFloat::get()).
    bool setP (juce::AudioProcessor& p, const juce::String& id, float v)
    {
        auto* rp = param (p, id);
        auto* gp = dynamic_cast<GranulatorProcessor*> (&p);
        if (rp == nullptr || gp == nullptr) { std::printf ("    !! unknown param %s\n", id.toRawUTF8()); return false; }
        auto* raw = gp->parameters.getRawParameterValue (id);
        const float n0 = rp->convertTo0to1 (v);
        float up = n0, dn = n0;
        for (int k = 0; k < 2048; ++k)
        {
            for (float c : { up, dn })
            {
                rp->setValueNotifyingHost (c);
                if (juce::exactlyEqual (raw->load(), v)) return true;
            }
            up = std::nextafter (up, 2.0f);
            dn = std::nextafter (dn, -1.0f);
        }
        rp->setValueNotifyingHost (n0);
        return false;
    }

    void setAll (juce::AudioProcessor& p, const P& ps)
    {
        for (auto& kv : ps) setP (p, kv.first, kv.second);
    }

    //==========================================================================
    // Position-deterministic stimuli.
    inline float hashNoise (juce::int64 i, int ch, float amp = 0.5f)
    {
        uint64_t x = static_cast<uint64_t> (i) * 0x9E3779B97F4A7C15ull + (ch != 0 ? 0xD1B54A32D192ED03ull : 0x2545F4914F6CDD1Dull);
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ull;
        x ^= x >> 27; x *= 0x94D049BB133111EBull;
        x ^= x >> 31;
        const double u = static_cast<double> (x >> 40) / static_cast<double> (1ull << 24);   // [0, 1)
        return static_cast<float> ((2.0 * u - 1.0) * amp);
    }
    inline std::function<float (juce::int64, int)> noiseStim (float amp = 0.5f)
    {
        return [amp] (juce::int64 i, int ch) { return hashNoise (i, ch, amp); };
    }
    inline std::function<float (juce::int64, int)> sineStim (double hz, double fs, double amp = 0.5)
    {
        return [hz, fs, amp] (juce::int64 i, int) { return static_cast<float> (amp * std::sin (2.0 * kPi * hz * static_cast<double> (i) / fs)); };
    }

    //==========================================================================
    struct Event { juce::int64 at; std::function<void (Proc&)> fn; };

    struct Job
    {
        double fs = 48000.0;
        int inCh = 2, outCh = 2;
        int prepBlock = 512;
        P params;
        uint64_t seed = 0x5EEDF00Dull;
        juce::int64 len = 48000;
        int block = 512;                                   // 0 = seeded variable (includes 1 and 8192)
        std::function<float (juce::int64, int)> stim = noiseStim();
        std::vector<Event> events;
        std::function<void (Proc&)> pre;                   // before prepareToPlay (hooks)
        std::function<void (Proc&, juce::int64)> after;   // after each block, with the next position
    };

    struct Out
    {
        std::vector<float> L, R;
        std::unique_ptr<Proc> p;
        ogran::Engine& eng() const { return p->getEngineForTest(); }
    };

    Out render (const Job& j)
    {
        using CS = juce::AudioChannelSet;
        Out o;
        o.p = make();
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (j.inCh == 1 ? CS::mono() : CS::stereo());
        l.outputBuses.add (j.outCh == 1 ? CS::mono() : CS::stereo());
        o.p->setBusesLayout (l);
        setAll (*o.p, j.params);
        o.p->setSeed (j.seed);
        if (j.pre) j.pre (*o.p);
        o.p->prepareToPlay (j.fs, j.prepBlock);

        const int numCh = juce::jmax (j.inCh, j.outCh);
        o.L.assign ((size_t) j.len, 0.0f);
        o.R.assign ((size_t) j.len, 0.0f);
        juce::AudioBuffer<float> blk (numCh, 8192);
        juce::MidiBuffer midi;
        juce::Random vr (0xB10CC5EEDull);
        std::vector<Event> ev = j.events;
        std::sort (ev.begin(), ev.end(), [] (const Event& a, const Event& b) { return a.at < b.at; });
        size_t ei = 0;
        int varStep = 0;

        juce::int64 pos = 0;
        while (pos < j.len)
        {
            while (ei < ev.size() && ev[ei].at <= pos) ev[ei++].fn (*o.p);

            int b = j.block;
            if (b == 0)
            {
                const int k = varStep++;
                b = k == 0 ? 1 : k == 1 ? 8192 : (k % 17 == 0 ? 8192 : (k % 11 == 0 ? 1 : 1 + vr.nextInt (2048)));
            }
            juce::int64 n = juce::jmin ((juce::int64) b, j.len - pos);
            if (ei < ev.size()) n = juce::jmin (n, ev[ei].at - pos);   // events land on a SHARED boundary
            const int ni = (int) n;

            blk.setSize (numCh, ni, false, false, true);
            for (int ch = 0; ch < numCh; ++ch)
                for (int i = 0; i < ni; ++i)
                    blk.setSample (ch, i, ch < j.inCh ? j.stim (pos + i, ch) : 0.0f);

           #if JUCE_MAC
            if (armDuringProcess) allocArmed = true;
           #endif
            o.p->processBlock (blk, midi);
           #if JUCE_MAC
            allocArmed = false;
           #endif

            const float* l0 = blk.getReadPointer (0);
            const float* r0 = blk.getReadPointer (j.outCh > 1 ? 1 : 0);
            std::copy (l0, l0 + ni, o.L.begin() + pos);
            std::copy (r0, r0 + ni, o.R.begin() + pos);
            pos += n;
            if (j.after) j.after (*o.p, pos);
        }
        return o;
    }

    struct Diff { int count = 0; double maxAbs = 0.0; juce::int64 first = -1; };
    Diff diff (const Out& a, const Out& b)
    {
        Diff d;
        for (size_t i = 0; i < a.L.size() && i < b.L.size(); ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = ch ? a.R[i] : a.L[i], y = ch ? b.R[i] : b.L[i];
                if (std::memcmp (&x, &y, sizeof (float)) != 0)
                {
                    if (d.count++ == 0) d.first = (juce::int64) i;
                    d.maxAbs = juce::jmax (d.maxAbs, std::abs ((double) x - (double) y));
                }
            }
        if (a.L.size() != b.L.size()) d.count += 1;
        return d;
    }

    double rmsDb (const std::vector<float>& x, size_t from, size_t to)
    {
        double acc = 0.0;
        for (size_t i = from; i < to && i < x.size(); ++i) acc += (double) x[i] * x[i];
        const double ms = acc / (double) juce::jmax ((size_t) 1, to - from);
        return ms > 0.0 ? 10.0 * std::log10 (ms) : -400.0;
    }

    //==========================================================================
    // FFT (O-GrainScatter alias-gate).
    void fft (std::vector<std::complex<double>>& a)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = -2.0 * kPi / static_cast<double> (len);
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    const auto u = a[i + k], v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
    }
    std::vector<double> powerSpectrum (const std::vector<float>& x, size_t from, size_t n)
    {
        std::vector<std::complex<double>> a (n);
        for (size_t i = 0; i < n; ++i)
        {
            const double t = static_cast<double> (i) / static_cast<double> (n);
            const double w = 0.35875 - 0.48829 * std::cos (2 * kPi * t) + 0.14128 * std::cos (4 * kPi * t)
                           - 0.01168 * std::cos (6 * kPi * t);
            a[i] = w * static_cast<double> (x[from + i]);
        }
        fft (a);
        std::vector<double> p (n / 2);
        for (size_t k = 0; k < n / 2; ++k) p[k] = std::norm (a[k]);
        return p;
    }
    struct Split { double wanted = 0.0, spurious = 0.0; };
    Split splitPower (const std::vector<double>& p, double fs, size_t n, double wantedHz,
                      double audibleHz = 20000.0, double guardHz = 400.0)
    {
        Split s;
        for (size_t k = 1; k < p.size(); ++k)
        {
            const double f = static_cast<double> (k) * fs / static_cast<double> (n);
            if (f >= audibleHz) break;
            if (wantedHz >= 0.0 && std::abs (f - wantedHz) <= guardHz) s.wanted += p[k];
            else s.spurious += p[k];
        }
        return s;
    }
    double dB10 (double p) { return p > 0.0 ? 10.0 * std::log10 (p) : -400.0; }

    // Worst second difference |x[n+1] − 2x[n] + x[n−1]| over [from, to).
    double maxCurv (const std::vector<float>& x, size_t from, size_t to)
    {
        double m = 0.0;
        for (size_t n = juce::jmax ((size_t) 1, from); n + 1 < to && n + 1 < x.size(); ++n)
            m = juce::jmax (m, std::abs ((double) x[n + 1] - 2.0 * (double) x[n] + (double) x[n - 1]));
        return m;
    }

    double floorFor (const ogran::Engine::SpawnRecord& r)
    {
        const double margin = (double) ogran::Ring::margin (r.level);
        return r.dir > 0 ? margin + juce::jmax (0.0, (r.rate - 1.0) * r.len) + 1.0 : margin + 1.0;
    }

    //==========================================================================
    // E0 — output law units
    void outputLawRows()
    {
        std::printf ("\n-- E0. output law (Hann closed form, single normalisation)\n");
        const double c1 = ogran::hannCumW2 (1.0);
        row (std::abs (c1 - 0.375) <= 1e-15, "E0 hannCumW2 (1) == 0.375 (1e-15)", juce::String (c1 - 0.375, 3));

        // Continuity at N = q: N = density · size / 1000.
        double worstJump = 0.0;
        for (int q : { 32, 64, 96, 128 })
        {
            const double dens = 100.0, sizeAt = (double) q / dens * 1000.0;   // ms → N == q
            const double a = ogran::effectiveOverlap (dens, sizeAt * (1.0 - 1e-9), 48000.0, q);
            const double b = ogran::effectiveOverlap (dens, sizeAt * (1.0 + 1e-9), 48000.0, q);
            const double at = ogran::effectiveOverlap (dens, sizeAt, 48000.0, q);
            worstJump = juce::jmax (worstJump, juce::jmax (std::abs (a - at), std::abs (b - at)) / q);
        }
        row (worstJump < 1e-6, "E0 continuity at N = q (32/64/96/128), rel jump < 1e-6", juce::String (worstJump, 3));

        const double m032 = ogran::hannMeanSquare (0.32);
        const double eo = ogran::effectiveOverlap (200.0, 2000.0, 48000.0, 128);
        const double dbVsNaive = 10.0 * std::log10 (eo / 128.0);
        row (std::abs (eo / 43.02 - 1.0) <= 0.005 && std::abs (m032 - 0.12604) < 5e-5,
             "E0 200 g/s x 2 s, q 128: P/mu2 = 43.02 (+-0.5 %)  [plan's ~61 is wrong]",
             juce::String::formatted ("M(0.32) %.5f  P/mu2 %.3f  %.2f dB vs naive 128", m032, eo, dbVsNaive));

        const float g1 = ogran::streamOutputGain (1.0, 1000.0, 48000.0, 128);    // N = 1
        const float gL = ogran::streamOutputGain (0.5, 5.0, 48000.0, 128);       // N << 1
        const float g4 = ogran::streamOutputGain (4.0, 1000.0, 48000.0, 128);    // N = 4
        row (juce::exactlyEqual (g1, 1.0f) && juce::exactlyEqual (gL, 1.0f) && juce::exactlyEqual (g4, 0.5f),
             "E0 g_out exactly 1.0f at N <= 1, 0.5f at N = 4", juce::String::formatted ("%.9g %.9g %.9g", g1, gL, g4));
    }

    //==========================================================================
    // E1 — FUNC-01 audible at position 0
    void funcRows()
    {
        std::printf ("\n-- E1-E4. FUNC-01 / FUNC-07\n");
        {
            const double fs = 48000.0;
            const P base { { "mix", 100.0f }, { "s1_position", 0.0f }, { "s1_spray", 0.0f } };
            Job j; j.params = base; j.len = 48000;
            const auto a = render (j);
            const auto& r0 = a.p->getEngineForTest().diag.record (0);
            const size_t win = (size_t) (fs / 10.0 + 0.1 * fs);   // interval (10 g/s) + len (100 ms)
            const double db = rmsDb (a.L, 0, win);
            Job c = j; c.params.push_back ({ "s1_position", 100.0f });
            const auto b = render (c);
            const double dbc = rmsDb (b.L, 0, win);
            const bool live = dbc < -40.0;
            row (db > -40.0 && live && r0.onsetSample == 0,
                 "E1 audible within interval + len at position 0 (ctrl pos 100 % silent)",
                 juce::String::formatted ("%.1f dBFS in %d smp, first onset @%lld; ctrl %.1f dBFS%s", db, (int) win,
                                          (long long) r0.onsetSample, dbc, live ? "" : "  VACUOUS"));
        }

        // E2 — buffer_length change
        {
            const double fs = 48000.0;
            // (a) + (b): everything randomised, the floor-wins case exercised.
            const juce::int64 c = (juce::int64) (3.0 * fs) + 7;
            Job j; j.len = (juce::int64) (6.0 * fs);
            j.params = { { "mix", 100.0f }, { "buffer_length", 30.0f }, { "s1_position", 100.0f }, { "s1_spray", 100.0f },
                         { "s1_size", 2000.0f }, { "s1_size_rand", 100.0f }, { "s1_density", 20.0f }, { "s1_jitter", 50.0f },
                         { "s1_pitch_rand", 24.0f }, { "s1_reverse_prob", 50.0f } };
            j.events.push_back ({ c, [] (Proc& p) { setP (p, "buffer_length", 1.0f); } });
            auto a = render (j);
            auto& dg = a.eng().diag;
            const juce::int64 grid = ((c + ogran::kCell - 1) / ogran::kCell) * ogran::kCell;
            int post = 0, bad = 0, floorWins = 0, preOver = 0, staleSpan = 0;
            double worstOver = -1e30;
            const uint64_t n0 = dg.logCount > (uint64_t) ogran::Engine::kSpawnLogSize ? dg.logCount - ogran::Engine::kSpawnLogSize : 0;
            for (uint64_t k = n0; k < dg.logCount; ++k)
            {
                const auto& r = dg.record (k);
                const double margin = (double) ogran::Ring::margin (r.level);
                if (r.onsetSample < grid)
                {
                    if (r.delay > margin + 1.0 * fs) ++preOver;
                    continue;
                }
                ++post;
                if (std::abs ((double) r.spanSamples - 1.0 * fs) > 1.0) ++staleSpan;
                const double bound = juce::jmax (margin + 1.0 * fs, floorFor (r));
                if (r.delay > bound + 1e-6) ++bad;
                if (floorFor (r) > margin + 1.0 * fs) ++floorWins;
                worstOver = juce::jmax (worstOver, r.delay - bound);
            }
            row (post > 0 && bad == 0 && staleSpan == 0 && preOver > 0,
                 "E2a post-change spawns: D <= max(margin + span_new, floor(r, len))",
                 juce::String::formatted ("%d post-change spawns, %d over, %d stale span, %d floor-wins; pre-change D > new span: %d; worst D-bound %.1f",
                                          post, bad, staleSpan, floorWins, preOver, worstOver));
            row (dg.reachViolations == 0, "E2b reach violations == 0", juce::String ((juce::int64) dg.reachViolations));
        }
        {
            // (c) step click: tonal, spray 0, jitter 0, vs the SAME motion without the change.
            const double fs = 48000.0;
            const juce::int64 c = (juce::int64) (5.0 * fs) + 11;
            Job ref; ref.len = (juce::int64) (7.5 * fs); ref.stim = sineStim (437.3, fs);   // NOT a whole number of cycles over the 3 s D change
            ref.params = { { "mix", 100.0f }, { "buffer_length", 8.0f }, { "s1_position", 50.0f }, { "s1_spray", 0.0f },
                           { "s1_jitter", 0.0f } };
            Job sub = ref;
            sub.events.push_back ({ c, [] (Proc& p) { setP (p, "buffer_length", 2.0f); } });
            const auto a = render (sub);
            const auto b = render (ref);
            const size_t from = (size_t) c, to = (size_t) (c + (juce::int64) (2.0 * fs));
            const double ca = maxCurv (a.L, from, to), cb = maxCurv (b.L, from, to);
            const double ratio = ca / juce::jmax (1e-12, cb);
            double changed = 0.0;
            for (size_t i = from; i < to; ++i) changed = juce::jmax (changed, std::abs ((double) a.L[i] - b.L[i]));
            // Positive control: a 0.05 step injected into the reference must read as a click.
            auto bs = b.L;
            for (size_t i = from + 24000; i < bs.size(); ++i) bs[i] += 0.05f;
            const double ctrl = maxCurv (bs, from, to) / juce::jmax (1e-12, cb);
            row (ratio <= 2.0 && changed > 1e-3 && ctrl > 2.0 && a.eng().diag.reachViolations == 0,
                 "E2c buffer_length 8 -> 2 s: step click <= 2.0x same-motion ref",
                 juce::String::formatted ("ratio %.3f (curv %.3g vs %.3g), subject differs by %.3f, ctrl step %.1fx", ratio, ca, cb, changed, ctrl));
        }

        // E3 — mix 100 % has no dry
        {
            const double fs = 48000.0;
            auto corr = [fs] (float mix)
            {
                Job j; j.len = (juce::int64) (9.0 * fs);
                j.params = { { "mix", mix }, { "s1_position", 50.0f } };
                const auto a = render (j);
                double sxy = 0, sxx = 0, syy = 0;
                for (juce::int64 i = (juce::int64) (5.0 * fs); i < j.len; ++i)
                {
                    const double x = a.L[(size_t) i], y = hashNoise (i, 0);
                    sxy += x * y; sxx += x * x; syy += y * y;
                }
                return sxy / std::sqrt (juce::jmax (1e-30, sxx * syy));
            };
            const double c100 = corr (100.0f), c50 = corr (50.0f);
            row (std::abs (c100) < 0.05 && c50 > 0.5, "E3 mix 100 %, position 4 s: |corr(out, dry)| < 0.05 (ctrl mix 50 % > 0.5)",
                 juce::String::formatted ("corr %.4f, ctrl %.3f", c100, c50));
        }

        // E4 — R2 regression
        {
            auto p = make();
            bool ok = true; juce::String det;
            for (auto* id : { "output_gain", "input_gain", "s1_level" })
            {
                auto* f = dynamic_cast<juce::AudioParameterFloat*> (param (*p, id));
                f->setValueNotifyingHost (static_cast<juce::AudioProcessorParameter*> (f)->getDefaultValue());
                const float db = f->get();
                const float naive = juce::Decibels::decibelsToGain (db);
                const float g = juce::String (id) == "s1_level" ? ogran::levelDbToGain (db) : ogran::dbToGain (db);
                ok = ok && juce::exactlyEqual (g, 1.0f);
                det << id << " " << juce::String::formatted ("%.3g dB naive %.9g snapped %.9g; ", db, naive, g);
            }
            auto* og = dynamic_cast<juce::AudioParameterFloat*> (param (*p, "output_gain"));
            const bool residue = ! juce::exactlyEqual (og->get(), 0.0f) && ! juce::exactlyEqual (juce::Decibels::decibelsToGain (og->get()), 1.0f);
            p->prepareToPlay (48000.0, 512);
            const bool latched = juce::exactlyEqual (ogran::dbToGain (p->getEngineForTest().getGridParams().outputGainDb), 1.0f);
            row (ok && residue && latched, "E4 gains exactly 1.0f after normalised round trip (residue present)", det);
        }
    }

    //==========================================================================
    // A1 — DSP-02 alias
    void aliasRows()
    {
        std::printf ("\n-- A. DSP-02 alias (processor end-to-end, 44.1 kHz, +24 st)\n");
        const double fs = 44100.0;
        constexpr size_t kN = 1 << 16;
        const juce::int64 len = (juce::int64) (fs * 4.0);
        const P ps { { "mix", 100.0f }, { "s1_pitch", 24.0f }, { "s1_pitch_rand", 0.0f }, { "s1_position", 10.0f },
                     { "s1_size", 100.0f }, { "s1_density", 50.0f } };

        auto sweep = [fs, len] (double f0, double f1)
        {
            auto v = std::make_shared<std::vector<float>> ((size_t) len);
            const juce::int64 period = juce::int64 (1) << 15;
            double phase = 0.0;
            for (juce::int64 i = 0; i < len; ++i)
            {
                const double u = (double) (i % period) / (double) period;
                const double tri = u < 0.5 ? 2.0 * u : 2.0 - 2.0 * u;
                (*v)[(size_t) i] = (float) (0.5 * std::sin (phase));
                phase += 2.0 * kPi * f0 * std::pow (f1 / f0, tri) / fs;
                if (phase > 2.0 * kPi) phase -= 2.0 * kPi;
            }
            return std::function<float (juce::int64, int)> ([v] (juce::int64 i, int) { return (*v)[(size_t) i]; });
        };

        auto measure = [&] (bool level0, juce::String& det)
        {
            auto run = [&] (std::function<float (juce::int64, int)> stim)
            {
                Job j; j.fs = fs; j.len = len; j.params = ps; j.stim = std::move (stim); j.seed = 0xA11A5ull;
                j.pre = [level0] (Proc& p) { p.getEngineForTest().hooks.forceLevel0 = level0; };
                auto o = render (j);
                return o.L;
            };
            const auto yr = run (sineStim (250.0, fs));
            const double ref = splitPower (powerSpectrum (yr, yr.size() - kN, kN), fs, kN, 1000.0).wanted;
            double worst = -400.0, worstHz = 0.0;
            for (double hz : { 6000.0, 8000.0, 10000.0, 12500.0, 15000.0, 17500.0, 19500.0 })
            {
                const auto y = run (sineStim (hz, fs));
                const double d = dB10 (splitPower (powerSpectrum (y, y.size() - kN, kN), fs, kN, -1.0).spurious / ref);
                if (d > worst) { worst = d; worstHz = hz; }
            }
            const double nyqIn = fs * 0.5 / 4.0;
            const auto yo = run (sweep (nyqIn * 1.02, 20000.0));
            const auto yi = run (sweep (50.0, 0.85 * 20000.0 / 4.0));
            const double sw = dB10 (splitPower (powerSpectrum (yo, yo.size() - kN, kN), fs, kN, -1.0).spurious
                                    / splitPower (powerSpectrum (yi, yi.size() - kN, kN), fs, kN, -1.0).spurious);
            det = juce::String::formatted ("tones worst %.1f dB (in %.0f Hz); sweep %.1f dB; ref %.1f dB", worst, worstHz, sw, dB10 (ref));
            return std::make_pair (juce::jmax (worst, sw), ref);
        };

        juce::String d1, d0;
        const auto r1 = measure (false, d1);
        row (r1.second > 1e-6 && r1.first <= -60.0, "A1 +24 st: out-of-band tones + sweep <= -60 dB", d1);
        const auto r0 = measure (true, d0);
        row (r0.first > -60.0, "A1n negative control forceLevel0 FAILS the -60 dB threshold", d0 + (r0.first > -60.0 ? "" : "  VACUOUS"));
    }

    //==========================================================================
    // L — DSP-01 long buffer
    void longBufferRows()
    {
        std::printf ("\n-- L. DSP-01 long buffer (periodic corpus, double reference)\n");
        const double fs = 48000.0;
        constexpr int kPer = 160;   // 300 Hz
        auto stim = [] (juce::int64 i, int)
        {
            const double ph = 2.0 * kPi * (double) (i % kPer) / kPer;
            return (float) (0.4 * std::sin (ph) + 0.25 * std::sin (3.0 * ph + 0.3) + 0.15 * std::sin (7.0 * ph + 1.1));
        };

        // Onset interval must be an EXACT multiple of the period, so the density has
        // to land exactly through the (skewed) host-normalised path: pick the first
        // candidate 300/m (interval 160·m) that does.
        float dens = 0.0f;
        {
            auto p = make();
            for (float d : { 2.0f, 2.5f, 3.125f, 4.0f, 1.875f, 3.75f, 1.5625f })
                if (setP (*p, "s1_density", d)) { dens = d; break; }
        }
        const juce::int64 ivl = dens > 0.0f ? (juce::int64) std::llround (fs / (double) dens) : 24000;
        row (dens > 0.0f && (ivl % kPer) == 0, "L0 density lands exactly on a period multiple (host-normalised)",
             juce::String::formatted ("density %.4f g/s, interval %lld = %lld periods", dens, (long long) ivl, (long long) (ivl / kPer)));
        auto firstAt = [ivl] (double secs) { return (juce::int64) std::ceil (secs * 48000.0 / (double) ivl); };

        struct Rate { const char* name; float st; };
        const Rate rates[] = { { "0.5", -12.0f }, { "~1.5", 7.0195500087f }, { "4", 24.0f } };

        for (const auto& rt : rates)
        {
            for (int variant = 0; variant < 2; ++variant)
            {
                // variant 0: D ~ 0.5 s, grain at 1 s vs grain at 26 s (readAbs ~1.25 M)
                // variant 1: D ~ 25.5 s (buffer 30 s, position 85 %), grains at 26 s vs 28 s
                Job j; j.stim = stim; j.block = 480;
                // PLAN-2.2 T11: stereo_spread defaults to 30 % — the null compares two
                // grains sample for sample, so they need identical (unity) grain gains.
                j.params = { { "mix", 100.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 0.0f }, { "s1_density", dens },
                             { "s1_size", 200.0f }, { "s1_pitch", rt.st }, { "s1_stereo_spread", 0.0f },
                             { "buffer_length", variant == 0 ? 1.0f : 30.0f }, { "s1_position", variant == 0 ? 50.0f : 85.0f } };
                const juce::int64 kA = firstAt (variant == 0 ? 1.0 : 26.0), kB = firstAt (variant == 0 ? 26.0 : 28.0);
                j.len = kB * ivl + ivl;
                const juce::int64 kC = firstAt (25.0);
                const juce::int64 checkAt = kC * ivl + 4800;   // >= 25.1 s: that grain at age 4800
                double refErr = -1.0; int checked = 0;
                j.after = [&] (Proc& p, juce::int64 pos)
                {
                    if (pos != checkAt) return;
                    auto& e = p.getEngineForTest();
                    const auto& pool = e.getPool();
                    for (int s = 0; s < ogran::kPoolLive; ++s)
                    {
                        if (! pool.isActive (s)) continue;
                        const auto& g = pool.getGrains()[(size_t) s];
                        for (uint64_t k = e.diag.logCount; k-- > 0;)
                        {
                            const auto& r = e.diag.record (k);
                            if (r.slot != s) continue;
                            const double ref = r.anchor + (double) r.dir * r.rate * ((double) g.age + r.frac);
                            refErr = juce::jmax (refErr, std::abs (g.readAbs - ref));
                            ++checked;
                            break;
                        }
                    }
                };
                const bool exact = dens > 0.0f;
                auto o = render (j);
                auto& dg = o.eng().diag;
                const ogran::Engine::SpawnRecord* ra = nullptr; const ogran::Engine::SpawnRecord* rb = nullptr;
                for (uint64_t k = 0; k < dg.logCount; ++k)
                {
                    const auto& r = dg.record (k);
                    if (r.onsetSample == kA * ivl) ra = &r;
                    if (r.onsetSample == kB * ivl) rb = &r;
                }
                double mx = 0.0, peak = 0.0;
                if (ra != nullptr && rb != nullptr)
                {
                    const size_t n = (size_t) std::ceil (ra->len) + 2;
                    for (size_t i = 0; i < n; ++i)
                    {
                        const double x = o.L[(size_t) ra->onsetSample + i], y = o.L[(size_t) rb->onsetSample + i];
                        mx = juce::jmax (mx, std::abs (x - y));
                        peak = juce::jmax (peak, std::abs (x));
                    }
                }
                const bool sameD = ra != nullptr && rb != nullptr && juce::exactlyEqual (ra->delay, rb->delay)
                                   && juce::exactlyEqual (ra->frac, 0.0) && juce::exactlyEqual (rb->frac, 0.0);
                row (exact && sameD && peak > 0.1 && mx <= 1e-5 && dg.reachViolations == 0,
                     juce::String ("L1 periodic null, rate ") + rt.name + (variant == 0 ? ", grain @1 s vs @26 s" : ", D 25.5 s: @26 s vs @28 s"),
                     juce::String::formatted ("max|d| %.3g (%.1f dBFS), peak %.2f, D %.1f s", mx, dB10 (mx * mx), peak,
                                              ra ? ra->delay / fs : -1.0));
                if (variant == 0)
                {
                    // Model control: a float accumulator over the same span would miss by this much.
                    float fa = (float) ((double) (kC * ivl + 1) - (ra ? ra->delay : 0.0)); const float fstep = (float) std::exp2 (rt.st / 12.0);
                    for (int k = 0; k < 4800; ++k) fa += fstep;
                    const double fref = ((double) (kC * ivl + 1) - (ra ? ra->delay : 0.0)) + 4800.0 * std::exp2 ((double) rt.st / 12.0);
                    row (checked > 0 && refErr >= 0.0 && refErr <= 1e-6,
                         juce::String ("L2 double reference at >= 25.1 s, rate ") + rt.name + ": |readAbs - ref| <= 1e-6",
                         juce::String::formatted ("%d grains, max err %.3g smp (float-accumulator model: %.3g)", checked, refErr, std::abs ((double) fa - fref)));
                }
            }
        }
    }

    //==========================================================================
    // N — PERF-03 block-size null, R1 negative control, R4, oversize
    void nullRows()
    {
        std::printf ("\n-- N. PERF-03 block-size null (+ R1 / R4 / oversize)\n");
        const double fs = 48000.0;

        Job base;
        base.len = (juce::int64) (6.0 * fs);
        base.prepBlock = 512;
        base.params = { { "mix", 70.0f }, { "buffer_length", 4.0f }, { "s1_position", 30.0f }, { "s1_spray", 50.0f },
                        { "s1_size", 500.0f }, { "s1_size_rand", 50.0f }, { "s1_density", 100.0f }, { "s1_jitter", 50.0f },
                        { "s1_pitch_rand", 12.0f }, { "s1_reverse_prob", 50.0f }, { "grain_limit", 0.0f } /* 32 */,
                        { "s1_pan", -30.0f } };
        base.events.push_back ({ 100003, [] (Proc& p)
        {
            setP (p, "s1_pitch", 5.0f); setP (p, "mix", 40.0f); setP (p, "buffer_length", 2.0f);
            setP (p, "output_gain", -3.0f); setP (p, "s1_pan", 40.0f); setP (p, "s1_level", -4.0f);
        } });
        base.events.push_back ({ 150011, [] (Proc& p) { setP (p, "s1_enable", 0.0f); } });
        base.events.push_back ({ 170017, [] (Proc& p) { setP (p, "s1_enable", 1.0f); } });

        Job j32 = base; j32.block = 32;
        const auto ref = render (j32);
        const auto& rd = ref.p->getEngineForTest().diag;
        const bool live = rd.kills > 0 && rd.spawns > 200 && ref.p->getEngineForTest().getEpoch (0) > 0;
        Tally t;
        juce::String worst;
        for (int b : { 441, 512, 4096, 0 })
        {
            Job jb = base; jb.block = b;
            const auto o = render (jb);
            const auto d = diff (ref, o);
            const bool ok = d.count == 0 && o.p->getEngineForTest().diag.spawns == rd.spawns
                            && o.p->getEngineForTest().diag.reachViolations == 0;
            t.check (ok, (b == 0 ? juce::String ("var") : juce::String (b)) + juce::String::formatted (": %d diffs max %.3g @%lld", d.count, d.maxAbs, (long long) d.first));
        }
        row (t.bad == 0 && live && rd.reachViolations == 0, "N1 block null 441/512/4096/var(1,8192) vs 32: max|d| == 0",
             t.detail (4) + juce::String::formatted ("  [spawns %lld, kills %lld, steals %lld, epoch %llu]", (long long) rd.spawns,
                                                     (long long) rd.kills, (long long) rd.steals,
                                                     (unsigned long long) ref.p->getEngineForTest().getEpoch (0)));

        {
            Job js = j32; js.seed = base.seed ^ 0x1ull;
            const auto o = render (js);
            const auto d = diff (ref, o);
            row (d.count > 0, "N2 different-seed control differs", juce::String (d.count) + " diffs");
        }

        // N3 — R1 (20 s, 128 grains, 441 vs 32).
        //
        // Pre-R1, (double) W − readAbs reconstructs a different (t, phi) for two
        // heads W only when the two differences STRADDLE a power of two (same
        // binade: RN (x + k) == RN (x) + k), and Sterbenz makes it exact once
        // readAbs >= W/2. Even then the kernel weight moves ~1e-10 — usually
        // absorbed by the 16-tap float sum. So the stimulus must PARK the read
        // delay just under 2^19 (memory: stimulus below threshold is vacuous):
        // rate 1 grains whose delay sits in [2^19 − 32, 2^19 − 1) straddle on
        // every host-cut piece, for t in (10.9, 20) s (real audio, non-Sterbenz).
        // `hazard reads` (test-hook counter: level-0 (t, row, u) differs between
        // the host-cut head and the grid head) proves the stimulus reached it.
        {
            auto runCfg = [&] (const P& params, int b, bool noQuant)
            {
                Job k; k.len = (juce::int64) (20.0 * fs); k.block = b; k.params = params;
                k.pre = [noQuant, b] (Proc& p)
                {
                    p.getEngineForTest().hooks.disableFracQuant = noQuant;
                    p.getEngineForTest().hooks.countR1Hazards   = b != 32;   // liveness counter (does not touch audio)
                };
                return render (k);
            };
            // D = margin 9 + pos·span + spray·span·(u − ½) ≈ 2^19 − 15 ± 7.
            const P parked { { "mix", 100.0f }, { "buffer_length", 30.0f }, { "s1_position", 36.4072222f }, { "s1_spray", 0.001f },
                             { "s1_size", 2000.0f }, { "s1_density", 64.0f }, { "s1_jitter", 0.0f }, { "grain_limit", 3.0f } };
            const P random { { "mix", 100.0f }, { "buffer_length", 30.0f }, { "s1_position", 36.41f }, { "s1_spray", 2.0f },
                             { "s1_size", 2000.0f }, { "s1_density", 64.0f }, { "s1_jitter", 60.0f }, { "s1_pitch_rand", 12.0f },
                             { "s1_reverse_prob", 30.0f }, { "grain_limit", 3.0f } };
            const auto off32 = runCfg (parked, 32, true), off441 = runCfg (parked, 441, true);
            const auto on32 = runCfg (parked, 32, false), on441 = runCfg (parked, 441, false);
            const auto dOff = diff (off32, off441), dOn = diff (on32, on441);
            const auto hz = on441.p->getEngineForTest().diag.r1Hazards;
            row (dOn.count == 0 && hz > 0, "N3 R1 on: 20 s / 128 grains (parked on 2^19), 441 vs 32 bit-identical",
                 juce::String::formatted ("%d diffs over %lld hazard reads; maxAlive %lld", dOn.count, (long long) hz,
                                          (long long) on32.p->getEngineForTest().diag.maxAlive));
            row (dOff.count >= 1, "N3n R1 negative control (disableFracQuant): >= 1 diff",
                 juce::String::formatted ("%d diffs, max %.3g, first @%lld (%.2f s); %lld hazard reads%s", dOff.count, dOff.maxAbs,
                                          (long long) dOff.first, (double) dOff.first / fs,
                                          (long long) off441.p->getEngineForTest().diag.r1Hazards,
                                          dOff.count >= 1 ? "" : "  VACUOUS GATE"));
            const auto rOn32 = runCfg (random, 32, false), rOn441 = runCfg (random, 441, false);
            const auto dR = diff (rOn32, rOn441);
            row (dR.count == 0, "N3 R1 on: 20 s / 128 grains, all randomisation, 441 vs 32 bit-identical",
                 juce::String::formatted ("%d diffs over %lld hazard reads", dR.count, (long long) rOn441.p->getEngineForTest().diag.r1Hazards));
        }

        // N4 — R4 epoch across re-enable.
        {
            Job r4; r4.len = (juce::int64) (2.0 * fs); r4.block = 512;
            r4.params = { { "mix", 100.0f }, { "s1_spray", 100.0f }, { "s1_density", 50.0f }, { "s1_pitch_rand", 7.0f },
                          { "s1_reverse_prob", 50.0f } };
            const juce::int64 offAt = 24000, onAt = 48000;
            r4.events.push_back ({ offAt, [] (Proc& p) { setP (p, "s1_enable", 0.0f); } });
            r4.events.push_back ({ onAt, [] (Proc& p) { setP (p, "s1_enable", 1.0f); } });
            const auto o = render (r4);
            const auto& dg = o.p->getEngineForTest().diag;
            std::vector<double> b1, b2; uint64_t firstIdx2 = 0; juce::int64 firstOn2 = -1;
            for (uint64_t k = 0; k < dg.logCount; ++k)
            {
                const auto& r = dg.record (k);
                if (r.onsetSample < offAt) { if (b1.size() < 8) b1.push_back (r.delay); }
                else if (r.onsetSample >= onAt && b2.size() < 8)
                {
                    if (b2.empty()) { firstIdx2 = r.globalIndex; firstOn2 = r.onsetSample; }
                    b2.push_back (r.delay);
                }
            }
            int same = 0;
            for (size_t i = 0; i < juce::jmin (b1.size(), b2.size()); ++i) same += juce::exactlyEqual (b1[i], b2[i]) ? 1 : 0;
            row (b1.size() == 8 && b2.size() == 8 && same == 0 && firstIdx2 > 0 && firstIdx2 == o.p->getEngineForTest().getEpoch (0),
                 "N4 R4: second-burst D set != first; counter continues (epoch)",
                 juce::String::formatted ("%d/8 equal; burst-2 first index %llu @%lld (first grid after on = %lld)", same,
                                          (unsigned long long) firstIdx2, (long long) firstOn2, (long long) (((onAt + 31) / 32) * 32)));
        }

        // O1 — oversize host blocks: prepare 512, then 8192 and 4096 at position 0.
        {
            auto p = make();
            setAll (*p, { { "mix", 100.0f }, { "s1_position", 0.0f }, { "s1_spray", 0.0f }, { "s1_density", 200.0f },
                          { "s1_size", 5.0f }, { "s1_pitch", 24.0f } });
            p->setSeed (0x0B5ull);
            p->prepareToPlay (fs, 512);
            constexpr int kGuard = 256;
            constexpr float kPoison = -12345.678f;
            std::vector<float> mem[2];
            for (auto& m : mem) m.assign (8192 + 2 * kGuard, kPoison);
            bool finite = true, hostGuards = true;
            juce::MidiBuffer midi;
            juce::int64 pos = 0;
            for (int k = 0; k < 24; ++k)
            {
                const int n = (k % 2 == 0) ? 8192 : 4096;
                float* chans[2] = { mem[0].data() + kGuard, mem[1].data() + kGuard };
                for (int ch = 0; ch < 2; ++ch)
                {
                    std::fill (mem[ch].begin(), mem[ch].end(), kPoison);
                    for (int i = 0; i < n; ++i) chans[ch][i] = hashNoise (pos + i, ch);
                }
                juce::AudioBuffer<float> buf (chans, 2, n);
                p->processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch)
                {
                    for (int i = 0; i < n; ++i) finite = finite && std::isfinite (chans[ch][i]);
                    for (int i = 0; i < kGuard; ++i)
                        hostGuards = hostGuards && juce::exactlyEqual (mem[ch][(size_t) i], kPoison);
                    for (int i = kGuard + n; i < (int) mem[ch].size(); ++i)   // everything past the block
                        hostGuards = hostGuards && juce::exactlyEqual (mem[ch][(size_t) i], kPoison);
                }
                pos += n;
            }
            auto& e = p->getEngineForTest();
            row (finite && hostGuards && e.guardsIntact() && e.diag.reachViolations == 0 && e.diag.spawns > 500,   // 3.07 s × 200 g/s ≈ 614
                 "O1 oversize: prepare 512, blocks 8192/4096 at position 0",
                 juce::String::formatted ("finite %d, host guards %d, engine canaries %d, reach viol %lld, spawns %lld",
                                          (int) finite, (int) hostGuards, (int) e.guardsIntact(), (long long) e.diag.reachViolations,
                                          (long long) e.diag.spawns));
        }
    }

    //==========================================================================
    // M1 — PERF-01 allocations; R3 — grain_limit cap
    void allocAndCapRows()
    {
        std::printf ("\n-- M / R3. PERF-01 allocations, PERF-02 grain_limit cap\n");
        const double fs = 48000.0;
       #if JUCE_MAC
        {
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

            Job j; j.len = (juce::int64) (12.0 * fs); j.block = 512;
            j.params = { { "mix", 60.0f }, { "s1_spray", 50.0f }, { "s1_size", 1000.0f }, { "s1_size_rand", 40.0f },
                         { "s1_density", 200.0f }, { "s1_jitter", 50.0f }, { "s1_pitch_rand", 24.0f },
                         { "s1_reverse_prob", 50.0f }, { "grain_limit", 3.0f } };
            // 0-3 s: native steals (cap 128, N 200); 3-6 s: R3 kills (cap 32);
            // enable toggles; buffer_length sweep every 100 ms; 9 s: cap 64.
            j.events.push_back ({ (juce::int64) (3.0 * fs), [] (Proc& p) { setP (p, "grain_limit", 0.0f); } });
            for (int k = 0; k < 6; ++k)
                j.events.push_back ({ (juce::int64) ((6.0 + 0.5 * k) * fs) + 13, [k] (Proc& p) { setP (p, "s1_enable", (k % 2) ? 1.0f : 0.0f); } });
            for (int k = 1; k < 120; ++k)
                j.events.push_back ({ (juce::int64) (0.1 * k * fs) + 5, [k] (Proc& p) { setP (p, "buffer_length", 1.0f + (float) ((k * 7) % 30)); } });
            j.events.push_back ({ (juce::int64) (9.0 * fs), [] (Proc& p) { setP (p, "grain_limit", 1.0f); } });

            // Warm-up is the first block (unarmed); arm from the second.
            armDuringProcess = false;
            j.after = [] (Proc&, juce::int64) { armDuringProcess = true; };
            const auto o = render (j);
            armDuringProcess = false;
            malloc_logger = nullptr;
            const auto& dg = o.p->getEngineForTest().diag;
            const int n = allocCount;
            row (liveness == 1, "M1 liveness: a deliberate malloc on the render thread counts exactly 1", juce::String (liveness));
            row (liveness == 1 && n == 0 && dg.steals > 0 && dg.kills > 0 && o.p->getEngineForTest().getEpoch (0) > 0,
                 "M1 0 allocations: spawn / steal / R3 kill / toggles / buffer_length sweeps",
                 juce::String::formatted ("%d allocs; spawns %lld steals %lld kills %lld", n, (long long) dg.spawns,
                                          (long long) dg.steals, (long long) dg.kills));
        }
       #else
        row (false, "M1 alloc gate is macOS-only");
       #endif

        for (int cap : { 32, 64 })
        {
            Job j; j.len = (juce::int64) (3.0 * fs); j.block = 32;
            j.params = { { "mix", 100.0f }, { "s1_density", 200.0f }, { "s1_size", 1000.0f }, { "s1_spray", 30.0f },
                         { "grain_limit", cap == 32 ? 0.0f : 1.0f } };
            int scanMax = 0;
            j.after = [&scanMax] (Proc& p, juce::int64)
            {
                const auto& pool = p.getEngineForTest().getPool();
                int alive = 0;
                for (int s = 0; s < ogran::kPoolLive; ++s)
                    if (pool.isActive (s) && ! (pool.getGrains()[(size_t) s].killStep > 0.0f)) ++alive;
                scanMax = juce::jmax (scanMax, alive);
            };
            const auto o = render (j);
            const auto& dg = o.p->getEngineForTest().diag;
            row (dg.maxAlive <= cap && scanMax <= cap && dg.kills > 0 && dg.maxAlive == cap,
                 "R3 grain_limit " + juce::String (cap) + ", density 200, size 1 s: maxAlive <= cap, kills > 0",
                 juce::String::formatted ("engine maxAlive %lld, harness scan %d, kills %lld, steals %lld", (long long) dg.maxAlive,
                                          scanMax, (long long) dg.kills, (long long) dg.steals));
        }
    }

    //==========================================================================
    // S0 — steal-free bit-identity vs the native pool (PLAN-2.2 P9/T1)
    void stealFreeRows()
    {
        std::printf ("\n-- S. PERF-02 steal-free bit-identity vs native pool\n");
        const double fs = 48000.0;
        auto job = [fs] (float density, float sizeMs, float limitIdx, bool native)
        {
            Job j; j.len = (juce::int64) (4.0 * fs); j.block = 512;
            j.params = { { "mix", 100.0f }, { "s1_density", density }, { "s1_size", sizeMs }, { "s1_jitter", 20.0f },
                         { "grain_limit", limitIdx } };
            j.pre = [native] (Proc& p) { p.getEngineForTest().hooks.nativeAcquire = native; };
            return j;
        };
        auto stats = [] (const Out& o)
        {
            const auto& d = o.eng().diag;
            return juce::String::formatted ("spawns %lld steals %lld kills %lld maxAlive %lld", (long long) d.spawns,
                                            (long long) d.steals, (long long) d.kills, (long long) d.maxAlive);
        };

        {
            const auto e = render (job (100.0f, 1000.0f, 3.0f /* 128 */, false));
            const auto n = render (job (100.0f, 1000.0f, 3.0f, true));
            const auto& de = e.eng().diag; const auto& dn = n.eng().diag;
            const auto d = diff (e, n);
            const double lvl = rmsDb (e.L, (size_t) fs, e.L.size());
            const bool live = de.spawns > 300 && de.spawns == dn.spawns && lvl > -40.0;
            const bool stealFree = de.steals == 0 && de.kills == 0 && dn.steals == 0 && dn.kills == 0;
            row (live && stealFree && d.count == 0,
                 "S0 steal-free (cap 128, N 100, jitter 20 %, 4 s): engine vs native max|d| == 0",
                 juce::String::formatted ("%d diffs max %.3g; ", d.count, d.maxAbs) + "engine " + stats (e) + " | native " + stats (n)
                     + juce::String::formatted ("; wet %.1f dB", lvl) + (stealFree && live ? "" : "  VACUOUS"));
        }
        {
            const auto e = render (job (150.0f, 2000.0f, 1.0f /* 64 */, false));
            const auto n = render (job (150.0f, 2000.0f, 1.0f, true));
            const auto d = diff (e, n);
            const bool engaged = e.eng().diag.kills > 0 && n.eng().diag.kills == 0 && n.eng().diag.steals > 0;
            row (d.count > 0 && engaged, "S0n negative control (N 300, cap 64): engine vs native DIFFER",
                 juce::String::formatted ("%d diffs max %.3g; ", d.count, d.maxAbs) + "engine " + stats (e) + " | native " + stats (n));

            const auto e128 = render (job (150.0f, 2000.0f, 3.0f, false));
            const auto n128 = render (job (150.0f, 2000.0f, 3.0f, true));
            const auto d128 = diff (e128, n128);
            std::printf ("  [INFO] S0n' N 300 at cap 128: %d diffs max %.3g (identical by construction on 2.1); engine %s | native %s\n",
                         d128.count, d128.maxAbs, stats (e128).toRawUTF8(), stats (n128).toRawUTF8());
        }
    }

    //==========================================================================
    // C1 — CPU baseline (non-blocking)
    void cpuBaseline()
    {
        std::printf ("\n-- C. CPU baseline (non-blocking, recorded)\n");
        const double fs = 48000.0;
        auto timeIt = [fs] (bool grains, long long& alive)
        {
            auto p = make();
            setAll (*p, { { "mix", 100.0f }, { "s1_density", 64.0f }, { "s1_size", 2000.0f }, { "grain_limit", 3.0f },
                          { "s1_enable", grains ? 1.0f : 0.0f } });
            p->setSeed (0xC0FFEEull);
            p->prepareToPlay (fs, 128);
            juce::AudioBuffer<float> buf (2, 128);
            juce::MidiBuffer midi;
            juce::int64 pos = 0;
            auto runFor = [&] (double secs)
            {
                const juce::int64 end = pos + (juce::int64) (secs * fs);
                for (; pos < end; pos += 128)
                {
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < 128; ++i) buf.setSample (ch, i, hashNoise (pos + i, ch));
                    p->processBlock (buf, midi);
                }
            };
            runFor (4.0);   // fill to steady state (N = 128)
            int a = 0;
            for (int s = 0; s < ogran::kPoolLive; ++s) a += p->getEngineForTest().getPool().isActive (s) ? 1 : 0;
            alive = a;
            const auto t0 = std::chrono::steady_clock::now();
            runFor (10.0);
            const auto t1 = std::chrono::steady_clock::now();
            return std::chrono::duration<double> (t1 - t0).count() / 10.0;   // fraction of one core
        };
        long long aOn = 0, aOff = 0;
        const double on = timeIt (true, aOn), off = timeIt (false, aOff);
        std::printf ("  [INFO] C1 128 grains Hann, no FX, 48 k / 128: %.2f %% of one core; engine-bypass (s1 off) %.3f %%; ratio %.1fx  (live grains %lld)\n",
                     on * 100.0, off * 100.0, on / juce::jmax (1e-12, off), aOn);
        std::printf ("         (input fill + harness overhead included in both; datum: 128 isolated reads = 6.5 %%)\n");
    }
} } // namespace eg (anonymous)

static void engineRows()
{
    std::printf ("\n== E. Stage 2 / Phase 2.1 engine rows\n");
    eg::outputLawRows();
    eg::funcRows();
    eg::aliasRows();
    eg::longBufferRows();
    eg::nullRows();
    eg::allocAndCapRows();
    eg::stealFreeRows();
    eg::cpuBaseline();
}
