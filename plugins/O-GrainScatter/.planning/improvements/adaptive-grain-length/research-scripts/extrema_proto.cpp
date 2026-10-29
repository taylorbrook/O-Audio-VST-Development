// O-GrainScatter adaptive-grain-length research prototype (Nielsen DAFx26 extrema density).
// Standalone C++17, no JUCE.   c++ -std=c++17 -O2 extrema_proto.cpp -o extrema_proto && ./extrema_proto
//
// What it measures
//   1. Engage cost: loop-signal build (mid, seam crossfade exactly as FreezeManager::locate)
//      + extrema analysis (B-spline derivative, eps hysteresis, reverse-linear sub-sample
//      position), at 48 k and 96 k over a 2 s capture; one-shot vs chunked (bit-identical).
//   2. Extremum count M / N on sine, noise, silence, quiet noise, music-ish, adversarial.
//   3. Calibration: fixed measurement window (K0 so the time-median span = Tref = 100 ms),
//      log-domain morph len = clamp(GS * exp(a * (ln L - ln Lmed) + c)), c solved per block
//      so the mean over P = 256 read points = GS. Mean error over a dense grid, sparse/dense
//      ratio, Grain Size proportionality, Jensen shortfall without the c solve.
//   4. Per-spawn lookup cost (binary search + fractional-K span + exp/log).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

namespace
{
constexpr float kEps = 0.001f;   // -60 dB (paper 2.2)

// ── Loop signal: what FreezeManager plays, mid = (L+R)/2, one period [xfade, length) ──
struct Snapshot { std::vector<float> L, R; int length = 0, xfade = 0, loopLength = 0; };

Snapshot makeSnapshot (const std::vector<float>& l, const std::vector<float>& r, double fs)
{
    Snapshot s; s.L = l; s.R = r; s.length = (int) l.size();
    s.xfade = std::min ((int) (fs * 0.010), s.length / 4);   // FreezeManager: min(maxXfade, len/4)
    s.loopLength = s.length - s.xfade;
    return s;
}

// y[i] = loop signal at snapshot position b = xfade + i, i in [0, loopLength)
void buildLoop (const Snapshot& s, float* y)
{
    const int seamStart = s.length - s.xfade;
    const float halfPi = 1.57079632679489661923f;
    for (int i = 0; i < s.loopLength; ++i)
    {
        const int b = s.xfade + i;
        float v = 0.5f * (s.L[(size_t) b] + s.R[(size_t) b]);
        if (s.xfade > 0 && b >= seamStart)
        {
            const float fade = (float) (b - seamStart) / (float) s.xfade;
            const int b2 = b - s.loopLength;
            const float v2 = 0.5f * (s.L[(size_t) b2] + s.R[(size_t) b2]);
            v = std::cos (fade * halfPi) * v + std::sin (fade * halfPi) * v2;
        }
        y[i] = v;
    }
}

// ── Extrema analysis, resumable (chunked == one-shot, bit for bit) ──
// Periodic signal y[0..n). Derivative of the cubic B-spline at the knot (t = 0, eq. 3):
// b'(-1) = -1/2, b'(0) = 0, b'(1) = +1/2, b'(2) = 0  ->  d[i] = (y[i+1] - y[i-1]) / 2,
// zero at Nyquist. Sign change of d between i-1 and i (sign of the last NONZERO d, so a
// run of exact zeros - silence - is not an extremum) -> candidate. alpha = |d[i-1]| /
// (|d[i-1]| + |d[i]|), n_m = (i - 1) + alpha (eq. 4-5). v_m = B-spline value at n_m (eq. 6).
// Kept iff |v_m - v_prev| > eps (hysteresis against the last KEPT extremum, 2.2).
struct Analyzer
{
    const float* y = nullptr; int n = 0;
    float* out = nullptr; int cap = 0;
    int i = 0, M = 0; float dPrev = 0.0f; int sPrev = 0; float vPrev = 0.0f;

    void start (const float* yy, int nn, float* o, int c)
    { y = yy; n = nn; out = o; cap = c; i = 0; M = 0; dPrev = 0.0f; sPrev = 0; vPrev = n > 0 ? y[0] : 0.0f; }

    float at (int k) const { k %= n; if (k < 0) k += n; return y[k]; }

    float bspline (double pos) const   // uniform cubic B-spline value at pos (eq. 1-2)
    {
        const int k = (int) std::floor (pos); const float t = (float) (pos - k);
        const float u = 1.0f - t, t2 = t * t, t3 = t2 * t;
        const float b0 = u * u * u * (1.0f / 6.0f), b1 = (3.0f * t3 - 6.0f * t2 + 4.0f) * (1.0f / 6.0f);
        const float b2 = (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) * (1.0f / 6.0f), b3 = t3 * (1.0f / 6.0f);
        if (k >= 1 && k + 2 < n)   // fast path: no wrap
            return y[k - 1] * b0 + y[k] * b1 + y[k + 1] * b2 + y[k + 2] * b3;
        return at (k - 1) * b0 + at (k) * b1 + at (k + 1) * b2 + at (k + 2) * b3;
    }

    // processes up to `count` samples; returns true when done
    bool step (int count)
    {
        const int end = std::min (n, i + count);
        for (; i < end; ++i)
        {
            const float ym = (i == 0) ? y[n - 1] : y[i - 1];
            const float yp = (i == n - 1) ? y[0] : y[i + 1];
            const float d = 0.5f * (yp - ym);
            const int s = (d > 0.0f) - (d < 0.0f);
            if (s != 0)
            {
                if (sPrev != 0 && s != sPrev)
                {
                    const float a0 = std::fabs (dPrev), a1 = std::fabs (d);
                    // dPrev is the last NONZERO derivative; with zeros between, the
                    // crossing sits somewhere in that run - use its last sample as i-1.
                    const double nm = (double) (i - 1) + (double) (a0 / (a0 + a1));
                    const float vm = bspline (nm);
                    if (std::fabs (vm - vPrev) > kEps && M < cap)
                    {
                        double p = nm < 0.0 ? nm + n : nm;
                        out[M++] = (float) p;
                        vPrev = vm;
                    }
                }
                sPrev = s; dPrev = d;
            }
        }
        return i >= n;
    }
};

// ── Span lookup: L(p) = n[m+K] - n[m], m = last extremum <= p, fractional K, wrap ──
struct Map
{
    const float* pos = nullptr; int M = 0; double lp = 0;

    double nAt (long j) const   // unwrapped position of extremum j (any integer)
    {
        long q = j / M, r = j % M; if (r < 0) { r += M; --q; }
        return (double) pos[r] + (double) q * lp;
    }
    long indexAtOrBefore (double p) const
    {
        const long u = (long) (std::upper_bound (pos, pos + M, (float) p) - pos);
        return u - 1;   // -1 = the last extremum of the previous period
    }
    double span (double p, double K) const
    {
        const long m = indexAtOrBefore (p);
        const long k0 = (long) K; const double f = K - (double) k0;
        const double a = nAt (m + k0), b = nAt (m + k0 + 1);
        return (1.0 - f) * a + f * b - nAt (m);
    }
    // reverse grain: span of the K extrema BEHIND p (m = first extremum >= p, going back)
    double spanBack (double p, double K) const
    {
        long m = (long) (std::lower_bound (pos, pos + M, (float) p) - pos);   // first >= p
        const long k0 = (long) K; const double f = K - (double) k0;
        const double a = nAt (m - k0), b = nAt (m - k0 - 1);
        return nAt (m) - ((1.0 - f) * a + f * b);
    }
};

// ── Calibration (scheme B): K0 per snapshot, c per block ──
struct Calib
{
    static constexpr int P = 256;
    double K0 = 0, lnMed = 0; std::vector<double> lr;   // sorted ln(L/Lmed)
    bool valid = false;

    void build (const Map& mp, double fs, double TrefMs)
    {
        valid = false; lr.assign (P, 0.0);
        if (mp.M < 8 || mp.lp < 0.2 * fs) return;    // fallback: fixed length
        const double Tref = TrefMs * 1e-3 * fs;
        double K = std::max (1.0, (double) mp.M * Tref / mp.lp);   // count-based first guess
        std::vector<double> L (P);
        for (int it = 0; it < 4; ++it)
        {
            for (int j = 0; j < P; ++j) L[(size_t) j] = mp.span ((j + 0.5) * mp.lp / P, K);
            std::vector<double> t = L; std::nth_element (t.begin(), t.begin() + P / 2, t.end());
            const double med = t[P / 2];
            K = std::clamp (K * Tref / std::max (med, 1e-9), 1.0, (double) mp.M - 2.0);
        }
        K0 = K;
        for (int j = 0; j < P; ++j) L[(size_t) j] = mp.span ((j + 0.5) * mp.lp / P, K0);
        std::vector<double> t = L; std::nth_element (t.begin(), t.begin() + P / 2, t.end());
        lnMed = std::log (t[P / 2]);
        for (int j = 0; j < P; ++j) lr[(size_t) j] = std::log (L[(size_t) j]) - lnMed;
        std::sort (lr.begin(), lr.end());
        valid = true;
    }

    // mean over the P points of clamp(gs * exp(a * lr + c), lo, hi)
    double meanFor (double gs, double a, double c, double lo, double hi) const
    {
        double s = 0; for (double v : lr) s += std::clamp (gs * std::exp (a * v + c), lo, hi);
        return s / P;
    }
    // per block: solve meanFor(...) = gs (monotone in c) - bisection, 30 steps
    double solveC (double gs, double a, double lo, double hi) const
    {
        double c0 = -12, c1 = 12;
        for (int it = 0; it < 30; ++it) { const double cm = 0.5 * (c0 + c1); (meanFor (gs, a, cm, lo, hi) < gs ? c0 : c1) = cm; }
        return 0.5 * (c0 + c1);
    }
    // per block, cheap: E_j = exp(a lr_j) once (256 exp), prefix sums, then each F(c)
    // evaluation is two binary searches (sorted lr, a > 0 -> E ascending). 40 bisection steps.
    double solveCFast (double gs, double a, double lo, double hi) const
    {
        double E[P], S[P + 1]; S[0] = 0;
        for (int j = 0; j < P; ++j) { E[j] = std::exp (a * lr[(size_t) j]); S[j + 1] = S[j] + E[j]; }
        auto F = [&] (double c) {
            const double g = gs * std::exp (c);
            const int iLo = (int) (std::lower_bound (E, E + P, lo / g) - E);   // E_j * g >= lo from here
            const int iHi = (int) (std::upper_bound (E, E + P, hi / g) - E);   // E_j * g >  hi from here
            return (lo * iLo + g * (S[iHi] - S[iLo]) + hi * (P - iHi)) / P;
        };
        double c0 = -12, c1 = 12;
        for (int it = 0; it < 40; ++it) { const double cm = 0.5 * (c0 + c1); (F (cm) < gs ? c0 : c1) = cm; }
        return 0.5 * (c0 + c1);
    }
    // closed form, unclamped: mean exp(a lr + c) = 1
    double closedC (double a) const { double s = 0; for (double v : lr) s += std::exp (a * v); return -std::log (s / P); }
};

double nowUs() { using namespace std::chrono; return duration<double, std::micro> (steady_clock::now().time_since_epoch()).count(); }

// ── stimuli (stereo, 2 s + nothing else) ──
enum class Stim { Sine55, Sine1k, Noise, Silence, Noise80, Noise50, SparseDense, BassSnare, Music, Fs4, AdvNyq };
const char* stimName (Stim s)
{
    switch (s) { case Stim::Sine55: return "sine 55 Hz 0.5"; case Stim::Sine1k: return "sine 1 kHz 0.5";
        case Stim::Noise: return "white noise +-0.3"; case Stim::Silence: return "silence";
        case Stim::Noise80: return "noise +-1e-4 (-80 dB)"; case Stim::Noise50: return "noise +-3e-3 (-50 dB)";
        case Stim::SparseDense: return "1 s sine 55 | 1 s noise (eq rms)"; case Stim::BassSnare: return "bass 60 Hz + snare bursts";
        case Stim::Music: return "skip-tilt 'music' stimulus"; case Stim::Fs4: return "sine fs/4 (M=N/2 case)";
        case Stim::AdvNyq: return "adversarial decaying Nyquist square"; }
    return "?";
}

void makeStim (Stim k, double fs, int N, std::vector<float>& l, std::vector<float>& r)
{
    l.assign ((size_t) N, 0.0f); r.assign ((size_t) N, 0.0f);
    std::mt19937 g (0x5EED); std::uniform_real_distribution<float> u (-1.0f, 1.0f);
    const double tp = 6.283185307179586;
    for (int i = 0; i < N; ++i)
    {
        const double t = i / fs; float a = 0, b = 0;
        switch (k)
        {
            case Stim::Sine55: a = b = (float) (0.5 * std::sin (tp * 55 * t)); break;
            case Stim::Sine1k: a = b = (float) (0.5 * std::sin (tp * 1000 * t)); break;
            case Stim::Noise: a = 0.3f * u (g); b = 0.3f * u (g); break;
            case Stim::Silence: break;
            case Stim::Noise80: a = 1e-4f * u (g); b = 1e-4f * u (g); break;
            case Stim::Noise50: a = 3e-3f * u (g); b = 3e-3f * u (g); break;
            case Stim::SparseDense:
                if (i < N / 2) a = b = (float) (0.5 * std::sin (tp * 55 * t));
                else { a = 0.612f * u (g); b = a; }   // same rms as the sine (0.354), mono so mid keeps it
                break;
            case Stim::BassSnare:
            {
                const double ph = std::fmod (t, 0.5);
                double v = 0.5 * std::exp (-ph * 3.0) * std::sin (tp * 60 * t);
                const double ps = std::fmod (t + 0.25, 0.5);
                v += 0.4 * std::exp (-ps / 0.06) * u (g);
                a = b = (float) v; break;
            }
            case Stim::Music:
            {
                const double env = 0.5 + 0.5 * std::sin (tp * 0.7 * t);
                const float nn = 0.05f * u (g);
                a = (float) (0.3 * env * std::sin (tp * 220 * t) + 0.1 * std::sin (tp * 1337 * t)) + nn;
                b = (float) (0.3 * env * std::sin (tp * 330 * t) + 0.1 * std::sin (tp * 911 * t)) - nn;
                break;
            }
            case Stim::Fs4: a = b = (float) (0.5 * std::sin (tp * fs / 4 * t + 0.3)); break;
            case Stim::AdvNyq:
            {   // even samples 0.9 - delta n, odd -0.9 + delta n: d alternates sign EVERY sample
                const float dl = 1.0e-6f; const float v = (i & 1) ? (-0.9f + dl * i) : (0.9f - dl * i);
                a = b = v; break;
            }
        }
        l[(size_t) i] = a; r[(size_t) i] = b;
    }
}
} // namespace

int main()
{
    std::printf ("== 1. engage cost (2 s capture, median of 41 runs, Apple M4 Max, -O2) ==\n");
    for (double fs : { 48000.0, 96000.0 })
    {
        const int N = (int) (fs * 2.0) + 1;   // PluginProcessor: maxFreezeSamples
        for (Stim k : { Stim::Noise, Stim::Music, Stim::Sine55, Stim::AdvNyq })
        {
            std::vector<float> l, r; makeStim (k, fs, N, l, r);
            Snapshot s = makeSnapshot (l, r, fs);
            std::vector<float> y ((size_t) s.loopLength), ext ((size_t) s.loopLength);
            std::vector<double> tb, ta, tc, tcal;
            int M = 0; double K0 = 0;
            for (int rep = 0; rep < 41; ++rep)
            {
                double t0 = nowUs(); buildLoop (s, y.data()); double t1 = nowUs();
                Analyzer an; an.start (y.data(), s.loopLength, ext.data(), (int) ext.size()); an.step (s.loopLength);
                double t2 = nowUs(); M = an.M;
                Map mp { ext.data(), M, (double) s.loopLength };
                Calib cb; cb.build (mp, fs, 100.0); double t3 = nowUs(); K0 = cb.K0;
                // chunked: 64 x 32-sample blocks' worth per step = 2048
                Analyzer a2; a2.start (y.data(), s.loopLength, ext.data(), (int) ext.size());
                std::vector<float> ext2 = ext;
                a2.out = ext2.data();
                double t4 = nowUs(); while (! a2.step (2048)) {} double t5 = nowUs();
                if (rep == 0 && (a2.M != M || ! std::equal (ext.begin(), ext.begin() + M, ext2.begin())))
                    std::printf ("  CHUNKED MISMATCH\n");
                tb.push_back (t1 - t0); ta.push_back (t2 - t1); tcal.push_back (t3 - t2); tc.push_back (t5 - t4);
            }
            auto med = [] (std::vector<double> v) { std::nth_element (v.begin(), v.begin() + v.size() / 2, v.end()); return v[v.size() / 2]; };
            std::printf ("  %5.0f k %-36s build %6.1f us  extrema %6.1f us  (chunked 2048: %6.1f)  calib %5.1f us  M=%6d (M/N %.3f) K0=%.1f\n",
                         fs / 1000, stimName (k), med (tb), med (ta), med (tc), med (tcal), M, (double) M / s.loopLength, K0);
        }
    }

    std::printf ("\n== 2. extremum counts, 48 k, 2 s ==\n");
    const double fs = 48000.0; const int N = 96001;
    for (Stim k : { Stim::Sine55, Stim::Sine1k, Stim::Noise, Stim::Silence, Stim::Noise80, Stim::Noise50,
                    Stim::SparseDense, Stim::BassSnare, Stim::Music, Stim::Fs4, Stim::AdvNyq })
    {
        std::vector<float> l, r; makeStim (k, fs, N, l, r);
        Snapshot s = makeSnapshot (l, r, fs);
        std::vector<float> y ((size_t) s.loopLength), ext ((size_t) s.loopLength);
        buildLoop (s, y.data());
        Analyzer an; an.start (y.data(), s.loopLength, ext.data(), (int) ext.size()); an.step (s.loopLength);
        std::printf ("  %-38s M = %6d   M/N = %.4f   mean gap %.1f samples\n", stimName (k), an.M,
                     (double) an.M / s.loopLength, an.M ? (double) s.loopLength / an.M : 0.0);
    }

    std::printf ("\n== 3. calibration: Tref 100 ms, clamp 10..500 ms, P = 256, truth = 8192-point grid ==\n");
    const double lo = 0.010 * fs, hi = 0.500 * fs;
    for (Stim k : { Stim::Sine55, Stim::Noise, Stim::SparseDense, Stim::BassSnare, Stim::Music })
    {
        std::vector<float> l, r; makeStim (k, fs, N, l, r);
        Snapshot s = makeSnapshot (l, r, fs);
        std::vector<float> y ((size_t) s.loopLength), ext ((size_t) s.loopLength);
        buildLoop (s, y.data());
        Analyzer an; an.start (y.data(), s.loopLength, ext.data(), (int) ext.size()); an.step (s.loopLength);
        Map mp { ext.data(), an.M, (double) s.loopLength };
        Calib cb; cb.build (mp, fs, 100.0);
        std::printf (" %s  (M %d, K0 %.1f, valid %d)\n", stimName (k), an.M, cb.K0, (int) cb.valid);
        if (! cb.valid) continue;
        const int G = 8192;
        std::vector<double> lnL (G);
        for (int j = 0; j < G; ++j) lnL[(size_t) j] = std::log (mp.span ((j + 0.37) * mp.lp / G, cb.K0)) - cb.lnMed;
        for (double gsMs : { 20.0, 50.0, 100.0, 200.0, 400.0 })
            for (double a : { 0.25, 0.5, 1.0 })
            {
                const double gs = gsMs * 1e-3 * fs;
                const double c = cb.solveC (gs, a, lo, hi), c0 = cb.closedC (a);
                double sum = 0, sum0 = 0, sA = 0, sB = 0, nA = 0, nB = 0, mn = 1e30, mx = 0;
                for (int j = 0; j < G; ++j)
                {
                    const double len = std::clamp (gs * std::exp (a * lnL[(size_t) j] + c), lo, hi);
                    sum += len; sum0 += std::clamp (gs * std::exp (a * lnL[(size_t) j] + c0), lo, hi);
                    mn = std::min (mn, len); mx = std::max (mx, len);
                    if (j < G / 2) { sA += len; ++nA; } else { sB += len; ++nB; }
                }
                std::printf ("   GS %5.0f ms a %.2f: mean %6.1f ms (%+5.1f %%)  [no c-solve: %+6.1f %%]  range %5.1f..%5.1f ms  1st/2nd half %.2f\n",
                             gsMs, a, sum / G / fs * 1e3, 100.0 * (sum / G / gs - 1.0), 100.0 * (sum0 / G / gs - 1.0),
                             mn / fs * 1e3, mx / fs * 1e3, (sA / nA) / (sB / nB));
            }
        // CONTEXT's first guess: K = GS * M / len (count-based), len = clamp(span_K), a = 1
        for (double gsMs : { 50.0, 100.0, 200.0 })
        {
            const double gs = gsMs * 1e-3 * fs; const double K = std::max (1.0, gs * mp.M / mp.lp);
            double sum = 0; for (int j = 0; j < G; ++j) sum += std::clamp (mp.span ((j + 0.37) * mp.lp / G, K), lo, hi);
            std::printf ("   [CONTEXT K = GS*M/len] GS %3.0f ms: K %.1f, mean %6.1f ms (%+6.1f %%)\n", gsMs, K, sum / G / fs * 1e3, 100.0 * (sum / G / gs - 1.0));
        }
        {   // 100 % reverse grains: span measured backwards, same calibration
            const double gs = 0.1 * fs, c = cb.solveC (gs, 1.0, lo, hi); double sum = 0;
            for (int j = 0; j < G; ++j) sum += std::clamp (gs * std::exp (std::log (mp.spanBack ((j + 0.37) * mp.lp / G, cb.K0)) - cb.lnMed + c), lo, hi);
            std::printf ("   reverse (backward span, forward calibration) GS 100 a 1: mean %6.1f ms (%+5.1f %%)\n", sum / G / fs * 1e3, 100.0 * (sum / G / gs - 1.0));
        }
        // proportionality: per-point ratio len(200)/len(100) at a = 1, over unclamped points
        {
            const double g1 = 0.1 * fs, g2 = 0.2 * fs;
            const double c1 = cb.solveC (g1, 1.0, lo, hi), c2 = cb.solveC (g2, 1.0, lo, hi);
            double rmin = 1e9, rmax = 0; int cnt = 0;
            for (int j = 0; j < G; ++j)
            {
                const double a1 = g1 * std::exp (lnL[(size_t) j] + c1), a2 = g2 * std::exp (lnL[(size_t) j] + c2);
                if (a1 > lo && a1 < hi && a2 > lo && a2 < hi) { const double q = a2 / a1; rmin = std::min (rmin, q); rmax = std::max (rmax, q); ++cnt; }
            }
            std::printf ("   proportionality GS 100 -> 200, a 1: unclamped points %d / %d, ratio %.4f .. %.4f\n", cnt, G, rmin, rmax);
        }
    }

    std::printf ("\n== 3b. reverse span vs forward span (sparse-dense, K0) ==\n");
    {
        std::vector<float> l, r; makeStim (Stim::SparseDense, fs, N, l, r);
        Snapshot s = makeSnapshot (l, r, fs);
        std::vector<float> y ((size_t) s.loopLength), ext ((size_t) s.loopLength);
        buildLoop (s, y.data());
        Analyzer an; an.start (y.data(), s.loopLength, ext.data(), (int) ext.size()); an.step (s.loopLength);
        Map mp { ext.data(), an.M, (double) s.loopLength };
        Calib cb; cb.build (mp, fs, 100.0);
        for (double tMs : { 250.0, 750.0, 1250.0, 1750.0 })
        {
            const double p = tMs * 1e-3 * fs - s.xfade;
            std::printf ("   p %6.0f ms: forward span %8.1f ms  backward span %8.1f ms\n", tMs,
                         mp.span (p, cb.K0) / fs * 1e3, mp.spanBack (p, cb.K0) / fs * 1e3);
        }
    }

    std::printf ("\n== 4. per-spawn lookup cost (noise snapshot, M ~ 64 k) ==\n");
    {
        std::vector<float> l, r; makeStim (Stim::Noise, fs, N, l, r);
        Snapshot s = makeSnapshot (l, r, fs);
        std::vector<float> y ((size_t) s.loopLength), ext ((size_t) s.loopLength);
        buildLoop (s, y.data());
        Analyzer an; an.start (y.data(), s.loopLength, ext.data(), (int) ext.size()); an.step (s.loopLength);
        Map mp { ext.data(), an.M, (double) s.loopLength };
        Calib cb; cb.build (mp, fs, 100.0);
        const double gs = 0.1 * fs, c = cb.solveC (gs, 1.0, lo, hi);
        volatile double sink = 0; const int R = 200000; std::mt19937 g (1);
        std::uniform_real_distribution<double> u (0.0, mp.lp);
        std::vector<double> ps (R); for (auto& p : ps) p = u (g);
        double t0 = nowUs();
        for (int i = 0; i < R; ++i) sink = sink + std::clamp (gs * std::exp (1.0 * (std::log (mp.span (ps[(size_t) i], cb.K0)) - cb.lnMed) + c), lo, hi);
        double t1 = nowUs();
        std::printf ("   %.1f ns per spawn (binary search over M = %d + span + log/exp)\n", (t1 - t0) * 1e3 / R, an.M);
        double t2 = nowUs(); double cc = 0; for (int i = 0; i < 100; ++i) cc += cb.solveC (gs * (1 + i * 1e-4), 0.7, lo, hi); double t3 = nowUs();
        std::printf ("   %.1f us per block c-solve (30 bisection steps x 256 exp) [%g]\n", (t3 - t2) / 100, cc * 0);
        double t4 = nowUs(); volatile double cf = 0; for (int i = 0; i < 1000; ++i) cf = cf + cb.solveCFast (gs * (1 + i * 1e-5), 0.7, lo, hi); double t5 = nowUs();
        std::printf ("   %.2f us per block fast c-solve (256 exp + prefix sums + 40 x 2 binary searches); |c_fast - c_slow| = %.2e\n",
                     (t5 - t4) / 1000, std::fabs (cb.solveCFast (gs, 0.7, lo, hi) - cb.solveC (gs, 0.7, lo, hi)));
    }
    return 0;
}
