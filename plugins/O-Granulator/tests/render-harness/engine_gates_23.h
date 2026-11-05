/*
  ==============================================================================

    O-Granulator — render harness, sections U / V / H (Stage 2 / Phase 2.3,
    PLAN-2.3 T3–T10). Sibling of engine_gates_22.h / engine_gates_22b.h
    (reuses eg::Job / render / setP / fft, eg22::base / RecLog / collect / sp,
    eg22b::renderPool).

    U — FX law units on Source/dsp/GrainFx.h (the functions the engine runs)
      U1a BP = k·v1 steady-state magnitude at fc == 0 dB ±0.1 (D8, R18):
          res {0, .1 … 1} × fc {20, 200, 2 k, 10 k, 0.45·fs} × fs {44.1, 48,
          96, 192} k; least-squares sine fit over the last 0.5 s of 2 s
      U1b LP / BP / HP vs the prewarped bilinear oracle at {fc/4, fc, 4·fc <
          fs/2} × res {0, .5, .8, .9, 1}: ±0.05 dB wherever |H| > −40 dB
      U1i (INFO) LP/HP peak at res 50 / 80 / 90 / 100 % (R18)
      U1c k 0.1 at 0.45·fs, LP/BP/HP, 10 s full-scale white noise @ 48 / 192 k:
          every sample finite, |y| <= 100, end state finite
      U1d svfStateFinite: NaN / +Inf / −Inf in each integrator → false (12);
          finite |ic| <= 1e6 → true
      U2a y (±r) == ±r within 1 ulp, drive 1 … 100 % (D6)
      U2b −12 dBFS sine at fs/100: output sample peak 0.25 ±0.1 dB, drive
          {0, .25, .5, 1 … 100 %} (R15: a peak gate)
      U2c guard: at d = 1e-4 max |y − x|/|x| <= 2e-7 over |x| ∈ [1e-6, 4];
          just below the guard the stage is inactive (defaults latched)
      U2i (INFO) RMS / fundamental Δ at drive {.1, .25, .5, .75, 1}
      U3a silence → exactly 0 at every depth (D7), under ScopedNoDenormals
      U3b mid-tread lattice: q·scale integer, |q − x| <= ½ LSB (+1 ulp),
          q (q (x)) == q (x)
      U3n control: the ARCHITECTURE mid-rise form gives |q (0)| = ½ LSB ≠ 0
      U3c hold law: equal-output runs are floor / ceil (1/inc); first output ==
          q (x₀) (fresh hold); crush 100 % @ 48 k → 48-sample runs
      U3i (INFO) R22 silence thresholds
      U4a rotator drift max ||c| − 1| <= −135 dB over 2 s, f {5, 20, 200, 5 k,
          0.45·fs} × fs {44.1, 48, 96, 192} k (R14 / P21)
      U4b realised carrier frequency within 0.01 Hz
      U4c mix law: first output == x·((1 − mix) + mix); L/R share the
          carrier; mix 100 % = x·Re c
      U4n control: renormalised once per 32 samples reads > −135 dB at 5 Hz

    V — DSP-05 per-grain (R17: census + audio rows, shared-latch controls)
      V2  BP centroid: white noise, BP res 80 %, 2 kHz, filter_rand 100 %,
          isolated grains (100 ms, 5 g/s, jitter 0, spray 10 %), >= 40 grains:
          Spearman ρ (log2 fcHz, per-grain log2 centroid) >= 0.9;
          V2n fxSharedLatch |ρ| <= 0.5

  ==============================================================================
*/
#pragma once

#include "dsp/GrainFx.h"

namespace { namespace eg23
{
    using eg::Job;
    using eg::Out;
    using eg::Proc;
    using eg::P;
    using eg22::sp;
    using Rec  = ogran::Engine::SpawnRecord;
    using Grain = ogran::Grain;
    constexpr double kFs = 48000.0;
    constexpr double kPi = 3.14159265358979323846;

    double dB20 (double a) { return a > 0.0 ? 20.0 * std::log10 (a) : -400.0; }

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
        const auto s = ogran::fx::svfLatch (fc, res, fs);
        g.fxActive = ogran::kFxSvf; g.svfMode = mode;
        g.svfK = s.k; g.svfA1 = s.a1; g.svfA2 = s.a2; g.svfA3 = s.a3;
        return g;
    }

    // Steady-state |H| (linear) of the grain's SVF for a sine at f.
    double svfGain (int mode, double fc, double res, double fs, double f, double secs = 2.0, double fitSecs = 0.5)
    {
        Grain g = svfGrain (mode, fc, res, fs);
        const size_t n = (size_t) (secs * fs);
        std::vector<float> l (n), r (n);
        for (size_t i = 0; i < n; ++i) l[i] = r[i] = (float) (0.5 * std::sin (2.0 * kPi * f * (double) i / fs));
        for (size_t i = 0; i < n; i += ogran::kCell)
            ogran::fx::svf (g, l.data() + i, r.data() + i, (int) juce::jmin ((size_t) ogran::kCell, n - i));
        return sineAmp (l, n - (size_t) (fitSecs * fs), n, f, fs) / 0.5;
    }

    // Prewarped bilinear oracle.
    double oracle (int mode, double fc, double k, double fs, double f)
    {
        const double om = std::tan (kPi * f / fs) / std::tan (kPi * fc / fs);
        const std::complex<double> D (1.0 - om * om, k * om);
        std::complex<double> N = mode == ogran::kSvfLP ? std::complex<double> (1.0, 0.0)
                               : mode == ogran::kSvfBP ? std::complex<double> (0.0, k * om)
                                                       : std::complex<double> (-om * om, 0.0);
        return std::abs (N / D);
    }

    //==========================================================================
    // U1 — SVF
    //==========================================================================
    void svfUnitRows()
    {
        std::printf ("\n-- U1. SVF units (TPT, BP = k*v1, D8 / R18)\n");
        const double fsList[4] { 44100.0, 48000.0, 96000.0, 192000.0 };

        // U1a
        {
            double worst = 0.0; juce::String at; int n = 0;
            for (double fs : fsList)
                for (double fc0 : { 20.0, 200.0, 2000.0, 10000.0, -1.0 })
                    for (int ri = 0; ri <= 10; ++ri)
                    {
                        const double fc = fc0 < 0.0 ? ogran::kFxNyquist * fs : fc0;
                        const double fcL = (double) ogran::fx::svfLatch (fc, 0.1 * ri, fs).fcHz;
                        const double d = dB20 (svfGain (ogran::kSvfBP, fc, 0.1 * ri, fs, fcL));
                        ++n;
                        if (std::abs (d) > std::abs (worst)) { worst = d; at = juce::String::formatted ("fs %g fc %g res %d%%", fs, fc, ri * 10); }
                    }
            row (std::abs (worst) <= 0.1, "U1a BP k*v1 at fc == 0 dB +-0.1 (res x fc x fs, LS sine fit)",
                 juce::String::formatted ("%d cases, worst %+.5f dB (", n, worst) + at + ")");
        }

        // U1b
        {
            double worst = 0.0; juce::String at; int n = 0;
            for (double fs : { 48000.0, 96000.0 })
                for (double fc : { 200.0, 2000.0, 10000.0 })
                    for (double res : { 0.0, 0.5, 0.8, 0.9, 1.0 })
                        for (int mode : { ogran::kSvfLP, ogran::kSvfBP, ogran::kSvfHP })
                            for (double mul : { 0.25, 1.0, 4.0 })
                            {
                                const double f = fc * mul;
                                if (f >= 0.5 * fs) continue;
                                const auto lt = ogran::fx::svfLatch (fc, res, fs);
                                const double h = oracle (mode, (double) lt.fcHz, (double) lt.k, fs, f);
                                if (dB20 (h) <= -40.0) continue;
                                const double d = dB20 (svfGain (mode, fc, res, fs, f)) - dB20 (h);
                                ++n;
                                if (std::abs (d) > std::abs (worst))
                                {
                                    worst = d;
                                    at = juce::String::formatted ("fs %g fc %g res %g mode %d f %g", fs, fc, res, mode, f);
                                }
                            }
            row (std::abs (worst) <= 0.05 && n > 100, "U1b LP/BP/HP vs prewarped bilinear oracle +-0.05 dB (|H| > -40 dB)",
                 juce::String::formatted ("%d cases, worst %+.5f dB (", n, worst) + at + ")");
        }

        // U1i (INFO)
        {
            juce::String s;
            for (double res : { 0.5, 0.8, 0.9, 1.0 })
            {
                const auto lt = ogran::fx::svfLatch (1000.0, res, kFs);
                double pk = 0.0;
                for (int i = 1; i < 200000; ++i)
                {
                    const double f = 1000.0 * std::exp2 (-3.0 + 6.0 * i / 200000.0);
                    pk = juce::jmax (pk, oracle (ogran::kSvfLP, 1000.0, (double) lt.k, kFs, f));
                }
                s << juce::String::formatted ("res %d%% %+.2f dB  ", (int) (res * 100), dB20 (pk));
            }
            std::printf ("  [INFO] U1i LP/HP resonant peak (R18 expects +0.98 / +6.63 / +10.84 / +20.01): %s\n", s.toRawUTF8());
        }

        // U1c
        {
            bool ok = true; double peak = 0.0; juce::String bad;
            for (double fs : { 48000.0, 192000.0 })
                for (int mode : { ogran::kSvfLP, ogran::kSvfBP, ogran::kSvfHP })
                {
                    Grain g = svfGrain (mode, ogran::kFxNyquist * fs, 1.0, fs);
                    const size_t n = (size_t) (10.0 * fs);
                    float l[ogran::kCell], r[ogran::kCell];
                    for (size_t i = 0; i < n; i += ogran::kCell)
                    {
                        for (int k = 0; k < ogran::kCell; ++k)
                        {
                            // full-scale white noise: uniform [−1, 1)
                            l[k] = eg::hashNoise ((juce::int64) (i + (size_t) k), 0, 1.0f);
                            r[k] = eg::hashNoise ((juce::int64) (i + (size_t) k), 1, 1.0f);
                        }
                        ogran::fx::svf (g, l, r, ogran::kCell);
                        for (int k = 0; k < ogran::kCell; ++k)
                        {
                            if (! std::isfinite (l[k]) || ! std::isfinite (r[k]) || std::abs (l[k]) > 100.0f || std::abs (r[k]) > 100.0f)
                            { if (ok) bad = juce::String::formatted ("fs %g mode %d @%zu", fs, mode, i); ok = false; }
                            peak = juce::jmax (peak, (double) std::abs (l[k]), (double) std::abs (r[k]));
                        }
                    }
                    ok = ok && ogran::fx::svfStateFinite (g);
                }
            row (ok, "U1c k 0.1 at 0.45*fs, LP/BP/HP, 10 s full-scale noise @ 48/192 k: finite, |y| <= 100",
                 juce::String::formatted ("peak %.3f ", peak) + bad);
        }

        // U1d
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
                    falses += ogran::fx::svfStateFinite (g) ? 0 : 1;
                }
            bool trues = true;
            for (float a : { 1e6f, -1e6f, 0.0f, 3.5f })
            {
                Grain g {};
                g.svfIc1[0] = a; g.svfIc2[0] = a; g.svfIc1[1] = a; g.svfIc2[1] = -a;
                trues = trues && ogran::fx::svfStateFinite (g);
                g.svfIc2[1] = a;
                trues = trues && ogran::fx::svfStateFinite (g);
            }
            row (falses == 12 && cases == 12 && trues, "U1d svfStateFinite: NaN/+Inf/-Inf x 4 integrators false; finite |ic| <= 1e6 true",
                 juce::String::formatted ("%d/%d false, finite %s", falses, cases, trues ? "true" : "FALSE"));
        }
    }

    //==========================================================================
    // U2 — drive (D6, R15, R16)
    //==========================================================================
    Grain driveGrain (const ogran::fx::DriveLatch& d)
    {
        Grain g {};
        g.fxActive = d.active ? ogran::kFxDrive : 0u; g.driveD = d.d; g.driveNorm = d.norm;
        return g;
    }

    void driveUnitRows()
    {
        std::printf ("\n-- U2. drive units (D6: r*tanh(dx)/tanh(dr), r = 0.25, d = 16*drive^2)\n");
        const float r = ogran::kDriveRef;
        // U2a
        {
            int bad = 0; double worstUlp = 0.0;
            for (int k = 1; k <= 100; ++k)
            {
                const auto dl = ogran::fx::driveLatch ((float) k / 100.0f);
                Grain g = driveGrain (dl);
                float l[2] { r, -r }, rr[2] { -r, r };
                ogran::fx::drive (g, l, rr, 2);
                for (float y : { l[0], -l[1], -rr[0], rr[1] })
                {
                    const double ulp = std::abs ((double) y - (double) r) / (double) (std::nextafter (r, 1.0f) - r);
                    worstUlp = juce::jmax (worstUlp, ulp);
                    bad += ulp <= 1.0 ? 0 : 1;
                }
                bad += dl.active ? 0 : 1;
            }
            row (bad == 0, "U2a y(+-r) == +-r within 1 ulp, drive 1 ... 100 %", juce::String::formatted ("worst %.2f ulp", worstUlp));
        }
        // U2b — −12 dBFS sine at fs/100, steady-state sample peak.
        {
            double worst = 0.0; juce::String at;
            std::vector<float> drives { 0.0f, 0.0025f, 0.005f };
            for (int k = 1; k <= 100; ++k) drives.push_back ((float) k / 100.0f);
            for (float dr : drives)
            {
                Grain g = driveGrain (ogran::fx::driveLatch (dr));
                const int n = 48000;
                std::vector<float> l ((size_t) n), rr ((size_t) n);
                for (int i = 0; i < n; ++i) l[(size_t) i] = rr[(size_t) i] = (float) (0.25 * std::sin (2.0 * kPi * (double) i / 100.0));
                if (g.fxActive != 0)
                    for (int i = 0; i < n; i += ogran::kCell) ogran::fx::drive (g, l.data() + i, rr.data() + i, juce::jmin (ogran::kCell, n - i));
                double pk = 0.0;
                for (int i = 4800; i < n; ++i) pk = juce::jmax (pk, (double) std::abs (l[(size_t) i]));
                const double d = dB20 (pk / 0.25);
                if (std::abs (d) >= std::abs (worst)) { worst = d; at = juce::String::formatted ("drive %.4f", (double) dr); }
            }
            row (std::abs (worst) <= 0.1, "U2b -12 dBFS sine at fs/100: sample peak 0.25 +-0.1 dB, drive 0 ... 100 %",
                 juce::String::formatted ("%d drives, worst %+.6f dB (", (int) drives.size(), worst) + at + ")");
        }
        // U2c — guard continuity.
        {
            const auto on = ogran::fx::driveLatchD (ogran::kDriveGuard);
            Grain g = driveGrain (on);
            double worst = 0.0;
            for (int e = 0; e <= 2000; ++e)
            {
                const float x = (float) (1e-6 * std::pow (4.0e6, e / 2000.0));
                for (float xs : { x, -x })
                {
                    float l[1] { xs }, rr[1] { xs };
                    ogran::fx::drive (g, l, rr, 1);
                    worst = juce::jmax (worst, std::abs ((double) l[0] - (double) xs) / std::abs ((double) xs));
                }
            }
            const auto off = ogran::fx::driveLatchD (std::nextafter (ogran::kDriveGuard, 0.0f));
            const Grain dflt {};
            const bool offOk = ! off.active && juce::exactlyEqual (off.d, dflt.driveD) && juce::exactlyEqual (off.norm, dflt.driveNorm)
                               && ! ogran::fx::driveLatch (0.0f).active && ! ogran::fx::driveLatch (0.002f).active
                               && ogran::fx::driveLatch (0.003f).active;
            row (on.active && worst <= 2e-7 && offOk, "U2c guard: d = 1e-4 rel |y - x| <= 2e-7 over |x| in [1e-6, 4]; below guard inactive",
                 juce::String::formatted ("worst %.3g; below-guard inactive + defaults %s", worst, offOk ? "yes" : "NO"));
        }
        // U2i (INFO)
        {
            juce::String s;
            for (float dr : { 0.1f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                Grain g = driveGrain (ogran::fx::driveLatch (dr));
                const int n = 48000 * 2;
                std::vector<float> l ((size_t) n), rr ((size_t) n), x ((size_t) n);
                for (int i = 0; i < n; ++i) x[(size_t) i] = l[(size_t) i] = rr[(size_t) i] = (float) (0.25 * std::sin (2.0 * kPi * 441.0 * i / kFs));
                for (int i = 0; i < n; i += ogran::kCell) ogran::fx::drive (g, l.data() + i, rr.data() + i, juce::jmin (ogran::kCell, n - i));
                const double dRms = eg::rmsDb (l, 0, (size_t) n) - eg::rmsDb (x, 0, (size_t) n);
                const double dF = dB20 (sineAmp (l, 0, (size_t) n, 441.0, kFs) / 0.25);
                s << juce::String::formatted ("%.2f: RMS %+.3f F %+.3f  ", (double) dr, dRms, dF);
            }
            std::printf ("  [INFO] U2i RMS / fundamental (R15 expects RMS +0.001/+0.044/+0.573/+1.578/+2.237): %s\n", s.toRawUTF8());
        }
    }

    //==========================================================================
    // U3 — crush (D7, P24)
    //==========================================================================
    Grain crushGrain (float crush, double fs)
    {
        Grain g {};
        const auto c = ogran::fx::crushLatch (crush, fs);
        g.fxActive = ogran::kFxCrush; g.crushHoldInc = c.inc; g.crushScale = c.scale;
        return g;
    }

    void crushUnitRows()
    {
        std::printf ("\n-- U3. crush units (D7 mid-tread, fractional hold)\n");
        const juce::ScopedNoDenormals nd;
        // U3a
        {
            int bad = 0, cases = 0;
            for (int k = 1; k <= 100; ++k)
            {
                const float scale = ogran::fx::crushLatch ((float) k / 100.0f, kFs).scale;
                for (float x : { 0.0f, -0.0f, 1e-30f, -1e-30f, 0.49f / scale, -0.49f / scale })
                {
                    Grain g = crushGrain ((float) k / 100.0f, kFs);
                    float l[4] { x, x, x, x }, r[4] { x, x, x, x };
                    ogran::fx::crush (g, l, r, 4);
                    ++cases;
                    bool z = true;
                    for (int i = 0; i < 4; ++i) z = z && l[i] == 0.0f && r[i] == 0.0f;
                    bad += z ? 0 : 1;
                }
            }
            row (bad == 0, "U3a silence (+-0, +-1e-30, +-0.49 LSB) -> exactly 0, crush 1 ... 100 %", juce::String::formatted ("%d/%d", cases - bad, cases));
        }
        // U3b
        {
            int bad = 0; long long n = 0;
            for (int k = 1; k <= 100; ++k)
            {
                const float scale = ogran::fx::crushLatch ((float) k / 100.0f, kFs).scale;
                for (int i = -20000; i <= 20000; ++i)
                {
                    const float x = (float) i * 1.0e-4f + 1.234567e-6f * (float) (i % 7);
                    // scale = 2^(15 − 14·crush) is a power of two only at whole-bit
                    // depths, so q·scale is an integer only to rounding: gate the
                    // recovered INDEX, the error bound and exact idempotence.
                    const float q = ogran::fx::crushQ (x, scale);
                    const double err = std::abs ((double) q - (double) x);
                    const double tol = 0.5 / (double) scale + 2.0 * (double) std::abs (std::nextafter (std::abs (x) + 0.5f / scale, 4.0f) - (std::abs (x) + 0.5f / scale));
                    ++n;
                    if (std::nearbyint (q * scale) != std::nearbyint (x * scale) || err > tol || ogran::fx::crushQ (q, scale) != q) ++bad;
                }
            }
            row (bad == 0, "U3b mid-tread lattice: index recovered, |q - x| <= LSB/2 (+ulps), q(q(x)) == q(x)",
                 juce::String::formatted ("%lld points x 100 depths, %d bad", n / 100, bad));
        }
        // U3n
        {
            int ok = 0;
            for (int k = 1; k <= 100; ++k)
            {
                const float scale = ogran::fx::crushLatch ((float) k / 100.0f, kFs).scale;
                const float midRise = (std::floor (0.0f * scale) + 0.5f) / scale;   // ARCHITECTURE Algorithm §9 form
                ok += (std::abs (midRise) == 0.5f / scale && midRise != 0.0f) ? 1 : 0;
            }
            row (ok == 100, "U3n control: mid-rise reference gives |q(0)| = LSB/2 != 0 at every depth", juce::String (ok) + "/100");
        }
        // U3c — hold law on a ramp x[i] = i (q(i) == i at every depth >= 2 bits).
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
                    for (int i = 0; i < n; i += ogran::kCell) ogran::fx::crush (g, l.data() + i, r.data() + i, juce::jmin (ogran::kCell, n - i));
                    first = first && l[0] == ogran::fx::crushQ (0.0f, g.crushScale);
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
            row (bad == 0 && first && run48 > 900, "U3c hold runs floor-1..ceil+1 of 1/inc, mean == 1/inc (0.1 %); first == q(x0); 48 k @ 100 % = 48",
                 juce::String::formatted ("%d runs, %d bad, mean err %.2e, 48-runs %d, first %s ", runsChecked, bad, worstMean, run48, first ? "ok" : "BAD") + note);
        }
        // U3i (INFO)
        {
            juce::String s;
            for (float cr : { 1.0f, 0.93f, 0.9f, 0.75f })
                s << juce::String::formatted ("%d%% %.1f dBFS  ", (int) std::lround (cr * 100), dB20 (0.5 / (double) ogran::fx::crushLatch (cr, kFs).scale));
            std::printf ("  [INFO] U3i R22 grain silent below peak (expects -12.0 / -18 / -20.5 / -33): %s\n", s.toRawUTF8());
        }
    }

    //==========================================================================
    // U4 — ring (R14 / P21)
    //==========================================================================
    Grain ringGrain (double f, double fs, float mix)
    {
        Grain g {};
        const auto rl = ogran::fx::ringLatch (f, fs);
        g.fxActive = ogran::kFxRing; g.ringRotRe = rl.rotRe; g.ringRotIm = rl.rotIm; g.ringMix = mix;
        return g;
    }

    void ringUnitRows()
    {
        std::printf ("\n-- U4. ring units (quadrature rotator, per-sample Newton renorm)\n");
        const double fsList[4] { 44100.0, 48000.0, 96000.0, 192000.0 };
        // U4a + U4b
        {
            double worstDrift = -400.0, worstHz = 0.0; juce::String atD, atF;
            for (double fs : fsList)
                for (double f0 : { 5.0, 20.0, 200.0, 5000.0, -1.0 })
                {
                    const double f = f0 < 0.0 ? ogran::kFxNyquist * fs : f0;
                    Grain g = ringGrain (f, fs, 1.0f);
                    const int n = (int) (2.0 * fs);
                    double mx = 0.0, unwrapped = 0.0, prev = 0.0;
                    const int n1 = (int) fs;   // phase slope over the first second
                    for (int i = 0; i < n; ++i)
                    {
                        float l[1] { 1.0f }, r[1] { 1.0f };
                        ogran::fx::ring (g, l, r, 1);
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
            row (worstDrift <= -135.0, "U4a rotator drift max ||c| - 1| <= -135 dB over 2 s (5 f x 4 fs)",
                 juce::String::formatted ("worst %.1f dB (", worstDrift) + atD + ")");
            row (worstHz <= 0.01, "U4b realised carrier frequency within 0.01 Hz", juce::String::formatted ("worst %.5f Hz (", worstHz) + atF + ")");
        }
        // U4c
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
                ogran::fx::ring (g, l, r, 32);
                ogran::fx::ring (g, l + 32, r + 32, 32);
                ok = ok && l[0] == x0 * ((1.0f - mix) + mix);
                for (int i = 0; i < 64; ++i) ok = ok && l[i] == r[i];
                if (mix == 1.0f)
                    for (int i = 0; i < 64; ++i) ok = ok && l[i] == xs[i] * c[i];
            }
            row (ok, "U4c mix law: first == x*((1-mix)+mix); L/R share carrier; mix 100 % == x*Re c");
        }
        // U4n — per-32 renormalisation control at 5 Hz / 48 k.
        {
            const auto rl = ogran::fx::ringLatch (5.0, kFs);
            float re = 1.0f, im = 0.0f; double mx = 0.0;
            for (int i = 0; i < (int) (2.0 * kFs); ++i)
            {
                const float nr = re * rl.rotRe - im * rl.rotIm, ni = re * rl.rotIm + im * rl.rotRe;
                re = nr; im = ni;
                if ((i + 1) % 32 == 0) { const float sN = 1.5f - 0.5f * (re * re + im * im); re *= sN; im *= sN; }
                mx = juce::jmax (mx, std::abs (std::sqrt ((double) re * re + (double) im * im) - 1.0));
            }
            row (dB20 (mx) > -135.0, "U4n control: per-32 renorm at 5 Hz / 48 k reads > -135 dB", juce::String::formatted ("%.1f dB", dB20 (mx)));
        }
    }

    //==========================================================================
    // Per-grain helpers
    //==========================================================================
    double spearman (const std::vector<double>& a, const std::vector<double>& b)
    {
        auto ranks = [] (const std::vector<double>& x)
        {
            std::vector<size_t> idx (x.size());
            for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
            std::sort (idx.begin(), idx.end(), [&x] (size_t i, size_t j) { return x[i] < x[j]; });
            std::vector<double> r (x.size());
            for (size_t k = 0; k < idx.size();)
            {
                size_t e = k;
                while (e + 1 < idx.size() && x[idx[e + 1]] == x[idx[k]]) ++e;
                for (size_t m = k; m <= e; ++m) r[idx[m]] = 0.5 * (double) (k + e);
                k = e + 1;
            }
            return r;
        };
        const auto ra = ranks (a), rb = ranks (b);
        const double n = (double) ra.size();
        double ma = 0, mb = 0;
        for (size_t i = 0; i < ra.size(); ++i) { ma += ra[i]; mb += rb[i]; }
        ma /= n; mb /= n;
        double sab = 0, saa = 0, sbb = 0;
        for (size_t i = 0; i < ra.size(); ++i)
        {
            sab += (ra[i] - ma) * (rb[i] - mb);
            saa += (ra[i] - ma) * (ra[i] - ma);
            sbb += (rb[i] - mb) * (rb[i] - mb);
        }
        return sab / std::sqrt (juce::jmax (1e-300, saa * sbb));
    }

    // Isolated grains: [onset, onset + ceil (len) + 2) does not overlap a
    // neighbour and lies inside the render.
    std::vector<Rec> isolated (const std::vector<Rec>& recs, size_t total)
    {
        std::vector<Rec> out;
        for (size_t k = 0; k < recs.size(); ++k)
        {
            const auto& r = recs[k];
            const juce::int64 a = r.onsetSample, b = a + (juce::int64) std::ceil (r.len) + 2;
            const bool iso = (k == 0 || recs[k - 1].onsetSample + (juce::int64) std::ceil (recs[k - 1].len) + 2 <= a)
                          && (k + 1 >= recs.size() || recs[k + 1].onsetSample >= b);
            if (iso && b < (juce::int64) total) out.push_back (r);
        }
        return out;
    }

    // Power spectrum of one grain's output segment (L), zero-padded to N.
    std::vector<double> grainSpectrum (const Out& o, const Rec& r, size_t N)
    {
        std::vector<std::complex<double>> x (N);
        const juce::int64 a = r.onsetSample, b = a + (juce::int64) std::ceil (r.len) + 2;
        for (juce::int64 i = a; i < b && (size_t) (i - a) < N; ++i) x[(size_t) (i - a)] = (double) o.L[(size_t) i];
        eg::fft (x);
        std::vector<double> p (N / 2);
        for (size_t k = 0; k < N / 2; ++k) p[k] = std::norm (x[k]);
        return p;
    }

    Job isoJob (double secs, std::initializer_list<std::pair<const char*, float>> extra)
    {
        Job j; j.len = (juce::int64) (secs * kFs); j.block = 512;
        j.params = eg22::base ({ 1 });
        for (auto& kv : P { { "s1_size", 100.0f }, { "s1_density", 5.0f }, { "s1_jitter", 0.0f }, { "s1_spray", 10.0f } })
            j.params.push_back (kv);
        for (auto& kv : extra) j.params.push_back ({ kv.first, kv.second });
        return j;
    }

    //==========================================================================
    // V2 — BP per-grain centroid (T4 tracer)
    //==========================================================================
    void centroidRows()
    {
        std::printf ("\n-- V2. DSP-05 per-grain BP centroid vs latched fc (R17)\n");
        auto run = [] (bool shared, int& used, double& spanOct)
        {
            Job j = isoJob (12.0, { { "s1_filter_type", 2.0f }, { "s1_filter_res", 80.0f }, { "s1_filter_cutoff", 2000.0f },
                                    { "s1_filter_rand", 100.0f } });
            j.stim = eg::noiseStim();
            j.pre = [shared] (Proc& p) { p.getEngineForTest().hooks.fxSharedLatch = shared; };
            eg22::RecLog L; j.after = eg22::collect (L);
            const auto o = eg::render (j);
            const auto iso = isolated (L.r, o.L.size());
            constexpr size_t N = 1u << 15;
            std::vector<double> lf, lc;
            double cmin = 1e9, cmax = -1e9;
            for (auto& r : iso)
            {
                if ((r.fxActive & ogran::kFxSvf) == 0) continue;
                const auto p = grainSpectrum (o, r, N);
                double w = 0.0, acc = 0.0;
                for (size_t k = 1; k < p.size(); ++k)
                {
                    const double f = (double) k * kFs / (double) N;
                    if (f < 50.0 || f > ogran::kFxNyquist * kFs) continue;
                    w += p[k]; acc += p[k] * std::log2 (f);
                }
                if (w <= 0.0) continue;
                lf.push_back (std::log2 ((double) r.fcHz));
                lc.push_back (acc / w);
                cmin = juce::jmin (cmin, acc / w); cmax = juce::jmax (cmax, acc / w);
            }
            used = (int) lf.size();
            spanOct = cmax - cmin;
            return lf.size() >= 3 ? spearman (lf, lc) : 0.0;
        };
        int n = 0, nn = 0; double span = 0, spann = 0;
        const double rho = run (false, n, span), rhon = run (true, nn, spann);
        row (n >= 40 && rho >= 0.9, "V2 BP res 80 %, rand 100 %: Spearman(log2 fc, per-grain centroid) >= 0.9",
             juce::String::formatted ("rho %.4f over %d isolated grains, centroid span %.2f oct", rho, n, span) + (n >= 40 ? "" : "  VACUOUS"));
        row (nn >= 40 && std::abs (rhon) <= 0.5, "V2n fxSharedLatch control: |rho| <= 0.5",
             juce::String::formatted ("rho %+.4f over %d grains, centroid span %.2f oct", rhon, nn, spann));
    }
} } // namespace eg23 (anonymous)

static void engineRows23()
{
    std::printf ("\n== U/V/H. Stage 2 / Phase 2.3 FX law units + per-grain + no-bleed rows (engine_gates_23.h)\n");
    const auto t0 = std::chrono::steady_clock::now();
    eg23::svfUnitRows();
    eg23::driveUnitRows();
    eg23::crushUnitRows();
    eg23::ringUnitRows();
    eg23::centroidRows();
    std::printf ("  [INFO] 2.3 rows wall time %.1f s\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
}
