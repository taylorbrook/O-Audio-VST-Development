/*
  ==============================================================================

    O-GranulatorInstrument: the factory-source generator (PLAN-4 PD5 / PD6).

      O-GranulatorInstrument-render-harness --gen-factory <dir> [--force]
      O-GranulatorInstrument-render-harness --pin-factory <dir>

    1.2.0: --gen-factory writes only the recipes whose file is missing
    (--force: also the ones that exist), and never one of the 1.0.0 set (the
    first 8 below; their bytes are pinned by 64 presets and by every saved
    session). --pin-factory decodes every .flac in a folder and prints its two
    pins as JSON lines for tools/factory_table.py --apply-pins.

    Writes the curated synthetic factory sources (48 kHz, 24-bit FLAC through the
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

    1.2.0 adds 19 (levels by finishNew: the same targets, and never above a
    -20 dBFS loud frame; pitched at C3 unless noted):
      fm_bell        two-operator FM tine, decaying index             1   5 s
      glass          struck glass, partials 2.32 / 4.25 / 6.63        1   2 s
      bowed          1 / k harmonics, body resonances, vibrato        1   8 s
      reed           odd harmonics, a formant near 1.5 kHz            1   6 s
      organ          drawbars 8' ... 1', tremulant, key click         1   8 s
      choir          six voices, "ah" drifting to "oo"                2   8 s
      sub            C1 (root 24): sine + a little 2nd / 3rd / 4th    1   6 s
      saw_stack      seven detuned saws across the field              2   6 s
      chord_minor    C3 Eb3 G3 C4, dark                               2   8 s
      chord_sus      C3 G3 C4 F4 G4, open                             2   8 s
      kalimba        tine: fundamental + an overtone near 5.4 x       1   3 s
      marimba_bar    bar: 1, 4 x, 10 x, fast decay                    1 2.5 s
      metal_plate    dense plate modes, the lowest at C3              1   6 s
      wind           pink noise, wandering resonance, gusts           2   8 s
      rain_synth     dense drops over a high hiss                     2   8 s
      breath         air through resonances at C3 (flute-like)        1   8 s
      shimmer        sparkles on high harmonics of C3                 2   8 s
      clicks         irregular ticks, about 13 a second               1   4 s
      pulse_triplet  120 bpm, 2 bars of triplets: tom + wood tick     1   4 s

  ==============================================================================
*/

#include <JuceHeader.h>
#include "dsp/GiHash.h"
#include "dsp/SlotLoader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#if defined (__clang__)
 #pragma clang fp contract(off)
#endif

namespace factorygen
{
    int run (const juce::File& dir, bool force);
    int pin (const juce::File& dir);
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

        //======================================================================
        // 1.2.0: the 19 later recipes. Same rules as above (splitmix only,
        // double phase accumulators, no FMA). Pitched ones at C3 except `sub`
        // (C1; the manifest root says so). Level: finishNew.
        //======================================================================

        /** 90th-percentile 400 ms frame of the channel mean, linear RMS: the
            measure of the harness row FLV1 and of tools/factory_ingest.py. */
        double loudFrame (const std::vector<Chan>& x)
        {
            const int hop = 19200, n = static_cast<int> (x[0].size()) / hop;
            if (n < 1)
                return rmsOf (x);
            std::vector<double> f ((size_t) n, 0.0);
            const double invC = 1.0 / (double) x.size();
            for (int k = 0; k < n; ++k)
            {
                double acc = 0.0;
                for (int i = 0; i < hop; ++i)
                {
                    double m = 0.0;
                    for (const auto& c : x)
                        m += c[(size_t) (k * hop + i)];
                    m *= invC;
                    acc += m * m;
                }
                f[(size_t) k] = std::sqrt (acc / (double) hop);
            }
            std::sort (f.begin(), f.end());
            const double pos = 0.9 * (double) (n - 1);
            const int lo = static_cast<int> (pos);
            const double fr = pos - (double) lo;
            return lo + 1 < n ? f[(size_t) lo] + (f[(size_t) lo + 1] - f[(size_t) lo]) * fr : f[(size_t) lo];
        }

        /** As finish (tonal: -12 dBFS peak; noise: -18 dBFS RMS on a 16-bit
            grid, peak <= -1 dBFS), and never louder than a -20 dBFS loud frame
            (a sine at -12 dBFS peak would sit 5 dB above the 1.0.0 set). */
        ogi::SlotLoader::Pristine finishNew (const char* id, std::vector<Chan> x, bool noise)
        {
            double g = noise ? std::min (0.12589254117941673 / rmsOf (x), 0.89125093813374556 / peakOf (x))
                             : 0.25118864315095801 / peakOf (x);
            g = std::min (g, 0.1 / loudFrame (x));
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
                        v = std::nearbyint (v * 32768.0) / 32768.0;
                    p.pcm.setSample (c, i, static_cast<float> (v));
                }
            return p;
        }

        /** Kellett's pink filter. */
        struct Pink
        {
            double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
            double tick (double w)
            {
                b0 = 0.99886 * b0 + w * 0.0555179;
                b1 = 0.99332 * b1 + w * 0.0750759;
                b2 = 0.96900 * b2 + w * 0.1538520;
                b3 = 0.86650 * b3 + w * 0.3104856;
                b4 = 0.55000 * b4 + w * 0.5329522;
                b5 = -0.7616 * b5 - w * 0.0168980;
                const double pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362;
                b6 = w * 0.115926;
                return pink;
            }
        };

        /** State-variable filter (trapezoidal), safe under per-sample cutoff moves. */
        struct Svf
        {
            double ic1 = 0.0, ic2 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0, k = 1.0;
            double lp = 0.0, bp = 0.0, hp = 0.0;
            void set (double fc, double q)
            {
                const double g = std::tan (juce::MathConstants<double>::pi * fc / kFs);
                k = 1.0 / q;
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
            }
            void tick (double x)
            {
                const double v3 = x - ic2;
                const double v1 = a1 * ic1 + a2 * v3;
                const double v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0 * v1 - ic1;
                ic2 = 2.0 * v2 - ic2;
                lp = v2;
                bp = v1;
                hp = x - k * v1 - v2;
            }
        };

        /** A decaying sine partial added into x from sample `at`. */
        void addPartial (Chan& x, int at, double hz, double amp, double tau, double phase)
        {
            if (hz >= 0.45 * kFs)
                return;
            const int n = static_cast<int> (x.size());
            const double d = std::exp (-1.0 / (tau * kFs));
            Phase osc (hz, phase);
            double env = amp;
            for (int i = at; i < n && env > 1.0e-7; ++i)
            {
                x[(size_t) i] += env * osc.tick();
                env *= d;
            }
        }

        /** A short burst of one-pole-filtered noise (a strike, a pick, a thumb). */
        void addStrike (Chan& x, int at, SplitMix& r, double amp, double tau, double smooth, bool highPass)
        {
            const int n = static_cast<int> (x.size());
            const double d = std::exp (-1.0 / (tau * kFs));
            double z = 0.0, env = amp;
            for (int i = at; i < n && env > 1.0e-6; ++i)
            {
                const double w = r.bipolar();
                z += smooth * (w - z);
                x[(size_t) i] += env * (highPass ? w - z : z);
                env *= d;
            }
        }

        ogi::SlotLoader::Pristine fmBell()
        {
            // Two-operator FM tine: carrier C3, modulator 3.5 x, the index falls
            // from 5 to 0.15 (bright attack, near-sine tail).
            const int n = static_cast<int> (5.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            double pc = 0.0, pm = 0.0, env = 1.0, idx = 4.85;
            const double dEnv = std::exp (-1.0 / (1.7 * kFs)), dIdx = std::exp (-1.0 / (0.45 * kFs));
            for (int i = 0; i < n; ++i)
            {
                x[0][(size_t) i] = env * std::sin (kTwoPi * pc + (idx + 0.15) * std::sin (kTwoPi * pm));
                pc += kC3 / kFs;         if (pc >= 1.0) pc -= 1.0;
                pm += 3.5 * kC3 / kFs;   if (pm >= 1.0) pm -= 1.0;
                env *= dEnv;
                idx *= dIdx;
            }
            fadeEnds (x, 24, 4800);
            return finishNew ("fm_bell", std::move (x), false);
        }

        ogi::SlotLoader::Pristine glass()
        {
            // A short struck glass: sparse high partials, no hum.
            const int n = static_cast<int> (2.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x474C415353ull);   // "GLASS"
            struct Partial { double ratio, amp, tau; };
            const Partial ps[] = { { 1.0, 1.0, 0.55 }, { 2.32, 0.75, 0.38 }, { 4.25, 0.55, 0.24 }, { 6.63, 0.38, 0.15 }, { 9.38, 0.2, 0.09 } };
            for (const auto& q : ps)
                addPartial (x[0], 0, kC3 * q.ratio, q.amp, q.tau, r.unit());
            addStrike (x[0], 0, r, 0.5, 0.0008, 0.35, true);
            fadeEnds (x, 12, 2400);
            return finishNew ("glass", std::move (x), false);
        }

        /** Harmonics of C3 through fixed resonances, shared vibrato: the body
            of `bowed` and `reed`. gainOf (k) is the source spectrum. */
        template <typename Gain>
        void harmonicVoice (Chan& out, double f0, double seconds, SplitMix& r, const double* formHz, const double* formBw, const double* formG, int formants,
                            double flat, double vibHz, double vibCents, double vibOnset, double topHz, Gain gainOf)
        {
            const int n = static_cast<int> (seconds * kFs);
            const int K = static_cast<int> (topHz / f0);
            std::vector<double> ph ((size_t) K), amp ((size_t) K);
            for (int k = 1; k <= K; ++k)
            {
                ph[(size_t) (k - 1)] = r.unit();
                const double fk = f0 * k;
                double resp = flat;
                for (int j = 0; j < formants; ++j)
                {
                    const double u = (fk - formHz[j]) / (0.5 * formBw[j]);
                    resp += formG[j] / std::sqrt (1.0 + u * u);
                }
                amp[(size_t) (k - 1)] = gainOf (k) * resp;
            }
            Phase vib (vibHz, 0.0);
            const double depth = vibCents * 0.00057762265046662105;   // ln 2 / 1200
            for (int i = 0; i < n && i < static_cast<int> (out.size()); ++i)
            {
                const double on = vibOnset > 0.0 ? std::min (1.0, (double) i / (vibOnset * kFs)) : 1.0;
                const double m = 1.0 + depth * on * vib.tick();
                double v = 0.0;
                for (int k = 1; k <= K; ++k)
                {
                    auto& p = ph[(size_t) (k - 1)];
                    v += amp[(size_t) (k - 1)] * std::sin (kTwoPi * p);
                    p += f0 * k * m / kFs;
                    if (p >= 1.0) p -= 1.0;
                }
                out[(size_t) i] += v;
            }
        }

        ogi::SlotLoader::Pristine bowed()
        {
            // A bowed string: 1 / k harmonics through body resonances, vibrato
            // fading in, a slow bow-pressure swell, a little bow noise.
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x424F574544ull);   // "BOWED"
            const double fHz[] = { 280.0, 460.0, 1100.0, 2700.0 }, fBw[] = { 90.0, 120.0, 400.0, 900.0 }, fG[] = { 0.9, 0.7, 0.45, 0.3 };
            harmonicVoice (x[0], kC3, 8.0, r, fHz, fBw, fG, 4, 0.35, 5.3, 7.0, 1.6, 9000.0, [] (int k) { return 1.0 / (double) k; });
            Phase swell (0.11, 0.75);
            Svf bn;
            bn.set (2400.0, 0.8);
            for (int i = 0; i < n; ++i)
            {
                bn.tick (r.bipolar());
                const double s = 0.8 + 0.2 * swell.tick();
                x[0][(size_t) i] = s * (x[0][(size_t) i] + 0.035 * bn.bp);
            }
            fadeEnds (x, 7200, 9600);
            return finishNew ("bowed", std::move (x), false);
        }

        ogi::SlotLoader::Pristine reed()
        {
            // A single reed: strong odd harmonics, weak even ones, a formant
            // near 1.5 kHz, no vibrato, a slow breath shimmer.
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x52454544ull);   // "REED"
            const double fHz[] = { 1500.0, 3200.0 }, fBw[] = { 500.0, 900.0 }, fG[] = { 0.8, 0.25 };
            harmonicVoice (x[0], kC3, 6.0, r, fHz, fBw, fG, 2, 0.6, 4.0, 0.0, 0.0, 7000.0,
                           [] (int k) { return (k % 2 == 1 ? 1.0 : 0.09) / (double) k; });
            Phase sh (0.37, 0.2);
            Svf air;
            air.set (3000.0, 0.7);
            for (int i = 0; i < n; ++i)
            {
                air.tick (r.bipolar());
                x[0][(size_t) i] = (0.9 + 0.1 * sh.tick()) * (x[0][(size_t) i] + 0.02 * air.hp);
            }
            fadeEnds (x, 2400, 7200);
            return finishNew ("reed", std::move (x), false);
        }

        ogi::SlotLoader::Pristine organ()
        {
            // Drawbars 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1' on C3: pure sines, a slow
            // tremulant, a key click.
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x4F5247414Eull);   // "ORGAN"
            const double ratio[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0 }, amp[] = { 0.9, 0.7, 0.5, 0.6, 0.22, 0.2, 0.35 };
            for (int d = 0; d < 7; ++d)
            {
                Phase osc (kC3 * ratio[d], r.unit());
                Phase trem (0.83 + 0.07 * d, r.unit());
                for (int i = 0; i < n; ++i)
                    x[0][(size_t) i] += amp[d] * (0.93 + 0.07 * trem.tick()) * osc.tick();
            }
            addStrike (x[0], 0, r, 0.6, 0.003, 0.5, true);
            fadeEnds (x, 96, 4800);
            return finishNew ("organ", std::move (x), false);
        }

        ogi::SlotLoader::Pristine choir()
        {
            // Six voices (three a side: two at C3, one at C4), each its own
            // detune and vibrato, through "ah" formants drifting to "oo".
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            SplitMix r (0x43484F4952ull);   // "CHOIR"
            struct V { int ch; double oct, cents, gain, vibHz; };
            const V vs[] = { { 0, 1.0, -7.0, 1.0, 4.7 }, { 0, 1.0, 9.0, 0.9, 5.6 }, { 0, 2.0, 2.0, 0.55, 5.1 },
                             { 1, 1.0, -9.0, 1.0, 5.3 }, { 1, 1.0, 7.0, 0.9, 4.9 }, { 1, 2.0, -2.0, 0.55, 5.8 } };
            const double ah[] = { 700.0, 1100.0, 2600.0 }, oo[] = { 400.0, 800.0, 2500.0 };
            const double bw[] = { 110.0, 120.0, 160.0 }, fg[] = { 1.0, 0.45, 0.14 };
            for (const auto& v : vs)
            {
                const double f0 = kC3 * v.oct * std::exp2 (v.cents / 1200.0);
                const int K = static_cast<int> (6000.0 / f0);
                std::vector<double> ph ((size_t) K);
                for (auto& p : ph)
                    p = r.unit();
                Phase vib (v.vibHz, r.unit());
                Phase drift (0.07 + 0.05 * r.unit(), r.unit());
                auto& out = x[(size_t) v.ch];
                for (int i = 0; i < n; ++i)
                {
                    const double s = 0.5 + 0.5 * drift.tick();
                    const double F[3] = { ah[0] + (oo[0] - ah[0]) * s, ah[1] + (oo[1] - ah[1]) * s, ah[2] + (oo[2] - ah[2]) * s };
                    const double m = 1.0 + 0.0035 * vib.tick();
                    double acc = 0.0;
                    for (int k = 1; k <= K; ++k)
                    {
                        const double fk = f0 * k;
                        double resp = 0.0;
                        for (int j = 0; j < 3; ++j)
                        {
                            const double u = (fk - F[j]) / (0.5 * bw[j]);
                            resp += fg[j] / std::sqrt (1.0 + u * u);
                        }
                        auto& p = ph[(size_t) (k - 1)];
                        acc += resp / std::sqrt ((double) k) * std::sin (kTwoPi * p);
                        p += fk * m / kFs;
                        if (p >= 1.0) p -= 1.0;
                    }
                    out[(size_t) i] += v.gain * acc;
                }
            }
            fadeEnds (x, 9600, 9600);
            return finishNew ("choir", std::move (x), false);
        }

        ogi::SlotLoader::Pristine sub()
        {
            // C1 (32.70 Hz; manifest root 24): a sine with a little 2nd / 3rd /
            // 4th harmonic so it reads on small speakers, a slow level drift.
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            const double f0 = kC3 * 0.25;
            const double amp[] = { 1.0, 0.32, 0.11, 0.045 };
            for (int h = 1; h <= 4; ++h)
            {
                Phase osc (f0 * h, 0.0);
                Phase drift (0.09 * h, 0.25 * h);
                for (int i = 0; i < n; ++i)
                    x[0][(size_t) i] += amp[h - 1] * (0.92 + 0.08 * drift.tick()) * osc.tick();
            }
            fadeEnds (x, 2400, 4800);
            return finishNew ("sub", std::move (x), false);
        }

        ogi::SlotLoader::Pristine sawStack()
        {
            // Seven saws, detuned symmetrically about C3, spread across the
            // field. Additive (band-limited), rolled off above 5 kHz.
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            SplitMix r (0x53415753544Bull);   // "SAWSTK"
            const double cents[] = { -18.0, -11.0, -5.0, 0.0, 5.0, 11.0, 18.0 };
            const double pan[]   = { 0.1, 0.8, 0.3, 0.5, 0.7, 0.2, 0.9 };
            Chan one ((size_t) n);
            for (int s = 0; s < 7; ++s)
            {
                const double f0 = kC3 * std::exp2 (cents[s] / 1200.0);
                std::fill (one.begin(), one.end(), 0.0);
                const int K = static_cast<int> (9000.0 / f0);
                for (int k = 1; k <= K; ++k)
                {
                    const double fk = f0 * k, u = fk / 5000.0;
                    const double a = 1.0 / ((double) k * (1.0 + u * u));
                    Phase osc (fk, r.unit());
                    for (int i = 0; i < n; ++i)
                        one[(size_t) i] += a * osc.tick();
                }
                const double gl = std::cos (0.5 * juce::MathConstants<double>::pi * pan[s]), gr = std::sin (0.5 * juce::MathConstants<double>::pi * pan[s]);
                for (int i = 0; i < n; ++i)
                {
                    x[0][(size_t) i] += gl * one[(size_t) i];
                    x[1][(size_t) i] += gr * one[(size_t) i];
                }
            }
            fadeEnds (x, 480, 4800);
            return finishNew ("saw_stack", std::move (x), false);
        }

        /** A sustained chord: per note a soft additive tone (1 / k^roll), a
            detuned pair left / right, independent slow swells. */
        ogi::SlotLoader::Pristine chord (const char* id, std::uint64_t seed, const double* semis, const double* gains, int notes, double roll, int harmonics)
        {
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            SplitMix r (seed);
            for (int c = 0; c < 2; ++c)
                for (int q = 0; q < notes; ++q)
                {
                    const double f0 = kC3 * std::exp2 (semis[q] / 12.0) * std::exp2 ((c == 0 ? -2.5 : 2.5) / 1200.0);
                    Phase swell (0.06 + 0.12 * r.unit(), r.unit());
                    std::vector<Phase> osc;
                    std::vector<double> amp;
                    for (int k = 1; k <= harmonics; ++k)
                    {
                        osc.emplace_back (f0 * k, r.unit());
                        amp.push_back (gains[q] / std::pow ((double) k, roll));
                    }
                    for (int i = 0; i < n; ++i)
                    {
                        double v = 0.0;
                        for (int k = 0; k < harmonics; ++k)
                            v += amp[(size_t) k] * osc[(size_t) k].tick();
                        x[(size_t) c][(size_t) i] += (0.72 + 0.28 * swell.tick()) * v;
                    }
                }
            fadeEnds (x, 7200, 9600);
            return finishNew (id, std::move (x), false);
        }

        ogi::SlotLoader::Pristine chordMinor()
        {
            const double semis[] = { 0.0, 3.0, 7.0, 12.0 }, gains[] = { 1.0, 0.8, 0.8, 0.5 };   // C3 Eb3 G3 C4, dark
            return chord ("chord_minor", 0x43484D494Eull, semis, gains, 4, 1.7, 10);
        }

        ogi::SlotLoader::Pristine chordSus()
        {
            const double semis[] = { 0.0, 7.0, 12.0, 17.0, 19.0 }, gains[] = { 1.0, 0.8, 0.7, 0.65, 0.5 };   // C3 G3 C4 F4 G4, open and brighter
            return chord ("chord_sus", 0x4348535553ull, semis, gains, 5, 1.15, 16);
        }

        ogi::SlotLoader::Pristine kalimba()
        {
            // A tine: the fundamental and one strong overtone near 5.4 x that
            // dies fast, a thumb noise.
            const int n = static_cast<int> (3.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x4B414C494Dull);   // "KALIM"
            addPartial (x[0], 0, kC3, 1.0, 0.95, 0.0);
            addPartial (x[0], 0, kC3 * 5.4, 0.8, 0.2, r.unit());
            addPartial (x[0], 0, kC3 * 2.0, 0.05, 0.4, r.unit());
            addPartial (x[0], 0, kC3 * 14.7, 0.15, 0.04, r.unit());
            addStrike (x[0], 0, r, 0.3, 0.0025, 0.25, false);
            fadeEnds (x, 12, 2400);
            return finishNew ("kalimba", std::move (x), false);
        }

        ogi::SlotLoader::Pristine marimbaBar()
        {
            // A tuned bar: 1, 4 x and 10 x, fast decay, a soft mallet thump.
            const int n = static_cast<int> (2.5 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x4D4152494Dull);   // "MARIM"
            addPartial (x[0], 0, kC3, 1.0, 0.3, 0.0);
            addPartial (x[0], 0, kC3 * 4.0, 0.5, 0.12, r.unit());
            addPartial (x[0], 0, kC3 * 10.0, 0.22, 0.035, r.unit());
            addStrike (x[0], 0, r, 0.9, 0.005, 0.05, false);
            fadeEnds (x, 12, 2400);
            return finishNew ("marimba_bar", std::move (x), false);
        }

        ogi::SlotLoader::Pristine metalPlate()
        {
            // A struck plate: dense inharmonic modes (m^2 + n^2), the lowest at
            // C3, each a beating pair; long bright clang.
            const int n = static_cast<int> (6.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x504C415445ull);   // "PLATE"
            for (int a = 1; a <= 7; ++a)
                for (int b = a; b <= 8; ++b)
                {
                    const double ratio = (double) (a * a + b * b) / 2.0;
                    const double hz = kC3 * ratio;
                    if (hz > 11000.0)
                        continue;
                    const double amp = (ratio < 1.5 ? 1.0 : (0.25 + 0.6 * r.unit()) / std::sqrt (ratio));
                    const double tau = 2.6 / (1.0 + 0.16 * ratio);
                    const double split = ratio < 1.5 ? 0.0 : 0.0006 * (r.unit() - 0.5);
                    addPartial (x[0], 0, hz * (1.0 + split), amp * 0.6, tau, r.unit());
                    addPartial (x[0], 0, hz * (1.0 - split), amp * 0.4, tau * 0.9, r.unit());
                }
            addStrike (x[0], 0, r, 0.6, 0.0012, 0.6, true);
            fadeEnds (x, 12, 4800);
            return finishNew ("metal_plate", std::move (x), false);
        }

        ogi::SlotLoader::Pristine wind()
        {
            // Pink noise through a resonance that wanders 250 Hz ... 1.6 kHz,
            // with gusts. Left and right share the gusts, not the noise.
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            for (int c = 0; c < 2; ++c)
            {
                SplitMix r (0x57494E44ull + (std::uint64_t) c * 0x1000193ull);   // "WIND" + channel
                Pink pink;
                Svf f, f2;
                Phase l1 (0.13, 0.10), l2 (0.31, 0.55), l3 (0.071, 0.80 + 0.03 * c), g1 (0.19, 0.4), g2 (0.47, 0.9);
                for (int i = 0; i < n; ++i)
                {
                    const double s = 0.5 + 0.25 * l1.tick() + 0.15 * l2.tick() + 0.1 * l3.tick();   // 0 ... 1
                    const double gust = 0.55 + 0.3 * g1.tick() + 0.15 * g2.tick();                  // 0.1 ... 1
                    if ((i & 15) == 0)
                    {
                        f.set (250.0 * std::exp (s * 1.856), 2.2);            // 250 * 6.4^s
                        f2.set (900.0 * std::exp (s * 1.2), 1.1);
                    }
                    const double p = pink.tick (r.bipolar());
                    f.tick (p);
                    f2.tick (p);
                    x[(size_t) c][(size_t) i] = gust * (f.bp + 0.35 * f2.bp * gust);
                }
            }
            fadeEnds (x, 4800, 4800);
            return finishNew ("wind", std::move (x), true);
        }

        ogi::SlotLoader::Pristine rainSynth()
        {
            // Dense drops (about 450 a second a side: short bright bursts of
            // random size) over a steady high hiss.
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            for (int c = 0; c < 2; ++c)
            {
                SplitMix r (0x5241494Eull + (std::uint64_t) c * 0x9E37ull);   // "RAIN" + channel
                Svf hiss, drop;
                hiss.set (5200.0, 0.6);
                double dropEnv = 0.0, dropDecay = 0.0, lp = 0.0;
                for (int i = 0; i < n; ++i)
                {
                    const double w = r.bipolar();
                    hiss.tick (w);
                    if (r.unit() < 450.0 / kFs)
                    {
                        const double u = r.unit();
                        dropEnv = 0.2 + 1.6 * u * u * u * u;
                        dropDecay = std::exp (-1.0 / ((0.0006 + 0.004 * r.unit()) * kFs));
                        drop.set (1800.0 + 6000.0 * r.unit(), 2.5);
                    }
                    drop.tick (w);
                    lp += 0.02 * (w - lp);
                    x[(size_t) c][(size_t) i] = 0.16 * hiss.hp + dropEnv * drop.bp + 0.25 * lp;
                    dropEnv *= dropDecay;
                }
            }
            fadeEnds (x, 2400, 2400);
            return finishNew ("rain_synth", std::move (x), true);
        }

        ogi::SlotLoader::Pristine breath()
        {
            // Air through a pipe: noise into resonances at C3 and its first
            // harmonics (flute-like), plus the air itself; two slow breaths.
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x425245415448ull);   // "BREATH"
            constexpr int R = 6;
            double y1[R] = {}, y2[R] = {}, c1[R], c2[R], g[R];
            const double gain[R] = { 1.0, 0.55, 0.4, 0.22, 0.14, 0.08 };
            for (int k = 0; k < R; ++k)
            {
                const double hz = kC3 * (k + 1), bw = 2.2 + 1.4 * k;       // Hz
                const double rad = std::exp (-juce::MathConstants<double>::pi * bw / kFs);
                c1[k] = 2.0 * rad * std::cos (kTwoPi * hz / kFs);
                c2[k] = rad * rad;
                g[k] = gain[k] * (1.0 - rad);
            }
            Svf air;
            air.set (1800.0, 0.5);
            Phase br (0.25, 0.75);   // two breaths over the file
            for (int i = 0; i < n; ++i)
            {
                const double w = r.bipolar();
                double v = 0.0;
                for (int k = 0; k < R; ++k)
                {
                    const double y = g[k] * w + c1[k] * y1[k] - c2[k] * y2[k];
                    y2[k] = y1[k];
                    y1[k] = y;
                    v += y;
                }
                air.tick (w);
                const double e = 0.5 + 0.5 * br.tick();
                x[0][(size_t) i] = (0.25 + 0.75 * e) * (v + 0.012 * (0.4 + 0.6 * e) * air.hp);
            }
            fadeEnds (x, 4800, 4800);
            return finishNew ("breath", std::move (x), true);
        }

        ogi::SlotLoader::Pristine shimmer()
        {
            // Sparkles: short Hann-windowed sines on high harmonics of C3
            // (C6 ... C8, the notes of a C major chord), scattered left / right.
            const int n = static_cast<int> (8.0 * kFs);
            std::vector<Chan> x (2, Chan ((size_t) n, 0.0));
            SplitMix r (0x5348494D4D4552ull);   // "SHIMMER"
            const int harm[] = { 8, 10, 12, 16, 20, 24, 32, 40, 48, 64 };
            const int count = 420;
            for (int q = 0; q < count; ++q)
            {
                const int k = harm[(int) (r.unit() * 10.0) % 10];
                const int len = static_cast<int> ((0.09 + 0.5 * r.unit() * r.unit()) * kFs);
                const int at = static_cast<int> (r.unit() * (double) (n - len));
                const double amp = (0.25 + 0.75 * r.unit()) * 8.0 / (double) k;
                const double pan = r.unit();
                const double gl = std::cos (0.5 * juce::MathConstants<double>::pi * pan), gr = std::sin (0.5 * juce::MathConstants<double>::pi * pan);
                Phase osc (kC3 * k, r.unit());
                for (int i = 0; i < len; ++i)
                {
                    const double wv = 0.5 - 0.5 * std::cos (kTwoPi * (double) i / (double) len);
                    const double v = amp * wv * wv * osc.tick();
                    x[0][(size_t) (at + i)] += gl * v;
                    x[1][(size_t) (at + i)] += gr * v;
                }
            }
            fadeEnds (x, 480, 480);
            return finishNew ("shimmer", std::move (x), false);
        }

        ogi::SlotLoader::Pristine clicks()
        {
            // Irregular ticks: about 13 a second, exponential gaps (15 ms
            // floor), each a short ping at its own pitch and size.
            const int n = static_cast<int> (4.0 * kFs);
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x434C49434B53ull);   // "CLICKS"
            double t = 0.01;
            while (t < 3.93)
            {
                const int at = static_cast<int> (t * kFs);
                const double u = r.unit();
                const double amp = 0.12 + 0.88 * u * u;
                const double hz = 700.0 * std::exp (2.2 * r.unit());           // 700 Hz ... 6.3 kHz
                addPartial (x[0], at, hz, amp, 0.0009 + 0.004 * r.unit(), 0.0);
                addStrike (x[0], at, r, 0.5 * amp, 0.0004, 0.6, true);
                t += 0.015 - 0.062 * std::log (1.0 - 0.999 * r.unit());
            }
            fadeEnds (x, 12, 240);
            return finishNew ("clicks", std::move (x), true);
        }

        ogi::SlotLoader::Pristine pulseTriplet()
        {
            // 120 bpm, 2 bars of 12 / 8 feel = 4.000 s exactly (a seamless
            // loop): a low tom on the beat, a wood tick on the two triplets
            // after it, the last of each bar accented.
            const int n = static_cast<int> (4.0 * kFs);
            const int step = 8000;   // one triplet eighth at 120 bpm
            std::vector<Chan> x (1, Chan ((size_t) n, 0.0));
            SplitMix r (0x545249504C4554ull);   // "TRIPLET"
            for (int s = 0; s < 24; ++s)
            {
                const int at = s * step;
                if (s % 3 == 0)
                {
                    double ph = 0.0, env = (s % 12 == 0 ? 1.0 : 0.8);
                    const double ed = std::exp (-1.0 / (0.11 * kFs));
                    for (int i = 0; i < step * 2 && at + i < n; ++i)
                    {
                        const double f = 78.0 + 70.0 * std::exp (-(double) i / (0.02 * kFs));
                        x[0][(size_t) (at + i)] += env * std::sin (kTwoPi * ph);
                        ph += f / kFs;
                        if (ph >= 1.0) ph -= 1.0;
                        env *= ed;
                    }
                }
                else
                {
                    const double acc = (s % 12 == 11 ? 0.62 : (s % 3 == 2 ? 0.4 : 0.28));
                    addPartial (x[0], at, 1150.0, acc, 0.012, 0.0);
                    addPartial (x[0], at, 2730.0, acc * 0.5, 0.006, 0.25);
                    addStrike (x[0], at, r, acc * 0.5, 0.0015, 0.5, true);
                }
            }
            fadeEnds (x, 24, 240);
            return finishNew ("pulse_triplet", std::move (x), false);
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
        // 1.2.0 (written only when missing; the 8 above are never written again)
        { "fm_bell",     &fmBell,     1, 5.0 }, { "glass",        &glass,        1, 2.0 },
        { "bowed",       &bowed,      1, 8.0 }, { "reed",         &reed,         1, 6.0 },
        { "organ",       &organ,      1, 8.0 }, { "choir",        &choir,        2, 8.0 },
        { "sub",         &sub,        1, 6.0 }, { "saw_stack",    &sawStack,     2, 6.0 },
        { "chord_minor", &chordMinor, 2, 8.0 }, { "chord_sus",    &chordSus,     2, 8.0 },
        { "kalimba",     &kalimba,    1, 3.0 }, { "marimba_bar",  &marimbaBar,   1, 2.5 },
        { "metal_plate", &metalPlate, 1, 6.0 }, { "wind",         &wind,         2, 8.0 },
        { "rain_synth",  &rainSynth,  2, 8.0 }, { "breath",       &breath,       1, 8.0 },
        { "shimmer",     &shimmer,    2, 8.0 }, { "clicks",       &clicks,       1, 4.0 },
        { "pulse_triplet", &pulseTriplet, 1, 4.0 },
    };

    std::uint64_t fullPcmHash (const juce::AudioBuffer<float>& pcm) { return fullHash (pcm); }

    /** The 1.0.0 set: generated once (PLAN-4 T7), never written again. */
    static bool isFrozen (const char* id)
    {
        for (const char* f : { "harmonic", "pluck", "bell", "vowel", "noise_bed", "dust", "drone", "pulse" })
            if (std::strcmp (f, id) == 0)
                return true;
        return false;
    }

    /** One line per decoded file: bytes, shape, levels, the two pins. */
    static bool report (const juce::String& fileName, const void* data, std::size_t size, int wantChannels, int wantFrames, bool json)
    {
        ogi::SlotLoader::Pristine d;
        juce::String err;
        const auto id = fileName.upToLastOccurrenceOf (".", false, false);
        const bool ok = ogi::SlotLoader::decodeMemoryFlac (data, size, id, 48000.0, d, err);
        const bool shape = ok && juce::exactlyEqual (d.rate, 48000.0)
                        && (wantChannels <= 0 || (d.pcm.getNumChannels() == wantChannels && d.pcm.getNumSamples() == wantFrames));
        if (json)
        {
            if (ok)
                std::printf ("{\"id\": \"%s\", \"channels\": %d, \"frames\": %d, \"bytes\": %d, \"kHash\": \"0x%016llx\", \"kFull\": \"0x%016llx\"}\n",
                             id.toRawUTF8(), d.pcm.getNumChannels(), d.pcm.getNumSamples(), (int) size,
                             (unsigned long long) ogi::gihash::contentHash (d.pcm), (unsigned long long) fullHash (d.pcm));
            else
                std::printf ("decode failed: %s (%s)\n", fileName.toRawUTF8(), err.toRawUTF8());
            return shape;
        }
        double pk = 0.0, acc = 0.0;
        for (int c = 0; c < d.pcm.getNumChannels(); ++c)
            for (int i = 0; i < d.pcm.getNumSamples(); ++i)
            {
                const double v = d.pcm.getSample (c, i);
                pk = std::max (pk, std::abs (v));
                acc += v * v;
            }
        const double rms = std::sqrt (acc / std::max (1, d.pcm.getNumChannels() * d.pcm.getNumSamples()));
        std::printf ("%-18s %8d bytes  %d ch %7d frames  peak %6.2f dBFS  rms %6.2f dBFS  kHash 0x%016llxull  kFull 0x%016llxull  %s\n",
                     fileName.toRawUTF8(), (int) size, d.pcm.getNumChannels(), d.pcm.getNumSamples(),
                     20.0 * std::log10 (std::max (pk, 1e-12)), 20.0 * std::log10 (std::max (rms, 1e-12)),
                     (unsigned long long) (ok ? ogi::gihash::contentHash (d.pcm) : 0),
                     (unsigned long long) (ok ? fullHash (d.pcm) : 0), shape ? "ok" : (ok ? "SHAPE MISMATCH" : err.toRawUTF8()));
        return shape;
    }

    /** --gen-factory <dir> [--force]: writes the recipes whose file is
        MISSING. --force rewrites the ones that exist too, except the 1.0.0
        set, which this function never writes (1.2.0: 64 presets and every
        saved session pin those bytes). A missing 1.0.0 file is an error, not
        a reason to regenerate it: restore it from git. */
    int run (const juce::File& dir, bool force)
    {
        dir.createDirectory();
        int rc = 0, written = 0, kept = 0;
        std::int64_t total = 0, largest = 0;
        for (const auto& row : kRecipes)
        {
            const auto f = dir.getChildFile (juce::String (row.id) + ".flac");
            const bool frozen = isFrozen (row.id);
            if (frozen && ! f.existsAsFile())
            {
                std::printf ("%-18s MISSING: one of the 1.0.0 set, never regenerated (git restore it)\n", f.getFileName().toRawUTF8());
                rc = 1;
                continue;
            }
            if (f.existsAsFile() && (frozen || ! force))
            {
                juce::MemoryBlock have;
                f.loadFileAsData (have);
                total += (std::int64_t) have.getSize();
                largest = std::max (largest, (std::int64_t) have.getSize());
                ++kept;
                continue;
            }
            const auto p = row.make();
            juce::MemoryBlock mb;
            if (! ogi::SlotLoader::encodeFlac24 (p, mb, 5))
            {
                std::printf ("encode failed: %s\n", row.id);
                rc = 1;
                continue;
            }
            f.replaceWithData (mb.getData(), mb.getSize());
            ++written;
            total += (std::int64_t) mb.getSize();
            largest = std::max (largest, (std::int64_t) mb.getSize());
            if (! report (f.getFileName(), mb.getData(), mb.getSize(), row.channels, (int) (row.seconds * kFs), false))
                rc = 1;
        }
        std::printf ("%d written, %d kept; recipes total %lld bytes, largest %lld bytes (the embedded budget is FSP1's)\n",
                     written, kept, (long long) total, (long long) largest);
        return rc;
    }

    /** --pin-factory <dir>: decodes every .flac in the folder (name order)
        and prints one JSON line per file: the form tools/factory_table.py
        --apply-pins reads. */
    int pin (const juce::File& dir)
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.flac");
        files.sort();
        int rc = files.isEmpty() ? 1 : 0;
        for (const auto& f : files)
        {
            juce::MemoryBlock mb;
            if (! f.loadFileAsData (mb) || ! report (f.getFileName(), mb.getData(), mb.getSize(), 0, 0, true))
                rc = 1;
        }
        return rc;
    }
}
