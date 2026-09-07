// Shared kernels for the live wave-terrain micro-benchmark (plain C++17, no JUCE).
#pragma once
#include <cmath>
#include <vector>
#include <cstdint>
#include <algorithm>

static constexpr float kPi   = 3.14159265358979f;
static constexpr float kTwoPi = 6.28318530717959f;

struct Pt { float x, y; };

// ── Table sine (4096 entries + guard, linear interp), input in radians ────────
struct SinTable
{
    static constexpr int N = 4096;
    float t[N + 1];
    SinTable() { for (int i = 0; i <= N; ++i) t[i] = std::sin (kTwoPi * i / N); }
    inline float sin (float rad) const
    {
        float p = rad * (N / kTwoPi);
        p -= std::floor (p / N) * N;           // wrap to [0,N)
        int i = (int) p; float f = p - i;
        return t[i] + f * (t[i + 1] - t[i]);
    }
    inline float cos (float rad) const { return sin (rad + kPi * 0.5f); }
};

// juce::dsp::FastMathApproximations::tanh (Padé) — same formula Terrain uses.
inline float fastTanh (float x)
{
    float x2 = x * x;
    float num = x * (135135.f + x2 * (17325.f + x2 * (378.f + x2)));
    float den = 135135.f + x2 * (62370.f + x2 * (3150.f + x2 * 28.f));
    return num / den;
}
inline float saturate (float s, float scale) { return fastTanh (s * scale * 1.31303528551f); }

// ── Analytic terrains (Terrain.h case 0 and case 7) ──────────────────────────
inline float sineProduct_std (Pt p, float a, float b)
{
    return std::sin (p.x * 6.f * (a + 0.5f)) * std::sin (p.y * 6.f * (b + 0.5f));
}
inline float sineProduct_tab (const SinTable& T, Pt p, float a, float b)
{
    return T.sin (p.x * 6.f * (a + 0.5f)) * T.sin (p.y * 6.f * (b + 0.5f));
}
inline float system14_std (Pt p, float a, float b, float c)
{
    float aa = a * 36.f + 6.f, bb = b * 2.f - 1.f, cc = c * 2.f - 1.f;
    float dx = p.x + bb, dy = p.y + cc;
    return std::cos (aa * std::sin (std::sqrt (dx * dx + dy * dy)));   // pow(x,2) → x*x
}
inline float system14_tab (const SinTable& T, Pt p, float a, float b, float c)
{
    float aa = a * 36.f + 6.f, bb = b * 2.f - 1.f, cc = c * 2.f - 1.f;
    float dx = p.x + bb, dy = p.y + cc;
    return T.cos (aa * T.sin (std::sqrt (dx * dx + dy * dy)));
}

// ── Image terrain: W×W float, bilinear, orbit in [-1,1]² mapped to texels ────
struct ImageTerrain
{
    int W; std::vector<float> px;
    explicit ImageTerrain (int w) : W (w), px ((size_t) w * w)
    {
        // smooth-ish synthetic content (so the test is about cost, not aliasing)
        for (int y = 0; y < w; ++y) for (int x = 0; x < w; ++x)
            px[(size_t) y * w + x] = std::sin (x * 0.05f) * std::cos (y * 0.031f) * 0.8f;
    }
    inline float read (Pt p) const
    {
        float fx = (p.x * 0.5f + 0.5f) * (W - 1), fy = (p.y * 0.5f + 0.5f) * (W - 1);
        fx = std::min (std::max (fx, 0.f), (float) (W - 1) - 1e-3f);
        fy = std::min (std::max (fy, 0.f), (float) (W - 1) - 1e-3f);
        int ix = (int) fx, iy = (int) fy; float tx = fx - ix, ty = fy - iy;
        const float* r0 = &px[(size_t) iy * W + ix];
        const float* r1 = r0 + W;
        float a = r0[0] + tx * (r0[1] - r0[0]);
        float b = r1[0] + tx * (r1[1] - r1[0]);
        return a + ty * (b - a);
    }
};

// ── Chebyshev terrain: f(x,y) = Σ_n Σ_m c[n][m] T_n(x) T_m(y), 2-D Clenshaw ──
template <int D>   // D = number of coefficients per axis (degree D-1)
struct ChebTerrain
{
    float c[D][D];
    inline float eval (Pt p) const
    {
        const float x2 = 2.f * p.x, y2 = 2.f * p.y;
        float g[D];
        for (int n = 0; n < D; ++n)                   // inner Clenshaw over m (y)
        {
            float b1 = 0.f, b2 = 0.f;
            for (int m = D - 1; m >= 1; --m) { float b0 = c[n][m] + y2 * b1 - b2; b2 = b1; b1 = b0; }
            g[n] = c[n][0] + p.y * b1 - b2;
        }
        float b1 = 0.f, b2 = 0.f;                     // outer Clenshaw over n (x)
        for (int n = D - 1; n >= 1; --n) { float b0 = g[n] + x2 * b1 - b2; b2 = b1; b1 = b0; }
        return g[0] + p.x * b1 - b2;
    }
};

// ── Orbits (Trajectory.h): θ in radians ──────────────────────────────────────
inline Pt ellipse_std (float th, float a)      { return { std::sin (th) * a, std::cos (th) }; }
inline Pt ellipse_tab (const SinTable& T, float th, float a) { return { T.sin (th) * a, T.cos (th) }; }

inline Pt epitrochoid7_std (float th, float a)
{
    float d = a + 0.01f, r = (1.f - d) / 8.f, R = 7.f * r, k = (R + r) / r;
    return { (R + r) * std::cos (th) - d * std::cos (k * th),
             (R + r) * std::sin (th) - d * std::sin (k * th) };
}
inline Pt epitrochoid7_tab (const SinTable& T, float th, float a)
{
    float d = a + 0.01f, r = (1.f - d) / 8.f, R = 7.f * r, k = (R + r) / r;
    return { (R + r) * T.cos (th) - d * T.cos (k * th),
             (R + r) * T.sin (th) - d * T.sin (k * th) };
}
inline Pt superellipse_std (float th, float ma, float mb, float mc)
{
    float n = ma * ma * 5.f + 0.5f, a = mb * 0.5f + 0.5f, b = mc * 0.5f + 0.5f;
    float c = std::cos (th), s = std::sin (th);
    float r = std::pow (std::pow (std::abs (c / a), n) + std::pow (std::abs (s / b), n), -1.f / n);
    return { r * c, r * s };
}
inline Pt superellipse_tab (const SinTable& T, float th, float ma, float mb, float mc)
{
    float n = ma * ma * 5.f + 0.5f, a = mb * 0.5f + 0.5f, b = mc * 0.5f + 0.5f;
    float c = T.cos (th), s = T.sin (th);
    float r = std::pow (std::pow (std::abs (c / a), n) + std::pow (std::abs (s / b), n), -1.f / n);
    return { r * c, r * s };
}
// Butterfly (Trajectory.h #3): r = e^cos(θ+a) − 2cos4θ + sin^5((2θ−π)/24)
inline Pt butterfly_std (float th, float a)
{
    float r = std::exp (std::cos (th + a * kTwoPi)) - 2.f * std::cos (4.f * th)
            + std::pow (std::sin ((2.f * th - kPi) / 24.f), 5.f);
    return { r * std::cos (th), r * std::sin (th) };
}
inline Pt circle (float th) { return { std::cos (th), std::sin (th) }; }

// ── O-Prism-style mipmapped wavetable (2048 + guard, 10 levels), trilinear, double ──
struct Wavetable
{
    static constexpr int kTableSize = 2048, kFrameSize = 2049, kLevels = 10;
    int numFrames; std::vector<float> data;
    explicit Wavetable (int frames) : numFrames (frames), data ((size_t) kLevels * frames * kFrameSize)
    {
        for (int l = 0; l < kLevels; ++l) for (int f = 0; f < frames; ++f)
        {
            float* d = &data[((size_t) l * frames + f) * kFrameSize];
            for (int i = 0; i < kTableSize; ++i) d[i] = std::sin (kTwoPi * i / kTableSize * (1 + f % 3)) / (l + 1);
            d[kTableSize] = d[0];
        }
    }
    inline float get (int l, int f, int i) const { return data[((size_t) l * numFrames + f) * kFrameSize + i]; }

    // Mirrors WavetableOscillator::readSample (per-sample log2 mip select).
    inline double read (double phase, float position, double frequency, double sr) const
    {
        phase -= std::floor (phase);
        double samplePos = phase * kTableSize; int idx0 = (int) samplePos; double frac = samplePos - idx0;
        double framePos = position * (numFrames - 1); int f0 = (int) framePos;
        int f1 = std::min (f0 + 1, numFrames - 1); double ff = framePos - f0;
        double baseFreq = sr / kTableSize;
        double lv = std::log2 (std::max (frequency, baseFreq) / baseFreq);
        lv = std::min (std::max (lv, 0.0), (double) (kLevels - 1));
        int l0 = (int) lv, l1 = std::min (l0 + 1, kLevels - 1); double lf = lv - l0;
        auto lerp = [] (double a, double b, double t) { return a + t * (b - a); };
        double s00 = lerp (get (l0, f0, idx0), get (l0, f0, idx0 + 1), frac);
        double s01 = lerp (get (l0, f1, idx0), get (l0, f1, idx0 + 1), frac);
        double s10 = lerp (get (l1, f0, idx0), get (l1, f0, idx0 + 1), frac);
        double s11 = lerp (get (l1, f1, idx0), get (l1, f1, idx0 + 1), frac);
        return lerp (lerp (s00, s01, ff), lerp (s10, s11, ff), lf);
    }
};

// ── 2× halfband polyphase-IIR (two allpass paths, 4 coefficients) — the
//    juce::dsp::Oversampling filterHalfBandPolyphaseIIR family, for cost only ──
struct Halfband2x
{
    float a0[2] = { 0.07986642623635751f, 0.5453536510711322f };
    float a1[2] = { 0.28382934487410993f, 0.8344118914807379f };
    struct AP { float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        inline float run (float x, float a) { float y = a * (x - y2) + x2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; } };
    AP p0[2], p1[2];
    inline float down (float xe, float xo)   // two input samples → one output
    {
        float u = p0[1].run (p0[0].run (xe, a0[0]), a0[1]);
        float v = p1[1].run (p1[0].run (xo, a1[0]), a1[1]);
        return 0.5f * (u + v);
    }
    inline void up (float x, float& y0, float& y1)   // one input → two outputs
    {
        y0 = p0[1].run (p0[0].run (x, a0[0]), a0[1]);
        y1 = p1[1].run (p1[0].run (x, a1[0]), a1[1]);
    }
};
