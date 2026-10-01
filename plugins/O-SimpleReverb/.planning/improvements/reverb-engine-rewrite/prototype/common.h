// Research prototype for the O-SimpleReverb engine rewrite (milestone
// reverb-engine-rewrite). NOT production code: no JUCE, no RT discipline,
// double precision. It exists to put numbers behind RESEARCH.md.
//
// Build:  clang++ -std=c++17 -O2 -o proto proto.cpp && ./proto
#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using Vec = std::vector<double>;
static const double kPi = 3.14159265358979323846;

// ── delay line: read-before-write, delay >= 1 ────────────────────────────────
struct Delay {
    std::vector<double> b; int w = 0, mask = 0;
    void prepare(int maxDelay) { int n = 1; while (n < maxDelay + 8) n <<= 1; b.assign((size_t) n, 0.0); mask = n - 1; w = 0; }
    void push(double x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    double at(int d) const { return b[(size_t) ((w - d) & mask)]; }          // d >= 1: last written is d = 1
    double readLinear(double d) const { const int i = (int) d; const double f = d - i; return at(i) + f * (at(i + 1) - at(i)); }
    // 3rd-order Lagrange, fractional part in the CENTRAL interval (|H| <= 1).
    double readCubic(double d) const {
        const int i = (int) d; const double f = d - i;                        // read between at(i) and at(i+1)
        const double ym = at(i - 1), y0 = at(i), y1 = at(i + 1), y2 = at(i + 2);
        const double c0 = -f * (f - 1) * (f - 2) / 6.0, c1 = (f + 1) * (f - 1) * (f - 2) / 2.0,
                     c2 = -(f + 1) * f * (f - 2) / 2.0, c3 = (f + 1) * f * (f - 1) / 6.0;
        return c0 * ym + c1 * y0 + c2 * y1 + c3 * y2;
    }
};

// Schroeder allpass  H = (-g + z^-D) / (1 - g z^-D)
struct Allpass {
    Delay d; int len = 1; double g = 0.5;
    void prepare(int length, double coeff) { len = std::max(1, length); g = coeff; d.prepare(len + 64); }
    double process(double x) { const double z = d.at(len); const double v = x + g * z; d.push(v); return z - g * v; }
    double processMod(double x, double lenFrac) { const double z = d.readCubic(lenFrac); const double v = x + g * z; d.push(v); return z - g * v; }
};

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double operator()(double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    static Biquad lp(double fs, double f, double q) { Biquad b; const double w = 2 * kPi * f / fs, al = std::sin(w) / (2 * q), c = std::cos(w), a0 = 1 + al;
        b.b0 = (1 - c) / 2 / a0; b.b1 = (1 - c) / a0; b.b2 = b.b0; b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0; return b; }
    static Biquad hp(double fs, double f, double q) { Biquad b; const double w = 2 * kPi * f / fs, al = std::sin(w) / (2 * q), c = std::cos(w), a0 = 1 + al;
        b.b0 = (1 + c) / 2 / a0; b.b1 = -(1 + c) / a0; b.b2 = b.b0; b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0; return b; }
};

// 4th-order Butterworth band (HP f1 then LP f2)
static Vec band(const Vec& x, double fs, double f1, double f2)
{
    Biquad h1 = Biquad::hp(fs, f1, 0.541), h2 = Biquad::hp(fs, f1, 1.307), l1 = Biquad::lp(fs, f2, 0.541), l2 = Biquad::lp(fs, f2, 1.307);
    Vec y(x.size());
    for (size_t i = 0; i < x.size(); ++i) y[i] = l2(l1(h2(h1(x[i]))));
    return y;
}

// RT60 from the Schroeder backward integral: least-squares line over
// [-hiDb, -loDb] of the decay curve, extrapolated to 60 dB. T30 = (5, 35).
static double rt60(const Vec& ir, double fs, double hiDb = 5.0, double loDb = 35.0)
{
    const size_t n = ir.size();
    Vec edc(n); double acc = 0.0;
    for (size_t i = n; i-- > 0;) { acc += ir[i] * ir[i]; edc[i] = acc; }
    if (acc <= 0.0) return 0.0;
    double sx = 0, sy = 0, sxx = 0, sxy = 0; long cnt = 0;
    for (size_t i = 0; i < n; ++i) {
        const double db = 10.0 * std::log10(edc[i] / acc + 1e-300);
        if (db > -hiDb) continue;
        if (db < -loDb) break;
        const double t = (double) i / fs; sx += t; sy += db; sxx += t * t; sxy += t * db; ++cnt;
    }
    if (cnt < 8) return 0.0;
    const double slope = (cnt * sxy - sx * sy) / (cnt * sxx - sx * sx);   // dB / s
    return -60.0 / slope;
}
static double midRt60(const Vec& ir, double fs) { return rt60(band(ir, fs, 354.0, 1414.0), fs); }     // 500 + 1k octaves
static double hfRt60(const Vec& ir, double fs)  { return rt60(band(ir, fs, 5657.0, 11314.0), fs); }   // 8k octave
static double lfRt60(const Vec& ir, double fs)  { return rt60(band(ir, fs, 88.0, 177.0), fs); }       // 125 octave

static double correlation(const Vec& a, const Vec& b, size_t from, size_t to)
{
    double ab = 0, aa = 0, bb = 0;
    for (size_t i = from; i < to && i < a.size(); ++i) { ab += a[i] * b[i]; aa += a[i] * a[i]; bb += b[i] * b[i]; }
    return ab / std::sqrt(aa * bb + 1e-300);
}

// Abel & Huang normalised echo density: time (ms) at which the 20 ms window
// first reads >= 0.9 (1.0 = Gaussian). The "mixing time".
static double mixingTimeMs(const Vec& ir, double fs)
{
    const int W = (int) (0.02 * fs);
    for (size_t s = 0; s + (size_t) W < ir.size(); s += (size_t) (W / 8)) {
        double e = 0; for (int i = 0; i < W; ++i) e += ir[s + (size_t) i] * ir[s + (size_t) i];
        const double sd = std::sqrt(e / W); if (sd < 1e-12) continue;
        int out = 0; for (int i = 0; i < W; ++i) if (std::abs(ir[s + (size_t) i]) > sd) ++out;
        if ((double) out / W / 0.3173 >= 0.9) return 1000.0 * (double) s / fs;
    }
    return -1.0;
}

// normalised echo density (0..1) of the 20 ms window starting at t seconds
static double nedAt(const Vec& ir, double fs, double t)
{
    const int W = (int) (0.02 * fs); const size_t s = (size_t) (t * fs); if (s + (size_t) W >= ir.size()) return -1;
    double e = 0; for (int i = 0; i < W; ++i) e += ir[s + (size_t) i] * ir[s + (size_t) i];
    const double sd = std::sqrt(e / W); int out = 0; for (int i = 0; i < W; ++i) if (std::abs(ir[s + (size_t) i]) > sd) ++out;
    return (double) out / W / 0.3173;
}

// the plugin's two-grain octave-up shifter (Source/ModulationFx.h), as is
struct OctaveUp {
    std::vector<double> buf; int mask = 0, w = 0, grain = 1; double phase = 0;
    void prepare(double fs) { grain = std::max(16, (int) std::lround(0.040 * fs)); int n = 1; while (n < grain + 4) n <<= 1; buf.assign((size_t) n, 0.0); mask = n - 1; w = 0; phase = 0; }
    double read(double delay) const { const double pos = (double) w - delay, fl = std::floor(pos), fr = pos - fl; const int i0 = (int) fl & mask, i1 = (i0 + 1) & mask; return buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]); }
    double process(double x) {
        buf[(size_t) w] = x; double out = 0;
        for (int tap = 0; tap < 2; ++tap) { double p = phase + 0.5 * tap; if (p >= 1) p -= 1; const double s = std::sin(kPi * p); out += s * s * read(grain * (1.0 - p)); }
        phase += 1.0 / grain; if (phase >= 1) phase -= 1; w = (w + 1) & mask; return out;
    }
};

struct Timer { std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    double seconds() const { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); } };
