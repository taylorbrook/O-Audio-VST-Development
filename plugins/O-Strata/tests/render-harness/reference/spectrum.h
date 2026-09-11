// FFT, analyse() and the exact-cycle bookkeeping adapted from
// research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/alias.cpp
// (own code, no terrain formulas). Header-only, JUCE-free. Used by the H1 / H2 /
// H4 / H5 / H6 gates and the FUNC-02/03 centroid checks.

#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

namespace spectrum
{
    using cd = std::complex<double>;
    constexpr double kPi = 3.141592653589793;

    inline bool isPow2 (size_t n) { return n != 0 && (n & (n - 1)) == 0; }

    /** In-place radix-2 FFT (N a power of two). */
    inline void fft (std::vector<cd>& a)
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
            const double ang = -2.0 * kPi / double (len);
            const cd wlen (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                cd w (1.0, 0.0);
                for (size_t j = 0; j < len / 2; ++j)
                {
                    const cd u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

    /** Power spectrum |X[b]|² of a real signal windowed by `window` (same length), zero-padded to nfft. */
    inline std::vector<double> powerSpectrum (const double* x, size_t n, size_t nfft, const std::vector<double>* window = nullptr)
    {
        std::vector<cd> a (nfft, cd (0.0, 0.0));
        for (size_t i = 0; i < n && i < nfft; ++i)
            a[i] = cd (window != nullptr ? x[i] * (*window)[i] : x[i], 0.0);
        fft (a);
        std::vector<double> p (nfft / 2 + 1);
        for (size_t b = 0; b < p.size(); ++b)
            p[b] = std::norm (a[b]);
        return p;
    }

    inline std::vector<double> hann (size_t n)
    {
        std::vector<double> w (n);
        for (size_t i = 0; i < n; ++i)
            w[i] = 0.5 - 0.5 * std::cos (2.0 * kPi * double (i) / double (n));
        return w;
    }

    inline double db (double power, double ref) { return 10.0 * std::log10 (std::max (power, 1e-300) / std::max (ref, 1e-300)); }

    // ─── Exact-cycle analysis (bench alias.cpp Part 1 + Part 2 conventions) ───
    struct ExactCycle
    {
        double fundPow = 0.0;          // |X[k]|²
        double maxHarmPow = 0.0;       // strongest |X[k·h]|² inside the band
        double nonHarmPow = 0.0;       // Σ |X[b]|² over b mod k ≠ 0 inside the band, b ≥ 1
        int    peakNonHarmBin = -1;
        int    maxHarmIdxAboveMinus100dB = 0;   // highest h with |X[kh]|² ≥ maxHarmPow·1e−10
        double nonHarmOverFund() const { return db (nonHarmPow, fundPow); }
        double nonHarmOverMax()  const { return db (nonHarmPow, maxHarmPow); }
    };

    /** y holds exactly k cycles in N samples (rectangular window over a true period);
        band limits the analysis to bins [1, bandBins]. */
    inline ExactCycle analyseExactCycle (const double* y, size_t N, int k, size_t bandBins)
    {
        std::vector<cd> a (N);
        for (size_t i = 0; i < N; ++i) a[i] = cd (y[i], 0.0);
        fft (a);
        ExactCycle r;
        const size_t top = std::min (bandBins, N / 2);
        r.fundPow = std::norm (a[(size_t) k]);
        for (size_t b = 1; b <= top; ++b)
        {
            const double p = std::norm (a[b]);
            if (b % (size_t) k == 0)
                r.maxHarmPow = std::max (r.maxHarmPow, p);
            else
            {
                r.nonHarmPow += p;
                if (r.peakNonHarmBin < 0 || p > std::norm (a[(size_t) r.peakNonHarmBin])) r.peakNonHarmBin = (int) b;
            }
        }
        for (size_t b = (size_t) k; b <= top; b += (size_t) k)
            if (std::norm (a[b]) >= r.maxHarmPow * 1e-10)
                r.maxHarmIdxAboveMinus100dB = (int) (b / (size_t) k);
        return r;
    }

    // ─── Windowed helpers ───

    /** Hann-windowed power spectrum of one frame. */
    inline std::vector<double> windowedSpectrum (const double* x, size_t frame, size_t nfft)
    {
        const auto w = hann (frame);
        return powerSpectrum (x, frame, nfft, &w);
    }

    /** Number of harmonic peaks above `thresholdDb` relative to the strongest bin: the
        spectrum is scanned at multiples of f0 (± 1 bin) up to fmax. */
    inline int partialsAbove (const std::vector<double>& spec, double binHz, double f0, double fmax, double thresholdDb)
    {
        double maxP = 0.0;
        for (size_t b = 1; b < spec.size(); ++b) maxP = std::max (maxP, spec[b]);
        int count = 0;
        for (int h = 1; h * f0 < fmax; ++h)
        {
            const double bc = h * f0 / binHz;
            const int b0 = std::max (1, (int) std::floor (bc) - 1), b1 = std::min ((int) spec.size() - 1, (int) std::ceil (bc) + 1);
            double p = 0.0;
            for (int b = b0; b <= b1; ++b) p = std::max (p, spec[(size_t) b]);
            if (db (p, maxP) >= thresholdDb) ++count;
        }
        return count;
    }

    /** Power-weighted spectral centroid in Hz over bins [1, fmax]. */
    inline double spectralCentroid (const std::vector<double>& spec, double binHz, double fmax)
    {
        double num = 0.0, den = 0.0;
        for (size_t b = 1; b < spec.size() && b * binHz <= fmax; ++b) { num += b * binHz * spec[b]; den += spec[b]; }
        return den > 0.0 ? num / den : 0.0;
    }

    /** Peak bin with parabolic interpolation (Hz). Searches [fmin, fmax]. */
    inline double peakFrequencyInterpolated (const std::vector<double>& spec, double binHz, double fmin, double fmax)
    {
        size_t best = 1; double bp = -1.0;
        for (size_t b = 1; b + 1 < spec.size(); ++b)
        {
            const double f = b * binHz;
            if (f < fmin || f > fmax) continue;
            if (spec[b] > bp) { bp = spec[b]; best = b; }
        }
        // parabolic interpolation on the log-magnitude
        const double a = 0.5 * std::log10 (std::max (spec[best - 1], 1e-300));
        const double c0 = 0.5 * std::log10 (std::max (spec[best], 1e-300));
        const double c = 0.5 * std::log10 (std::max (spec[best + 1], 1e-300));
        const double denom = a - 2.0 * c0 + c;
        const double delta = std::abs (denom) > 1e-12 ? 0.5 * (a - c) / denom : 0.0;
        return (double (best) + delta) * binHz;
    }

    /** Max power in the bins around f (± halfWidth Hz). */
    inline double peakPowerNear (const std::vector<double>& spec, double binHz, double f, double halfWidth)
    {
        double p = 0.0;
        const int b0 = std::max (1, (int) std::floor ((f - halfWidth) / binHz)), b1 = std::min ((int) spec.size() - 1, (int) std::ceil ((f + halfWidth) / binHz));
        for (int b = b0; b <= b1; ++b) p = std::max (p, spec[(size_t) b]);
        return p;
    }

    inline double cents (double f, double ref) { return 1200.0 * std::log2 (f / ref); }
}
