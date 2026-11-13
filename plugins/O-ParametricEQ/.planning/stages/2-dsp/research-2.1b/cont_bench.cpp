// O-ParametricEQ, Stage 2 research for Phase 2.1b (continuity of the polish).
// A scratch bench over the real Source/dsp headers: the shipped polish with
// run-time knobs, scored for continuity (the C1 line test), accuracy (G1 grid,
// weak rows) and never-worse. Not plugin code and not a gate.
//
//   clang++ -O3 -std=c++17 -I ../../../../Source cont_bench.cpp -o cont_bench
//   ./cont_bench <mode> [key=value ...]
//
// Modes: sanity | diag | c1 | acc | rand        (see main)

#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

using namespace eq;

//==============================================================================
struct Cfg
{
    int steps = 2, extra = 2;
    double lambda = 1e-3, mu = 0.1;
    double keep = 0.002, full = 0.006, ext = 0.03, ramp = 0.1;

    double trust = 0.0;         // > 0: smooth limit on the norm of every step (log units)
    double distLo = 0.0, distHi = 0.0;  // accept fade on max |phi - phi0|
    double total = 0.0;         // > 0: smooth limit on |phi - phi0| after every step
    int key = 0;                // analog-keyed fade, see keyWeight
    double keyA = 0.0, keyB = 0.0, keyC = 0.0, keyD = 0.0;
    int accept = 0;             // accept-rule variant
    int ls = 0;                 // > 0: smooth "best of" along the segment (see polishV)
    double muRes = 0.0;         // > 0: mu scales with the seed's mean-square residual
    double nu = 0.0, nuRes = 0.0;   // Levenberg damping on the diagonal only: nu + nuRes x (current mean-square residual, dB^2)
    double nuPow = 1.0;         // exponent on the mean-square residual
    int val = 0;                // 1: accept also scores points at the fitted and the seed section's own pole / zero
    int topPts = 0;             // extra fixed points, linear over the top octave of the band
    int quiet = 0;
    int rc = 0;                 // 1: shelf sections: a pair above rcW x pi is re-centred there (seed side only)
    double rcW = 0.95;
    double rcQa = 0.0, rcQb = 0.0;  // > 0: the move toward rcW x pi fades in from pair Q = rcQa to rcQb (log scale)
    int rcSeedOnly = 0;         // 1: the 'seed' column also uses the re-centred seed (for seed-vs-seed tables)
    int polishOn = 1;
};

static Cfg cfg;

struct Trace
{
    double S0 = 0, S2 = 0, S1 = 0, wx = 0, a = 0, key = 1;
    double phi0[8] {}, phiK[8][8] {};
    int n = 0;
    bool ran = false;
};

static double sstep (double x)   { return detail::smoothStep (x); }

struct Work { int n = 0; double t[64] {}, target[64] {}; };

static double objectiveV (const Work& wk, const double* phi)
{
    double e[8];
    for (int i = 0; i < 8; ++i) e[i] = std::exp (phi[i]);
    double S = 0.0;
    for (int j = 0; j < wk.n; ++j)
    {
        const double r = detail::polishDbPerNeper * (detail::polishEvalRow (e, wk.t[j], nullptr) - wk.target[j]);
        S += r * r;
    }
    return S;
}

static double analogLog (const AnalogSection& p, double w)
{
    const double wz2 = p.wz * p.wz, wp2 = p.wp * p.wp;
    const double kz = wz2 / (p.Qz * p.Qz), kp = wp2 / (p.Qp * p.Qp);
    const double sc = p.c * p.c * (wp2 * wp2) / (wz2 * wz2);
    const double x = w * w, dz = wz2 - x, dp = wp2 - x;
    return std::log (sc * (dz * dz + kz * x) / (dp * dp + kp * x));
}

/** Squared dB residuals of phi at the pole and zero frequency of the section psi (clamped into the band). */
static double valResidual (const PolishGrid& grid, const AnalogSection& proto, const double* phi, const double* psi)
{
    double e[8];
    for (int i = 0; i < 8; ++i) e[i] = std::exp (phi[i]);
    const double wPole = 2.0 * std::atan (std::exp (psi[0]));
    const double wZero = 2.0 * std::atan (std::exp (psi[0] + 0.5 * (psi[2] - psi[4])));
    double S = 0.0;
    for (double w : { wPole, wZero })
    {
        w = std::min (std::max (w, grid.wLo), grid.wHi);
        const double tn = std::tan (w / 2.0);
        const double r = detail::polishDbPerNeper * (detail::polishEvalRow (e, tn * tn, nullptr) - analogLog (proto, w));
        S += r * r;
    }
    return S;
}

//==============================================================================
static bool stepV (const Work& wk, const double* phi0, double* phi, double mu)
{
    constexpr int nf = polish::numFree;
    double e[8];
    for (int i = 0; i < 8; ++i) e[i] = std::exp (phi[i]);

    double A[nf][nf] = {};
    double b[nf] = {};
    double Scur = 0.0;

    for (int j = 0; j < wk.n; ++j)
    {
        double J[nf];
        const double r = detail::polishDbPerNeper * (detail::polishEvalRow (e, wk.t[j], J) - wk.target[j]);
        Scur += r * r;
        for (int a = 0; a < nf; ++a)
        {
            const double ja = detail::polishDbPerNeper * J[a];
            b[a] += ja * r;
            for (int c = 0; c <= a; ++c)
                A[a][c] += ja * (detail::polishDbPerNeper * J[c]);
        }
    }

    const double damp = cfg.nu + cfg.nuRes * std::pow (Scur / wk.n, cfg.nuPow);

    for (int a = 0; a < nf; ++a)
    {
        A[a][a] += cfg.lambda * A[a][a] + mu + damp;
        b[a] += mu * (phi[detail::polishFree[a]] - phi0[detail::polishFree[a]]);
    }

    for (int a = 0; a < nf; ++a)
        for (int c = 0; c <= a; ++c)
        {
            double s = A[a][c];
            for (int q = 0; q < c; ++q) s -= A[a][q] * A[c][q];
            if (a == c) { if (! (s > 0.0)) return false; A[a][c] = std::sqrt (s); }
            else A[a][c] = s / A[c][c];
        }

    for (int a = 0; a < nf; ++a)
    {
        double s = b[a];
        for (int q = 0; q < a; ++q) s -= A[a][q] * b[q];
        b[a] = s / A[a][a];
    }
    for (int a = nf - 1; a >= 0; --a)
    {
        double s = b[a];
        for (int q = a + 1; q < nf; ++q) s -= A[q][a] * b[q];
        b[a] = s / A[a][a];
    }

    if (cfg.trust > 0.0)
    {
        double n2 = 0.0;
        for (int a = 0; a < nf; ++a) n2 += b[a] * b[a];
        const double sc = cfg.trust / std::sqrt (cfg.trust * cfg.trust + n2);
        for (int a = 0; a < nf; ++a) b[a] *= sc;
    }

    bool finite = true;
    for (int a = 0; a < nf; ++a)
    {
        phi[detail::polishFree[a]] -= b[a];
        finite = finite && std::isfinite (phi[detail::polishFree[a]]);
    }

    if (cfg.total > 0.0 && finite)
    {
        double n2 = 0.0;
        for (int a = 0; a < nf; ++a)
        {
            const double d = phi[detail::polishFree[a]] - phi0[detail::polishFree[a]];
            n2 += d * d;
        }
        const double sc = cfg.total / std::sqrt (cfg.total * cfg.total + n2);
        for (int a = 0; a < nf; ++a)
        {
            const int i = detail::polishFree[a];
            phi[i] = phi0[i] + sc * (phi[i] - phi0[i]);
        }
    }

    return finite;
}

/** A weight in [0, 1] from the analog section alone. */
static double keyWeight (const PolishGrid& grid, const AnalogSection& p)
{
    switch (cfg.key)
    {
        case 0: return 1.0;

        // 1: fade out as the HIGHER of (wz, wp) passes the top of the match band.
        //    full below keyA x wHi, zero above keyB x wHi (log scale).
        case 1:
        {
            const double wTop = std::max (p.wz, p.wp) / grid.wHi;
            return sstep ((std::log (cfg.keyB) - std::log (wTop)) / (std::log (cfg.keyB) - std::log (cfg.keyA)));
        }

        // 2: the same on the LOWER of the two.
        case 2:
        {
            const double wLow = std::min (p.wz, p.wp) / grid.wHi;
            return sstep ((std::log (cfg.keyB) - std::log (wLow)) / (std::log (cfg.keyB) - std::log (cfg.keyA)));
        }

        // 3: key 1 times a fade on the section Q (max of Qz, Qp): full below keyC, zero above keyD.
        case 3:
        {
            const double wTop = std::max (p.wz, p.wp) / grid.wHi;
            const double q = std::max (p.Qz, p.Qp);
            const double a = sstep ((std::log (cfg.keyB) - std::log (wTop)) / (std::log (cfg.keyB) - std::log (cfg.keyA)));
            const double b = sstep ((std::log (cfg.keyD) - std::log (q)) / (std::log (cfg.keyD) - std::log (cfg.keyC)));
            return 1.0 - (1.0 - a) * (1.0 - b);     // fades only where BOTH are out
        }
    }

    return 1.0;
}

static SectionParams polishV (const PolishGrid& grid, const AnalogSection& trueProto,
                              const SectionParams& seed, PolishInfo* info, Trace* tr)
{
    int stepsRun = 0;
    if (info) { info->weight = 0.0; info->steps = 0; }

    const double s[8] = { seed.g, seed.k, seed.mL, seed.mB, seed.mH, seed.G, seed.cL, seed.cH };
    double phi0[8];
    bool finite = true;
    for (int i = 0; i < 8; ++i) { phi0[i] = std::log (s[i]); finite = finite && std::isfinite (phi0[i]); }

    Work wk;
    double w[64];
    int n = 0;
    for (int i = 0; i < polish::fixedPoints; ++i) { w[n] = grid.w[i]; wk.t[n] = grid.t[i]; ++n; }
    for (int i = 0; i < cfg.topPts; ++i)
    {
        w[n] = grid.wHi * (0.5 + 0.5 * (i + 0.5) / cfg.topPts);
        const double tn = std::tan (w[n] / 2.0);
        wk.t[n] = tn * tn;
        ++n;
    }
    const int firstTied = n;
    {
        const double centre[2] = { trueProto.wz, trueProto.wp };
        const double quality[2] = { trueProto.Qz, trueProto.Qp };
        for (int pair = 0; pair < 2; ++pair)
        {
            w[n++] = centre[pair];
            for (int i = 0; i < polish::numTiedScales; ++i)
            {
                const double h = polish::tiedScales[i] / (2.0 * quality[pair]);
                const double rt = std::sqrt (1.0 + h * h);
                w[n++] = centre[pair] * (rt + h);
                w[n++] = centre[pair] * (rt - h);
            }
        }
    }
    for (int j = firstTied; j < n; ++j)
    {
        double x = w[j];
        x = (x < grid.wLo) ? grid.wLo : x;
        x = (x > grid.wHi) ? grid.wHi : x;
        w[j] = x;
        const double tn = std::tan (x / 2.0);
        wk.t[j] = tn * tn;
    }
    wk.n = n;
    const int np = n;
    for (int j = 0; j < np; ++j)
    {
        wk.target[j] = analogLog (trueProto, w[j]);
        finite = finite && std::isfinite (wk.target[j]);
    }

    if (! finite) return seed;

    const double S0 = objectiveV (wk, phi0);
    if (tr) { tr->S0 = S0; for (int i = 0; i < 8; ++i) tr->phi0[i] = phi0[i]; }

    if (! std::isfinite (S0) || S0 / np <= cfg.keep * cfg.keep) return seed;

    const double kw = keyWeight (grid, trueProto);
    if (tr) tr->key = kw;
    if (! (kw > 0.0)) return seed;

    const double mu = cfg.muRes > 0.0 ? cfg.mu * (1.0 + cfg.muRes * S0 / np) : cfg.mu;

    double phi[8];
    for (int i = 0; i < 8; ++i) phi[i] = phi0[i];

    for (int it = 0; it < cfg.steps; ++it)
    {
        ++stepsRun;
        if (! stepV (wk, phi0, phi, mu)) return seed;
        if (tr) { for (int i = 0; i < 8; ++i) tr->phiK[tr->n][i] = phi[i]; ++tr->n; }
    }

    const double S2 = objectiveV (wk, phi);
    if (! std::isfinite (S2)) return seed;

    const double wx = sstep ((std::sqrt (S2 / np) - cfg.ext) / cfg.ext);
    if (tr) { tr->S2 = S2; tr->wx = wx; tr->ran = true; }

    if (wx > 0.0 && cfg.extra > 0)
    {
        double phi4[8];
        bool ok = true;
        for (int i = 0; i < 8; ++i) phi4[i] = phi[i];
        for (int it = 0; it < cfg.extra; ++it)
        {
            ++stepsRun;
            ok = ok && stepV (wk, phi0, phi4, mu);
            if (tr && ok) { for (int i = 0; i < 8; ++i) tr->phiK[tr->n][i] = phi4[i]; ++tr->n; }
        }
        if (ok)
            for (int i = 0; i < 8; ++i) phi[i] = phi[i] + wx * (phi4[i] - phi[i]);
    }

    const double S1 = objectiveV (wk, phi);
    finite = std::isfinite (S1);
    for (int i = 0; i < 8; ++i) finite = finite && std::isfinite (phi[i]);
    if (! finite) return seed;

    const double keep2 = cfg.keep * cfg.keep;
    const double full2 = cfg.full * cfg.full;
    double A0 = S0, A1 = S1;
    if (cfg.val)
    {
        A0 += valResidual (grid, trueProto, phi0, phi0) + valResidual (grid, trueProto, phi0, phi);
        A1 += valResidual (grid, trueProto, phi, phi0) + valResidual (grid, trueProto, phi, phi);
        if (! std::isfinite (A0) || ! std::isfinite (A1)) return seed;
    }
    double a = sstep ((S0 / np - keep2) / (full2 - keep2)) * sstep ((1.0 - A1 / A0) / cfg.ramp) * kw;

    if (cfg.distHi > 0.0)
    {
        double d = 0.0;
        for (int i = 0; i < 8; ++i) d = std::max (d, std::fabs (phi[i] - phi0[i]));
        a *= sstep ((cfg.distHi - d) / (cfg.distHi - cfg.distLo));
    }

    if (tr) { tr->S1 = S1; tr->a = a; }
    if (! (a > 0.0)) return seed;

    SectionParams out = seed;
    out.g  = std::exp (phi0[0] + a * (phi[0] - phi0[0]));
    out.k  = std::exp (phi0[1] + a * (phi[1] - phi0[1]));
    out.mB = std::exp (phi0[3] + a * (phi[3] - phi0[3]));
    out.mH = std::exp (phi0[4] + a * (phi[4] - phi0[4]));
    out.G  = std::exp (phi0[5] + a * (phi[5] - phi0[5]));
    out.cH = std::exp (phi0[7] + a * (phi[7] - phi0[7]));

    const double free6[polish::numFree] = { out.g, out.k, out.mB, out.mH, out.G, out.cH };
    for (int i = 0; i < polish::numFree; ++i)
        if (! (std::isfinite (free6[i]) && free6[i] > 0.0)) return seed;

    if (info) { info->weight = a; info->steps = stepsRun; }
    return out;
}

/** A pair (w0, Q) above wL is replaced by (wL, Qe): same DC value, same magnitude at wL. */
static void recentrePair (double& w0, double& Q, double wL)
{
    if (! (w0 > wL)) return;
    double beta = 1.0;
    if (cfg.rcQb > 0.0)
        beta = sstep ((std::log (Q) - std::log (cfg.rcQa)) / (std::log (cfg.rcQb) - std::log (cfg.rcQa)));
    if (! (beta > 0.0)) return;
    const double wT = std::exp (std::log (w0) + beta * (std::log (wL) - std::log (w0)));
    const double x = wT / w0, x2 = x * x;
    const double P = (1.0 - x2) * (1.0 - x2) + x2 / (Q * Q);
    double Qe = 1.0 / std::sqrt (P);
    Qe = std::min (std::max (Qe, recentreQMin), recentreQMax);
    w0 = wT;
    Q = Qe;
}

//==============================================================================
/** Gain types only (bell, shelves, tilt). polished = 0 seed, 1 the variant, 2 the MLR seed of 2.1 whatever cfg.rc says. */
static void designV (double fs, const PolishGrid& grid, const BandSettings& s, BandDesign& out,
                     int polished, Trace* traces = nullptr)
{
    AnalogBand proto;
    buildPrototype (s, fs, proto);
    out.numSections = proto.numSections;
    out.inverted = s.gainDbEff < 0.0;

    for (int i = 0; i < maxSections; ++i)
    {
        const auto u = static_cast<std::size_t> (i);
        out.cls[u] = proto.cls[u];
        out.trueProto[u] = proto.trueSide[u];
        out.polishWeight[u] = 0.0f;

        if (i >= proto.numSections) { out.params[u] = detail::neutralParams(); continue; }

        AnalogSection a = proto.designSide[u];
        if (proto.cls[u] == SectionClass::gain2)
        {
            if (cfg.rc && polished != 2 && s.type != Type::bell)
            {
                recentrePair (a.wz, a.Qz, cfg.rcW * pi);
                recentrePair (a.wp, a.Qp, cfg.rcW * pi);
            }
            out.params[u] = mlr (a.wz, a.Qz, a.wp, a.Qp, a.c);
            if (polished == 1)
            {
                PolishInfo info;
                out.params[u] = polishV (grid, proto.trueSide[u], out.params[u], &info, traces ? &traces[i] : nullptr);
                out.polishWeight[u] = static_cast<float> (info.weight);
            }
        }
        else
            out.params[u] = firstOrderShelf (proto.wc, proto.shelfGain[u], proto.shelfHigh[u]);
    }

    if (s.type == Type::tilt && proto.numSections > 0)
    {
        const auto last = static_cast<std::size_t> (proto.numSections - 1);
        out.params[last].cL *= proto.tiltScalar;
        out.params[last].cH *= proto.tiltScalar;
        out.trueProto[last].c *= proto.tiltScalar;
    }

    if (out.inverted)
        for (int i = 0; i < proto.numSections; ++i)
            invert (out.params[static_cast<std::size_t> (i)]);
}

//==============================================================================
struct Rng
{
    explicit Rng (std::uint64_t seed) : state (seed) {}
    std::uint64_t next()
    {
        state += 0x9E3779B97F4A7C15ull;
        std::uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    double uniform()   { return static_cast<double> (next() >> 11) * (1.0 / 9007199254740992.0); }
    std::uint64_t state;
};

static void parallelFor (int n, const std::function<void (int)>& fn)
{
    const int nt = static_cast<int> (std::max (1u, std::thread::hardware_concurrency()));
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < nt; ++t)
        pool.emplace_back ([&] { for (;;) { const int i = next.fetch_add (1); if (i >= n) return; fn (i); } });
    for (auto& th : pool) th.join();
}

static const char* typeName (Type t)
{
    switch (t) { case Type::bell: return "bell"; case Type::lowShelf: return "lowShelf";
                 case Type::highShelf: return "highShelf"; case Type::tilt: return "tilt"; default: return "?"; }
}

static double bandMag2 (const BandDesign& d, double t2)
{
    double m = 1.0;
    for (int i = 0; i < d.numSections; ++i) m *= sectionMag2 (t2, d.params[static_cast<std::size_t> (i)]);
    return m;
}

//==============================================================================
// Error of a band against its true prototype: 400 log points + points tied to
// every section's zero and pole pair.
struct EvalGrid
{
    explicit EvalGrid (double fsIn) : fs (fsIn)
    {
        fHi = std::min (20000.0, 0.499 * fs);
        wLo = 2.0 * pi * 20.0 / fs;
        wHi = 2.0 * pi * fHi / fs;
        for (int i = 0; i < 400; ++i)
            w.push_back (2.0 * pi * 20.0 * std::pow (fHi / 20.0, i / 399.0) / fs);
    }
    double fs, fHi, wLo, wHi;
    std::vector<double> w;
};

static double bandError (const EvalGrid& g, const BandDesign& d)
{
    double worst = 0.0;
    const auto point = [&] (double w)
    {
        const double e = std::fabs (bandMagDb (d, w) - analogBandMagDb (d, w));
        if (! (e <= worst)) worst = std::isfinite (e) ? e : 1e9;
    };
    for (const double w : g.w) point (w);

    static const double scales[5] = { 0.25, 0.5, 1.0, 1.5, 3.0 };
    for (int i = 0; i < d.numSections; ++i)
    {
        const AnalogSection& a = d.trueProto[static_cast<std::size_t> (i)];
        if (d.cls[static_cast<std::size_t> (i)] != SectionClass::gain2) continue;
        const double c[2] = { a.wz, a.wp }, q[2] = { a.Qz, a.Qp };
        for (int p = 0; p < 2; ++p)
        {
            point (std::min (std::max (c[p], g.wLo), g.wHi));
            for (const double sc : scales)
            {
                const double h = sc / (2.0 * q[p]);
                const double rt = std::sqrt (1.0 + h * h);
                point (std::min (std::max (c[p] * (rt + h), g.wLo), g.wHi));
                point (std::min (std::max (c[p] * (rt - h), g.wLo), g.wHi));
            }
        }
    }
    return worst;
}

/** One section against its own prototype (boost side). */
static double sectionError (const EvalGrid& g, const SectionParams& p, const AnalogSection& a)
{
    BandDesign d {};
    d.numSections = 1;
    d.params[0] = p;
    d.cls[0] = SectionClass::gain2;
    d.trueProto[0] = a;
    d.inverted = false;
    return bandError (g, d);
}

//==============================================================================
// C1 lines
struct Line
{
    std::string family;
    int index;
    Type type;
    int slopeIndex;
    double fs, fc0, fc1, gain0, gain1, q0, q1;
};

struct LineResult { double seedJump[2] {}, polJump[2] {}; double xAt = 0; bool bad = false; };

static BandSettings atX (const Line& l, double x)
{
    BandSettings s;
    s.type = l.type;
    s.freqHz = std::exp (std::log (l.fc0) + x * (std::log (l.fc1) - std::log (l.fc0)));
    s.gainDbEff = l.gain0 + x * (l.gain1 - l.gain0);
    s.q = std::exp (std::log (l.q0) + x * (std::log (l.q1) - std::log (l.q0)));
    s.slopeIndex = l.slopeIndex;
    return s;
}

static LineResult runLine (const Line& line, int n0 = 8000, int n1 = 32000)
{
    constexpr int numPoints = 160;
    const int stepCounts[2] = { n0, n1 };
    double t2[numPoints];
    const double fHi = std::min (20000.0, 0.499 * line.fs);
    for (int p = 0; p < numPoints; ++p)
        t2[p] = tanSquaredHalf (2.0 * pi * 20.0 * std::pow (fHi / 20.0, p / (numPoints - 1.0)) / line.fs);

    PolishGrid grid;
    grid.prepare (line.fs);
    BandDesign sd {}, pd {};
    double ps[numPoints] {}, pp[numPoints] {};
    LineResult r;

    for (int pass = 0; pass < 2; ++pass)
    {
        const int n = stepCounts[pass];
        double ws = 1.0, wp = 1.0;
        for (int i = 0; i < n; ++i)
        {
            const double x = static_cast<double> (i) / (n - 1.0);
            const BandSettings s = atX (line, x);
            designV (line.fs, grid, s, sd, 0);
            designV (line.fs, grid, s, pd, cfg.polishOn);
            for (int p = 0; p < numPoints; ++p)
            {
                const double a = bandMag2 (sd, t2[p]);
                const double b = bandMag2 (pd, t2[p]);
                if (i > 0)
                {
                    const double ra = a > ps[p] ? a / ps[p] : ps[p] / a;
                    const double rb = b > pp[p] ? b / pp[p] : pp[p] / b;
                    if (! (ra >= 1.0) || ! (rb >= 1.0) || ! std::isfinite (ra) || ! std::isfinite (rb)) r.bad = true;
                    if (ra > ws) ws = ra;
                    if (rb > wp) { wp = rb; if (pass == 1) r.xAt = x; }
                }
                ps[p] = a;
                pp[p] = b;
            }
        }
        r.seedJump[pass] = 10.0 * std::log10 (ws);
        r.polJump[pass] = 10.0 * std::log10 (wp);
    }
    return r;
}

struct Family { const char* name; int slopeIndex; int mode; };   // mode 0 bell, 1 shelves, 2 tilt

static std::vector<Line> makeLines (int perFamily, std::uint64_t seed, const std::vector<double>& rates, int famMask)
{
    static const Family fams[] = {
        { "bell", 1, 0 }, { "shelf12", 1, 1 }, { "shelf24", 3, 1 }, { "shelf48", 5, 1 }, { "shelf96", 7, 1 },
        { "tilt12", 1, 2 }, { "tilt48", 5, 2 }, { "shelf18", 2, 1 }, { "shelf72", 6, 1 }
    };
    std::vector<Line> lines;
    Rng rng (seed);
    int fi = 0;
    for (const Family& f : fams)
    {
        const bool use = (famMask >> fi++) & 1;
        for (const double fs : rates)
            for (int k = 0; k < perFamily; ++k)
            {
                Line l;
                l.family = f.name;
                l.index = k;
                l.type = f.mode == 0 ? Type::bell : (f.mode == 2 ? Type::tilt : (k % 2 == 0 ? Type::lowShelf : Type::highShelf));
                l.slopeIndex = f.slopeIndex;
                l.fs = fs;
                l.fc0 = 10.0 * std::pow (3000.0, rng.uniform());
                l.gain0 = -60.0 + 120.0 * rng.uniform();
                l.q0 = 0.025 * std::pow (1600.0, rng.uniform());
                l.fc1 = 10.0 * std::pow (3000.0, rng.uniform());
                l.gain1 = -60.0 + 120.0 * rng.uniform();
                l.q1 = 0.025 * std::pow (1600.0, rng.uniform());
                if (use) lines.push_back (l);
            }
    }
    return lines;
}

static std::string describe (const Line& l)
{
    char buf[400];
    std::snprintf (buf, sizeof buf, "%s #%d %s si %d fs %.0f: fc %.9g -> %.9g, gain %.9g -> %.9g, Q %.9g -> %.9g",
                   l.family.c_str(), l.index, typeName (l.type), l.slopeIndex, l.fs, l.fc0, l.fc1, l.gain0, l.gain1, l.q0, l.q1);
    return buf;
}

static int modeC1 (int perFamily, std::uint64_t seed, const std::vector<double>& rates, int famMask, int verbose)
{
    const std::vector<Line> lines = makeLines (perFamily, seed, rates, famMask);
    std::vector<LineResult> res (lines.size());
    parallelFor (static_cast<int> (lines.size()), [&] (int i) { res[static_cast<std::size_t> (i)] = runLine (lines[static_cast<std::size_t> (i)]); });

    struct Tally { int lines = 0, viol = 0, hard = 0, ratioOnly = 0; double worstRatio = 0, worstShrink = 0, worstHardDb = 0; };
    std::vector<std::pair<std::string, Tally>> tallies;
    int total = 0, totalHard = 0;
    const double fl = 1e-9;

    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        char key[80];
        std::snprintf (key, sizeof key, "%-8s %6.0f", lines[i].family.c_str(), lines[i].fs);
        if (tallies.empty() || tallies.back().first != key) tallies.push_back ({ key, {} });
        Tally& t = tallies.back().second;
        const LineResult& r = res[i];
        ++t.lines;
        double ratio = 0;
        for (int p = 0; p < 2; ++p) ratio = std::max (ratio, std::max (r.polJump[p], fl) / std::max (r.seedJump[p], fl));
        const double shrink = r.polJump[1] > fl ? std::max (r.polJump[1], fl) / std::max (r.polJump[0], fl) : 0.0;
        t.worstRatio = std::max (t.worstRatio, ratio);
        t.worstShrink = std::max (t.worstShrink, shrink);
        const bool viol = r.bad || ratio > 2.0 || shrink > 0.5;
        const bool hard = r.bad || shrink > 0.5;       // does not shrink: a jump
        if (viol) { ++t.viol; ++total; }
        if (hard) { ++t.hard; ++totalHard; t.worstHardDb = std::max (t.worstHardDb, r.polJump[1]); }
        if (viol && ! hard) ++t.ratioOnly;
        if (viol && verbose)
            std::printf ("  VIOL %s | seed %.5f / %.5f  pol %.5f / %.5f dB  ratio %.2f shrink %.3f  x %.6f%s\n",
                         describe (lines[i]).c_str(), r.seedJump[0], r.seedJump[1], r.polJump[0], r.polJump[1],
                         ratio, shrink, r.xAt, hard ? "  HARD" : "");
    }

    for (const auto& kv : tallies)
        std::printf ("  %s  lines %3d  viol %2d (jump %2d, steep-only %2d)  worst ratio %9.3f  worst shrink %.3f  worst jump %.4f dB\n",
                     kv.first.c_str(), kv.second.lines, kv.second.viol, kv.second.hard, kv.second.ratioOnly,
                     kv.second.worstRatio, kv.second.worstShrink, kv.second.worstHardDb);
    std::printf ("C1 total: %zu lines, %d violations, %d jumps (non-shrinking)\n", lines.size(), total, totalHard);
    return total;
}

//==============================================================================
// Accuracy tables
static BandSettings mk (Type t, double fc, double g, double q, int si)
{
    BandSettings s; s.type = t; s.freqHz = fc; s.gainDbEff = g; s.q = q; s.slopeIndex = si; return s;
}

struct Cell { double seed = 0, pol = 0, old = 0; int worse = 0, n = 0; };

/** Worst band error over fc in [fLo, fHi] (nf log points), the Q list, +-gain, low and high (or the bell). */
static Cell cellOf (double fs, Type type, int si, double fLo, double fHi, int nf, const std::vector<double>& qs, double gain)
{
    EvalGrid eg (fs);
    PolishGrid grid;
    grid.prepare (fs);
    Cell c;
    std::vector<Type> types;
    if (type == Type::bell) types = { Type::bell };
    else if (type == Type::tilt) types = { Type::tilt };
    else types = { Type::lowShelf, Type::highShelf };

    for (const Type t : types)
        for (int i = 0; i < nf; ++i)
        {
            const double fc = fLo * std::pow (fHi / fLo, nf > 1 ? i / (nf - 1.0) : 0.0);
            for (const double q : qs)
                for (const double sign : { 1.0, -1.0 })
                {
                    BandDesign sd {}, pd {};
                    const BandSettings s = mk (t, fc, sign * gain, q, si);
                    BandDesign od {};
                    designV (fs, grid, s, od, 2);
                    c.old = std::max (c.old, bandError (eg, od));
                    designV (fs, grid, s, sd, 0);
                    designV (fs, grid, s, pd, cfg.polishOn);
                    const double es = bandError (eg, sd), ep = bandError (eg, pd);
                    c.seed = std::max (c.seed, es);
                    c.pol = std::max (c.pol, ep);
                    ++c.n;
                    if (ep > es + 1e-3) ++c.worse;
                }
        }
    return c;
}

static std::vector<double> logList (double a, double b, int n)
{
    std::vector<double> v;
    for (int i = 0; i < n; ++i) v.push_back (a * std::pow (b / a, i / (n - 1.0)));
    return v;
}

static void accRow (const char* name, double fs, Type type, int si, double fLo, double fHi, int nf, const std::vector<double>& qs)
{
    static const double gains[3] = { 6.0, 12.0, 30.0 };
    Cell c[3];
    parallelFor (3, [&] (int i) { c[i] = cellOf (fs, type, si, fLo, fHi, nf, qs, gains[i]); });
    std::printf ("  %-28s %5.1fk  2.1 seed %6.3f %6.3f %6.3f | seed %6.3f %6.3f %6.3f | pol %6.3f %6.3f %6.3f | worse %d %d %d\n", name, fs / 1000.0,
                 c[0].old, c[1].old, c[2].old, c[0].seed, c[1].seed, c[2].seed, c[0].pol, c[1].pol, c[2].pol, c[0].worse, c[1].worse, c[2].worse);
}

static void modeAcc()
{
    const std::vector<double> qBell = logList (0.025, 40.0, 17);
    std::vector<double> qSh = logList (0.025, 1.0, 8);
    qSh.push_back (1.41);

    for (const double fs : { 44100.0, 48000.0, 96000.0 })
    {
        std::printf ("-- gated grids, fs %.0f (tolerance 44.1/48k: 0.122 0.194 0.410; 96k: 0.042 0.054 0.090)\n", fs);
        accRow ("bell Q .025-40, 20-20k", fs, Type::bell, 1, 20, 20000, 40, qBell);
        accRow ("shelf 12 Q<=1.41, 20-20k", fs, Type::lowShelf, 1, 20, 20000, 40, qSh);
        for (const int si : { 3, 4, 5, 6, 7 })
        {
            char nm[64];
            std::snprintf (nm, sizeof nm, "shelf %d dB/oct Q 1, 20-20k", 6 * slopeOrder[si]);
            accRow (nm, fs, Type::lowShelf, si, 20, 20000, 40, { 1.0 });
        }
        accRow ("tilt 12 Q 1", fs, Type::tilt, 1, 20, 20000, 40, { 1.0 });
        if (fs > 50000.0) continue;
        std::printf ("-- INFO rows, fs %.0f\n", fs);
        accRow ("shelf 18 Q 1, <=10k", fs, Type::lowShelf, 2, 20, 10000, 30, { 1.0 });
        accRow ("shelf 18 Q 1, 10-20k", fs, Type::lowShelf, 2, 10300, 20000, 12, { 1.0 });
        accRow ("weak sh12 Q 2-10, <=10k", fs, Type::lowShelf, 1, 20, 10000, 30, { 2.0, 4.0, 10.0 });
        accRow ("weak sh12 Q 2-10, 10-20k", fs, Type::lowShelf, 1, 10300, 20000, 12, { 2.0, 4.0, 10.0 });
        accRow ("weak sh12 Q 20-40, <=10k", fs, Type::lowShelf, 1, 20, 10000, 30, { 20.0, 40.0 });
        accRow ("weak sh12 Q 20-40, 10-20k", fs, Type::lowShelf, 1, 10300, 20000, 12, { 20.0, 40.0 });
        accRow ("sh48 Q .25 .5 2 4, <=10k", fs, Type::lowShelf, 5, 20, 10000, 30, { 0.25, 0.5, 2.0, 4.0 });
        accRow ("sh48 Q .25 .5 2 4, 10-20k", fs, Type::lowShelf, 5, 10300, 20000, 12, { 0.25, 0.5, 2.0, 4.0 });
        accRow ("sh96 Q 4 10 40, 20-20k", fs, Type::lowShelf, 7, 20, 20000, 40, { 4.0, 10.0, 40.0 });
        accRow ("bell 20-30k", fs, Type::bell, 1, 20000, 30000, 20, qBell);
        accRow ("sh12 Q<=1.41, 20-30k", fs, Type::lowShelf, 1, 20000, 30000, 20, qSh);
        accRow ("weak sh12 Q 2-40, 20-30k", fs, Type::lowShelf, 1, 20000, 30000, 20, { 2.0, 4.0, 10.0, 20.0, 40.0 });
    }
}

//==============================================================================
// Random full-range: never-worse per section, "far" sections, error distribution
static void modeRand (int n)
{
    struct Row { const char* name; int mode; int si; };
    static const Row rows[] = { { "bell", 0, 1 }, { "shelf12", 1, 1 }, { "shelf48", 1, 5 }, { "shelf96", 1, 7 } };

    for (const double fs : { 44100.0, 48000.0, 96000.0 })
        for (const Row& row : rows)
        {
            const int nt = 16;
            struct Acc { long sections = 0, worse = 0, far = 0, polished = 0; double worstExcess = 0; double sumSeed = 0, sumPol = 0;
                         long big = 0, bigFixed = 0; double worstPol = 0; };
            std::vector<Acc> accs (nt);
            parallelFor (nt, [&] (int t)
            {
                Rng rng (0xABCDEF00ull + static_cast<std::uint64_t> (t) * 7919u + static_cast<std::uint64_t> (row.si) * 31u + static_cast<std::uint64_t> (fs));
                EvalGrid eg (fs);
                PolishGrid grid;
                grid.prepare (fs);
                Acc& a = accs[static_cast<std::size_t> (t)];
                for (int i = 0; i < n / nt; ++i)
                {
                    const double fc = 10.0 * std::pow (3000.0, rng.uniform());
                    const double g = -60.0 + 120.0 * rng.uniform();
                    const double q = 0.025 * std::pow (1600.0, rng.uniform());
                    const Type ty = row.mode == 0 ? Type::bell : (rng.uniform() < 0.5 ? Type::lowShelf : Type::highShelf);
                    BandSettings s = mk (ty, fc, std::fabs (g), q, row.si);     // boost side: sections comparable
                    BandDesign sd {}, pd {};
                    designV (fs, grid, s, sd, 0);
                    designV (fs, grid, s, pd, cfg.polishOn);
                    for (int k = 0; k < sd.numSections; ++k)
                    {
                        const auto u = static_cast<std::size_t> (k);
                        if (sd.cls[u] != SectionClass::gain2) continue;
                        ++a.sections;
                        if (pd.polishWeight[u] > 0.0f) ++a.polished;
                        const double es = sectionError (eg, sd.params[u], sd.trueProto[u]);
                        const double ep = sectionError (eg, pd.params[u], sd.trueProto[u]);
                        if (ep > es + 1e-3) { ++a.worse; a.worstExcess = std::max (a.worstExcess, ep - es); }
                        const double rg = std::fabs (std::log (pd.params[u].g / sd.params[u].g));
                        const double rk = std::fabs (std::log (pd.params[u].k / sd.params[u].k));
                        if (std::max (rg, rk) > std::log (10.0)) ++a.far;
                        a.sumSeed += es;
                        a.sumPol += ep;
                        if (es > 1.0) { ++a.big; if (ep < 0.5) ++a.bigFixed; }
                        a.worstPol = std::max (a.worstPol, ep);
                    }
                }
            });
            Acc s;
            for (const Acc& a : accs)
            {
                s.sections += a.sections; s.worse += a.worse; s.far += a.far; s.polished += a.polished;
                s.worstExcess = std::max (s.worstExcess, a.worstExcess); s.sumSeed += a.sumSeed; s.sumPol += a.sumPol;
                s.big += a.big; s.bigFixed += a.bigFixed; s.worstPol = std::max (s.worstPol, a.worstPol);
            }
            std::printf ("  %-8s %6.0f  sections %7ld  w>0 %5.1f %%  worse %4ld (max +%.3f dB)  far %4ld  mean err seed %.4f -> %.4f dB  "
                         "seed>1dB %6ld of which pol<0.5dB %5.1f %%  worst pol %.2f\n",
                         row.name, fs, s.sections, 100.0 * s.polished / std::max (1L, s.sections), s.worse, s.worstExcess, s.far,
                         s.sumSeed / std::max (1L, s.sections), s.sumPol / std::max (1L, s.sections), s.big,
                         100.0 * s.bigFixed / std::max (1L, s.big), s.worstPol);
        }
}


//==============================================================================
// Map: where is the fit reliable? Sections binned by the upper pair's
// frequency over Nyquist (u) and the section Q.
static void modeMap (int n, double fs)
{
    static const double uEdge[] = { 0.5, 0.8, 0.9, 1.0, 1.1, 1.3, 2.0, 1e9 };
    static const double qEdge[] = { 0.7, 1.5, 4.0, 12.0, 1e9 };
    constexpr int NU = 8, NQ = 5;
    struct C { long n = 0, worse = 0, far = 0, big = 0, fixed = 0, w = 0; double es = 0, ep = 0, wmax = 0, epmax = 0; };
    const int nt = 16;
    std::vector<std::vector<C>> all (nt, std::vector<C> (NU * NQ));

    parallelFor (nt, [&] (int t)
    {
        Rng rng (0x51515151ull + static_cast<std::uint64_t> (t) * 104729u);
        EvalGrid eg (fs);
        PolishGrid grid;
        grid.prepare (fs);
        for (int i = 0; i < n / nt; ++i)
        {
            static const int sis[4] = { 1, 3, 5, 7 };
            const int si = sis[rng.next() % 4];
            const BandSettings s = mk (rng.uniform() < 0.5 ? Type::lowShelf : Type::highShelf, 10.0 * std::pow (3000.0, rng.uniform()),
                                       60.0 * rng.uniform(), 0.025 * std::pow (1600.0, rng.uniform()), si);
            BandDesign sd {}, pd {};
            designV (fs, grid, s, sd, 0);
            designV (fs, grid, s, pd, cfg.polishOn);
            for (int k = 0; k < sd.numSections; ++k)
            {
                const auto u = static_cast<std::size_t> (k);
                if (sd.cls[u] != SectionClass::gain2) continue;
                const AnalogSection& a = sd.trueProto[u];
                const double uu = std::max (a.wz, a.wp) / pi;
                int iu = 0, iq = 0;
                while (uu >= uEdge[iu]) ++iu;
                while (a.Qp >= qEdge[iq]) ++iq;
                C& c = all[static_cast<std::size_t> (t)][static_cast<std::size_t> (iu * NQ + iq)];
                const double es = sectionError (eg, sd.params[u], a), ep = sectionError (eg, pd.params[u], a);
                ++c.n; c.es += es; c.ep += ep; c.epmax = std::max (c.epmax, ep);
                if (pd.polishWeight[u] > 0.0f) ++c.w;
                if (ep > es + 1e-3) { ++c.worse; c.wmax = std::max (c.wmax, ep - es); }
                if (std::max (std::fabs (std::log (pd.params[u].g / sd.params[u].g)), std::fabs (std::log (pd.params[u].k / sd.params[u].k))) > std::log (10.0)) ++c.far;
                if (es > 0.5) { ++c.big; if (ep < 0.25 * es) ++c.fixed; }
            }
        }
    });

    C hard, rest;
    if (! cfg.quiet) std::printf ("fs %.0f; cell: n | mean seed -> mean pol dB | worst pol | worse (max excess) | far | of seed>0.5 dB: %% cut 4x\n", fs);
    for (int iu = 0; iu < NU; ++iu)
        for (int iq = 0; iq < NQ; ++iq)
        {
            C c;
            for (int t = 0; t < nt; ++t)
            {
                const C& x = all[static_cast<std::size_t> (t)][static_cast<std::size_t> (iu * NQ + iq)];
                c.n += x.n; c.worse += x.worse; c.far += x.far; c.big += x.big; c.fixed += x.fixed; c.w += x.w;
                c.es += x.es; c.ep += x.ep; c.wmax = std::max (c.wmax, x.wmax); c.epmax = std::max (c.epmax, x.epmax);
            }
            {
                C& g = (uEdge[iu] > 0.9 && qEdge[iq] > 4.0) ? hard : rest;
                g.n += c.n; g.worse += c.worse; g.far += c.far; g.big += c.big; g.fixed += c.fixed;
                g.es += c.es; g.ep += c.ep; g.wmax = std::max (g.wmax, c.wmax); g.epmax = std::max (g.epmax, c.epmax);
            }
            if (c.n == 0 || cfg.quiet) continue;
            std::printf ("  u<%-4.1f Q<%-4.1f n %7ld | %7.3f -> %7.3f | %6.2f | worse %5ld (+%6.2f) | far %4ld | big %6ld cut4x %5.1f %%\n",
                         uEdge[iu] > 1e8 ? 9.9 : uEdge[iu], qEdge[iq] > 1e8 ? 99.0 : qEdge[iq], c.n, c.es / c.n, c.ep / c.n, c.epmax,
                         c.worse, c.wmax, c.far, c.big, 100.0 * c.fixed / std::max (1L, c.big));
        }
    for (const C* g : { &hard, &rest })
        std::printf ("  %6.0f %s n %7ld | mean %6.3f -> %6.3f dB | worst pol %6.2f | worse %5ld (+%6.2f) | far %4ld | seed>0.5: cut 4x %5.1f %%\n",
                     fs, g == &hard ? "HARD (u>=0.9, Q>=4)" : "rest               ", g->n, g->es / std::max (1L, g->n), g->ep / std::max (1L, g->n),
                     g->epmax, g->worse, g->wmax, g->far, 100.0 * g->fixed / std::max (1L, g->big));
}

//==============================================================================
static void printTrace (const BandSettings& s, double fs)
{
    PolishGrid grid;
    grid.prepare (fs);
    Trace tr[maxSections];
    BandDesign d {}, sd {};
    designV (fs, grid, s, sd, 0);
    designV (fs, grid, s, d, 1, tr);
    std::printf ("    settings %s fc %.10g gain %.10g Q %.10g si %d\n", typeName (s.type), s.freqHz, s.gainDbEff, s.q, s.slopeIndex);
    for (int i = 0; i < d.numSections; ++i)
    {
        const auto u = static_cast<std::size_t> (i);
        if (d.cls[u] != SectionClass::gain2) continue;
        const AnalogSection& a = d.trueProto[u];
        const Trace& t = tr[i];
        std::printf ("    sec %d  wz %.4f Qz %.3f wp %.4f Qp %.3f | rms0 %.4f dB  rms2 %.4f  wx %.4f  rms1 %.4f  S1/S0 %.5f  key %.3f  a %.5f  weight %.5f\n",
                     i, a.wz, a.Qz, a.wp, a.Qp, std::sqrt (t.S0 / 26), std::sqrt (t.S2 / 26), t.wx, std::sqrt (t.S1 / 26),
                     t.S0 > 0 ? t.S1 / t.S0 : 0.0, t.key, t.a, static_cast<double> (d.polishWeight[u]));
        if (! t.ran) continue;
        static const int fr[6] = { 0, 1, 3, 4, 5, 7 };
        std::printf ("        seed  ");
        for (const int f : fr) std::printf (" %9.4f", t.phi0[f]);
        std::printf ("   (ln g k mB mH G cH)\n");
        for (int k = 0; k < t.n; ++k)
        {
            std::printf ("        step%d ", k + 1);
            for (const int f : fr) std::printf (" %9.4f", t.phiK[k][f] - t.phi0[f]);
            std::printf ("   (delta from seed)\n");
        }
    }
}

static void printTrace (const BandSettings& s, double fs);
static void modeDiag (const Line& line)
{
    const LineResult r = runLine (line);
    std::printf ("line: %s\n  seed %.5f / %.5f  pol %.5f / %.5f dB, worst at x %.8f\n", describe (line).c_str(),
                 r.seedJump[0], r.seedJump[1], r.polJump[0], r.polJump[1], r.xAt);

    constexpr int numPoints = 160;
    double t2[numPoints];
    const double fHi = std::min (20000.0, 0.499 * line.fs);
    for (int p = 0; p < numPoints; ++p)
        t2[p] = tanSquaredHalf (2.0 * pi * 20.0 * std::pow (fHi / 20.0, p / (numPoints - 1.0)) / line.fs);
    PolishGrid grid;
    grid.prepare (line.fs);

    const auto diff = [&] (double xa, double xb)
    {
        BandDesign a {}, b {};
        designV (line.fs, grid, atX (line, xa), a, 1);
        designV (line.fs, grid, atX (line, xb), b, 1);
        double worst = 0;
        for (int p = 0; p < numPoints; ++p)
            worst = std::max (worst, std::fabs (10.0 * std::log10 (bandMag2 (a, t2[p]) / bandMag2 (b, t2[p]))));
        return worst;
    };

    double lo = r.xAt - 1.0 / 31999.0, hi = r.xAt;
    std::printf ("  neighbour steps, low (x %.9f):\n", lo);
    printTrace (atX (line, lo), line.fs);
    std::printf ("  neighbour steps, high (x %.9f):\n", hi);
    printTrace (atX (line, hi), line.fs);
    if (cfg.quiet) return;
    for (int it = 0; it < 60 && hi - lo > 1e-15; ++it)
    {
        const double mid = 0.5 * (lo + hi);
        if (diff (lo, mid) > diff (mid, hi)) hi = mid; else lo = mid;
        if (it % 6 == 5) std::printf ("  bisect %2d: width %.3e  jump across %.4f dB\n", it + 1, hi - lo, diff (lo, hi));
    }
    std::printf ("  low side (x %.15f):\n", lo);
    printTrace (atX (line, lo), line.fs);
    std::printf ("  high side (x %.15f):\n", hi);
    printTrace (atX (line, hi), line.fs);
}

//==============================================================================
static void modeSanity()
{
    // The bench's copy with default knobs must equal the shipped Designer bit for bit.
    Rng rng (42);
    long differ = 0, n = 0;
    for (const double fs : { 44100.0, 48000.0, 96000.0 })
    {
        Designer des;
        des.prepare (fs);
        PolishGrid grid;
        grid.prepare (fs);
        for (int i = 0; i < 200000; ++i)
        {
            const Type ty = static_cast<Type> (std::vector<int> { 0, 1, 2, 7 }[rng.next() % 4]);
            const BandSettings s = mk (ty, 10.0 * std::pow (3000.0, rng.uniform()), -60.0 + 120.0 * rng.uniform(),
                                       0.025 * std::pow (1600.0, rng.uniform()), static_cast<int> (rng.next() % 8));
            BandDesign a {}, b {};
            des.design (s, a);
            designV (fs, grid, s, b, 1);
            ++n;
            for (int k = 0; k < a.numSections; ++k)
            {
                const auto u = static_cast<std::size_t> (k);
                const SectionParams& p = a.params[u];
                const SectionParams& q = b.params[u];
                const double x[8] = { p.g, p.k, p.mL, p.mB, p.mH, p.G, p.cL, p.cH };
                const double y[8] = { q.g, q.k, q.mL, q.mB, q.mH, q.G, q.cL, q.cH };
                if (std::memcmp (x, y, sizeof x) != 0 || a.polishWeight[u] != b.polishWeight[u]) { ++differ; break; }
            }
        }
    }
    std::printf ("sanity: %ld designs, %ld differ from eq::Designer::design\n", n, differ);
}

//==============================================================================
int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("mode?\n"); return 2; }
    const std::string mode = argv[1];
    int lines = 60, verbose = 0, famMask = 0x7f, n = 400000;
    std::uint64_t seed = 0xC1C1C1C1ull;
    std::vector<double> rates { 44100.0, 48000.0 };
    Line dl { "diag", 27, Type::highShelf, 5, 48000.0, 29410.5446, 1942.12035, -28.0638385, 12.8669676, 15.3728095, 1.48341725 };

    for (int i = 2; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eqp = a.find ('=');
        if (eqp == std::string::npos) continue;
        const std::string k = a.substr (0, eqp);
        const double v = std::atof (a.c_str() + eqp + 1);
        if (k == "steps") cfg.steps = static_cast<int> (v);
        else if (k == "extra") cfg.extra = static_cast<int> (v);
        else if (k == "lambda") cfg.lambda = v;
        else if (k == "mu") cfg.mu = v;
        else if (k == "keep") cfg.keep = v;
        else if (k == "full") cfg.full = v;
        else if (k == "ext") cfg.ext = v;
        else if (k == "ramp") cfg.ramp = v;
        else if (k == "trust") cfg.trust = v;
        else if (k == "total") cfg.total = v;
        else if (k == "distLo") cfg.distLo = v;
        else if (k == "distHi") cfg.distHi = v;
        else if (k == "key") cfg.key = static_cast<int> (v);
        else if (k == "keyA") cfg.keyA = v;
        else if (k == "keyB") cfg.keyB = v;
        else if (k == "keyC") cfg.keyC = v;
        else if (k == "keyD") cfg.keyD = v;
        else if (k == "muRes") cfg.muRes = v;
        else if (k == "nu") cfg.nu = v;
        else if (k == "nuRes") cfg.nuRes = v;
        else if (k == "nuPow") cfg.nuPow = v;
        else if (k == "val") cfg.val = static_cast<int> (v);
        else if (k == "topPts") cfg.topPts = static_cast<int> (v);
        else if (k == "quiet") cfg.quiet = static_cast<int> (v);
        else if (k == "rc") cfg.rc = static_cast<int> (v);
        else if (k == "rcW") cfg.rcW = v;
        else if (k == "rcQa") cfg.rcQa = v;
        else if (k == "rcQb") cfg.rcQb = v;
        else if (k == "polish") cfg.polishOn = static_cast<int> (v);
        else if (k == "lines") lines = static_cast<int> (v);
        else if (k == "verbose") verbose = static_cast<int> (v);
        else if (k == "fam") famMask = static_cast<int> (std::strtol (a.c_str() + eqp + 1, nullptr, 0));
        else if (k == "seed") seed = std::strtoull (a.c_str() + eqp + 1, nullptr, 0);
        else if (k == "n") n = static_cast<int> (v);
        else if (k == "rates") { rates.clear(); const char* p = a.c_str() + eqp + 1; while (*p) { rates.push_back (std::strtod (p, const_cast<char**> (&p))); if (*p == ',') ++p; } }
        else if (k == "dtype") dl.type = static_cast<Type> (static_cast<int> (v));
        else if (k == "dsi") dl.slopeIndex = static_cast<int> (v);
        else if (k == "dfs") dl.fs = v;
        else if (k == "dfc0") dl.fc0 = v;
        else if (k == "dfc1") dl.fc1 = v;
        else if (k == "dg0") dl.gain0 = v;
        else if (k == "dg1") dl.gain1 = v;
        else if (k == "dq0") dl.q0 = v;
        else if (k == "dq1") dl.q1 = v;
        else { std::printf ("unknown key %s\n", k.c_str()); return 2; }
    }

    if (mode == "sanity") modeSanity();
    else if (mode == "diag") modeDiag (dl);
    else if (mode == "c1") modeC1 (lines, seed, rates, famMask, verbose);
    else if (mode == "acc") modeAcc();
    else if (mode == "rand") modeRand (n);
    else if (mode == "map") for (const double fs : rates) modeMap (n, fs);
    else { std::printf ("unknown mode\n"); return 2; }
    return 0;
}
