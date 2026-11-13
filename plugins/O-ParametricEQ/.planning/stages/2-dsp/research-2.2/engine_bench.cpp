// O-ParametricEQ, Stage 2 research for Phase 2.2 (static engine, placement,
// transparency). A scratch bench over the real Source/dsp headers: a prototype
// of the section engine (trapezoidal SVF + TPT one-pole, double state) driven
// by eq::Designer::design. Not plugin code and not a gate.
//
//   clang++ -O3 -std=c++17 -I ../../../../Source engine_bench.cpp -o bin/engine_bench
//   ./bin/engine_bench <mode>
//
// Modes: ir | ms | g8 | neutral | blk | cpu | scale

#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace eq;

//==============================================================================
// Prototype engine
//==============================================================================
struct SvfRt  { double a1, a2, a3, k, mL, mB, mH; };
struct OpRt   { double Gc, cL, cH; };

struct SectionRt
{
    SvfRt svf;  OpRt op;
    bool hasSvf = false, hasOp = false;
};

static SectionRt toRt (const SectionParams& p)
{
    SectionRt r;
    r.hasSvf = p.hasSvf;  r.hasOp = p.hasOnePole;
    r.svf.a1 = 1.0 / (1.0 + p.g * (p.g + p.k));
    r.svf.a2 = p.g * r.svf.a1;
    r.svf.a3 = p.g * r.svf.a2;
    r.svf.k = p.k;  r.svf.mL = p.mL;  r.svf.mB = p.mB;  r.svf.mH = p.mH;
    r.op.Gc = p.G / (1.0 + p.G);  r.op.cL = p.cL;  r.op.cH = p.cH;
    return r;
}

struct LaneState { double ic1[maxSections] = {}, ic2[maxSections] = {}, s[maxSections] = {}; };

// Section-major: one section over the whole block, in place.
static inline void runSvf (const SvfRt& c, double& ic1, double& ic2, double* x, int n)
{
    double s1 = ic1, s2 = ic2;
    for (int i = 0; i < n; ++i)
    {
        const double in = x[i];
        const double v3 = in - s2;
        const double v1 = c.a1 * s1 + c.a2 * v3;
        const double v2 = s2 + c.a2 * s1 + c.a3 * v3;
        s1 = 2.0 * v1 - s1;
        s2 = 2.0 * v2 - s2;
        x[i] = c.mL * v2 + c.mB * v1 + c.mH * (in - c.k * v1 - v2);
    }
    ic1 = s1;  ic2 = s2;
}

static inline void runOp (const OpRt& c, double& st, double* x, int n)
{
    double s = st;
    for (int i = 0; i < n; ++i)
    {
        const double in = x[i];
        const double v = (in - s) * c.Gc;
        const double lp = v + s;
        s = lp + v;
        x[i] = c.cL * lp + c.cH * (in - lp);
    }
    st = s;
}

struct BandRt
{
    int n = 0;
    SectionRt sec[maxSections];
    LaneState lane[2];

    void set (const BandDesign& d)
    {
        n = d.numSections;
        for (int i = 0; i < n; ++i) sec[i] = toRt (d.params[(size_t) i]);
    }
    void clear() { lane[0] = LaneState();  lane[1] = LaneState(); }

    void run (int l, double* x, int num)
    {
        for (int i = 0; i < n; ++i)
        {
            if (sec[i].hasSvf) runSvf (sec[i].svf, lane[l].ic1[i], lane[l].ic2[i], x, num);
            if (sec[i].hasOp)  runOp  (sec[i].op,  lane[l].s[i], x, num);
        }
    }
};

static BandDesign designOf (Designer& d, Type t, double f, double g, double q, int slope)
{
    BandSettings s { t, f, g, q, slope };
    BandDesign out;
    d.design (s, out);
    return out;
}

static uint64_t rngState = 0x9E3779B97F4A7C15ull;
static double urand()
{
    rngState ^= rngState << 13;  rngState ^= rngState >> 7;  rngState ^= rngState << 17;
    return (double) (rngState >> 11) * (1.0 / 9007199254740992.0);
}
static float noise() { return (float) ((urand() * 2.0 - 1.0) * 0.25); }

static const char* typeName (Type t)
{
    static const char* n[] = { "Bell", "LowShelf", "HighShelf", "LowCut", "HighCut", "Notch", "BandPass", "Tilt" };
    return n[(int) t];
}
static const int slopeDb[8] = { 6, 12, 18, 24, 36, 48, 72, 96 };

//==============================================================================
// ir: rendered impulse response against the closed form
//==============================================================================
struct IrCase { Type t; double f, g, q; int slope; };

static void modeIr()
{
    const IrCase cases[] = {
        { Type::bell, 1000, 12, 1, 1 },      { Type::bell, 10, 30, 40, 1 },        { Type::bell, 18000, -18, 5, 1 },
        { Type::bell, 25000, 12, 2, 1 },     { Type::bell, 100, -30, 0.025, 1 },
        { Type::lowShelf, 100, 6, 1, 1 },    { Type::lowShelf, 300, -12, 1, 7 },   { Type::lowShelf, 60, 9, 1, 2 },
        { Type::highShelf, 8000, -12, 1, 5 },{ Type::highShelf, 15000, 30, 3, 7 },
        { Type::lowCut, 80, 0, 1, 3 },       { Type::lowCut, 30, 0, 1, 7 },        { Type::lowCut, 200, 0, 4, 0 },
        { Type::highCut, 12000, 0, 1, 1 },   { Type::highCut, 6000, 0, 1, 7 },     { Type::highCut, 26000, 0, 2, 5 },
        { Type::notch, 1000, 0, 10, 1 },     { Type::notch, 50, 0, 40, 1 },
        { Type::bandPass, 500, 0, 2, 1 },    { Type::bandPass, 10, 0, 40, 1 },
        { Type::tilt, 1000, 6, 1, 1 },       { Type::tilt, 700, -12, 1, 5 },
    };
    const double rates[] = { 44100.0, 96000.0 };
    const int F = 160;

    std::printf ("%-10s %7s %5s %6s %3s %6s | %9s | %-22s | %-22s | %s\n", "type", "fc", "gain", "Q", "sl", "fs",
                 "len s", "double: >-80 / all", "float: >-80 / all", "first |err|>0.01 dB level (float)");

    for (double fs : rates)
        for (const auto& c : cases)
        {
            Designer des;  des.prepare (fs);
            BandDesign bd = designOf (des, c.t, c.f, c.g, c.q, c.slope);
            BandRt band;  band.set (bd);

            // render until the tail is 300 dB below the peak, or 120 s
            const int chunk = 4096;
            const long cap = (long) (120.0 * fs);
            std::vector<double> h;  h.reserve (1 << 20);
            double peak = 0.0;
            std::vector<double> buf ((size_t) chunk);
            long pos = 0;
            while (pos < cap)
            {
                std::fill (buf.begin(), buf.end(), 0.0);
                if (pos == 0) buf[0] = 1.0;
                band.run (0, buf.data(), chunk);
                double m = 0.0;
                for (double v : buf) m = std::max (m, std::fabs (v));
                peak = std::max (peak, m);
                h.insert (h.end(), buf.begin(), buf.end());
                pos += chunk;
                if (m < peak * 1e-15) break;
            }
            const size_t N = h.size();

            double errD = 0, errDall = 0, errF = 0, errFall = 0, firstBad = 1e9;
            for (int j = 0; j < F; ++j)
            {
                const double hz = 20.0 * std::pow (std::min (20000.0, 0.499 * fs) / 20.0, (double) j / (F - 1));
                const double w = 2.0 * pi * hz / fs;
                const double cr = std::cos (w), ci = -std::sin (w);
                double pr = 1.0, pi_ = 0.0, ar = 0, ai = 0, fr = 0, fi = 0;
                for (size_t n = 0; n < N; ++n)
                {
                    const double v = h[n];
                    const double vf = (double) (float) v;
                    ar += v * pr;   ai += v * pi_;
                    fr += vf * pr;  fi += vf * pi_;
                    const double t = pr * cr - pi_ * ci;
                    pi_ = pr * ci + pi_ * cr;  pr = t;
                    if ((n & 4095) == 4095) { const double m = std::sqrt (pr * pr + pi_ * pi_);  pr /= m;  pi_ /= m; }
                }
                const double ref = bandMagDb (bd, w);
                const double dD = std::fabs (powerToDb (ar * ar + ai * ai) - ref);
                const double dF = std::fabs (powerToDb (fr * fr + fi * fi) - ref);
                errDall = std::max (errDall, dD);  errFall = std::max (errFall, dF);
                if (ref > -80.0) { errD = std::max (errD, dD);  errF = std::max (errF, dF); }
                if (dF > 0.01) firstBad = std::min (firstBad, -ref);
            }
            char a[64], b[64], fb[32];
            std::snprintf (a, sizeof a, "%.2e / %.2e", errD, errDall);
            std::snprintf (b, sizeof b, "%.2e / %.2e", errF, errFall);
            if (firstBad > 1e8) std::snprintf (fb, sizeof fb, "none");
            else std::snprintf (fb, sizeof fb, "%.0f dB", -firstBad);
            std::printf ("%-10s %7.0f %5.0f %6.3f %3d %6.0f | %9.2f | %-22s | %-22s | %s\n",
                         typeName (c.t), c.f, c.g, c.q, slopeDb[c.slope], fs, (double) N / fs, a, b, fb);
        }
}

//==============================================================================
// ms: Mid / Side difference injection
//==============================================================================
static void modeMs()
{
    const double fs = 48000.0;
    const int N = 1 << 17;
    Designer des;  des.prepare (fs);

    struct Cfg { const char* name; int nb; double g; bool side; };
    const Cfg cfgs[] = { { "1 Mid bell +6", 1, 6, false }, { "1 Mid bell +30", 1, 30, false }, { "1 Mid bell -30", 1, -30, false },
                         { "24 Mid bells +6", 24, 6, false }, { "24 Mid bells +12", 24, 12, false },
                         { "1 Side bell +30", 1, 30, true }, { "24 Side bells +12", 24, 12, true } };

    std::printf ("%-20s | %-28s | %-28s | %s\n", "config (48 kHz)", "untouched M/S err, double out",
                 "float out (dB re input peak)", "re output peak");

    for (const auto& c : cfgs)
    {
        rngState = 12345;
        std::vector<float> inL ((size_t) N), inR ((size_t) N);
        for (int i = 0; i < N; ++i) { inL[(size_t) i] = noise();  inR[(size_t) i] = noise(); }
        std::vector<double> L (inL.begin(), inL.end()), R (inR.begin(), inR.end()), m ((size_t) N);

        std::vector<BandRt> bands ((size_t) c.nb);
        for (int b = 0; b < c.nb; ++b)
        {
            const double f = 40.0 * std::pow (400.0, c.nb > 1 ? (double) b / (c.nb - 1) : 0.5);
            bands[(size_t) b].set (designOf (des, Type::bell, f, c.g, 1.0, 1));
        }
        for (auto& band : bands)
        {
            for (int i = 0; i < N; ++i)
                m[(size_t) i] = c.side ? 0.5 * (L[(size_t) i] - R[(size_t) i]) : 0.5 * (L[(size_t) i] + R[(size_t) i]);
            std::vector<double> y (m);
            band.run (0, y.data(), N);
            for (int i = 0; i < N; ++i)
            {
                const double d = y[(size_t) i] - m[(size_t) i];
                L[(size_t) i] += d;
                R[(size_t) i] += c.side ? -d : d;
            }
        }
        double inPeak = 0, outPeak = 0, eD = 0, eF = 0;
        for (int i = 0; i < N; ++i)
        {
            const double il = inL[(size_t) i], ir = inR[(size_t) i];
            inPeak = std::max ({ inPeak, std::fabs (il), std::fabs (ir) });
            outPeak = std::max ({ outPeak, std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]) });
            const double refU = c.side ? 0.5 * (il + ir) : 0.5 * (il - ir);       // the untouched component
            const double uD = c.side ? 0.5 * (L[(size_t) i] + R[(size_t) i]) : 0.5 * (L[(size_t) i] - R[(size_t) i]);
            const double lf = (double) (float) L[(size_t) i], rf = (double) (float) R[(size_t) i];
            const double uF = c.side ? 0.5 * (lf + rf) : 0.5 * (lf - rf);
            eD = std::max (eD, std::fabs (uD - refU));
            eF = std::max (eF, std::fabs (uF - refU));
        }
        std::printf ("%-20s | %10.1f dB re input peak   | %10.1f                   | %8.1f   (out peak %+.1f dB re in)\n", c.name,
                     20 * std::log10 (std::max (eD, 1e-300) / inPeak), 20 * std::log10 (std::max (eF, 1e-300) / inPeak),
                     20 * std::log10 (std::max (eF, 1e-300) / outPeak), 20 * std::log10 (outPeak / inPeak));
    }
}

//==============================================================================
// g8: first non-zero output sample
//==============================================================================
static void modeG8()
{
    std::printf ("%-10s %7s %4s %7s | %-12s %-12s | first non-zero index: double / float\n", "type", "fc", "sl", "fs", "y[0] double", "y[0] float");
    const double rates[] = { 44100.0, 192000.0 };
    struct C { Type t; double f; int slope; };
    const C cs[] = { { Type::highCut, 10, 7 }, { Type::highCut, 10, 5 }, { Type::highCut, 10, 3 }, { Type::highCut, 100, 7 }, { Type::highCut, 1000, 7 },
                     { Type::highCut, 10, 1 }, { Type::highCut, 10, 0 }, { Type::bandPass, 10, 1 }, { Type::lowCut, 30000, 7 }, { Type::notch, 10, 1 },
                     { Type::bell, 10, 1 } };
    for (double fs : rates)
        for (const auto& c : cs)
        {
            Designer des;  des.prepare (fs);
            BandRt band;  band.set (designOf (des, c.t, c.f, 12.0, 0.025, c.slope));
            const int n = 1 << 16;
            std::vector<double> x ((size_t) n, 0.0);  x[0] = 1.0;
            band.run (0, x.data(), n);
            int fd = -1, ff = -1;
            for (int i = 0; i < n; ++i) { if (fd < 0 && x[(size_t) i] != 0.0) fd = i;  if (ff < 0 && (float) x[(size_t) i] != 0.0f) ff = i; }
            // also with flush-to-zero semantics for float: treat |v| < FLT_MIN as zero
            int fz = -1;
            for (int i = 0; i < n; ++i) if (std::fabs ((float) x[(size_t) i]) >= 1.17549435e-38f) { fz = i;  break; }
            std::printf ("%-10s %7.0f %4d %7.0f | %-12.3e %-12.3e | %d / %d (normal float: %d)\n", typeName (c.t), c.f, slopeDb[c.slope], fs,
                         x[0], (double) (float) x[0], fd, ff, fz);
        }
}

//==============================================================================
// neutral: is a 0 dB gain band bit-transparent?  scale: gEff forms
//==============================================================================
static void modeNeutral()
{
    const double fs = 48000.0;
    Designer des;  des.prepare (fs);
    const int N = 1 << 16;
    struct C { Type t; double f, q; int slope; };
    const C cs[] = { { Type::bell, 1000, 1, 1 }, { Type::bell, 10, 40, 1 }, { Type::bell, 25000, 1, 1 }, { Type::lowShelf, 100, 1, 1 },
                     { Type::highShelf, 8000, 1, 7 }, { Type::highShelf, 8000, 1, 0 }, { Type::tilt, 1000, 1, 3 } };
    std::printf ("0 dB gain band, 48 kHz, noise -12 dBFS: samples that differ after float conversion, worst double error\n");
    for (const auto& c : cs)
    {
        BandDesign bd = designOf (des, c.t, c.f, 0.0, c.q, c.slope);
        bool ident = true;
        for (int i = 0; i < bd.numSections; ++i)
        {
            const auto& p = bd.params[(size_t) i];
            if (p.hasSvf && ! (p.mL == 1.0 && p.mH == 1.0 && p.mB == p.k)) ident = false;
            if (p.hasOnePole && ! (p.cL == 1.0 && p.cH == 1.0)) ident = false;
        }
        BandRt band;  band.set (bd);
        rngState = 777;
        std::vector<float> in ((size_t) N);
        for (auto& v : in) v = noise();
        std::vector<double> x (in.begin(), in.end());
        band.run (0, x.data(), N);
        int diff = 0;  double worst = 0;
        for (int i = 0; i < N; ++i)
        {
            if ((float) x[(size_t) i] != in[(size_t) i]) ++diff;
            worst = std::max (worst, std::fabs (x[(size_t) i] - (double) in[(size_t) i]));
        }
        std::printf ("  %-10s fc %6.0f Q %4.1f %2d dB/oct: params exactly neutral %s; float samples differing %d of %d; worst %.2e (%.0f dB re 0.25)\n",
                     typeName (c.t), c.f, c.q, slopeDb[c.slope], ident ? "yes" : "NO", diff, N, worst, 20 * std::log10 (std::max (worst, 1e-300) / 0.25));
    }
}

// A 0 dB band on a signal with very small samples next to large ones.
static void modeNeutralTiny()
{
    const double fs = 48000.0;
    Designer des;  des.prepare (fs);
    const int N = 1 << 18;
    for (int nb : { 1, 24 })
    {
        std::vector<BandRt> bands ((size_t) nb);
        for (int b = 0; b < nb; ++b) bands[(size_t) b].set (designOf (des, Type::bell, 40.0 * std::pow (400.0, nb > 1 ? b / 23.0 : 0.5), 0.0, 4.0, 1));
        rngState = 2024;
        std::vector<float> in ((size_t) N);
        for (int i = 0; i < N; ++i) in[(size_t) i] = ((i / 64) % 2 == 0) ? noise() * 4.0f : noise() * 1.0e-9f;   // 0 dBFS bursts, then -180 dBFS
        std::vector<double> x (in.begin(), in.end());
        for (auto& band : bands) band.run (0, x.data(), N);
        int diff = 0;
        for (int i = 0; i < N; ++i) if ((float) x[(size_t) i] != in[(size_t) i]) ++diff;
        std::printf ("  %2d bell(s) at 0 dB, Q 4, bursts 0 dBFS / -180 dBFS: float samples differing %d of %d\n", nb, diff, N);
    }
}

static void modeScale()
{
    // gEff = gain * scale / 100 with scale = 100: which form returns the float gain exactly?
    int badA = 0, badB = 0, badC = 0;  const int n = 2000000;
    rngState = 99;
    for (int i = 0; i < n; ++i)
    {
        const float g = (float) (urand() * 60.0 - 30.0);
        const float sc = 100.0f;
        const double a = (double) g * (double) sc / 100.0;          // (g * s) / 100
        const double b = (double) g * ((double) sc / 100.0);        // g * (s / 100)
        const double c = (double) g * ((double) sc * 0.01);         // g * (s * 0.01)
        if (a != (double) g) ++badA;
        if (b != (double) g) ++badB;
        if (c != (double) g) ++badC;
    }
    std::printf ("gain_scale = 100, %d random float gains: (g*s)/100 inexact %d; g*(s/100) inexact %d; g*(s*0.01) inexact %d\n", n, badA, badB, badC);
    // scale 0 -> exactly 0 for all forms; scale 200 -> exactly 2g for all forms (power of two)
}

//==============================================================================
// blk: block-size invariance of the static engine
//==============================================================================
static void modeBlk()
{
    const double fs = 48000.0;
    Designer des;  des.prepare (fs);
    const int N = 1 << 16;
    rngState = 4242;
    std::vector<double> in ((size_t) N);
    for (auto& v : in) v = (double) noise();
    const int sizes[] = { 1, 7, 16, 32, 64, 512, 2048, 4096, N };
    std::vector<double> ref;
    for (int bs : sizes)
    {
        BandRt a, b;
        a.set (designOf (des, Type::highShelf, 6000, 9, 2, 7));
        b.set (designOf (des, Type::lowCut, 40, 0, 1, 5));
        std::vector<double> x (in);
        for (int p = 0; p < N; p += bs)
        {
            const int n = std::min (bs, N - p);
            a.run (0, x.data() + p, n);
            b.run (0, x.data() + p, n);
        }
        if (ref.empty()) ref = x;
        std::printf ("  block %5d: %s\n", bs, std::memcmp (ref.data(), x.data(), sizeof (double) * (size_t) N) == 0 ? "bit-identical to block 1" : "DIFFERS");
    }
}

//==============================================================================
// cpu
//==============================================================================
using Clock = std::chrono::steady_clock;
static double secs (Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double> (b - a).count(); }

// Two lanes of one section in one loop (shared coefficients, adjacent states).
static inline void runSvf2 (const SvfRt& c, double* st /* ic1L ic1R ic2L ic2R */, double* l, double* r, int n)
{
    double s1l = st[0], s1r = st[1], s2l = st[2], s2r = st[3];
    for (int i = 0; i < n; ++i)
    {
        const double il = l[i], ir = r[i];
        const double v3l = il - s2l, v3r = ir - s2r;
        const double v1l = c.a1 * s1l + c.a2 * v3l, v1r = c.a1 * s1r + c.a2 * v3r;
        const double v2l = s2l + c.a2 * s1l + c.a3 * v3l, v2r = s2r + c.a2 * s1r + c.a3 * v3r;
        s1l = 2.0 * v1l - s1l;  s1r = 2.0 * v1r - s1r;
        s2l = 2.0 * v2l - s2l;  s2r = 2.0 * v2r - s2r;
        l[i] = c.mL * v2l + c.mB * v1l + c.mH * (il - c.k * v1l - v2l);
        r[i] = c.mL * v2r + c.mB * v1r + c.mH * (ir - c.k * v1r - v2r);
    }
    st[0] = s1l;  st[1] = s1r;  st[2] = s2l;  st[3] = s2r;
}

// Ramping path: the five SVF parameters move linearly per sample, one division.
static inline void runSvfRamp (SectionParams p, const SectionParams& inc, double& ic1, double& ic2, double* x, int n)
{
    double s1 = ic1, s2 = ic2;
    for (int i = 0; i < n; ++i)
    {
        const double a1 = 1.0 / (1.0 + p.g * (p.g + p.k));
        const double a2 = p.g * a1, a3 = p.g * a2;
        const double in = x[i];
        const double v3 = in - s2;
        const double v1 = a1 * s1 + a2 * v3;
        const double v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0 * v1 - s1;  s2 = 2.0 * v2 - s2;
        x[i] = p.mL * v2 + p.mB * v1 + p.mH * (in - p.k * v1 - v2);
        p.g += inc.g;  p.k += inc.k;  p.mL += inc.mL;  p.mB += inc.mB;  p.mH += inc.mH;
    }
    ic1 = s1;  ic2 = s2;
}

static void modeCpu()
{
    const double fs = 48000.0;
    const int block = 512;
    const int blocks = (int) (20.0 * fs / block);
    const double audioSecs = (double) blocks * block / fs;
    Designer des;  des.prepare (fs);

    rngState = 31337;
    std::vector<float> fl ((size_t) block), fr ((size_t) block);
    for (int i = 0; i < block; ++i) { fl[(size_t) i] = noise();  fr[(size_t) i] = noise(); }
    std::vector<double> L ((size_t) block), R ((size_t) block);
    double sink = 0;

    struct Cfg { const char* name; Type t; int slope; double g; };
    const Cfg cfgs[] = { { "24 cuts x 96 dB/oct (PERF-02)", Type::highCut, 7, 0 }, { "24 shelves x 96 dB/oct", Type::highShelf, 7, 6 },
                         { "24 bells", Type::bell, 1, 6 }, { "24 cuts x 12 dB/oct", Type::lowCut, 1, 0 } };

    std::printf ("48 kHz stereo, 512-sample blocks, %.0f s of audio, scalar, this machine. %% of one core.\n", audioSecs);
    std::printf ("%-32s | %-10s | %-10s | %-12s\n", "settled", "per lane", "lane pair", "ns / SVF-sample (per lane)");

    for (const auto& c : cfgs)
    {
        std::vector<BandRt> bands (24);
        int svfCount = 0;
        for (int b = 0; b < 24; ++b)
        {
            const double f = (c.t == Type::highCut) ? 20000.0 - 300.0 * b : 30.0 * std::pow (500.0, b / 23.0);
            BandDesign bd = designOf (des, c.t, f, c.g, 1.0, c.slope);
            bands[(size_t) b].set (bd);
            for (int i = 0; i < bd.numSections; ++i) if (bd.params[(size_t) i].hasSvf) ++svfCount;
        }
        // A: section-major, lane by lane
        auto t0 = Clock::now();
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < block; ++i) { L[(size_t) i] = fl[(size_t) i];  R[(size_t) i] = fr[(size_t) i]; }
            for (auto& band : bands) { band.run (0, L.data(), block);  band.run (1, R.data(), block); }
            sink += L[7] + R[9];
        }
        auto t1 = Clock::now();
        // B: section-major, the two lanes in one loop (SVF only; one-poles lane by lane)
        for (auto& band : bands) band.clear();
        std::vector<std::array<double, 4>> st (24 * maxSections, std::array<double, 4> { 0, 0, 0, 0 });
        auto t2 = Clock::now();
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < block; ++i) { L[(size_t) i] = fl[(size_t) i];  R[(size_t) i] = fr[(size_t) i]; }
            for (int b = 0; b < 24; ++b)
            {
                auto& band = bands[(size_t) b];
                for (int i = 0; i < band.n; ++i)
                {
                    if (band.sec[i].hasSvf) runSvf2 (band.sec[i].svf, st[(size_t) (b * maxSections + i)].data(), L.data(), R.data(), block);
                    if (band.sec[i].hasOp) { runOp (band.sec[i].op, band.lane[0].s[i], L.data(), block);  runOp (band.sec[i].op, band.lane[1].s[i], R.data(), block); }
                }
            }
            sink += L[7] + R[9];
        }
        auto t3 = Clock::now();
        // C: sample-major (every section of a band inside one sample loop), lane by lane
        for (auto& band : bands) band.clear();
        auto t4 = Clock::now();
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < block; ++i) { L[(size_t) i] = fl[(size_t) i];  R[(size_t) i] = fr[(size_t) i]; }
            for (auto& band : bands)
                for (int l = 0; l < 2; ++l)
                {
                    double* x = l == 0 ? L.data() : R.data();
                    auto& stt = band.lane[l];
                    for (int i = 0; i < block; ++i)
                    {
                        double v = x[i];
                        for (int q = 0; q < band.n; ++q)
                        {
                            if (band.sec[q].hasSvf)
                            {
                                const auto& cc = band.sec[q].svf;
                                const double v3 = v - stt.ic2[q];
                                const double v1 = cc.a1 * stt.ic1[q] + cc.a2 * v3;
                                const double v2 = stt.ic2[q] + cc.a2 * stt.ic1[q] + cc.a3 * v3;
                                stt.ic1[q] = 2.0 * v1 - stt.ic1[q];  stt.ic2[q] = 2.0 * v2 - stt.ic2[q];
                                v = cc.mL * v2 + cc.mB * v1 + cc.mH * (v - cc.k * v1 - v2);
                            }
                            if (band.sec[q].hasOp)
                            {
                                const auto& oc = band.sec[q].op;
                                const double u = (v - stt.s[q]) * oc.Gc;
                                const double lp = u + stt.s[q];
                                stt.s[q] = lp + u;
                                v = oc.cL * lp + oc.cH * (v - lp);
                            }
                        }
                        x[i] = v;
                    }
                }
            sink += L[7] + R[9];
        }
        auto t5 = Clock::now();
        const double a = secs (t0, t1), bsec = secs (t2, t3), csec = secs (t4, t5);
        std::printf ("%-32s | %8.2f %% | %8.2f %% | %.2f | sample-major %.2f %%\n", c.name, 100 * a / audioSecs, 100 * bsec / audioSecs,
                     svfCount ? 1e9 * a / ((double) blocks * block * 2 * svfCount) : 0.0, 100 * csec / audioSecs);
    }

    // pass-through conversion cost (float -> double -> float), no bands
    {
        std::vector<float> ol ((size_t) block), orr ((size_t) block);
        auto t0 = Clock::now();
        for (int k = 0; k < blocks * 20; ++k)
        {
            for (int i = 0; i < block; ++i) { L[(size_t) i] = fl[(size_t) i];  R[(size_t) i] = fr[(size_t) i]; }
            for (int i = 0; i < block; ++i) { ol[(size_t) i] = (float) L[(size_t) i];  orr[(size_t) i] = (float) R[(size_t) i]; }
            sink += ol[(size_t) (k & 511)];
        }
        auto t1 = Clock::now();
        std::printf ("float -> double -> float round trip, stereo: %.3f %% of a core\n", 100 * secs (t0, t1) / (audioSecs * 20));
    }

    // ramping path: 24 cuts x 96 dB/oct, every section interpolating (parameters held equal: inc = 0, same arithmetic)
    {
        std::vector<BandDesign> ds (24);
        for (int b = 0; b < 24; ++b) ds[(size_t) b] = designOf (des, Type::highCut, 20000.0 - 300.0 * b, 0, 1.0, 7);
        std::vector<LaneState> ls (48);
        volatile double tiny = 1.0e-13;      // non-zero and opaque, so the division cannot be hoisted
        SectionParams inc {};  inc.g = inc.k = inc.mL = inc.mB = inc.mH = tiny;
        auto t0 = Clock::now();
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < block; ++i) { L[(size_t) i] = fl[(size_t) i];  R[(size_t) i] = fr[(size_t) i]; }
            for (int b = 0; b < 24; ++b)
                for (int i = 0; i < ds[(size_t) b].numSections; ++i)
                    for (int off = 0; off < block; off += 16)     // control grid K = 16
                    {
                        runSvfRamp (ds[(size_t) b].params[(size_t) i], inc, ls[(size_t) (2 * b)].ic1[i], ls[(size_t) (2 * b)].ic2[i], L.data() + off, 16);
                        runSvfRamp (ds[(size_t) b].params[(size_t) i], inc, ls[(size_t) (2 * b + 1)].ic1[i], ls[(size_t) (2 * b + 1)].ic2[i], R.data() + off, 16);
                    }
            sink += L[7] + R[9];
        }
        auto t1 = Clock::now();
        std::printf ("ramping path, 24 cuts x 96 dB/oct, 16-sample segments, per-sample interpolation + division: %.2f %% of a core\n",
                     100 * secs (t0, t1) / audioSecs);
    }

    // design cost per band (real Designer::design), 3000 ticks/s x 24 bands
    {
        struct D { const char* name; Type t; int slope; double g; double q; };
        const D dsn[] = { { "bell", Type::bell, 1, 9, 2 }, { "high cut 96", Type::highCut, 7, 0, 1 }, { "high shelf 12", Type::highShelf, 1, 9, 1 },
                          { "high shelf 48", Type::highShelf, 5, 9, 1 }, { "high shelf 96", Type::highShelf, 7, 9, 1 }, { "tilt 96", Type::tilt, 7, 9, 1 } };
        for (const auto& d : dsn)
        {
            const int reps = 200000;
            BandDesign out;
            auto t0 = Clock::now();
            for (int i = 0; i < reps; ++i)
            {
                BandSettings s { d.t, 200.0 + 15000.0 * (double) (i % 997) / 997.0, d.g + 1e-3 * (i % 13), d.q, d.slope };
                des.design (s, out);
                sink += out.params[0].g;
            }
            auto t1 = Clock::now();
            const double us = 1e6 * secs (t0, t1) / reps;
            std::printf ("design %-14s: %6.3f us per band; 24 bands at 3000 ticks/s = %5.1f %% of a core\n", d.name, us, 100 * us * 1e-6 * 24 * 3000);
        }
    }
    std::printf ("(sink %g)\n", sink);
}

//==============================================================================
int main (int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "ir") modeIr();
    else if (mode == "tiny") modeNeutralTiny();
    else if (mode == "ms") modeMs();
    else if (mode == "g8") modeG8();
    else if (mode == "neutral") modeNeutral();
    else if (mode == "scale") modeScale();
    else if (mode == "blk") modeBlk();
    else if (mode == "cpu") modeCpu();
    else { std::printf ("modes: ir | ms | g8 | neutral | scale | blk | cpu\n");  return 1; }
    return 0;
}
