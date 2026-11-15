// O-ParametricEQ Phase 2.6 research bench: display truth (G9 a), analyzer
// calibration, listen crossfades, packet precision, display-model cost.
//
// Scratch code. Reads the real Source/dsp headers and changes nothing.
//
//   clang++ -O3 -std=c++17 -I ../../../../Source tap_bench.cpp -o <scratch>/tap_bench
//   <scratch>/tap_bench decay | g9 | damped | json | analyzer | listen | cost
//                       | g9diag | listen2 | listen3 | listen3b | g9unit | all
//   (output of `all`: bench-output.txt)
//
// The engine here is the double engine: 24 eq::SlotBand started settled, run
// in index order with SlotBand::process, the float output simulated by
// rounding each output sample to float (the processor's input impulse 1.0 is
// exact in float, its lanes are double, its output is the rounded double).

#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/BandParams.h"
#include "dsp/SlotBand.h"
#include "dsp/ControlRamp.h"
#include "dsp/Fade.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace
{
constexpr double pi = 3.14159265358979323846;
using cd = std::complex<double>;
const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };

double db20 (double a) { return 20.0 * std::log10 (std::max (a, 1e-300)); }

//==============================================================================
// Random configurations (full parameter range, as G5 draws them)
//==============================================================================
struct Config
{
    int numBands;
    std::array<eq::RawBand, 24> raw;
};

Config randomConfig (std::mt19937_64& rng, int minBands, int maxBands)
{
    std::uniform_real_distribution<double> u (0.0, 1.0);
    Config c {};
    c.numBands = minBands + (int) (rng() % (uint64_t) (maxBands - minBands + 1));

    for (int b = 0; b < 24; ++b)
    {
        eq::RawBand r {};
        r.enabled = b < c.numBands ? 1.0f : 0.0f;
        r.bypass = 0.0f;
        r.type = (float) (rng() % 8);
        r.freq = (float) std::exp (std::log (10.0) + u (rng) * (std::log (30000.0) - std::log (10.0)));
        r.gain = (float) (-30.0 + 60.0 * u (rng));
        r.q = (float) std::exp (std::log (0.025) + u (rng) * (std::log (40.0) - std::log (0.025)));
        r.slope = (float) (rng() % 8);
        r.placement = (float) (rng() % 5);
        c.raw[(size_t) b] = r;
    }

    return c;
}

eq::BandInput inputOf (const eq::RawBand& r, bool stereo)
{
    eq::BandInput in = eq::makeBandInput (r, 100.0f, 0.0);
    in.placement = eq::effectivePlacement (in.placement, stereo);
    return in;
}

//==============================================================================
// Closed form: 2x2 composition in index order (ARCHITECTURE "True-response display")
//==============================================================================
struct M2 { cd a, b, c, d; };   // [[a b] [c d]]

M2 mul (const M2& x, const M2& y)   // x * y
{
    return { x.a * y.a + x.b * y.c, x.a * y.b + x.b * y.d,
             x.c * y.a + x.d * y.c, x.c * y.b + x.d * y.d };
}

M2 placementMatrix (eq::Placement p, cd F)
{
    const cd one (1.0, 0.0), zero (0.0, 0.0);
    const cd h = 0.5 * (F - one);

    switch (p)
    {
        case eq::Placement::stereo: return { F, zero, zero, F };
        case eq::Placement::left:   return { F, zero, zero, one };
        case eq::Placement::right:  return { one, zero, zero, F };
        case eq::Placement::mid:    return { one + h, h, h, one + h };
        case eq::Placement::side:   return { one + h, -h, -h, one + h };
    }

    return { one, zero, zero, one };
}

// The four drawn responses: LL, RR, MM, SS.
std::array<cd, 4> drawn (const M2& m)
{
    return { m.a, m.d, 0.5 * (m.a + m.b + m.c + m.d), 0.5 * (m.a - m.b - m.c + m.d) };
}

// Section response at a complex s (bilinear domain): the SVF and one-pole
// realisations are the bilinear images of these, so on the unit circle
// s = j tan (w/2) gives ResponseEval's closed forms.
cd sectionAtS (cd s, const eq::SectionParams& p)
{
    cd h (1.0, 0.0);

    if (p.hasSvf)
    {
        const cd x = s / p.g;
        h *= (p.mL + p.mB * x + p.mH * x * x) / (1.0 + p.k * x + x * x);
    }

    if (p.hasOnePole)
    {
        const cd x = s / p.G;
        h *= (p.cL + p.cH * x) / (1.0 + x);
    }

    return h;
}

struct Designed
{
    std::vector<eq::BandDesign> designs;
    std::vector<eq::Placement> placements;
};

Designed designAll (const Config& c, eq::Designer& designer, bool stereo)
{
    Designed d;

    for (int b = 0; b < 24; ++b)
    {
        const eq::BandInput in = inputOf (c.raw[(size_t) b], stereo);

        if (! in.active)
            continue;

        eq::BandDesign bd {};
        designer.design (in.settings, bd);
        d.designs.push_back (bd);
        d.placements.push_back (in.placement);
    }

    return d;
}

M2 composeAtS (const Designed& d, cd s)
{
    M2 m { cd (1, 0), cd (0, 0), cd (0, 0), cd (1, 0) };

    for (size_t i = 0; i < d.designs.size(); ++i)
    {
        cd F (1.0, 0.0);
        for (int k = 0; k < d.designs[i].numSections; ++k)
            F *= sectionAtS (s, d.designs[i].params[(size_t) k]);
        m = mul (placementMatrix (d.placements[i], F), m);
    }

    return m;
}

M2 composeOnCircle (const Designed& d, double w)
{
    return composeAtS (d, cd (0.0, std::tan (w / 2.0)));
}

//==============================================================================
// Double engine
//==============================================================================
struct Engine
{
    double fs;
    bool stereo = true;
    eq::Designer designer;
    eq::FadeTable table;
    std::vector<eq::SlotBand> bands = std::vector<eq::SlotBand> (24);

    explicit Engine (double rate) : fs (rate)
    {
        designer.prepare (fs);
        table.prepare (fs);
        for (auto& b : bands)
            b.prepare (eq::controlGridSize (fs), eq::rampLengthTicks (fs));
    }

    void start (const Config& c)
    {
        for (int b = 0; b < 24; ++b)
            bands[(size_t) b].startSettled (inputOf (c.raw[(size_t) b], stereo), designer);
    }

    void process (double* l, double* r, int n)
    {
        for (auto& b : bands)
            if (b.isRunning())
                b.process (l, r, n, stereo, table, designer);
    }
};

// Slowest pole of a design set: its decay in dB per second.
double slowestDecayDbPerSecond (const Designed& d, double fs)
{
    double slow = 1e300;

    for (const auto& bd : d.designs)
        for (int k = 0; k < bd.numSections; ++k)
        {
            const auto& p = bd.params[(size_t) k];

            if (p.hasSvf)
            {
                const cd disc = std::sqrt (cd (p.k * p.k - 4.0, 0.0));
                for (const cd sig : { (-p.k + disc) / 2.0, (-p.k - disc) / 2.0 })
                {
                    const cd s = p.g * sig;
                    const double r = std::abs ((1.0 + s) / (1.0 - s));
                    slow = std::min (slow, -20.0 * std::log10 (r) * fs);
                }
            }

            if (p.hasOnePole)
            {
                const double r = std::abs ((1.0 - p.G) / (1.0 + p.G));
                slow = std::min (slow, -20.0 * std::log10 (r) * fs);
            }
        }

    return slow;
}

//==============================================================================
// Impulse rendering (the render harness's renderImpulse rule: 1 s steps until
// the last 10 ms is 140 dB below the peak, cap 20 s), four stimuli.
//==============================================================================
struct Irs
{
    // [stimulus][channel]; stimuli: L, R, mid (1, 1), side (1, -1)
    std::array<std::array<std::vector<float>, 2>, 4> y;
    double seconds = 0.0;
    bool reached = false;
};

Irs renderIrs (Engine& e, const Config& c, double capSeconds, double tailDb = 140.0)
{
    Irs out;
    const double in[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
    const int step = (int) std::lround (e.fs);
    const int tailLength = (int) std::lround (e.fs * 0.010);
    const double tailRatio = std::pow (10.0, -tailDb / 20.0);
    constexpr int block = 4096;
    std::vector<double> l (block), r (block);

    for (int s = 0; s < 4; ++s)
    {
        e.start (c);
        auto& yl = out.y[(size_t) s][0];
        auto& yr = out.y[(size_t) s][1];
        yl.clear();
        yr.clear();
        int position = 0;
        bool reached = false;
        double seconds = 0.0;

        for (int second = 1; second <= (int) capSeconds && ! reached; ++second)
        {
            const int end = second * step;

            while (position < end)
            {
                const int n = std::min (block, end - position);
                std::fill (l.begin(), l.begin() + n, 0.0);
                std::fill (r.begin(), r.begin() + n, 0.0);

                if (position == 0)
                {
                    l[0] = in[s][0];
                    r[0] = in[s][1];
                }

                e.process (l.data(), r.data(), n);

                for (int i = 0; i < n; ++i)
                {
                    yl.push_back ((float) l[(size_t) i]);
                    yr.push_back ((float) r[(size_t) i]);
                }

                position += n;
            }

            double peak = 0.0, tail = 0.0;
            const int total = (int) yl.size();

            for (int i = 0; i < total; ++i)
            {
                const double v = std::max (std::abs ((double) yl[(size_t) i]), std::abs ((double) yr[(size_t) i]));
                peak = std::max (peak, v);
                if (i >= total - tailLength)
                    tail = std::max (tail, v);
            }

            seconds = (double) position / e.fs;
            reached = peak > 0.0 && tail <= peak * tailRatio;
        }

        out.seconds = std::max (out.seconds, seconds);
        out.reached = (s == 0) ? reached : (out.reached && reached);
    }

    return out;
}

// The four drawn responses from the IRs: LL from stimulus L / channel L, RR
// from R / R, MM = 0.5 (yL + yR) of stimulus mid, SS = 0.5 (yL - yR) of side.
// Weighted DFT: sum h[n] a^n e^{-jwn} (a = 1: the unit circle).
std::array<cd, 4> measure (const Irs& ir, double w, double a)
{
    std::array<cd, 4> out;

    for (int q = 0; q < 4; ++q)
    {
        const auto& yl = ir.y[(size_t) q][0];
        const auto& yr = ir.y[(size_t) q][1];
        const size_t N = yl.size();
        const double cr = a * std::cos (w), ci = -a * std::sin (w);
        double pr = 1.0, pim = 0.0, ar = 0.0, ai = 0.0;
        const double mag = a;

        for (size_t n = 0; n < N; ++n)
        {
            double v;
            switch (q)
            {
                case 0: v = yl[n]; break;
                case 1: v = yr[n]; break;
                case 2: v = 0.5 * ((double) yl[n] + (double) yr[n]); break;
                default: v = 0.5 * ((double) yl[n] - (double) yr[n]); break;
            }

            ar += v * pr;
            ai += v * pim;
            const double t = pr * cr - pim * ci;
            pim = pr * ci + pim * cr;
            pr = t;

            if ((n & 4095u) == 4095u)
            {
                // renormalise to the exact a^(n+1)
                const double want = std::pow (mag, (double) (n + 1));
                const double m = std::sqrt (pr * pr + pim * pim);
                if (m > 0.0) { pr *= want / m; pim *= want / m; }
            }
        }

        out[(size_t) q] = cd (ar, ai);
    }

    return out;
}

std::vector<double> logGrid (double lo, double hi, int n)
{
    std::vector<double> v;
    for (int i = 0; i < n; ++i)
        v.push_back (std::exp (std::log (lo) + (std::log (hi) - std::log (lo)) * i / (n - 1)));
    return v;
}

//==============================================================================
// decay: how long the measured IR must be (model, from the pole radii)
//==============================================================================
void runDecay()
{
    std::printf ("\n== decay [MODEL]: slowest pole of random full-range configurations, seconds to fall 140 dB (1000 draws per rate)\n");
    std::printf ("%-9s %-10s %8s %8s %8s %8s %8s   %s\n", "rate", "bands", "p50", "p90", "p99", "max", ">20 s", "share > 20 s");

    for (double fs : rates)
        for (int maxB : { 1, 8, 24 })
        {
            std::mt19937_64 rng (1234 + (uint64_t) fs + (uint64_t) maxB);
            eq::Designer designer;
            designer.prepare (fs);
            std::vector<double> t;
            int over = 0;

            for (int i = 0; i < 1000; ++i)
            {
                const Config c = randomConfig (rng, maxB == 1 ? 1 : 1, maxB);
                const Designed d = designAll (c, designer, true);
                const double rate = slowestDecayDbPerSecond (d, fs);
                const double s = rate > 0 ? 140.0 / rate : 1e9;
                t.push_back (s);
                if (s > 20.0) ++over;
            }

            std::sort (t.begin(), t.end());
            std::printf ("%-9.0f 1..%-7d %8.2f %8.2f %8.1f %8.1f %8d   %.1f %%\n", fs, maxB, t[500], t[900], t[990], t[999], over, over / 10.0);
        }
}

//==============================================================================
// g9: closed form vs the measured float IR on the unit circle
//==============================================================================
struct G9Result { double worst = 0; int scored = 0; double worstAt = 0; int which = -1; };

G9Result scoreUnit (const Designed& d, const Irs& ir, double fs, const std::vector<double>& hz)
{
    G9Result r;
    for (double f : hz)
    {
        const double w = 2.0 * pi * f / fs;
        const auto cf = drawn (composeOnCircle (d, w));
        const auto me = measure (ir, w, 1.0);

        for (int q = 0; q < 4; ++q)
        {
            const double ref = db20 (std::abs (cf[(size_t) q]));
            if (ref < -80.0) continue;
            const double e = std::abs (db20 (std::abs (me[(size_t) q])) - ref);
            ++r.scored;
            if (e > r.worst) { r.worst = e; r.worstAt = f; r.which = q; }
        }
    }
    return r;
}

void runG9 (int perRate, int maxBands)
{
    std::printf ("\n== g9 (a) [BENCH]: closed form (2x2 composition of ResponseEval forms) vs DFT of the float-rounded double-engine IR, unit circle\n");
    std::printf ("   %d random full-range configurations per rate, 1..%d bands, stereo; 96 log points 10 Hz .. min (30 kHz, 0.49 fs); LL / RR / MM / SS where the closed form is above -80 dB\n", perRate, maxBands);
    std::printf ("%-9s %8s %10s %12s %12s %10s\n", "rate", "reached", "capped", "worst (ok)", "worst (cap)", "max s");

    for (double fs : rates)
    {
        std::mt19937_64 rng (99 + (uint64_t) fs + (uint64_t) maxBands);
        Engine e (fs);
        const auto hz = logGrid (10.0, std::min (30000.0, 0.49 * fs), 96);
        int reached = 0, capped = 0;
        double worstOk = 0, worstCap = 0, maxS = 0;

        for (int i = 0; i < perRate; ++i)
        {
            const Config c = randomConfig (rng, 1, maxBands);
            const Designed d = designAll (c, e.designer, true);
            const Irs ir = renderIrs (e, c, 20.0);
            const G9Result r = scoreUnit (d, ir, fs, hz);
            maxS = std::max (maxS, ir.seconds);

            if (ir.reached) { ++reached; worstOk = std::max (worstOk, r.worst); }
            else            { ++capped;  worstCap = std::max (worstCap, r.worst); }
        }

        std::printf ("%-9.0f %8d %10d %12.2e %12.2e %10.1f\n", fs, reached, capped, worstOk, worstCap, maxS);
    }
}

//==============================================================================
// damped: weighted DFT (a < 1) of a fixed-length IR vs the closed form at
// z = e^{jw} / a, i.e. s = (z - 1) / (z + 1)
//==============================================================================
void runDamped (int perRate, int maxBands)
{
    std::printf ("\n== damped (a) [BENCH]: fixed-length IR (2^17 samples) weighted by a^n, a^N = 1e-9; closed form at z = e^{jw}/a; every draw scored\n");
    std::printf ("   same draws as g9; also the planted-defect control: one band's g perturbed by 1e-4 relative in the closed form\n");
    std::printf ("%-9s %8s %14s %16s\n", "rate", "draws", "worst dB", "control min dB");

    for (double fs : rates)
    {
        std::mt19937_64 rng (99 + (uint64_t) fs + (uint64_t) maxBands);
        Engine e (fs);
        const auto hz = logGrid (10.0, std::min (30000.0, 0.49 * fs), 96);
        const int N = 1 << 17;
        const double a = std::pow (1e-9, 1.0 / N);
        double worst = 0.0, controlMin = 1e9;

        for (int i = 0; i < perRate; ++i)
        {
            const Config c = randomConfig (rng, 1, maxBands);
            Designed d = designAll (c, e.designer, true);

            // fixed length render: 2^17 samples (cap 1 "second" replaced by N)
            Irs ir;
            {
                const double in[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
                std::vector<double> l (N, 0.0), r (N, 0.0);
                for (int s = 0; s < 4; ++s)
                {
                    e.start (c);
                    std::fill (l.begin(), l.end(), 0.0);
                    std::fill (r.begin(), r.end(), 0.0);
                    l[0] = in[s][0]; r[0] = in[s][1];
                    for (int p = 0; p < N; p += 4096)
                        e.process (l.data() + p, r.data() + p, 4096);
                    ir.y[(size_t) s][0].assign (l.begin(), l.end());
                    ir.y[(size_t) s][1].assign (r.begin(), r.end());
                    for (auto& v : ir.y[(size_t) s][0]) v = (float) v;
                    for (auto& v : ir.y[(size_t) s][1]) v = (float) v;
                }
            }

            Designed bad = d;
            bad.designs[0].params[0].g *= (1.0 + 1e-4);
            double controlWorst = 0.0;

            for (double f : hz)
            {
                const double w = 2.0 * pi * f / fs;
                const cd z = std::polar (1.0 / a, w);
                const cd s = (z - 1.0) / (z + 1.0);
                const auto cf = drawn (composeAtS (d, s));
                const auto cb = drawn (composeAtS (bad, s));
                const auto me = measure (ir, w, a);

                for (int q = 0; q < 4; ++q)
                {
                    const double ref = db20 (std::abs (cf[(size_t) q]));
                    if (ref < -80.0) continue;
                    worst = std::max (worst, std::abs (db20 (std::abs (me[(size_t) q])) - ref));
                    controlWorst = std::max (controlWorst, std::abs (db20 (std::abs (me[(size_t) q])) - db20 (std::abs (cb[(size_t) q]))));
                }
            }

            controlMin = std::min (controlMin, controlWorst);
        }

        std::printf ("%-9.0f %8d %14.2e %16.2e\n", fs, perRate, worst, controlMin);
    }
}

//==============================================================================
// json: packet precision. JUCE serialiseDouble keeps about 16 significant
// digits (13 - 20 decimals by magnitude; 15 in scientific form outside
// [1e-5, 1e6]). Round every section parameter to 16 significant digits and
// compare the drawn responses.
//==============================================================================
double round16 (double v)
{
    char buf[64];
    std::snprintf (buf, sizeof buf, "%.15e", v);
    return std::strtod (buf, nullptr);
}

void runJson()
{
    std::printf ("\n== json [BENCH]: every section parameter rounded to 16 significant digits (JUCE JSON) vs exact, drawn responses above -80 dB, 400 draws per rate, 1..24 bands, 512 log points\n");
    std::printf ("%-9s %14s\n", "rate", "worst dB");

    for (double fs : rates)
    {
        std::mt19937_64 rng (7 + (uint64_t) fs);
        eq::Designer designer;
        designer.prepare (fs);
        const auto hz = logGrid (10.0, std::min (30000.0, 0.499 * fs), 512);
        double worst = 0.0;

        for (int i = 0; i < 400; ++i)
        {
            const Config c = randomConfig (rng, 1, 24);
            const Designed d = designAll (c, designer, true);
            Designed r = d;
            for (auto& bd : r.designs)
                for (auto& p : bd.params)
                {
                    p.g = round16 (p.g); p.k = round16 (p.k); p.mL = round16 (p.mL); p.mB = round16 (p.mB); p.mH = round16 (p.mH);
                    p.G = round16 (p.G); p.cL = round16 (p.cL); p.cH = round16 (p.cH);
                }

            for (double f : hz)
            {
                const double w = 2.0 * pi * f / fs;
                const auto a = drawn (composeOnCircle (d, w));
                const auto b = drawn (composeOnCircle (r, w));
                for (int q = 0; q < 4; ++q)
                {
                    const double ref = db20 (std::abs (a[(size_t) q]));
                    if (ref < -80.0) continue;
                    worst = std::max (worst, std::abs (db20 (std::abs (b[(size_t) q])) - ref));
                }
            }
        }

        std::printf ("%-9.0f %14.2e\n", fs, worst);
    }
}

//==============================================================================
// analyzer: calibration of the ARCHITECTURE pipeline
//==============================================================================
void fft (std::vector<cd>& a)
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
        const double ang = -2.0 * pi / (double) len;
        const cd wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            cd w (1.0, 0.0);
            for (size_t j = 0; j < len / 2; ++j)
            {
                const cd u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

enum Win { hann, flattop, bh4 };
const char* winName[] = { "Hann", "flat-top", "Blackman-Harris 4" };

std::vector<double> window (Win type, int n)
{
    std::vector<double> w ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const double x = 2.0 * pi * i / n;     // periodic
        switch (type)
        {
            case hann:    w[(size_t) i] = 0.5 - 0.5 * std::cos (x); break;
            case flattop: w[(size_t) i] = 0.21557895 - 0.41663158 * std::cos (x) + 0.277263158 * std::cos (2 * x)
                                          - 0.083578947 * std::cos (3 * x) + 0.006947368 * std::cos (4 * x); break;
            case bh4:     w[(size_t) i] = 0.35875 - 0.48829 * std::cos (x) + 0.14128 * std::cos (2 * x) - 0.01168 * std::cos (3 * x); break;
        }
    }
    return w;
}

int windowSize (double fs) { return fs <= 48000.0 ? 4096 : fs <= 96000.0 ? 8192 : 16384; }

// Display points: 256 log-spaced over 10 Hz .. 30 kHz (the mockup's axis),
// spans between geometric midpoints. Value: the maximum bin inside the span;
// a span with no bin reads the dB interpolation (linear in frequency) of the
// two bins around its centre. Points above 0.5 fs are empty.
std::vector<double> displayPoints (const std::vector<double>& binDb, double binHz, double fs)
{
    const int P = 256;
    std::vector<double> out ((size_t) P, -300.0);
    const double lo = std::log (10.0), hi = std::log (30000.0);

    for (int i = 0; i < P; ++i)
    {
        const double fc = std::exp (lo + (hi - lo) * i / (P - 1));
        if (fc >= 0.5 * fs) break;
        const double f0 = std::exp (lo + (hi - lo) * (i - 0.5) / (P - 1));
        const double f1 = std::exp (lo + (hi - lo) * (i + 0.5) / (P - 1));
        const int b0 = (int) std::ceil (f0 / binHz), b1 = (int) std::floor (f1 / binHz);
        double v = -300.0;
        if (b1 >= b0)
            for (int b = std::max (b0, 0); b <= std::min (b1, (int) binDb.size() - 1); ++b)
                v = std::max (v, binDb[(size_t) b]);
        else
        {
            const double x = fc / binHz;
            const int k = (int) std::floor (x);
            const double t = x - k;
            if (k + 1 < (int) binDb.size())
                v = (1 - t) * binDb[(size_t) k] + t * binDb[(size_t) k + 1];
        }
        out[(size_t) i] = v;
    }
    return out;
}

// Reading of a sine (peak amplitude A) at frequency f: the largest display
// value, in dB re full-scale sine (scale 2 / sum w).
double readSine (double fs, double f, double A, Win type, int pad, double phase)
{
    const int n = windowSize (fs);
    const auto w = window (type, n);
    double sw = 0.0;
    for (double v : w) sw += v;
    std::vector<cd> buf ((size_t) (n * pad), cd (0, 0));
    for (int i = 0; i < n; ++i)
        buf[(size_t) i] = cd ((float) (A * std::sin (2.0 * pi * f * i / fs + phase)) * w[(size_t) i], 0.0);
    fft (buf);
    const size_t half = buf.size() / 2;
    std::vector<double> binDb (half + 1);
    for (size_t k = 0; k <= half; ++k)
        binDb[k] = db20 (std::abs (buf[k]) * 2.0 / sw);
    const auto pts = displayPoints (binDb, fs / (double) buf.size(), fs);
    return *std::max_element (pts.begin(), pts.end());
}

void runAnalyzer()
{
    const double A = std::pow (10.0, -12.0 / 20.0);
    std::printf ("\n== analyzer [BENCH]: -12 dBFS sine (both channels, mono sum 0.5 (L + R) = the sine), window per ARCHITECTURE (4096 / 8192 / 16384), FFT of the window zero-padded x pad,\n");
    std::printf ("   scale 2 / sum (w), 256 display points 10 Hz - 30 kHz (max in span; dB interpolation where a span holds no bin). Reading = largest display value. 8 phases each.\n");
    std::printf ("%-18s %4s | %-31s | %-40s\n", "window", "pad", "1 kHz reading at 44.1 / 96 / 192", "sweep 20 Hz - 20 kHz (300 log tones): min .. max, 5 rates");

    for (Win type : { hann, bh4, flattop })
        for (int pad : { 1, 2, 4 })
        {
            double r1[3];
            const double r1rates[3] = { 44100.0, 96000.0, 192000.0 };
            for (int k = 0; k < 3; ++k)
            {
                double lo = 1e9;
                for (int ph = 0; ph < 8; ++ph)
                    lo = std::min (lo, readSine (r1rates[k], 1000.0, A, type, pad, ph * pi / 4));
                r1[k] = lo;
            }

            double mn = 1e9, mx = -1e9;
            for (double fs : rates)
                for (int t = 0; t < 300; ++t)
                {
                    const double f = std::exp (std::log (20.0) + (std::log (20000.0) - std::log (20.0)) * t / 299.0);
                    for (int ph = 0; ph < 2; ++ph)
                    {
                        const double v = readSine (fs, f, A, type, pad, ph * 1.3);
                        mn = std::min (mn, v);
                        mx = std::max (mx, v);
                    }
                }

            std::printf ("%-18s %4d | %8.3f %8.3f %8.3f       | %8.3f .. %8.3f\n", winName[type], pad, r1[0], r1[1], r1[2], mn, mx);
        }

    // Resolution cost: -6 dB main-lobe width of each window at 44.1 kHz, Hz
    std::printf ("   -6 dB main-lobe full width (Hz) at 11.7 Hz bin spacing: Hann 2.00 bins = 23 Hz, Blackman-Harris 4 2.72 bins = 32 Hz, flat-top 3.7 bins = 43 Hz (window tables)\n");
}

//==============================================================================
// listen: the EQ <-> listen crossfade and a listen band change, G3 step measures
//==============================================================================
eq::RawBand raw (int type, double f, double g, double q, int slope, int place)
{
    return eq::RawBand { 1.0f, 0.0f, (float) type, (float) f, (float) g, (float) q, (float) slope, (float) place };
}

// D11 mapping: Bell / Notch / Band Pass -> Band Pass (f, Q); Low Shelf / Low Cut
// -> 12 dB/oct low-pass (High Cut, Q 1) at f; High Shelf / High Cut -> 12 dB/oct
// high-pass (Low Cut, Q 1); Tilt -> Band Pass, Q 0.7. Gain irrelevant.
eq::RawBand listenOf (const eq::RawBand& r)
{
    const int t = (int) r.type;
    using T = eq::Type;
    switch ((T) t)
    {
        case T::bell: case T::notch: case T::bandPass: return raw ((int) T::bandPass, r.freq, 0.0, r.q, 0, 0);
        case T::lowShelf: case T::lowCut:              return raw ((int) T::highCut, r.freq, 0.0, 1.0, 1, 0);
        case T::highShelf: case T::highCut:            return raw ((int) T::lowCut, r.freq, 0.0, 1.0, 1, 0);
        case T::tilt:                                  return raw ((int) T::bandPass, r.freq, 0.0, 0.7, 0, 0);
    }
    return r;
}

struct Mono
{
    double fs;
    eq::Designer designer;
    eq::FadeTable table;
    eq::SlotBand band;

    explicit Mono (double rate) : fs (rate)
    {
        designer.prepare (fs);
        table.prepare (fs);
        band.prepare (eq::controlGridSize (fs), eq::rampLengthTicks (fs));
    }

    void start (const eq::RawBand& r) { band.startSettled (inputOf (r, false), designer); }
    void request (const eq::RawBand& r) { band.request (inputOf (r, false), designer); }

    // processes with control ticks on the grid (for fades inside the band)
    void run (std::vector<double>& x, int from, int to, uint64_t& clock)
    {
        const int K = eq::controlGridSize (fs);
        const int sleep = eq::sleepLengthTicks (fs, K);
        double dummy[1];
        (void) dummy;
        int p = from;
        while (p < to)
        {
            if (clock % (uint64_t) K == 0)
                band.controlTick (designer, sleep);
            const int m = std::min (to - p, K - (int) (clock % (uint64_t) K));
            band.process (x.data() + p, nullptr, m, false, table, designer);
            p += m;
            clock += (uint64_t) m;
        }
    }
};

double maxD2 (const std::vector<double>& y, int a, int b)
{
    double m = 0.0;
    for (int i = std::max (a, 2); i < std::min (b, (int) y.size()); ++i)
        m = std::max (m, std::abs (y[(size_t) i] - 2 * y[(size_t) i - 1] + y[(size_t) i - 2]));
    return m;
}

double localKappa (const std::vector<double>& y, int a, int b, double fs)
{
    const int frame = (int) std::lround (0.005 * fs);
    const double floorRms = std::pow (10.0, -80.0 / 20.0);
    double k = 0.0;
    for (int s = std::max (a, 2); s + frame <= std::min (b, (int) y.size()); s += frame)
    {
        double e = 0.0, d = 0.0;
        for (int i = s; i < s + frame; ++i)
        {
            e += y[(size_t) i] * y[(size_t) i];
            d = std::max (d, std::abs (y[(size_t) i] - 2 * y[(size_t) i - 1] + y[(size_t) i - 2]));
        }
        const double rms = std::sqrt (e / frame);
        if (rms < floorRms) continue;
        k = std::max (k, d / rms);
    }
    return k;
}

struct ListenCase { const char* name; eq::RawBand band; };

void runListen()
{
    const double fs = 48000.0;
    const int total = (int) (1.5 * fs);
    const int tOn = (int) (0.5 * fs) + 7, tOff = (int) (1.0 * fs) + 11;   // off the grid on purpose

    const ListenCase cases[] = {
        { "Bell 1 kHz +12 Q 2",          raw (0, 1000, 12, 2, 0, 0) },
        { "Bell 50 Hz -18 Q 4",          raw (0, 50, -18, 4, 0, 0) },
        { "Bell 5 kHz +6 Q 40",          raw (0, 5000, 6, 40, 0, 0) },
        { "Bell 10 Hz +12 Q 40",         raw (0, 10, 12, 40, 0, 0) },
        { "Low Shelf 200 Hz +12 24 dB",  raw (1, 200, 12, 1, 2, 0) },
        { "High Shelf 8 kHz -12 12 dB",  raw (2, 8000, -12, 1, 1, 0) },
        { "Low Cut 100 Hz 48 dB",        raw (3, 100, 0, 1, 4, 0) },
        { "High Cut 2 kHz 96 dB",        raw (4, 2000, 0, 1, 7, 0) },
        { "Notch 50 Hz Q 10",            raw (5, 50, 0, 10, 0, 0) },
        { "Band Pass 1 kHz Q 10",        raw (6, 1000, 0, 10, 0, 0) },
        { "Tilt 1 kHz +12",              raw (7, 1000, 12, 1, 1, 0) },
    };

    std::printf ("\n== listen [BENCH]: 48 kHz, tone -6 dBFS. Listen on at 0.5 s, off at 1.0 s (off the grid): output = (1 - w) EQ + w listen, w the Fade.h 20 ms raised cosine;\n");
    std::printf ("   the listen filter (one SlotBand, D11 mapping) starts from rest at the start of the fade to listen. (a) = largest |d2y| over the event -50 .. +70 ms / larger of the two held renders' over the same window;\n");
    std::printf ("   (b) 1 kHz local kappa / larger held kappa. HARD = the switch with no fade (control). Limits: (a) 2.0x, (b) 2.5x, HARD >= 10x at (a).\n");
    std::printf ("%-28s | %-7s %-7s %-7s %-9s | %-7s %-7s %-7s %-9s\n", "listened band", "50 on", "50 off", "1k on", "1k off", "1k k on", "1k k off", "HARD 50", "HARD 1k");

    double worstA = 0, worstK = 0, minHard = 1e9;

    for (const auto& lc : cases)
    {
        double res[2][2], kap[2][2], hard[2];

        for (int tone = 0; tone < 2; ++tone)
        {
            const double f = tone == 0 ? 50.0 : 1000.0;
            double worstOverPhase[2] = { 0, 0 }, worstK2[2] = { 0, 0 }, hardMin = 1e9;

            for (int ph = 0; ph < 4; ++ph)
            {
                std::vector<double> x ((size_t) total);
                for (int i = 0; i < total; ++i) x[(size_t) i] = 0.5 * std::sin (2 * pi * f * i / fs + ph * pi / 4);

                // held EQ render
                std::vector<double> eqY = x;
                { Mono m (fs); m.start (lc.band); uint64_t c = 0; m.run (eqY, 0, total, c); }
                // held listen render (warm: started at 0)
                std::vector<double> liY = x;
                { Mono m (fs); m.start (listenOf (lc.band)); uint64_t c = 0; m.run (liY, 0, total, c); }
                // listen path for the event: started from rest at tOn
                std::vector<double> liEv = x;
                { Mono m (fs); m.start (listenOf (lc.band)); std::vector<double> pre (x.begin(), x.begin() + tOn);
                  // rest until tOn: zero the listen output before it
                  std::vector<double> part (x.begin() + tOn, x.end());
                  uint64_t c = (uint64_t) tOn; m.run (part, 0, (int) part.size(), c);
                  for (int i = 0; i < total; ++i) liEv[(size_t) i] = i < tOn ? 0.0 : part[(size_t) (i - tOn)]; }

                const int L = (int) std::lround (0.020 * fs);
                auto wOf = [&] (int i, int t0, bool up) {
                    const int n = i - t0;
                    double w = n < 0 ? 0.0 : n >= L ? 1.0 : eq::FadeTable::weightOf (n, L);
                    return up ? w : 1.0 - w;
                };

                std::vector<double> y ((size_t) total), yh ((size_t) total);
                for (int i = 0; i < total; ++i)
                {
                    double w = i < tOff ? wOf (i, tOn, true) : wOf (i, tOff, false);
                    y[(size_t) i] = (1 - w) * eqY[(size_t) i] + w * liEv[(size_t) i];
                    const double wh = (i >= tOn && i < tOff) ? 1.0 : 0.0;
                    yh[(size_t) i] = (1 - wh) * eqY[(size_t) i] + wh * liEv[(size_t) i];
                }

                const int pre = (int) (0.050 * fs), post = (int) (0.070 * fs);
                for (int ev = 0; ev < 2; ++ev)
                {
                    const int t = ev == 0 ? tOn : tOff;
                    const double ref = std::max (maxD2 (eqY, t - pre, t + post), maxD2 (liY, t - pre, t + post));
                    worstOverPhase[ev] = std::max (worstOverPhase[ev], maxD2 (y, t - pre, t + post) / ref);
                    hardMin = std::min (hardMin, maxD2 (yh, t - pre, t + post) / ref);
                    if (tone == 1)
                    {
                        const double kr = std::max (localKappa (eqY, t - pre, t + post, fs), localKappa (liY, t - pre, t + post, fs));
                        worstK2[ev] = std::max (worstK2[ev], localKappa (y, t - pre, t + post, fs) / kr);
                    }
                }
            }

            res[tone][0] = worstOverPhase[0]; res[tone][1] = worstOverPhase[1];
            kap[tone][0] = worstK2[0]; kap[tone][1] = worstK2[1];
            hard[tone] = hardMin;
        }

        std::printf ("%-28s | %-7.2f %-7.2f %-7.2f %-9.2f | %-7.2f %-8.2f %-7.0f %-9.0f\n", lc.name,
                     res[0][0], res[0][1], res[1][0], res[1][1], kap[1][0], kap[1][1], hard[0], hard[1]);
        worstA = std::max ({ worstA, res[0][0], res[0][1], res[1][0], res[1][1] });
        worstK = std::max ({ worstK, kap[1][0], kap[1][1] });
        minHard = std::min ({ minHard, hard[0], hard[1] });
    }

    std::printf ("   worst (a) %.2fx, worst (b) %.2fx, smallest HARD %.0fx\n", worstA, worstK, minHard);

    // Listen band change inside the listen SlotBand: listen A held, request B at 0.5 s (structural fade, 20 ms)
    std::printf ("\n   listen band change (the listen SlotBand's own structural crossfade, B from rest), 50 Hz / 1 kHz (a), 1 kHz kappa:\n");
    const std::pair<ListenCase, ListenCase> changes[] = {
        { cases[0], cases[4] }, { cases[4], cases[5] }, { cases[1], cases[9] }, { cases[6], cases[7] }, { cases[2], cases[10] }, { cases[8], cases[0] },
    };
    double worstCh = 0, worstChK = 0;
    for (const auto& ch : changes)
    {
        double r[2] = { 0, 0 }, kk = 0;
        for (int tone = 0; tone < 2; ++tone)
        {
            const double f = tone == 0 ? 50.0 : 1000.0;
            for (int ph = 0; ph < 4; ++ph)
            {
                std::vector<double> x ((size_t) total);
                for (int i = 0; i < total; ++i) x[(size_t) i] = 0.5 * std::sin (2 * pi * f * i / fs + ph * pi / 4);
                std::vector<double> a = x, b = x, y = x;
                { Mono m (fs); m.start (listenOf (ch.first.band)); uint64_t c = 0; m.run (a, 0, total, c); }
                { Mono m (fs); m.start (listenOf (ch.second.band)); uint64_t c = 0; m.run (b, 0, total, c); }
                { Mono m (fs); m.start (listenOf (ch.first.band)); uint64_t c = 0; m.run (y, 0, tOn, c);
                  m.request (listenOf (ch.second.band)); m.run (y, tOn, total, c); }
                const int pre = (int) (0.050 * fs), post = (int) (0.070 * fs);
                const double ref = std::max (maxD2 (a, tOn - pre, tOn + post), maxD2 (b, tOn - pre, tOn + post));
                r[tone] = std::max (r[tone], maxD2 (y, tOn - pre, tOn + post) / ref);
                if (tone == 1)
                    kk = std::max (kk, localKappa (y, tOn - pre, tOn + post, fs)
                                       / std::max (localKappa (a, tOn - pre, tOn + post, fs), localKappa (b, tOn - pre, tOn + post, fs)));
            }
        }
        std::printf ("   %-28s -> %-28s  %5.2f %5.2f   k %5.2f\n", ch.first.name, ch.second.name, r[0], r[1], kk);
        worstCh = std::max ({ worstCh, r[0], r[1] });
        worstChK = std::max (worstChK, kk);
    }
    std::printf ("   worst (a) %.2fx, worst (b) %.2fx\n", worstCh, worstChK);
}

//==============================================================================
// cost: a full display push (24 bands designed on the message thread)
//==============================================================================
void runCost()
{
    std::printf ("\n== cost [BENCH] (this machine, -O3, scalar): a full DisplayModel push = 24 designs\n");
    for (double fs : { 44100.0, 96000.0 })
    {
        eq::Designer designer;
        designer.prepare (fs);
        struct C { const char* name; eq::RawBand r; } cs[] = {
            { "24 bells", raw (0, 1000, 12, 2, 0, 0) },
            { "24 shelves 96 dB/oct", raw (1, 300, 12, 1, 7, 0) },
            { "24 cuts 96 dB/oct", raw (3, 300, 0, 1, 7, 0) },
        };
        for (auto& c : cs)
        {
            eq::BandDesign bd {};
            const int reps = 2000;
            auto t0 = std::chrono::steady_clock::now();
            double sink = 0;
            for (int i = 0; i < reps; ++i)
                for (int b = 0; b < 24; ++b)
                {
                    eq::RawBand r = c.r;
                    r.freq = (float) (r.freq * (1.0 + 1e-4 * ((i * 24 + b) % 97)));
                    designer.design (inputOf (r, true).settings, bd);
                    sink += bd.params[0].g;
                }
            auto t1 = std::chrono::steady_clock::now();
            const double us = std::chrono::duration<double, std::micro> (t1 - t0).count() / reps;
            std::printf ("   %-7.0f %-24s %8.1f us per push%s\n", fs, c.name, us, sink == 12345 ? " " : "");
        }
    }
}

//==============================================================================
// g9diag: where the error comes from. Same draws as g9 (1..8 bands). For each
// scored point: |err| bucketed by the closed-form level re 1 and re the
// drawn response's own maximum; float-rounded vs double IR; unit circle with
// the 20 s rule vs damped 2^17.
//==============================================================================
void runG9Diag()
{
    std::printf ("\n== g9diag [BENCH]: worst |err| dB by closed-form level, 12 draws x 5 rates, 40 log points, 1..8 bands. U = unit circle (fixed 8 s), D = damped 2^17\n");
    std::printf ("   'rel' buckets: level re the largest value of that drawn response over the grid\n");
    const char* bn[] = { "> -40", "-40..-60", "-60..-80" };
    double w[2][2][3] = {};   // [float?][U/D][bucket]
    double wr[2][2][3] = {};  // relative buckets
    for (double fs : rates)
    {
        std::mt19937_64 rng (99 + (uint64_t) fs + 8);
        Engine e (fs);
        const auto hz = logGrid (10.0, std::min (30000.0, 0.49 * fs), 40);
        const int N = 1 << 17;
        const double a = std::pow (1e-9, 1.0 / N);
        for (int i = 0; i < 12; ++i)
        {
            const Config c = randomConfig (rng, 1, 8);
            const Designed d = designAll (c, e.designer, true);
            // double path: measure directly from double renders
            for (int mode = 0; mode < 2; ++mode)
                for (int fl = 0; fl < 2; ++fl)
                {
                    const double in[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
                    const int len = mode == 0 ? (int) std::lround (8.0 * fs) : N;
                    const double aa = mode == 0 ? 1.0 : a;
                    std::vector<double> l (len), r (len);
                    std::array<std::array<std::vector<double>, 2>, 4> y;
                    for (int s = 0; s < 4; ++s)
                    {
                        e.start (c);
                        std::fill (l.begin(), l.end(), 0.0); std::fill (r.begin(), r.end(), 0.0);
                        l[0] = in[s][0]; r[0] = in[s][1];
                        for (int p = 0; p < len; p += 4096) e.process (l.data() + p, r.data() + p, std::min (4096, len - p));
                        if (fl) for (int k = 0; k < len; ++k) { l[(size_t) k] = (float) l[(size_t) k]; r[(size_t) k] = (float) r[(size_t) k]; }
                        y[(size_t) s][0] = l; y[(size_t) s][1] = r;
                    }
                    std::vector<std::array<cd, 4>> cfs, mes;
                    std::array<double, 4> mx { -1e9, -1e9, -1e9, -1e9 };
                    for (double f : hz)
                    {
                        const double wv = 2.0 * pi * f / fs;
                        const cd z = std::polar (1.0 / aa, wv);
                        const cd s = mode == 0 ? cd (0.0, std::tan (wv / 2.0)) : (z - 1.0) / (z + 1.0);
                        const auto cf = drawn (composeAtS (d, s));
                        std::array<cd, 4> me;
                        for (int q = 0; q < 4; ++q)
                        {
                            const cd step = std::polar (aa, -wv);
                            cd ph (1, 0), acc (0, 0);
                            for (int n = 0; n < len; ++n)
                            {
                                double v = q == 0 ? y[0][0][(size_t) n] : q == 1 ? y[1][1][(size_t) n]
                                         : q == 2 ? 0.5 * (y[2][0][(size_t) n] + y[2][1][(size_t) n]) : 0.5 * (y[3][0][(size_t) n] - y[3][1][(size_t) n]);
                                acc += v * ph;
                                ph *= step;
                                if ((n & 4095) == 4095) ph = std::polar (std::pow (aa, n + 1.0), -wv * (n + 1.0));
                            }
                            me[(size_t) q] = acc;
                            mx[(size_t) q] = std::max (mx[(size_t) q], db20 (std::abs (cf[(size_t) q])));
                        }
                        cfs.push_back (cf); mes.push_back (me);
                    }
                    for (size_t k = 0; k < hz.size(); ++k)
                        for (int q = 0; q < 4; ++q)
                        {
                            const double ref = db20 (std::abs (cfs[k][(size_t) q]));
                            const double err = std::abs (db20 (std::abs (mes[k][(size_t) q])) - ref);
                            if (ref >= -80.0) { const int b = ref > -40 ? 0 : ref > -60 ? 1 : 2; w[fl][mode][b] = std::max (w[fl][mode][b], err); }
                            const double rel = ref - mx[(size_t) q];
                            if (rel >= -80.0) { const int b = rel > -40 ? 0 : rel > -60 ? 1 : 2; wr[fl][mode][b] = std::max (wr[fl][mode][b], err); }
                        }
                }
        }
    }
    for (int fl = 0; fl < 2; ++fl)
        for (int mode = 0; mode < 2; ++mode)
            std::printf ("   %-6s %s  abs: %s %.2e  %s %.2e  %s %.2e   rel: %s %.2e  %s %.2e  %s %.2e\n", fl ? "float" : "double", mode ? "D" : "U",
                         bn[0], w[fl][mode][0], bn[1], w[fl][mode][1], bn[2], w[fl][mode][2], bn[0], wr[fl][mode][0], bn[1], wr[fl][mode][1], bn[2], wr[fl][mode][2]);
}

//==============================================================================
// listen2: a pre-roll before the listen fade (the filter runs from rest, the
// weight stays 0 for P samples), and a listen band change as a crossfade
// (B from rest, pre-roll P) instead of the SlotBand's ramp / crossfade.
//==============================================================================
void runListen2()
{
    const double fs = 48000.0;
    const int total = (int) (1.5 * fs);
    const int tOn = (int) (0.5 * fs) + 7;
    const int L = (int) std::lround (0.020 * fs);
    const ListenCase* cases = nullptr; (void) cases;
    const std::pair<const char*, eq::RawBand> cs[] = {
        { "Bell 1 kHz +12 Q 2", raw (0, 1000, 12, 2, 0, 0) }, { "Bell 50 Hz -18 Q 4", raw (0, 50, -18, 4, 0, 0) },
        { "Bell 5 kHz +6 Q 40", raw (0, 5000, 6, 40, 0, 0) }, { "Bell 10 Hz +12 Q 40", raw (0, 10, 12, 40, 0, 0) },
        { "Low Shelf 200 Hz +12", raw (1, 200, 12, 1, 2, 0) }, { "High Shelf 8 kHz -12", raw (2, 8000, -12, 1, 1, 0) },
        { "Low Cut 100 Hz 48 dB", raw (3, 100, 0, 1, 4, 0) }, { "High Cut 2 kHz 96 dB", raw (4, 2000, 0, 1, 7, 0) },
        { "Notch 50 Hz Q 10", raw (5, 50, 0, 10, 0, 0) }, { "Band Pass 1 kHz Q 10", raw (6, 1000, 0, 10, 0, 0) },
        { "Band Pass 8 kHz Q 40", raw (6, 8000, 0, 40, 0, 0) }, { "Band Pass 200 Hz Q 0.3", raw (6, 200, 0, 0.3, 0, 0) },
        { "Tilt 1 kHz +12", raw (7, 1000, 12, 1, 1, 0) }, { "Bell 2 kHz +12 Q 10", raw (0, 2000, 12, 10, 0, 0) },
        { "Band Pass 300 Hz Q 40", raw (6, 300, 0, 40, 0, 0) }, { "Band Pass 150 Hz Q 20", raw (6, 150, 0, 20, 0, 0) },
        { "Bell 400 Hz +12 Q 40", raw (0, 400, 12, 40, 0, 0) }, { "Band Pass 3 kHz Q 40", raw (6, 3000, 0, 40, 0, 0) },
        { "Low Shelf 20 Hz +12", raw (1, 20, 12, 1, 1, 0) }, { "High Cut 120 Hz 12 dB", raw (4, 120, 0, 1, 1, 0) },
    };
    const int nc = (int) (sizeof cs / sizeof cs[0]);

    auto tone = [&] (double f, int ph) { std::vector<double> x ((size_t) total); for (int i = 0; i < total; ++i) x[(size_t) i] = 0.5 * std::sin (2 * pi * f * i / fs + ph * pi / 4); return x; };
    auto render = [&] (const eq::RawBand& b, const std::vector<double>& x, int from) {
        std::vector<double> y ((size_t) total, 0.0);
        std::vector<double> part (x.begin() + from, x.end());
        Mono m (fs); m.start (b); uint64_t c = (uint64_t) from; m.run (part, 0, (int) part.size(), c);
        for (int i = from; i < total; ++i) y[(size_t) i] = part[(size_t) (i - from)];
        return y; };
    auto wAt = [&] (int i, int t0) { const int n = i - t0; return n < 0 ? 0.0 : n >= L ? 1.0 : eq::FadeTable::weightOf (n, L); };

    std::printf ("\n== listen2 [BENCH]: pre-roll P before the fade to the listen path (filter from rest at the request, weight 0 for P), 48 kHz, 4 phases, (a) both tones / (b) 1 kHz kappa\n");
    std::printf ("   listen on: %d listened bands; band change: all %d x %d ordered pairs as a crossfade between two listen filters, B from rest\n", nc, nc, nc - 1);
    std::printf ("%-8s | %-26s | %-26s\n", "P ms", "listen on: worst (a), (b)", "band change: worst (a), (b), cases > 2.0x");

    for (double Pms : { 0.0, 20.0, 30.0, 50.0, 100.0, 200.0 })
    {
        const int P = (int) std::lround (Pms * 1e-3 * fs);
        double onA = 0, onK = 0, chA = 0, chK = 0; int chOver = 0;
        const int pre = (int) (0.050 * fs), post = (int) (0.070 * fs) + P;
        for (int t = 0; t < 2; ++t)
        {
            const double f = t == 0 ? 50.0 : 1000.0;
            for (int ph = 0; ph < 4; ++ph)
            {
                const auto x = tone (f, ph);
                std::vector<std::vector<double>> held, ev;
                for (int k = 0; k < nc; ++k) { held.push_back (render (listenOf (cs[k].second), x, 0)); ev.push_back (render (listenOf (cs[k].second), x, tOn)); }
                for (int k = 0; k < nc; ++k)
                {
                    const auto eqY = render (cs[k].second, x, 0);
                    std::vector<double> y ((size_t) total);
                    for (int i = 0; i < total; ++i) { const double w = wAt (i, tOn + P); y[(size_t) i] = (1 - w) * eqY[(size_t) i] + w * ev[(size_t) k][(size_t) i]; }
                    const double ref = std::max (maxD2 (eqY, tOn - pre, tOn + post), maxD2 (held[(size_t) k], tOn - pre, tOn + post));
                    onA = std::max (onA, maxD2 (y, tOn - pre, tOn + post) / ref);
                    if (t == 1) onK = std::max (onK, localKappa (y, tOn - pre, tOn + post, fs) / std::max (localKappa (eqY, tOn - pre, tOn + post, fs), localKappa (held[(size_t) k], tOn - pre, tOn + post, fs)));
                    for (int j = 0; j < nc; ++j)
                    {
                        if (j == k) continue;
                        for (int i = 0; i < total; ++i) { const double w = wAt (i, tOn + P); y[(size_t) i] = (1 - w) * held[(size_t) k][(size_t) i] + w * ev[(size_t) j][(size_t) i]; }
                        const double r2 = std::max (maxD2 (held[(size_t) k], tOn - pre, tOn + post), maxD2 (held[(size_t) j], tOn - pre, tOn + post));
                        const double v = maxD2 (y, tOn - pre, tOn + post) / r2;
                        if (v > 2.0) ++chOver;
                        chA = std::max (chA, v);
                        if (t == 1) chK = std::max (chK, localKappa (y, tOn - pre, tOn + post, fs) / std::max (localKappa (held[(size_t) k], tOn - pre, tOn + post, fs), localKappa (held[(size_t) j], tOn - pre, tOn + post, fs)));
                    }
                }
            }
        }
        std::printf ("%-8.0f | %8.2f %8.2f          | %8.2f %8.2f %6d / %d\n", Pms, onA, onK, chA, chK, chOver, 2 * 4 * nc * (nc - 1));
    }
}
std::vector<std::pair<double,double>> listenRP { { 20, 0 }, { 20, 10 }, { 20, 20 }, { 50, 0 }, { 50, 30 }, { 100, 0 }, { 100, 50 } };
void runListen3()
{
    const double fs = 48000.0;
    const int total = (int) (1.5 * fs);
    const int tOn = (int) (0.5 * fs) + 7;
    const int L = (int) std::lround (0.020 * fs);
    const ListenCase* cases = nullptr; (void) cases;
    const std::pair<const char*, eq::RawBand> cs[] = {
        { "Bell 1 kHz +12 Q 2", raw (0, 1000, 12, 2, 0, 0) }, { "Bell 50 Hz -18 Q 4", raw (0, 50, -18, 4, 0, 0) },
        { "Bell 5 kHz +6 Q 40", raw (0, 5000, 6, 40, 0, 0) }, { "Bell 10 Hz +12 Q 40", raw (0, 10, 12, 40, 0, 0) },
        { "Low Shelf 200 Hz +12", raw (1, 200, 12, 1, 2, 0) }, { "High Shelf 8 kHz -12", raw (2, 8000, -12, 1, 1, 0) },
        { "Low Cut 100 Hz 48 dB", raw (3, 100, 0, 1, 4, 0) }, { "High Cut 2 kHz 96 dB", raw (4, 2000, 0, 1, 7, 0) },
        { "Notch 50 Hz Q 10", raw (5, 50, 0, 10, 0, 0) }, { "Band Pass 1 kHz Q 10", raw (6, 1000, 0, 10, 0, 0) },
        { "Band Pass 8 kHz Q 40", raw (6, 8000, 0, 40, 0, 0) }, { "Band Pass 200 Hz Q 0.3", raw (6, 200, 0, 0.3, 0, 0) },
        { "Tilt 1 kHz +12", raw (7, 1000, 12, 1, 1, 0) }, { "Bell 2 kHz +12 Q 10", raw (0, 2000, 12, 10, 0, 0) },
        { "Band Pass 300 Hz Q 40", raw (6, 300, 0, 40, 0, 0) }, { "Band Pass 150 Hz Q 20", raw (6, 150, 0, 20, 0, 0) },
        { "Bell 400 Hz +12 Q 40", raw (0, 400, 12, 40, 0, 0) }, { "Band Pass 3 kHz Q 40", raw (6, 3000, 0, 40, 0, 0) },
        { "Low Shelf 20 Hz +12", raw (1, 20, 12, 1, 1, 0) }, { "High Cut 120 Hz 12 dB", raw (4, 120, 0, 1, 1, 0) },
    };
    const int nc = (int) (sizeof cs / sizeof cs[0]);

    auto tone = [&] (double f, int ph) { std::vector<double> x ((size_t) total); for (int i = 0; i < total; ++i) x[(size_t) i] = 0.5 * std::sin (2 * pi * f * i / fs + ph * pi / 4); return x; };
    int R = 0;
    auto render = [&] (const eq::RawBand& b, const std::vector<double>& x, int from) {
        std::vector<double> y ((size_t) total, 0.0);
        std::vector<double> part (x.begin() + from, x.end());
        if (from > 0)
            for (int i = 0; i < (int) part.size() && i < R; ++i) part[(size_t) i] *= eq::FadeTable::weightOf (i, R);
        Mono m (fs); m.start (b); uint64_t c = (uint64_t) from; m.run (part, 0, (int) part.size(), c);
        for (int i = from; i < total; ++i) y[(size_t) i] = part[(size_t) (i - from)];
        return y; };
    auto wAt = [&] (int i, int t0) { const int n = i - t0; return n < 0 ? 0.0 : n >= L ? 1.0 : eq::FadeTable::weightOf (n, L); };

    std::printf ("\n== listen3 [BENCH]: the listen filter's INPUT faded in over R (raised cosine) from the request, output weight after pre-roll P, 48 kHz, 4 phases\n");
    std::printf ("   listen on: %d listened bands; band change: all %d x %d ordered pairs as a crossfade between two listen filters, B from rest\n", nc, nc, nc - 1);
    std::printf ("%-12s | %-26s | %-26s\n", "R / P ms", "listen on: worst (a), (b)", "band change: worst (a), (b), cases > 2.0x");

    for (auto rp : listenRP)
    {
        const double Pms = rp.second;
        R = (int) std::lround (rp.first * 1e-3 * fs);
        const int P = (int) std::lround (Pms * 1e-3 * fs);
        double onA = 0, onK = 0, chA = 0, chK = 0; int chOver = 0;
        const int pre = (int) (0.050 * fs), post = (int) (0.070 * fs) + P;
        for (int t = 0; t < 2; ++t)
        {
            const double f = t == 0 ? 50.0 : 1000.0;
            for (int ph = 0; ph < 4; ++ph)
            {
                const auto x = tone (f, ph);
                std::vector<std::vector<double>> held, ev;
                for (int k = 0; k < nc; ++k) { held.push_back (render (listenOf (cs[k].second), x, 0)); ev.push_back (render (listenOf (cs[k].second), x, tOn)); }
                for (int k = 0; k < nc; ++k)
                {
                    const auto eqY = render (cs[k].second, x, 0);
                    std::vector<double> y ((size_t) total);
                    for (int i = 0; i < total; ++i) { const double w = wAt (i, tOn + P); y[(size_t) i] = (1 - w) * eqY[(size_t) i] + w * ev[(size_t) k][(size_t) i]; }
                    const double ref = std::max (maxD2 (eqY, tOn - pre, tOn + post), maxD2 (held[(size_t) k], tOn - pre, tOn + post));
                    onA = std::max (onA, maxD2 (y, tOn - pre, tOn + post) / ref);
                    if (t == 1) onK = std::max (onK, localKappa (y, tOn - pre, tOn + post, fs) / std::max (localKappa (eqY, tOn - pre, tOn + post, fs), localKappa (held[(size_t) k], tOn - pre, tOn + post, fs)));
                    for (int j = 0; j < nc; ++j)
                    {
                        if (j == k) continue;
                        for (int i = 0; i < total; ++i) { const double w = wAt (i, tOn + P); y[(size_t) i] = (1 - w) * held[(size_t) k][(size_t) i] + w * ev[(size_t) j][(size_t) i]; }
                        const double r2 = std::max (maxD2 (held[(size_t) k], tOn - pre, tOn + post), maxD2 (held[(size_t) j], tOn - pre, tOn + post));
                        const double v = maxD2 (y, tOn - pre, tOn + post) / r2;
                        if (v > 2.0) ++chOver;
                        chA = std::max (chA, v);
                        if (t == 1) chK = std::max (chK, localKappa (y, tOn - pre, tOn + post, fs) / std::max (localKappa (held[(size_t) k], tOn - pre, tOn + post, fs), localKappa (held[(size_t) j], tOn - pre, tOn + post, fs)));
                    }
                }
            }
        }
        std::printf ("%4.0f / %-5.0f | %8.2f %8.2f          | %8.2f %8.2f %6d / %d\n", rp.first, Pms, onA, onK, chA, chK, chOver, 2 * 4 * nc * (nc - 1));
    }
}

//==============================================================================
// g9unit: unit circle, draws whose slowest pole falls 200 dB within 6 s (model),
// IR rendered for 1.25 x that time, float output; scored where the closed form
// is within 80 dB of that response's maximum over the grid.
//==============================================================================
void runG9Unit()
{
    std::printf ("\n== g9unit [BENCH]: unit circle, float IR, draws (1..8 bands) whose slowest pole decays 200 dB within 6 s, length 1.25 x that; 96 points; within 80 dB of the response max\n");
    std::printf ("%-9s %8s %10s %12s\n", "rate", "kept", "of drawn", "worst dB");
    for (double fs : rates)
    {
        std::mt19937_64 rng (4242 + (uint64_t) fs);
        Engine e (fs);
        const auto hz = logGrid (10.0, std::min (30000.0, 0.49 * fs), 96);
        int kept = 0, drawnN = 0; double worst = 0;
        while (kept < 25)
        {
            const Config c = randomConfig (rng, 1, 8);
            ++drawnN;
            const Designed d = designAll (c, e.designer, true);
            const double rate = slowestDecayDbPerSecond (d, fs);
            const double t = rate > 0 ? 200.0 / rate : 1e9;
            if (t > 6.0) continue;
            ++kept;
            const int len = std::max (4096, (int) std::lround (1.25 * t * fs));
            Irs ir;
            const double in[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
            std::vector<double> l ((size_t) len), r ((size_t) len);
            for (int s = 0; s < 4; ++s)
            {
                e.start (c);
                std::fill (l.begin(), l.end(), 0.0); std::fill (r.begin(), r.end(), 0.0);
                l[0] = in[s][0]; r[0] = in[s][1];
                for (int p = 0; p < len; p += 4096) e.process (l.data() + p, r.data() + p, std::min (4096, len - p));
                ir.y[(size_t) s][0].resize ((size_t) len); ir.y[(size_t) s][1].resize ((size_t) len);
                for (int k = 0; k < len; ++k) { ir.y[(size_t) s][0][(size_t) k] = (float) l[(size_t) k]; ir.y[(size_t) s][1][(size_t) k] = (float) r[(size_t) k]; }
            }
            std::vector<std::array<cd, 4>> cfs, mes; std::array<double, 4> mx { -1e9, -1e9, -1e9, -1e9 };
            for (double f : hz)
            {
                const double w = 2.0 * pi * f / fs;
                cfs.push_back (drawn (composeOnCircle (d, w)));
                mes.push_back (measure (ir, w, 1.0));
                for (int q = 0; q < 4; ++q) mx[(size_t) q] = std::max (mx[(size_t) q], db20 (std::abs (cfs.back()[(size_t) q])));
            }
            for (size_t k = 0; k < hz.size(); ++k)
                for (int q = 0; q < 4; ++q)
                {
                    const double ref = db20 (std::abs (cfs[k][(size_t) q]));
                    if (ref < mx[(size_t) q] - 80.0) continue;
                    worst = std::max (worst, std::abs (db20 (std::abs (mes[k][(size_t) q])) - ref));
                }
        }
        std::printf ("%-9.0f %8d %10d %12.2e\n", fs, kept, drawnN, worst);
    }
}
} // namespace

int main (int argc, char** argv)
{
    const std::string m = argc > 1 ? argv[1] : "all";
    const bool all = m == "all";
    if (all || m == "decay")    runDecay();
    if (all || m == "g9")       { runG9 (50, 8); runG9 (20, 24); }
    if (all || m == "damped")   { runDamped (50, 8); runDamped (20, 24); }
    if (all || m == "json")     runJson();
    if (all || m == "analyzer") runAnalyzer();
    if (all || m == "listen")   runListen();
    if (all || m == "cost")     runCost();
    if (all || m == "g9diag")   runG9Diag();
    if (all || m == "listen2")  runListen2();
    if (all || m == "listen3")  runListen3();
    if (m == "listen3b") { listenRP = { { 20, 20 }, { 30, 30 }, { 50, 50 }, { 100, 100 } }; runListen3(); }
    if (all || m == "g9unit")   runG9Unit();
    return 0;
}
