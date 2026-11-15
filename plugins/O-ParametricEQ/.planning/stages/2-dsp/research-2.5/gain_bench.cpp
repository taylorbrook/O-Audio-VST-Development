// O-ParametricEQ Phase 2.5 research bench: output-stage gain ramps and the
// curve-derived auto gain estimator.
//
// Scratch code. Reads the real Source/dsp headers (Designer, ResponseEval,
// BandParams, Fade) and changes nothing.
//
//   clang++ -O3 -std=c++17 -I ../../../../Source gain_bench.cpp -o <scratch>/gain_bench
//   <scratch>/gain_bench steps | retarget | sweep | stress | ag | ripple | cost | all
//   (output of `all`: bench-output.txt)
//
// Ramp shapes (per sample gain sequence, events on the 16-sample grid):
//   LIN   linear in amplitude, restart from the current value on a retarget
//   DBG   linear in dB on the control grid, per-sample linear interpolation of
//         the amplitude between ticks (the Phase 2.3 band-ramp pattern)
//   RC    raised cosine in amplitude (the Fade.h weight), restart on retarget
//   RCL   raised cosine, a retarget during a ramp is latched until its end
//   HER   cubic Hermite in amplitude, the current slope carried on a retarget
//   HERM  HER with the carried slope clamped to 3 delta / N (monotone)
//   HDB   cubic Hermite in dB, slope carried, 10^(dB/20) per sample
//   HARD  the target at once (negative control)
//   HERG  HERM, and a falling slope carried into a rise limited to 3 g0 / N
//   P2 / P3 / P4  a cascade of 2 / 3 / 4 one-poles in amplitude, time constant
//         set so a step is within 1e-3 of its size at N; snapped to the
//         target when every stage is within 1e-9

#include "dsp/Designer.h"
#include "dsp/ResponseEval.h"
#include "dsp/BandParams.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double fs = 48000.0;
constexpr int K = 16;

double dbToLin (double db) { return std::pow (10.0, db / 20.0); }
double linToDb (double g) { return 20.0 * std::log10 (g); }

enum Shape { LIN, DBG, RC, RCL, HER, HERM, HDB, HARD, HERG, P2, P3, P4, numShapes };
const char* shapeName[] = { "LIN", "DBG", "RC", "RCL", "HER", "HERM", "HDB", "HARD", "HERG", "P2", "P3", "P4" };

// Pole cascade: m one-poles, time constant chosen so a step is within 1e-3 of
// its size after N samples (gamma-tail: e^-x sum_{j<m} x^j/j! = 1e-3).
double cascadeX (int m) { return m == 2 ? 9.2335 : m == 3 ? 11.2289 : 13.0622; }

//==============================================================================
struct Ramp
{
    Shape shape;
    int N;              // samples
    double cur = 1.0, prev = 1.0;       // amplitude now and one sample ago
    double curDb = 0.0, prevDb = 0.0;
    // segment
    bool running = false;
    int i = 0;
    double g0 = 1.0, s0 = 0.0, g1 = 1.0;    // amplitude or dB (HDB)
    double pendingTarget = 0.0; bool pending = false;
    // DBG
    int ticks = 1, tickIdx = 0, sub = 0;
    double dbStep = 0.0, linA = 1.0, linB = 1.0, dbTarget = 0.0;
    // pole cascade
    double y[4] = { 1, 1, 1, 1 }; double polT = 1.0, alpha = 0.0; int poles = 0;

    Ramp (Shape s, int n, double startDb) : shape (s), N (n)
    {
        cur = prev = dbToLin (startDb);
        curDb = prevDb = startDb;
        ticks = std::max (1, (int) std::lround ((double) N / K));
        poles = s == P2 ? 2 : s == P3 ? 3 : s == P4 ? 4 : 0;
        if (poles) { alpha = 1.0 - std::exp (-cascadeX (poles) / N); for (auto& v : y) v = cur; polT = cur; }
    }

    void startSegment (double targetDb)
    {
        running = true; i = 0;
        switch (shape)
        {
            case LIN: case RC: case RCL:
                g0 = cur; g1 = dbToLin (targetDb); s0 = 0.0; break;
            case HER: case HERM: case HERG:
            {
                g0 = cur; g1 = dbToLin (targetDb); s0 = cur - prev;
                const double d = g1 - g0;
                if ((shape == HERM || shape == HERG) && d != 0.0 && s0 * d > 0.0 && std::abs (s0 * N) > 3.0 * std::abs (d))
                    s0 = 3.0 * d / N;
                // HERG: a falling slope carried into a rise is limited to 3 g0 / N,
                // so the dip stays above about g0 / 2 (never reaches zero)
                if (shape == HERG && s0 < 0.0 && d >= 0.0 && -s0 * N > 3.0 * g0)
                    s0 = -3.0 * g0 / N;
                if (shape == HERG && d == 0.0) s0 = std::clamp (s0, -3.0 * g0 / N, 3.0 * g0 / N);
                break;
            }
            case HDB:
                g0 = curDb; g1 = targetDb; s0 = curDb - prevDb; break;
            case DBG:
                dbTarget = targetDb; tickIdx = 0; sub = 0;
                dbStep = (targetDb - curDb) / ticks;
                linA = cur; linB = dbToLin (curDb + dbStep);
                break;
            case HARD:
                cur = prev = dbToLin (targetDb); curDb = prevDb = targetDb; running = false; break;
            default: break;
        }
    }

    // called on a tick sample, before next()
    void retarget (double targetDb)
    {
        if (poles) { polT = dbToLin (targetDb); running = true; return; }
        if (shape == RCL && running) { pendingTarget = targetDb; pending = true; return; }
        startSegment (targetDb);
    }

    double next()
    {
        prev = cur; prevDb = curDb;
        if (! running) return cur;

        if (poles)
        {
            y[0] += alpha * (polT - y[0]);
            for (int j = 1; j < poles; ++j) y[j] += alpha * (y[j - 1] - y[j]);
            bool near = true;
            for (int j = 0; j < poles; ++j) near = near && std::abs (y[j] - polT) <= 1e-9;
            if (near) { for (int j = 0; j < poles; ++j) y[j] = polT; running = false; }
            cur = y[poles - 1];
            return cur;
        }

        if (shape == DBG)
        {
            ++sub;
            cur = linA + (linB - linA) * (double) sub / K;
            if (sub == K)
            {
                sub = 0; ++tickIdx;
                double db = curDb + dbStep;
                if (tickIdx >= ticks) { db = dbTarget; }
                curDb = db; cur = dbToLin (db);
                if (tickIdx >= ticks) { running = false; return cur; }
                linA = cur; linB = dbToLin (curDb + dbStep);
                if (tickIdx + 1 >= ticks) linB = dbToLin (dbTarget);
            }
            return cur;
        }

        ++i;
        const double t = (double) i / N;
        double v;
        switch (shape)
        {
            case LIN: v = g0 + (g1 - g0) * t; break;
            case RC: case RCL:
            {
                const double w = 0.5 - 0.5 * std::cos (pi * (double) i / (N + 1));
                v = g0 + w * (g1 - g0);
                break;
            }
            default:
            {
                const double t2 = t * t, t3 = t2 * t;
                const double h00 = 2 * t3 - 3 * t2 + 1, h10 = t3 - 2 * t2 + t, h01 = -2 * t3 + 3 * t2;
                v = h00 * g0 + h10 * (N * s0) + h01 * g1;
            }
        }
        if (i >= N) { v = g1; running = false; }

        if (shape == HDB) { curDb = v; cur = dbToLin (v); }
        else { cur = v; }

        if (! running && shape == RCL && pending) { pending = false; startSegment (pendingTarget); }
        return cur;
    }
};

//==============================================================================
double maxD2 (const std::vector<double>& y, size_t a, size_t b)
{
    double m = 0.0;
    for (size_t n = std::max<size_t> (a, 1); n + 1 < std::min (b, y.size()); ++n)
        m = std::max (m, std::abs (y[n + 1] - 2.0 * y[n] + y[n - 1]));
    return m;
}

double localKappa (const std::vector<double>& y, size_t from, size_t to)
{
    const size_t frame = (size_t) std::lround (0.005 * fs);
    double worst = 0.0;
    const size_t end = std::min (to, y.size() - 1);
    for (size_t a = std::max<size_t> (from, 1); a + frame < end; a += frame)
    {
        double e = 0, d2 = 0;
        for (size_t i = a; i < a + frame; ++i)
        {
            e += y[i] * y[i];
            d2 = std::max (d2, std::abs (y[i + 1] - 2 * y[i] + y[i - 1]));
        }
        const double rms = std::sqrt (e / frame);
        if (rms < 1e-4) continue;
        worst = std::max (worst, d2 / rms);
    }
    return worst;
}

std::vector<double> tone (double f, double phase, size_t len)
{
    std::vector<double> x (len);
    for (size_t n = 0; n < len; ++n) x[n] = 0.5 * std::sin (2 * pi * f * n / fs + phase);
    return x;
}

double asFloat (double v) { return (double) (float) v; }

// Render a list of (sample, targetDb) events; events must sit on ticks.
struct Event { size_t at; double db; };

std::vector<double> render (Shape s, int N, double startDb, const std::vector<Event>& ev,
                            const std::vector<double>& x, std::vector<double>* gains = nullptr)
{
    Ramp r (s, N, startDb);
    std::vector<double> y (x.size());
    size_t e = 0;
    for (size_t n = 0; n < x.size(); ++n)
    {
        while (e < ev.size() && ev[e].at == n) { r.retarget (ev[e].db); ++e; }
        const double g = r.next();
        if (gains) (*gains)[n] = g;
        y[n] = asFloat (g * x[n]);
    }
    return y;
}

std::vector<double> held (double db, const std::vector<double>& x)
{
    std::vector<double> y (x.size());
    const double g = dbToLin (db);
    for (size_t n = 0; n < x.size(); ++n) y[n] = asFloat (g * x[n]);
    return y;
}

//==============================================================================
// steps: G3 step rows for a single gain change
void runSteps()
{
    std::printf ("\n== steps: largest second difference / louder held render (G3 (a), limit 2.0); kappa / held at 1 kHz (G3 (b), limit 2.5)\n");
    const double levels[] = { -36, -12, 0, 12, 36 };
    struct Grp { const char* name; int N; std::vector<std::pair<double, double>> pairs; };
    std::vector<Grp> groups;
    {
        Grp g { "output gain 20 ms, 20 pairs of -36/-12/0/+12/+36", (int) std::lround (0.020 * fs), {} };
        for (double a : levels) for (double b : levels) if (a != b) g.pairs.push_back ({ a, b });
        groups.push_back (g);
        Grp h { "auto gain 100 ms, 0 <-> -18 / -6 / +6 / +12", (int) std::lround (0.100 * fs), {} };
        for (double b : { -18.0, -6.0, 6.0, 12.0 }) { h.pairs.push_back ({ 0, b }); h.pairs.push_back ({ b, 0 }); }
        groups.push_back (h);
    }

    for (auto& grp : groups)
    {
        std::printf ("\n  %s\n  %-5s | %-28s | %-28s | %s\n", grp.name, "shape",
                     "50 Hz d2 worst (pair)", "1 kHz d2 worst (pair)", "1 kHz kappa worst / >2.5");
        for (int s = 0; s < numShapes; ++s)
        {
            if (s == RCL || s == HERM || s == HERG || s == HDB || s == DBG) continue;  // identical to RC / HER for a single step
            double w50 = 0, w1k = 0, wk = 0; int over = 0; std::string p50, p1k;
            for (auto& pr : grp.pairs)
                for (double f : { 50.0, 1000.0 })
                    for (int ph = 0; ph < 8; ++ph)
                    {
                        const size_t edit = 24000 + (size_t) (K * ph * 37);
                        const size_t len = edit + grp.N + 9600;
                        auto x = tone (f, 0.3 + ph * 0.7, len);
                        auto y = render ((Shape) s, grp.N, pr.first, { { edit, pr.second } }, x);
                        auto ha = held (pr.first, x), hb = held (pr.second, x);
                        const size_t a = edit - 2400, b = edit + grp.N + 2400;
                        const double hd = std::max (maxD2 (ha, a, b), maxD2 (hb, a, b));
                        const double r = maxD2 (y, a, b) / hd;
                        char buf[64]; std::snprintf (buf, sizeof buf, "%+.0f->%+.0f", pr.first, pr.second);
                        if (f == 50.0) { if (r > w50) { w50 = r; p50 = buf; } }
                        else
                        {
                            if (r > w1k) { w1k = r; p1k = buf; }
                            const double hk = std::max (localKappa (ha, a, b), localKappa (hb, a, b));
                            const double k = localKappa (y, a, b) / hk;
                            wk = std::max (wk, k);
                            if (ph == 0 && k > 2.5) ++over;
                        }
                    }
            std::printf ("  %-5s | %10.3f  %-16s | %10.3f  %-16s | %8.3f / %d\n", shapeName[s], w50, p50.c_str(), w1k, p1k.c_str(), wk, over);
        }
    }
}

//==============================================================================
// retarget: a second change inside the first ramp
void runRetarget()
{
    std::printf ("\n== retarget inside a 20 ms ramp: largest second difference / louder held render (all levels involved), worst over 8 phases\n");
    const int N = (int) std::lround (0.020 * fs);
    struct Case { double a, b, c; };
    const Case cases[] = { { 0, 36, -36 }, { 0, 36, 0 }, { 0, -36, 36 }, { 36, -36, 36 }, { 36, -36, 0 }, { -36, 36, 12 }, { 0, 12, 18 }, { 0, 12, 36 } };
    const double fracs[] = { 0.25, 0.5, 0.75, 0.9 };
    std::printf ("  %-22s", "case");
    for (int s : { RC, RCL, HERM, HERG, P2, P3, P4, HARD }) std::printf (" %8s", shapeName[s]);
    std::printf ("   (50 Hz; 1 kHz below)\n");
    for (double f : { 50.0, 1000.0 })
    {
        for (auto& c : cases)
            for (double fr : fracs)
            {
                std::printf ("  %+3.0f>%+3.0f>%+3.0f @%.2f %4.0fHz", c.a, c.b, c.c, fr, f);
                for (int s : { RC, RCL, HERM, HERG, P2, P3, P4, HARD })
                {
                    double worst = 0;
                    for (int ph = 0; ph < 8; ++ph)
                    {
                        const size_t e1 = 24000 + (size_t) (K * ph * 37);
                        const size_t e2 = e1 + (size_t) (K * std::lround (fr * N / K));
                        const size_t len = e1 + 3 * N + 9600;
                        auto x = tone (f, 0.3 + ph * 0.7, len);
                        auto y = render ((Shape) s, N, c.a, { { e1, c.b }, { e2, c.c } }, x);
                        const size_t a = e1 - 2400, b = e1 + 2 * N + 2400;
                        double hd = 0;
                        for (double l : { c.a, c.b, c.c }) hd = std::max (hd, maxD2 (held (l, x), a, b));
                        worst = std::max (worst, maxD2 (y, a, b) / hd);
                    }
                    std::printf (" %8.2f", worst);
                }
                std::printf ("\n");
            }
    }
}

//==============================================================================
// sweep: host automation of the output gain, delivered at block starts
void runSweep()
{
    std::printf ("\n== sweep: automation applied per sample (reference) vs ramp fed at block starts\n");
    std::printf ("   ratio = largest second difference / reference's; kappa ratio at 1 kHz; overshoot = max dB above the automation's range; lag = |dB| behind at sweep end + 20 ms\n");
    const int N = (int) std::lround (0.020 * fs);
    struct Sw { const char* name; double secs; };
    for (Sw sw : { Sw { "-30 -> +30 -> -30 dB, 20 ms each way (G3 gain sweep)", 0.020 }, Sw { "-30 -> +30 -> -30 dB, 50 ms", 0.050 }, Sw { "-30 -> +30 -> -30 dB, 1 s", 1.0 } })
    {
        std::printf ("\n  %s\n  %-5s %5s | %10s %10s | %8s | %9s | %8s\n", sw.name, "shape", "block", "d2 50 Hz", "d2 1 kHz", "k 1 kHz", "overshoot", "lag");
        const size_t s1 = 24000, len1 = (size_t) std::lround (sw.secs * fs);
        const size_t hold = 4800, s2 = s1 + len1 + hold, end = s2 + len1;
        auto autoDb = [&] (size_t n) -> double {
            if (n < s1) return -30;
            if (n < s1 + len1) return -30 + 60.0 * (double) (n - s1) / len1;
            if (n < s2) return 30;
            if (n < end) return 30 - 60.0 * (double) (n - s2) / len1;
            return -30;
        };
        const size_t len = end + 19200;
        for (int s : { LIN, RC, RCL, HERM, HERG, P2, P3, P4 })
            for (int block : { 32, 512 })
            {
                double r50 = 0, r1k = 0, rk = 0, over = 0, lag = 0;
                for (double f : { 50.0, 1000.0 })
                {
                    auto x = tone (f, 0.3, len);
                    std::vector<double> ref (len);
                    for (size_t n = 0; n < len; ++n) ref[n] = asFloat (dbToLin (autoDb (n)) * x[n]);
                    std::vector<Event> ev; double last = -30;
                    for (size_t b0 = 0; b0 < len; b0 += block)
                    {
                        const double t = autoDb (b0);
                        if (t != last) { ev.push_back ({ b0, t }); last = t; }
                    }
                    std::vector<double> g (len);
                    auto y = render ((Shape) s, N, -30, ev, x, &g);
                    const size_t a = s1 - 2400, b = end + 9600;
                    const double r = maxD2 (y, a, b) / maxD2 (ref, a, b);
                    if (f == 50.0) r50 = r; else { r1k = r; rk = localKappa (y, a, b) / localKappa (ref, a, b); }
                    for (size_t n = a; n < b; ++n)
                    {
                        const double db = linToDb (g[n]);
                        over = std::max (over, std::max (db - 30.0, -30.0 - db));
                    }
                    lag = std::max (lag, std::abs (linToDb (g[s1 + len1 + 960]) - 30.0));
                }
                std::printf ("  %-5s %5d | %10.2f %10.2f | %8.2f | %9.3f | %8.3f\n", shapeName[s], block, r50, r1k, rk, over, lag);
            }
    }
}

//==============================================================================
// Auto gain estimator
struct Grid
{
    std::vector<double> t2;
    void make (int n, double lo, double hi, bool midpoint, double rate)
    {
        t2.resize ((size_t) n);
        const double a = std::log (lo), b = std::log (hi);
        for (int i = 0; i < n; ++i)
        {
            const double u = midpoint ? (i + 0.5) / n : (double) i / (n - 1);
            const double f = std::exp (a + (b - a) * u);
            t2[(size_t) i] = eq::tanSquaredHalf (2 * pi * f / rate);
        }
    }
};

eq::BandDesign designBand (eq::Designer& d, int type, double f, double gainDb, double q, int slope)
{
    eq::RawBand raw { 1.0f, 0.0f, (float) type, (float) f, (float) gainDb, (float) q, (float) slope, 0.0f };
    const eq::BandInput in = eq::makeBandInput (raw, 100.0f, 0.0);
    eq::BandDesign bd;
    d.design (in.settings, bd);
    return bd;
}

// mean over the grid of the product of the bands' |H|^2
double meanPower (const std::vector<const eq::BandDesign*>& bands, const Grid& g)
{
    double sum = 0;
    for (double t2 : g.t2)
    {
        double p = 1.0;
        for (auto* b : bands)
            for (int i = 0; i < b->numSections; ++i)
                p *= eq::sectionMag2 (t2, b->params[(size_t) i]);
        sum += p;
    }
    return sum / (double) g.t2.size();
}

double agDb (const std::vector<const eq::BandDesign*>& bands, const Grid& g)
{
    return -10.0 * std::log10 (meanPower (bands, g));
}

const char* typeName[] = { "Bell", "LowShelf", "HighShelf", "LowCut", "HighCut", "Notch", "BandPass", "Tilt" };
const int slopeDb[] = { 6, 12, 18, 24, 36, 48, 72, 96 };

void runAg()
{
    std::printf ("\n== auto gain: estimate on the grid vs pink truth (dense 40 000-point midpoint mean over 40 Hz - 16 kHz of the same digital design)\n");
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        eq::Designer d; d.prepare (rate);
        Grid truth; truth.make (40000, 40, 16000, true, rate);
        Grid g64i, g64m, g128, g256, g512;
        g64i.make (64, 40, 16000, false, rate); g64m.make (64, 40, 16000, true, rate);
        g128.make (128, 40, 16000, true, rate); g256.make (256, 40, 16000, true, rate); g512.make (512, 40, 16000, true, rate);
        struct C { int type; double f, gain, q; int slope; };
        const C cases[] = {
            { 1, 200, 12, 1, 1 }, { 1, 200, 12, 1, 3 }, { 1, 200, 12, 1, 7 }, { 1, 200, -12, 3, 1 },
            { 2, 8000, 12, 1, 1 }, { 2, 8000, -18, 1, 5 }, { 2, 15000, 12, 4, 7 },
            { 0, 1000, 12, 0.5, 0 }, { 0, 1000, 12, 2, 0 }, { 0, 1000, 30, 10, 0 }, { 0, 3000, -30, 40, 0 },
            { 3, 100, 0, 1, 3 }, { 3, 300, 0, 1, 7 }, { 4, 5000, 0, 1, 5 }, { 5, 1000, 0, 4, 0 }, { 6, 1000, 0, 10, 0 },
            { 7, 1000, 12, 1, 1 }, { 7, 1000, -12, 1, 5 },
        };
        std::printf ("\n  %.1f kHz  %-34s | %8s | %8s %8s %8s %8s %8s  (error dB, est - truth)\n", rate / 1000, "band", "truth", "64 incl", "64 mid", "128", "256", "512");
        for (auto& c : cases)
        {
            auto bd = designBand (d, c.type, c.f, c.gain, c.q, c.slope);
            std::vector<const eq::BandDesign*> v { &bd };
            const double t = agDb (v, truth);
            char nm[64]; std::snprintf (nm, sizeof nm, "%s %.0f Hz %+.0f dB Q %.2g %d dB/oct", typeName[c.type], c.f, c.gain, c.q, slopeDb[c.slope]);
            std::printf ("  %-43s | %+8.3f | %+8.3f %+8.3f %+8.3f %+8.3f %+8.3f\n", nm, t,
                         agDb (v, g64i) - t, agDb (v, g64m) - t, agDb (v, g128) - t, agDb (v, g256) - t, agDb (v, g512) - t);
        }
    }
}

// ripple: a narrow bell swept in frequency; how much the estimate wobbles
void runRipple()
{
    std::printf ("\n== ripple: bell swept 200 Hz - 8 kHz in 4000 log steps, 48 kHz; peak-to-peak and worst |est - truth| (dB)\n");
    eq::Designer d; d.prepare (48000.0);
    Grid truth; truth.make (20000, 40, 16000, true, 48000.0);
    const int ns[] = { 64, 128, 256, 512, 1024 };
    Grid gs[5];
    for (int i = 0; i < 5; ++i) gs[i].make (ns[i], 40, 16000, true, 48000.0);
    std::printf ("  %-20s", "bell");
    for (int n : ns) std::printf (" | %4d: p-p   worst", n);
    std::printf ("\n");
    for (double q : { 40.0, 20.0, 10.0, 4.0, 1.0 })
        for (double gain : { 30.0, 12.0, 6.0, -30.0 })
        {
            std::printf ("  Q %4.0f %+5.0f dB      ", q, gain);
            for (int gi = 0; gi < 5; ++gi)
            {
                double lo = 1e9, hi = -1e9, worst = 0;
                for (int s = 0; s < 4000; ++s)
                {
                    const double f = 200.0 * std::pow (40.0, s / 3999.0);
                    auto bd = designBand (d, 0, f, gain, q, 0);
                    std::vector<const eq::BandDesign*> v { &bd };
                    const double e = agDb (v, gs[gi]) - agDb (v, truth);
                    lo = std::min (lo, e); hi = std::max (hi, e); worst = std::max (worst, std::abs (e));
                }
                std::printf (" | %8.3f %7.3f", hi - lo, worst);
            }
            std::printf ("\n");
        }
}

// cost on the audio thread
void runCost()
{
    std::printf ("\n== cost (this machine, -O3, scalar): per changed band = design + |H|^2 product over the grid; per refresh = 2 lanes x 24 bands product + mean + log\n");
    eq::Designer d; d.prepare (48000.0);
    for (int n : { 64, 128, 256, 512 })
    {
        Grid g; g.make (n, 40, 16000, true, 48000.0);
        std::vector<double> vec ((size_t) n);
        struct C { const char* name; int type; int slope; };
        for (C c : { C { "bell", 0, 0 }, C { "shelf 96 dB/oct (8 sections)", 1, 7 }, C { "low cut 96 dB/oct", 3, 7 } })
        {
            const int iters = 20000;
            volatile double sink = 0;
            auto t0 = std::chrono::steady_clock::now();
            for (int it = 0; it < iters; ++it)
            {
                auto bd = designBand (d, c.type, 300.0 + (it % 97), 6.0 + (it % 7), 1.0, c.slope);
                for (int i = 0; i < n; ++i)
                {
                    double p = 1.0;
                    for (int s = 0; s < bd.numSections; ++s) p *= eq::sectionMag2 (g.t2[(size_t) i], bd.params[(size_t) s]);
                    vec[(size_t) i] = p;
                }
                sink = sink + vec[(size_t) (it % n)];
            }
            const double us = std::chrono::duration<double, std::micro> (std::chrono::steady_clock::now() - t0).count() / iters;
            std::printf ("  grid %4d  %-30s %8.2f us per band\n", n, c.name, us);
        }
        // refresh: product of 24 cached vectors per lane, mean, log
        std::vector<std::vector<double>> cache (24, std::vector<double> ((size_t) n, 1.01));
        const int iters = 200000;
        volatile double sink = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int it = 0; it < iters; ++it)
        {
            double total = 0;
            for (int lane = 0; lane < 2; ++lane)
            {
                double sum = 0;
                for (int i = 0; i < n; ++i)
                {
                    double p = 1.0;
                    for (int b = 0; b < 24; ++b) p *= cache[(size_t) b][(size_t) i];
                    sum += p;
                }
                total += sum;
            }
            sink = sink + std::log10 (total * (1 + it * 1e-12));
        }
        const double us = std::chrono::duration<double, std::micro> (std::chrono::steady_clock::now() - t0).count() / iters;
        std::printf ("  grid %4d  refresh (2 lanes x 24 bands)        %8.2f us\n", n, us);
    }
}
// stress: random targets at random block starts; extremes of the gain
void runStress()
{
    std::printf ("\n== stress: 200 s of random output-gain automation (-36 ... +36 dB) at random block sizes 1 ... 2048, retarget at block starts\n");
    std::printf ("   min gain (dB), max gain (dB), samples below -40 dB or above +40 dB, samples below 0 amplitude\n");
    const int N = (int) std::lround (0.020 * fs);
    for (int s : { LIN, RC, RCL, HER, HERM, HERG, HDB, P2, P3, P4 })
    {
        unsigned long long rng = 12345;
        auto rnd = [&] { rng = rng * 6364136223846793005ULL + 1442695040888963407ULL; return (double) (rng >> 11) / 9007199254740992.0; };
        Ramp r ((Shape) s, N, 0.0);
        double lo = 1e9, hi = -1e9; long outside = 0, neg = 0;
        size_t n = 0; const size_t total = (size_t) (200 * fs);
        while (n < total)
        {
            int block = 1 + (int) (rnd() * 2048);
            // mostly small moves with occasional extremes, as automation does
            const double u = rnd();
            const double t = u < 0.5 ? -36.0 + 72.0 * rnd() : (u < 0.75 ? (rnd() < 0.5 ? -36.0 : 36.0) : std::clamp (linToDb (std::max (r.cur, 1e-9)) + (rnd() - 0.5) * 6.0, -36.0, 36.0));
            // retarget on the next tick: align the block start to K
            const size_t at = ((n + K - 1) / K) * K;
            while (n < at) { r.next(); ++n; }
            r.retarget (t);
            for (int i = 0; i < block && n < total; ++i, ++n)
            {
                const double g = r.next();
                if (g <= 0) { ++neg; continue; }
                const double db = linToDb (g);
                lo = std::min (lo, db); hi = std::max (hi, db);
                if (db < -40 || db > 40) ++outside;
            }
        }
        std::printf ("  %-5s min %8.2f  max %7.2f  outside %8ld  nonpositive %8ld\n", shapeName[s], lo, hi, outside, neg);
    }
}
} // namespace

int main (int argc, char** argv)
{
    const std::string what = argc > 1 ? argv[1] : "all";
    if (what == "steps" || what == "all") runSteps();
    if (what == "retarget" || what == "all") runRetarget();
    if (what == "sweep" || what == "all") runSweep();
    if (what == "ag" || what == "all") runAg();
    if (what == "ripple" || what == "all") runRipple();
    if (what == "cost" || what == "all") runCost();
    if (what == "stress" || what == "all") runStress();
    return 0;
}
