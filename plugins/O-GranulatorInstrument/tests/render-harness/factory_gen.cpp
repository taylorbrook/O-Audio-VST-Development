/*
  ==============================================================================

    O-GranulatorInstrument: the factory-source generator (PLAN-4 PD5 / PD6).

      O-GranulatorInstrument-render-harness --gen-factory <dir> [--force]

    Writes the 8 curated factory sources (48 kHz, 24-bit FLAC through the
    plugin's own encoder, SlotLoader::encodeFlac24 quality 5) and prints, per
    source: bytes, channels / frames, the 131 072-frame contentHash (kHash*)
    and the full-PCM hash (kFull*), then the size budget verdict (each
    <= 3 145 728 B, total <= 8 388 608 B).

    Determinism (RESEARCH-4 A§Q4): this file is its own translation unit,
    compiled with -ffp-contract=off (CMakeLists.txt) and the pragma below, so
    a + b * c is never fused into an FMA; the only randomness is splitmix64
    (integer); phases are double accumulators wrapped in [0, 1); no
    std::*_distribution, no std::pow in an inner loop. Regeneration is
    byte-identical on mac arm64 (the claim); the checked-in FLACs are what
    ships, and FLAC decode is exact on every platform (the kFull* pins).

    Pitched sources have their fundamental (the bell: its prime) at
    C3 = 130.8128 Hz, so a fresh lane plays in tune at the default
    sN_root_note 48. Tonal: -12 dBFS peak. Noise: -18 dBFS RMS on a 16-bit
    grid (FLAC wasted bits), never above -1 dBFS peak.

      id         content                                         ch  length
      harmonic   24 additive partials, slow per-partial swells    1   8 s
      pluck      KS-like decaying partials + a short noise pick    1   3 s
      bell       inharmonic 1 / 2 / 2.76 / 5.40 / 8.93 (+ hum)     1   6 s
      vowel      pulse harmonics through 3 formants a -> o -> i    1   8 s
      noise_bed  pink-ish noise, slow spectral sweep, L / R seeds  2   6 s
      dust       sparse crackle + band noise                       2   6 s
      drone      C2 / G2 / C3 stacks, decorrelated L / R detune    2  12 s
      pulse      120 bpm, 2 bars: sine-drop kick + noise hat       1   4 s

  ==============================================================================
*/

#include <JuceHeader.h>
#include "dsp/GiHash.h"
#include "dsp/SlotLoader.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#if defined (__clang__)
 #pragma clang fp contract(off)
#endif

namespace factorygen
{
    int run (const juce::File& dir, bool force);
    std::uint64_t fullPcmHash (const juce::AudioBuffer<float>& pcm);

    namespace
    {
        constexpr double kFs = 48000.0;
        constexpr double kC3 = 130.81278265029931;   // 440 * 2^(-21/12)
        constexpr double kTwoPi = 6.283185307179586476925286766559;

        //======================================================================
        struct SplitMix
        {
            std::uint64_t s;
            explicit SplitMix (std::uint64_t seed) : s (seed) {}
            std::uint64_t next()
            {
                std::uint64_t z = (s += 0x9E3779B97F4A7C15ull);
                z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
                z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
                return z ^ (z >> 31);
            }
            /** [-1, 1) from the top 53 bits (exact in double). */
            double bipolar() { return static_cast<double> (next() >> 11) * (2.0 / 9007199254740992.0) - 1.0; }
            /** [0, 1). */
            double unit() { return static_cast<double> (next() >> 11) * (1.0 / 9007199254740992.0); }
        };

        struct Phase
        {
            double p = 0.0, inc = 0.0;
            Phase (double hz, double start = 0.0) : p (start), inc (hz / kFs) {}
            double tick()
            {
                const double v = std::sin (kTwoPi * p);
                p += inc;
                if (p >= 1.0) p -= 1.0;
                return v;
            }
        };

        /** One channel buffer in double, then scaled and written as float. */
        using Chan = std::vector<double>;

        double peakOf (const std::vector<Chan>& x)
        {
            double pk = 0.0;
            for (const auto& c : x)
                for (double v : c)
                    pk = std::max (pk, std::abs (v));
            return pk;
        }

        double rmsOf (const std::vector<Chan>& x)
        {
            double acc = 0.0;
            std::size_t n = 0;
            for (const auto& c : x)
            {
                for (double v : c)
                    acc += v * v;
                n += c.size();
            }
            return n > 0 ? std::sqrt (acc / static_cast<double> (n)) : 0.0;
        }

        void fadeEnds (std::vector<Chan>& x, int inFrames, int outFrames)
        {
            for (auto& c : x)
            {
                const int n = static_cast<int> (c.size());
                for (int i = 0; i < inFrames && i < n; ++i)
                    c[(size_t) i] *= 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * (double) i / (double) inFrames);
                for (int i = 0; i < outFrames && i < n; ++i)
                    c[(size_t) (n - 1 - i)] *= 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * (double) i / (double) outFrames);
            }
        }

        /** Tonal: -12 dBFS peak. Noise: -18 dBFS RMS on a 16-bit grid, capped
            at -1 dBFS peak. */
        ogi::SlotLoader::Pristine finish (const char* id, std::vector<Chan> x, bool noise)
        {
            double g;
            if (noise)
            {
                const double gr = 0.12589254117941673 / rmsOf (x);   // -18 dBFS RMS
                const double gp = 0.89125093813374556 / peakOf (x);  // -1 dBFS peak cap
                g = std::min (gr, gp);
            }
            else
            {
                g = 0.25118864315095801 / peakOf (x);                // -12 dBFS peak
            }
            ogi::SlotLoader::Pristine p;
            p.rate = kFs;
            p.name = id;
            const int C = static_cast<int> (x.size()), n = static_cast<int> (x[0].size());
            p.pcm.setSize (C, n);
            for (int c = 0; c < C; ++c)
                for (int i = 0; i < n; ++i)
                {
                    double v = x[(size_t) c][(size_t) i] * g;
                    if (noise)
                        v = std::nearbyint (v * 32768.0) / 32768.0;   // 16-bit resolution in the 24-bit container
                    p.pcm.setSample (c, i, static_cast<float> (v));
                }
            return p;
        }

        //======================================================================
        ogi::SlotLoader::Pristine harmonic()
        {
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x4841524D4F4E4943ull);   // "HARMONIC"
            for (int k = 1; k <= 24; ++k)
            {
                const double amp = 1.0 / std::pow ((double) k, 1.1);
                Phase osc (kC3 * k, r.unit());
                Phase swell (0.05 + 0.35 * r.unit(), r.unit());
                const double depth = 0.25 + 0.5 * r.unit() * ((double) k / 24.0);
                for (int i = 0; i < n; ++i)
                {
                    const double a = amp * (1.0 - depth * (0.5 + 0.5 * swell.tick()));
                    x[0][(size_t) i] += a * osc.tick();
                }
            }
            fadeEnds (x, 480, 4800);
            return finish ("harmonic", std::move (x), false);
        }

        ogi::SlotLoader::Pristine pluck()
        {
            const int n = static_cast<int> (3.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x504C55434Bull);   // "PLUCK"
            for (int k = 1; k <= 30; ++k)
            {
                const double f = kC3 * k * (1.0 + 0.00035 * (double) (k * k));   // slight string stiffness
                if (f > 18000.0)
                    break;
                const double amp = 1.0 / (double) k * (k % 2 == 0 ? 0.7 : 1.0);
                const double tau = 1.4 / (1.0 + 0.18 * std::pow ((double) k, 1.3));   // seconds
                const double d = std::exp (-1.0 / (tau * kFs));
                Phase osc (f, r.unit());
                double env = amp;
                for (int i = 0; i < n; ++i)
                {
                    x[0][(size_t) i] += env * osc.tick();
                    env *= d;
                }
            }
            // The pick: 6 ms of decaying, low-passed noise.
            double z = 0.0, env = 0.35;
            const double d = std::exp (-1.0 / (0.002 * kFs));
            for (int i = 0; i < 288; ++i)
            {
                z += 0.5 * (r.bipolar() - z);
                x[0][(size_t) i] += env * z;
                env *= d;
            }
            fadeEnds (x, 24, 2400);
            return finish ("pluck", std::move (x), false);
        }

        ogi::SlotLoader::Pristine bell()
        {
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x42454C4Cull);   // "BELL"
            struct Partial { double ratio, amp, tau; };
            const Partial ps[] = { { 0.5, 0.35, 4.5 }, { 1.0, 1.0, 3.6 }, { 1.19, 0.25, 2.2 }, { 2.0, 0.6, 2.4 },
                                   { 2.76, 0.5, 1.6 }, { 5.40, 0.3, 0.9 }, { 8.93, 0.18, 0.5 }, { 13.34, 0.08, 0.3 } };
            for (const auto& q : ps)
            {
                const double d = std::exp (-1.0 / (q.tau * kFs));
                Phase osc (kC3 * q.ratio, r.unit());
                Phase beat (0.9 + 0.8 * r.unit(), r.unit());   // a pair of close modes beating
                double env = q.amp;
                for (int i = 0; i < n; ++i)
                {
                    x[0][(size_t) i] += env * (0.85 + 0.15 * beat.tick()) * osc.tick();
                    env *= d;
                }
            }
            double z = 0.0, env = 0.25;   // the strike
            const double d = std::exp (-1.0 / (0.0015 * kFs));
            for (int i = 0; i < 240; ++i)
            {
                z += 0.7 * (r.bipolar() - z);
                x[0][(size_t) i] += env * z;
                env *= d;
            }
            fadeEnds (x, 24, 4800);
            return finish ("bell", std::move (x), false);
        }

        ogi::SlotLoader::Pristine vowel()
        {
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x564F57454Cull);   // "VOWEL"
            // Formants (Hz, bandwidth Hz, gain): a -> o -> i.
            struct F3 { double f[3], b[3], g[3]; };
            const F3 a { { 800.0, 1150.0, 2900.0 }, { 80.0, 90.0, 120.0 }, { 1.0, 0.50, 0.18 } };
            const F3 o { { 450.0,  800.0, 2830.0 }, { 70.0, 80.0, 100.0 }, { 1.0, 0.35, 0.10 } };
            const F3 ii { { 270.0, 2140.0, 2950.0 }, { 60.0, 90.0, 100.0 }, { 1.0, 0.30, 0.22 } };
            const int K = static_cast<int> (7000.0 / kC3);
            std::vector<Phase> osc;
            for (int k = 1; k <= K; ++k)
                osc.emplace_back (kC3 * k, r.unit());
            Phase vib (5.1, 0.0);
            const int half = n / 2;
            for (int i = 0; i < n; ++i)
            {
                const F3& from = i < half ? a : o;
                const F3& to   = i < half ? o : ii;
                const double t = (double) (i < half ? i : i - half) / (double) half;
                const double s = t * t * (3.0 - 2.0 * t);   // smoothstep
                double F[3], B[3], G[3];
                for (int j = 0; j < 3; ++j)
                {
                    F[j] = from.f[j] + (to.f[j] - from.f[j]) * s;
                    B[j] = from.b[j] + (to.b[j] - from.b[j]) * s;
                    G[j] = from.g[j] + (to.g[j] - from.g[j]) * s;
                }
                const double vibAmt = 1.0 + 0.004 * vib.tick() * std::min (1.0, (double) i / (1.5 * kFs));
                double v = 0.0;
                for (int k = 1; k <= K; ++k)
                {
                    const double fk = kC3 * k * vibAmt;
                    double resp = 0.0;
                    for (int j = 0; j < 3; ++j)
                    {
                        const double u = (fk - F[j]) / (0.5 * B[j]);
                        resp += G[j] / std::sqrt (1.0 + u * u);
                    }
                    auto& ph = osc[(size_t) (k - 1)];
                    v += resp / std::sqrt ((double) k) * std::sin (kTwoPi * ph.p);
                    ph.p += ph.inc * vibAmt;
                    if (ph.p >= 1.0) ph.p -= 1.0;
                }
                x[0][(size_t) i] = v;
            }
            fadeEnds (x, 960, 4800);
            return finish ("vowel", std::move (x), false);
        }

        ogi::SlotLoader::Pristine noiseBed()
        {
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            for (int c = 0; c < 2; ++c)
            {
                SplitMix r (0x4E4F495345ull + (std::uint64_t) c * 0x1000193ull);   // "NOISE" + channel
                double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;   // Kellett pink
                double lp = 0.0;
                Phase sweep (1.0 / 6.0, c == 0 ? 0.75 : 0.80);   // one cycle over the file, L / R offset
                for (int i = 0; i < n; ++i)
                {
                    const double w = r.bipolar();
                    b0 = 0.99886 * b0 + w * 0.0555179;
                    b1 = 0.99332 * b1 + w * 0.0750759;
                    b2 = 0.96900 * b2 + w * 0.1538520;
                    b3 = 0.86650 * b3 + w * 0.3104856;
                    b4 = 0.55000 * b4 + w * 0.5329522;
                    b5 = -0.7616 * b5 - w * 0.0168980;
                    const double pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362;
                    b6 = w * 0.115926;
                    // Cutoff 250 Hz ... 6 kHz, exponential in the sweep.
                    const double s = 0.5 + 0.5 * sweep.tick();
                    const double fc = 250.0 * std::exp (s * 3.178);   // 250 * 24^s
                    const double a = 1.0 - std::exp (-kTwoPi * fc / kFs);
                    lp += a * (pink - lp);
                    x[(size_t) c][(size_t) i] = lp;
                }
            }
            fadeEnds (x, 960, 960);
            return finish ("noise_bed", std::move (x), true);
        }

        ogi::SlotLoader::Pristine dust()
        {
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            for (int c = 0; c < 2; ++c)
            {
                SplitMix r (0x44555354ull + (std::uint64_t) c * 0x9E37ull);   // "DUST" + channel
                // Band noise: the difference of two one-poles (~400 Hz ... 3 kHz), low.
                double lpA = 0.0, lpB = 0.0;
                const double aA = 1.0 - std::exp (-kTwoPi * 3000.0 / kFs);
                const double aB = 1.0 - std::exp (-kTwoPi * 400.0 / kFs);
                // Crackle: Poisson-ish clicks, ~28 / s, each a few-sample decaying burst.
                double click = 0.0, clickEnv = 0.0;
                const double clickDecay = std::exp (-1.0 / (0.0004 * kFs));
                for (int i = 0; i < n; ++i)
                {
                    const double w = r.bipolar();
                    lpA += aA * (w - lpA);
                    lpB += aB * (w - lpB);
                    const double band = (lpA - lpB) * 0.18;
                    if (r.unit() < 28.0 / kFs)
                    {
                        const double u = r.unit();
                        clickEnv = 0.15 + 0.85 * u * u * u;   // mostly soft, a few loud
                        click = r.bipolar() < 0.0 ? -1.0 : 1.0;
                    }
                    const double cr = click * clickEnv * (0.6 + 0.4 * w);
                    clickEnv *= clickDecay;
                    x[(size_t) c][(size_t) i] = band + cr;
                }
            }
            fadeEnds (x, 480, 480);
            return finish ("dust", std::move (x), true);
        }

        ogi::SlotLoader::Pristine drone()
        {
            const int n = static_cast<int> (12.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            SplitMix r (0x44524F4E45ull);   // "DRONE"
            struct Voice { double hz, amp; };
            const Voice vs[] = { { kC3 * 0.5, 1.0 }, { kC3 * 0.5 * 1.4983070768766815, 0.55 }, { kC3, 0.6 } };   // C2, G2, C3
            for (int c = 0; c < 2; ++c)
            {
                const double cents = c == 0 ? -3.5 : 3.5;
                const double det = std::exp2 (cents / 1200.0);
                for (const auto& v : vs)
                {
                    for (int h = 1; h <= 8; ++h)
                    {
                        const double amp = v.amp / (double) (h * h) * (h == 1 ? 1.0 : 0.8);
                        Phase osc (v.hz * h * det, r.unit());
                        Phase drift (0.03 + 0.09 * r.unit(), r.unit());
                        for (int i = 0; i < n; ++i)
                            x[(size_t) c][(size_t) i] += amp * (0.7 + 0.3 * drift.tick()) * osc.tick();
                    }
                }
            }
            fadeEnds (x, 4800, 9600);
            return finish ("drone", std::move (x), false);
        }

        ogi::SlotLoader::Pristine pulse()
        {
            // 120 bpm, 2 bars of 4/4 = 4.000 s exactly (a seamless loop).
            const int n = static_cast<int> (4.0 * kFs);
            const int beat = static_cast<int> (0.5 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x50554C5345ull);   // "PULSE"
            for (int b = 0; b < 8; ++b)
            {
                // Kick: sine 150 -> 45 Hz exponential drop, 280 ms decay.
                const int at = b * beat;
                double ph = 0.0, env = 1.0;
                const double ed = std::exp (-1.0 / (0.28 * kFs * (b % 4 == 0 ? 1.0 : 0.8)));
                for (int i = 0; i < beat && at + i < n; ++i)
                {
                    const double t = (double) i / kFs;
                    const double f = 45.0 + 105.0 * std::exp (-t / 0.035);
                    x[0][(size_t) (at + i)] += env * std::sin (kTwoPi * ph);
                    ph += f / kFs;
                    if (ph >= 1.0) ph -= 1.0;
                    env *= ed;
                }
                // Hat on the off-beat: high-passed noise, 30 ms.
                const int hat = at + beat / 2;
                double prev = 0.0, henv = (b % 2 == 0 ? 0.32 : 0.24);
                const double hd = std::exp (-1.0 / (0.03 * kFs));
                for (int i = 0; i < beat / 2 && hat + i < n; ++i)
                {
                    const double w = r.bipolar();
                    x[0][(size_t) (hat + i)] += henv * (w - prev);
                    prev = w;
                    henv *= hd;
                }
            }
            fadeEnds (x, 24, 240);
            return finish ("pulse", std::move (x), false);
        }

        /** XXH64 over the FULL canonical PCM stream (the contentHash layout with
            K = every frame) — the kFull* pin (PD6). */
        std::uint64_t fullHash (const juce::AudioBuffer<float>& pcm)
        {
            const int C = std::min (2, pcm.getNumChannels()), K = pcm.getNumSamples();
            ogi::gihash::Xxh64Stream h (0);
            auto put32 = [&h] (std::uint32_t v)
            {
                const std::uint8_t b[4] = { (std::uint8_t) v, (std::uint8_t) (v >> 8), (std::uint8_t) (v >> 16), (std::uint8_t) (v >> 24) };
                h.update (b, 4);
            };
            put32 ((std::uint32_t) C);
            put32 ((std::uint32_t) K);
            for (int i = 0; i < K; ++i)
                for (int c = 0; c < C; ++c)
                    put32 ((std::uint32_t) ogi::gihash::quantise (pcm.getSample (c, i)));
            return h.digest();
        }
    }

    /** The recipe table, in GiFactory.h order. */
    using Recipe = ogi::SlotLoader::Pristine (*)();
    struct RecipeRow { const char* id; Recipe make; int channels; double seconds; };
    static const RecipeRow kRecipes[] = {
        { "harmonic",  &harmonic, 1,  8.0 }, { "pluck",     &pluck,    1,  3.0 },
        { "bell",      &bell,     1,  6.0 }, { "vowel",     &vowel,    1,  8.0 },
        { "noise_bed", &noiseBed, 2,  6.0 }, { "dust",      &dust,     2,  6.0 },
        { "drone",     &drone,    2, 12.0 }, { "pulse",     &pulse,    1,  4.0 },
    };

    std::uint64_t fullPcmHash (const juce::AudioBuffer<float>& pcm) { return fullHash (pcm); }

    int run (const juce::File& dir, bool force)
    {
        dir.createDirectory();
        int rc = 0;
        std::int64_t total = 0, largest = 0;
        for (const auto& row : kRecipes)
        {
            const auto p = row.make();
            const auto f = dir.getChildFile (juce::String (row.id) + ".flac");
            if (f.exists() && ! force)
            {
                std::printf ("refusing to overwrite %s (use --force)\n", f.getFullPathName().toRawUTF8());
                rc = 1;
                continue;
            }
            juce::MemoryBlock mb;
            if (! ogi::SlotLoader::encodeFlac24 (p, mb, 5))
            {
                std::printf ("encode failed: %s\n", row.id);
                rc = 1;
                continue;
            }
            f.replaceWithData (mb.getData(), mb.getSize());
            ogi::SlotLoader::Pristine d;
            juce::String err;
            const bool ok = ogi::SlotLoader::decodeMemoryFlac (mb.getData(), mb.getSize(), row.id, 48000.0, d, err);
            const bool shape = ok && d.pcm.getNumChannels() == row.channels && d.pcm.getNumSamples() == (int) (row.seconds * kFs);
            total += (std::int64_t) mb.getSize();
            largest = std::max (largest, (std::int64_t) mb.getSize());
            double pk = 0.0, acc = 0.0;
            for (int c = 0; c < d.pcm.getNumChannels(); ++c)
                for (int i = 0; i < d.pcm.getNumSamples(); ++i)
                {
                    const double v = d.pcm.getSample (c, i);
                    pk = std::max (pk, std::abs (v));
                    acc += v * v;
                }
            const double rms = std::sqrt (acc / std::max (1, d.pcm.getNumChannels() * d.pcm.getNumSamples()));
            std::printf ("%-16s %8d bytes  %d ch %7d frames  peak %6.2f dBFS  rms %6.2f dBFS  kHash 0x%016llxull  kFull 0x%016llxull  %s\n",
                         f.getFileName().toRawUTF8(), (int) mb.getSize(), d.pcm.getNumChannels(), d.pcm.getNumSamples(),
                         20.0 * std::log10 (std::max (pk, 1e-12)), 20.0 * std::log10 (std::max (rms, 1e-12)),
                         (unsigned long long) (ok ? ogi::gihash::contentHash (d.pcm) : 0),
                         (unsigned long long) (ok ? fullHash (d.pcm) : 0), shape ? "ok" : (ok ? "SHAPE MISMATCH" : err.toRawUTF8()));
            if (! shape)
                rc = 1;
        }
        const bool budget = total <= 8388608 && largest <= 3145728;
        std::printf ("total %lld bytes (<= 8388608), largest %lld bytes (<= 3145728): %s\n", (long long) total, (long long) largest,
                     budget ? "within budget" : "OVER BUDGET");
        return budget ? rc : 1;
    }
}
