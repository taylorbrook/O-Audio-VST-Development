/*
  ==============================================================================

    O-GrainScatter — alias gate (v3.3.0, band-limited reader)

    Built twice from this one driver:
      O-GrainScatter-alias-gate        current Source/ (v3.3.0, grain-engine reader)
      O-GrainScatter-alias-gate-v320   v3.2.0 = commit 9c6c22d0 via git archive
                                       (Lagrange-4, no band limiting): the negative
                                       control. It must FAIL rows 1, 2 and 4.

    Engine rows drive the plugin's own DelayBuffer / FreezeManager / GrainPool
    (the exact headers of each tree) so Doppler can be forced and the grain
    pattern is deterministic: Hann grains of 100 ms, one at a time (hop = length),
    so a grain's output power does not depend on its phase against the previous
    grain. Metric: POWER below 20 kHz outside +-400 Hz of the wanted component
    (when the wanted f*r is below Nyquist), re the power of an in-band reference
    tone (input 1 kHz / r, output 1 kHz) — total spurious power, so it is never
    lower than the worst single alias. 44.1 kHz (the strictest audible band).

      1  stereo path, +12/+24/+36 st: 36 tones 50 Hz..20 kHz, and an out-of-band
         sweep (fs/2r .. 20 kHz, every audible output is alias) re an in-band
         sweep. Verdict <= -60 dB.
      1e processor end-to-end, +24 st transpose via the parameter (spawn guard,
         density gain, real scheduler): out-of-band tones re a 1 kHz-out tone.
      2  +36 st spatial path (mono W channel), Doppler forced to 1.0 and 1.4.
         Verdict <= -60 dB.
      3  Doppler 2.0: +36 st (r' = 2, set S6) and +35 st (r' 3.78, set S7, best
         effort) — REPORT only (plan decision 2).
      4  frozen +36 st (tones on whole cycles of the capture loop, so the seam
         crossfade is phase-continuous). Verdict <= -60 dB.
      5  alignment: a Gaussian transient read at r = 1.99 (level 0) and 2.01
         (level 1), timed to cross the read head at the grain centre; output
         cross-correlation lag, in source samples, <= 0.05. Positive control: a
         0.2-sample source offset must be detected.
      6  passband: output -1 dB point per rate, 44.1 and 48 kHz (REPORT).
      7  CPU: 64 grains at +24 st, stereo and spatial, ns per output sample
         (REPORT; compare the two binaries).

    Exit 0: v3.3.0 build — every verdict row PASS. v3.2.0 build — rows 1, 2 and 4
    all FAIL (the gate can see aliasing); a v3.2.0 PASS there = vacuous gate.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <vector>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double kPi = juce::MathConstants<double>::pi;
   #ifdef GS_ALIAS_V320
    constexpr const char* kBuild = "v3.2.0 (9c6c22d0, negative control)";
   #else
    constexpr const char* kBuild = "v3.3.0";
   #endif

    int verdictFails = 0;               // rows with a verdict that failed
    std::map<std::string, bool> rowOk;  // row group -> all passed

    void verdict (const std::string& group, bool ok, const juce::String& name, const juce::String& detail)
    {
        std::printf ("  [%s] %-46s %s\n", ok ? "PASS" : "FAIL", name.toRawUTF8(), detail.toRawUTF8());
        if (! ok) ++verdictFails;
        auto it = rowOk.find (group);
        rowOk[group] = (it == rowOk.end() ? true : it->second) && ok;
    }

    void report (const juce::String& name, const juce::String& detail)
    {
        std::printf ("  [ -- ] %-46s %s\n", name.toRawUTF8(), detail.toRawUTF8());
    }

    double dB10 (double p) { return p > 0.0 ? 10.0 * std::log10 (p) : -400.0; }

    // ── FFT / spectra ──────────────────────────────────────────────────────
    void fft (std::vector<std::complex<double>>& a)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = -2.0 * kPi / static_cast<double> (len);
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    const auto u = a[i + k], v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
    }

    // Blackman-Harris power spectrum of x[from, from + n), n a power of two.
    std::vector<double> powerSpectrum (const std::vector<float>& x, size_t from, size_t n)
    {
        std::vector<std::complex<double>> a (n);
        for (size_t i = 0; i < n; ++i)
        {
            const double t = static_cast<double> (i) / static_cast<double> (n);
            const double w = 0.35875 - 0.48829 * std::cos (2 * kPi * t) + 0.14128 * std::cos (4 * kPi * t)
                           - 0.01168 * std::cos (6 * kPi * t);
            a[i] = w * static_cast<double> (x[from + i]);
        }
        fft (a);
        std::vector<double> p (n / 2);
        for (size_t k = 0; k < n / 2; ++k)
            p[k] = std::norm (a[k]);
        return p;
    }

    struct Split { double wanted = 0.0, spurious = 0.0; };

    // Power below `audibleHz` split into the wanted cluster (+-guardHz around wantedHz,
    // when wantedHz >= 0) and everything else.
    Split splitPower (const std::vector<double>& p, double fs, size_t n, double wantedHz,
                      double audibleHz = 20000.0, double guardHz = 400.0)
    {
        Split s;
        for (size_t k = 1; k < p.size(); ++k)
        {
            const double f = static_cast<double> (k) * fs / static_cast<double> (n);
            if (f >= audibleHz) break;
            if (wantedHz >= 0.0 && std::abs (f - wantedHz) <= guardHz) s.wanted += p[k];
            else s.spurious += p[k];
        }
        return s;
    }

    // ── engine harness ─────────────────────────────────────────────────────
    struct Engine
    {
        double fs = 44100.0;
        float rate = 1.0f;
        float doppler = 1.0f;      // forced on every voice each sample (spatial only)
        bool spatial = false;
        bool frozen = false;
        int grainLen = 4410;
        int hop = 4410;
        int numOut = 1 << 17;
        double extraPos = 0.0;     // added to every grain's position (row 5 control)
        double frozenStart = -1.0; // >= 0: every frozen grain starts at this snapshot position
        int maxGrains = 1;         // grains spawned per hop (CPU row)
    };

    // Renders the wet mono output (stereo: L channel; spatial: HOA W). `in` is indexed by
    // absolute push count. Spawns a grain at every hop.
    std::vector<float> renderEngine (const Engine& e, const std::function<float (juce::int64)>& in,
                                     std::function<double (juce::int64, int)> posFor = nullptr,
                                     double* nsPerSample = nullptr)
    {
        auto delay = std::make_unique<DelayBuffer>();
        auto freeze = std::make_unique<FreezeManager>();
        auto pool = std::make_unique<GrainPool>();
        delay->prepare (e.fs, 12.0f);
        freeze->prepare (e.fs, static_cast<int> (e.fs * 2.0) + 1);
        pool->setSampleRate (e.fs);
        pool->prepareSpatial (static_cast<float> (e.fs));
        pool->setGrainShape (GrainShape::Hann);
        pool->clearVoices();

        const double speed = static_cast<double> (e.rate) * (e.spatial ? e.doppler : 1.0f);
        const double defaultPos = e.grainLen * juce::jmax (0.0, speed - 1.0) + 1500.0;
        juce::int64 pushes = 0;
        const int pre = e.frozen ? static_cast<int> (e.fs * 2.0) + 40000
                                 : static_cast<int> (defaultPos) + e.grainLen + 4000;
        for (int i = 0; i < pre; ++i)
        {
            const float v = in (pushes++);
            delay->pushSample (v, v);
        }
        if (e.frozen)
            freeze->engage (*delay, 0);

        std::vector<float> out (static_cast<size_t> (e.numOut));
        float hoa[kHOA3Channels];
        const auto t0 = std::chrono::steady_clock::now();
        for (int n = 0; n < e.numOut; ++n)
        {
            const float v = in (pushes++);
            delay->pushSample (v, v);
            freeze->advance();

            if (n % e.hop == 0)
                for (int g = 0; g < e.maxGrains; ++g)
                {
                    GrainParams gp;
                    gp.playbackRate = e.rate;
                    gp.grainLengthSamples = e.grainLen;
                    gp.positionInSamples = (posFor ? posFor (pushes, g) : defaultPos + 37.0 * g) + e.extraPos;
                    gp.panPosition = 0.5f;
                    gp.readFromFrozen = e.frozen;
                    if (e.frozen)
                    {
                        gp.frozenSlot = 0;
                        gp.frozenBase = e.frozenStart >= 0.0 ? e.frozenStart
                                                             : freeze->getSpawnBase (gp.positionInSamples);
                    }
                    pool->spawnGrain (gp);
                }

            if (e.spatial)
            {
                for (auto& vc : pool->getVoices())
                    if (vc.active) vc.dopplerFactor = e.doppler;
                std::fill (std::begin (hoa), std::end (hoa), 0.0f);
                pool->processSampleSpatial (*delay, *freeze, hoa);
                out[static_cast<size_t> (n)] = hoa[0];
            }
            else
            {
                float l = 0.0f, r = 0.0f;
                pool->processSample (*delay, *freeze, l, r);
                out[static_cast<size_t> (n)] = l;
            }
        }
        if (nsPerSample != nullptr)
            *nsPerSample = std::chrono::duration<double, std::nano> (std::chrono::steady_clock::now() - t0).count()
                           / e.numOut;
        return out;
    }

    auto sineIn (double hz, double fs, double amp = 0.5)
    {
        return [hz, fs, amp] (juce::int64 i) { return static_cast<float> (amp * std::sin (2.0 * kPi * hz * static_cast<double> (i) / fs)); };
    }

    // Phase-continuous exponential up/down sweep f0 -> f1 -> f0, period `len` pushes
    // (no wrap click). Stateful: the engine calls it once per push, in order.
    auto sweepIn (double f0, double f1, double fs, juce::int64 len, double amp = 0.5)
    {
        return [=, phase = 0.0, next = juce::int64 (0)] (juce::int64 i) mutable
        {
            jassert (i == next); ++next;
            const double u = static_cast<double> (i % len) / static_cast<double> (len);
            const double tri = u < 0.5 ? 2.0 * u : 2.0 - 2.0 * u;
            const double f = f0 * std::pow (f1 / f0, tri);
            const float v = static_cast<float> (amp * std::sin (phase));
            phase += 2.0 * kPi * f / fs;
            if (phase > 2.0 * kPi) phase -= 2.0 * kPi;
            return v;
        };
    }

    constexpr size_t kN = 1 << 16;   // analysis length (last kN samples of the render)

    double refPower (Engine e, double fs)
    {
        const double fin = 1000.0 / (static_cast<double> (e.rate) * (e.spatial ? e.doppler : 1.0f));
        const auto y = renderEngine (e, sineIn (fin, fs));
        const auto p = powerSpectrum (y, y.size() - kN, kN);
        return splitPower (p, fs, kN, 1000.0).wanted;
    }

    // Worst tone + sweep spurious for an engine config. Returns worst dB.
    double aliasRow (const std::string& group, const juce::String& label, Engine e, bool isVerdict,
                     const std::vector<double>& tones, bool withSweep)
    {
        const double fs = e.fs;
        const double speed = static_cast<double> (e.rate) * (e.spatial ? e.doppler : 1.0f);
        const double pRef = refPower (e, fs);
        double worst = -400.0, worstHz = 0.0;
        for (double f : tones)
        {
            const auto y = renderEngine (e, sineIn (f, fs));
            const auto p = powerSpectrum (y, y.size() - kN, kN);
            const double fo = f * speed;
            const auto s = splitPower (p, fs, kN, fo < fs * 0.5 ? fo : -1.0);
            const double d = dB10 (s.spurious / pRef);
            if (d > worst) { worst = d; worstHz = f; }
        }
        juce::String det = juce::String::formatted ("tones: worst %6.1f dB (in %5.0f Hz)", worst, worstHz);
        double sweepDb = -400.0;
        if (withSweep)
        {
            const juce::int64 len = juce::int64 (1) << 15;   // 2 full up/down periods in the analysis window
            const double nyqIn = fs * 0.5 / speed;
            const auto yo = renderEngine (e, sweepIn (nyqIn * 1.02, 20000.0, fs, len));
            const auto yi = renderEngine (e, sweepIn (50.0, 0.85 * 20000.0 / speed, fs, len));
            const auto po = powerSpectrum (yo, yo.size() - kN, kN);
            const auto pi = powerSpectrum (yi, yi.size() - kN, kN);
            const double alias = splitPower (po, fs, kN, -1.0).spurious;
            const double ref = splitPower (pi, fs, kN, -1.0).spurious;
            sweepDb = dB10 (alias / ref);
            det << juce::String::formatted ("; sweep %6.1f dB", sweepDb);
        }
        const double w = juce::jmax (worst, sweepDb);
        if (isVerdict) verdict (group, w <= -60.0, label, det);
        else report (label, det);
        return w;
    }

    std::vector<double> toneSet()
    {
        std::vector<double> t;
        for (int i = 0; i < 24; ++i) t.push_back (50.0 * std::pow (400.0, i / 23.0));   // 50 Hz .. 20 kHz
        for (double f = 9000.0; f <= 19500.0; f += 1000.0) t.push_back (f + 250.0);
        return t;
    }

    // ── row 5: alignment ───────────────────────────────────────────────────
    // One grain whose read head crosses a Gaussian transient (sigma 24 source samples)
    // at the grain centre tc. The spawn sees write count W0; the peak is at source time
    // W0 - 8000, so position = r tc + 8000 (+ srcOffset) independently of W0.
    std::vector<float> transientGrain (float rate, double fs, double srcOffset)
    {
        Engine e; e.fs = fs; e.rate = rate; e.numOut = 8192; e.hop = 1 << 30; e.grainLen = 4410;
        const double sigma = 24.0;
        const double r = static_cast<double> (rate);
        const double tc = e.grainLen / 2.0;
        // renderEngine's pre-fill length, so the first spawn sees W0 = pre + 1
        const double defaultPos = e.grainLen * juce::jmax (0.0, r - 1.0) + 1500.0;
        const juce::int64 pre = static_cast<juce::int64> (static_cast<int> (defaultPos) + e.grainLen + 4000);
        const double peakAt = static_cast<double> (pre + 1) - 8000.0;
        auto in = [peakAt, sigma] (juce::int64 i)
        {
            const double d = (static_cast<double> (i) - peakAt) / sigma;
            return static_cast<float> (std::exp (-0.5 * d * d));
        };
        return renderEngine (e, in, [=] (juce::int64, int) { return r * tc + 8000.0 + srcOffset; });
    }

    // Peak lag of the cross-correlation b vs a (b later = positive), parabolic.
    double xcorrLag (const std::vector<float>& a, const std::vector<float>& b, int maxLag = 20)
    {
        auto xc = [&] (int lag)
        {
            double s = 0.0;
            for (size_t i = 0; i < a.size(); ++i)
            {
                const long j = static_cast<long> (i) + lag;
                if (j >= 0 && j < static_cast<long> (b.size()))
                    s += static_cast<double> (a[i]) * b[static_cast<size_t> (j)];
            }
            return s;
        };
        int best = 0; double bv = -1e30;
        for (int l = -maxLag; l <= maxLag; ++l) { const double v = xc (l); if (v > bv) { bv = v; best = l; } }
        const double ym = xc (best - 1), y0 = xc (best), yp = xc (best + 1);
        const double den = ym - 2.0 * y0 + yp;
        return best + (den != 0.0 ? 0.5 * (ym - yp) / den : 0.0);
    }

    // ── row 6: passband ────────────────────────────────────────────────────
    // Output frequency where the wanted level falls 1 dB below the 1 kHz-out reference.
    double passbandKHz (float rate, double fs)
    {
        Engine e; e.fs = fs; e.rate = rate; e.numOut = 1 << 15; e.grainLen = 2205; e.hop = 2205;
        e.extraPos = 0.37;   // fractional start: a rate-1 grain must not sit on integer taps
        const size_t n = 1 << 14;
        auto level = [&] (double fo)
        {
            const auto y = renderEngine (e, sineIn (fo / rate, fs));
            const auto p = powerSpectrum (y, y.size() - n, n);
            return splitPower (p, fs, n, fo, fs * 0.5, 600.0).wanted;
        };
        const double ref = level (1000.0);
        const double top = juce::jmin (juce::jmin (fs * 0.5 - 300.0, 21500.0),
                                       0.98 * fs * 0.5 * juce::jmin (1.0, static_cast<double> (rate)));
        double lo = 1000.0, hi = -1.0;
        for (double fo = 1500.0; fo <= top; fo += 500.0)
        {
            if (dB10 (level (fo) / ref) < -1.0) { hi = fo; break; }
            lo = fo;
        }
        if (hi < 0.0) return top / 1000.0;   // flat to the top of the scan
        for (int i = 0; i < 7; ++i)
        {
            const double mid = 0.5 * (lo + hi);
            if (dB10 (level (mid) / ref) < -1.0) hi = mid; else lo = mid;
        }
        return 0.5 * (lo + hi) / 1000.0;
    }

    // ── row 1e: processor end-to-end ───────────────────────────────────────
    using Params = std::map<juce::String, float>;

    void setParams (juce::AudioProcessor& p, const Params& params)
    {
        for (auto* ap : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
            {
                auto it = params.find (rp->getParameterID());
                if (it != params.end())
                    rp->setValueNotifyingHost (rp->convertTo0to1 (it->second));
            }
    }

    std::vector<float> renderProcessor (const Params& ps, double fs, double hz, int len)
    {
        std::unique_ptr<GrainScatterProcessor> p (dynamic_cast<GrainScatterProcessor*> (createPluginFilter()));
        p->setPlayConfigDetails (2, 2, fs, 512);
        setParams (*p, ps);
        p->setRngSeed (0xA11A5ull);
        p->prepareToPlay (fs, 512);
        p->reset();
        std::vector<float> out (static_cast<size_t> (len));
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        for (int pos = 0; pos < len; pos += 512)
        {
            const int n = juce::jmin (512, len - pos);
            block.setSize (2, n, false, false, true);
            for (int i = 0; i < n; ++i)
            {
                const float v = static_cast<float> (0.5 * std::sin (2.0 * kPi * hz * (pos + i) / fs));
                block.setSample (0, i, v);
                block.setSample (1, i, v);
            }
            p->processBlock (block, midi);
            for (int i = 0; i < n; ++i) out[static_cast<size_t> (pos + i)] = block.getSample (0, i);
        }
        return out;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    std::printf ("O-GrainScatter alias gate — %s (verdict <= -60 dB audible spurious)\n\n", kBuild);
    const double fs = 44100.0;
    const auto tones = toneSet();

    // ── 1: stereo ──────────────────────────────────────────────────────────
    std::printf ("Row 1 — stereo path, 44.1 kHz\n");
    for (int st : { 12, 24, 36 })
    {
        Engine e; e.fs = fs; e.rate = static_cast<float> (std::exp2 (st / 12.0));
        aliasRow ("1", juce::String::formatted ("+%d st stereo", st), e, true, tones, true);
    }

    // ── 1e: processor end-to-end ───────────────────────────────────────────
    std::printf ("Row 1e — processor end-to-end, +24 st transpose\n");
    {
        const Params ps { { "dry_level", -60.0f }, { "wet_level", 0.0f }, { "feedback", 0.0f },
                          { "sync_mode", 0.0f }, { "probability", 100.0f }, { "pitch_random", 0.0f },
                          { "pitch_transpose", 24.0f }, { "pitch_fine", 0.0f }, { "pan_random", 0.0f },
                          { "reverse", 0.0f }, { "size_random", 0.0f }, { "amp_random", 0.0f },
                          { "scan_position", 20.0f }, { "grain_shape", 0.0f }, { "spatial_mode", 0.0f },
                          { "freeze", 0.0f }, { "spread", 30.0f }, { "grain_size", 100.0f }, { "density", 50.0f } };
        const int len = static_cast<int> (fs * 4.0);
        const auto yr = renderProcessor (ps, fs, 250.0, len);
        const auto pr = powerSpectrum (yr, yr.size() - kN, kN);
        const double ref = splitPower (pr, fs, kN, 1000.0).wanted;
        double worst = -400.0, worstHz = 0.0;
        for (double hz : { 6000.0, 8000.0, 10000.0, 12500.0, 15000.0, 17500.0, 19500.0 })
        {
            const auto y = renderProcessor (ps, fs, hz, len);
            const auto p = powerSpectrum (y, y.size() - kN, kN);
            const double d = dB10 (splitPower (p, fs, kN, -1.0).spurious / ref);
            if (d > worst) { worst = d; worstHz = hz; }
        }
        verdict ("1", ref > 1e-6 && worst <= -60.0, "+24 st processBlock, out-of-band tones",
                 juce::String::formatted ("worst %6.1f dB (in %5.0f Hz), ref %.1f dB", worst, worstHz, dB10 (ref)));
    }

    // ── 2 / 3: spatial ─────────────────────────────────────────────────────
    std::printf ("Row 2 — +36 st spatial path (mono W), Doppler forced\n");
    for (float dop : { 1.0f, 1.4f })
    {
        Engine e; e.fs = fs; e.rate = 8.0f; e.spatial = true; e.doppler = dop;
        aliasRow ("2", juce::String::formatted ("+36 st spatial Doppler %.1f", dop), e, true, tones, true);
    }
    std::printf ("Row 3 — Doppler 2.0 (REPORT only)\n");
    {
        Engine e; e.fs = fs; e.rate = 8.0f; e.spatial = true; e.doppler = 2.0f;
        aliasRow ("3", "+36 st spatial Doppler 2.0 (r' 2.00, S6)", e, false, tones, true);
        e.rate = static_cast<float> (std::exp2 (35.0 / 12.0));
        aliasRow ("3", "+35 st spatial Doppler 2.0 (r' 3.78, S7)", e, false, tones, true);
    }

    // ── 4: frozen ──────────────────────────────────────────────────────────
    std::printf ("Row 4 — frozen +36 st\n");
    {
        // loop = (2 s + 1) - 10 ms crossfade; tones on whole cycles of it
        const int len = static_cast<int> (fs * 2.0) + 1;
        const int loop = len - juce::jmin (static_cast<int> (fs * 0.010), len / 4);
        std::vector<double> ft;
        for (double f : tones)
            ft.push_back (std::round (f * loop / fs) * fs / loop);
        // Every grain reads snapshot [1000, 1000 + 8 x 4410): inside the loop, clear of the
        // seam crossfade. (Grains crossing the seam get the v3.0.4 equal-power blend of
        // correlated audio, a ~+3 dB bump — amplitude modulation in BOTH builds, not
        // aliasing; it is out of this row's scope.)
        Engine e; e.fs = fs; e.rate = 8.0f; e.frozen = true; e.frozenStart = 1000.0;
        aliasRow ("4", "frozen +36 st stereo", e, true, ft, false);
    }

    // ── 5: alignment ───────────────────────────────────────────────────────
    std::printf ("Row 5 — octave-boundary alignment (r 1.99 level 0 vs 2.01 level 1)\n");
    {
        const auto a = transientGrain (1.99f, fs, 0.0);
        const auto b = transientGrain (2.01f, fs, 0.0);
        const auto c = transientGrain (2.01f, fs, 0.2);   // positive control: 0.2 source samples
        auto peakOf = [] (const std::vector<float>& v) { size_t im = 0; for (size_t i = 0; i < v.size(); ++i) if (v[i] > v[im]) im = i; return std::make_pair (im, v[im]); };
        const auto pa = peakOf (a), pb = peakOf (b);
        // liveness: both grains must actually carry the transient, at the grain centre
        verdict ("5", pa.second > 0.5f && pb.second > 0.5f && std::abs ((double) pa.first - 2205.0) <= 2.0
                       && std::abs ((double) pb.first - 2205.0) <= 2.0,
                 "transient present at the grain centre", juce::String::formatted ("peaks %.3f @ %d, %.3f @ %d",
                     (double) pa.second, (int) pa.first, (double) pb.second, (int) pb.first));
        const double lag = xcorrLag (a, b) * 2.0;          // output samples -> source samples (r ~ 2)
        const double lagC = xcorrLag (a, c) * 2.0;
        verdict ("5", std::abs (lag) <= 0.05, "transient lag 1.99 vs 2.01",
                 juce::String::formatted ("%+.4f source samples", lag));
        verdict ("5", std::abs (lagC) > 0.05, "positive control: +0.2 sample offset seen",
                 juce::String::formatted ("%+.4f source samples", lagC));
    }

    // ── 6: passband ────────────────────────────────────────────────────────
    std::printf ("Row 6 — output -1 dB passband, kHz (REPORT)\n");
    std::printf ("         rate    st      r'   level   44.1 k   48 k   | RESEARCH §5 T16 @44.1 / @48\n");
    {
        struct R { double rate; const char* research; };
        const R rates[] = { { 0.5, "8.0 / 9.6 (r' 0.5)" }, { 0.707, "11.2 / 14.1" }, { 1.0, "15.9 / 19.2" },
                            { 1.19, "18.9 / 23.0*" }, { 1.414, "12.6 / 17.4" }, { 1.68, "11.0 / 13.6" },
                            { 1.99, "9.1 / 10.9" }, { 2.0, "15.9 / 19.2 (r' 1)" }, { 2.83, "12.6 / 17.4 (r' 1.414)" },
                            { 3.98, "9.1 / 10.9 (r' 1.99)" }, { 4.0, "(r' 1)" }, { 7.96, "(r' 1.99)" }, { 8.0, "(r' 1)" } };
        for (const auto& r : rates)
        {
            const int lvl = r.rate >= 8.0 ? 3 : r.rate >= 4.0 ? 2 : r.rate >= 2.0 ? 1 : 0;
            const double pb44 = passbandKHz (static_cast<float> (r.rate), 44100.0);
            const double pb48 = passbandKHz (static_cast<float> (r.rate), 48000.0);
            std::printf ("       %6.3f %+6.1f  %6.3f    %d     %6.2f  %6.2f   | %s\n", r.rate, 12.0 * std::log2 (r.rate),
                         r.rate / (1 << lvl), lvl, pb44, pb48, r.research);
        }
    }

    // ── 7: CPU ─────────────────────────────────────────────────────────────
    std::printf ("Row 7 — CPU, 64 grains at +24 st, 1 s grains (REPORT)\n");
    for (bool sp : { false, true })
    {
        Engine e; e.fs = 48000.0; e.rate = 4.0f; e.spatial = sp; e.grainLen = 48000; e.hop = 1 << 30;
        e.maxGrains = 64; e.numOut = 40000;
        double best = 1e30;
        for (int k = 0; k < 5; ++k)
        {
            double ns = 0.0;
            renderEngine (e, sineIn (440.0, e.fs), [] (juce::int64, int g) { return 3.0 * 48000.0 + 1500.0 + 211.0 * g; }, &ns);
            best = juce::jmin (best, ns);
        }
        report (juce::String (sp ? "spatial" : "stereo") + " 64 grains +24 st",
                juce::String::formatted ("%.0f ns / output sample (%.1f ns per grain-sample)", best, best / 64.0));
    }

    // ── result ─────────────────────────────────────────────────────────────
   #ifdef GS_ALIAS_V320
    const bool r1 = rowOk["1"], r2 = rowOk["2"], r4 = rowOk["4"];
    const bool allFail = ! r1 && ! r2 && ! r4;
    std::printf ("\nNegative control: rows 1 / 2 / 4 %s / %s / %s -> %s\n", r1 ? "PASS" : "FAIL", r2 ? "PASS" : "FAIL",
                 r4 ? "PASS" : "FAIL", allFail ? "the gate sees v3.2.0's aliasing (OK)" : "VACUOUS: v3.2.0 passed a row");
    return allFail ? 0 : 1;
   #else
    std::printf ("\n%s (%d failing)\n", verdictFails == 0 ? "ALL PASS" : "FAIL", verdictFails);
    return verdictFails == 0 ? 0 : 1;
   #endif
}
