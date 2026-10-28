/*
   This file is part of the Ouaricon grain-engine module.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
  ==============================================================================

    grain-engine tests. Exit 0 iff every check passes.

      W*  WindowLut moments — μ₁/μ₂ checked numerically, three independent ways:
          (1) a double-precision rebuild of each discrete table, (2) 2^20-step
          Simpson quadrature of each window's CONTINUOUS definition, and
          (3) the closed forms where they exist.
      R*  CounterRng — golden values from the plugin copies; Draw equivalence.
      S*  GrainScheduler — fractional grid exactness, block-size invariance,
          drop-without-shift, jitter mean, reschedule.
      P*  GrainPool — steal-free bitwise unity, steal fade continuity, victim
          choice, spare reuse, refuse policy.
      B*  BandlimitedReader (1.1.0) — unity DC per phase, level time alignment,
          half-band and sinc stopbands (driven with tones), block-split
          invariance, alloc-free audio path, snapshot == live reads, read guard.

    Build: see tests/CMakeLists.txt.

  ==============================================================================
*/

#include <juce_core/juce_core.h>

// Exact float equality is the point of several checks here (bitwise unity
// gains, canonicalised halves, golden values) — not an accident.
#if defined (__clang__)
 #pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined (__GNUC__)
 #pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "BandlimitedReader.h"
#include "CounterRng.h"
#include "GrainPool.h"
#include "GrainScheduler.h"
#include "WindowLut.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <new>
#include <random>
#include <string>
#include <vector>

// Counting global operator new (B5: the reader's audio path must not allocate).
static std::atomic<long> gAllocCount { 0 };
void* operator new (std::size_t n)
{
    gAllocCount.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t n) { return operator new (n); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

using namespace ouaricon::grain;

namespace
{
int failures = 0, passes = 0;

void check (bool ok, const char* id, const char* what, const std::string& detail = {})
{
    (ok ? passes : failures) += 1;
    std::printf ("%s  %-4s %s%s%s\n", ok ? "PASS" : "FAIL", id, what,
                 detail.empty() ? "" : "  — ", detail.c_str());
}

std::string fmt (const char* f, double a, double b = 0.0, double c = 0.0)
{
    char buf[256];
    std::snprintf (buf, sizeof buf, f, a, b, c);
    return buf;
}

//==============================================================================
// Continuous window definitions, in double, transcribed from the header's
// documentation of each shape — NOT from WindowLut::build().
constexpr double kPi = 3.14159265358979323846;

double hannC (double x)       { return 0.5 * (1.0 - std::cos (2.0 * kPi * x)); }
double triC (double x)        { return 1.0 - std::abs (2.0 * x - 1.0); }

double tukeyC (double x, double alpha)
{
    const double te = alpha * 0.5;
    const double u  = std::min (x, 1.0 - x);
    return u < te ? 0.5 * (1.0 - std::cos (kPi * u / te)) : 1.0;
}

double gaussC (double x)
{
    const double s = 0.18, edge = std::exp (-0.5 * (0.5 / s) * (0.5 / s));
    const double d = (x - 0.5) / s;
    return std::max (0.0, (std::exp (-0.5 * d * d) - edge) / (1.0 - edge));
}

double expoC (double x)
{
    const double att = 0.02, k = 5.0, end = std::exp (-k);
    if (x < att) return 0.5 * (1.0 - std::cos (kPi * x / att));
    return std::max (0.0, (std::exp (-k * (x - att) / (1.0 - att)) - end) / (1.0 - end));
}

// Composite Simpson over [a, b] with n (even) intervals.
double simpson (const std::function<double (double)>& f, double a, double b, int n)
{
    const double h = (b - a) / n;
    double s = f (a) + f (b);
    for (int i = 1; i < n; ++i)
        s += f (a + i * h) * ((i & 1) ? 4.0 : 2.0);
    return s * h / 3.0;
}

struct Cont { double mu1, mu2, mu1Lo, mu1Hi, mu2Lo, mu2Hi; };

Cont continuousMoments (const std::function<double (double)>& w)
{
    constexpr int n = 1 << 20;
    // Halves are integrated separately so the kinks at 0.5 (Triangular) and at
    // the taper ends never sit inside a Simpson panel's interior by accident.
    const double l1 = simpson (w, 0.0, 0.5, n) * 2.0;
    const double h1 = simpson (w, 0.5, 1.0, n) * 2.0;
    const double l2 = simpson ([&] (double x) { const double v = w (x); return v * v; }, 0.0, 0.5, n) * 2.0;
    const double h2 = simpson ([&] (double x) { const double v = w (x); return v * v; }, 0.5, 1.0, n) * 2.0;
    return { 0.5 * (l1 + h1), 0.5 * (l2 + h2), l1, h1, l2, h2 };
}

//==============================================================================
void testWindowLut()
{
    const WindowLut lut { 2048 };
    constexpr int N = 2048;

    const std::function<double (double)> defs[] = {
        hannC, [] (double x) { return tukeyC (x, 0.5); }, gaussC, triC, expoC
    };
    const char* names[] = { "hann", "tukey", "gaussian", "triangular", "expoDecay" };

    // W1 — (1) discrete rebuild in double. Same grid (i / (N-1)), same half
    // split (N/2 | N/2), table values from the continuous definition in double.
    // Tolerance covers float table rounding only (~1e-7 relative).
    {
        double worst = 0.0;
        for (int s = 0; s < WindowLut::kNumShapes; ++s)
        {
            double l1 = 0, h1 = 0, l2 = 0, h2 = 0;
            for (int i = 0; i < N; ++i)
            {
                const double v = defs[s] ((double) i / (N - 1));
                (i < N / 2 ? l1 : h1) += v;
                (i < N / 2 ? l2 : h2) += v * v;
            }
            const double mu1 = 0.5 * (l1 + h1) / (N / 2), mu2 = 0.5 * (l2 + h2) / (N / 2);
            const auto m = lut.getMoments (s);
            worst = std::max ({ worst, std::abs (m.mu1 - mu1) / mu1, std::abs (m.mu2 - mu2) / mu2 });
        }
        check (worst < 2e-6, "W1", "μ₁/μ₂ == double-precision rebuild of the 2048-pt tables",
               fmt ("worst rel err %.3g", worst));
    }

    // W2 — (2) continuous quadrature. The table is a 2048-point sampling with
    // both endpoints, so the discrete mean sits O(1/N) from the integral:
    // Hann's discrete μ₁ is 0.5·(N-1)/N... allow 1/N.
    {
        double worst = 0.0;
        std::string at;
        for (int s = 0; s < WindowLut::kNumShapes; ++s)
        {
            const auto c = continuousMoments (defs[s]);
            const auto m = lut.getMoments (s);
            const double e = std::max ({ std::abs (m.mu1 - c.mu1),   std::abs (m.mu2 - c.mu2),
                                         std::abs (m.mu1Lo - c.mu1Lo), std::abs (m.mu1Hi - c.mu1Hi),
                                         std::abs (m.mu2Lo - c.mu2Lo), std::abs (m.mu2Hi - c.mu2Hi) });
            if (e > worst) { worst = e; at = names[s]; }
            std::printf ("      %-10s μ₁ %.6f (∫ %.6f)   μ₂ %.6f (∫ %.6f)\n",
                         names[s], (double) m.mu1, c.mu1, (double) m.mu2, c.mu2);
        }
        check (worst < 1.0 / N, "W2", "μ₁/μ₂ and half-moments within 1/N of Simpson quadrature",
               fmt ("worst abs err %.3g", worst) + " (" + at + ")");
    }

    // W3 — (3) closed forms.
    {
        const auto h = lut.getMoments (WindowLut::hann), t = lut.getMoments (WindowLut::triangular);
        const double e = std::max ({ std::abs (h.mu1 - 0.5), std::abs (h.mu2 - 0.375),
                                     std::abs (t.mu1 - 0.5), std::abs (t.mu2 - 1.0 / 3.0) });
        check (e < 1.0 / N, "W3", "Hann μ₁=1/2 μ₂=3/8, Triangular μ₁=1/2 μ₂=1/3", fmt ("worst %.3g", e));
    }

    // W4 — Tukey across all 100 reachable α: closed forms 1 − α/2, 1 − 5α/8,
    // and quadrature of the continuous window at that α.
    {
        double worstClosed = 0.0, worstQuad = 0.0;
        for (int k = 0; k < WindowLut::kNumTaperSteps; ++k)
        {
            const float a = WindowLut::taperAlphaAt (k);
            const auto m = lut.getMoments (WindowLut::tukey, a);
            worstClosed = std::max ({ worstClosed, std::abs (m.mu1 - (1.0 - 0.5 * a)),
                                                   std::abs (m.mu2 - (1.0 - 0.625 * a)) });
            if (k % 11 == 0 || k == WindowLut::kNumTaperSteps - 1)
            {
                const auto c = continuousMoments ([a] (double x) { return tukeyC (x, a); });
                worstQuad = std::max ({ worstQuad, std::abs (m.mu1 - c.mu1), std::abs (m.mu2 - c.mu2) });
            }
        }
        check (worstClosed < 1.0 / N, "W4", "Tukey μ₁ = 1 − α/2, μ₂ = 1 − 5α/8 at all 100 α",
               fmt ("worst %.3g", worstClosed));
        check (worstQuad < 1.0 / N, "W5", "Tukey moments match quadrature across α",
               fmt ("worst %.3g", worstQuad));
    }

    // W6 — the accessors agree with getMoments bitwise (one source of truth).
    {
        bool ok = true;
        for (int s = 0; s < WindowLut::kNumShapes; ++s)
            for (float a : { 0.01f, 0.5f, 1.0f })
            {
                const auto m = lut.getMoments (s, a);
                ok = ok && m.mu1 == lut.getMean (s, a) && m.mu2 == lut.getMeanSquare (s, a);
            }
        check (ok, "W6", "getMoments == getMean / getMeanSquare bitwise");
    }

    // W7 — symmetric shapes: halves equal BITWISE, so tilt norm is exactly 1.0f.
    // Expo-Decay's halves must differ (the canonicalisation must not eat it).
    {
        bool ok = true;
        for (int s : { 0, 1, 2, 3 })
        {
            const auto m = lut.getMoments (s);
            ok = ok && m.mu1Lo == m.mu1Hi && m.mu2Lo == m.mu2Hi;
            for (float t : { 0.05f, 0.3f, 0.5f, 0.77f, 0.95f })
                ok = ok && lut.getTiltNorm (s, t) == 1.0f;
        }
        for (int k = 0; k < WindowLut::kNumTaperSteps; ++k)
        {
            const auto m = lut.getMoments (WindowLut::tukey, WindowLut::taperAlphaAt (k));
            ok = ok && m.mu2Lo == m.mu2Hi && m.mu1Lo == m.mu1Hi;
        }
        const auto e = lut.getMoments (WindowLut::expoDecay);
        check (ok, "W7", "symmetric shapes: lo == hi bitwise, tiltNorm == 1.0f exactly");
        check (e.mu2Lo > 5.0f * e.mu2Hi, "W8", "Expo-Decay halves stay distinct",
               fmt ("μ₂ lo %.4f hi %.4f", e.mu2Lo, e.mu2Hi));
    }

    // W9 — normalisations are the documented functions of the moments.
    {
        const float mh2 = lut.getMoments (WindowLut::hann).mu2, mh1 = lut.getMoments (WindowLut::hann).mu1;
        double worst = 0.0;
        for (int s = 0; s < WindowLut::kNumShapes; ++s)
            for (float tilt : { 0.2f, 0.5f, 0.8f })
            {
                const auto m = lut.getMoments (s);
                const double sn = std::sqrt ((double) mh2 / m.mu2);
                const double q  = tilt * m.mu2Lo + (1.0 - tilt) * m.mu2Hi;
                const double tn = std::sqrt (m.mu2 / q);
                const double m1 = tilt * m.mu1Lo + (1.0 - tilt) * m.mu1Hi;
                const double ln = mh1 / (sn * tn * m1);
                worst = std::max ({ worst, std::abs (lut.getShapeNorm (s) - sn) / sn,
                                           std::abs (lut.getTiltNorm (s, tilt) - tn) / tn,
                                           std::abs (lut.getLoopNorm (s, tilt) - ln) / ln });
            }
        check (lut.getShapeNorm (WindowLut::hann) == 1.0f && lut.getLoopNorm (WindowLut::hann, 0.5f) == 1.0f
                 && worst < 1e-6, "W9", "shape/tilt/loop norms = √(μ₂ₕ/μ₂), √(μ₂/q_t), μ₁ₕ/(norm·μ₁_t)",
               fmt ("worst rel %.3g", worst));
    }
}

//==============================================================================
void testCounterRng()
{
    // Golden values computed from O-TextureForge 1.6.0's CounterRng.h (the
    // O-simpleGrain, O-Freeze and O-GrainScatter copies produce the same).
    struct G { uint64_t seed, counter; uint32_t slot; uint64_t hash; int n1000; };
    const G golden[] = {
        { 0x0000000000000000ull, 0ull, 0u, 0x238275BC38FCBE91ull, 138 },
        { 0x0000000000000000ull, 0ull, 3u, 0xD0D12C1410F51724ull, 815 },
        { 0x0000000000000000ull, 1ull, 0u, 0xB18A02F46D8D86C3ull, 693 },
        { 0x0000000000000000ull, 1000003ull, 3u, 0xC57302AA0C47493Aull, 771 },
        { 0x1234567890ABCDEFull, 0ull, 0u, 0x4E6C7B2D7DB09281ull, 306 },
        { 0x1234567890ABCDEFull, 1ull, 3u, 0x1A4E8ACBD77D8EC1ull, 102 },
        { 0x1234567890ABCDEFull, 1000003ull, 0u, 0xB2266439FFF01E94ull, 695 },
        { 0xFFFFFFFFFFFFFFFFull, 0ull, 3u, 0xDE11EDAC2F82FDF3ull, 867 },
        { 0xFFFFFFFFFFFFFFFFull, 1ull, 0u, 0x52CF158ABCFC0039ull, 323 },
        { 0xFFFFFFFFFFFFFFFFull, 1000003ull, 3u, 0x390DF7CE21DFD679ull, 222 },
    };
    bool ok = true;
    for (const auto& g : golden)
        ok = ok && CounterRng::hash (g.seed, g.counter, g.slot) == g.hash
                && CounterRng::uniformInt (g.seed, g.counter, g.slot, 1000) == g.n1000
                && CounterRng::uniform (g.seed, g.counter, g.slot)
                     == (float) (g.hash >> 40) * (1.0f / 16777216.0f);
    check (ok, "R1", "hash / uniform / uniformInt match the plugin copies' golden values");

    bool eq = true;
    for (uint64_t c = 0; c < 1000; ++c)
    {
        const CounterRng::Draw d (0xABCDull, 0x5C4ED0000001ull, c);
        eq = eq && d.nextFloat (4) == CounterRng::uniform (CounterRng::mix (0xABCDull ^ 0x5C4ED0000001ull), c, 4)
                && d.nextInt (2, 17) == CounterRng::uniformInt (CounterRng::mix (0xABCDull ^ 0x5C4ED0000001ull), c, 2, 17);
    }
    check (eq, "R2", "Draw(seed, stream, c) == uniform(mix(seed^stream), c, slot)");

    double sum = 0.0; float lo = 1.0f, hi = 0.0f; bool inRange = true;
    constexpr int M = 1000000;
    for (int i = 0; i < M; ++i)
    {
        const float u = CounterRng::uniform (42, (uint64_t) i, 1);
        sum += u; lo = std::min (lo, u); hi = std::max (hi, u);
        const int k = CounterRng::uniformInt (42, (uint64_t) i, 2, 7);
        inRange = inRange && k >= 0 && k < 7;
    }
    check (std::abs (sum / M - 0.5) < 1e-3 && lo >= 0.0f && hi < 1.0f && inRange, "R3",
           "uniform in [0,1) with mean 0.5; uniformInt in [0,n)", fmt ("mean %.5f", sum / M));
}

//==============================================================================
struct AbsOnset { int64_t sample; double frac; uint64_t index; };

std::vector<AbsOnset> runScheduler (int totalSamples, int blockSize, int capacity,
                                    const std::function<double (uint64_t)>& fn,
                                    int* droppedOut = nullptr)
{
    GrainScheduler s;
    s.reset();
    std::vector<Onset> buf ((size_t) std::max (1, capacity));
    std::vector<AbsOnset> all;
    int dropped = 0;
    for (int start = 0; start < totalSamples; start += blockSize)
    {
        const int n = std::min (blockSize, totalSamples - start);
        const auto r = s.process (n, buf.data(), capacity, [&] (uint64_t k) { return fn (k); });
        for (int i = 0; i < r.count; ++i)
            all.push_back ({ start + buf[(size_t) i].offset, buf[(size_t) i].frac, buf[(size_t) i].index });
        dropped += r.dropped;
    }
    if (droppedOut) *droppedOut = dropped;
    return all;
}

void testScheduler()
{
    // S1 — fixed fractional interval: onset k's exact instant (sample − frac)
    // is k·I, never truncated; frac in [0, 1).
    {
        const double I = 137.37;
        const auto on = runScheduler (48000 * 30, 512, 4096, [I] (uint64_t) { return I; });
        double worst = 0.0; bool fracOk = true, idxOk = true;
        for (size_t k = 0; k < on.size(); ++k)
        {
            worst  = std::max (worst, std::abs (((double) on[k].sample - on[k].frac) - (double) k * I));
            fracOk = fracOk && on[k].frac >= 0.0 && on[k].frac < 1.0;
            idxOk  = idxOk && on[k].index == k;
        }
        const size_t expected = (size_t) std::floor ((48000.0 * 30 - 1) / I) + 1;
        check (worst < 1e-6 && fracOk && idxOk && on.size() == expected, "S1",
               "interval 137.37: onset k at exactly k·I (sample − frac), frac ∈ [0,1)",
               fmt ("%.0f onsets, worst drift %.3g samples", (double) on.size(), worst));
    }

    // S2 — block-size invariance with counter-keyed jitter.
    {
        auto fn = [] (uint64_t k)
        {
            return GrainScheduler::jittered (211.7, 0.8f, CounterRng::uniform (7, k, 0));
        };
        const auto ref = runScheduler (200000, 1, 4096, fn);
        bool ok = ! ref.empty();
        for (int bs : { 7, 64, 333, 512, 4096, 16384 })
        {
            const auto o = runScheduler (200000, bs, 4096, fn);
            ok = ok && o.size() == ref.size();
            for (size_t i = 0; ok && i < o.size(); ++i)
                ok = o[i].sample == ref[i].sample && std::memcmp (&o[i].frac, &ref[i].frac, sizeof (double)) == 0
                     && o[i].index == ref[i].index;
        }
        check (ok, "S2", "jittered onset stream bit-identical at block sizes 1/7/64/333/512/4096/16384",
               fmt ("%.0f onsets", (double) ref.size()));
    }

    // S3 — capacity overflow drops, never shifts.
    {
        auto fn = [] (uint64_t) { return 1.5; };
        int dropped = 0;
        const auto full    = runScheduler (4096, 512, 4096, fn);
        const auto clipped = runScheduler (4096, 512, 16, fn, &dropped);
        bool ok = (int) clipped.size() + dropped == (int) full.size() && dropped > 0;
        size_t j = 0;
        for (const auto& c : clipped)
        {
            while (j < full.size() && full[j].index != c.index) ++j;
            ok = ok && j < full.size() && full[j].sample == c.sample && full[j].frac == c.frac;
        }
        check (ok, "S3", "capacity drops are counted and later onsets do not move",
               fmt ("%.0f kept, %.0f dropped", (double) clipped.size(), (double) dropped));
    }

    // S4 — jittered(): mean exactly nominal in expectation, floor at 0.1×.
    {
        double sum = 0.0, mn = 1e9;
        constexpr int M = 1000000;
        for (int i = 0; i < M; ++i)
        {
            const double v = GrainScheduler::jittered (400.0, 1.0f, CounterRng::uniform (3, (uint64_t) i, 0));
            sum += v; mn = std::min (mn, v);
        }
        check (std::abs (sum / M / 400.0 - 1.0) < 2e-3 && mn >= 40.0
                 && GrainScheduler::jittered (400.0, 0.0f, 0.99f) == 400.0,
               "S4", "jittered: mean = nominal, floor (1 − 0.9)·nominal, amount 0 = identity",
               fmt ("mean/nominal %.5f, min %.2f", sum / M / 400.0, mn));
    }

    // S5 — rescheduleNext: shorter-than-elapsed fires next sample at frac 0;
    // a longer one moves the pending onset to last + new.
    {
        GrainScheduler s; s.reset();
        Onset o[8];
        auto r = s.process (1, o, 8, 100.25);                // onset 0 at sample 0
        r = s.process (50, o, 8, 100.25);                    // samples 1..50: nothing
        const bool none = r.count == 0;
        s.rescheduleNext (10.0);                             // already 51 elapsed
        r = s.process (1, o, 8, 100.25);
        const bool fired = r.count == 1 && o[0].offset == 0 && o[0].frac == 0.0 && o[0].index == 1;

        GrainScheduler t; t.reset();
        t.process (1, o, 8, 100.0);
        t.rescheduleNext (250.5);
        int at = -1; double fr = -1.0;
        for (int i = 1; i < 400 && at < 0; ++i)
            if (t.process (1, o, 8, 100.0).count == 1) { at = i; fr = o[0].frac; }
        check (none && fired && at == 251 && std::abs (fr - 0.5) < 1e-12, "S5",
               "rescheduleNext: elapsed → next sample, frac 0; longer → last + new (251, frac 0.5)");
    }
}

//==============================================================================
struct TestGrain
{
    double phase = 0.0;    // 0..1 progress
    double inc   = 0.0;
    float  level = 0.0f;
    int    id    = -1;
};

void testPool()
{
    constexpr double fs = 48000.0;
    constexpr int fadeSamples = 240;   // 5 ms

    // P1 — steal-free: every fader gain is exactly 1.0f and contributions are
    // the unscaled values, bitwise.
    {
        GrainPool<TestGrain, 8, 2> pool; pool.prepare (fs);
        for (int k = 0; k < 6; ++k) { auto& g = pool.obtain(); g.level = 0.1f * (float) (k + 1); g.inc = 1e-4; }
        bool ok = true;
        for (int n = 0; n < 2000; ++n)
            pool.render ([&] (TestGrain& g, auto& fade)
            {
                const float v = g.level * fade.next();
                ok = ok && std::memcmp (&v, &g.level, sizeof (float)) == 0;
                g.phase += g.inc;
                return g.phase < 1.0;
            });
        check (ok && pool.countActive() == 6 && pool.liveWeight() == 6.0f, "P1",
               "steal-free: fade gain 1.0f exactly, contributions bitwise unscaled");
    }

    // P2 — steal: the victim keeps rendering in a fade slot; its contribution
    // falls by exactly 1/240 per sample from where it was (no step), reaches 0
    // and the slot retires; the new grain takes the victim's slot.
    {
        GrainPool<TestGrain, 4, 2> pool; pool.prepare (fs);
        for (int k = 0; k < 4; ++k) { auto& g = pool.obtain(); g.level = 1.0f; g.id = k; }
        for (int n = 0; n < 10; ++n) pool.render ([] (TestGrain&, auto& f) { f.next(); return true; });

        auto& fresh = pool.obtain();           // steals id 0 (earliest birth)
        const bool tookSlot = &fresh == &pool.getGrains()[0];
        fresh.level = 0.0f; fresh.id = 99;

        std::vector<float> victim;
        int retiredAt = -1;
        for (int n = 0; n < 400; ++n)
        {
            float v = -1.0f;
            pool.render ([&] (TestGrain& g, auto& f) { const float x = g.level * f.next(); if (g.id == 0) v = x; return true; });
            if (v >= 0.0f) victim.push_back (v);
            else if (retiredAt < 0) retiredAt = n;
        }
        if (victim.empty())       // never rendered after the steal = a hard cut;
            victim.push_back (0.0f);   // front() != 1 then fails the check below
        double maxStep = 1.0 - victim.front();
        for (size_t i = 1; i < victim.size(); ++i)
            maxStep = std::max (maxStep, (double) std::abs (victim[i] - victim[i - 1]));
        // Retirement is the last step: from the final rendered value to silence.
        // Float accumulation leaves that value a hair above 0 (not exactly 0).
        maxStep = std::max (maxStep, (double) victim.back());
        check (tookSlot && victim.front() == 1.0f && maxStep <= 1.0 / fadeSamples + 1e-6
                 && victim.back() < 1.0f / fadeSamples && retiredAt >= fadeSamples && retiredAt <= fadeSamples + 1
                 && pool.countActive() == 4,
               "P2", "steal: victim fades 1→0 over 5 ms in a spare slot, max step 1/240, then retires",
               fmt ("max step %.5f (hard cut = 1), retired after %.0f samples", maxStep, retiredAt));
    }

    // P3 — progress-based victim: the grain furthest through its window.
    {
        GrainPool<TestGrain, 4, 2> pool; pool.prepare (fs);
        const double ph[] = { 0.2, 0.9, 0.4, 0.6 };
        for (int k = 0; k < 4; ++k) { auto& g = pool.obtain(); g.phase = ph[k]; g.id = k; }
        auto& fresh = pool.obtain ([] (const TestGrain& g) { return g.phase; });
        check (&fresh == &pool.getGrains()[1] && pool.getGrains()[4].id == 1, "P3",
               "obtain(progress) steals the largest-progress grain (phase 0.9)");
    }

    // P4 — spares exhausted: the spare nearest silence is reused.
    {
        GrainPool<TestGrain, 2, 2> pool; pool.prepare (fs);
        for (int k = 0; k < 2; ++k) pool.obtain().id = k;
        pool.obtain().id = 2;                                     // steal → spare A fading
        for (int n = 0; n < 100; ++n) pool.render ([] (TestGrain&, auto& f) { f.next(); return true; });
        pool.obtain().id = 3;                                     // steal → spare B fresh
        for (int n = 0; n < 10; ++n) pool.render ([] (TestGrain&, auto& f) { f.next(); return true; });
        const float gA = pool.getFadeGain (2), gB = pool.getFadeGain (3);
        const int quieter = gA < gB ? 2 : 3;
        pool.obtain().id = 4;                                     // steal → reuses the quieter spare
        const float gNew = pool.getFadeGain (quieter);
        check (gA < gB && gNew == 1.0f && pool.countActive() == 4, "P4",
               "spares exhausted: the spare nearest silence is reused",
               fmt ("spare gains %.3f / %.3f", gA, gB));
    }

    // P5 — tryObtain refuses when full and disturbs nothing; release frees.
    {
        GrainPool<TestGrain, 3, 1> pool; pool.prepare (fs);
        for (int k = 0; k < 3; ++k) pool.tryObtain()->id = k;
        const bool refused = pool.tryObtain() == nullptr && pool.countActive() == 3;
        pool.release (pool.getGrains()[1]);
        auto* g = pool.tryObtain();
        check (refused && g == &pool.getGrains()[1] && g->id == -1 && pool.countActive() == 3, "P5",
               "tryObtain: nullptr when full (no steal); released slot reused, value-initialised");
    }

    // P6 — liveWeight counts a fading grain by its gain.
    {
        GrainPool<TestGrain, 2, 2> pool; pool.prepare (fs);
        pool.obtain(); pool.obtain(); pool.obtain();
        for (int n = 0; n < 120; ++n) pool.render ([] (TestGrain&, auto& f) { f.next(); return true; });
        const float w = pool.liveWeight();
        check (std::abs (w - 2.5f) < 1e-4f && pool.countActive() == 3, "P6",
               "liveWeight = live count + fading gain", fmt ("%.5f (expect 2.5)", w));
    }
}

//==============================================================================
// BandlimitedReader
using Ring = MipRing<3>;
using Snap = MipSnapshot<3>;

void fftInPlace (std::vector<std::complex<double>>& a)
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
        const double ang = -2.0 * kPi / (double) len;
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v; a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

// Worst audible spurious (dB re a unit input tone) of a tone at fIn (level-0 cycles)
// read on `level` at residual r' with sinc set `set`. Output rate = level-0 rate;
// audible = below aOut (cycles/sample). 4-term Blackman-Harris, the wanted
// component's +-8 bins excluded when it lies below the output Nyquist.
double spuriousDb (double fIn, int level, double rPrime, int set, double aOut, double* wantedDb = nullptr)
{
    constexpr int M = 8192;
    const double rate = rPrime * (double) (1 << level);          // level-0 samples per output sample
    const int need = (int) std::ceil (M * rate) + Ring::margin (level) + 4096;
    Ring ring;
    ring.prepare (need + 64);
    for (int i = 0; i < need; ++i)
    {
        const float v = (float) std::sin (2.0 * kPi * fIn * i);
        ring.push (v, v);
    }
    std::vector<std::complex<double>> buf (M);
    double wsum = 0.0;
    const double startDelay = (double) Ring::margin (level) + M * rate + 1.37;
    for (int n = 0; n < M; ++n)
    {
        const double x = (double) n / M;
        const double w = 0.35875 - 0.48829 * std::cos (2 * kPi * x) + 0.14128 * std::cos (4 * kPi * x) - 0.01168 * std::cos (6 * kPi * x);
        wsum += w;
        buf[(size_t) n] = w * (double) ring.read (level, 0, startDelay - n * rate, set);
    }
    fftInPlace (buf);
    const double ref = wsum / 2.0;                                  // unit sine peak bin
    const double fOut = fIn * rate;
    const bool wanted = fOut < 0.5;
    const double wantedBin = fOut * M;
    double worst = 1e-30, want = 0.0;
    for (int k = 1; k < M / 2; ++k)
    {
        const double mag = std::abs (buf[(size_t) k]) / ref;
        if (wanted && std::abs (k - wantedBin) <= 8.0) { want = std::max (want, mag); continue; }
        if ((double) k / M < aOut) worst = std::max (worst, mag);
    }
    if (wantedDb) *wantedDb = 20.0 * std::log10 (want + 1e-30);
    return 20.0 * std::log10 (worst);
}

void testBandlimitedReader()
{
    const SincBank& bank = sincBank();

    // B1 — unity DC on every stored row and on lerped phases.
    {
        double worstRow = 0.0, worstLerp = 0.0;
        for (int s = 0; s < SincBank::kNumSets; ++s)
        {
            for (int p = 0; p <= SincBank::kPhases; ++p)
            {
                double sum = 0.0;
                for (int k = 0; k < SincBank::kTaps; ++k) sum += bank.row (s, p)[k];
                worstRow = std::max (worstRow, std::abs (sum - 1.0));
            }
            for (int i = 0; i < 4096; ++i)
            {
                float kk[SincBank::kTaps];
                bank.kernel (s, (i + 0.5) / 4096.0, kk);
                double sum = 0.0;
                for (float v : kk) sum += v;
                worstLerp = std::max (worstLerp, std::abs (sum - 1.0));
            }
        }
        check (worstRow <= 1e-6 && worstLerp <= 1e-6, "B1", "every phase of every sinc set sums to 1 ± 1e-6 (stored + lerped)",
               fmt ("worst |sum-1|: rows %.2e, lerped %.2e", worstRow, worstLerp));
    }

    // B2 — level alignment: a Gaussian transient (sigma 16 smp, < -40 dB at fs/32) read
    // on levels 0..3 through the D(l) mapping peaks at the same level-0 time.
    {
        Ring ring;
        ring.prepare (1 << 15);
        const double tPeak = 9000.3;
        const int total = 12000;
        for (int i = 0; i < total; ++i)
        {
            const double d = (i - tPeak) / 16.0;
            const float v = (float) std::exp (-0.5 * d * d);
            ring.push (v, -v);
        }
        auto peakTime = [&] (int level)
        {
            // Coarse argmax on a 1/8 grid, then a least-squares parabola through log(y)
            // over +-8 samples (129 reads): a Gaussian's log is exactly a parabola, and
            // the window dwarfs the kernel's +-4e-5 phase-periodic passband ripple, which
            // a 3-point parabola on the flat top would read as a peak shift.
            double c = 0.0, bestV = -1e9;
            for (double t = tPeak - 4.0; t <= tPeak + 4.0; t += 0.125)
            {
                const double v = ring.read (level, 0, (double) total - t, 0);
                if (v > bestV) { bestV = v; c = t; }
            }
            double S[5] = {}, Ty[3] = {};
            for (int j = -64; j <= 64; ++j)
            {
                const double x = j / 8.0;
                const double y = std::log ((double) ring.read (level, 0, (double) total - (c + x), 0));
                double xp = 1.0;
                for (int e = 0; e < 5; ++e) { S[e] += xp; if (e < 3) Ty[e] += xp * y; xp *= x; }
            }
            // normal equations for y = a + b x + d x^2
            const double A[3][3] = { { S[0], S[1], S[2] }, { S[1], S[2], S[3] }, { S[2], S[3], S[4] } };
            auto det3 = [] (const double m[3][3])
            {
                return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
                     + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
            };
            const double D = det3 (A);
            double Ab[3][3], Ad[3][3];
            for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) { Ab[i][j] = A[i][j]; Ad[i][j] = A[i][j]; }
            for (int i = 0; i < 3; ++i) { Ab[i][1] = Ty[i]; Ad[i][2] = Ty[i]; }
            const double b = det3 (Ab) / D, d = det3 (Ad) / D;
            return c - b / (2.0 * d);
        };
        const double p0 = peakTime (0);
        double worst = 0.0;
        std::string det = fmt ("level0 peak %.4f (true %.1f)", p0, tPeak);
        for (int l = 1; l <= 3; ++l)
        {
            const double e = peakTime (l) - p0;
            worst = std::max (worst, std::abs (e));
            det += fmt ("; L%.0f %+.4f", l, e);
        }
        check (worst <= 0.05 && std::abs (p0 - tPeak) <= 0.05, "B2",
               "levels 1/2/3 peak at the level-0 time (<= 0.05 sample)", det);
    }

    // B3a — half-band stopband from 0.25 + delta(44.1 k) = 0.2732: <= -90 dB.
    {
        double worst = 0.0;
        for (int i = 0; i <= 20000; ++i)
        {
            const double f = 0.25 + 0.5 * (0.5 - 20.0 / 44.1) + i * (0.5 - 0.2732) / 20000.0;
            double re = 0.5;   // centre tap, phase reference at the centre
            for (int m = 0; m < 32; ++m)
                re += 2.0 * blr::kHalfbandOdd[m] * std::cos (2.0 * kPi * f * (2 * m + 1));
            worst = std::max (worst, std::abs (re));
        }
        double dc = 0.5; for (double c : blr::kHalfbandOdd) dc += 2.0 * c;
        check (20.0 * std::log10 (worst) <= -90.0 && std::abs (dc - 1.0) < 1e-12, "B3a",
               "half-band stopband <= -90 dB from 0.2732 (44.1 k), DC 1",
               fmt ("%.1f dB, DC - 1 = %.1e", 20.0 * std::log10 (worst), dc - 1.0));
    }

    // B3b — sinc sets S0..S6 driven at their upper-edge r' with tones 20 Hz..20 kHz
    // (44.1 k), on level 0 and (S1..S6) level 1, where the half-band transition band
    // is in play too: worst audible spurious <= -60 dB. S7 reported at 2.83 / 3.99.
    {
        const double fs = 44100.0, aOut = 20000.0 / fs;
        std::vector<double> tones;
        for (int i = 0; i < 24; ++i) tones.push_back (20.0 * std::pow (1000.0, i / 23.0));   // 20 Hz .. 20 kHz log
        for (double f = 9000.0; f <= 20000.0; f += 250.0) tones.push_back (f);             // dense top
        for (int s = 0; s <= 7; ++s)
        {
            const double rr[] = { blr::kSincSets[s].edge, 0.0 };
            std::vector<double> rs { s == 7 ? std::pow (2.0, 1.5) : rr[0] };
            if (s == 7) rs.push_back (3.99);
            for (double r : rs)
            {
                double worst = -300.0, worstF = 0.0;
                for (int level = 0; level <= (s == 0 ? 0 : 1); ++level)
                    for (double f : tones)
                    {
                        const double d = spuriousDb (f / fs / (level ? 1.0 : 1.0), level, r, s, aOut);
                        if (d > worst) { worst = d; worstF = f; }
                    }
                char id[8]; std::snprintf (id, sizeof id, "B3%c", (char) ('b' + s));
                char what[96];
                std::snprintf (what, sizeof what, "S%d at r' %.3f: worst audible spurious <= -60 dB%s", s, r, s == 7 ? " (S7: REPORT)" : "");
                if (s == 7)
                    std::printf ("INFO  %-4s %s  — %.1f dB (tone %.0f Hz)\n", id, what, worst, worstF);
                else
                    check (worst <= -60.0, id, what, fmt ("%.1f dB (tone %.0f Hz)", worst, worstF));
            }
        }
    }

    // B4 — block-split invariance: 1 x N vs random chunks with reads in between.
    {
        std::mt19937 rng (77);
        std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        const int n = 50000;
        std::vector<float> xl ((size_t) n), xr ((size_t) n);
        for (int i = 0; i < n; ++i) { xl[(size_t) i] = u (rng); xr[(size_t) i] = u (rng); }
        Ring a, b;
        a.prepare (1 << 16); b.prepare (1 << 16);
        for (int i = 0; i < n; ++i) a.push (xl[(size_t) i], xr[(size_t) i]);
        int pos = 0;
        volatile float sink = 0.0f;
        std::uniform_int_distribution<int> chunk (1, 777);
        while (pos < n)
        {
            const int c = std::min (n - pos, chunk (rng));
            for (int i = 0; i < c; ++i) b.push (xl[(size_t) (pos + i)], xr[(size_t) (pos + i)]);
            pos += c;
            sink = sink + b.read (3, 0, 600.0, 2);
        }
        bool same = true;
        for (int l = 0; l <= 3 && same; ++l)
        {
            same = a.getLevelCount (l) == b.getLevelCount (l);
            for (std::int64_t j = 0; j < a.getLevelCount (l) && same; ++j)
                for (int ch = 0; ch < 2; ++ch)
                    same = same && (a.levelSample (l, ch, j) == b.levelSample (l, ch, j));
        }
        check (same && a.getLevelCount (1) == n / 2 && a.getLevelCount (3) == n / 8, "B4",
               "every level bit-identical for 1 x N vs random chunks", fmt ("levels %.0f/%.0f/%.0f samples", (double) a.getLevelCount (1), (double) a.getLevelCount (2), (double) a.getLevelCount (3)));
    }

    // B5 — alloc-free after prepare: push / read* / capture / snapshot read*.
    {
        Ring ring; Snap snap;
        ring.prepare (100000);
        snap.prepare (30000);
        float l = 0, r = 0, acc = 0;
        const long before = gAllocCount.load();
        for (int i = 0; i < 60000; ++i)
        {
            ring.push ((float) std::sin (i * 0.01), (float) std::cos (i * 0.013));
            if (i > 2000)
                for (int lv = 0; lv <= 3; ++lv)
                {
                    acc += ring.read (lv, 1, 600.0 + i % 97 * 0.31, lv * 2);
                    ring.readStereo (lv, 700.25, 7, l, r); acc += l + r;
                    acc += ring.readMono (lv, 900.5, 3);
                }
            if (i == 40000) snap.captureFrom (ring, Ring::margin (3), 30000);
            if (i > 40000)
            {
                acc += snap.read (2, 0, 15000.3, 4);
                snap.readStereo (3, 2000.7, 1, l, r); acc += l + r;
                acc += snap.readMono (1, 29999.9, 6);
            }
        }
        ring.clear(); snap.clear();
        const long after = gAllocCount.load();
        check (after == before && std::isfinite (acc), "B5", "no allocation in push / read / capture / snapshot read / clear",
               fmt ("%.0f allocations", (double) (after - before)));
    }

    // B6 — snapshot reads == live reads at the same absolute times (bit-identical).
    {
        std::mt19937 rng (5);
        std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        Ring ring; Snap snap;
        ring.prepare (1 << 17);
        snap.prepare (40000);
        for (int i = 0; i < 90001; ++i) ring.push (u (rng), u (rng));
        const int len = 40000, endDelay = Ring::margin (3);
        snap.captureFrom (ring, endDelay, len);
        const std::int64_t W = ring.getWriteCount();
        bool same = true; int n = 0;
        double worst = 0.0;
        for (int l = 0; l <= 3; ++l)
            for (int s = 0; s < 8; ++s)
                for (int k = 0; k < 300; ++k)
                {
                    const int bI = 64 + (k * 131) % (len - 128);
                    const double frac = (double) ((k * 37) % 1024) / 1024.0;
                    const double b = bI + frac;
                    // absolute time = origin + b = W - delay
                    const double delay = (double) (W - snap.getOrigin() - bI) - frac;
                    float sl, sr, ll, lr;
                    snap.readStereo (l, b, s, sl, sr);
                    ring.readStereo (l, delay, s, ll, lr);
                    const float sm = snap.readMono (l, b, s), lm = ring.readMono (l, delay, s);
                    same = same && sl == ll && sr == lr && sm == lm && snap.read (l, 1, b, s) == ring.read (l, 1, delay, s);
                    worst = std::max (worst, (double) std::abs (sl - ll));
                    ++n;
                }
        // the newest captured sample is settled on every level
        float e0 = snap.read (3, 0, len - 1.0, 0), e1 = ring.read (3, 0, (double) (endDelay + 1), 0);
        check (same && e0 == e1, "B6", "MipSnapshot reads == live reads at the same absolute time, every level/set, bit-identical",
               fmt ("%.0f reads, worst |diff| %.1e", (double) n, worst));
    }

    // B7 — read guard: at delay margin(l) (and up to +1), the newest tap is the newest
    // level sample — never past it — for every head parity.
    {
        bool ok = true; int tight = 0;
        for (int l = 0; l <= 3; ++l)
            for (std::int64_t W = 4096; W < 4096 + 64; ++W)
                for (int k = 0; k < 16; ++k)
                {
                    const double delay = Ring::margin (l) + k / 16.0;
                    const double dI = std::floor (delay), df = delay - dI;
                    std::int64_t t = W - (std::int64_t) dI; double phi = 0.0;
                    if (df > 0.0) { t -= 1; phi = 1.0 - df; }
                    const auto p = Ring::levelPos (l, t, phi);
                    const std::int64_t newest = (W >> l) - 1;
                    ok = ok && p.index + blr::kTapsAhead <= newest;
                    tight += (p.index + blr::kTapsAhead == newest) ? 1 : 0;
                }
        check (ok && tight > 0 && Ring::margin (0) == 9 && Ring::margin (1) == 80 && Ring::margin (2) == 222 && Ring::margin (3) == 506,
               "B7", "margin(l) = 9/80/222/506 keeps every tap settled (and is tight)", fmt ("%.0f tight cases", (double) tight));
    }

    // B8 — latchLevel / setFor.
    {
        const bool lv = latchLevel (0.125) == 0 && latchLevel (1.999) == 0 && latchLevel (2.0) == 1 && latchLevel (3.99) == 1
                     && latchLevel (4.0) == 2 && latchLevel (7.99) == 2 && latchLevel (8.0) == 3 && latchLevel (16.0) == 3;
        bool st = SincBank::setFor (0.5) == 0 && SincBank::setFor (1.0) == 0 && SincBank::setFor (1.0000001) == 1
               && SincBank::setFor (2.0) == 6 && SincBank::setFor (2.0000001) == 7 && SincBank::setFor (3.99) == 7;
        for (int k = 1; k <= 6; ++k)
            st = st && SincBank::setFor (blr::kSincSets[k].edge) == k && SincBank::setFor (blr::kSincSets[k].edge * 1.0000001) == k + 1;
        check (lv && st, "B8", "latchLevel = clamp(floor(log2 r), 0, 3); setFor = first set whose edge >= r'");
    }
}
} // namespace

int main()
{
    testWindowLut();
    testCounterRng();
    testScheduler();
    testPool();
    testBandlimitedReader();
    std::printf ("\n%s  %d passed, %d failed\n", failures == 0 ? "ALL PASS" : "FAILED", passes, failures);
    return failures == 0 ? 0 : 1;
}
