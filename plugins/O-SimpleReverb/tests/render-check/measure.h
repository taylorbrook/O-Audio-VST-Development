/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
/*
    Reverb measurements for render-check (reverb-engine-rewrite milestone).

    Everything below the "through the processor" line works on plain vectors,
    so it measures an engine header driven directly as well as the plugin. The
    two templates at the end go through prepareToPlay/processBlock only and
    know nothing about what is inside the processor: the same code produced the
    v1.14.0 baseline and gates v2.0.0.

    Conventions:
      - impulse responses are WET 100 / DRY 0, the same unit impulse in L and R
        ("mono input"), taken after a silent settle so smoothers have landed;
      - RT60 is ISO 3382 T30: Schroeder backward integral, least-squares line
        over -5..-35 dB, extrapolated to 60 dB. "mid" is the 500 Hz + 1 kHz
        octaves (354-1414 Hz). An IR must be at least 1.35 x T60 + 0.4 s long;
      - times "re onset" count from the first sample within 40 dB of the IR's
        peak, so a type's pre-delay does not move them.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>
#include <vector>

namespace measure
{

using Vec = std::vector<double>;
struct Stereo { Vec l, r; };

constexpr double kPi = 3.14159265358979323846;

// RBJ biquad, transposed direct form II.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    double operator()(double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    static Biquad lowPass(double fs, double f, double q)
    {
        Biquad b;
        const double w = 2.0 * kPi * f / fs, al = std::sin(w) / (2.0 * q), c = std::cos(w), a0 = 1.0 + al;
        b.b0 = (1.0 - c) / 2.0 / a0; b.b1 = (1.0 - c) / a0; b.b2 = b.b0;
        b.a1 = -2.0 * c / a0; b.a2 = (1.0 - al) / a0;
        return b;
    }

    static Biquad highPass(double fs, double f, double q)
    {
        Biquad b;
        const double w = 2.0 * kPi * f / fs, al = std::sin(w) / (2.0 * q), c = std::cos(w), a0 = 1.0 + al;
        b.b0 = (1.0 + c) / 2.0 / a0; b.b1 = -(1.0 + c) / a0; b.b2 = b.b0;
        b.a1 = -2.0 * c / a0; b.a2 = (1.0 - al) / a0;
        return b;
    }
};

// 4th-order Butterworth high-pass at f1 into 4th-order Butterworth low-pass at f2.
inline Vec band(const Vec& x, double fs, double f1, double f2)
{
    Biquad h1 = Biquad::highPass(fs, f1, 0.541), h2 = Biquad::highPass(fs, f1, 1.307),
           l1 = Biquad::lowPass(fs, f2, 0.541),  l2 = Biquad::lowPass(fs, f2, 1.307);
    Vec y(x.size());
    for (size_t i = 0; i < x.size(); ++i)
        y[i] = l2(l1(h2(h1(x[i]))));
    return y;
}

// Schroeder energy decay curve in dB re the total (edc[0] = 0 dB).
inline Vec decayCurveDb(const Vec& ir)
{
    Vec edc(ir.size());
    double acc = 0.0;
    for (size_t i = ir.size(); i-- > 0;) { acc += ir[i] * ir[i]; edc[i] = acc; }
    for (auto& e : edc)
        e = acc > 0.0 ? 10.0 * std::log10(e / acc + 1.0e-300) : -3000.0;
    return edc;
}

// RT60 (s) from the line fitted to the decay curve between -hiDb and -loDb.
// 0 when the curve has fewer than 8 points in that range.
inline double rt60(const Vec& ir, double fs, double hiDb = 5.0, double loDb = 35.0)
{
    const Vec edc = decayCurveDb(ir);
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    long cnt = 0;
    for (size_t i = 0; i < edc.size(); ++i) {
        if (edc[i] > -hiDb) continue;
        if (edc[i] < -loDb) break;
        const double t = (double) i / fs;
        sx += t; sy += edc[i]; sxx += t * t; sxy += t * edc[i]; ++cnt;
    }
    if (cnt < 8) return 0.0;
    const double slope = ((double) cnt * sxy - sx * sy) / ((double) cnt * sxx - sx * sx);   // dB / s
    return slope < 0.0 ? -60.0 / slope : 0.0;
}

inline double midRt60(const Vec& ir, double fs) { return rt60(band(ir, fs, 354.0, 1414.0), fs); }     // 500 Hz + 1 kHz octaves
inline double hfRt60(const Vec& ir, double fs)  { return rt60(band(ir, fs, 5657.0, 11314.0), fs); }   // 8 kHz octave
inline double lfRt60(const Vec& ir, double fs)  { return rt60(band(ir, fs, 88.0, 177.0), fs); }       // 125 Hz octave

// Time (s) at which the decay curve first falls below -db; -1 if it never does.
inline double decayTimeTo(const Vec& ir, double fs, double db)
{
    const Vec edc = decayCurveDb(ir);
    for (size_t i = 0; i < edc.size(); ++i)
        if (edc[i] < -db) return (double) i / fs;
    return -1.0;
}

// Normalised zero-lag correlation of a and b over [from, to).
inline double correlation(const Vec& a, const Vec& b, size_t from, size_t to)
{
    double ab = 0, aa = 0, bb = 0;
    for (size_t i = from; i < to && i < a.size() && i < b.size(); ++i) {
        ab += a[i] * b[i]; aa += a[i] * a[i]; bb += b[i] * b[i];
    }
    return ab / std::sqrt(aa * bb + 1.0e-300);
}

// Goertzel power of x at f over [from, to).
inline double goertzel(const Vec& x, double f, double fs, size_t from, size_t to)
{
    const double w = 2.0 * kPi * f / fs, c = 2.0 * std::cos(w);
    double s1 = 0, s2 = 0;
    for (size_t i = from; i < to && i < x.size(); ++i) {
        const double s0 = x[i] + c * s1 - s2;
        s2 = s1; s1 = s0;
    }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}

// The fine structure of a tail's spectrum: dB at every stepHz from f0 to f1 of
// the Hann-windowed segment [t0, t1) seconds, minus a moving average
// detrendHz wide. The moving average takes out what an EQ or a damping filter
// does (a tilt); what is left is where the resonances are.
inline Vec tailSpectrumDb(const Vec& ir, double fs, double t0, double t1,
                          double f0 = 200.0, double f1 = 2000.0, double stepHz = 0.5, double detrendHz = 100.0)
{
    const size_t from = std::min(ir.size(), (size_t) (t0 * fs)), to = std::min(ir.size(), (size_t) (t1 * fs));
    Vec seg;
    if (to > from + 1) {
        seg.resize(to - from);
        for (size_t i = 0; i < seg.size(); ++i)
            seg[i] = ir[from + i] * (0.5 - 0.5 * std::cos(2.0 * kPi * (double) i / (double) (seg.size() - 1)));
    }
    Vec db;
    for (double f = f0; f < f1; f += stepHz)
        db.push_back(10.0 * std::log10(goertzel(seg, f, fs, 0, seg.size()) + 1.0e-30));

    const long half = std::max(1L, (long) (0.5 * detrendHz / stepHz)), n = (long) db.size();
    Vec out(db.size());
    for (long i = 0; i < n; ++i) {
        double sum = 0; long cnt = 0;
        for (long j = std::max(0L, i - half); j <= std::min(n - 1, i + half); ++j) { sum += db[(size_t) j]; ++cnt; }
        out[(size_t) i] = db[(size_t) i] - sum / (double) cnt;
    }
    return out;
}

// Pearson correlation of two equally long series.
inline double pearson(const Vec& a, const Vec& b)
{
    const size_t n = std::min(a.size(), b.size());
    if (n < 2) return 0.0;
    double ma = 0, mb = 0;
    for (size_t i = 0; i < n; ++i) { ma += a[i]; mb += b[i]; }
    ma /= (double) n; mb /= (double) n;
    double ab = 0, aa = 0, bb = 0;
    for (size_t i = 0; i < n; ++i) {
        ab += (a[i] - ma) * (b[i] - mb); aa += (a[i] - ma) * (a[i] - ma); bb += (b[i] - mb) * (b[i] - mb);
    }
    return ab / std::sqrt(aa * bb + 1.0e-300);
}

// Index of the first sample within relDb of the IR's peak (the first arrival).
inline size_t onsetIndex(const Vec& ir, double relDb = -40.0)
{
    double peak = 0;
    for (double v : ir) peak = std::max(peak, std::abs(v));
    const double thr = peak * std::pow(10.0, relDb / 20.0);
    for (size_t i = 0; i < ir.size(); ++i)
        if (std::abs(ir[i]) >= thr && peak > 0.0) return i;
    return 0;
}

// Discrete arrivals in the windowMs after `from`: local maxima of |ir| (no
// larger sample within sepMs either side) at or above relDb re the largest
// sample in that window. Times are ms re `from`; amp keeps its sign.
struct Tap { double ms; double amp; };
inline std::vector<Tap> earlyTaps(const Vec& ir, double fs, size_t from,
                                  double windowMs = 60.0, double relDb = -20.0, double sepMs = 0.25)
{
    const size_t to = std::min(ir.size(), from + (size_t) (windowMs * 0.001 * fs));
    const long sep = std::max(1L, (long) (sepMs * 0.001 * fs));
    double peak = 0;
    for (size_t i = from; i < to; ++i) peak = std::max(peak, std::abs(ir[i]));
    const double thr = peak * std::pow(10.0, relDb / 20.0);
    std::vector<Tap> taps;
    if (peak <= 0.0) return taps;
    for (long i = (long) from; i < (long) to; ++i) {
        const double v = std::abs(ir[(size_t) i]);
        if (v < thr) continue;
        bool isMax = true;
        for (long j = std::max(0L, i - sep); j <= std::min((long) ir.size() - 1, i + sep) && isMax; ++j) {
            const double other = std::abs(ir[(size_t) j]);
            if ((j < i && other >= v) || (j > i && other > v)) isMax = false;   // of equal neighbours, the first
        }
        if (isMax) taps.push_back({ 1000.0 * (double) ((size_t) i - from) / fs, ir[(size_t) i] });
    }
    return taps;
}

// How many taps of `a` have no tap of `b` within tolMs.
inline int tapsWithoutPartner(const std::vector<Tap>& a, const std::vector<Tap>& b, double tolMs = 0.1)
{
    int alone = 0;
    for (const auto& ta : a) {
        bool found = false;
        for (const auto& tb : b) found = found || std::abs(ta.ms - tb.ms) <= tolMs;
        if (! found) ++alone;
    }
    return alone;
}

// Abel & Huang normalised echo density of the 20 ms window starting at
// sample `start`: the fraction of samples outside one standard deviation,
// over the Gaussian expectation (1.0 = Gaussian, i.e. fully mixed). -1 if the
// window runs past the IR or is silent.
inline double echoDensityAt(const Vec& ir, double fs, size_t start)
{
    const size_t W = (size_t) (0.02 * fs);
    if (start + W >= ir.size()) return -1.0;
    double e = 0;
    for (size_t i = 0; i < W; ++i) e += ir[start + i] * ir[start + i];
    const double sd = std::sqrt(e / (double) W);
    if (sd < 1.0e-12) return -1.0;
    size_t out = 0;
    for (size_t i = 0; i < W; ++i) if (std::abs(ir[start + i]) > sd) ++out;
    return (double) out / (double) W / 0.3173;
}

// Mixing time: ms after `from` at which the echo density first reads >= 0.9.
inline double mixingTimeMs(const Vec& ir, double fs, size_t from)
{
    const size_t hop = std::max((size_t) 1, (size_t) (0.0025 * fs));
    for (size_t s = from; s < ir.size(); s += hop) {
        const double d = echoDensityAt(ir, fs, s);
        if (d >= 0.9) return 1000.0 * (double) (s - from) / fs;
    }
    return -1.0;
}

// When a narrow band (f / 1.06 .. f x 1.06) first arrives: the first sample of
// the band within relDb of the band's largest in the windowMs after `from`, in
// ms re `from`, less the same reading of the band filter's own impulse
// response (so a low band's slower filter does not read as a later arrival).
// The band filter runs twice (48 dB/octave skirts): once leaves a wide-band
// burst an octave away only 9 dB down in the band, which reads as an arrival.
// The FIRST arrival, not the largest sample: in a tail that is still building
// up the largest sample of a band falls anywhere (v1.14.0, whose types are not
// dispersive, read -89..+12 ms between 1 and 3 kHz on that metric).
inline double bandArrivalMs(const Vec& ir, double fs, size_t from, double f, double windowMs = 160.0, double relDb = -20.0)
{
    auto firstArrival = [&](const Vec& x) {
        const Vec nb = band(band(x, fs, f / 1.06, f * 1.06), fs, f / 1.06, f * 1.06);
        double m = 0;
        for (double v : nb) m = std::max(m, std::abs(v));
        const double thr = m * std::pow(10.0, relDb / 20.0);
        for (size_t i = 0; i < nb.size(); ++i) if (std::abs(nb[i]) >= thr && m > 0.0) return (double) i;
        return 0.0;
    };
    const size_t n = (size_t) (windowMs * 0.001 * fs);
    Vec seg(n, 0.0), unit(n, 0.0);
    for (size_t i = 0; i < n && from + i < ir.size(); ++i) seg[i] = ir[from + i];
    unit[0] = 1.0;
    return 1000.0 * (firstArrival(seg) - firstArrival(unit)) / fs;
}

// Where a narrow band's energy sits in the windowMs after `from`: its centre
// of gravity in ms re `from`, less the same reading of the band filter's own
// impulse response. For ONE echo in the window this is the echo's group delay
// at f. The first arrival is not: a dispersed echo's leading edge in a band is
// whatever the filter's skirts pass of the faster frequencies below it (a
// spring whose 3 kHz is 9.8 ms behind its 1 kHz read 1.2 ms on bandArrivalMs).
// Over a window that holds a tail and not an echo it says nothing.
inline double bandCentroidMs(const Vec& ir, double fs, size_t from, double f, double windowMs)
{
    auto centre = [&](const Vec& x) {
        const Vec nb = band(band(x, fs, f / 1.06, f * 1.06), fs, f / 1.06, f * 1.06);
        double e = 0, te = 0;
        for (size_t i = 0; i < nb.size(); ++i) { e += nb[i] * nb[i]; te += (double) i * nb[i] * nb[i]; }
        return e > 0.0 ? te / e : 0.0;
    };
    const size_t n = (size_t) (windowMs * 0.001 * fs);
    Vec seg(n, 0.0), unit(n, 0.0);
    for (size_t i = 0; i < n && from + i < ir.size(); ++i) seg[i] = ir[from + i];
    unit[0] = 1.0;
    return 1000.0 * (centre(seg) - centre(unit)) / fs;
}

// Whether the mid band (354-1414 Hz) comes in echoes, and how far apart: the
// lag between minMs and maxMs at which the envelope of the spanMs after `from`
// best matches itself, and how well (1 = the same shape again, about 0 = no
// echo). The envelope is the rectified band over 1 ms, less its own 70 ms
// average, so a tail that merely decays has no lag it prefers.
struct Period { double ms; double strength; };
inline Period envelopePeriod(const Vec& ir, double fs, size_t from, double minMs = 15.0, double maxMs = 70.0, double spanMs = 250.0)
{
    const Vec mid = band(ir, fs, 354.0, 1414.0);
    const long n = (long) (spanMs * 0.001 * fs), w = std::max(1L, (long) (0.001 * fs)), slow = (long) (0.035 * fs);
    Vec raw((size_t) n, 0.0), env((size_t) n);
    for (long i = 0; i < n; ++i) {
        double a = 0;
        for (long j = 0; j < w && from + (size_t) (i + j) < mid.size(); ++j) a += std::abs(mid[from + (size_t) (i + j)]);
        raw[(size_t) i] = a / (double) w;
    }
    for (long i = 0; i < n; ++i) {
        double a = 0; long cnt = 0;
        for (long j = std::max(0L, i - slow); j <= std::min(n - 1, i + slow); ++j) { a += raw[(size_t) j]; ++cnt; }
        env[(size_t) i] = raw[(size_t) i] - a / (double) cnt;
    }
    double e0 = 0;
    for (double v : env) e0 += v * v;
    Period best { 0.0, 0.0 };
    if (e0 <= 0.0) return best;
    for (long lag = (long) (minMs * 0.001 * fs); lag <= (long) (maxMs * 0.001 * fs) && lag < n; ++lag) {
        double a = 0;
        for (long i = 0; i + lag < n; ++i) a += env[(size_t) i] * env[(size_t) (i + lag)];
        if (a / e0 > best.strength) best = { 1000.0 * (double) lag / fs, a / e0 };
    }
    return best;
}

// Energy at and above f0 re the energy in [lo, hi), in dB, read off the
// spectrum. Not through band(): a 4th-order high-pass at 6 kHz passes what is
// at 4 kHz only 14 dB down, and read a spring that stops at 4.5 kHz as -15 dB
// "above 6 kHz" where the spectrum says -24.
inline double energyAboveDb(const Vec& x, double fs, double f0, double lo, double hi)
{
    int order = 1;
    while ((size_t) 1 << order < x.size()) ++order;
    const size_t n = (size_t) 1 << order;
    juce::dsp::FFT fft(order);
    std::vector<float> data(2 * n, 0.0f);
    for (size_t i = 0; i < x.size(); ++i) data[i] = (float) x[i];
    fft.performRealOnlyForwardTransform(data.data(), true);
    double above = 0, ref = 0;
    for (size_t k = 0; k <= n / 2; ++k) {
        const double f = (double) k * fs / (double) n, p = (double) data[2 * k] * data[2 * k] + (double) data[2 * k + 1] * data[2 * k + 1];
        if (f >= f0) above += p;
        if (f >= lo && f < hi) ref += p;
    }
    return 10.0 * std::log10((above + 1.0e-30) / (ref + 1.0e-30));
}

// Gaussian noise decaying 60 dB every t60 seconds: the measurer's own test signal.
inline Vec decayingNoise(double fs, double t60, double seconds, unsigned seed)
{
    std::mt19937 rng(seed);
    std::normal_distribution<double> nd(0.0, 1.0);
    Vec x((size_t) (seconds * fs));
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = nd(rng) * std::pow(10.0, -3.0 * (double) i / (fs * t60));
    return x;
}

// ── through the processor ────────────────────────────────────────────────────

// Stereo impulse response of `proc` with its parameters as they stand: a
// silent settle, then a unit impulse in both channels.
template <typename Processor>
Stereo impulseResponse(Processor& proc, double fs, int blockSize, double seconds, double settleSeconds = 0.5)
{
    proc.setPlayConfigDetails(2, 2, fs, blockSize);
    proc.prepareToPlay(fs, blockSize);
    juce::AudioBuffer<float> buf(2, blockSize);
    juce::MidiBuffer midi;
    for (int b = 0; b < (int) (settleSeconds * fs) / blockSize + 1; ++b) {
        buf.clear();
        proc.processBlock(buf, midi);
    }
    const size_t n = (size_t) (seconds * fs);
    Stereo s { Vec(n), Vec(n) };
    for (size_t pos = 0; pos < n; pos += (size_t) blockSize) {
        buf.clear();
        if (pos == 0) { buf.setSample(0, 0, 1.0f); buf.setSample(1, 0, 1.0f); }
        proc.processBlock(buf, midi);
        for (size_t i = 0; i < (size_t) blockSize && pos + i < n; ++i) {
            s.l[pos + i] = buf.getSample(0, (int) i);
            s.r[pos + i] = buf.getSample(1, (int) i);
        }
    }
    return s;
}

// Wall time spent in processBlock per second of audio, as a percentage
// (white noise in, median of `runs`). A report figure only: wall time depends
// on the machine and its load, so it never decides a PASS.
template <typename Processor>
double cpuPercent(Processor& proc, double fs, int blockSize = 512, double seconds = 5.0, int runs = 5)
{
    proc.setPlayConfigDetails(2, 2, fs, blockSize);
    proc.prepareToPlay(fs, blockSize);
    juce::AudioBuffer<float> buf(2, blockSize);
    juce::MidiBuffer midi;
    std::mt19937 rng(17);
    std::normal_distribution<float> nd(0.0f, 0.1f);
    const int blocks = (int) (seconds * fs) / blockSize;
    std::vector<double> pct;
    for (int run = -1; run < runs; ++run) {          // run -1 warms up and is dropped
        double spent = 0.0;
        for (int b = 0; b < blocks; ++b) {
            for (int i = 0; i < blockSize; ++i) { const float x = nd(rng); buf.setSample(0, i, x); buf.setSample(1, i, x); }
            const auto t0 = std::chrono::steady_clock::now();
            proc.processBlock(buf, midi);
            spent += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        }
        if (run >= 0) pct.push_back(100.0 * spent / ((double) blocks * blockSize / fs));
    }
    std::sort(pct.begin(), pct.end());
    return pct[pct.size() / 2];
}

} // namespace measure
