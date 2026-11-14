// O-ParametricEQ, Stage 2 research for Phase 2.3 (continuous modulation).
// A scratch bench over the real Source/dsp headers: a prototype of the control
// grid, the control ramps and the ramping path of the section engine, driven
// by eq::Designer::design. Not plugin code and not a gate.
//
//   clang++ -O3 -std=c++17 -I ../../../../Source ramp_bench.cpp -o <scratch>/ramp_bench
//   ramp_bench <mode> [args]
//
// Modes:
//   g3      gate G3 sweep rows: local kappa against the held references, against a same-motion
//           per-sample reference (K = 1), with per-sample interpolation off, and with no smoothing
//   g3var   the same rows for other grids and interpolation forms, as a ratio to the reference
//   lyap    the SVF state matrix: norm, two-step norm, a greedy adversary; the designer's g / k / G ranges
//   g5 [s]  random stress in double, 8 rates, 24 bands stereo (default 20 s of audio per rate)
//   g7      noise floor and ring-down, 192 kHz, bell 10 Hz Q 40
//   blk     bit identity over host block sizes with parameter events on a fixed sample grid
//   cpu     settled and sweeping cost, 48 kHz stereo
//   keep    a continuous change keeps the filter state (a row for VERIFICATION-2.2 gap 1)
//   zero    design continuity where the gain changes sign

#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/BandParams.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#if defined (__aarch64__)
 #include <fenv.h>
#endif

using namespace eq;

//==============================================================================
// Prototype engine
//==============================================================================
struct Rt   // runtime coefficients of one section
{
    double a1 = 1, a2 = 0, a3 = 0, k = 1, mL = 1, mB = 1, mH = 1;
    double Gc = 0, cL = 1, cH = 1;
};

static Rt toRt (const SectionParams& p)
{
    Rt r;
    r.a1 = 1.0 / (1.0 + p.g * (p.g + p.k));
    r.a2 = p.g * r.a1;
    r.a3 = p.g * r.a2;
    r.k = p.k;  r.mL = p.mL;  r.mB = p.mB;  r.mH = p.mH;
    r.Gc = p.G / (1.0 + p.G);  r.cL = p.cL;  r.cH = p.cH;
    return r;
}

static inline double svfTick (double a1, double a2, double a3, double k, double mL, double mB, double mH,
                              double& ic1, double& ic2, double x)
{
    const double v3 = x - ic2;
    const double v1 = a1 * ic1 + a2 * v3;
    const double v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0 * v1 - ic1;
    ic2 = 2.0 * v2 - ic2;
    return mL * v2 + mB * v1 + mH * (x - k * v1 - v2);
}

static inline double opTick (double Gc, double cL, double cH, double& s, double x)
{
    const double v = (x - s) * Gc;
    const double lp = v + s;
    s = lp + v;
    return cL * lp + cH * (x - lp);
}

enum Interp
{
    interpSpec = 0,     // per-sample linear on g k mL mB mH G cL cH; a1 and Gc by division (ARCHITECTURE)
    interpHold = 1,     // no per-sample interpolation: the tick's design is applied at once
    interpCoef = 2,     // per-sample linear on the runtime coefficients a1 a2 a3 k m.. Gc c.. (no division)
    interpGc   = 3,     // as Spec, but the one-pole interpolates Gc (one division fewer)
    interpGeo  = 4      // as Spec, but g and k move geometrically (a ratio per sample), m.. linearly
};

struct RBand
{
    bool active = false;
    bool haveDesign = false;
    BandSettings tgt {};
    double lf = 0, gdb = 0, lq = 0;             // ramped control values
    double lfInc = 0, gInc = 0, lqInc = 0;
    int stepsLeft = 0;
    bool retarget = false;

    int n = 0;
    bool hasSvf[maxSections] = {}, hasOp[maxSections] = {};
    SectionParams P[maxSections], T[maxSections], inc[maxSections];
    Rt rt[maxSections], rtT[maxSections], rtInc[maxSections];
    bool seg = false;

    double ic1[maxSections][2] = {}, ic2[maxSections][2] = {}, s[maxSections][2] = {};

    void clearState()
    {
        std::memset (ic1, 0, sizeof (ic1));  std::memset (ic2, 0, sizeof (ic2));  std::memset (s, 0, sizeof (s));
    }
};

static long flagChanges = 0;      // a continuous move that changed numSections or a half flag
static long designCalls = 0;
static double gMin = 1e300, gMax = 0, kMin = 1e300, kMax = 0, GMin = 1e300, GMax = 0, mMax = 0;
static bool trackRanges = false;

struct Engine
{
    double fs = 48000;
    int K = 16;
    int rampTicks = 60;
    int interp = interpSpec;
    bool smooth = true;
    int lanes = 1;
    int phase = 0;
    Designer des;
    std::vector<RBand> bands;

    void prepare (double rate, int numBands, int Kopt = 0, double rampMs = 20.0)
    {
        fs = rate;
        K = Kopt > 0 ? Kopt : (rate <= 48000.0 ? 16 : (rate <= 96000.0 ? 32 : 64));
        rampTicks = std::max (1, (int) std::lround (rampMs * 0.001 * rate / K));
        des.prepare (rate);
        bands.assign ((size_t) numBands, RBand());
        phase = 0;
    }

    void designInto (RBand& b, const BandSettings& s, SectionParams* out)
    {
        BandDesign d;
        des.design (s, d);
        ++designCalls;

        if (b.haveDesign)
        {
            bool same = d.numSections == b.n;
            for (int i = 0; same && i < d.numSections; ++i)
                same = d.params[(size_t) i].hasSvf == b.hasSvf[i] && d.params[(size_t) i].hasOnePole == b.hasOp[i];
            if (! same) ++flagChanges;
        }

        b.n = d.numSections;
        for (int i = 0; i < maxSections; ++i)
        {
            out[i] = d.params[(size_t) i];
            b.hasSvf[i] = i < d.numSections && d.params[(size_t) i].hasSvf;
            b.hasOp[i]  = i < d.numSections && d.params[(size_t) i].hasOnePole;

            if (trackRanges && i < d.numSections)
            {
                const auto& p = d.params[(size_t) i];
                if (p.hasSvf)
                {
                    gMin = std::min (gMin, p.g);  gMax = std::max (gMax, p.g);
                    kMin = std::min (kMin, p.k);  kMax = std::max (kMax, p.k);
                    mMax = std::max ({ mMax, std::fabs (p.mL), std::fabs (p.mB), std::fabs (p.mH) });
                }
                if (p.hasOnePole)
                {
                    GMin = std::min (GMin, p.G);  GMax = std::max (GMax, p.G);
                    mMax = std::max ({ mMax, std::fabs (p.cL), std::fabs (p.cH) });
                }
            }
        }
    }

    void snap (RBand& b, const BandSettings& s, bool clear)
    {
        b.tgt = s;
        b.lf = std::log (s.freqHz);  b.gdb = s.gainDbEff;  b.lq = std::log (s.q);
        b.stepsLeft = 0;  b.retarget = false;  b.seg = false;
        b.haveDesign = false;
        designInto (b, s, b.P);
        b.haveDesign = true;
        for (int i = 0; i < maxSections; ++i) { b.T[i] = b.P[i];  b.rt[i] = toRt (b.P[i]); }
        if (clear) b.clearState();
    }

    // Called at a host block start.
    void setTarget (int index, const BandSettings& s, bool active = true)
    {
        RBand& b = bands[(size_t) index];

        if (! active) { if (b.active) { b.active = false; b.haveDesign = false; b.clearState(); } return; }

        const bool structural = ! b.active || ! b.haveDesign || s.type != b.tgt.type || s.slopeIndex != b.tgt.slopeIndex;
        b.active = true;

        if (structural)     { snap (b, s, true);  return; }

        if (s.freqHz == b.tgt.freqHz && s.gainDbEff == b.tgt.gainDbEff && s.q == b.tgt.q)
            return;

        if (! smooth)       { snap (b, s, false); return; }     // hard control: Phase 2.2 behaviour

        b.tgt = s;
        b.retarget = true;
    }

    void tick()
    {
        for (auto& b : bands)
        {
            if (! b.active) continue;

            if (b.seg)
            {
                for (int i = 0; i < b.n; ++i) { b.P[i] = b.T[i];  b.rt[i] = toRt (b.P[i]); }
                b.seg = false;
            }

            if (b.retarget)
            {
                b.retarget = false;
                b.stepsLeft = rampTicks;
                b.lfInc = (std::log (b.tgt.freqHz) - b.lf) / rampTicks;
                b.gInc  = (b.tgt.gainDbEff - b.gdb) / rampTicks;
                b.lqInc = (std::log (b.tgt.q) - b.lq) / rampTicks;
            }

            if (b.stepsLeft > 0)
            {
                BandSettings s = b.tgt;

                if (--b.stepsLeft == 0)
                {
                    b.lf = std::log (b.tgt.freqHz);  b.gdb = b.tgt.gainDbEff;  b.lq = std::log (b.tgt.q);
                    // s is the target itself: the settled design equals a static design, bit for bit
                }
                else
                {
                    b.lf += b.lfInc;  b.gdb += b.gInc;  b.lq += b.lqInc;
                    s.freqHz = std::exp (b.lf);  s.gainDbEff = b.gdb;  s.q = std::exp (b.lq);
                }

                designInto (b, s, b.T);

                if (interp == interpHold)
                {
                    for (int i = 0; i < b.n; ++i) { b.P[i] = b.T[i];  b.rt[i] = toRt (b.P[i]); }
                }
                else
                {
                    const double r = 1.0 / K;
                    for (int i = 0; i < b.n; ++i)
                    {
                        const auto& p = b.P[i];  const auto& t = b.T[i];  auto& d = b.inc[i];
                        d.g = (t.g - p.g) * r;  d.k = (t.k - p.k) * r;
                        d.mL = (t.mL - p.mL) * r;  d.mB = (t.mB - p.mB) * r;  d.mH = (t.mH - p.mH) * r;
                        d.G = (t.G - p.G) * r;  d.cL = (t.cL - p.cL) * r;  d.cH = (t.cH - p.cH) * r;

                        b.rtT[i] = toRt (t);
                        const Rt& a = b.rt[i];  const Rt& z = b.rtT[i];  Rt& q = b.rtInc[i];
                        q.a1 = (z.a1 - a.a1) * r;  q.a2 = (z.a2 - a.a2) * r;  q.a3 = (z.a3 - a.a3) * r;
                        q.k = (z.k - a.k) * r;  q.mL = (z.mL - a.mL) * r;  q.mB = (z.mB - a.mB) * r;  q.mH = (z.mH - a.mH) * r;
                        q.Gc = (z.Gc - a.Gc) * r;  q.cL = (z.cL - a.cL) * r;  q.cH = (z.cH - a.cH) * r;

                        if (interp == interpGeo)
                        {
                            d.g = std::exp (std::log (t.g / p.g) * r);      // per-sample ratios
                            d.k = std::exp (std::log (t.k / p.k) * r);
                        }
                    }
                    b.seg = true;
                }
            }
        }
    }

    // m samples of one band, both lanes in one sample loop.
    void runBand (RBand& b, double* L, double* R, int m)
    {
        const int nl = lanes;
        double* io[2] = { L, R };

        if (! b.seg)
        {
            for (int l = 0; l < nl; ++l)
            {
                double* x = io[l];
                for (int j = 0; j < m; ++j)
                {
                    double v = x[j];
                    for (int q = 0; q < b.n; ++q)
                    {
                        const Rt& c = b.rt[q];
                        if (b.hasSvf[q]) v = svfTick (c.a1, c.a2, c.a3, c.k, c.mL, c.mB, c.mH, b.ic1[q][l], b.ic2[q][l], v);
                        if (b.hasOp[q])  v = opTick (c.Gc, c.cL, c.cH, b.s[q][l], v);
                    }
                    x[j] = v;
                }
            }
            return;
        }

        if (interp == interpCoef)
        {
            for (int j = 0; j < m; ++j)
            {
                double v0 = L[j], v1 = nl > 1 ? R[j] : 0.0;
                for (int q = 0; q < b.n; ++q)
                {
                    Rt& c = b.rt[q];  const Rt& d = b.rtInc[q];
                    c.a1 += d.a1;  c.a2 += d.a2;  c.a3 += d.a3;  c.k += d.k;  c.mL += d.mL;  c.mB += d.mB;  c.mH += d.mH;
                    c.Gc += d.Gc;  c.cL += d.cL;  c.cH += d.cH;
                    if (b.hasSvf[q])
                    {
                        v0 = svfTick (c.a1, c.a2, c.a3, c.k, c.mL, c.mB, c.mH, b.ic1[q][0], b.ic2[q][0], v0);
                        if (nl > 1) v1 = svfTick (c.a1, c.a2, c.a3, c.k, c.mL, c.mB, c.mH, b.ic1[q][1], b.ic2[q][1], v1);
                    }
                    if (b.hasOp[q])
                    {
                        v0 = opTick (c.Gc, c.cL, c.cH, b.s[q][0], v0);
                        if (nl > 1) v1 = opTick (c.Gc, c.cL, c.cH, b.s[q][1], v1);
                    }
                }
                L[j] = v0;  if (nl > 1) R[j] = v1;
            }
            return;
        }

        const bool gcLin = interp == interpGc || interp == interpGeo;
        const bool geo = interp == interpGeo;

        for (int j = 0; j < m; ++j)
        {
            double v0 = L[j], v1 = nl > 1 ? R[j] : 0.0;
            for (int q = 0; q < b.n; ++q)
            {
                SectionParams& p = b.P[q];  const SectionParams& d = b.inc[q];
                if (b.hasSvf[q])
                {
                    if (geo) { p.g *= d.g;  p.k *= d.k; } else { p.g += d.g;  p.k += d.k; }
                    p.mL += d.mL;  p.mB += d.mB;  p.mH += d.mH;
                    const double a1 = 1.0 / (1.0 + p.g * (p.g + p.k));
                    const double a2 = p.g * a1, a3 = p.g * a2;
                    v0 = svfTick (a1, a2, a3, p.k, p.mL, p.mB, p.mH, b.ic1[q][0], b.ic2[q][0], v0);
                    if (nl > 1) v1 = svfTick (a1, a2, a3, p.k, p.mL, p.mB, p.mH, b.ic1[q][1], b.ic2[q][1], v1);
                }
                if (b.hasOp[q])
                {
                    double Gc;
                    p.cL += d.cL;  p.cH += d.cH;
                    if (gcLin) { Rt& c = b.rt[q];  c.Gc += b.rtInc[q].Gc;  Gc = c.Gc; }
                    else       { p.G += d.G;  Gc = p.G / (1.0 + p.G); }
                    v0 = opTick (Gc, p.cL, p.cH, b.s[q][0], v0);
                    if (nl > 1) v1 = opTick (Gc, p.cL, p.cH, b.s[q][1], v1);
                }
            }
            L[j] = v0;  if (nl > 1) R[j] = v1;
        }
    }

    void process (double* L, double* R, int n)
    {
        int i = 0;
        while (i < n)
        {
            if (phase == 0) tick();
            const int m = std::min (n - i, K - phase);
            for (auto& b : bands)
                if (b.active)
                    runBand (b, L + i, R != nullptr ? R + i : nullptr, m);
            phase = (phase + m) % K;
            i += m;
        }
    }
};

//==============================================================================
static uint64_t rngState = 0x9E3779B97F4A7C15ull;
static double urand()
{
    rngState ^= rngState << 13;  rngState ^= rngState >> 7;  rngState ^= rngState << 17;
    return (double) (rngState >> 11) * (1.0 / 9007199254740992.0);
}

static const char* typeName (Type t)
{
    static const char* n[] = { "Bell", "LowShelf", "HighShelf", "LowCut", "HighCut", "Notch", "BandPass", "Tilt" };
    return n[(int) t];
}

using Clock = std::chrono::steady_clock;
static double secs (Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double> (b - a).count(); }

static void setFlushToZero()
{
   #if defined (__aarch64__)
    uint64_t fpcr;
    __asm__ __volatile__ ("mrs %0, fpcr" : "=r" (fpcr));
    fpcr |= (1ull << 24);
    __asm__ __volatile__ ("msr fpcr, %0" : : "r" (fpcr));
   #endif
}

//==============================================================================
// Local kappa (gate G3): max over 5 ms frames of max |d2 y| / frame RMS,
// frames below -80 dBFS skipped.
//==============================================================================
static double localKappa (const std::vector<double>& y, double fs, size_t from, size_t to, size_t* where = nullptr)
{
    const size_t frame = (size_t) std::lround (0.005 * fs);
    double worst = 0.0;

    for (size_t a = std::max<size_t> (from, 1); a + frame < std::min (to, y.size() - 1); a += frame)
    {
        double e = 0.0, d2 = 0.0;
        for (size_t i = a; i < a + frame; ++i)
        {
            e += y[i] * y[i];
            d2 = std::max (d2, std::fabs (y[i + 1] - 2.0 * y[i] + y[i - 1]));
        }
        const double rms = std::sqrt (e / (double) frame);
        if (rms < 1.0e-4) continue;
        if (d2 / rms > worst) { worst = d2 / rms;  if (where) *where = a; }
    }
    return worst;
}

//==============================================================================
// g3: sweep rows
//==============================================================================
struct SweepCase
{
    const char* name;
    Type type;  int slope;
    // value at u in [0, 1] (0 = A, 1 = B)
    double fA, fB, gA, gB, qA, qB;
    double sweepSec;
};

static BandSettings at (const SweepCase& c, double u)
{
    BandSettings s;
    s.type = c.type;  s.slopeIndex = c.slope;
    s.freqHz = std::exp (std::log (c.fA) + u * (std::log (c.fB) - std::log (c.fA)));
    s.gainDbEff = c.gA + u * (c.gB - c.gA);
    s.q = std::exp (std::log (c.qA) + u * (std::log (c.qB) - std::log (c.qA)));
    return s;
}

struct Variant { int K; double rampMs; int interp; bool smooth; };

// motion: 0 = sweep (A, to B, hold, back to A, hold); 1 = held at A; 2 = held at B
// pattern 0: lead 0.5 s | A->B in T | hold 0.4 s | B->A in T | hold 0.4 s
// pattern 1: lead 0.5 s | A->B->A in T (there and back, no hold) | hold 0.5 s, repeated twice
static std::vector<double> renderSweep (const SweepCase& c, double tone, int block, const Variant& v, int motion,
                                        int pattern, double fs, size_t* evalFrom)
{
    Engine e;
    e.prepare (fs, 1, v.K, v.rampMs);
    e.interp = v.interp;  e.smooth = v.smooth;

    const double T = c.sweepSec;
    const double lead = 0.5;
    const double total = pattern == 0 ? lead + T + 0.4 + T + 0.4 : lead + 2.0 * (T + 0.5);
    const size_t n = (size_t) (total * fs);
    *evalFrom = (size_t) (0.3 * fs);

    auto uAt = [&] (double t) -> double
    {
        if (motion == 1) return 0.0;
        if (motion == 2) return 1.0;
        if (t < lead) return 0.0;
        t -= lead;
        if (pattern == 0)
        {
            if (t < T) return t / T;
            t -= T;
            if (t < 0.4) return 1.0;
            t -= 0.4;
            if (t < T) return 1.0 - t / T;
            return 0.0;
        }
        t = std::fmod (t, T + 0.5);
        if (t < 0.5 * T) return t / (0.5 * T);
        if (t < T) return 2.0 - t / (0.5 * T);
        return 0.0;
    };

    std::vector<double> y (n);
    for (size_t i = 0; i < n; ++i)
        y[i] = 0.5 * std::sin (2.0 * pi * tone * (double) i / fs);

    for (size_t pos = 0; pos < n; pos += (size_t) block)
    {
        const int m = (int) std::min<size_t> ((size_t) block, n - pos);
        e.setTarget (0, at (c, uAt ((double) pos / fs)));
        e.process (y.data() + pos, nullptr, m);
    }
    return y;
}

static void modeG3 (bool variants)
{
    const double fs = 48000.0;

    for (double tone : { 50.0, 1000.0 })
    {
        const SweepCase cases[] = {
            { "Bell +18 Q1, freq 10 Hz - 30 kHz, 50 ms",        Type::bell, 0,      10, 30000, 18, 18, 1, 1, 0.050 },
            { "Bell at tone Q1, gain -30 - +30, 20 ms",         Type::bell, 0,      tone, tone, -30, 30, 1, 1, 0.020 },
            { "Bell +18 at 2x tone, Q 0.025 - 40, 20 ms",       Type::bell, 0,      2 * tone, 2 * tone, 18, 18, 0.025, 40, 0.020 },
            { "LowCut 96 Q1, freq 10 Hz - 30 kHz, 50 ms",       Type::lowCut, 7,    10, 30000, 0, 0, 1, 1, 0.050 },
            { "LowCut 96 at 30 Hz, Q 0.025 - 40, 20 ms",        Type::lowCut, 7,    30, 30, 0, 0, 0.025, 40, 0.020 },
            { "HighShelf 12 +18, freq 10 Hz - 30 kHz, 50 ms",   Type::highShelf, 1, 10, 30000, 18, 18, 1, 1, 0.050 },
            { "HighShelf 12 at tone/2, gain -30 - +30, 20 ms",  Type::highShelf, 1, tone / 2, tone / 2, -30, 30, 1, 1, 0.020 },
            { "HighShelf 12 +18 at tone, Q 0.025 - 40, 20 ms",  Type::highShelf, 1, tone, tone, 18, 18, 0.025, 40, 0.020 },
            { "HighShelf 96 +18, freq 10 Hz - 30 kHz, 50 ms",   Type::highShelf, 7, 10, 30000, 18, 18, 1, 1, 0.050 },
        };

        if (! variants)
        {
            std::printf ("\n== tone %.0f Hz, fs 48 k. kappa of a held sine = %.3e\n", tone,
                         std::sqrt (2.0) * std::pow (2.0 * pi * tone / fs, 2.0));
            std::printf ("%-48s %5s %3s | %9s %9s | %8s %8s %8s %8s | %8s %8s\n", "case", "block", "pat",
                         "held A", "held B", "spec/h", "ideal/h", "hold/h", "hard/h", "spec/id", "hard/id");
        }
        else
            std::printf ("\n== tone %.0f Hz: ratio to the same-motion ideal (K = 1), block 32 / 512\n", tone);

        for (const auto& c : cases)
        {
            for (int pattern : { 0, 1 })
            {
                if (pattern == 1 && c.fA == c.fB) continue;   // there-and-back only for frequency

                if (variants)
                {
                    struct VN { const char* name; Variant v; };
                    const VN vs[] = {
                        { "spec K16 20ms",     { 16, 20, interpSpec, true } },
                        { "K4",                { 4, 20, interpSpec, true } },
                        { "K64",               { 64, 20, interpSpec, true } },
                        { "coef-interp K16",   { 16, 20, interpCoef, true } },
                        { "Gc-interp K16",     { 16, 20, interpGc, true } },
                        { "geo K16",           { 16, 20, interpGeo, true } },
                        { "K8",                { 8, 20, interpSpec, true } },
                        { "geo K32",           { 32, 20, interpGeo, true } },
                        { "hold K16",          { 16, 20, interpHold, true } },
                    };
                    std::printf ("%-48s p%d |", c.name, pattern);
                    for (const auto& vn : vs)
                    {
                        double worst = 0;
                        for (int block : { 32, 512 })
                        {
                            size_t from;
                            Variant idealV { 1, 20, interpHold, true };
                            const auto yi = renderSweep (c, tone, block, idealV, 0, pattern, fs, &from);
                            const auto ys = renderSweep (c, tone, block, vn.v, 0, pattern, fs, &from);
                            const double ki = localKappa (yi, fs, from, yi.size());
                            const double ks = localKappa (ys, fs, from, ys.size());
                            worst = std::max (worst, ki > 0 ? ks / ki : 0.0);
                        }
                        std::printf (" %s %.2f |", vn.name, worst);
                    }
                    std::printf ("\n");
                    continue;
                }

                for (int block : { 32, 512 })
                {
                    size_t from;
                    const Variant spec { 16, 20, interpSpec, true }, ideal { 1, 20, interpHold, true },
                                  hold { 16, 20, interpHold, true }, hard { 16, 20, interpSpec, false };

                    const auto yA = renderSweep (c, tone, block, spec, 1, pattern, fs, &from);
                    const auto yB = renderSweep (c, tone, block, spec, 2, pattern, fs, &from);
                    const auto yS = renderSweep (c, tone, block, spec, 0, pattern, fs, &from);
                    const auto yI = renderSweep (c, tone, block, ideal, 0, pattern, fs, &from);
                    const auto yH = renderSweep (c, tone, block, hold, 0, pattern, fs, &from);
                    const auto yX = renderSweep (c, tone, block, hard, 0, pattern, fs, &from);

                    const double kA = localKappa (yA, fs, from, yA.size());
                    const double kB = localKappa (yB, fs, from, yB.size());
                    const double kS = localKappa (yS, fs, from, yS.size());
                    const double kI = localKappa (yI, fs, from, yI.size());
                    const double kH = localKappa (yH, fs, from, yH.size());
                    const double kX = localKappa (yX, fs, from, yX.size());
                    const double ref = std::max (kA, kB);

                    std::printf ("%-48s %5d  p%d | %9.3e %9.3e | %8.2f %8.2f %8.2f %8.2f | %8.3f %8.2f\n", c.name, block, pattern,
                                 kA, kB, kS / ref, kI / ref, kH / ref, kX / ref, kS / kI, kX / kI);
                }
            }
        }
    }
    std::printf ("\nflag changes during continuous moves: %ld\n", flagChanges);
}

//==============================================================================
// lyap: the state matrix of the SVF is a contraction in the Euclidean norm
//==============================================================================
static double norm2of (double a, double b, double c, double d)   // largest singular value of [[a b][c d]]
{
    const double p = a * a + b * b + c * c + d * d;
    const double det = a * d - b * c;
    const double disc = std::sqrt (std::max (0.0, p * p - 4.0 * det * det));
    return std::sqrt (0.5 * (p + disc));
}

static void modeLyap()
{
    // A(g, k) = 1/D [[1 - gk - g^2, -2g], [2g, 1 + gk - g^2]], D = 1 + gk + g^2
    auto A = [] (double g, double k, double* m)
    {
        const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
        m[0] = 2.0 * a1 - 1.0;  m[1] = -2.0 * a2;  m[2] = 2.0 * a2;  m[3] = 1.0 - 2.0 * a3;
    };

    double worst1 = 0.0, worst2 = 0.0, worst2rel = 0.0;
    const int N = 60;
    std::vector<double> gs, ks;
    for (int i = 0; i < N; ++i) gs.push_back (std::pow (10.0, -4.0 + 8.0 * i / (N - 1)));    // 1e-4 .. 1e4
    for (int i = 0; i < N; ++i) ks.push_back (std::pow (10.0, -4.0 + 6.0 * i / (N - 1)));    // 1e-4 .. 1e2

    for (double g1 : gs) for (double k1 : ks)
    {
        double m1[4];  A (g1, k1, m1);
        worst1 = std::max (worst1, norm2of (m1[0], m1[1], m1[2], m1[3]));

        for (double g2 : gs) for (double k2 : ks)
        {
            double m2[4];  A (g2, k2, m2);
            const double p0 = m2[0] * m1[0] + m2[1] * m1[2], p1 = m2[0] * m1[1] + m2[1] * m1[3];
            const double p2 = m2[2] * m1[0] + m2[3] * m1[2], p3 = m2[2] * m1[1] + m2[3] * m1[3];
            const double nn = norm2of (p0, p1, p2, p3);
            if (nn > worst2) { worst2 = nn; }
            (void) worst2rel;
        }
    }
    std::printf ("one step:  max ||A||_2 over g 1e-4..1e4, k 1e-4..1e2 (%d x %d)      = 1 %+.3e\n", N, N, worst1 - 1.0);
    std::printf ("two steps: max ||A(g2,k2) A(g1,k1)||_2 over the same grid squared = 1 %+.3e\n", worst2 - 1.0);

    // Adversarial products: random and greedy sequences, growth of the state norm with zero input.
    double worstGrowth = 0.0;
    for (int trial = 0; trial < 2000; ++trial)
    {
        double s1 = urand() * 2 - 1, s2 = urand() * 2 - 1;
        const double n0 = std::hypot (s1, s2);
        double peak = n0;
        for (int i = 0; i < 2000; ++i)
        {
            // greedy: of 8 random parameter sets take the one that leaves the largest norm
            double best = -1, b1 = 0, b2 = 0;
            for (int c = 0; c < 8; ++c)
            {
                const double g = std::pow (10.0, -4.0 + 8.0 * urand()), k = std::pow (10.0, -4.0 + 6.0 * urand());
                double m[4];  A (g, k, m);
                const double t1 = m[0] * s1 + m[1] * s2, t2 = m[2] * s1 + m[3] * s2;
                const double nn = std::hypot (t1, t2);
                if (nn > best) { best = nn; b1 = t1; b2 = t2; }
            }
            s1 = b1;  s2 = b2;
            peak = std::max (peak, best);
        }
        worstGrowth = std::max (worstGrowth, peak / n0);
    }
    std::printf ("greedy adversary, zero input, 2000 x 2000 steps: max state norm / initial = 1 %+.3e\n", worstGrowth - 1.0);

    // Parameter ranges the designer actually produces.
    trackRanges = true;
    for (double fs : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        gMin = kMin = GMin = 1e300;  gMax = kMax = GMax = mMax = 0;
        Engine e;  e.prepare (fs, 1);
        RBand b;
        for (int i = 0; i < 400000; ++i)
        {
            BandSettings s;
            s.type = (Type) (int) (urand() * 8);
            s.slopeIndex = (int) (urand() * 8);
            const double u = urand();
            s.freqHz = u < 0.05 ? 10.0 : (u > 0.95 ? 30000.0 : std::exp (std::log (10.0) + urand() * std::log (3000.0)));
            const double w = urand();
            s.gainDbEff = w < 0.05 ? -60.0 : (w > 0.95 ? 60.0 : -60.0 + 120.0 * urand());
            const double z = urand();
            s.q = z < 0.05 ? 0.025 : (z > 0.95 ? 40.0 : std::exp (std::log (0.025) + urand() * std::log (1600.0)));
            b.haveDesign = false;
            e.designInto (b, s, b.P);
        }
        std::printf ("fs %6.0f: g %.3e .. %.3e   k %.3e .. %.3e   G %.3e .. %.3e   max |m| %.3e\n",
                     fs, gMin, gMax, kMin, kMax, GMin, GMax, mMax);
    }
    trackRanges = false;
}

//==============================================================================
// g5: random stress
//==============================================================================
static double bandPeakDb (const Designer& des, const BandSettings& s, double fs)
{
    BandDesign d;
    des.design (s, d);
    double worst = 0.0;
    const double top = 0.4999 * fs;
    for (int i = 0; i < 48; ++i)
    {
        const double f = 5.0 * std::pow (top / 5.0, i / 47.0);
        worst = std::max (worst, bandMagDb (d, 2.0 * pi * f / fs));
    }
    worst = std::max (worst, bandMagDb (d, 2.0 * pi * std::min (s.freqHz, top) / fs));
    return worst;
}

static BandSettings randomSettings (bool keepStructure, const BandSettings& prev, double endsP)
{
    BandSettings s = prev;
    if (! keepStructure)
    {
        s.type = (Type) (int) (urand() * 8);
        s.slopeIndex = (int) (urand() * 8);
    }
    const double u = urand();
    s.freqHz = u < endsP ? 10.0 : (u > 1.0 - endsP ? 30000.0 : std::exp (std::log (10.0) + urand() * std::log (3000.0)));
    const double w = urand();
    const double gain = w < endsP ? -30.0 : (w > 1.0 - endsP ? 30.0 : -30.0 + 60.0 * urand());
    s.gainDbEff = gain;     // scaled by the caller
    const double z = urand();
    s.q = z < endsP ? 0.025 : (z > 1.0 - endsP ? 40.0 : std::exp (std::log (0.025) + urand() * std::log (1600.0)));
    return s;
}

static void modeG5 (double seconds, double stackLimitDb)
{
    setFlushToZero();
    std::printf ("G5-style stress, double engine, 24 bands stereo, %.0f s per rate; block sizes 4096, 512, 64, 7, 1 cycling every 2 s\n", seconds);
    std::printf ("gain = raw * scale / 100, both redrawn each block (ends 10 %% each); types and slopes redrawn every 50th block%s\n",
                 stackLimitDb > 0 ? "; draws limited by stack" : "");
    std::printf ("%8s | %9s %12s %12s %14s %10s | %8s %8s\n", "fs", "nonfinite", "peak dB re in", "> float max",
                 "peak - stack dB", "x realtime", "flagchg", "designs/s");

    const int blocks[] = { 4096, 512, 64, 7, 1 };

    for (double fs : { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
    {
        rngState = 0x9E3779B97F4A7C15ull ^ (uint64_t) fs;
        Engine e;
        e.prepare (fs, 24);
        e.lanes = 2;
        flagChanges = 0;  designCalls = 0;

        std::vector<BandSettings> cur (24);
        std::vector<bool> act (24, true);
        double runStack = 0.0;      // running max of the sum over bands of the target's static peak gain (dB, floor 0 each)
        double maxStack = 0.0;
        for (auto& s : cur) { s.type = Type::bell; s.slopeIndex = 1; s.freqHz = 1000; s.gainDbEff = 0; s.q = 1; }

        const size_t total = (size_t) (seconds * fs);
        std::vector<double> L (4096), R (4096);
        long nonFinite = 0, overFloat = 0;
        double peak = 0.0, inPeak = 0.0, worstOverStack = -1e9;
        size_t pos = 0;  long blockCount = 0;
        const auto t0 = Clock::now();

        while (pos < total)
        {
            const int bs = blocks[(size_t) ((double) pos / fs / 2.0) % 5];
            const int m = (int) std::min<size_t> ((size_t) bs, total - pos);
            const bool silent = std::fmod ((double) pos / fs, 5.0) > 4.0;    // one second in five is silence

            const bool restructure = (blockCount % 50) == 0;
            const double su = urand();
            const double scale = su < 0.1 ? 0.0 : (su > 0.9 ? 200.0 : 200.0 * urand());

            // The stack is only evaluated on larger blocks (cost); smaller blocks keep the running maxima.
            const bool evalStack = bs >= 512;
            double stackNow = 0.0;

            for (int b = 0; b < 24; ++b)
            {
                BandSettings s = randomSettings (! restructure, cur[(size_t) b], 0.1);
                const double raw = s.gainDbEff;
                s.gainDbEff = std::max (-60.0, std::min (60.0, raw * scale / 100.0));
                if (! usesSlope (s.type)) s.slopeIndex = 0;
                if (restructure) act[(size_t) b] = urand() > 0.15;
                cur[(size_t) b] = s;
                cur[(size_t) b].gainDbEff = raw;      // keep the raw gain for the next draw's structure copy
                e.setTarget (b, s, act[(size_t) b]);

                if (evalStack && act[(size_t) b])
                    stackNow += bandPeakDb (e.des, s, fs);
            }
            runStack = std::max (runStack, stackNow);
            maxStack = runStack;
            const double stack = runStack;

            for (int i = 0; i < m; ++i)
            {
                L[(size_t) i] = silent ? 0.0 : (urand() * 2.0 - 1.0) * 0.25;
                R[(size_t) i] = silent ? 0.0 : (urand() * 2.0 - 1.0) * 0.25;
                inPeak = std::max ({ inPeak, std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]) });
            }

            e.process (L.data(), R.data(), m);

            double blockPeak = 0.0;
            for (int i = 0; i < m; ++i)
            {
                const double a = std::fabs (L[(size_t) i]), c = std::fabs (R[(size_t) i]);
                if (! std::isfinite (a) || ! std::isfinite (c)) { ++nonFinite; continue; }
                blockPeak = std::max ({ blockPeak, a, c });
                if (a > 3.4e38 || c > 3.4e38) ++overFloat;
            }
            peak = std::max (peak, blockPeak);
            if (blockPeak > 0.0)
                worstOverStack = std::max (worstOverStack, 20.0 * std::log10 (blockPeak / 0.25) - stack);

            pos += (size_t) m;  ++blockCount;
        }

        const double wall = secs (t0, Clock::now());
        std::printf ("%8.0f | %9ld %12.1f %12ld %14.1f %10.1f | %8ld %8.0f | max stack %.1f dB\n", fs, nonFinite,
                     20.0 * std::log10 (peak / inPeak), overFloat, worstOverStack, seconds / wall, flagChanges,
                     (double) designCalls / seconds, maxStack);
    }
}

//==============================================================================
// g7: noise floor and ring-down at 192 kHz, bell 10 Hz Q 40 +12 dB
//==============================================================================
static void modeG7()
{
    setFlushToZero();
    const double fs = 192000.0;

    for (double gain : { 12.0, -12.0, 30.0 })
    {
        Engine e;  e.prepare (fs, 1);
        BandSettings s { Type::bell, 10.0, gain, 40.0, 0 };
        e.setTarget (0, s);

        const size_t n = (size_t) (7.0 * fs);
        std::vector<double> y (n);
        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / fs;
            const double fade = t < 2.0 ? 0.5 - 0.5 * std::cos (pi * t / 2.0) : 1.0;
            y[i] = 0.5 * fade * std::sin (2.0 * pi * 1000.0 * t);
        }
        e.process (y.data(), nullptr, (int) n);

        auto residual = [&] (bool asFloat)
        {
            const size_t a = (size_t) (4.0 * fs);
            long double ss = 0, cc = 0, sc = 0, ys = 0, yc = 0;
            for (size_t i = a; i < n; ++i)
            {
                const double t = (double) i / fs;
                const double sn = std::sin (2.0 * pi * 1000.0 * t), cs = std::cos (2.0 * pi * 1000.0 * t);
                const double v = asFloat ? (double) (float) y[i] : y[i];
                ss += sn * sn;  cc += cs * cs;  sc += sn * cs;  ys += v * sn;  yc += v * cs;
            }
            const long double det = ss * cc - sc * sc;
            const long double A = (ys * cc - yc * sc) / det, B = (yc * ss - ys * sc) / det;
            long double r2 = 0;
            for (size_t i = a; i < n; ++i)
            {
                const double t = (double) i / fs;
                const double v = asFloat ? (double) (float) y[i] : y[i];
                const long double r = v - A * std::sin (2.0 * pi * 1000.0 * t) - B * std::cos (2.0 * pi * 1000.0 * t);
                r2 += r * r;
            }
            return 10.0 * std::log10 ((double) (r2 / (long double) (n - a)) + 1e-300);
        };

        std::printf ("Bell 10 Hz Q 40 %+.0f dB, 192 k: residual after removing 1 kHz, last 3 s: double %.1f dBFS RMS, float %.1f dBFS RMS\n",
                     gain, residual (false), residual (true));

        // Silence after excitation: the sine stops abruptly; 1 s windows.
        std::vector<double> z ((size_t) fs);
        double prev = 1e9;  bool monotone = true;  double at60 = 0, first = 0, at10 = 0;
        const auto t0 = Clock::now();
        for (int sec = 1; sec <= 60; ++sec)
        {
            std::fill (z.begin(), z.end(), 0.0);
            e.process (z.data(), nullptr, (int) z.size());
            double e2 = 0;
            for (double v : z) e2 += v * v;
            const double db = 10.0 * std::log10 (e2 / (double) z.size() + 1e-320 * 1e-320);
            if (db > prev + 1e-9) monotone = false;
            prev = db;
            if (sec == 1) first = db;
            if (sec == 10) at10 = db;
            if (sec == 60) at60 = db;
        }
        const double tSilence = secs (t0, Clock::now());
        std::printf ("   silence after an abrupt stop: 1st s %.1f dBFS, 10th s %.1f, 60th s %.1f (%.2f dB/s); monotone %s; 60 s in %.2f s\n",
                     first, at10, at60, (at10 - at60) / 50.0, monotone ? "yes" : "NO", tSilence);
    }

    // Stop with a 100 ms fade-out instead of abruptly, +12 dB.
    {
        Engine e;  e.prepare (fs, 1);
        e.setTarget (0, BandSettings { Type::bell, 10.0, 12.0, 40.0, 0 });
        const size_t n = (size_t) (7.0 * fs);
        std::vector<double> y (n);
        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / fs;
            double fade = t < 2.0 ? 0.5 - 0.5 * std::cos (pi * t / 2.0) : 1.0;
            if (t > 5.0) fade = 0.5 + 0.5 * std::cos (pi * (t - 5.0) / 2.0);
            y[i] = 0.5 * fade * std::sin (2.0 * pi * 1000.0 * t);
        }
        e.process (y.data(), nullptr, (int) n);
        std::vector<double> z ((size_t) fs);
        double first = 0, at60 = 0;
        for (int sec = 1; sec <= 60; ++sec)
        {
            std::fill (z.begin(), z.end(), 0.0);
            e.process (z.data(), nullptr, (int) z.size());
            double e2 = 0;  for (double v : z) e2 += v * v;
            const double db = 10.0 * std::log10 (e2 / (double) z.size() + 1e-320 * 1e-320);
            if (sec == 1) first = db;
            if (sec == 60) at60 = db;
        }
        std::printf ("   +12 dB, sine faded out over 2 s (raised cosine): 1st s %.1f dBFS, 60th s %.1f\n", first, at60);
    }

    // Timing: silence against signal, flush-to-zero on (as in the plugin).
    {
        Engine e;  e.prepare (fs, 24);  e.lanes = 2;
        for (int b = 0; b < 24; ++b) e.setTarget (b, BandSettings { Type::bell, 10.0 + b, 12.0, 40.0, 0 });
        std::vector<double> L (512), R (512);
        auto run = [&] (bool signal, double sec)
        {
            const auto t0 = Clock::now();
            for (size_t pos = 0; pos < (size_t) (sec * fs); pos += 512)
            {
                for (int i = 0; i < 512; ++i) { L[(size_t) i] = signal ? (urand() - 0.5) * 0.5 : 0.0;  R[(size_t) i] = L[(size_t) i]; }
                e.process (L.data(), R.data(), 512);
            }
            return secs (t0, Clock::now()) / sec;
        };
        // the noise generator is part of both timings only through the fill: time the fill alone and subtract
        const double withSig = run (true, 20.0);
        const double silent = run (false, 60.0);
        const double silentLate = run (false, 20.0);
        std::printf ("   24 bells 10-33 Hz Q 40, 192 k stereo: cost with noise %.4f (includes the noise generator), silence %.4f, silence after 60 s %.4f of real time\n",
                     withSig, silent, silentLate);
    }
}

//==============================================================================
// blk: parameter events on multiples of 2048 at block sizes 32, 64, 512, 2048
//==============================================================================
static void modeBlk()
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (int align : { 2048, 1000 })
        {
            std::vector<std::vector<double>> outs;
            const int sizes[] = { 32, 64, 512, 2048, 1, 7, 4096 };
            for (int bs : sizes)
            {
                rngState = 12345;
                Engine e;  e.prepare (fs, 4);  e.lanes = 2;
                const size_t n = 2048 * 96;
                std::vector<double> L (n), R (n);
                for (size_t i = 0; i < n; ++i) { L[i] = (urand() - 0.5) * 0.5;  R[i] = (urand() - 0.5) * 0.5; }
                uint64_t evRng = 777;
                auto ev = [&] { evRng ^= evRng << 13; evRng ^= evRng >> 7; evRng ^= evRng << 17; return (double) (evRng >> 11) / 9007199254740992.0; };
                BandSettings cur[4] = { { Type::bell, 200, 6, 1, 0 }, { Type::highShelf, 4000, -9, 1, 7 },
                                        { Type::lowCut, 60, 0, 2, 5 }, { Type::tilt, 900, 4, 1, 3 } };
                size_t nextEvent = 0;
                for (size_t pos = 0; pos < n; pos += (size_t) bs)
                {
                    // An event is visible to the block that starts at or after it.
                    while (nextEvent <= pos)
                    {
                        for (auto& s : cur)
                        {
                            s.freqHz = std::exp (std::log (10.0) + ev() * std::log (3000.0));
                            s.gainDbEff = -30.0 + 60.0 * ev();
                            s.q = std::exp (std::log (0.025) + ev() * std::log (1600.0));
                        }
                        nextEvent += (size_t) align;
                    }
                    for (int b = 0; b < 4; ++b) e.setTarget (b, cur[b]);
                    const int m = (int) std::min<size_t> ((size_t) bs, n - pos);
                    e.process (L.data() + pos, R.data() + pos, m);
                }
                L.insert (L.end(), R.begin(), R.end());
                outs.push_back (L);
            }
            std::printf ("fs %6.0f, events every %4d samples: vs block 32:", fs, align);
            for (size_t i = 1; i < outs.size(); ++i)
            {
                size_t diff = 0;
                for (size_t j = 0; j < outs[0].size(); ++j)
                    if (std::memcmp (&outs[0][j], &outs[i][j], sizeof (double)) != 0) ++diff;
                std::printf ("  %d: %zu", sizes[i], diff);
            }
            std::printf ("  (samples differing)\n");
        }
    }
}

//==============================================================================
// cpu: sweeping cost
//==============================================================================
static void modeCpu()
{
    setFlushToZero();
    const double fs = 48000.0;
    struct Case { const char* name; Type t; int slope; double g; };
    const Case cases[] = {
        { "24 cuts x 96 dB/oct (PERF-02)", Type::highCut, 7, 0 },
        { "24 shelves x 96 dB/oct",        Type::highShelf, 7, 9 },
        { "24 shelves x 48 dB/oct",        Type::highShelf, 5, 9 },
        { "24 shelves x 12 dB/oct",        Type::highShelf, 1, 9 },
        { "24 bells",                      Type::bell, 0, 9 },
        { "24 tilts x 96 dB/oct",          Type::tilt, 7, 9 },
    };

    std::printf ("48 kHz stereo, 512-sample blocks, 20 s, noise; %% of one core\n");
    std::printf ("%-32s | %8s | %10s %10s %10s | %10s\n", "case", "settled", "sweep spec", "sweep Gc", "sweep coef", "design only");

    for (const auto& c : cases)
    {
        double res[5] = {};
        for (int mode = 0; mode < 5; ++mode)
        {
            Engine e;  e.prepare (fs, 24);  e.lanes = 2;
            e.interp = mode == 2 ? interpGc : (mode == 3 ? interpCoef : interpSpec);
            std::vector<double> L (512), R (512);
            rngState = 99;
            const size_t n = (size_t) (20.0 * fs);
            double fill = 0.0;
            {
                const auto t0 = Clock::now();
                for (size_t pos = 0; pos < n; pos += 512)
                    for (int i = 0; i < 512; ++i) { L[(size_t) i] = (urand() - 0.5) * 0.5;  R[(size_t) i] = (urand() - 0.5) * 0.5; }
                fill = secs (t0, Clock::now());
            }
            const auto t0 = Clock::now();
            long blk = 0;
            for (size_t pos = 0; pos < n; pos += 512, ++blk)
            {
                for (int i = 0; i < 512; ++i) { L[(size_t) i] = (urand() - 0.5) * 0.5;  R[(size_t) i] = (urand() - 0.5) * 0.5; }
                for (int b = 0; b < 24; ++b)
                {
                    BandSettings s { c.t, 0, c.g, 1.0, c.slope };
                    // sweeping: a triangle in log frequency, 40 Hz - 12 kHz, period 2 s, per band offset; gain wobble
                    const double ph = std::fmod ((double) pos / fs / 2.0 + b / 24.0, 1.0);
                    const double tri = ph < 0.5 ? 2 * ph : 2 - 2 * ph;
                    s.freqHz = mode == 0 ? 40.0 * std::pow (300.0, b / 23.0) : 40.0 * std::pow (300.0, tri);
                    if (mode != 0 && c.g != 0) s.gainDbEff = c.g * (0.5 + tri);
                    e.setTarget (b, s);
                }
                if (mode == 4)
                {
                    // design only: tick the control without running audio
                    for (int i = 0; i < 512; i += e.K) e.tick();
                    for (auto& b : e.bands) if (b.seg) { for (int i = 0; i < b.n; ++i) b.P[i] = b.T[i]; }
                }
                else
                    e.process (L.data(), R.data(), 512);
            }
            res[mode] = 100.0 * (secs (t0, Clock::now()) - fill) / 20.0;
        }
        std::printf ("%-32s | %8.2f | %10.2f %10.2f %10.2f | %10.2f\n", c.name, res[0], res[1], res[2], res[3], res[4]);
    }
}

//==============================================================================
// keep: a continuous change keeps the filter state (verify 2.2, gap 1)
//==============================================================================
static void modeKeep()
{
    const double fs = 48000.0;
    // A ringing low bell: excite with a burst, then silence; edit the gain during the ring.
    for (int variant = 0; variant < 3; ++variant)
    {
        Engine e;  e.prepare (fs, 1);
        BandSettings s { Type::bell, 40.0, 24.0, 20.0, 0 };
        e.setTarget (0, s);
        const size_t n = (size_t) fs;
        std::vector<double> y (n, 0.0);
        for (size_t i = 0; i < 480; ++i) y[i] = 0.5 * std::sin (2.0 * pi * 40.0 * (double) i / fs);
        const size_t edit = 24000;      // a block boundary, a tick boundary
        e.process (y.data(), nullptr, (int) edit);
        s.gainDbEff = 18.0;
        if (variant == 1) e.bands[0].clearState();                 // mutant: the edit resets the filter
        if (variant == 2) { e.smooth = false; }                    // 2.2 behaviour: step, state kept
        e.setTarget (0, s);
        e.process (y.data() + edit, nullptr, (int) (n - edit));

        // Continuity at the edit: |y[edit] - (2 y[edit-1] - y[edit-2])| against the ring's own curvature just before.
        const double pred = 2.0 * y[edit - 1] - y[edit - 2];
        double ownD2 = 0;
        for (size_t i = edit - 2400; i + 1 < edit; ++i) ownD2 = std::max (ownD2, std::fabs (y[i + 1] - 2 * y[i] + y[i - 1]));
        double after = 0;
        for (size_t i = edit; i < edit + 2400; ++i) after = std::max (after, std::fabs (y[i]));
        double before = 0;
        for (size_t i = edit - 2400; i < edit; ++i) before = std::max (before, std::fabs (y[i]));
        std::printf ("%-28s: ring peak before %.3e, after %.3e; |y[edit] - extrapolation| = %.3e = %.1f x the ring's own max d2\n",
                     variant == 0 ? "ramp, state kept" : (variant == 1 ? "mutant: state cleared" : "2.2 step, state kept"),
                     before, after, std::fabs (y[edit] - pred), std::fabs (y[edit] - pred) / ownD2);
    }
}


//==============================================================================
// zero: is the design continuous where the gain changes sign (boost <-> inverse)?
//==============================================================================
static void modeZero (double eps)
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        Designer d;  d.prepare (fs);
        double worst = 0.0, worstStep = 0.0;
        for (int i = 0; i < 200000; ++i)
        {
            BandSettings s;
            const Type types[] = { Type::bell, Type::lowShelf, Type::highShelf, Type::tilt };
            s.type = types[(int) (urand() * 4)];
            s.slopeIndex = (int) (urand() * 8);
            s.freqHz = std::exp (std::log (10.0) + urand() * std::log (3000.0));
            s.q = std::exp (std::log (0.025) + urand() * std::log (1600.0));
            BandDesign a, b, c;
            s.gainDbEff = eps;   d.design (s, a);
            s.gainDbEff = -eps;  d.design (s, b);
            s.gainDbEff = 3 * eps;  d.design (s, c);
            for (int q = 0; q < a.numSections; ++q)
            {
                const auto& x = a.params[(size_t) q];  const auto& y = b.params[(size_t) q];  const auto& z = c.params[(size_t) q];
                // relative change across zero against the same-size step on one side
                auto rel = [] (double u, double v) { return std::fabs (u - v) / std::max (1e-300, std::max (std::fabs (u), std::fabs (v))); };
                const double across = std::max ({ rel (x.g, y.g), rel (x.k, y.k), rel (x.mL, y.mL), rel (x.mB, y.mB), rel (x.mH, y.mH),
                                                  x.hasOnePole ? rel (x.G, y.G) : 0.0, x.hasOnePole ? rel (x.cL, y.cL) : 0.0, x.hasOnePole ? rel (x.cH, y.cH) : 0.0 });
                const double side = std::max ({ rel (x.g, z.g), rel (x.k, z.k), rel (x.mL, z.mL), rel (x.mB, z.mB), rel (x.mH, z.mH),
                                                x.hasOnePole ? rel (x.G, z.G) : 0.0, x.hasOnePole ? rel (x.cL, z.cL) : 0.0, x.hasOnePole ? rel (x.cH, z.cH) : 0.0 });
                worst = std::max (worst, across);
                if (side > 0) worstStep = std::max (worstStep, across / side);
            }
        }
        std::printf ("fs %6.0f: gain +%g -> -%g dB, 200 000 settings: worst relative parameter change %.3e; worst ratio to the one-sided step of the same size %.2f\n",
                     fs, eps, eps, worst, worstStep);
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "g3")         modeG3 (false);
    else if (mode == "g3var") modeG3 (true);
    else if (mode == "lyap")  modeLyap();
    else if (mode == "g5")    modeG5 (argc > 2 ? std::atof (argv[2]) : 20.0, argc > 3 ? std::atof (argv[3]) : 0.0);
    else if (mode == "g7")    modeG7();
    else if (mode == "blk")   modeBlk();
    else if (mode == "cpu")   modeCpu();
    else if (mode == "keep")  modeKeep();
    else if (mode == "zero")  { modeZero (0.02);  modeZero (0.0002); }
    else { std::printf ("modes: g3 | g3var | lyap | g5 [seconds] | g7 | blk | cpu | keep | zero\n"); return 1; }
    (void) typeName;
    return 0;
}
