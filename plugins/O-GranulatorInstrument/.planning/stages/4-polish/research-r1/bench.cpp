// Scratch micro-benchmark: ReverbUnit 1.2.0 (read-only include by absolute path).
#include "/Users/taylorbrook/Dev/VST-development/modules/effects/reverb-unit/cpp/ReverbUnit.h"
#include <ctime>
#include <cstdio>
#include <random>
#include <algorithm>
#include <cstring>
static double thr() { timespec ts{}; clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
struct Tank {
    ReverbUnit r; std::atomic<float> en{1}, size{.5f}, damp{.5f}, pre{20}, mix{0}, mod{.2f}, shim{0};
    Tank() { r.bind (&en,&size,&damp,&pre,&mix,&mod,&shim); }
};
static std::vector<float> noise (int n, unsigned seed, float a) { std::mt19937 g (seed); std::uniform_real_distribution<float> d (-a, a); std::vector<float> v ((size_t) n); for (auto& x : v) x = d (g); return v; }

// % of one core = thread-cpu / audio seconds (measure25's unit).
static double pct (int tanks, int block, float mixV, float sizeV, double fs, double seconds, bool silentIn)
{
    std::vector<std::unique_ptr<Tank>> t;
    for (int i = 0; i < tanks; ++i) { t.push_back (std::make_unique<Tank>()); t.back()->mix = mixV; t.back()->size = sizeV; t.back()->r.prepare (fs, block); }
    const int total = (int) (seconds * fs);
    auto src = noise (2 * (total + block), 5, silentIn ? 0.0f : 0.1f);
    std::vector<float> L ((size_t) block), R ((size_t) block);
    float* ch[2] { L.data(), R.data() };
    juce::AudioBuffer<float> view; view.setDataToReferTo (ch, 2, block);
    double best = 1e30; volatile float sink = 0;
    for (int rep = 0; rep < 5; ++rep)
    {
        double cpu = 0;
        for (int pos = 0; pos + block <= total; pos += block)
            for (auto& u : t)
            {
                std::memcpy (L.data(), &src[(size_t) (2 * pos)], sizeof (float) * (size_t) block);
                std::memcpy (R.data(), &src[(size_t) (2 * pos + block)], sizeof (float) * (size_t) block);
                const double a = thr();
                u->r.process (view);
                cpu += thr() - a;
                sink = sink + L[0];
            }
        best = std::min (best, 100.0 * cpu / (seconds * 1e9));
    }
    return best;
}

// Null test: 4 identical Post tanks (one per stream, wet·tank + dry) summed vs 1 tank on the sum.
static void nullTest (float mixV, float sizeV, float amp, int streams, const char* tag)
{
    const double fs = 48000; const int block = 32, total = 48000 * 8;
    Tank one; one.mix = mixV; one.size = sizeV; one.r.prepare (fs, block);
    std::vector<std::unique_ptr<Tank>> four;
    for (int i = 0; i < streams; ++i) { four.push_back (std::make_unique<Tank>()); four.back()->mix = mixV; four.back()->size = sizeV; four.back()->r.prepare (fs, block); }
    std::vector<std::vector<float>> sL, sR;
    for (int i = 0; i < streams; ++i) { sL.push_back (noise (total, 11 + i, amp)); sR.push_back (noise (total, 31 + i, amp)); }
    // input only in the first 4 s, tail for 4 s
    for (int i = 0; i < streams; ++i) for (int n = total / 2; n < total; ++n) sL[i][n] = sR[i][n] = 0.0f;
    std::vector<float> aL (block), aR (block), bL (block), bR (block), tL (block), tR (block);
    double errPk = 0, refPk = 0, errSq = 0, refSq = 0; long differ = 0, count = 0;
    for (int pos = 0; pos + block <= total; pos += block)
    {
        for (int n = 0; n < block; ++n) { float l = sL[0][pos+n], r = sR[0][pos+n]; for (int i = 1; i < streams; ++i) { l += sL[i][pos+n]; r += sR[i][pos+n]; } aL[n] = l; aR[n] = r; }
        float* ca[2] { aL.data(), aR.data() }; juce::AudioBuffer<float> va; va.setDataToReferTo (ca, 2, block); one.r.process (va);
        for (int i = 0; i < streams; ++i)
        {
            std::memcpy (tL.data(), &sL[i][pos], sizeof (float) * block); std::memcpy (tR.data(), &sR[i][pos], sizeof (float) * block);
            float* ct[2] { tL.data(), tR.data() }; juce::AudioBuffer<float> vt; vt.setDataToReferTo (ct, 2, block); four[i]->r.process (vt);
            for (int n = 0; n < block; ++n) { if (i == 0) { bL[n] = tL[n]; bR[n] = tR[n]; } else { bL[n] += tL[n]; bR[n] += tR[n]; } }
        }
        for (int n = 0; n < block; ++n)
        {
            const double e = (double) aL[n] - bL[n]; errPk = std::max (errPk, std::abs (e)); refPk = std::max (refPk, (double) std::abs (aL[n]));
            errSq += e * e; refSq += (double) aL[n] * aL[n]; differ += (aL[n] != bL[n]) + (aR[n] != bR[n]); count += 2;
        }
    }
    std::printf ("NULL %-28s streams %d mix %.2f size %.2f amp %.2f: differ %ld / %ld samples, err peak %.3g (%.1f dB re ref peak %.3f), err RMS %.1f dB re ref RMS\n",
                 tag, streams, mixV, sizeV, amp, differ, count, errPk, 20 * std::log10 (std::max (errPk, 1e-30) / refPk), refPk, 10 * std::log10 (std::max (errSq, 1e-60) / refSq));
}

int main()
{
    const double fs = 48000;
    std::printf ("sizeof(ReverbUnit) = %zu bytes (heap excluded)\n", sizeof (ReverbUnit));
    for (int block : { 32, 128, 512 })
    {
        std::printf ("block %4d: 1 tank mix 0 (idle) %.4f %% | 1 tank mix .45 size .5 %.3f %% | size 1.0 mix 1.0 %.3f %% | silent in, mix .45 %.3f %% | 4 tanks mix .45 %.3f %% | 4 tanks mix1 size1 %.3f %%\n", block,
                     pct (1, block, 0.0f, 0.5f, fs, 10, false), pct (1, block, 0.45f, 0.5f, fs, 10, false), pct (1, block, 1.0f, 1.0f, fs, 10, false),
                     pct (1, block, 0.45f, 0.5f, fs, 10, true), pct (4, block, 0.45f, 0.5f, fs, 10, false), pct (4, block, 1.0f, 1.0f, fs, 10, false));
    }
    std::printf ("96 k block 128: 1 tank %.3f %%, 4 tanks %.3f %%\n", pct (1, 128, 0.45f, 0.5f, 96000, 10, false), pct (4, 128, 0.45f, 0.5f, 96000, 10, false));
    // prepare cost
    { Tank t; const double a = thr(); for (int i = 0; i < 200; ++i) t.r.prepare (48000, 32); std::printf ("prepare(48k): %.1f us each; reset(): ", (thr() - a) / 200 / 1000);
      const double b = thr(); for (int i = 0; i < 2000; ++i) t.r.reset(); std::printf ("%.1f us each\n", (thr() - b) / 2000 / 1000); }
    nullTest (0.45f, 0.5f, 0.05f, 4, "typical level");
    nullTest (0.45f, 0.5f, 0.05f, 2, "typical level 2 streams");
    nullTest (1.0f, 1.0f, 0.05f, 4, "mix 1 size 1");
    nullTest (0.45f, 0.5f, 0.05f, 1, "control: 1 stream");
    nullTest (1.0f, 1.0f, 1.5f, 4, "hot (tank clip +-2)");
    return 0;
}
