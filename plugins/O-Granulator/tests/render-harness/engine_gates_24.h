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
      U7a fb HP / LP (k = kFbK) vs the bilinear oracle ±0.05 dB, peak <= 1 + 1e-6,
          LP clamp 0.45·fs;  U7b diffuser lengths / allpass |H| / mix law;
          U7c D12 fbSat;  U7d AGC;  U7e R36 canonical +0;  U7f NaN / Inf scrub
      U8  tap law 1/(S1 + 1): peak Σw <= S1 + max w (the unimodal theorem) and
          0.95·peak·g_fb < 0 dB over 65 steps × 3 tilts × 200 N;  U8n ARCH law
          at Tukey 0.25 >= +4 dB (PLAN's −0.16 dB holds only for N >= 1: INFO)
      FB1 recirculation;  FB2a–d HP / LP / saturation / diffusion change the loop
      FB3a linear decay at FX gain <= 1 (16 corners, N 1.25);  FB3b tone /
          diffusion / pitch extremes;  FB3n ARCH-law control grows
      FB4 FX-gain corners bounded;  FB5 tap: no pan, muted streams feed 0

    Row-name note: 2.2 already owns F1–F5 (FUNC-03), so the PLAN's feedback
    rows F1–F5 are named FB1–FB5 here (SUMMARY-2.4 deviation).

  ==============================================================================
*/
#pragma once

#include <atomic>
#include <thread>
#include "dsp/GrainFx.h"
#include "dsp/OutputLaw.h"
#include "dsp/FeedbackChain.h"

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
    //==========================================================================
    // U7 — feedback chain stage laws (T6)
    //==========================================================================
    double tickGain (bool hpMode, const ogran::TptCoeffs& c, double fs, double f, double secs = 2.0, double fitSecs = 0.5)
    {
        const size_t n = (size_t) (secs * fs);
        std::vector<float> y (n);
        float ic1 = 0.0f, ic2 = 0.0f;
        for (size_t i = 0; i < n; ++i)
        {
            const float x = (float) (0.5 * std::sin (2.0 * kPi * f * (double) i / fs));
            y[i] = hpMode ? ogran::FeedbackChain::hpTick (ic1, ic2, c, x) : ogran::FeedbackChain::lpTick (ic1, ic2, c, x);
        }
        return sineAmp (y, n - (size_t) (fitSecs * fs), n, f, fs) / 0.5;
    }

    double bilinear (bool hpMode, double fcLatched, double k, double fs, double f)
    {
        const double om = std::tan (kPi * f / fs) / std::tan (kPi * fcLatched / fs);
        const std::complex<double> D (1.0 - om * om, k * om);
        return std::abs ((hpMode ? std::complex<double> (-om * om, 0.0) : std::complex<double> (1.0, 0.0)) / D);
    }

    void chainUnitRows()
    {
        std::printf ("\n-- U7. feedback chain stages (TPT k = kFbK, diffuser, D12, AGC, R36, scrub)\n");
        const double k = (double) ogran::kFbK;

        // U7a
        {
            double worst = 0.0, peak = 0.0; juce::String at; int n = 0;
            for (double fs : { 44100.0, 48000.0, 192000.0 })
                for (int hpMode = 1; hpMode >= 0; --hpMode)
                    for (double fc0 : hpMode ? std::vector<double> { 20.0, 80.0, 2000.0 } : std::vector<double> { 1000.0, 12000.0, -1.0 })
                    {
                        const double fc = fc0 < 0.0 ? 0.45 * fs : fc0;
                        const auto c = ogran::tptCoeffs (fc, k, fs);
                        const double fcL = juce::jlimit (ogran::kFxMinHz, ogran::kFbNyquist * fs, fc);
                        for (int j = -16; j <= 16; ++j)
                        {
                            const double f = fcL * std::exp2 (0.125 * j);
                            if (f >= 0.49 * fs || f < 5.0) continue;
                            const double h = bilinear (hpMode != 0, fcL, k, fs, f);
                            const double secs = juce::jmax (2.0, 40.0 / f);
                            const double g = tickGain (hpMode != 0, c, fs, f, secs, juce::jmax (0.5, 20.0 / f));
                            peak = juce::jmax (peak, g);
                            if (dB20 (h) <= -40.0) continue;
                            const double d = dB20 (g) - dB20 (h);
                            ++n;
                            if (std::abs (d) > std::abs (worst)) { worst = d; at = juce::String::formatted ("fs %g %s fc %g f %.1f", fs, hpMode ? "HP" : "LP", fc, f); }
                        }
                    }
            const auto cl = ogran::tptCoeffs (20000.0, k, 44100.0), cr = ogran::tptCoeffs (0.45 * 44100.0, k, 44100.0);
            const bool clampOk = bitsOf (cl.a1) == bitsOf (cr.a1) && bitsOf (cl.a2) == bitsOf (cr.a2) && bitsOf (cl.a3) == bitsOf (cr.a3);
            row (std::abs (worst) <= 0.05 && peak <= 1.0 + 1.0e-6 && clampOk && n > 100,
                 "U7a fb HP/LP (k = kFbK) vs bilinear oracle +-0.05 dB; peak |H| <= 1 + 1e-6; LP clamp 0.45 fs",
                 juce::String::formatted ("%d cases, worst %+.5f dB (", n, worst) + at + juce::String::formatted ("), peak %.8f, clamp %s", peak, clampOk ? "ok" : "BAD"));
        }

        // U7b
        {
            const auto d48 = ogran::diffuserLengths (48000.0), d44 = ogran::diffuserLengths (44100.0);
            const int t48[4] { 226, 398, 667, 1042 }, t44[4] { 207, 366, 613, 957 };
            bool lens = true;
            for (int s = 0; s < 4; ++s) lens = lens && d48.n[s] == t48[s] && d44.n[s] == t44[s];
            double worst = 0.0;
            for (int s = 0; s < 4; ++s)
            {
                const int M = d48.n[s];
                std::vector<float> line ((size_t) M, 0.0f);
                const size_t N = 1u << 17;
                std::vector<std::complex<double>> a (N);
                int p = 0;
                for (size_t i = 0; i < N; ++i)
                {
                    a[i] = ogran::allpassStep (line.data(), p, i == 0 ? 1.0f : 0.0f);
                    if (++p >= M) p = 0;
                }
                eg::fft (a);
                for (size_t b = 0; b < N / 2; ++b) worst = juce::jmax (worst, std::abs (20.0 * std::log10 (std::abs (a[b]))));
            }
            // Mix law through the chain's own stage vs a hand cascade; m = 0 bitwise x (signed zeros too).
            auto ch = std::make_unique<ogran::FeedbackChain>(); ch->prepare (48000.0);
            auto c0 = std::make_unique<ogran::FeedbackChain>(); c0->prepare (48000.0);
            std::vector<float> ref[2][4];
            for (int c = 0; c < 2; ++c) for (int s = 0; s < 4; ++s) ref[c][s].assign ((size_t) d48.n[s], 0.0f);
            int rp[4] {};
            int mixBad = 0, zeroBad = 0;
            const float m = 0.37f;
            for (int i = 0; i < 48000; ++i)
            {
                const float xl = eg::hashNoise (i, 0, 0.5f), xr = (i % 7 == 0) ? -0.0f : eg::hashNoise (i, 1, 0.5f);
                float al = xl, ar = xr;
                for (int s = 0; s < 4; ++s)
                {
                    al = ogran::allpassStep (ref[0][s].data(), rp[s], al);
                    ar = ogran::allpassStep (ref[1][s].data(), rp[s], ar);
                    if (++rp[s] >= d48.n[s]) rp[s] = 0;
                }
                const float el = (1.0f - m) * xl + m * al, er = (1.0f - m) * xr + m * ar;
                float yl = xl, yr = xr;
                ch->stepDiffuser (yl, yr, m);
                if (bitsOf (yl) != bitsOf (el) || bitsOf (yr) != bitsOf (er)) ++mixBad;
                float zl = xl, zr = xr;
                c0->stepDiffuser (zl, zr, 0.0f);
                if (bitsOf (zl) != bitsOf (xl) || bitsOf (zr) != bitsOf (xr)) ++zeroBad;
            }
            row (lens && worst <= 1.0e-4 && mixBad == 0 && zeroBad == 0,
                 "U7b diffuser: lengths (kDiffuserMs) == tables @ 44.1/48 k; |H| = 1 +-1e-4 dB; mix law exact; m 0 bitwise",
                 juce::String::formatted ("48k %d/%d/%d/%d, 44.1k %d/%d/%d/%d; worst |H| %.2e dB; mix %d bad, m0 %d bad",
                                          d48.n[0], d48.n[1], d48.n[2], d48.n[3], d44.n[0], d44.n[1], d44.n[2], d44.n[3], worst, mixBad, zeroBad));
        }

        // U7c
        {
            bool ok = true; double worstSlope = 0.0; juce::String bad;
            const bool d1 = bitsOf (1.0f + ogran::kSatSpan * 0.0f) == bitsOf (1.0f);
            for (float d : { 1.0f, 2.4f, 4.5f, 8.0f })
            {
                const double s = (double) ogran::fbSat (1.0e-6f, d) / 1.0e-6;
                worstSlope = juce::jmax (worstSlope, std::abs (s - 1.0));
                float prev = -1.0e30f;
                for (int i = -20000; i <= 20000; ++i)
                {
                    const float x = 0.005f * (float) i;
                    const float y = ogran::fbSat (x, d);
                    if (std::abs (y) > 1.0f / d * (1.0f + 1.0e-6f) || y < prev) { ok = false; if (bad.isEmpty()) bad = juce::String::formatted ("d %g x %g", d, x); }
                    prev = y;
                }
            }
            row (ok && d1 && worstSlope <= 1.0e-6, "U7c D12 fbSat: slope 1 at 0 (1e-6), |y| <= 1/d, monotone; d == 1 at sat 0",
                 juce::String::formatted ("worst slope err %.2e", worstSlope) + (bad.isEmpty() ? juce::String() : " bad " + bad));
        }

        // U7d
        {
            const double fs = 48000.0;
            auto c = std::make_unique<ogran::FeedbackChain>(); c->prepare (fs);
            float gainEnd = 0.0f; int cross63 = -1;
            for (int i = 0; i < (int) fs; ++i)
            {
                float l = 1.0f, r = 1.0f;
                c->stepAgc (l, r);
                if (cross63 < 0 && c->env >= (float) (1.0 - std::exp (-1.0))) cross63 = i + 1;
                gainEnd = l;
            }
            int crossRel = -1;
            for (int i = 0; i < (int) (2.0 * fs); ++i)
            {
                float l = 0.0f, r = 0.0f;
                c->stepAgc (l, r);
                if (crossRel < 0 && c->env <= (float) std::exp (-1.0)) crossRel = i + 1;
            }
            auto z = std::make_unique<ogran::FeedbackChain>(); z->prepare (fs);
            float zl = 0.0f, zr = -0.0f;
            z->stepAgc (zl, zr);
            const bool zeroOk = std::isfinite (zl) && std::isfinite (zr) && zl == 0.0f && zr == 0.0f;
            const int attWant = (int) std::lround (ogran::kAgcAttackS * fs), relWant = (int) std::lround (ogran::kAgcReleaseS * fs);
            row (std::abs (gainEnd - 0.7f) <= 0.001f && std::abs (cross63 - attWant) <= 1 && std::abs (crossRel - relWant) <= relWant / 100 && zeroOk,
                 "U7d AGC: 0 dBFS settles to 0.7 +-0.001; 63 % attack 1 ms +-1 smp; release 400 ms +-1 %; env 0 -> gain 1",
                 juce::String::formatted ("gain %.5f, attack %d smp (want %d), release %d smp (want %d), env-0 %s", (double) gainEnd, cross63, attWant,
                                          crossRel, relWant, zeroOk ? "ok" : "BAD"));
        }

        // U7e — R36: ±0 in → +0 stored, at feedback 0 and at a live knob.
        {
            int bad = 0, n = 0;
            for (float knobV : { 0.0f, 0.95f })
            {
                auto c = std::make_unique<ogran::FeedbackChain>(); c->prepare (48000.0);
                c->setCutoffs (400.0, 3000.0);
                ogran::ExactRamp knob, diff, sat;
                knob.reset (knobV); diff.reset (0.7f); sat.reset (1.0f + ogran::kSatSpan * 0.8f);
                float l[ogran::kCell], r[ogran::kCell];
                for (int blk = 0; blk < 3000; ++blk)
                {
                    for (int i = 0; i < ogran::kCell; ++i) { l[i] = ((blk + i) & 1) ? -0.0f : 0.0f; r[i] = ((blk + i) % 3 == 0) ? 0.0f : -0.0f; }
                    c->process (l, r, ogran::kCell, 0, knob, diff, sat);
                    for (int i = 0; i < ogran::kCell; ++i) { n += 2; bad += (bitsOf (l[i]) != 0u) + (bitsOf (r[i]) != 0u); }
                }
            }
            row (bad == 0, "U7e R36: +-0 tap -> stored return is +0 (bit pattern 0), knob 0 and 0.95", juce::String::formatted ("%d/%d samples +0", n - bad, n));
        }

        // U7f — scrub.
        {
            int okCases = 0, cases = 0;
            const float bads[3] { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };
            for (float v : bads)
                for (int where = 0; where < 3; ++where)
                {
                    ++cases;
                    auto c = std::make_unique<ogran::FeedbackChain>(); c->prepare (48000.0);
                    c->setCutoffs (80.0, 12000.0);
                    ogran::ExactRamp knob, diff, sat;
                    knob.reset (0.95f); diff.reset (0.5f); sat.reset (2.4f);
                    float l[ogran::kCell], r[ogran::kCell];
                    for (int blk = 0; blk < 200; ++blk)
                    {
                        for (int i = 0; i < ogran::kCell; ++i) { l[i] = eg::hashNoise (blk * 32 + i, 0); r[i] = eg::hashNoise (blk * 32 + i, 1); }
                        c->process (l, r, ogran::kCell, 0, knob, diff, sat);
                    }
                    const bool warm = c->env > 0.0f && c->hpIc1[0] != 0.0f;
                    float xl = where != 1 ? v : 0.1f, xr = where != 0 ? v : 0.1f;
                    const auto before = c->scrubs;
                    c->process (&xl, &xr, 1, 0, knob, diff, sat);
                    bool zero = c->env == 0.0f;
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        zero = zero && c->hpIc1[ch] == 0.0f && c->hpIc2[ch] == 0.0f && c->lpIc1[ch] == 0.0f && c->lpIc2[ch] == 0.0f;
                        for (int s = 0; s < 4; ++s)
                            for (int i = 0; i < ogran::kDiffuserMaxLen; ++i) zero = zero && c->line[ch][s][i] == 0.0f;
                    }
                    if (warm && zero && bitsOf (xl) == 0u && bitsOf (xr) == 0u && c->scrubs == before + 1) ++okCases;
                }
            // Finite inputs never trip it: 10 s of full-scale noise, knob 0.95, diffusion 1, sat 0.
            auto c = std::make_unique<ogran::FeedbackChain>(); c->prepare (48000.0);
            c->setCutoffs (20.0, 20000.0);
            ogran::ExactRamp knob, diff, sat;
            knob.reset (0.95f); diff.reset (1.0f); sat.reset (1.0f);
            float l[ogran::kCell], r[ogran::kCell];
            for (int blk = 0; blk < 15000; ++blk)
            {
                for (int i = 0; i < ogran::kCell; ++i) { l[i] = eg::hashNoise (blk * 32 + i, 0, 1.0f); r[i] = eg::hashNoise (blk * 32 + i, 1, 1.0f); }
                c->process (l, r, ogran::kCell, 0, knob, diff, sat);
            }
            row (okCases == 9 && c->scrubs == 0, "U7f scrub: NaN/+Inf/-Inf on L/R/both -> 0 out, all state 0, scrubs +1; 10 s noise never trips",
                 juce::String::formatted ("%d/%d cases; finite run scrubs %lld", okCases, cases, (long long) c->scrubs));
        }
    }

    //==========================================================================
    // U8 — the bound tap law (R29, signed off): peak Σw · 1/(S1 + 1)
    //==========================================================================
    void tapLawRows (const ogran::WindowMorphBank& bank)
    {
        std::printf ("\n-- U8. feedback tap law g_fb = 1/(S1 + 1) (R29)\n");
        // One case: peak_t Σ_k w(t − kH) over a period, S1, and the window's own max.
        struct Case { double peak = 0.0, s1 = 0.0, g = 1.0, maxW = 0.0; };
        auto evalCase = [&bank] (int step, float skew, double N, bool arch)
        {
            ogran::WindowSel win;
            win.step = step;
            win.tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * skew);
            const float* tab = bank.table (step);
            Case c;
            c.s1 = ogran::fbTapS1 (N, 1.0e9, win, bank);
            c.g  = arch ? (double) ogran::fbTapGainArch (c.s1) : (double) ogran::fbTapGain (c.s1);
            auto w = [&] (double u) { return win.hann() ? (double) ogran::hannWindow (u) : (double) ogran::WindowMorphBank::read (tab, win.tilt, u); };
            for (int k = 0; k < 8192; ++k) c.maxW = juce::jmax (c.maxW, w ((double) k / 8192.0));
            const double L = 1.0, H = L / N;
            const double dt = L / 2048.0;
            const int nt = juce::jmax (1, (int) std::ceil (H / dt));
            for (int it = 0; it < nt; ++it)
            {
                const double t = (double) it * H / (double) nt;
                double sum = 0.0;
                for (double u = t; u < L; u += H)   // every grain alive at t: phase u = t + kH (k >= 0)
                    sum += w (u);
                c.peak = juce::jmax (c.peak, sum);
            }
            return c;
        };
        auto peakDb = [&evalCase] (int step, float skew, double N, bool arch)
        {
            const auto c = evalCase (step, skew, N, arch);
            return 20.0 * std::log10 (c.peak * c.g);
        };

        // U8: every step × tilt {−1, 0, 1} × N on a 200-point log grid — 4 threads.
        // Gate = the unimodal theorem the law rests on (R29): peak Σw <= S1 + max w,
        // and with the 0.95 knob every generation decays: 0.95·peak·g_fb < 1.
        // PLAN-2.4's "peak·g <= −0.16 dB" cannot hold at small N (N 0.05 Expo:
        // S1 0.0103 → 1/(S1 + 1) alone is −0.089 dB) — reported as INFO, erratum.
        std::vector<double> Ns (200);
        for (int i = 0; i < 200; ++i) Ns[(size_t) i] = 0.05 * std::pow (40.0 / 0.05, i / 199.0);
        std::vector<std::pair<int, float>> combos;
        for (int st = 0; st < ogran::kWindowSteps; ++st) for (float sk : { -1.0f, 0.0f, 1.0f }) combos.push_back ({ st, sk });
        std::vector<double> worst (4, -400.0), worstExcess (4, -1.0e9), worstMaxW (4, 0.0), worstN1 (4, -400.0);
        std::vector<juce::String> where (4), whereEx (4);
        parallelRange (combos.size(), 4, [&] (std::uint64_t lo, std::uint64_t hi, int t)
        {
            for (auto c = lo; c < hi; ++c)
                for (double N : Ns)
                {
                    const auto r = evalCase (combos[(size_t) c].first, combos[(size_t) c].second, N, false);
                    const double d = 20.0 * std::log10 (ogran::kFbKnob * r.peak * r.g);
                    const double ex = r.peak - (r.s1 + r.maxW);
                    worstMaxW[(size_t) t] = juce::jmax (worstMaxW[(size_t) t], r.maxW);
                    if (N >= 1.0) worstN1[(size_t) t] = juce::jmax (worstN1[(size_t) t], 20.0 * std::log10 (r.peak * r.g));
                    if (d > worst[(size_t) t]) { worst[(size_t) t] = d; where[(size_t) t] = juce::String::formatted ("step %d skew %+.0f N %.3f", combos[(size_t) c].first, (double) combos[(size_t) c].second, N); }
                    if (ex > worstExcess[(size_t) t]) { worstExcess[(size_t) t] = ex; whereEx[(size_t) t] = juce::String::formatted ("step %d skew %+.0f N %.3f", combos[(size_t) c].first, (double) combos[(size_t) c].second, N); }
                }
        });
        double w = -400.0, ex = -1.0e9, mw = 0.0, wN1 = -400.0; juce::String at, atEx;
        for (int t = 0; t < 4; ++t)
        {
            if (worst[(size_t) t] > w) { w = worst[(size_t) t]; at = where[(size_t) t]; }
            if (worstExcess[(size_t) t] > ex) { ex = worstExcess[(size_t) t]; atEx = whereEx[(size_t) t]; }
            mw = juce::jmax (mw, worstMaxW[(size_t) t]); wN1 = juce::jmax (wN1, worstN1[(size_t) t]);
        }
        row (ex <= 1.0e-6 && w < 0.0, "U8 bound tap law 1/(S1 + 1): peak sum(w) <= S1 + max w; 0.95 * peak * g_fb < 0 dB (65 steps x 3 tilts x 200 N)",
             juce::String::formatted ("theorem excess worst %+.2e (", ex) + atEx + juce::String::formatted ("); per-generation worst %+.3f dB (", w) + at
                 + juce::String::formatted ("); bank max w %.5f", mw));
        std::printf ("  [INFO] U8 peak*g_fb (no 0.95): worst over N >= 1 %+.3f dB (PLAN threshold -0.16 dB; N < 1 approaches 0 dB as S1 -> 0)\n", wN1);

        // U8n: the ARCH law 1/max (1, S1) at Tukey 0.25 (steps 20–23, skew 0) must exceed +4 dB.
        double wa = -400.0; juce::String atA;
        for (int st = 20; st <= 23; ++st)
            for (double N : Ns)
            {
                const double d = peakDb (st, 0.0f, N, true);
                if (d > wa) { wa = d; atA = juce::String::formatted ("step %d N %.3f", st, N); }
            }
        row (wa >= 4.0, "U8n ARCH law 1/max(1, S1) at Tukey 0.25: peak >= +4.0 dB (U8 discriminates)",
             juce::String::formatted ("worst %+.3f dB (", wa) + atA + ")");
    }
    //==========================================================================
    // FB — feedback network rows (T7). 2.2 owns F1–F5, hence FB1–FB5.
    //==========================================================================
    // −52 dBFS RMS uniform noise (amp = 10^(−52/20)·√3).
    constexpr float kNoise52 = 0.0043442f;

    std::vector<double> frameDb (const Out& o, double fs, double t0, double t1, double frameS = 0.01)
    {
        std::vector<double> y;
        const size_t fr = (size_t) (frameS * fs);
        for (size_t a = (size_t) (t0 * fs); a + fr <= (size_t) (t1 * fs) && a + fr <= o.L.size(); a += fr)
        {
            double e = 0.0;
            for (size_t i = a; i < a + fr; ++i) e += (double) o.L[i] * o.L[i] + (double) o.R[i] * o.R[i];
            e /= 2.0 * (double) fr;
            y.push_back (e > 0.0 ? 10.0 * std::log10 (e) : -400.0);
        }
        return y;
    }

    /** Decay slope (dB/s): LS fit of 10 ms frame levels over [t0, t1), stopping at
        the first frame below floorDb. Fewer than 3 frames above the floor: the
        drop from the first frame to the floor over the elapsed time (a decay too
        fast to fit is still a decay). */
    // frameS: 10 ms for short loops; ONE GENERATION for a long / phase-locked loop —
    // its level is the energy of a whole loop period (dominated by the strongest
    // phase), never the mean of per-phase dB values (that averages the log, so a
    // growing peak phase is outvoted by the decaying rest of the cycle).
    struct Fit { double slope = 0.0; int frames = 0; double firstDb = -400.0, lastDb = -400.0; };
    Fit decayFit (const Out& o, double fs, double t0, double t1, double floorDb = -240.0, double frameS = 0.01)
    {
        Fit f;
        const auto y = frameDb (o, fs, t0, t1, frameS);
        std::vector<double> xs, ys;
        for (size_t k = 0; k < y.size(); ++k)
        {
            if (y[k] < floorDb) break;
            xs.push_back (frameS * (double) k); ys.push_back (y[k]);
        }
        f.frames = (int) xs.size();
        if (! ys.empty()) { f.firstDb = ys.front(); f.lastDb = ys.back(); }
        if (xs.size() < 3)
        {
            f.slope = ys.empty() ? -1.0e6 : -(f.firstDb - floorDb) / (frameS * (double) (xs.size() + 1));
            return f;
        }
        double mx = 0, my = 0;
        for (size_t k = 0; k < xs.size(); ++k) { mx += xs[k]; my += ys[k]; }
        mx /= (double) xs.size(); my /= (double) xs.size();
        double sxy = 0, sxx = 0;
        for (size_t k = 0; k < xs.size(); ++k) { sxy += (xs[k] - mx) * (ys[k] - my); sxx += (xs[k] - mx) * (xs[k] - mx); }
        f.slope = sxy / sxx;
        return f;
    }

    // s1 only, wet only, spray 0 / jitter 0 / position 0 (the phase-locked corner).
    P fbBase (float feedback)
    {
        P p = eg22::base ({ 1 });
        for (auto& kv : P { { "feedback", feedback }, { "fb_diffusion", 0.0f }, { "s1_spray", 0.0f }, { "s1_jitter", 0.0f },
                            { "s1_position", 0.0f } })
            p.push_back (kv);
        return p;
    }

    std::function<float (juce::int64, int)> burstStim (double fs, double at, double len, float amp)
    {
        const juce::int64 a = (juce::int64) (at * fs), b = (juce::int64) ((at + len) * fs);
        return [a, b, amp] (juce::int64 i, int ch) { return (i >= a && i < b) ? eg::hashNoise (i, ch, amp) : 0.0f; };
    }
    std::function<float (juce::int64, int)> gatedNoise (double fs, double until, float amp)
    {
        const juce::int64 b = (juce::int64) (until * fs);
        return [b, amp] (juce::int64 i, int ch) { return i < b ? eg::hashNoise (i, ch, amp) : 0.0f; };
    }

    double bandDb (const Out& o, double fs, double t0, double t1, double loHz, double hiHz)
    {
        const size_t n = 1u << 15;
        double acc = 0.0; int segs = 0;
        for (size_t a = (size_t) (t0 * fs); a + n <= (size_t) (t1 * fs) && a + n <= o.L.size(); a += n / 2)
        {
            const auto p = eg::powerSpectrum (o.L, a, n);
            for (size_t k = 1; k < p.size(); ++k)
            {
                const double f = (double) k * fs / (double) n;
                if (f >= loHz && f < hiHz) acc += p[k];
            }
            ++segs;
        }
        return segs ? 10.0 * std::log10 (juce::jmax (1e-300, acc / segs)) : -400.0;
    }

    double crestDb (const std::vector<float>& x, size_t a, size_t b)
    {
        double e = 0.0, pk = 0.0;
        for (size_t i = a; i < b && i < x.size(); ++i) { e += (double) x[i] * x[i]; pk = juce::jmax (pk, (double) std::abs (x[i])); }
        const double rms = std::sqrt (e / (double) juce::jmax ((size_t) 1, b - a));
        return rms > 0.0 ? 20.0 * std::log10 (pk / rms) : 0.0;
    }

    // Normalised autocorrelation max over lags [l0, l1] of x[a, b) (mean removed).
    double acorrPeak (const std::vector<float>& x, size_t a, size_t b, int l0, int l1)
    {
        double mean = 0.0;
        for (size_t i = a; i < b; ++i) mean += x[i];
        mean /= (double) (b - a);
        double e = 0.0;
        for (size_t i = a; i < b; ++i) e += (x[i] - mean) * (x[i] - mean);
        double best = -1.0;
        for (int L = l0; L <= l1; ++L)
        {
            double c = 0.0;
            for (size_t i = a; i + (size_t) L < b; ++i) c += (x[i] - mean) * (x[i + (size_t) L] - mean);
            best = juce::jmax (best, c / juce::jmax (1e-300, e));
        }
        return best;
    }

    void feedbackRows()
    {
        std::printf ("\n-- FB. FUNC-05 feedback network (tap law 1/(S1+1), chain, decay scope R30)\n");

        // FB1 — recirculates. A long loop (position 1.25 % of 8 s → D ≈ 100 ms): at
        // position 0 the loop period is 42 samples and even a recirculating loop
        // is gone in microseconds of audio, so the window would read the floor.
        {
            auto job = [] (float fb)
            {
                Job j; j.len = (juce::int64) (2.0 * kFs);
                j.params = fbBase (fb);
                for (auto& kv : P { { "s1_density", 50.0f }, { "s1_size", 50.0f }, { "s1_position", 1.25f } }) j.params.push_back (kv);
                j.stim = burstStim (kFs, 1.0, 0.010, 0.1f * 1.7320508f);   // −20 dBFS RMS
                return j;
            };
            const auto on = eg::render (job (50.0f)), off = eg::render (job (0.0f));
            // The direct grains read the burst at 1.1 s (+ 10 ms burst + 50 ms grain);
            // after that only the loop's 2nd, 3rd … generations can sound.
            const double t0 = 1.0 + 0.100 + 0.010 + 0.050 + 0.030;
            const double eOn = eg22::db (eg22::meanPow (on, (size_t) (t0 * kFs), (size_t) ((t0 + 0.2) * kFs)));
            const double eOff = eg22::db (eg22::meanPow (off, (size_t) (t0 * kFs), (size_t) ((t0 + 0.2) * kFs)));
            row (eOn - eOff >= 20.0 && eOn > -200.0, "FB1 feedback 50 % recirculates: 200 ms after the direct grains >= 20 dB over fb 0",
                 juce::String::formatted ("fb 50 %% %.1f dB, fb 0 %.1f dB (delta %.1f)", eOn, eOff, eOn - eOff));
        }

        // FB2 — each chain stage changes the loop (burst at −6 dBFS, feedback 80 %).
        {
            auto job = [] (P extra)
            {
                Job j; j.len = (juce::int64) (3.0 * kFs);
                j.params = fbBase (80.0f);
                for (auto& kv : P { { "s1_density", 50.0f }, { "s1_size", 50.0f } }) j.params.push_back (kv);
                for (auto& kv : extra) j.params.push_back (kv);
                j.stim = burstStim (kFs, 1.0, 0.020, 0.5f * 1.7320508f);   // −6 dBFS RMS
                return j;
            };
            const double t0 = 1.1, t1 = 2.9;
            // Tone: a sustained long loop (position 5 % → D ≈ 400 ms, N 10, fb 100 %:
            // ≈ −2 dB per generation) under continuous −20 dBFS noise; the output is
            // the direct grains + the loop, and the fb filter acts once per generation.
            auto tone = [] (P extra)
            {
                Job j; j.len = (juce::int64) (6.0 * kFs);
                j.params = fbBase (100.0f);
                for (auto& kv : P { { "s1_density", 50.0f }, { "s1_size", 200.0f }, { "s1_position", 5.0f } }) j.params.push_back (kv);
                for (auto& kv : extra) j.params.push_back (kv);
                j.stim = eg::noiseStim (0.1f * 1.7320508f);
                return j;
            };
            // HP
            const auto h80 = eg::render (tone ({ { "fb_low_cut", 80.0f } })), h1k = eg::render (tone ({ { "fb_low_cut", 1000.0f } }));
            const double hd = bandDb (h80, kFs, 2.0, 6.0, 20.0, 500.0) - bandDb (h1k, kFs, 2.0, 6.0, 20.0, 500.0);
            row (hd >= 1.0, "FB2a fb HP 80 -> 1000 Hz (long loop, fb 100 %): energy < 500 Hz falls >= 1 dB", juce::String::formatted ("%.2f dB", hd));
            // LP
            const auto l12 = eg::render (tone ({ { "fb_high_cut", 12000.0f } })), l2 = eg::render (tone ({ { "fb_high_cut", 2000.0f } }));
            const double ld = bandDb (l12, kFs, 2.0, 6.0, 4000.0, 20000.0) - bandDb (l2, kFs, 2.0, 6.0, 4000.0, 20000.0);
            row (ld >= 1.0, "FB2b fb LP 12 k -> 2 kHz (long loop, fb 100 %): energy > 4 kHz falls >= 1 dB", juce::String::formatted ("%.2f dB", ld));
            // Saturation (D12): crest of the hot loop.
            const auto s0 = eg::render (job ({ { "fb_saturation", 0.0f } })), s1 = eg::render (job ({ { "fb_saturation", 100.0f } }));
            const size_t a = (size_t) (t0 * kFs), b = (size_t) (t1 * kFs);
            const double cd = std::abs (crestDb (s0.L, a, b) - crestDb (s1.L, a, b));
            const double sd = std::abs (eg22::db (eg22::meanPow (s0, a, b)) - eg22::db (eg22::meanPow (s1, a, b)));
            row (cd >= 0.5 || sd >= 1.0, "FB2c fb saturation 0 -> 100 %: loop crest moves >= 0.5 dB (or level >= 1 dB)",
                 juce::String::formatted ("crest %.2f dB, level %.2f dB", cd, sd));
            // Diffusion: echo density — the autocorrelation peak at the loop period.
            const auto d0 = eg::render (job ({ { "fb_diffusion", 0.0f } })), d1 = eg::render (job ({ { "fb_diffusion", 100.0f } }));
            const int D = ogran::Ring::margin (0) + 1, T = D + ogran::kFbDelay;
            const double r0 = acorrPeak (d0.L, a, a + (size_t) (0.5 * kFs), T - 2, T + 2);
            const double r1 = acorrPeak (d1.L, a, a + (size_t) (0.5 * kFs), T - 2, T + 2);
            row (r1 <= 0.5 * r0 && r0 > 0.2, "FB2d fb diffusion 0 -> 100 %: autocorrelation peak at the loop period falls >= 50 %",
                 juce::String::formatted ("loop period %d smp: rho %.3f -> %.3f", T, r0, r1));
        }

        // FB3 — linear decay at FX gain <= 1 (P35, signed off) + the ARCH-law control.
        {
            // Two loop regimes per corner: position 0 (T = margin + 1 + 32 = 42 samples
            // << H — the per-generation gain averages Σw(t) over the grain period) and
            // PHASE-LOCKED (D + 31 == H = fs/density: every generation meets the SAME
            // Σw phase — the R29 worst case, where the ARCH law grows at its peak).
            struct Corner { juce::String name; P extra; bool arch = false; bool shortInput = false; };
            std::vector<Corner> cs;
            const float wins[4] { 0.0f, 0.333f, 0.667f, 1.0f };
            const char* wn[4] { "Hann", "Tukey", "Trap", "Expo" };
            const double H = kFs / 12.5, span = 8.0 * kFs;
            // Loop period T = (D − 1) + 32: a grain at output time n reads ring index
            // n + 1 − D (the head is one past the newest sample), and the tap re-enters
            // the ring 32 samples later. Lock: D + 31 == H, so no drift per generation.
            const float lockPct = (float) (100.0 * (H - (ogran::kFbDelay - 1) - ogran::Ring::margin (0)) / span);
            for (int lock = 0; lock < 2; ++lock)
                for (int w = 0; w < 4; ++w)
                {
                    const juce::String tag = juce::String (wn[w]) + (lock ? "/lock" : "");
                    const float pos = lock ? lockPct : 0.0f;
                    cs.push_back ({ tag + "/Off", { { "s1_window", wins[w] }, { "s1_position", pos } } });
                    cs.push_back ({ tag + "/BP100", { { "s1_window", wins[w] }, { "s1_position", pos }, { "s1_filter_type", 2.0f }, { "s1_filter_res", 100.0f }, { "s1_filter_cutoff", 1000.0f } } });
                    cs.push_back ({ tag + "/LP30", { { "s1_window", wins[w] }, { "s1_position", pos }, { "s1_filter_type", 1.0f }, { "s1_filter_res", 30.0f }, { "s1_filter_cutoff", 1000.0f } } });
                    cs.push_back ({ tag + "/ring100", { { "s1_window", wins[w] }, { "s1_position", pos }, { "s1_ring_mix", 100.0f }, { "s1_ring_freq", 300.0f } } });
                }
            const size_t nA = cs.size();
            cs.push_back ({ "lowcut2k", { { "s1_window", 0.333f }, { "fb_low_cut", 2000.0f } } });
            cs.push_back ({ "highcut1k", { { "s1_window", 0.333f }, { "fb_high_cut", 1000.0f } } });
            cs.push_back ({ "diff100", { { "s1_window", 0.333f }, { "fb_diffusion", 100.0f } } });
            cs.push_back ({ "+12st", { { "s1_window", 0.333f }, { "s1_pitch", 12.0f } } });
            cs.push_back ({ "+24st", { { "s1_window", 0.333f }, { "s1_pitch", 24.0f } } });
            const size_t nB = cs.size();
            cs.push_back ({ "ARCH Tukey/lock", { { "s1_window", 0.333f }, { "s1_position", lockPct } }, true, true });

            std::vector<Job> jobs; std::vector<double> cost;
            for (auto& c : cs)
            {
                Job j; j.len = (juce::int64) (5.5 * kFs); j.block = 512;
                j.params = fbBase (100.0f);
                // N = 1.25: the R29 worst overlap for Tukey / Trapezoid (12.5 g/s × 100 ms).
                for (auto& kv : P { { "s1_density", 12.5f }, { "s1_size", 100.0f } }) j.params.push_back (kv);
                for (auto& kv : c.extra) j.params.push_back (kv);
                // The ARCH control: 0.3 s of input at −120 dBFS. It grows ≈ +3.3 dB per
                // generation (41 dB/s) at the locked peak phase, so a −52 dBFS input
                // reaches the AGC ceiling inside the fit window and sustains there.
                j.stim = gatedNoise (kFs, c.shortInput ? 0.3 : 3.0, c.shortInput ? kNoise52 * 2.5119e-4f : kNoise52);
                if (c.arch) j.pre = [] (Proc& p) { p.getEngineForTest().hooks.fbArchLaw = true; };
                jobs.push_back (j); cost.push_back (1.0);
            }
            std::vector<Fit> fits (cs.size());
            std::vector<double> gen (cs.size(), 0.0);
            eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o)
            {
                // After the input stops, the direct reads end within D + len (≤ 80 + 100 ms).
                bool locked = false;
                for (auto& kv : cs[i].extra) locked = locked || (kv.first == "s1_position" && kv.second > 0.0f);
                const double genS = H / kFs;   // one loop period (locked: T == H)
                fits[i] = cs[i].shortInput ? decayFit (o, kFs, 0.4, 1.92, -240.0, genS)
                                           : locked ? decayFit (o, kFs, 3.3, 5.3, -240.0, genS) : decayFit (o, kFs, 3.02, 5.02);
                const double dbar = locked ? H - ogran::kFbDelay : (double) ogran::Ring::margin (0) + 1.0;   // credit uses D − 1 + 32 ≈ D + 32
                double credit = 0.0;
                bool diff = false;
                for (auto& kv : cs[i].extra) diff = diff || (kv.first == "fb_diffusion" && kv.second > 0.0f);
                if (diff) { const auto L = ogran::diffuserLengths (kFs); for (int s = 0; s < 4; ++s) credit += L.n[s]; }
                gen[i] = fits[i].slope * (dbar + ogran::kFbDelay + credit) / kFs;
            });
            auto summarise = [&] (size_t from, size_t to, double& worst, juce::String& where)
            {
                int bad = 0; worst = -1.0e9;
                for (size_t i = from; i < to; ++i)
                {
                    if (fits[i].slope > -0.05) ++bad;
                    if (fits[i].slope > worst) { worst = fits[i].slope; where = cs[i].name; }
                }
                return bad;
            };
            double wA, wB; juce::String atA, atB;
            const int badA = summarise (0, nA, wA, atA), badB = summarise (nA, nB, wB, atB);
            juce::String info;
            for (size_t i = 0; i < nB; ++i) info << cs[i].name << juce::String::formatted (" %.4g/gen; ", gen[i]);
            std::printf ("  [INFO] FB3 dB/gen (credit = margin+1 + 32 + diffuser sum when diffused): %s\n", info.toRawUTF8());
            row (badA == 0, "FB3a linear decay, 4 windows x {Off, BP 100, LP 30, ring 100} x {pos 0, phase-locked}, fb 100 %, N 1.25: dB/s <= -0.05",
                 juce::String::formatted ("%d/32 decay; slowest %+.2f dB/s (", 32 - badA, wA) + atA + ")");
            row (badB == 0, "FB3b tone / diffusion extremes + pitch +12/+24 (Tukey): dB/s <= -0.05",
                 juce::String::formatted ("%d/5 decay; slowest %+.1f dB/s (", 5 - badB, wB) + atB + ")");
            const auto& fn = fits[nB];
            row (fn.slope > 0.0, "FB3n control: ARCH law 1/max(1, S1) at Tukey / N 1.25, phase-locked, grows (dB/s > 0)",
                 juce::String::formatted ("%+.2f dB/s (%d frames, %.1f -> %.1f dB)", fn.slope, fn.frames, fn.firstDb, fn.lastDb));
        }

        // FB4 — FX-gain corners: boundedness (R30).
        {
            struct Corner { juce::String name; P extra; };
            std::vector<Corner> cs {
                { "drive100", { { "s1_drive", 100.0f } } },
                { "LPres100", { { "s1_filter_type", 1.0f }, { "s1_filter_res", 100.0f }, { "s1_filter_cutoff", 1000.0f } } },
                { "HPres100", { { "s1_filter_type", 3.0f }, { "s1_filter_res", 100.0f }, { "s1_filter_cutoff", 1000.0f } } },
                { "crush100", { { "s1_crush", 100.0f } } } };
            std::vector<Job> jobs; std::vector<double> cost;
            for (auto& c : cs)
            {
                Job j; j.len = (juce::int64) (20.0 * kFs); j.block = 512;
                j.params = fbBase (100.0f);
                for (auto& kv : P { { "s1_density", 12.5f }, { "s1_size", 100.0f }, { "output_gain", 0.0f } }) j.params.push_back (kv);
                for (auto& kv : c.extra) j.params.push_back (kv);
                constexpr juce::int64 sw = (juce::int64) (10.0 * kFs);
                j.stim = [] (juce::int64 i, int ch) { return eg::hashNoise (i, ch, i < sw ? kNoise52 : 0.5f * 1.7320508f); };
                jobs.push_back (j); cost.push_back (1.0);
            }
            struct Res { double peak = 0.0; bool finite = true, sub = false; double env = 0.0; };
            std::vector<Res> rs (cs.size());
            const float A = ogran::kKneeA;
            // Chain state sampled after each 10 s section (after() runs on the worker with its own processor).
            std::vector<int> subs (cs.size(), 0);
            for (size_t i = 0; i < jobs.size(); ++i)
            {
                int* sp2 = &subs[i];
                jobs[i].after = [sp2] (Proc& p, juce::int64 pos)
                {
                    if (pos % (juce::int64) (10.0 * kFs) < 512)
                    {
                        const auto& c = p.getEngineForTest().getChainForTest();
                        auto sub = [] (float x) { return std::fpclassify (x) == FP_SUBNORMAL; };
                        for (int ch = 0; ch < 2; ++ch)
                            if (sub (c.hpIc1[ch]) || sub (c.hpIc2[ch]) || sub (c.lpIc1[ch]) || sub (c.lpIc2[ch])) ++*sp2;
                        if (sub (c.env)) ++*sp2;
                    }
                };
            }
            eg22b::renderPool (jobs, cost, [&] (size_t i, Out& o)
            {
                auto& r = rs[i];
                for (size_t k = 0; k < o.L.size(); ++k)
                {
                    const float l = o.L[k], rr = o.R[k];
                    if (! std::isfinite (l) || ! std::isfinite (rr)) r.finite = false;
                    r.peak = juce::jmax (r.peak, (double) std::abs (l), (double) std::abs (rr));
                }
                r.env = o.eng().getChainForTest().env;
            });
            bool ok = true; juce::String det;
            const float d = 1.0f + ogran::kSatSpan * 0.2f;   // fb_saturation default 20 %
            for (size_t i = 0; i < cs.size(); ++i)
            {
                const auto& r = rs[i];
                const bool good = r.finite && r.peak <= (double) A && subs[i] == 0 && r.env <= 0.7 + 1.0 / d;
                ok = ok && good;
                det << cs[i].name << juce::String::formatted (" pk %.3f env %.3f sub %d%s; ", r.peak, r.env, subs[i], good ? "" : " BAD");
            }
            row (ok, "FB4 FX-gain corners (drive / LP res / HP res / crush 100 %) at fb 100 %, 20 s: peak <= A, finite, no subnormal, env <= 0.7 + 1/d", det);
        }

        // FB5 — tap composition: no pan, muted / un-soloed / disabled streams feed 0.
        {
            constexpr juce::int64 len = (juce::int64) (2.0 * kFs);
            auto job = [] (P extra, std::vector<float>* probe)
            {
                Job j; j.len = len; j.block = 512;
                j.params = eg22::base ({ 1, 2 });
                for (auto& kv : P { { "feedback", 50.0f }, { "s1_pan", 100.0f }, { "s2_level", 6.0f } }) j.params.push_back (kv);
                for (auto& kv : extra) j.params.push_back (kv);
                j.stim = [] (juce::int64 i, int) { return eg::hashNoise (i, 0, 0.3f); };   // mono source: bus L == R
                probe->assign ((size_t) (2 * len), 0.0f);
                j.pre = [probe] (Proc& p) { p.getEngineForTest().hooks.tapProbe = probe->data(); p.getEngineForTest().hooks.tapProbeLen = len; };
                return j;
            };
            std::vector<float> pPan, pRef, pMute, pSolo, pDis;
            eg::render (job ({ { "s2_enable", 0.0f } }, &pRef));
            pPan = pRef;
            int lrDiff = 0; double tapPk = 0.0;
            for (juce::int64 i = 0; i < len; ++i)
            {
                if (std::memcmp (&pPan[(size_t) (2 * i)], &pPan[(size_t) (2 * i + 1)], 4) != 0) ++lrDiff;
                tapPk = juce::jmax (tapPk, (double) std::abs (pPan[(size_t) (2 * i)]));
            }
            eg::render (job ({ { "s2_mute", 1.0f } }, &pMute));
            eg::render (job ({ { "s1_solo", 1.0f } }, &pSolo));
            eg::render (job ({ { "s2_enable", 0.0f }, { "s2_level", 6.0f } }, &pDis));
            auto maxd = [] (const std::vector<float>& a, const std::vector<float>& b)
            {
                double m = 0.0;
                for (size_t i = 0; i < a.size(); ++i) m = juce::jmax (m, std::abs ((double) a[i] - (double) b[i]));
                return m;
            };
            const double dm = maxd (pMute, pRef), ds = maxd (pSolo, pRef), dd = maxd (pDis, pRef);
            row (lrDiff == 0 && tapPk > 1.0e-3 && dm == 0.0 && ds == 0.0 && dd == 0.0,
                 "FB5 tap: hard-panned s1 gives tap L == R; muted / un-soloed / disabled s2 (+6 dB) adds exactly 0",
                 juce::String::formatted ("L!=R %d (tap peak %.3f); max|d| mute %.3g solo %.3g disabled %.3g", lrDiff, tapPk, dm, ds, dd));
        }
    }

}} // namespace eg24

static void engineRows24()
{
    eg24::d10UnitRows();
    eg24::kneeUnitRows();
    eg24::chainUnitRows();
    {
        auto p = make();
        eg24::tapLawRows (p->getEngineForTest().getBank());
    }
    eg24::feedbackRows();
}
