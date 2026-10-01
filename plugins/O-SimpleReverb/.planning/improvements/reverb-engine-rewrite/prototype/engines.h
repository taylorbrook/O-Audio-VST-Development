// Research prototype engines — see common.h. Stereo in, stereo out, one sample
// at a time.
#pragma once
#include "common.h"

// ═════════════════════════════════════════════════════════════════════════════
//  FDN  (Booth / Room / Hall / Ambient)
// ═════════════════════════════════════════════════════════════════════════════
struct FdnConfig {
    int    N        = 16;       // 8 or 16 (power of two: Hadamard)
    double minMs    = 20.0;     // shortest / longest line at sizeScale 1
    double maxMs    = 60.0;
    int    jitter   = 1;        // selects the per-type irregularity of the length ladder
    double t60      = 1.0;      // low/mid decay time, s
    double hfRatio  = 0.5;      // T60(high) / T60(low)
    double fcHz     = 5000.0;   // crossover of the two decay bands
    double modMs    = 0.0;      // peak excursion of the modulated lines (0 = static)
    double modHz    = 0.5;      // centre rate; lines spread 0.6x..1.6x around it
    double size     = 1.0;      // length scale
    bool   householder = false; // else Hadamard
    bool   linearInterp = false;// modulated reads: linear instead of 3rd-order Lagrange
    bool   allLinesFractional = false; // do NOT round unmodulated lines to integers
    bool   diffuse  = true;     // 4 input allpasses per channel
    double diffScale = 1.0;     // scales the input allpass lengths (their own ring is 3 D / -log10 g seconds)
    double midRefHz = 0.0;      // > 0: solve the shelf so |H| at this frequency (not at DC) meets the T60 target
};

struct Fdn {
    FdnConfig c; double fs = 48000;
    std::vector<Delay> line; Vec len, target, glo, ghi, lp, rate, phase, ratio; std::vector<bool> modulated;
    double lpA = 0; Allpass apL[4], apR[4];
    Vec bL, bR, cL, cR, v;

    void prepare(double sampleRate, const FdnConfig& cfg) {
        fs = sampleRate; c = cfg; const int N = c.N;
        line.assign((size_t) N, Delay()); len.assign((size_t) N, 0); target = len; glo = len; ghi = len; lp = len; rate = len; phase = len; ratio = len; v = len;
        modulated.assign((size_t) N, false);
        for (int i = 0; i < N; ++i) {
            line[(size_t) i].prepare((int) (c.maxMs * 4.2 * 0.001 * fs) + 64);
            // geometric ladder minMs..maxMs with a deterministic, type-specific irregularity
            const double u = (double) i / (N - 1);
            const double wob = 0.035 * std::sin(2.39996 * (i + 1) * (c.jitter + 1));   // golden-angle hash, +/-3.5 %
            ratio[(size_t) i] = std::pow(c.maxMs / c.minMs, u) * (1.0 + wob);
            modulated[(size_t) i] = c.modMs > 0.0 && (i % 2 == 1);
            rate[(size_t) i] = c.modHz * (0.6 + 1.0 * ((i * 7) % N) / (double) N);
            phase[(size_t) i] = 2 * kPi * ((i * 5) % N) / (double) N;                   // deterministic start phases
        }
        setSize(c.size, true);
        lpA = std::exp(-2 * kPi * c.fcHz / fs);
        // Hadamard rows as in/out vectors: orthogonal, so L and R excite and read different mode mixtures
        bL.assign((size_t) N, 0); bR = bL; cL = bL; cR = bL;
        for (int i = 0; i < N; ++i) {
            auto row = [&](int r) { return (__builtin_popcount((unsigned) (r & i)) & 1) ? -1.0 : 1.0; };
            bL[(size_t) i] = row(1) / std::sqrt((double) N) * std::sqrt(2.0); bR[(size_t) i] = row(2) / std::sqrt((double) N) * std::sqrt(2.0);
            cL[(size_t) i] = row(N / 2 + 1) / std::sqrt((double) N); cR[(size_t) i] = row(N / 2 + 2) / std::sqrt((double) N);
        }
        const double apMsL[4] = { 4.771, 3.595, 12.73, 9.307 }, apMsR[4] = { 5.107, 3.881, 11.83, 9.911 };
        for (int k = 0; k < 4; ++k) { apL[k].prepare((int) (apMsL[k] * c.diffScale * 0.001 * fs), k < 2 ? 0.75 : 0.625); apR[k].prepare((int) (apMsR[k] * c.diffScale * 0.001 * fs), k < 2 ? 0.75 : 0.625); }
        if (c.midRefHz > 0.0) { const double w = 2 * kPi * c.midRefHz / fs, dr = 1 - lpA * std::cos(w), di = lpA * std::sin(w), m = dr * dr + di * di;
            pRe = (1 - lpA) * dr / m; pIm = -(1 - lpA) * di / m; }
    }
    double pRe = 1, pIm = 0;    // the one-pole low-pass's response at midRefHz

    void setSize(double size, bool jump) {
        for (size_t i = 0; i < len.size(); ++i) {
            double L = c.minMs * ratio[i] * size * 0.001 * fs;
            if (! modulated[i] && ! c.allLinesFractional) L = std::round(L);
            target[i] = L; if (jump) len[i] = L;
        }
    }
    void setT60(double t) { c.t60 = t; }

    void process(double inL, double inR, double& outL, double& outR, double glideCoeff = 0.0) {
        const int N = c.N;
        if (c.diffuse) for (int k = 0; k < 4; ++k) { inL = apL[k].process(inL); inR = apR[k].process(inR); }
        outL = outR = 0;
        for (int i = 0; i < N; ++i) {
            const size_t u = (size_t) i;
            if (glideCoeff > 0.0) len[u] += (target[u] - len[u]) * glideCoeff;
            double d = len[u], x;
            if (modulated[u]) { phase[u] += 2 * kPi * rate[u] / fs; if (phase[u] > 2 * kPi) phase[u] -= 2 * kPi; d += c.modMs * 0.001 * fs * std::sin(phase[u]); }
            const bool integer = d == std::floor(d);
            x = integer ? line[u].at((int) d) : (c.linearInterp ? line[u].readLinear(d) : line[u].readCubic(d));
            // per-line absorption from the line's CURRENT length: RT60 holds while SIZE moves
            const double k = -3.0 * len[u] / fs;
            double gl = std::pow(10.0, k / c.t60); const double gh = std::pow(10.0, k / (c.t60 * c.hfRatio));
            if (c.midRefHz > 0.0) { const double p2 = pRe * pRe + pIm * pIm;   // |gh + d P| = gl  ->  d
                const double d = (-gh * pRe + std::sqrt(gh * gh * pRe * pRe - p2 * (gh * gh - gl * gl))) / p2; gl = std::min(0.9999, gh + d); }
            lp[u] = x + lpA * (lp[u] - x);                                  // one-pole low-pass at fc
            v[u] = gh * x + (gl - gh) * lp[u];                              // DC gain gl, Nyquist ~gh, |H| <= gl < 1
            outL += cL[u] * x; outR += cR[u] * x;
        }
        if (c.householder) { double s = 0; for (int i = 0; i < N; ++i) s += v[(size_t) i]; s *= 2.0 / N; for (int i = 0; i < N; ++i) v[(size_t) i] = v[(size_t) i] - s; }
        else { for (int h = 1; h < N; h <<= 1) for (int i = 0; i < N; i += h << 1) for (int j = i; j < i + h; ++j) { const double a = v[(size_t) j], b = v[(size_t) (j + h)]; v[(size_t) j] = a + b; v[(size_t) (j + h)] = a - b; }
               const double s = 1.0 / std::sqrt((double) N); for (int i = 0; i < N; ++i) v[(size_t) i] *= s; }
        for (int i = 0; i < N; ++i) line[(size_t) i].push(v[(size_t) i] + bL[(size_t) i] * inL + bR[(size_t) i] * inR);
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  PLATE  (Dattorro 1997 figure-8 tank, lengths scaled from 29761 Hz)
// ═════════════════════════════════════════════════════════════════════════════
struct PlateConfig {
    double t60 = 2.5, size = 1.0, damping = 0.25 /* one-pole coeff at 29761-equivalent */, bandwidth = 0.9995;
    double shimmer = 0.0;       // 0..1 share of the cross-feed that goes through the octave shifter
    double excursionMs = 0.27;  // 8 samples peak at 29761 Hz
    double decayOverride = -1;  // > 0: use this decay coefficient instead of the T60 map
    double loopFudge = 1.0;     // measured: the tank decays slower than the mean-loop-time formula says
    double shimmerComp = 0.0;   // share of the shimmer's mid-band drain given back through `decay`
    double decayCeiling = 0.97; // with a convex shimmer mix the loop gain is <= decay, so this is the stability bound
    double inDiffusion = 1.0;   // scales the two input-diffusion coefficients
};

struct Plate {
    PlateConfig c; double fs = 48000, k = 1, decay = 0.5, lfo = 0;
    Allpass inApL[4], inApR[4], ap1A, ap2A, ap1B, ap2B; Delay dA1, dA2, dB1, dB2; double dampA = 0, dampB = 0, bwL = 0, bwR = 0, shLpA = 0, shLpB = 0;
    OctaveUp shA, shB; int n[12];
    static constexpr double kRef = 29761.0;
    int S(double ref) const { return std::max(2, (int) std::lround(ref * k)); }

    void prepare(double sampleRate, const PlateConfig& cfg) {
        fs = sampleRate; c = cfg; k = fs / kRef * c.size;
        const double in[4] = { 142, 107, 379, 277 }, inR[4] = { 151, 113, 367, 283 };
        const double ks = fs / kRef;    // input diffusion does not scale with SIZE
        for (int i = 0; i < 4; ++i) { inApL[i].prepare((int) std::lround(in[i] * ks), c.inDiffusion * (i < 2 ? 0.75 : 0.625)); inApR[i].prepare((int) std::lround(inR[i] * ks), c.inDiffusion * (i < 2 ? 0.75 : 0.625)); }
        ap1A.prepare(S(672) + 64, -0.70); ap1A.len = S(672); ap1B.prepare(S(908) + 64, -0.70); ap1B.len = S(908);
        ap2A.prepare(S(1800), 0.50); ap2B.prepare(S(2656), 0.50);
        dA1.prepare(S(4453)); dA2.prepare(S(3720)); dB1.prepare(S(4217)); dB2.prepare(S(3163));
        shA.prepare(fs); shB.prepare(fs);
        setT60(c.t60);
    }
    // Mean loop time: the four delays plus the four allpasses' mean group delay
    // (= their length). Four multiplications by `decay` per trip round the figure-8.
    double loopSeconds() const { return (S(4453) + S(3720) + S(4217) + S(3163) + S(672) + S(1800) + S(908) + S(2656)) / fs; }
    void setT60(double t) { c.t60 = t;
        // the shifter sits on the TWO cross-feeds, `decay` is applied FOUR times per trip: half the drain per decay stage
        decay = std::pow(10.0, -3.0 * loopSeconds() * c.loopFudge / (4.0 * t)) / std::sqrt(1.0 - c.shimmerComp * c.shimmer);
        decay = c.decayOverride > 0 ? c.decayOverride : std::min(decay, c.decayCeiling); }

    double cross(double x, OctaveUp& sh, double& lpState) {
        if (c.shimmer <= 0.0) return x;
        lpState += 0.45 * (x - lpState);                           // keep the shifter's input below ~fs/4
        return (1.0 - c.shimmer) * x + c.shimmer * sh.process(lpState);   // CONVEX mix: gain <= 1
    }

    void process(double inL, double inR, double& outL, double& outR) {
        bwL += c.bandwidth * (inL - bwL); bwR += c.bandwidth * (inR - bwR); inL = bwL; inR = bwR;
        for (int i = 0; i < 4; ++i) { inL = inApL[i].process(inL); inR = inApR[i].process(inR); }
        lfo += 2 * kPi * 1.0 / fs; if (lfo > 2 * kPi) lfo -= 2 * kPi;
        const double exc = c.excursionMs * 0.001 * fs;
        const double fbA = decay * cross(dB2.at(S(3163)), shB, shLpB), fbB = decay * cross(dA2.at(S(3720)), shA, shLpA);
        const double damp = c.damping;
        // half A
        double a = ap1A.processMod(inL + fbA, S(672) + exc * std::sin(lfo));
        dA1.push(a); a = dA1.at(S(4453)); dampA = (1 - damp) * a + damp * dampA; a = ap2A.process(dampA * decay); dA2.push(a);
        // half B
        double b = ap1B.processMod(inR + fbB, S(908) + exc * std::cos(lfo));
        dB1.push(b); b = dB1.at(S(4217)); dampB = (1 - damp) * b + damp * dampB; b = ap2B.process(dampB * decay); dB2.push(b);
        // Dattorro's 14 output taps
        outL = 0.6 * (dB1.at(S(266)) + dB1.at(S(2974)) - ap2B.d.at(S(1913)) + dB2.at(S(1996)) - dA1.at(S(1990)) - ap2A.d.at(S(187)) - dA2.at(S(1066)));
        outR = 0.6 * (dA1.at(S(353)) + dA1.at(S(3627)) - ap2A.d.at(S(1228)) + dA2.at(S(2673)) - dB1.at(S(2111)) - ap2B.d.at(S(335)) - dB2.at(S(121)));
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  SPRING  (dispersive allpass cascade + band-limit inside a feedback delay)
// ═════════════════════════════════════════════════════════════════════════════
struct SpringConfig {
    double tdMs = 40.0;     // echo (round-trip) time at mid band
    double fcHz = 4500.0;   // transition frequency: top of the chirp band
    int    M = 80;          // stretched allpass sections
    double a1 = 0.62;       // their coefficient
    double t60 = 2.5;
    double hpHz = 100.0;
    double wobbleMs = 0.0, wobbleHz = 0.0;
};

struct Spring {
    SpringConfig c; double fs = 48000; int K = 5; double g = 0.5, loopLen = 1, ph = 0;
    std::vector<Vec> apBuf; std::vector<double> apState; int apW = 0;   // each section: K-sample delay of its state
    Delay loop; Biquad l1, l2, l3, h1;

    // group delay (samples) of ONE stretched allpass (a + z^-K)/(1 + a z^-K) at f
    double apGroupDelay(double f) const { const double w = 2 * kPi * f / fs * K; return K * (1 - c.a1 * c.a1) / (1 + 2 * c.a1 * std::cos(w) + c.a1 * c.a1); }

    void prepare(double sampleRate, const SpringConfig& cfg) {
        fs = sampleRate; c = cfg; K = std::max(1, (int) std::lround(fs / (2.0 * c.fcHz)));
        apBuf.assign((size_t) c.M, Vec((size_t) K, 0.0)); apW = 0;
        // 6th-order low-pass at the transition frequency keeps the loop inside the chirp band
        const double fl = std::min(0.95 * fs / (2.0 * K), c.fcHz);
        l1 = Biquad::lp(fs, fl, 0.5176); l2 = Biquad::lp(fs, fl, 0.7071); l3 = Biquad::lp(fs, fl, 1.9319); h1 = Biquad::hp(fs, c.hpHz, 0.7071);
        // echo time at 1 kHz = loop delay + cascade group delay there
        const double disp = c.M * apGroupDelay(1000.0);
        loopLen = std::max(4.0, c.tdMs * 0.001 * fs - disp);
        loop.prepare((int) loopLen + (int) (c.wobbleMs * 0.001 * fs) + 64);
        setT60(c.t60);
    }
    double echoSeconds(double f) const { return (loopLen + c.M * apGroupDelay(f)) / fs; }
    void setT60(double t) { c.t60 = t; g = std::pow(10.0, -3.0 * echoSeconds(1000.0) / t); }

    double process(double x) {
        double d = loopLen;
        if (c.wobbleMs > 0) { ph += 2 * kPi * c.wobbleHz / fs; if (ph > 2 * kPi) ph -= 2 * kPi; d += c.wobbleMs * 0.001 * fs * std::sin(ph); }
        double y = loop.readCubic(d);
        for (int m = 0; m < c.M; ++m) {               // (a + z^-K) / (1 + a z^-K), transposed direct form
            double& s = apBuf[(size_t) m][(size_t) apW];
            const double out = c.a1 * y + s; s = y - c.a1 * out; y = out;
        }
        apW = (apW + 1) % K;
        y = h1(l3(l2(l1(y))));
        loop.push(x - g * y);                          // inverting reflection at the spring end
        return y;
    }
};
