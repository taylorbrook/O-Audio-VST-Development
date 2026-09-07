// Spectral harness: (1) aliasing of a live analytic terrain at C6 with a butterfly orbit at
// 1x/2x/4x oversampling; (2) Chebyshev bandlimit claim. Renders exact-integer-cycle signals
// (f0 = fs*k/N) so harmonics land on bins k*h and aliases land off-grid (gcd(k,N)=1).
#include "kernels.h"
#include <complex>
#include <cstdio>
#include <random>
#include <functional>

using cd = std::complex<double>;
static void fft (std::vector<cd>& a)
{
    int n = (int) a.size();
    for (int i = 1, j = 0; i < n; ++i) { int bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[i], a[j]); }
    for (int len = 2; len <= n; len <<= 1)
    {
        double ang = -2 * M_PI / len; cd wl (std::cos (ang), std::sin (ang));
        for (int i = 0; i < n; i += len) { cd w (1); for (int j = 0; j < len / 2; ++j) { cd u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } }
    }
}

static constexpr int N = 65536;          // analysis length (base rate)
static constexpr double fs = 48000.0;
static double dB (double p) { return 10.0 * std::log10 (std::max (p, 1e-300)); }

// Kaiser-windowed sinc lowpass, cutoff fc (cycles/sample), taps odd.
static std::vector<double> kaiserLP (int taps, double fc, double beta)
{
    auto I0 = [] (double x) { double s = 1, t = 1; for (int k = 1; k < 60; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; };
    std::vector<double> h (taps); int M = taps - 1; double sum = 0;
    for (int n = 0; n < taps; ++n)
    {
        double m = n - M / 2.0; double s = (m == 0) ? 2 * fc : std::sin (2 * M_PI * fc * m) / (M_PI * m);
        double w = I0 (beta * std::sqrt (1 - std::pow (2.0 * n / M - 1, 2))) / I0 (beta);
        h[n] = s * w; sum += h[n];
    }
    for (auto& v : h) v /= sum; return h;
}

// Render `gen(theta)` at rate os*fs, decimate by os with FIR, return N base-rate samples (steady state).
static std::vector<double> renderDecimated (const std::function<float (float)>& gen, int k, int os)
{
    const double f0 = fs * k / N; const int P = N * os;            // P = exact period in oversampled samples (k cycles)
    std::vector<double> x (P); double ph = 0, inc = 2 * M_PI * f0 / (fs * os);
    for (int i = 0; i < P; ++i) { x[i] = gen ((float) ph); ph += inc; if (ph >= 2 * M_PI) ph -= 2 * M_PI; }
    std::vector<double> y (N);
    if (os == 1) return x;
    auto h = kaiserLP (1023, 0.46 / os, 12.0); const int half = (int) h.size() / 2;
    for (int i = 0; i < N; ++i)
    {
        int c = i * os; double acc = 0;
        for (int t = 0; t < (int) h.size(); ++t) { int idx = ((c - t + half) % P + P) % P; acc += h[t] * x[idx]; }  // periodic extension
        y[i] = acc;
    }
    return y;
}

struct Spec { double fundPow, maxHarmPow, nonHarmPow, peakNonHarm; int peakBin, maxHarmIdxAbove; };
// harmonics = bins k*h, h>=0; measure bins 1..maxBin (maxBin ~ 22 kHz)
static Spec analyse (const std::vector<double>& y, int k, double relThreshDb, double bandHz = 22000.0)
{
    std::vector<cd> a (N); for (int i = 0; i < N; ++i) a[i] = y[i];
    fft (a);
    int maxBin = (int) (bandHz / fs * N);
    Spec s {}; s.fundPow = std::norm (a[k]); s.maxHarmPow = 0; s.nonHarmPow = 0; s.peakNonHarm = 0; s.peakBin = 0; s.maxHarmIdxAbove = 0;
    for (int b = k; b <= maxBin; b += k) s.maxHarmPow = std::max (s.maxHarmPow, std::norm (a[b]));
    for (int b = 1; b <= maxBin; ++b)
    {
        double p = std::norm (a[b]);
        if (b % k == 0) { if (p > s.maxHarmPow * std::pow (10.0, relThreshDb / 10)) s.maxHarmIdxAbove = b / k; }
        else { s.nonHarmPow += p; if (p > s.peakNonHarm) { s.peakNonHarm = p; s.peakBin = b; } }
    }
    return s;
}

int main()
{
    std::printf ("Aliasing harness: N=%d, fs=%.0f, analysis band 0..22 kHz, exact-cycle rendering\n\n", N, fs);

    // ── Part 1: analytic terrain aliasing at C6 ──
    const int kC6 = 1429;                                    // f0 = 1046.63 Hz
    struct Case { const char* name; std::function<float (float)> gen; };
    Case cases[] = {
        { "butterfly(0.2) + sine-product(a=b=0.5) + tanh", [] (float th) { Pt p = butterfly_std (th, 0.3f); p.x *= 0.2f; p.y *= 0.2f; return saturate (sineProduct_std (p, 0.5f, 0.5f), 1.f); } },
        { "butterfly(0.2) + system-14(a=0.5, aa=24) + tanh", [] (float th) { Pt p = butterfly_std (th, 0.3f); p.x *= 0.2f; p.y *= 0.2f; return saturate (system14_std (p, 0.5f, 0.5f, 0.5f), 1.f); } },
        { "butterfly(0.2) + system-14(a=0.1, aa=9.6) + tanh", [] (float th) { Pt p = butterfly_std (th, 0.3f); p.x *= 0.2f; p.y *= 0.2f; return saturate (system14_std (p, 0.1f, 0.5f, 0.5f), 1.f); } },
        { "epitrochoid-7 + sine-product + tanh",           [] (float th) { Pt p = epitrochoid7_std (th, 0.3f); return saturate (sineProduct_std (p, 0.5f, 0.5f), 1.f); } },
    };
    std::printf ("%-52s %3s %14s %14s %10s\n", "case (C6 = 1046.63 Hz)", "OS", "nonharm/fund", "peak alias", "@Hz");
    for (auto& c : cases)
        for (int os : { 1, 2, 4, 8 })
        {
            auto y = renderDecimated (c.gen, kC6, os);
            auto s = analyse (y, kC6, -100);
            std::printf ("%-52s %2dx %11.1f dB %11.1f dB %9.0f\n", c.name, os, dB (s.nonHarmPow / s.fundPow), dB (s.peakNonHarm / s.fundPow), s.peakBin * fs / N);
        }

    // ── Part 2: Chebyshev bandlimit ──
    std::printf ("\nChebyshev terrain: max harmonic index above -100 dB (rel. strongest harmonic), float render, 1x, no tanh\n");
    ChebTerrain<16> full; std::mt19937 rng (11); std::uniform_real_distribution<float> u (-1.f, 1.f);
    for (auto& r : full.c) for (auto& v : r) v = u (rng) * 0.1f;
    ChebTerrain<16> tot16 = full;                                // total degree n+m <= 16
    for (int n = 0; n < 16; ++n) for (int m = 0; m < 16; ++m) if (n + m > 16) tot16.c[n][m] = 0;
    ChebTerrain<16> tot22 = full;                                // n+m <= 22  (22 * 1046.6 = 23.0 kHz < Nyquist)
    for (int n = 0; n < 16; ++n) for (int m = 0; m < 16; ++m) if (n + m > 22) tot22.c[n][m] = 0;

    const int kLow = 349;                                        // f0 = 255.6 Hz → harmonic 30 = 7.7 kHz
    struct CC { const char* name; const ChebTerrain<16>* t; int k; std::function<Pt (float)> orbit; int expect; };
    CC cc[] = {
        { "16x16 tensor (max n+m=30), circle, 255.6 Hz",   &full,  kLow, [] (float th) { return circle (th); }, 30 },
        { "n+m<=16, circle, 255.6 Hz",                     &tot16, kLow, [] (float th) { return circle (th); }, 16 },
        { "n+m<=16, ellipse 0.8 + offset 0.1, 255.6 Hz",   &tot16, kLow, [] (float th) { return Pt { 0.8f * std::cos (th) + 0.1f, std::sin (th) }; }, 16 },
        { "n+m<=16, epitrochoid-7 orbit, 255.6 Hz (16*8=128 > Nyq)", &tot16, kLow, [] (float th) { return epitrochoid7_std (th, 0.3f); }, 128 },
        { "n+m<=16, epitrochoid-7 orbit, 63.7 Hz (bound 128)", &tot16, 87, [] (float th) { return epitrochoid7_std (th, 0.3f); }, 128 },
        { "16x16 tensor (max 30) at C6 -> 30*f0 = 31 kHz", &full,  kC6,  [] (float th) { return circle (th); }, 30 },
        { "n+m<=22 at C6 -> 22*f0 = 23.0 kHz (truncated)", &tot22, kC6,  [] (float th) { return circle (th); }, 22 },
    };
    std::printf ("%-58s %8s %8s %14s %12s\n", "case", "expect", "maxIdx", "nonharm/max", "peak alias");
    for (auto& c : cc)
    {
        const ChebTerrain<16>* t = c.t; auto orb = c.orbit;
        auto y = renderDecimated ([t, orb] (float th) { return t->eval (orb (th)); }, c.k, 1);
        auto s = analyse (y, c.k, -100, 24000.0);
        std::printf ("%-58s %8d %8d %11.1f dB %9.1f dB\n", c.name, c.expect, s.maxHarmIdxAbove, dB (s.nonHarmPow / s.maxHarmPow), dB (s.peakNonHarm / s.maxHarmPow));
    }
    return 0;
}
