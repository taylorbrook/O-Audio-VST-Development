// R6 / R5 / R9 scratch: JUCE FFT contract, UPOLS-vs-direct gate on a random bank,
// dual-bank fade + queue, WindowedSinc resample dumps, PERF-02 shape.
#include <JuceHeader.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>
#include <fstream>

static int failures = 0;
static void check (const char* name, bool ok, const juce::String& detail = {})
{
    std::printf ("  [%s] %s%s%s\n", ok ? "PASS" : "FAIL", name, detail.isEmpty() ? "" : " — ", detail.toRawUTF8());
    if (! ok) ++failures;
}

constexpr int P = 64, N = 2 * P, ORDER = 7, NB = P + 1;   // 65 bins

// ───────────────────────── Part A: FFT contract ─────────────────────────────
static void partA()
{
    std::printf ("Part A — juce::dsp::FFT (order %d, N %d) contract\n", ORDER, N);
    juce::dsp::FFT fft (ORDER);
    std::vector<float> buf ((size_t) 2 * N, 0.0f);

    // A1: impulse at n = 3 → X[k] = e^{-2πi·3k/N}
    std::fill (buf.begin(), buf.end(), 0.0f);
    buf[3] = 1.0f;
    fft.performRealOnlyForwardTransform (buf.data(), true);
    double worst = 0.0;
    for (int k = 0; k < NB; ++k)
    {
        const double re = std::cos (-2.0 * M_PI * 3.0 * k / N), im = std::sin (-2.0 * M_PI * 3.0 * k / N);
        worst = std::max (worst, std::max (std::abs (buf[(size_t) 2 * k] - re), std::abs (buf[(size_t) 2 * k + 1] - im)));
    }
    check ("A1 forward(flag=true): bins 0..64 interleaved re/im = DFT (no scale), impulse at 3", worst < 1e-6, juce::String (worst, 9));
    std::printf ("    bin0 (%.6f, %.6f) bin64 (%.6f, %.6f) float[128..129] ; floats 130..133 = %.3g %.3g %.3g %.3g\n",
                 buf[0], buf[1], buf[128], buf[129], buf[130], buf[131], buf[132], buf[133]);

    // A2: flag=false → upper bins are conjugates
    std::fill (buf.begin(), buf.end(), 0.0f);
    buf[3] = 1.0f;
    fft.performRealOnlyForwardTransform (buf.data(), false);
    worst = 0.0;
    for (int k = NB; k < N; ++k)
        worst = std::max (worst, (double) std::max (std::abs (buf[(size_t) 2 * k] - buf[(size_t) 2 * (N - k)]),
                                           std::abs (buf[(size_t) 2 * k + 1] + buf[(size_t) 2 * (N - k) + 1])));
    check ("A2 forward(flag=false): bins 65..127 = conj (bins 63..1)", worst < 1e-6, juce::String (worst, 9));

    // A3: DC and Nyquist placement
    std::fill (buf.begin(), buf.end(), 0.0f);
    for (int n = 0; n < N; ++n) buf[(size_t) n] = (n & 1) ? -1.0f : 1.0f;
    fft.performRealOnlyForwardTransform (buf.data(), true);
    check ("A3 alternating ±1 → bin 64 (floats 128/129) = (128, 0), bin 0 = (0, 0)",
           std::abs (buf[128] - 128.0f) < 1e-4f && std::abs (buf[129]) < 1e-4f && std::abs (buf[0]) < 1e-4f && std::abs (buf[1]) < 1e-4f,
           juce::String::formatted ("bin64 (%.4f, %.4f) bin0 (%.4f, %.4f)", buf[128], buf[129], buf[0], buf[1]));

    // A4: round trip with 1/N in the inverse
    std::mt19937 rng (7);
    std::normal_distribution<float> nd (0.0f, 1.0f);
    std::vector<float> x ((size_t) N);
    for (auto& v : x) v = nd (rng);
    std::fill (buf.begin(), buf.end(), 0.0f);
    std::copy (x.begin(), x.end(), buf.begin());
    fft.performRealOnlyForwardTransform (buf.data(), true);
    fft.performRealOnlyInverseTransform (buf.data());
    worst = 0.0;
    for (int n = 0; n < N; ++n) worst = std::max (worst, (double) std::abs (buf[(size_t) n] - x[(size_t) n]));
    double tail = 0.0;
    for (int n = N; n < 2 * N; ++n) tail = std::max (tail, (double) std::abs (buf[(size_t) n]));
    check ("A4 forward(flag=true) → inverse == x (inverse carries 1/N; no explicit scaling needed)", worst < 1e-5, juce::String (worst, 9) + " tail max " + juce::String (tail, 3));

    // A5: inverse ignores bins 65..127 (garbage there) — Apple engine behaviour; Fallback mirrors internally
    std::fill (buf.begin(), buf.end(), 0.0f);
    std::copy (x.begin(), x.end(), buf.begin());
    fft.performRealOnlyForwardTransform (buf.data(), true);
    for (int i = 2 * NB; i < 2 * N; ++i) buf[(size_t) i] = 1234.5f;   // garbage in the negative-frequency half
    fft.performRealOnlyInverseTransform (buf.data());
    worst = 0.0;
    for (int n = 0; n < N; ++n) worst = std::max (worst, (double) std::abs (buf[(size_t) n] - x[(size_t) n]));
    check ("A5 inverse with garbage in bins 65..127 still == x (only bins 0..64 are read)", worst < 1e-5, juce::String (worst, 9));

    // A6: convolution theorem — IFFT(X ⊙ H)[64..127] == linear conv of [prev|cur] with h(64 taps) at 64..127
    std::vector<float> h ((size_t) P), xx ((size_t) N);
    for (auto& v : h) v = nd (rng) * 0.1f;
    for (auto& v : xx) v = nd (rng);
    std::vector<float> X ((size_t) 2 * N, 0.0f), H ((size_t) 2 * N, 0.0f);
    std::copy (xx.begin(), xx.end(), X.begin());
    std::copy (h.begin(), h.end(), H.begin());
    fft.performRealOnlyForwardTransform (X.data(), true);
    fft.performRealOnlyForwardTransform (H.data(), true);
    std::vector<float> Y ((size_t) 2 * N, 0.0f);
    for (int k = 0; k < NB; ++k)
    {
        const float ar = X[(size_t) 2 * k], ai = X[(size_t) 2 * k + 1], br = H[(size_t) 2 * k], bi = H[(size_t) 2 * k + 1];
        Y[(size_t) 2 * k]     = ar * br - ai * bi;
        Y[(size_t) 2 * k + 1] = ar * bi + ai * br;
    }
    fft.performRealOnlyInverseTransform (Y.data());
    worst = 0.0;
    for (int n = P; n < N; ++n)
    {
        double ref = 0.0;
        for (int t = 0; t < P; ++t) ref += (double) h[(size_t) t] * xx[(size_t) (n - t)];
        worst = std::max (worst, std::abs (ref - Y[(size_t) n]));
    }
    check ("A6 overlap-save: IFFT(X⊙H)[64..127] == linear convolution (complex MAC on bins 0..64 only)", worst < 1e-5, juce::String (worst, 9));
}

// ───────────────────── Part B: UPOLS convolver (D1 shape) ───────────────────
struct Bank { int nch = 16; int taps = 256; std::vector<std::vector<std::vector<float>>> h; }; // [ch][ear][tap]

struct Upols
{
    juce::dsp::FFT fft { ORDER };
    int K = 0;
    std::vector<int> nchActive;
    std::vector<std::vector<float>> spec;        // [bank] → [ch][ear][k][2*NB]
    std::vector<float> fdl;                      // [ch][k][2*NB]
    std::vector<float> prev;                     // [ch][P]
    float in[16][P] {};
    float out[2][P] {};
    float outB[2][P] {};                         // second bank during a fade
    std::vector<float> tmp, acc[2], accB[2];
    int fdlHead = 0;
    // bank state machine
    int cur = 0, pending = -1; bool fading = false; float mixCur = 0.0f, mixPrev = 0.0f; float rampStep = 1.0f / 15.0f;
    int queued = -1;

    void prepare (const std::vector<Bank>& banks, double fs)
    {
        K = (banks[0].taps + P - 1) / P;
        rampStep = (float) (64.0 / (0.02 * fs));
        spec.assign (banks.size(), {}); nchActive.assign (banks.size(), 16);
        for (int s = 0; s < (int) banks.size(); ++s)
        {
            const Bank& bk = banks[(size_t) s];
            spec[(size_t) s].assign ((size_t) 16 * 2 * K * 2 * NB, 0.0f);
            nchActive[(size_t) s] = bk.nch;
            for (int ch = 0; ch < bk.nch; ++ch)
                for (int ear = 0; ear < 2; ++ear)
                    for (int k = 0; k < K; ++k)
                    {
                        std::vector<float> buf ((size_t) 2 * N, 0.0f);
                        for (int t = 0; t < P; ++t)
                        {
                            const int idx = k * P + t;
                            buf[(size_t) t] = idx < bk.taps ? bk.h[(size_t) ch][(size_t) ear][(size_t) idx] : 0.0f;
                        }
                        fft.performRealOnlyForwardTransform (buf.data(), true);
                        std::copy (buf.begin(), buf.begin() + 2 * NB, spec[(size_t) s].begin() + (long) specIdx (ch, ear, k));
                    }
        }
        fdl.assign ((size_t) 16 * K * 2 * NB, 0.0f);
        prev.assign ((size_t) 16 * P, 0.0f);
        tmp.assign ((size_t) 2 * N, 0.0f);
        for (int e = 0; e < 2; ++e) { acc[e].assign ((size_t) 2 * N, 0.0f); accB[e].assign ((size_t) 2 * N, 0.0f); }
        std::memset (in, 0, sizeof (in)); std::memset (out, 0, sizeof (out)); std::memset (outB, 0, sizeof (outB));
        fdlHead = 0; cur = 0; pending = -1; fading = false; mixCur = mixPrev = 0.0f; queued = -1;
    }
    size_t specIdx (int ch, int ear, int k) const { return (size_t) (((ch * 2 + ear) * K + k) * 2 * NB); }
    size_t fdlIdx (int ch, int slot) const { return (size_t) ((ch * K + slot) * 2 * NB); }

    // latest wins while fading; a request for the current bank while idle is a no-op
    void requestBank (int b) { if (fading) { queued = b; return; } if (b == cur) return; pending = b; fading = true; mixCur = 0.0f; mixPrev = 0.0f; }

    // control step: runs at gridPos == 0, BEFORE the first sample of the new grid is rendered.
    void step()
    {
        // bank ramp (A3 form, LinearRamp exact arrival)
        mixPrev = mixCur;
        if (fading)
        {
            const float d = 1.0f - mixCur;
            mixCur = d <= rampStep ? 1.0f : mixCur + rampStep;
        }
        // 16 input FFTs — [prev | cur]
        fdlHead = (fdlHead + K - 1) % K;              // newest slot
        for (int ch = 0; ch < 16; ++ch)
        {
            std::copy (prev.begin() + ch * P, prev.begin() + (ch + 1) * P, tmp.begin());
            std::copy (in[ch], in[ch] + P, tmp.begin() + P);
            std::fill (tmp.begin() + N, tmp.end(), 0.0f);
            std::copy (in[ch], in[ch] + P, prev.begin() + ch * P);
            fft.performRealOnlyForwardTransform (tmp.data(), true);
            std::copy (tmp.begin(), tmp.begin() + 2 * NB, fdl.begin() + (long) fdlIdx (ch, fdlHead));
        }
        auto accumulate = [&] (int bank, std::vector<float>* dst)
        {
            for (int e = 0; e < 2; ++e) std::fill (dst[e].begin(), dst[e].end(), 0.0f);
            for (int ch = 0; ch < nchActive[(size_t) bank]; ++ch)
                for (int k = 0; k < K; ++k)
                {
                    const float* X = &fdl[fdlIdx (ch, (fdlHead + k) % K)];
                    for (int e = 0; e < 2; ++e)
                    {
                        const float* H = &spec[(size_t) bank][specIdx (ch, e, k)];
                        float* A = dst[e].data();
                        for (int b = 0; b < NB; ++b)
                        {
                            const float ar = X[2 * b], ai = X[2 * b + 1], br = H[2 * b], bi = H[2 * b + 1];
                            A[2 * b]     += ar * br - ai * bi;
                            A[2 * b + 1] += ar * bi + ai * br;
                        }
                    }
                }
            for (int e = 0; e < 2; ++e) fft.performRealOnlyInverseTransform (dst[e].data());
        };
        accumulate (cur, acc);
        for (int e = 0; e < 2; ++e) std::copy (acc[e].begin() + P, acc[e].begin() + N, out[e]);
        if (fading)
        {
            accumulate (pending, accB);
            for (int e = 0; e < 2; ++e) std::copy (accB[e].begin() + P, accB[e].begin() + N, outB[e]);
        }
    }
    // after the grid's samples are rendered: arrival handling at the NEXT step boundary
    void endOfGrid()
    {
        if (fading && mixCur == 1.0f)
        {
            cur = pending; pending = -1; fading = false; mixCur = mixPrev = 0.0f;
            if (queued >= 0) { const int q = queued; queued = -1; requestBank (q); }   // a queued request for cur is a no-op
        }
    }
    void push (const float* tap16, int pos) { for (int ch = 0; ch < 16; ++ch) in[ch][pos] = tap16[ch]; }
    float outputAt (int ear, int pos, float t) const
    {
        if (! fading) return out[ear][pos];
        const float m = mixPrev + t * (mixCur - mixPrev);
        return out[ear][pos] + m * (outB[ear][pos] - out[ear][pos]);
    }
};

// engine-shaped driver: persistent gridPos, step at gridPos == 0, push then read.
struct Driver
{
    Upols& u; int gridPos = 0;
    template <typename Hook>
    void process (const float* const* x16, float* L, float* R, int n, int absStart, Hook hook)
    {
        int i = 0;
        while (i < n)
        {
            if (gridPos == 0) { hook (absStart + i); u.step(); }
            const int run = std::min (n - i, P - gridPos);
            for (int j = 0; j < run; ++j)
            {
                float tap[16];
                for (int ch = 0; ch < 16; ++ch) tap[ch] = x16[ch][i + j];
                const float t = (float) (gridPos + j + 1) / (float) P;
                u.push (tap, gridPos + j);
                L[i + j] = u.outputAt (0, gridPos + j, t);
                R[i + j] = u.outputAt (1, gridPos + j, t);
            }
            i += run; gridPos += run;
            if (gridPos == P) { gridPos = 0; u.endOfGrid(); }
        }
    }
};

static Bank randomBank (unsigned seed, int nch, int taps)
{
    std::mt19937 rng (seed); std::normal_distribution<float> nd (0.0f, 1.0f);
    Bank b; b.nch = nch; b.taps = taps; b.h.assign (16, std::vector<std::vector<float>> (2, std::vector<float> ((size_t) taps, 0.0f)));
    for (int ch = 0; ch < nch; ++ch) for (int e = 0; e < 2; ++e) for (int t = 0; t < taps; ++t)
        b.h[(size_t) ch][(size_t) e][(size_t) t] = nd (rng) * std::exp (-3.0f * t / taps) / std::sqrt ((float) taps);
    return b;
}

// direct-form double reference: y[n] = Σ_ch Σ_t h[ch][e][t] · x[ch][n − 64 − t]
static std::vector<double> directRef (const Bank& b, const std::vector<std::vector<float>>& x, int total, int ear)
{
    std::vector<double> y ((size_t) total, 0.0);
    for (int ch = 0; ch < b.nch; ++ch)
        for (int n = 0; n < total; ++n)
        {
            double s = 0.0;
            for (int t = 0; t < b.taps; ++t)
            {
                const int m = n - P - t;
                if (m < 0) break;
                s += (double) b.h[(size_t) ch][(size_t) ear][(size_t) t] * x[(size_t) ch][(size_t) m];
            }
            y[(size_t) n] += s;
        }
    return y;
}

static void partB()
{
    std::printf ("Part B — UPOLS (D1 grid-synchronous) vs direct-form double, random bank\n");
    std::mt19937 rng (11); std::normal_distribution<float> nd (0.0f, 0.5f);
    const int total = 8192 + 64;
    std::vector<std::vector<float>> x (16, std::vector<float> ((size_t) total));
    for (auto& c : x) for (auto& v : c) v = nd (rng);
    std::vector<const float*> xp; for (auto& c : x) xp.push_back (c.data());

    for (int taps : { 256, 512 })
    {
        const Bank A = randomBank (1, 16, taps), B = randomBank (2, 16, taps);
        const auto refL = directRef (A, x, total, 0), refR = directRef (A, x, total, 1);
        double peak = 0.0; for (auto v : refL) peak = std::max (peak, std::abs (v));
        for (int block : { 32, 64, 100, 1024, 4096 })
        {
            Upols u; u.prepare ({ A, B }, 48000.0); Driver d { u };
            std::vector<float> L ((size_t) total), R ((size_t) total);
            std::vector<const float*> slice (16);
            for (int pos = 0; pos < total; pos += block)
            {
                const int len = std::min (block, total - pos);
                for (int ch = 0; ch < 16; ++ch) slice[(size_t) ch] = xp[(size_t) ch] + pos;
                d.process (slice.data(), L.data() + pos, R.data() + pos, len, pos, [] (int) {});
            }
            double worst = 0.0;
            for (int n = 0; n < total; ++n) worst = std::max (worst, std::max (std::abs (L[(size_t) n] - refL[(size_t) n]), std::abs (R[(size_t) n] - refR[(size_t) n])));
            bool headZero = true; for (int n = 0; n < P; ++n) headZero = headZero && L[(size_t) n] == 0.0f && R[(size_t) n] == 0.0f;
            check (juce::String::formatted ("B taps %d (K %d) host block %d: max|err| < 1e-5, out[0..63] == 0", taps, u.K, block).toRawUTF8(),
                   worst < 1e-5 && headZero, juce::String::formatted ("max|err| %.3e, ref peak %.3f", worst, peak));
        }
        // bit-identity across slicings
        auto render = [&] (const std::vector<int>& blocks)
        {
            Upols u; u.prepare ({ A, B }, 48000.0); Driver d { u };
            std::vector<float> L ((size_t) total), R ((size_t) total); std::vector<const float*> slice (16);
            int pos = 0, idx = 0;
            while (pos < total) { const int len = std::min (blocks[(size_t) (idx++ % blocks.size())], total - pos);
                for (int ch = 0; ch < 16; ++ch) slice[(size_t) ch] = xp[(size_t) ch] + pos;
                d.process (slice.data(), L.data() + pos, R.data() + pos, len, pos, [] (int) {}); pos += len; }
            return std::make_pair (L, R);
        };
        const auto a = render ({ 64 }), b = render ({ 1, 7, 100, 4096, 33 });
        check (juce::String::formatted ("B taps %d: ragged slicing {1,7,100,4096,33} bit-equal to 64", taps).toRawUTF8(), a.first == b.first && a.second == b.second);
    }

    // dual-bank fade + queue vs direct-form fade in double
    {
        const Bank A = randomBank (1, 16, 256), B = randomBank (2, 16, 256), C = randomBank (3, 16, 256);
        const auto rA = directRef (A, x, total, 0), rB = directRef (B, x, total, 0), rC = directRef (C, x, total, 0);
        const std::vector<double>* bank[3] = { &rA, &rB, &rC };
        // requests sampled at the control step (hook before step): B at 1024, C at 1024+7·64 (mid-fade → queued),
        // A at 1024+9·64 (still mid-fade → latest wins, replaces C), then C again at 4096 (idle → immediate).
        struct Req { int at, bank; };
        const std::vector<Req> reqs { { 1024, 1 }, { 1024 + 7 * 64, 2 }, { 1024 + 9 * 64, 0 }, { 4096, 2 } };
        auto refFade = [&]
        {
            std::vector<double> y ((size_t) total); float mixCur = 0, mixPrev = 0; bool fading = false; int cur = 0, pend = -1, queued = -1;
            const float stepv = (float) (64.0 / (0.02 * 48000.0));   // FLOAT ramp: double arrives one grid later (16 vs 15 steps)
            for (int n = 0; n < total; ++n)
            {
                if (n % P == 0)
                {
                    if (fading && mixCur == 1.0f) { cur = pend; pend = -1; fading = false; mixCur = mixPrev = 0.0f;
                                                   if (queued >= 0) { const int q = queued; queued = -1; if (q != cur) { pend = q; fading = true; } } }
                    for (const auto& r : reqs) if (r.at == n) { if (fading) queued = r.bank; else if (r.bank != cur) { pend = r.bank; fading = true; mixCur = mixPrev = 0.0f; } }
                    mixPrev = mixCur; if (fading) { const float d = 1.0f - mixCur; mixCur = d <= stepv ? 1.0f : mixCur + stepv; }
                }
                const double t = (double) (n % P + 1) / P;
                const double a = (*bank[cur])[(size_t) n];
                y[(size_t) n] = fading ? a + (double) (mixPrev + (float) t * (mixCur - mixPrev)) * ((*bank[pend])[(size_t) n] - a) : a;
            }
            return y;
        };
        const auto ref = refFade();
        for (int block : { 64, 1024 })
        {
            Upols u; u.prepare ({ A, B, C }, 48000.0); Driver d { u };
            std::vector<float> L ((size_t) total), R ((size_t) total); std::vector<const float*> slice (16);
            int arrivals = 0;
            for (int pos = 0; pos < total; pos += block)
            {
                const int len = std::min (block, total - pos);
                for (int ch = 0; ch < 16; ++ch) slice[(size_t) ch] = xp[(size_t) ch] + pos;
                d.process (slice.data(), L.data() + pos, R.data() + pos, len, pos, [&] (int n)
                {
                    for (const auto& r : reqs) if (r.at == n) u.requestBank (r.bank);
                });
            }
            (void) arrivals;
            double worst = 0.0; int worstN = 0;
            for (int n = 0; n < total; ++n) { const double e = std::abs (L[(size_t) n] - ref[(size_t) n]); if (e > worst) { worst = e; worstN = n; } }
            // liveness: the render must differ from a no-fade render of A and from B
            double dA = 0.0, dC = 0.0; for (int n = 2048; n < 3000; ++n) { dA = std::max (dA, std::abs (L[(size_t) n] - rA[(size_t) n])); dC = std::max (dC, std::abs (L[(size_t) n] - rC[(size_t) n])); }
            check (juce::String::formatted ("B bank fade A->B @1024, C queued @+7, A queued @+9 (latest wins), C @4096, block %d: max|err| vs direct-form fade < 1e-5", block).toRawUTF8(),
                   worst < 1e-5 && u.cur == 2 && ! u.fading, juce::String::formatted ("max|err| %.3e at n %d; final cur %d fading %d; liveness |L-A| %.3f |L-C| %.3f over 2048..3000", worst, worstN, u.cur, (int) u.fading, dA, dC));
        }
        // negative control: a SNAPPED bank switch (no fade) vs the fading reference must fail by a wide margin
        {
            double worst = 0.0; for (int n = 1024; n < 1024 + 15 * 64; ++n) worst = std::max (worst, std::abs (rB[(size_t) n] - ref[(size_t) n]));
            check ("B negative control: snapped A->B switch differs from the faded reference by >> 1e-5", worst > 1e-2, juce::String::formatted ("max|diff| %.3e", worst));
        }
    }
}

// ───────────── Part C: WindowedSinc resample dumps for the R5 scipy comparison ─────────────
static void partC()
{
    std::printf ("Part C — WindowedSinc resample dumps\n");
    // a realistic test FIR: decaying band-limited noise with a 20-sample lead (HRIR-like)
    std::mt19937 rng (5); std::normal_distribution<float> nd (0.0f, 1.0f);
    std::vector<float> h (256, 0.0f);
    for (int t = 20; t < 256; ++t) h[(size_t) t] = nd (rng) * std::exp (-4.0f * (t - 20) / 236.0f);
    // smooth it a little (3-tap) so it is not full-band white
    std::vector<float> hs (256, 0.0f);
    for (int t = 1; t < 255; ++t) hs[(size_t) t] = 0.25f * h[(size_t) t - 1] + 0.5f * h[(size_t) t] + 0.25f * h[(size_t) t + 1];
    const auto dump = [] (const char* name, const std::vector<float>& v) { std::ofstream f (name); f.precision (9); for (auto x : v) f << std::scientific << x << "\n"; };
    dump ("h48.txt", hs);
    for (double fs : { 96000.0, 44100.0, 88200.0, 192000.0 })
    {
        const double ratio = 48000.0 / fs;                                   // input samples per output sample
        const int zpre = (fs == 44100.0 || fs == 88200.0 || fs == 176400.0) ? 60 : 0;   // makes (100 + zpre)/ratio an integer
        const int nIn = zpre + 256 + 400;
        std::vector<float> in ((size_t) nIn, 0.0f);
        std::copy (hs.begin(), hs.end(), in.begin() + zpre);
        const int nOut = (int) std::floor (nIn / ratio);
        std::vector<float> out ((size_t) nOut, 0.0f);
        juce::Interpolators::WindowedSinc ws; ws.reset();
        const int used = ws.process (ratio, in.data(), out.data(), nOut, nIn, 0);
        char name[64]; std::snprintf (name, sizeof (name), "h%d_juce.txt", (int) fs);
        dump (name, out);
        std::printf ("    fs %.0f ratio %.6f zpre %d nIn %d nOut %d used %d expected delay (100+zpre)/ratio = %.3f out samples\n", fs, ratio, zpre, nIn, nOut, used, (100 + zpre) / ratio);
    }
    // ratio 1: pure delay? (contract check)
    {
        std::vector<float> in (656, 0.0f); std::copy (hs.begin(), hs.end(), in.begin());
        std::vector<float> out (656, 0.0f);
        juce::Interpolators::WindowedSinc ws; ws.reset();
        ws.process (1.0, in.data(), out.data(), 656, 656, 0);
        int best = -1; double bestErr = 1e9;
        for (int d = 0; d < 300; ++d) { double e = 0; for (int t = 0; t < 256; ++t) e = std::max (e, (double) std::abs (out[(size_t) (t + d)] - hs[(size_t) t])); if (e < bestErr) { bestErr = e; best = d; } }
        std::printf ("    ratio 1.0: best integer delay %d, max|err| %.3e (identity at integer positions?)\n", best, bestErr);
        dump ("h48000_juce.txt", out);
    }
}

// ───────────── Part D: PERF-02 shape — wall time of 10 s at order 3, K = 4 and K = 8 ─────────────
static void partD()
{
    std::printf ("Part D — throughput (Release? %s)\n",
#ifdef NDEBUG
    "yes"
#else
    "NO — debug numbers are meaningless"
#endif
    );
    for (int taps : { 256, 512 })
    {
        const Bank A = randomBank (1, 16, taps), B = randomBank (2, 16, taps);
        Upols u; u.prepare ({ A, B }, 48000.0); Driver d { u };
        const int total = 480000; std::vector<float> L (64), R (64);
        std::vector<std::vector<float>> x (16, std::vector<float> (64, 0.1f)); std::vector<const float*> xp; for (auto& c : x) xp.push_back (c.data());
        const auto t0 = std::chrono::steady_clock::now();
        for (int pos = 0; pos < total; pos += 64) d.process (xp.data(), L.data(), R.data(), 64, pos, [&] (int n) { if (n == 96000) u.requestBank (1); if (n == 192000) u.requestBank (0); });
        const double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        const double audio = total / (taps == 256 ? 48000.0 : 96000.0);
        std::printf ("    taps %d (K %d): %d samples in %.3f s → %.0f× real time at %s (sink %.3f)\n", taps, u.K, total, s, audio / s, taps == 256 ? "48 k" : "96 k", L[3]);
    }
}

int main()
{
    partA(); partB(); partC(); partD();
    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
