/*
  ==============================================================================

    O-Granulator — render harness, sections U5–U8 / FB (Stage 2 / Phase 2.4,
    PLAN-2.4 T4–T7, P44). Sibling of engine_gates_23.h.

      U5a D9 knee identity: every float with |x| <= T, both signs (2.11e9),
          knee (x) bitwise == x (4 threads)
      U5b shape over (T, 2^20]: monotone, y <= x, max y == float (A) (<= A,
          R34); NaN -> NaN; +-inf -> +-A
      U5c finite-difference slope (step >= 1e-3) <= 1 over (0, 8] (2-ulp
          rounding allowance per step); slope at T+ (step 1e-3) >= 0.999
      U6a D10 svfMake == 1.0f bitwise for res on a 1e-4 grid over [0, 0.3083]
          at 44.1 / 48 / 96 / 192 k; the first grid res with m < 1 is <= 31 %
          (m is continuous: just past 0.3083 it rounds to 1.0f in float)
      U6b LP / HP steady-sine peak (sine at the mapped peak) at res 100 % =
          +10.0 dB +-0.1 at fc 1 kHz and 0.45·fs × 44.1 / 48 / 96 / 192 k
      U6c raw (make 1) LP peak at res 50 / 80 / 90 / 100 % vs P(k) within
          +-0.01 dB; m == 1/sqrt (P) within 1 ulp
      (U7, U8 — T6; FB1–FB5 — T7: see below)

    Row-name note: 2.2 already owns F1–F5 (FUNC-03), so the PLAN's feedback
    rows F1–F5 are named FB1–FB5 here (SUMMARY-2.4 deviation).

  ==============================================================================
*/
#pragma once

#include <atomic>
#include <thread>
#include "dsp/GrainFx.h"
#include "dsp/OutputLaw.h"

namespace { namespace eg24
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    using Grain = ogran::Grain;
    constexpr double kFs = 48000.0;
    constexpr double kPi = 3.14159265358979323846;

    double dB20 (double a) { return a > 0.0 ? 20.0 * std::log10 (a) : -400.0; }

    std::uint32_t bitsOf (float x) { std::uint32_t b; std::memcpy (&b, &x, 4); return b; }
    float fromBits (std::uint32_t b) { float x; std::memcpy (&x, &b, 4); return x; }

    // Runs f (lo, hi) over [0, n) split into `threads` contiguous chunks.
    void parallelRange (std::uint64_t n, int threads, const std::function<void (std::uint64_t, std::uint64_t, int)>& f)
    {
        std::vector<std::thread> th;
        for (int t = 0; t < threads; ++t)
        {
            const std::uint64_t lo = n * (std::uint64_t) t / (std::uint64_t) threads;
            const std::uint64_t hi = n * (std::uint64_t) (t + 1) / (std::uint64_t) threads;
            th.emplace_back ([&f, lo, hi, t] { f (lo, hi, t); });
        }
        for (auto& x : th) x.join();
    }

    //==========================================================================
    // U5 — D9 knee (R34)
    //==========================================================================
    void kneeUnitRows()
    {
        std::printf ("\n-- U5. D9 wet knee (identity <= T, ceiling A, C1)\n");
        const float T = ogran::kKneeT, A = ogran::kKneeA;

        // U5a — exhaustive identity, both signs.
        {
            const std::uint32_t tb = bitsOf (T);
            std::atomic<std::uint64_t> bad { 0 };
            std::atomic<std::uint32_t> firstBad { 0xFFFFFFFFu };
            const std::uint64_t n = 2ull * ((std::uint64_t) tb + 1ull);
            parallelRange (n, 4, [&] (std::uint64_t lo, std::uint64_t hi, int)
            {
                std::uint64_t b = 0;
                for (std::uint64_t k = lo; k < hi; ++k)
                {
                    const std::uint32_t mag = (std::uint32_t) (k >> 1);
                    const std::uint32_t pat = mag | ((k & 1u) ? 0x80000000u : 0u);
                    const float x = fromBits (pat);
                    const float y = ogran::wetKnee (x);
                    if (bitsOf (y) != pat)
                    {
                        ++b;
                        std::uint32_t e = 0xFFFFFFFFu;
                        firstBad.compare_exchange_strong (e, pat);
                    }
                }
                bad += b;
            });
            row (bad.load() == 0, "U5a knee identity, every float |x| <= T both signs, bitwise",
                 juce::String::formatted ("%llu floats, %llu non-identical", (unsigned long long) n, (unsigned long long) bad.load())
                 + (bad.load() ? juce::String::formatted (" (first 0x%08x)", firstBad.load()) : juce::String()));
        }

        // U5b — shape over (T, 2^20].
        {
            const std::uint32_t lo = bitsOf (T) + 1u, hi = bitsOf (1048576.0f);
            const int th = 4;
            std::vector<std::uint64_t> mono (th, 0), above (th, 0);
            std::vector<float> mx (th, 0.0f), firstY (th, 0.0f), lastY (th, 0.0f);
            parallelRange ((std::uint64_t) (hi - lo + 1u), th, [&] (std::uint64_t a, std::uint64_t b, int t)
            {
                float prev = -1.0f, m = 0.0f;
                std::uint64_t nm = 0, na = 0;
                for (std::uint64_t k = a; k < b; ++k)
                {
                    const float x = fromBits (lo + (std::uint32_t) k);
                    const float y = ogran::wetKnee (x);
                    if (k == a) firstY[(size_t) t] = y;
                    if (k > a && y < prev) ++nm;
                    if (y > x) ++na;
                    m = juce::jmax (m, y);
                    prev = y;
                }
                lastY[(size_t) t] = prev;
                mono[(size_t) t] = nm; above[(size_t) t] = na; mx[(size_t) t] = m;
            });
            std::uint64_t nm = 0, na = 0; float m = 0.0f;
            for (int t = 0; t < th; ++t)
            {
                nm += mono[(size_t) t]; na += above[(size_t) t]; m = juce::jmax (m, mx[(size_t) t]);
                if (t > 0 && firstY[(size_t) t] < lastY[(size_t) t - 1]) ++nm;   // chunk seams
            }
            const float yn = ogran::wetKnee (std::numeric_limits<float>::quiet_NaN());
            const float yp = ogran::wetKnee (std::numeric_limits<float>::infinity());
            const float ym = ogran::wetKnee (-std::numeric_limits<float>::infinity());
            const bool ends = std::isnan (yn) && bitsOf (yp) == bitsOf (A) && bitsOf (ym) == bitsOf (-A);
            row (nm == 0 && na == 0 && bitsOf (m) == bitsOf (A) && ends,
                 "U5b knee (T, 2^20]: monotone, y <= x, max y == float(A); NaN, +-inf",
                 juce::String::formatted ("%u floats, %llu non-monotone, %llu y > x, max %.9g (A %.9g), NaN->%s, +inf->%.9g, -inf->%.9g",
                                          hi - lo + 1u, (unsigned long long) nm, (unsigned long long) na, (double) m, (double) A,
                                          std::isnan (yn) ? "NaN" : "num", (double) yp, (double) ym));
        }

        // U5c — slope.
        {
            double worst = 0.0, worstAllow = 0.0; float at = 0.0f; int n = 0, bad = 0;
            for (int k = 0; k < 8000; ++k)
            {
                const float a = 1.0e-3f * (float) k, b = 1.0e-3f * (float) (k + 1);
                const double h = (double) b - (double) a;
                const float ya = ogran::wetKnee (a), yb = ogran::wetKnee (b);
                const double s = ((double) yb - (double) ya) / h;
                const double ulp = (double) std::nextafter (yb, 4.0f) - (double) yb;
                const double allow = 1.0 + 2.0 * ulp / h;
                ++n;
                if (s > allow) ++bad;
                if (s - allow > worst - worstAllow || k == 0) { worst = s; worstAllow = allow; at = b; }
            }
            const float t1 = std::nextafter (T, 2.0f);
            const double sT = ((double) ogran::wetKnee (t1 + 1.0e-3f) - (double) ogran::wetKnee (t1)) / ((double) (t1 + 1.0e-3f) - (double) t1);
            row (bad == 0 && sT >= 0.999, "U5c knee slope <= 1 over (0, 8] (step 1e-3); slope at T+ >= 0.999",
                 juce::String::formatted ("%d steps, %d over (worst %.7f at %.3f, allowance %.7f); slope at T+ %.6f", n, bad, worst, (double) at, worstAllow, sT));
        }
    }

    //==========================================================================
    // U6 — D10 LP / HP half make-down (R33)
    //==========================================================================
    Grain svfGrainMake (int mode, double fc, double res, double fs, bool rawMake = false)
    {
        Grain g {};
        const auto s = ogran::fx::svfLatch (fc, res, fs);
        g.fxActive = ogran::kFxSvf; g.svfMode = mode;
        g.svfK = s.k; g.svfA1 = s.a1; g.svfA2 = s.a2; g.svfA3 = s.a3;
        g.svfMake = rawMake ? 1.0f : s.make;
        return g;
    }

    double sineAmp (const std::vector<float>& y, size_t from, size_t to, double f, double fs)
    {
        double ss = 0, cc = 0, sc = 0, ys = 0, yc = 0;
        for (size_t i = from; i < to; ++i)
        {
            const double ph = 2.0 * kPi * f * (double) i / fs;
            const double s = std::sin (ph), c = std::cos (ph);
            ss += s * s; cc += c * c; sc += s * c;
            ys += (double) y[i] * s; yc += (double) y[i] * c;
        }
        const double det = ss * cc - sc * sc;
        const double a = (ys * cc - yc * sc) / det, b = (yc * ss - ys * sc) / det;
        return std::sqrt (a * a + b * b);
    }

    double grainGain (Grain g, double fs, double f, double secs = 2.0, double fitSecs = 0.5)
    {
        const size_t n = (size_t) (secs * fs);
        std::vector<float> l (n), r (n);
        for (size_t i = 0; i < n; ++i) l[i] = r[i] = (float) (0.5 * std::sin (2.0 * kPi * f * (double) i / fs));
        for (size_t i = 0; i < n; i += ogran::kCell)
            ogran::fx::svf (g, l.data() + i, r.data() + i, (int) juce::jmin ((size_t) ogran::kCell, n - i));
        return sineAmp (l, n - (size_t) (fitSecs * fs), n, f, fs) / 0.5;
    }

    // Digital frequency of the analog resonance (prewarped bilinear map):
    // LP peaks at Ω = sqrt (1 − k²/2), HP at 1/Ω (relative to the prewarped fc).
    double peakHz (int mode, double fcLatched, double k, double fs)
    {
        const double om = std::sqrt (1.0 - 0.5 * k * k);
        const double w  = mode == ogran::kSvfLP ? om : 1.0 / om;
        return fs / kPi * std::atan (w * std::tan (kPi * fcLatched / fs));
    }

    double analyticP (double k) { return k < std::sqrt (2.0) ? 1.0 / (k * std::sqrt (1.0 - 0.25 * k * k)) : 1.0; }

    void d10UnitRows()
    {
        std::printf ("\n-- U6. D10 LP/HP half make-down (m = 1/sqrt P(k), R33)\n");
        const double fsList[4] { 44100.0, 48000.0, 96000.0, 192000.0 };

        // U6a
        {
            int notOne = 0, n = 0; juce::String at;
            for (double fs : fsList)
                for (int i = 0; i <= 3083; ++i)
                {
                    const double res = 1.0e-4 * i;
                    const float m = ogran::fx::svfLatch (1000.0, res, fs).make;
                    ++n;
                    if (bitsOf (m) != bitsOf (1.0f)) { if (notOne++ == 0) at = juce::String::formatted ("res %.4f fs %g m %.9g", res, fs, (double) m); }
                }
            // Above the threshold m is continuous (m → 1 as k → √2), so the first
            // few 1e-4 steps past 0.3083 still ROUND to 1.0f (m = 1 − ~5e-8 at
            // 0.3084). The sub-check: the first grid res with m < 1 is <= 31 %
            // (RESEARCH: m = 0.999995 there) at every fs.
            double firstBelow = 0.0;
            for (double fs : fsList)
            {
                double r = 0.3083;
                while (r < 0.32 && ! (ogran::fx::svfLatch (1000.0, r, fs).make < 1.0f)) r += 1.0e-4;
                firstBelow = juce::jmax (firstBelow, r);
            }
            row (notOne == 0 && firstBelow <= 0.31, "U6a svfMake == 1.0f bitwise, res [0, 0.3083] (1e-4 grid) x 4 fs; m < 1 by res 31 %",
                 juce::String::formatted ("%d cases, %d != 1; first res with m < 1: %.4f (m %.9g)", n, notOne, firstBelow,
                                          (double) ogran::fx::svfLatch (1000.0, firstBelow, 48000.0).make) + (notOne ? " (" + at + ")" : juce::String()));
        }

        // U6b
        {
            double worst = 0.0; juce::String at; int n = 0;
            for (double fs : fsList)
                for (double fc0 : { 1000.0, -1.0 })
                    for (int mode : { ogran::kSvfLP, ogran::kSvfHP })
                    {
                        const double fc = fc0 < 0.0 ? ogran::kFxNyquist * fs : fc0;
                        const auto lt = ogran::fx::svfLatch (fc, 1.0, fs);
                        const double f = peakHz (mode, (double) lt.fcHz, (double) lt.k, fs);
                        const double d = dB20 (grainGain (svfGrainMake (mode, fc, 1.0, fs), fs, f));
                        ++n;
                        if (std::abs (d - 10.0) > std::abs (worst - 10.0) || n == 1)
                        { worst = d; at = juce::String::formatted ("fs %g fc %g mode %d f %.1f", fs, fc, mode, f); }
                    }
            row (std::abs (worst - 10.0) <= 0.1, "U6b LP/HP peak at res 100 % = +10.0 dB +-0.1 (fc 1 k / 0.45 fs x 4 fs)",
                 juce::String::formatted ("%d cases, worst %+.4f dB (", n, worst) + at + ")");
        }

        // U6c
        {
            double worst = 0.0; juce::String s; int ulpBad = 0;
            for (double res : { 0.5, 0.8, 0.9, 1.0 })
            {
                const auto lt = ogran::fx::svfLatch (1000.0, res, kFs);
                const double k = (double) lt.k;
                const double f = peakHz (ogran::kSvfLP, (double) lt.fcHz, k, kFs);
                const double meas = dB20 (grainGain (svfGrainMake (ogran::kSvfLP, 1000.0, res, kFs, true), kFs, f));
                const double ana  = dB20 (analyticP (k));
                worst = juce::jmax (worst, std::abs (meas - ana));
                const float mRef = (float) (1.0 / std::sqrt (analyticP (k)));
                const auto du = (long) bitsOf (lt.make) - (long) bitsOf (mRef);
                if (std::abs (du) > 1) ++ulpBad;
                s << juce::String::formatted ("res %d%% %+.3f (P %+.3f) ", (int) (res * 100), meas, ana);
            }
            row (worst <= 0.01 && ulpBad == 0, "U6c raw LP peak vs P(k) +-0.01 dB (res 50/80/90/100 %); m == 1/sqrt P (1 ulp)",
                 juce::String::formatted ("worst %.4f dB, %d m off > 1 ulp; ", worst, ulpBad) + s);
        }
    }
}} // namespace eg24

static void engineRows24()
{
    eg24::d10UnitRows();
    eg24::kneeUnitRows();
}
