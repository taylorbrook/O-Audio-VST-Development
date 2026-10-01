// Research measurements for RESEARCH.md. See common.h.
//   clang++ -std=c++17 -O2 -o proto proto.cpp && ./proto
#include "engines.h"
#include <random>

struct Stereo { Vec l, r; };

template <typename Engine>
static Stereo impulse(Engine& e, double fs, double seconds)
{
    const size_t n = (size_t) (seconds * fs); Stereo s { Vec(n), Vec(n) };
    for (size_t i = 0; i < n; ++i) { const double x = i == 0 ? 1.0 : 0.0; e.process(x, x, s.l[i], s.r[i]); }
    return s;
}

static double goertzel(const Vec& x, double f, double fs, size_t from, size_t to)
{
    const double w = 2 * kPi * f / fs, c = 2 * std::cos(w); double s1 = 0, s2 = 0;
    for (size_t i = from; i < to && i < x.size(); ++i) { const double s0 = x[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}

struct Type { const char* name; double minMs, maxMs, t60, hf, fc, modMs, modHz; int jitter; };
static const Type kTypes[4] = {
    { "Booth",    2.9,  13.7, 0.40, 0.55, 6000, 0.00, 0.0, 1 },
    { "Room",     8.3,  37.9, 1.10, 0.50, 5000, 0.06, 0.7, 2 },
    { "Hall",    21.7,  83.1, 3.00, 0.45, 4000, 0.15, 0.5, 3 },
    { "Ambient", 30.7, 121.3, 7.00, 0.60, 3500, 0.25, 0.3, 4 },
};
static FdnConfig cfgFor(const Type& t, int N = 16) { FdnConfig c; c.N = N; c.minMs = t.minMs; c.maxMs = t.maxMs; c.t60 = t.t60; c.hfRatio = t.hf; c.fcHz = t.fc; c.modMs = t.modMs; c.modHz = t.modHz; c.jitter = t.jitter; return c; }

static void fdnRow(const char* label, const FdnConfig& c, double fs)
{
    Fdn f; f.prepare(fs, c);
    const auto s = impulse(f, fs, 1.35 * c.t60 + 0.4);
    const double mid = midRt60(s.l, fs), hf = hfRt60(s.l, fs), lf = lfRt60(s.l, fs);
    const size_t a = (size_t) (0.25 * c.t60 * fs), b = (size_t) (0.75 * c.t60 * fs);
    std::printf("  %-34s mid %6.3f s (%+5.1f %%)  hf %6.3f (want %5.2f, %+5.1f %%)  lf %6.3f  corrLR %+.3f  mix %5.1f ms\n",
                label, mid, 100 * (mid / c.t60 - 1), hf, c.t60 * c.hfRatio, 100 * (hf / (c.t60 * c.hfRatio) - 1), lf,
                correlation(s.l, s.r, a, b), mixingTimeMs(s.l, fs));
}

int main()
{
    char buf[128];

    std::printf("== 1. FDN: RT60 against target, DECAY x SIZE (48 kHz, N=16, Hadamard) ==\n");
    for (const auto& t : kTypes)
        for (double dec : { 0.5, 1.0, 2.0 })
            for (double size : { 0.5, 1.0, 2.0 }) {
                if (dec != 1.0 && size != 1.0) continue;
                auto c = cfgFor(t); c.t60 = t.t60 * dec; c.size = size;
                std::snprintf(buf, sizeof buf, "%s decay %.1fx size x%.1f", t.name, dec, size); fdnRow(buf, c, 48000);
            }

    std::printf("\n== 2. FDN: line count and matrix (Hall, 48 kHz) ==\n");
    for (int N : { 8, 16 }) for (bool hh : { false, true }) {
        auto c = cfgFor(kTypes[2], N); c.householder = hh;
        std::snprintf(buf, sizeof buf, "Hall N=%d %s", N, hh ? "Householder" : "Hadamard"); fdnRow(buf, c, 48000);
        c.diffuse = false; std::snprintf(buf, sizeof buf, "Hall N=%d %s no in-diffusion", N, hh ? "Householder" : "Hadamard"); fdnRow(buf, c, 48000);
    }
    { auto c = cfgFor(kTypes[0], 8); fdnRow("Booth N=8 Hadamard", c, 48000); c.N = 16; fdnRow("Booth N=16 Hadamard", c, 48000); }
    { auto c = cfgFor(kTypes[3], 8); fdnRow("Ambient N=8 Hadamard", c, 48000); }
    std::printf("  total loop delay (modes per Hz), size x1:");
    for (const auto& t : kTypes) for (int N : { 8, 16 }) { Fdn f; f.prepare(48000, cfgFor(t, N)); double s = 0; for (double L : f.len) s += L; std::printf("  %s N=%d %.2f s", t.name, N, s / 48000); }
    std::printf("\n");

    std::printf("\n== 3. FDN: interpolation in the loop (Hall, modulated lines) ==\n");
    { auto c = cfgFor(kTypes[2]); c.modMs = 0; fdnRow("static, integer lines", c, 48000);
      c = cfgFor(kTypes[2]); fdnRow("modulated, 3rd-order Lagrange", c, 48000);
      c.linearInterp = true; fdnRow("modulated, linear", c, 48000);
      c = cfgFor(kTypes[2]); c.modMs = 0; c.allLinesFractional = true; fdnRow("static, FRACTIONAL lines (Lagrange)", c, 48000);
      c.linearInterp = true; fdnRow("static, FRACTIONAL lines (linear)", c, 48000); }

    std::printf("\n== 4. FDN: sample rate ==\n");
    for (double fs : { 44100.0, 96000.0, 192000.0 }) for (int ti : { 0, 2 }) {
        std::snprintf(buf, sizeof buf, "%s @ %.0f Hz", kTypes[ti].name, fs); fdnRow(buf, cfgFor(kTypes[ti]), fs); }

    std::printf("\n== 5. FDN: SIZE glide on a ringing tail (Hall, 300 Hz sine feed) ==\n");
    for (double tauMs : { 0.0, 50.0, 250.0 }) {
        const double fs = 48000; Fdn f; auto c = cfgFor(kTypes[2]); c.size = 0.5; f.prepare(fs, c);
        const double coeff = tauMs > 0 ? 1.0 - std::exp(-1.0 / (tauMs * 0.001 * fs)) : 0.0;
        double maxD2 = 0, ref = 0, p1 = 0, p2 = 0, peak = 0; bool finite = true;
        for (int i = 0; i < (int) (6 * fs); ++i) {
            if (i == (int) (2 * fs)) f.setSize(2.0, tauMs == 0.0);
            if (i == (int) (4 * fs)) f.setSize(0.5, tauMs == 0.0);
            const double x = 0.5 * std::sin(2 * kPi * 300.0 * i / fs); double l, r; f.process(x, x, l, r, coeff);
            finite = finite && std::isfinite(l); peak = std::max(peak, std::abs(l));
            const double d2 = std::abs(l - 2 * p1 + p2); p2 = p1; p1 = l;
            if (i > (int) (1.0 * fs) && i < (int) (2 * fs)) ref = std::max(ref, d2);
            if (i >= (int) (2 * fs)) maxD2 = std::max(maxD2, d2);
        }
        std::printf("  glide tau %5.0f ms: finite %d  peak %.3f  max|d2| during moves / held = %.1fx\n", tauMs, (int) finite, peak, maxD2 / ref);
    }

    std::printf("\n== 6. Plate: decay coefficient -> RT60 ==\n");
    for (double size : { 0.7, 1.0, 1.4 }) for (double t : { 1.25, 2.5, 5.0 }) for (double sh : { 0.0, 0.08 }) {
        const double fs = 48000; Plate p; PlateConfig c; c.t60 = t; c.size = size; c.shimmer = sh; p.prepare(fs, c);
        const auto s = impulse(p, fs, 1.35 * t + 0.4);
        const double mid = midRt60(s.l, fs);
        std::printf("  size x%.1f T60 %.2f shimmer %.2f: decay %.4f loop %.3f s  mid %6.3f s (%+5.1f %%)  hf %6.3f  corrLR %+.3f  mix %5.1f ms\n",
                    size, t, sh, p.decay, p.loopSeconds(), mid, 100 * (mid / t - 1), hfRt60(s.l, fs),
                    correlation(s.l, s.r, (size_t) (0.25 * t * fs), (size_t) (0.75 * t * fs)), mixingTimeMs(s.l, fs));
    }
    for (double fs : { 44100.0, 96000.0 }) { Plate p; PlateConfig c; p.prepare(fs, c); const auto s = impulse(p, fs, 3.8);
        std::printf("  T60 2.50 @ %.0f Hz: mid %6.3f s (%+5.1f %%)\n", fs, midRt60(s.l, fs), 100 * (midRt60(s.l, fs) / 2.5 - 1)); }

    std::printf("\n== 7. Plate: shifter inside the loop ==\n");
    for (double sh : { 0.08, 0.25, 1.0 }) {     // worst case: decay forced to 0.9999 (far beyond any DECAY/SIZE setting)
        const double fs = 48000; Plate p; PlateConfig c; c.shimmer = sh; c.decayOverride = 0.9999; c.damping = 0.0; p.prepare(fs, c);
        std::mt19937 rng(11); std::normal_distribution<double> nd(0, 0.1);
        double e1 = 0, e2 = 0, e3 = 0, peak = 0;
        for (int i = 0; i < (int) (40 * fs); ++i) { const double x = i < (int) fs ? nd(rng) : 0.0; double l, r; p.process(x, x, l, r); peak = std::max(peak, std::abs(l));
            if (i >= (int) (2 * fs) && i < (int) (4 * fs)) e1 += l * l; if (i >= (int) (18 * fs) && i < (int) (20 * fs)) e2 += l * l; if (i >= (int) (38 * fs)) e3 += l * l; }
        std::printf("  decay 0.9999, damping 0, shimmer %.2f: energy 2-4 s 0 dB, 18-20 s %+.1f dB, 38-40 s %+.1f dB (peak %.2f) -> %s\n",
                    sh, 10 * std::log10(e2 / e1), 10 * std::log10(e3 / e1), peak, e3 < e2 && e2 < e1 ? "decaying" : "NOT decaying");
    }
    for (double sh : { 0.0, 0.04, 0.08, 0.15 }) {   // bloom: 300 Hz burst, then the octave's share of the tail over time
        const double fs = 48000; Plate p; PlateConfig c; c.shimmer = sh; p.prepare(fs, c); Vec y;
        for (int i = 0; i < (int) (3.6 * fs); ++i) { const double x = i < (int) (0.5 * fs) ? 0.5 * std::sin(2 * kPi * 300.0 * i / fs) : 0.0; double l, r; p.process(x, x, l, r); y.push_back(l); }
        auto ratio = [&](double t0, double t1, double f) { return 10 * std::log10(goertzel(y, f, fs, (size_t) (t0 * fs), (size_t) (t1 * fs)) / goertzel(y, 300.0, fs, (size_t) (t0 * fs), (size_t) (t1 * fs))); };
        std::printf("  shimmer %.2f: 600/300 Hz  %+.1f dB @0.1-0.5 s (burst on), %+.1f @0.6-1.1, %+.1f @1.6-2.1, %+.1f @2.6-3.1 | 1200/300 @2.6-3.1 %+.1f dB\n",
                    sh, ratio(0.1, 0.5, 600), ratio(0.6, 1.1, 600), ratio(1.6, 2.1, 600), ratio(2.6, 3.1, 600), ratio(2.6, 3.1, 1200));
    }

    std::printf("\n== 8. Spring: RT60, chirp ==\n");
    for (double fs : { 44100.0, 48000.0, 96000.0 }) for (double t : { 1.25, 2.5, 5.0 }) {
        if (fs != 48000.0 && t != 2.5) continue;
        Spring sp; SpringConfig c; c.t60 = t; sp.prepare(fs, c);
        const size_t n = (size_t) ((1.35 * t + 0.4) * fs); Vec y(n);
        for (size_t i = 0; i < n; ++i) y[i] = sp.process(i == 0 ? 1.0 : 0.0);
        std::printf("  %.0f Hz T60 %.2f: K %d  loop %.1f samples  g %.4f  mid RT60 %6.3f s (%+5.1f %%)  band 2-4k RT60 %6.3f\n",
                    fs, t, sp.K, sp.loopLen, sp.g, midRt60(y, fs), 100 * (midRt60(y, fs) / t - 1), rt60(band(y, fs, 2000, 4000), fs));
        if (fs == 48000.0 && t == 2.5) {
            std::printf("    first-echo arrival (envelope peak, ms) vs analytic echo time:");
            for (double f : { 300.0, 600.0, 1000.0, 2000.0, 3000.0, 3800.0 }) {
                const Vec nb = band(Vec(y.begin(), y.begin() + (long) (0.16 * fs)), fs, f / 1.06, f * 1.06);
                size_t arg = 0; double m = 0; for (size_t i = 0; i < nb.size(); ++i) if (std::abs(nb[i]) > m) { m = std::abs(nb[i]); arg = i; }
                std::printf("  %.0f Hz %.1f (%.1f)", f, 1000.0 * arg / fs, 1000.0 * sp.echoSeconds(f));
            }
            std::printf("\n");
        }
    }
    for (int M : { 40, 80, 120 }) for (double a1 : { 0.5, 0.62, 0.75 }) {
        Spring sp; SpringConfig c; c.M = M; c.a1 = a1; sp.prepare(48000, c);
        std::printf("  M %3d a1 %.2f: echo time 300 Hz %.1f ms, 1 kHz %.1f, 3 kHz %.1f, 4 kHz %.1f  (chirp span %.1f ms, loop delay %.1f ms)\n",
                    M, a1, 1e3 * sp.echoSeconds(300), 1e3 * sp.echoSeconds(1000), 1e3 * sp.echoSeconds(3000), 1e3 * sp.echoSeconds(4000),
                    1e3 * (sp.echoSeconds(4000) - sp.echoSeconds(300)), 1e3 * sp.loopLen / 48000);
    }

    std::printf("\n== 10. FDN: shelf solved at 1 kHz (mid-band target), Booth diffusers scaled ==\n");
    for (const auto& t : kTypes) for (double dec : { 0.5, 1.0, 2.0 }) for (double size : { 0.5, 1.0, 2.0 }) {
        if (dec != 1.0 && size != 1.0) continue;
        auto c = cfgFor(t); c.t60 = t.t60 * dec; c.size = size; c.midRefHz = 1000.0; if (t.t60 < 0.5) c.diffScale = 0.25;
        std::snprintf(buf, sizeof buf, "%s decay %.1fx size x%.1f", t.name, dec, size); fdnRow(buf, c, 48000);
    }
    for (double fs : { 44100.0, 96000.0 }) for (int ti : { 0, 2, 3 }) { auto c = cfgFor(kTypes[ti]); c.midRefHz = 1000.0; if (ti == 0) c.diffScale = 0.25;
        std::snprintf(buf, sizeof buf, "%s @ %.0f Hz", kTypes[ti].name, fs); fdnRow(buf, c, fs); }

    std::printf("\n== 11. FDN: do two types share resonances? correlation of the tail's dB spectrum, 200-2000 Hz ==\n");
    {
        auto spectrum = [&](FdnConfig c) { const double fs = 48000; c.t60 = 3.0; c.modMs = 0; Fdn f; f.prepare(fs, c); const auto s = impulse(f, fs, 2.6);
            Vec m; for (double fq = 200.0; fq < 2000.0; fq += 0.45) m.push_back(10 * std::log10(goertzel(s.l, fq, fs, (size_t) (0.5 * fs), (size_t) (2.5 * fs)) + 1e-30)); return m; };
        auto corr = [](const Vec& a, const Vec& b) { double ma = 0, mb = 0; for (size_t i = 0; i < a.size(); ++i) { ma += a[i]; mb += b[i]; } ma /= a.size(); mb /= a.size();
            double ab = 0, aa = 0, bb = 0; for (size_t i = 0; i < a.size(); ++i) { ab += (a[i] - ma) * (b[i] - mb); aa += (a[i] - ma) * (a[i] - ma); bb += (b[i] - mb) * (b[i] - mb); } return ab / std::sqrt(aa * bb); };
        Vec sp[4]; for (int i = 0; i < 4; ++i) sp[i] = spectrum(cfgFor(kTypes[i]));
        for (int i = 0; i < 4; ++i) for (int j = i + 1; j < 4; ++j) std::printf("  %s vs %s: %+.3f\n", kTypes[i].name, kTypes[j].name, corr(sp[i], sp[j]));
        auto same = cfgFor(kTypes[2]); same.fcHz = 2500; same.hfRatio = 0.3;    // same delays, different absorption/EQ = what v1.x does
        std::printf("  Hall vs Hall with different damping (same delays): %+.3f   <- the v1.x situation\n", corr(sp[2], spectrum(same)));
        auto sized = cfgFor(kTypes[2]); sized.size = 1.3; std::printf("  Hall vs Hall at SIZE x1.3: %+.3f\n", corr(sp[2], spectrum(sized)));
    }

    std::printf("\n== 12. FDN: SIZE glide, HF burst above 3 kHz (click metric of render-check section 5) ==\n");
    for (double tauMs : { 0.0, 50.0, 150.0, 250.0, 400.0 }) {
        const double fs = 48000; Fdn f; auto c = cfgFor(kTypes[2]); c.size = 0.5; c.midRefHz = 1000; f.prepare(fs, c);
        const double coeff = tauMs > 0 ? 1.0 - std::exp(-1.0 / (tauMs * 0.001 * fs)) : 0.0;
        Biquad h1 = Biquad::hp(fs, 3000, 0.541), h2 = Biquad::hp(fs, 3000, 1.307); double held = 1e-9, moved = 1e-9, peak = 0;
        for (int i = 0; i < (int) (8 * fs); ++i) {
            if (i == (int) (3 * fs)) f.setSize(2.0, tauMs == 0.0);
            if (i == (int) (5.5 * fs)) f.setSize(0.5, tauMs == 0.0);
            const double x = 0.5 * std::sin(2 * kPi * 300.0 * i / fs); double l, r; f.process(x, x, l, r, coeff);
            const double hf = std::abs(h2(h1(l))); peak = std::max(peak, std::abs(l));
            if (i > (int) (2 * fs) && i < (int) (3 * fs)) held = std::max(held, hf);
            if (i >= (int) (3 * fs)) moved = std::max(moved, hf);
        }
        std::printf("  glide tau %5.0f ms: HF burst %+.1f dB re wet peak (held %+.1f dB)\n", tauMs, 20 * std::log10(moved / peak), 20 * std::log10(held / peak));
    }

    std::printf("\n== 13. Plate: echo density over time (NED, 1 = Gaussian), and calibrated decay ==\n");
    { Fdn f; auto c = cfgFor(kTypes[2]); f.prepare(48000, c); const auto s = impulse(f, 48000, 1.0);
      std::printf("  FDN Hall (reference)          NED @ 20/50/100/200/400 ms: %.2f %.2f %.2f %.2f %.2f\n", nedAt(s.l, 48000, .02), nedAt(s.l, 48000, .05), nedAt(s.l, 48000, .1), nedAt(s.l, 48000, .2), nedAt(s.l, 48000, .4)); }
    for (double size : { 0.4, 0.55, 0.7, 1.0 }) for (double diff : { 1.0, 1.15 }) {
        const double fs = 48000; Plate p; PlateConfig c; c.size = size; c.inDiffusion = diff; p.prepare(fs, c); const auto s = impulse(p, fs, 1.0);
        std::printf("  Plate size x%.2f in-diffusion x%.2f (loop %.3f s) NED @ 20/50/100/200/400 ms: %.2f %.2f %.2f %.2f %.2f\n", size, diff, p.loopSeconds(),
                    nedAt(s.l, fs, .02), nedAt(s.l, fs, .05), nedAt(s.l, fs, .1), nedAt(s.l, fs, .2), nedAt(s.l, fs, .4));
    }
    for (double size : { 0.4, 0.55, 0.8 }) for (double t : { 1.25, 2.5, 5.0 }) for (double sh : { 0.0, 0.08, 0.12 }) {
        const double fs = 48000; Plate p; PlateConfig c; c.t60 = t; c.size = size; c.shimmer = sh; c.loopFudge = 1.055; c.shimmerComp = 0.9; p.prepare(fs, c);
        const auto s = impulse(p, fs, 1.35 * t + 0.4); const double mid = midRt60(s.l, fs);
        std::printf("  size x%.2f T60 %.2f shimmer %.2f (fudge 1.055, comp 0.9): decay %.4f  mid %6.3f s (%+5.1f %%)\n", size, t, sh, p.decay, mid, 100 * (mid / t - 1));
    }
    { const double fs = 48000; Plate p; PlateConfig c; c.t60 = 5.0; c.size = 0.4; c.shimmer = 0.12; c.loopFudge = 1.055; c.shimmerComp = 0.9; c.damping = 0; p.prepare(fs, c);
      std::mt19937 rng(5); std::normal_distribution<double> nd(0, 0.1); double e1 = 0, e2 = 0;
      for (int i = 0; i < (int) (30 * fs); ++i) { const double x = i < (int) (10 * fs) ? nd(rng) : 0.0; double l, r; p.process(x, x, l, r); if (i >= (int) (9 * fs) && i < (int) (10 * fs)) e1 += l * l; if (i >= (int) (29 * fs)) e2 += l * l; }
      std::printf("  worst case in range (T60 5 s, smallest size, shimmer 0.12, damping 0): decay %.4f, 10 s of noise then 19 s later %+.1f dB\n", p.decay, 10 * std::log10(e2 / e1)); }

    for (double sh : { 0.08, 0.12 }) {   // bloom at the calibrated settings, SIZE 50 scale
        const double fs = 48000; Plate p; PlateConfig c; c.shimmer = sh; c.size = 0.55; c.loopFudge = 1.055; c.shimmerComp = 0.9; p.prepare(fs, c); Vec y;
        for (int i = 0; i < (int) (3.6 * fs); ++i) { const double x = i < (int) (0.5 * fs) ? 0.5 * std::sin(2 * kPi * 300.0 * i / fs) : 0.0; double l, r; p.process(x, x, l, r); y.push_back(l); }
        auto ratio = [&](double t0, double t1, double f) { return 10 * std::log10(goertzel(y, f, fs, (size_t) (t0 * fs), (size_t) (t1 * fs)) / goertzel(y, 300.0, fs, (size_t) (t0 * fs), (size_t) (t1 * fs))); };
        std::printf("  size x0.55 shimmer %.2f: 600/300 Hz %+.1f dB @0.1-0.5 s, %+.1f @0.6-1.1, %+.1f @1.6-2.1, %+.1f @2.6-3.1 | 1200/300 @2.6-3.1 %+.1f\n",
                    sh, ratio(0.1, 0.5, 600), ratio(0.6, 1.1, 600), ratio(1.6, 2.1, 600), ratio(2.6, 3.1, 600), ratio(2.6, 3.1, 1200));
    }

    std::printf("\n== 9. Cost (this prototype, double precision, -O2, one core): x real time ==\n");
    for (double fs : { 48000.0, 96000.0 }) {
        const int n = (int) (20 * fs); double l, r, sink = 0;
        { Fdn f; f.prepare(fs, cfgFor(kTypes[3], 16)); Timer t; for (int i = 0; i < n; ++i) { f.process(i % 4800 == 0, 0, l, r); sink += l; } std::printf("  %.0f Hz  FDN N=16 (Ambient): %.0fx", fs, 20.0 / t.seconds()); }
        { Fdn f; f.prepare(fs, cfgFor(kTypes[3], 8)); Timer t; for (int i = 0; i < n; ++i) { f.process(i % 4800 == 0, 0, l, r); sink += l; } std::printf("   FDN N=8: %.0fx", 20.0 / t.seconds()); }
        { Plate p; PlateConfig c; c.shimmer = 0.08; p.prepare(fs, c); Timer t; for (int i = 0; i < n; ++i) { p.process(i % 4800 == 0, 0, l, r); sink += l; } std::printf("   Plate+shimmer: %.0fx", 20.0 / t.seconds()); }
        { Spring s[3]; SpringConfig c; for (auto& x : s) x.prepare(fs, c); Timer t; for (int i = 0; i < n; ++i) for (auto& x : s) sink += x.process(i % 4800 == 0); std::printf("   3 springs (M=80): %.0fx\n", 20.0 / t.seconds()); }
        if (sink == 12345.678) std::printf(" ");
    }
    return 0;
}
