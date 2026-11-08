/*
  ==============================================================================

    O-GranulatorInstrument — render harness, section FU (Stage 2 / Phase 2.3,
    PLAN-2.3 T7 / PD14): per-grain FX law units on Source/dsp/forked/GrainFx.h
    (namespace ogi::fx — a byte-for-byte ogran -> ogi rename of the parent's
    Source/dsp/GrainFx.h at 82406fce).

    Ported mechanically from O-Granulator @ 82406fce
    tests/render-harness/engine_gates_23.h :72-562 (helpers :74-137, U rows
    :140-562). Renames: ogran -> ogi, U* -> FU*, eg::hashNoise ->
    parity::hashNoise (salt 0 = bitwise the parent's hash), eg::rmsDb -> local
    fu23::rmsDb (same math). Criteria and thresholds verbatim. Additions:
      FU1b  inline negative control — the oracle without the D10 x make factor
            must deviate > 0.05 dB for LP/HP at a resonant setting
      FU1i  INFO prints the made-down peak next to the pre-D10 expectation
      FU3a  runs under its own juce::ScopedNoDenormals

    Included from main.cpp after engine_gates_21.h, parity_gates_22.h and
    engine_gates_22.h (row(), eg21::ctrl / eg21::f2, parity:: visible).

  ==============================================================================
*/
#pragma once

#include <cmath>
#include <complex>
#include <limits>
#include <vector>

#include "../parity/ParityRows.h"
#include "dsp/forked/GrainFx.h"

namespace { namespace fu23
{
    using Grain = ogi::Grain;
    constexpr double kFs = 48000.0;
    constexpr double kPi = 3.14159265358979323846;

    double dB20 (double a) { return a > 0.0 ? 20.0 * std::log10 (a) : -400.0; }

    // Local equivalent of the parent's eg::rmsDb (engine_gates.h @ 82406fce).
    double rmsDb (const std::vector<float>& x, size_t from, size_t to)
    {
        double acc = 0.0;
        for (size_t i = from; i < to && i < x.size(); ++i) acc += (double) x[i] * x[i];
        const double ms = acc / (double) juce::jmax ((size_t) 1, to - from);
        return ms > 0.0 ? 10.0 * std::log10 (ms) : -400.0;
    }

    // Least-squares amplitude of a known-frequency sinusoid over y[from, to).
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

    Grain svfGrain (int mode, double fc, double res, double fs)
    {
        Grain g {};
        const auto s = ogi::fx::svfLatch (fc, res, fs);
        g.fxActive = ogi::kFxSvf; g.svfMode = mode;
        g.svfK = s.k; g.svfA1 = s.a1; g.svfA2 = s.a2; g.svfA3 = s.a3;
        g.svfMake = s.make;   // 2.4 D10 (PLAN-2.4 P33): the production latch
        return g;
    }

    // Steady-state |H| (linear) of the grain's SVF for a sine at f.
    double svfGain (int mode, double fc, double res, double fs, double f, double secs = 2.0, double fitSecs = 0.5)
    {
        Grain g = svfGrain (mode, fc, res, fs);
        const size_t n = (size_t) (secs * fs);
        std::vector<float> l (n), r (n);
        for (size_t i = 0; i < n; ++i) l[i] = r[i] = (float) (0.5 * std::sin (2.0 * kPi * f * (double) i / fs));
        for (size_t i = 0; i < n; i += ogi::kCell)
            ogi::fx::svf (g, l.data() + i, r.data() + i, (int) juce::jmin ((size_t) ogi::kCell, n - i));
        return sineAmp (l, n - (size_t) (fitSecs * fs), n, f, fs) / 0.5;
    }

    // Prewarped bilinear oracle.
    double oracle (int mode, double fc, double k, double fs, double f)
    {
        const double om = std::tan (kPi * f / fs) / std::tan (kPi * fc / fs);
        const std::complex<double> D (1.0 - om * om, k * om);
        std::complex<double> N = mode == ogi::kSvfLP ? std::complex<double> (1.0, 0.0)
                               : mode == ogi::kSvfBP ? std::complex<double> (0.0, k * om)
                                                     : std::complex<double> (-om * om, 0.0);
        return std::abs (N / D);
    }

    //==========================================================================
    // FU1 — SVF
    //==========================================================================
    void svfUnitRows()
    {
        std::printf ("\n-- FU1. SVF units (TPT, BP = k*v1, D8 / R18)\n");
        const double fsList[4] { 44100.0, 48000.0, 96000.0, 192000.0 };

        // FU1a
        {
            double worst = 0.0; juce::String at; int n = 0;
            for (double fs : fsList)
                for (double fc0 : { 20.0, 200.0, 2000.0, 10000.0, -1.0 })
                    for (int ri = 0; ri <= 10; ++ri)
                    {
                        const double fc = fc0 < 0.0 ? ogi::kFxNyquist * fs : fc0;
                        const double fcL = (double) ogi::fx::svfLatch (fc, 0.1 * ri, fs).fcHz;
                        const double d = dB20 (svfGain (ogi::kSvfBP, fc, 0.1 * ri, fs, fcL));
                        ++n;
                        if (std::abs (d) > std::abs (worst)) { worst = d; at = juce::String::formatted ("fs %g fc %g res %d%%", fs, fc, ri * 10); }
                    }
            row (std::abs (worst) <= 0.1, "FU1a BP k*v1 at fc == 0 dB +-0.1 (res x fc x fs, LS sine fit)",
                 "meas " + juce::String::formatted ("%d cases, worst %+.5f dB (", n, worst) + at + ")"
                     + " | thr |d| <= 0.1 dB | margin " + eg21::f2 (0.1 - std::abs (worst), 5) + " dB | ctrl --");
        }

        // FU1b (+ inline negative control: oracle without the D10 x make factor)
        {
            double worst = 0.0; juce::String at; int n = 0;
            double worstCtrl = 0.0; juce::String atCtrl;
            for (double fs : { 48000.0, 96000.0 })
                for (double fc : { 200.0, 2000.0, 10000.0 })
                    for (double res : { 0.0, 0.5, 0.8, 0.9, 1.0 })
                        for (int mode : { ogi::kSvfLP, ogi::kSvfBP, ogi::kSvfHP })
                            for (double mul : { 0.25, 1.0, 4.0 })
                            {
                                const double f = fc * mul;
                                if (f >= 0.5 * fs) continue;
                                const auto lt = ogi::fx::svfLatch (fc, res, fs);
                                // 2.4 D10: LP / HP oracle × m(k) (BP unchanged, D8).
                                const double h0 = oracle (mode, (double) lt.fcHz, (double) lt.k, fs, f);
                                const double h = h0 * (mode == ogi::kSvfBP ? 1.0 : (double) lt.make);
                                if (dB20 (h) <= -40.0) continue;
                                const double g = dB20 (svfGain (mode, fc, res, fs, f));
                                const double d = g - dB20 (h);
                                ++n;
                                if (std::abs (d) > std::abs (worst))
                                {
                                    worst = d;
                                    at = juce::String::formatted ("fs %g fc %g res %g mode %d f %g", fs, fc, res, mode, f);
                                }
                                // control: same measurement vs the oracle with make treated as 1
                                // (LP / HP at a resonant setting only)
                                if (mode != ogi::kSvfBP && res >= 0.8 && dB20 (h0) > -40.0)
                                {
                                    const double dc = g - dB20 (h0);
                                    if (std::abs (dc) > std::abs (worstCtrl))
                                    {
                                        worstCtrl = dc;
                                        atCtrl = juce::String::formatted ("fs %g fc %g res %g mode %d f %g", fs, fc, res, mode, f);
                                    }
                                }
                            }
            const bool fired = std::abs (worstCtrl) > 0.05;
            row (std::abs (worst) <= 0.05 && n > 100 && fired, "FU1b LP/BP/HP vs prewarped bilinear oracle +-0.05 dB (|H| > -40 dB)",
                 "meas " + juce::String::formatted ("%d cases, worst %+.5f dB (", n, worst) + at + ")"
                     + " | thr |d| <= 0.05 dB, n > 100 | margin " + eg21::f2 (0.05 - std::abs (worst), 5) + " dB | "
                     + eg21::ctrl (fired, "oracle without x make (LP/HP, res >= 0.8) -> worst "
                                          + juce::String::formatted ("%+.4f dB (", worstCtrl) + atCtrl + ") > 0.05"));
        }

        // FU1i (INFO)
        {
            juce::String s;
            for (double res : { 0.5, 0.8, 0.9, 1.0 })
            {
                const auto lt = ogi::fx::svfLatch (1000.0, res, kFs);
                double pk = 0.0;
                for (int i = 1; i < 200000; ++i)
                {
                    const double f = 1000.0 * std::exp2 (-3.0 + 6.0 * i / 200000.0);
                    pk = juce::jmax (pk, oracle (ogi::kSvfLP, 1000.0, (double) lt.k, kFs, f));
                }
                s << juce::String::formatted ("res %d%% %+.2f dB (pre-make %+.2f)  ", (int) (res * 100),
                                              dB20 (pk * (double) lt.make), dB20 (pk));
            }
            std::printf ("  [INFO] FU1i LP/HP resonant peak (R18 pre-D10 expectation +0.98 / +6.63 / +10.84 / +20.01 dB; "
                         "with D10 make-down ~ +10 dB at res 100): %s\n", s.toRawUTF8());
        }

        // FU1c
        {
            bool ok = true; double peak = 0.0; juce::String bad;
            for (double fs : { 48000.0, 192000.0 })
                for (int mode : { ogi::kSvfLP, ogi::kSvfBP, ogi::kSvfHP })
                {
                    Grain g = svfGrain (mode, ogi::kFxNyquist * fs, 1.0, fs);
                    const size_t n = (size_t) (10.0 * fs);
                    float l[ogi::kCell], r[ogi::kCell];
                    for (size_t i = 0; i < n; i += ogi::kCell)
                    {
                        for (int k = 0; k < ogi::kCell; ++k)
                        {
                            // full-scale white noise: uniform [−1, 1)
                            l[k] = parity::hashNoise ((int64_t) (i + (size_t) k), 0, 1.0f);
                            r[k] = parity::hashNoise ((int64_t) (i + (size_t) k), 1, 1.0f);
                        }
                        ogi::fx::svf (g, l, r, ogi::kCell);
                        for (int k = 0; k < ogi::kCell; ++k)
                        {
                            if (! std::isfinite (l[k]) || ! std::isfinite (r[k]) || std::abs (l[k]) > 100.0f || std::abs (r[k]) > 100.0f)
                            { if (ok) bad = juce::String::formatted ("fs %g mode %d @%zu", fs, mode, i); ok = false; }
                            peak = juce::jmax (peak, (double) std::abs (l[k]), (double) std::abs (r[k]));
                        }
                    }
                    ok = ok && ogi::fx::svfStateFinite (g);
                }
            row (ok, "FU1c k 0.1 at 0.45*fs, LP/BP/HP, 10 s full-scale noise @ 48/192 k: finite, |y| <= 100",
                 "meas " + juce::String::formatted ("peak %.3f ", peak) + bad
                     + " | thr finite, |y| <= 100 | margin " + eg21::f2 (100.0 - peak, 3) + " | ctrl --");
        }

        // FU1d
        {
            int falses = 0, cases = 0;
            const float bads[3] { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };
            for (int field = 0; field < 4; ++field)
                for (float v : bads)
                {
                    Grain g {};
                    float* f[4] { &g.svfIc1[0], &g.svfIc2[0], &g.svfIc1[1], &g.svfIc2[1] };
                    *f[field] = v;
                    ++cases;
                    falses += ogi::fx::svfStateFinite (g) ? 0 : 1;
                }
            bool trues = true;
            for (float a : { 1e6f, -1e6f, 0.0f, 3.5f })
            {
                Grain g {};
                g.svfIc1[0] = a; g.svfIc2[0] = a; g.svfIc1[1] = a; g.svfIc2[1] = -a;
                trues = trues && ogi::fx::svfStateFinite (g);
                g.svfIc2[1] = a;
                trues = trues && ogi::fx::svfStateFinite (g);
            }
            row (falses == 12 && cases == 12 && trues, "FU1d svfStateFinite: NaN/+Inf/-Inf x 4 integrators false; finite |ic| <= 1e6 true",
                 "meas " + juce::String::formatted ("%d/%d false, finite %s", falses, cases, trues ? "true" : "FALSE")
                     + " | thr 12/12 false, finite true | margin " + juce::String (falses - 12) + " | ctrl --");
        }
    }

    //==========================================================================
    // FU2 — drive (D6, R15, R16)
    //==========================================================================
    Grain driveGrain (const ogi::fx::DriveLatch& d)
    {
        Grain g {};
        g.fxActive = d.active ? ogi::kFxDrive : 0u; g.driveD = d.d; g.driveNorm = d.norm;
        return g;
    }

    void driveUnitRows()
    {
        std::printf ("\n-- FU2. drive units (D6: r*tanh(dx)/tanh(dr), r = 0.25, d = 16*drive^2)\n");
        const float r = ogi::kDriveRef;
        // FU2a
        {
            int bad = 0; double worstUlp = 0.0;
            for (int k = 1; k <= 100; ++k)
            {
                const auto dl = ogi::fx::driveLatch ((float) k / 100.0f);
                Grain g = driveGrain (dl);
                float l[2] { r, -r }, rr[2] { -r, r };
                ogi::fx::drive (g, l, rr, 2);
                for (float y : { l[0], -l[1], -rr[0], rr[1] })
                {
                    const double ulp = std::abs ((double) y - (double) r) / (double) (std::nextafter (r, 1.0f) - r);
                    worstUlp = juce::jmax (worstUlp, ulp);
                    bad += ulp <= 1.0 ? 0 : 1;
                }
                bad += dl.active ? 0 : 1;
            }
            row (bad == 0, "FU2a y(+-r) == +-r within 1 ulp, drive 1 ... 100 %",
                 "meas " + juce::String::formatted ("worst %.2f ulp", worstUlp)
                     + " | thr <= 1 ulp, all active | margin " + eg21::f2 (1.0 - worstUlp, 2) + " ulp | ctrl --");
        }
        // FU2b — −12 dBFS sine at fs/100, steady-state sample peak.
        {
            double worst = 0.0; juce::String at;
            std::vector<float> drives { 0.0f, 0.0025f, 0.005f };
            for (int k = 1; k <= 100; ++k) drives.push_back ((float) k / 100.0f);
            for (float dr : drives)
            {
                Grain g = driveGrain (ogi::fx::driveLatch (dr));
                const int n = 48000;
                std::vector<float> l ((size_t) n), rr ((size_t) n);
                for (int i = 0; i < n; ++i) l[(size_t) i] = rr[(size_t) i] = (float) (0.25 * std::sin (2.0 * kPi * (double) i / 100.0));
                if (g.fxActive != 0)
                    for (int i = 0; i < n; i += ogi::kCell) ogi::fx::drive (g, l.data() + i, rr.data() + i, juce::jmin (ogi::kCell, n - i));
                double pk = 0.0;
                for (int i = 4800; i < n; ++i) pk = juce::jmax (pk, (double) std::abs (l[(size_t) i]));
                const double d = dB20 (pk / 0.25);
                if (std::abs (d) >= std::abs (worst)) { worst = d; at = juce::String::formatted ("drive %.4f", (double) dr); }
            }
            row (std::abs (worst) <= 0.1, "FU2b -12 dBFS sine at fs/100: sample peak 0.25 +-0.1 dB, drive 0 ... 100 %",
                 "meas " + juce::String::formatted ("%d drives, worst %+.6f dB (", (int) drives.size(), worst) + at + ")"
                     + " | thr |d| <= 0.1 dB | margin " + eg21::f2 (0.1 - std::abs (worst), 6) + " dB | ctrl --");
        }
        // FU2c — guard continuity.
        {
            const auto on = ogi::fx::driveLatchD (ogi::kDriveGuard);
            Grain g = driveGrain (on);
            double worst = 0.0;
            for (int e = 0; e <= 2000; ++e)
            {
                const float x = (float) (1e-6 * std::pow (4.0e6, e / 2000.0));
                for (float xs : { x, -x })
                {
                    float l[1] { xs }, rr[1] { xs };
                    ogi::fx::drive (g, l, rr, 1);
                    worst = juce::jmax (worst, std::abs ((double) l[0] - (double) xs) / std::abs ((double) xs));
                }
            }
            const auto off = ogi::fx::driveLatchD (std::nextafter (ogi::kDriveGuard, 0.0f));
            const Grain dflt {};
            const bool offOk = ! off.active && juce::exactlyEqual (off.d, dflt.driveD) && juce::exactlyEqual (off.norm, dflt.driveNorm)
                               && ! ogi::fx::driveLatch (0.0f).active && ! ogi::fx::driveLatch (0.002f).active
                               && ogi::fx::driveLatch (0.003f).active;
            row (on.active && worst <= 2e-7 && offOk, "FU2c guard: d = 1e-4 rel |y - x| <= 2e-7 over |x| in [1e-6, 4]; below guard inactive",
                 "meas " + juce::String::formatted ("worst %.3g; below-guard inactive + defaults %s", worst, offOk ? "yes" : "NO")
                     + " | thr <= 2e-7 | margin " + juce::String::formatted ("%.3g", 2e-7 - worst) + " | ctrl --");
        }
        // FU2i (INFO)
        {
            juce::String s;
            for (float dr : { 0.1f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                Grain g = driveGrain (ogi::fx::driveLatch (dr));
                const int n = 48000 * 2;
                std::vector<float> l ((size_t) n), rr ((size_t) n), x ((size_t) n);
                for (int i = 0; i < n; ++i) x[(size_t) i] = l[(size_t) i] = rr[(size_t) i] = (float) (0.25 * std::sin (2.0 * kPi * 441.0 * i / kFs));
                for (int i = 0; i < n; i += ogi::kCell) ogi::fx::drive (g, l.data() + i, rr.data() + i, juce::jmin (ogi::kCell, n - i));
                const double dRms = rmsDb (l, 0, (size_t) n) - rmsDb (x, 0, (size_t) n);
                const double dF = dB20 (sineAmp (l, 0, (size_t) n, 441.0, kFs) / 0.25);
                s << juce::String::formatted ("%.2f: RMS %+.3f F %+.3f  ", (double) dr, dRms, dF);
            }
            std::printf ("  [INFO] FU2i RMS / fundamental (R15 expects RMS +0.001/+0.044/+0.573/+1.578/+2.237): %s\n", s.toRawUTF8());
        }
    }

    //==========================================================================
    // FU3 — crush (D7, P24)
    //==========================================================================
    Grain crushGrain (float crush, double fs)
    {
        Grain g {};
        const auto c = ogi::fx::crushLatch (crush, fs);
        g.fxActive = ogi::kFxCrush; g.crushHoldInc = c.inc; g.crushScale = c.scale;
        return g;
    }

    void crushUnitRows()
    {
        std::printf ("\n-- FU3. crush units (D7 mid-tread, fractional hold)\n");
        const juce::ScopedNoDenormals nd;
        // FU3a — under its own ScopedNoDenormals (PD14).
        {
            const juce::ScopedNoDenormals ndFu3a;
            int bad = 0, cases = 0;
            for (int k = 1; k <= 100; ++k)
            {
                const float scale = ogi::fx::crushLatch ((float) k / 100.0f, kFs).scale;
                for (float x : { 0.0f, -0.0f, 1e-30f, -1e-30f, 0.49f / scale, -0.49f / scale })
                {
                    Grain g = crushGrain ((float) k / 100.0f, kFs);
                    float l[4] { x, x, x, x }, r[4] { x, x, x, x };
                    ogi::fx::crush (g, l, r, 4);
                    ++cases;
                    bool z = true;
                    for (int i = 0; i < 4; ++i) z = z && l[i] == 0.0f && r[i] == 0.0f;
                    bad += z ? 0 : 1;
                }
            }
            row (bad == 0, "FU3a silence (+-0, +-1e-30, +-0.49 LSB) -> exactly 0, crush 1 ... 100 %",
                 "meas " + juce::String::formatted ("%d/%d", cases - bad, cases)
                     + " | thr all exactly 0 | margin " + juce::String (-bad) + " | ctrl --");
        }
        // FU3b
        {
            int bad = 0; long long n = 0;
            for (int k = 1; k <= 100; ++k)
            {
                const float scale = ogi::fx::crushLatch ((float) k / 100.0f, kFs).scale;
                for (int i = -20000; i <= 20000; ++i)
                {
                    const float x = (float) i * 1.0e-4f + 1.234567e-6f * (float) (i % 7);
                    // scale = 2^(15 − 14·crush) is a power of two only at whole-bit
                    // depths, so q·scale is an integer only to rounding: gate the
                    // recovered INDEX, the error bound and exact idempotence.
                    const float q = ogi::fx::crushQ (x, scale);
                    const double err = std::abs ((double) q - (double) x);
                    const double tol = 0.5 / (double) scale + 2.0 * (double) std::abs (std::nextafter (std::abs (x) + 0.5f / scale, 4.0f) - (std::abs (x) + 0.5f / scale));
                    ++n;
                    if (std::nearbyint (q * scale) != std::nearbyint (x * scale) || err > tol || ogi::fx::crushQ (q, scale) != q) ++bad;
                }
            }
            row (bad == 0, "FU3b mid-tread lattice: index recovered, |q - x| <= LSB/2 (+ulps), q(q(x)) == q(x)",
                 "meas " + juce::String::formatted ("%lld points x 100 depths, %d bad", n / 100, bad)
                     + " | thr 0 bad | margin " + juce::String (-bad) + " | ctrl --");
        }
        // FU3n
        {
            int ok = 0;
            for (int k = 1; k <= 100; ++k)
            {
                const float scale = ogi::fx::crushLatch ((float) k / 100.0f, kFs).scale;
                const float midRise = (std::floor (0.0f * scale) + 0.5f) / scale;   // ARCHITECTURE Algorithm §9 form
                ok += (std::abs (midRise) == 0.5f / scale && midRise != 0.0f) ? 1 : 0;
            }
            row (ok == 100, "FU3n control: mid-rise reference gives |q(0)| = LSB/2 != 0 at every depth",
                 "meas " + juce::String (ok) + "/100 | thr 100/100 | margin " + juce::String (ok - 100) + " | ctrl (this row is the control)");
        }
        // FU3c — hold law on a ramp x[i] = i (q(i) == i at every depth >= 2 bits).
        {
            // The float phase accumulator drifts ~1e-6 per run, so a run may land
            // one sample past floor/ceil (1/inc); the RATE is gated by the mean.
            int bad = 0, runsChecked = 0; juce::String note; bool first = true; int run48 = 0; double worstMean = 0.0;
            for (double fs : { 44100.0, 48000.0 })
                for (float cr : { 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    Grain g = crushGrain (cr, fs);
                    const int n = (int) fs;
                    std::vector<float> l ((size_t) n), r ((size_t) n);
                    for (int i = 0; i < n; ++i) l[(size_t) i] = r[(size_t) i] = (float) i;
                    for (int i = 0; i < n; i += ogi::kCell) ogi::fx::crush (g, l.data() + i, r.data() + i, juce::jmin (ogi::kCell, n - i));
                    first = first && l[0] == ogi::fx::crushQ (0.0f, g.crushScale);
                    const double inv = 1.0 / (double) g.crushHoldInc;
                    const int lo = (int) std::floor (inv), hi = (int) std::ceil (inv);
                    int start = 0, runs = 0, firstStart = -1, lastStart = 0;
                    for (int i = 1; i < n; ++i)
                        if (l[(size_t) i] != l[(size_t) (i - 1)])
                        {
                            const int len = i - start;
                            ++runsChecked;
                            if (firstStart < 0) firstStart = i; else ++runs;
                            lastStart = i;
                            if (len < lo - 1 || len > hi + 1) { if (bad++ < 2) note << juce::String::formatted ("fs %g crush %.2f run %d (1/inc %.4f) ", fs, (double) cr, len, inv); }
                            if (fs == 48000.0 && cr == 1.0f && len == 48) ++run48;
                            start = i;
                        }
                    if (runs > 0)
                        worstMean = juce::jmax (worstMean, std::abs ((double) (lastStart - firstStart) / (double) runs / inv - 1.0));
                }
            bad += worstMean <= 1e-3 ? 0 : 1;
            row (bad == 0 && first && run48 > 900, "FU3c hold runs floor-1..ceil+1 of 1/inc, mean == 1/inc (0.1 %); first == q(x0); 48 k @ 100 % = 48",
                 "meas " + juce::String::formatted ("%d runs, %d bad, mean err %.2e, 48-runs %d, first %s ", runsChecked, bad, worstMean, run48, first ? "ok" : "BAD") + note
                     + " | thr 0 bad, mean err <= 1e-3, 48-runs > 900 | margin " + juce::String::formatted ("%.2e", 1e-3 - worstMean)
                     + ", " + juce::String (run48 - 900) + " runs | ctrl --");
        }
        // FU3i (INFO)
        {
            juce::String s;
            for (float cr : { 1.0f, 0.93f, 0.9f, 0.75f })
                s << juce::String::formatted ("%d%% %.1f dBFS  ", (int) std::lround (cr * 100), dB20 (0.5 / (double) ogi::fx::crushLatch (cr, kFs).scale));
            std::printf ("  [INFO] FU3i R22 grain silent below peak (expects -12.0 / -18 / -20.5 / -33): %s\n", s.toRawUTF8());
        }
    }

    //==========================================================================
    // FU4 — ring (R14 / P21)
    //==========================================================================
    Grain ringGrain (double f, double fs, float mix)
    {
        Grain g {};
        const auto rl = ogi::fx::ringLatch (f, fs);
        g.fxActive = ogi::kFxRing; g.ringRotRe = rl.rotRe; g.ringRotIm = rl.rotIm; g.ringMix = mix;
        return g;
    }

    void ringUnitRows()
    {
        std::printf ("\n-- FU4. ring units (quadrature rotator, per-sample Newton renorm)\n");
        const double fsList[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
        // FU4a + FU4b
        {
            double worstDrift = -400.0, worstHz = 0.0; juce::String atD, atF;
            for (double fs : fsList)
                for (double f0 : { 5.0, 20.0, 200.0, 5000.0, -1.0 })
                {
                    const double f = f0 < 0.0 ? ogi::kFxNyquist * fs : f0;
                    Grain g = ringGrain (f, fs, 1.0f);
                    const int n = (int) (2.0 * fs);
                    double mx = 0.0, unwrapped = 0.0, prev = 0.0;
                    const int n1 = (int) fs;   // phase slope over the first second
                    for (int i = 0; i < n; ++i)
                    {
                        float l[1] { 1.0f }, r[1] { 1.0f };
                        ogi::fx::ring (g, l, r, 1);
                        const double re = g.ringRe, im = g.ringIm;
                        mx = juce::jmax (mx, std::abs (std::sqrt (re * re + im * im) - 1.0));
                        if (i < n1)
                        {
                            const double ph = std::atan2 (im, re);
                            double d = ph - prev;
                            while (d > kPi) d -= 2.0 * kPi;
                            while (d < -kPi) d += 2.0 * kPi;
                            unwrapped += d; prev = ph;
                        }
                    }
                    const double fhat = unwrapped / (2.0 * kPi) * fs / (double) n1;
                    const double dd = dB20 (mx);
                    if (dd > worstDrift) { worstDrift = dd; atD = juce::String::formatted ("f %g fs %g", f, fs); }
                    const double df = std::abs (fhat - f);
                    if (df > worstHz) { worstHz = df; atF = juce::String::formatted ("f %g fs %g", f, fs); }
                }
            row (worstDrift <= -135.0, "FU4a rotator drift max ||c| - 1| <= -135 dB over 2 s (5 f x 4 fs)",
                 "meas " + juce::String::formatted ("worst %.1f dB (", worstDrift) + atD + ")"
                     + " | thr <= -135 dB | margin " + eg21::f2 (-135.0 - worstDrift, 1) + " dB | ctrl FU4n");
            row (worstHz <= 0.01, "FU4b realised carrier frequency within 0.01 Hz",
                 "meas " + juce::String::formatted ("worst %.5f Hz (", worstHz) + atF + ")"
                     + " | thr <= 0.01 Hz | margin " + eg21::f2 (0.01 - worstHz, 5) + " Hz | ctrl --");
        }
        // FU4c
        {
            bool ok = true;
            for (float mix : { 0.01f, 0.3f, 0.5f, 1.0f })
            {
                Grain g = ringGrain (440.0, kFs, mix);
                float l[64], r[64];
                for (int i = 0; i < 64; ++i) { l[i] = 0.37f + 0.01f * (float) i; r[i] = l[i]; }
                const float x0 = l[0];
                // replica rotator (same float ops) for the mix-100 % check
                float re = 1.0f, im = 0.0f; float c[64];
                for (int i = 0; i < 64; ++i)
                {
                    c[i] = re;
                    const float nr = re * g.ringRotRe - im * g.ringRotIm, ni = re * g.ringRotIm + im * g.ringRotRe;
                    const float sN = 1.5f - 0.5f * (nr * nr + ni * ni);
                    re = nr * sN; im = ni * sN;
                }
                float xs[64]; for (int i = 0; i < 64; ++i) xs[i] = l[i];
                ogi::fx::ring (g, l, r, 32);
                ogi::fx::ring (g, l + 32, r + 32, 32);
                ok = ok && l[0] == x0 * ((1.0f - mix) + mix);
                for (int i = 0; i < 64; ++i) ok = ok && l[i] == r[i];
                if (mix == 1.0f)
                    for (int i = 0; i < 64; ++i) ok = ok && l[i] == xs[i] * c[i];
            }
            row (ok, "FU4c mix law: first == x*((1-mix)+mix); L/R share carrier; mix 100 % == x*Re c",
                 juce::String ("meas ") + (ok ? "all bitwise" : "MISMATCH") + " | thr bitwise == | margin -- | ctrl --");
        }
        // FU4n — per-32 renormalisation control at 5 Hz / 48 k.
        {
            const auto rl = ogi::fx::ringLatch (5.0, kFs);
            float re = 1.0f, im = 0.0f; double mx = 0.0;
            for (int i = 0; i < (int) (2.0 * kFs); ++i)
            {
                const float nr = re * rl.rotRe - im * rl.rotIm, ni = re * rl.rotIm + im * rl.rotRe;
                re = nr; im = ni;
                if ((i + 1) % 32 == 0) { const float sN = 1.5f - 0.5f * (re * re + im * im); re *= sN; im *= sN; }
                mx = juce::jmax (mx, std::abs (std::sqrt ((double) re * re + (double) im * im) - 1.0));
            }
            row (dB20 (mx) > -135.0, "FU4n control: per-32 renorm at 5 Hz / 48 k reads > -135 dB",
                 "meas " + juce::String::formatted ("%.1f dB", dB20 (mx))
                     + " | thr > -135 dB | margin " + eg21::f2 (dB20 (mx) + 135.0, 1) + " dB | ctrl (this row is the control)");
        }
    }

    //==========================================================================
    void runAll()
    {
        std::printf ("\n== Phase 2.3 FX unit rows (FU, ported from O-Granulator @ 82406fce engine_gates_23.h)\n");
        svfUnitRows();
        driveUnitRows();
        crushUnitRows();
        ringUnitRows();
    }
}}
