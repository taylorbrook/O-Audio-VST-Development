// Timing micro-benchmark: ns/sample for terrain, orbit, image, Chebyshev, wavetable kernels.
// Each loop carries a (numerically negligible) dependency out→phase so the compiler
// cannot pipeline/vectorise across samples — matching a real per-sample voice loop.
#include "kernels.h"
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <functional>

static constexpr int kN = 1 << 21;
static constexpr int kReps = 5;
static const double kSR = 48000.0;
static float sink = 0.f;

template <typename F>
static double timeNs (F&& f)
{
    double best = 1e30;
    for (int r = 0; r < kReps; ++r)
    {
        auto t0 = std::chrono::steady_clock::now();
        float acc = f();
        auto t1 = std::chrono::steady_clock::now();
        sink += acc;
        double ns = std::chrono::duration<double, std::nano> (t1 - t0).count() / kN;
        best = std::min (best, ns);
    }
    return best;
}

struct Row { std::string name; double ns; };
static std::vector<Row> rows;
static void rec (const char* name, double ns) { rows.push_back ({ name, ns }); std::printf ("%-52s %8.2f ns\n", name, ns); }

int main()
{
    SinTable T;
    ImageTerrain img (512);
    ChebTerrain<16> cheb;
    { std::mt19937 rng (7); std::uniform_real_distribution<float> u (-1.f, 1.f);
      for (auto& r : cheb.c) for (auto& v : r) v = u (rng) * 0.1f; }
    Wavetable wt (64);
    const float inc = (float) (kTwoPi * 1046.5 / kSR);   // C6
    const float eps = 1e-20f;                           // dependency carrier

    std::printf ("Kernel micro-benchmark, N=%d samples/rep, best of %d reps (clang++ -O2)\n\n", kN, kReps);

    // ── Orbits only ──
    rec ("orbit: ellipse std::sin/cos", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_std (ph, 0.8f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("orbit: ellipse table sin/cos", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("orbit: epitrochoid-7 std", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = epitrochoid7_std (ph, 0.3f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("orbit: epitrochoid-7 table", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = epitrochoid7_tab (T, ph, 0.3f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("orbit: superellipse std (3x pow)", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = superellipse_std (ph, 0.6f, 0.5f, 0.7f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("orbit: superellipse table sin + 3x pow", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = superellipse_tab (T, ph, 0.6f, 0.5f, 0.7f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("orbit: butterfly std (exp+pow+4 trig)", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = butterfly_std (ph, 0.3f); acc += p.x + p.y; ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));

    // ── Terrains only (fixed cheap orbit: ellipse table) ──
    rec ("terrain: sine-product std::sin + fastTanh", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += saturate (sineProduct_std (p, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("terrain: sine-product table sin + fastTanh", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += saturate (sineProduct_tab (T, p, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("terrain: system-14 std (sqrt,sin,cos) + fastTanh", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += saturate (system14_std (p, 0.5f, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("terrain: system-14 table + fastTanh", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += saturate (system14_tab (T, p, 0.5f, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("terrain: 512x512 image bilinear + fastTanh", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += saturate (img.read (p), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("terrain: Chebyshev 16x16 Clenshaw (no tanh)", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += cheb.eval (p); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("terrain: Chebyshev 16x16 + fastTanh", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += saturate (cheb.eval (p), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));

    // ── Full live-oscillator combos (Terrain-like, per unison partial) ──
    rec ("live: epitrochoid-7 std + sine-product std", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = epitrochoid7_std (ph, 0.3f); acc += saturate (sineProduct_std (p, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("live: epitrochoid-7 table + sine-product table", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = epitrochoid7_tab (T, ph, 0.3f); acc += saturate (sineProduct_tab (T, p, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("live: butterfly std + system-14 std (worst)", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = butterfly_std (ph, 0.3f); p.x *= 0.2f; p.y *= 0.2f; acc += saturate (system14_std (p, 0.5f, 0.5f, 0.5f), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("live: epitrochoid-7 table + image bilinear", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = epitrochoid7_tab (T, ph, 0.3f); acc += saturate (img.read (p), 1.f); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));
    rec ("live: ellipse table + Chebyshev 16x16", timeNs ([&] { float ph = 0, acc = 0; for (int i = 0; i < kN; ++i) { Pt p = ellipse_tab (T, ph, 0.8f); acc += cheb.eval (p); ph += inc + acc * eps; if (ph > kTwoPi) ph -= kTwoPi; } return acc; }));

    // ── Baseline: O-Prism mipmapped wavetable read (trilinear, double) ──
    rec ("wavetable: trilinear + per-sample log2 (O-Prism)", timeNs ([&] { double ph = 0; float acc = 0; double f = 1046.5; for (int i = 0; i < kN; ++i) { acc += (float) wt.read (ph, 0.37f, f, kSR); ph += f / kSR + acc * 1e-30; if (ph >= 1.0) ph -= 1.0; } return acc; }));
    rec ("wavetable: trilinear, log2 hoisted (ideal)", timeNs ([&] { double ph = 0; float acc = 0; double f = 1046.5;
        // same read with level fixed: emulate by passing a frequency the log2 of which is cached — use a local copy of the arithmetic
        double baseFreq = kSR / 2048; double lv = std::log2 (f / baseFreq); int l0 = (int) lv, l1 = l0 + 1; double lf = lv - l0;
        for (int i = 0; i < kN; ++i) {
            double p = ph - std::floor (ph); double sp = p * 2048; int i0 = (int) sp; double fr = sp - i0;
            double fp = 0.37f * 63; int f0 = (int) fp, f1 = f0 + 1; double ff = fp - f0;
            auto L = [] (double a, double b, double t) { return a + t * (b - a); };
            double s00 = L (wt.get (l0, f0, i0), wt.get (l0, f0, i0 + 1), fr), s01 = L (wt.get (l0, f1, i0), wt.get (l0, f1, i0 + 1), fr);
            double s10 = L (wt.get (l1, f0, i0), wt.get (l1, f0, i0 + 1), fr), s11 = L (wt.get (l1, f1, i0), wt.get (l1, f1, i0 + 1), fr);
            acc += (float) L (L (s00, s01, ff), L (s10, s11, ff), lf);
            ph += f / kSR + acc * 1e-30; if (ph >= 1.0) ph -= 1.0; } return acc; }));

    // ── 2× halfband polyphase IIR (per output sample at base rate = 1 up + 1 down) ──
    rec ("oversampler: 2x halfband IIR up+down, per base sample", timeNs ([&] { Halfband2x hb; float acc = 0, x = 0.1f, ph = 0; for (int i = 0; i < kN; ++i) { float a, b; hb.up (x, a, b); float y = hb.down (a, b); acc += y; ph += inc; if (ph > kTwoPi) ph -= kTwoPi; x = ph * 0.1f + y * 1e-20f; } return acc; }));

    std::printf ("\n(sink=%g, finite=%d)\n", sink, (int) std::isfinite (sink));

    // ── Budget table ──
    auto find = [&] (const char* n) { for (auto& r : rows) if (r.name == n) return r.ns; return 0.0; };
    const double sr = 48000.0, core = 1e9;
    struct Cfg { const char* label; double ns; int os; };
    Cfg cfgs[] = {
        { "wavetable trilinear (O-Prism today), 1x",              find ("wavetable: trilinear + per-sample log2 (O-Prism)"), 1 },
        { "live analytic table-trig (epi7+sineprod), 2x",          find ("live: epitrochoid-7 table + sine-product table"), 2 },
        { "live analytic std-trig (epi7+sineprod), 2x",            find ("live: epitrochoid-7 std + sine-product std"), 2 },
        { "live worst (butterfly+system14 std), 2x",               find ("live: butterfly std + system-14 std (worst)"), 2 },
        { "live worst (butterfly+system14 std), 4x",               find ("live: butterfly std + system-14 std (worst)"), 4 },
        { "live image 512^2 bilinear (epi7 table), 2x",            find ("live: epitrochoid-7 table + image bilinear"), 2 },
        { "live Chebyshev 16x16 circular orbit, 1x (bandlimited)", find ("live: ellipse table + Chebyshev 16x16"), 1 },
    };
    std::printf ("\nPer-partial cost at 48 kHz (%% of one core), OS factor applied to the oscillator only:\n");
    std::printf ("%-58s %6s %10s %10s %12s %12s\n", "config", "OS", "1 partial", "1 voice*", "16v x2osc U1", "16v x2osc U4");
    for (auto& c : cfgs)
    {
        double perPartial = c.ns * c.os * sr / core * 100.0;
        std::printf ("%-58s %6d %9.3f%% %9.3f%% %11.2f%% %11.2f%%\n", c.label, c.os, perPartial, perPartial * 2, perPartial * 32, perPartial * 128);
    }
    std::printf ("* 1 voice = 2 oscillators, unison 1. Oversampler filter adds %.2f ns per base sample per oversampled stream.\n",
                 find ("oversampler: 2x halfband IIR up+down, per base sample"));
    return 0;
}
