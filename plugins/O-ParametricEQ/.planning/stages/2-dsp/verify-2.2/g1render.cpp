// Rendered half of gate G1 (verify 2.2, written in the verify session).
// Bells on the G1 grid at 44.1 kHz: the impulse response of eq::Band, rendered in
// double, against (a) the TRUE analog prototype at the Phase 2.1b tolerance and
// (b) the designer's closed form. Shelves: (b) asserted, (a) printed.
#include "dsp/EqTypes.h"
#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/Band.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

struct Result { double worstAnalog = 0, worstTolRatio = 0, worstClosed = 0, worstDesignAnalog = 0; int n = 0, capped = 0; double longest = 0; };

static void measure (const eq::Designer& des, eq::BandSettings s, double fs, double tolA, double tolB, Result& r)
{
    eq::BandDesign d;
    des.design (s, d);
    eq::Band band;
    band.setDesign (d);
    band.reset();

    const int nf = 64;
    double w[nf], cr[nf], ci[nf], pr[nf], pi_[nf], ar[nf] = {}, ai[nf] = {};
    for (int k = 0; k < nf; ++k)
    {
        const double f = 20.0 * std::pow (1000.0, k / double (nf - 1));
        w[k] = 2.0 * eq::pi * f / fs;
        cr[k] = std::cos (w[k]); ci[k] = -std::sin (w[k]);
        pr[k] = 1.0; pi_[k] = 0.0;
    }

    const int chunk = (int) (fs * 0.25), tailLen = (int) (fs * 0.200);
    std::vector<double> x ((size_t) chunk);
    double peak = 0.0; bool done = false; int chunks = 0;
    while (! done && chunks < 800)
    {
        std::fill (x.begin(), x.end(), 0.0);
        if (chunks == 0) x[0] = 1.0;
        band.processLane (0, x.data(), chunk);
        double tail = 0.0;
        for (int i = 0; i < chunk; ++i)
        {
            const double v = x[(size_t) i];
            peak = std::max (peak, std::abs (v));
            if (i >= chunk - tailLen) tail = std::max (tail, std::abs (v));
            for (int k = 0; k < nf; ++k)
            {
                ar[k] += v * pr[k]; ai[k] += v * pi_[k];
                const double t = pr[k] * cr[k] - pi_[k] * ci[k];
                pi_[k] = pr[k] * ci[k] + pi_[k] * cr[k];
                pr[k] = t;
            }
        }
        // Renormalise the phasors against drift.
        for (int k = 0; k < nf; ++k) { const double m = std::sqrt (pr[k] * pr[k] + pi_[k] * pi_[k]); pr[k] /= m; pi_[k] /= m; }
        ++chunks;
        done = tail <= peak * 1e-15;
    }
    if (! done) ++r.capped;
    r.longest = std::max (r.longest, chunks * 0.25);

    const double tol = tolA + tolB * std::abs (s.gainDbEff);
    for (int k = 0; k < nf; ++k)
    {
        const double rendered = 10.0 * std::log10 (ar[k] * ar[k] + ai[k] * ai[k]);
        const double closed = eq::bandMagDb (d, w[k]);
        const double analog = eq::analogBandMagDb (d, w[k]);
        r.worstClosed = std::max (r.worstClosed, std::abs (rendered - closed));
        r.worstDesignAnalog = std::max (r.worstDesignAnalog, std::abs (closed - analog));
        const double ea = std::abs (rendered - analog);
        r.worstAnalog = std::max (r.worstAnalog, ea);
        r.worstTolRatio = std::max (r.worstTolRatio, ea / tol);
    }
    ++r.n;
}

int main()
{
    const double fs = 44100.0;
    eq::Designer des;
    des.prepare (fs);

    const double gains[5] = { 3, 6, 12, 18, 30 };
    const double qs[7] = { 0.025, 0.1, 0.7071, 1.0, 4.0, 10.0, 40.0 };
    int fails = 0;

    std::printf ("Rendered G1 at 44.1 kHz (double engine, impulse, 64 points 20 Hz - 20 kHz, last 200 ms >= 300 dB down)\n\n-- bells, fc 20 Hz - 20 kHz x 40, Q 0.025 - 40 (7 values): rendered vs the TRUE analog prototype, 0.05 + 0.012 |g|\n");
    for (int gi = 0; gi < 5; ++gi)
    {
        Result res[2];
        std::vector<std::thread> th;
        for (int sign = 0; sign < 2; ++sign)
            th.emplace_back ([&, sign]
            {
                for (int fi = 0; fi < 40; ++fi)
                    for (double q : qs)
                    {
                        eq::BandSettings s { eq::Type::bell, 20.0 * std::pow (1000.0, fi / 39.0), (sign ? -1.0 : 1.0) * gains[gi], q, 0 };
                        measure (des, s, fs, 0.05, 0.012, res[sign]);
                    }
            });
        for (auto& t : th) t.join();
        for (int sign = 0; sign < 2; ++sign)
        {
            const auto& r = res[sign];
            const bool ok = r.worstTolRatio <= 1.0 && r.worstClosed <= 1e-6 && r.capped == 0;
            if (! ok) ++fails;
            std::printf ("  [%s] %+5.0f dB: %d bells; rendered vs analog worst %.4f dB (tolerance %.3f, %.0f %% of it); design vs analog %.4f; rendered vs closed form %.2e dB; longest render %.2f s, capped %d\n",
                         ok ? "PASS" : "FAIL", (sign ? -1.0 : 1.0) * gains[gi], r.n, r.worstAnalog, 0.05 + 0.012 * gains[gi], 100.0 * r.worstTolRatio, r.worstDesignAnalog, r.worstClosed, r.longest, r.capped);
        }
    }

    std::printf ("\n-- shelves, fc 20 Hz - 20 kHz x 40, +-6 / +-12 / +-30 dB: rendered vs the closed form <= 1e-6 dB (rendered vs analog printed)\n");
    const struct { eq::Type t; const char* name; } types[3] = { { eq::Type::lowShelf, "Low Shelf" }, { eq::Type::highShelf, "High Shelf" }, { eq::Type::tilt, "Tilt" } };
    const double sg[6] = { 6, -6, 12, -12, 30, -30 };
    for (const auto& ty : types)
        for (int slope = 0; slope < 8; ++slope)
        {
            Result r;
            for (int fi = 0; fi < 40; ++fi)
                for (double g : sg)
                {
                    eq::BandSettings s { ty.t, 20.0 * std::pow (1000.0, fi / 39.0), g, 1.0, slope };
                    measure (des, s, fs, 0.05, 0.012, r);
                }
            const bool ok = r.worstClosed <= 1e-6 && r.capped == 0;
            if (! ok) ++fails;
            std::printf ("  [%s] %-10s %2d dB/oct Q 1: %d settings; rendered vs closed form %.2e dB; rendered vs analog worst %.3f dB (design vs analog %.3f)\n",
                         ok ? "PASS" : "FAIL", ty.name, 6 * eq::slopeOrder[slope], r.n, r.worstClosed, r.worstAnalog, r.worstDesignAnalog);
        }

    std::printf ("\n%s (%d failing rows)\n", fails == 0 ? "ALL PASS" : "FAILED", fails);
    return fails == 0 ? 0 : 1;
}
