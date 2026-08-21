/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-Emulator render-harness — Phase 2.1 probe set (engine skeleton + SNES).

    Probes:

      P0 contract     — the 5 BINDING APVTS parameters (types, defaults,
                        ranges; console has exactly 5 choices).
      A  latency      — reported latency equals the engine's computed figure
                        (read through the harness accessor, never a mirrored
                        formula), bounded by kMaxWetLatencySamples.
      B  FUNC-02 null — mix 0 %: output bit-equals input delayed by the
                        REPORTED latency, no tolerance, after a >0.05 s
                        warm-up skip (DryWetMixer gain smoothers).
      AN invariance   — block-size digest sweep {64} / {512} / {4096} /
                        {1,7,64,333,4096}, each vs a fixed {512} reference
                        (O-Octagon AN shape). Bit-identity by memcmp.
      E  determinism  — two fresh same-config instances render bit-identically.
      Z  latency xcorr— measured wet delay (cross-correlation peak) within
                        ±15 samples of the reported figure (IIR passband
                        group delay budget, L120).
      C  crush        — min ≠ max liveness: crush 0 vs 100 renders differ,
                        both live, both bounded.
      S  SNES sig     — spectral signature: hi-band rolloff + centroid
                        darkening vs the input (Gaussian/AA/output-LP), and
                        codec noise-floor rise on a sine (ratios vs control
                        renders, never absolute magnitudes).
      U  QUAL-01      — pathological timeline (DC, silence, FS square,
                        1.0e-40f denormals, NaN stretch, sine recovery):
                        bounded finite output everywhere (input NaN is
                        scrubbed at the processBlock boundary), non-sticky
                        recovery.
      D  digests      — canonical FNV-1a 64 anchor. The Stage-1 passthrough
                        anchor is RETIRED and asserted MOVED
                        (pattern_reanchor_cross_version_digest_probe); the
                        Phase 2.1 anchor is recorded at the phase commit.

    Phase 2.2 probes (PS1 + SPU reverb):

      F  PS1!=SNES    — same material through both consoles: renders differ,
                        and PS1's 9-14 kHz band collapses vs SNES (22050 Hz
                        domain + lower AA corner).
      Z2 PS1 latency  — the PS1 pipeline lands on the SAME reported worst-case
                        figure (xcorr peak within ±15).
      R0 reverb live  — reverb 0 vs 100 renders differ, bounded (macro
                        min != max liveness).
      R1 IR tail      — wet-minus-dry isolates the reverb return on an
                        impulse: first reflection lands in the comb-delay
                        window after the aligned direct arrival, the early
                        tail is live, and the decay is short/murky (1.6-2.0 s
                        window << 50-450 ms window).
      R2 stability    — 60 s of noise @ reverb 100 % + crush 100 %: finite,
                        bounded, no growth (last-5s RMS vs 5-10s RMS).
      AN2 invariance  — block-size digest sweep RE-RUN with the reverb + PS1
                        codec engaged.
      D2 digest       — Phase 2.2 canonical (PS1 + reverb) anchor, 0-sentinel
                        RECORD pattern. The Phase 2.1 SNES anchor is expected
                        to SURVIVE 2.2 (reverb defaults to 0 and sits behind
                        an exact-inactive gate; the SNES op sequence and
                        priming are unchanged) — probe D still asserts it.

    Conventions (O-Bitrot harness header): setBaseline() first in every
    probe; setValueNotifyingHost only; position-hashed excitation; liveness
    clauses on every potentially-vacuous probe; no wall-clock in verdicts;
    fixed settle lengths.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace
{
    constexpr double kFs       = 48000.0;
    constexpr int    kMaxBlock = 4096;
    constexpr int    kRenderSamples = 512 * 64;   // canonical render length (32768)
    constexpr double kSigHz    = 1000.0;

    //==========================================================================
    // ── Digest anchors (plan decision #1: per-phase, re-anchor discipline) ───

    /** Stage-1 passthrough baseline — RETIRED. Phase 2.1's engine must MOVE
        this digest (a canonical render that still matches it means the wet
        pipeline is not in the signal path). */
    constexpr juce::uint64 kDigestStage1PassthroughRetired = 0x28e7675cdbec475cULL;

    /** Phase 2.1 canonical SNES digest (recorded at the 2.1 commit). STILL
        ASSERTED in Phase 2.2: the canonical render keeps reverb at its
        default 0, which holds the reverb path behind ConsoleEngine's sticky
        exact-inactive gate (no send, no tick, no `+=` on the output — an
        unconditional sum would flip -0.0 samples and move this digest), and
        the SNES pipeline's op sequence and upsample priming are unchanged.
        If this probe fails after a 2.2+ change, that is a REGRESSION signal
        on the SNES path, not a re-anchor event. */
    constexpr juce::uint64 kDigestAnchor21CanonicalSnes = 0x59d72af3f1b80676ULL;

    /** Phase 2.2 canonical PS1+reverb digest — 0 means NOT YET RECORDED: the
        probe prints the digest and passes with a RECORD-ME notice; fill this
        in at the Phase 2.2 commit (first green run on the reference
        machine). */
    constexpr juce::uint64 kDigestAnchor22Ps1ReverbCanonical = 0x58cb7f909f6a6e30ULL;

    //==========================================================================
    int failures = 0;

    void check (const char* name, bool ok, const juce::String& detail = {})
    {
        std::printf ("  [%s] %s%s%s\n", ok ? "PASS" : "FAIL", name,
                     detail.isEmpty() ? "" : " — ",
                     detail.isEmpty() ? "" : detail.toRawUTF8());
        if (! ok)
            ++failures;
    }

    //==========================================================================
    // ── Excitation: pure functions of (channel, absolute index) ─────────────

    float noiseAt (int ch, int n)
    {
        uint32_t h = (uint32_t) n * 2654435761u ^ (uint32_t) (ch + 1) * 40503u;
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        return (float) ((double) h / 2147483648.0 - 1.0);   // [-1, 1)
    }

    /** Half-scale noise: leaves headroom for the crush drive so the codec is
        exercised rather than parked at the rails. */
    float noiseHalfAt (int ch, int n)
    {
        return 0.5f * noiseAt (ch, n);
    }

    /** Position-computed sine (phase from n directly — never an accumulated
        oscillator). */
    float sineAt (int ch, int n)
    {
        juce::ignoreUnused (ch);
        return 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                        * kSigHz * (double) n / kFs);
    }

    /** QUAL-01 pathological timeline: DC, silence, full-scale square, a
        DENORMAL stretch (1.0e-40f — the Octagon AR case the Bitrot timeline
        lacks), 100 ms of NaN, then clean sine — recovery is the criterion. */
    float pathologicalAt (int ch, int n)
    {
        juce::ignoreUnused (ch);
        if (n <  24000) return 0.8f;                                    // DC
        if (n <  48000) return 0.0f;                                    // silence
        if (n <  72000) return ((n / 120) & 1) ? -1.0f : 1.0f;          // FS square ~200 Hz
        if (n <  96000) return 1.0e-40f;                                // denormals
        if (n < 100800) return std::numeric_limits<float>::quiet_NaN(); // 100 ms NaN
        return 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                        * kSigHz * (double) (n - 100800) / kFs);
    }

    /** Single impulse well past the latency + smoother warm-up (100 ms). */
    float impulseAt (int ch, int n)
    {
        juce::ignoreUnused (ch);
        return n == 4800 ? 0.9f : 0.0f;
    }

    using InputFn = float (*) (int ch, int n);

    //==========================================================================
    // ── Parameter helpers ────────────────────────────────────────────────────

    /** Engineering units, synchronously — bare setValue() leaves the cached
        atomics stale and tests the defaults while reporting green. */
    void setParam (OEmulatorAudioProcessor& proc, const char* id, float engineeringValue)
    {
        auto* p = proc.apvts.getParameter (id);
        jassert (p != nullptr);

        if (p != nullptr)
            p->setValueNotifyingHost (p->convertTo0to1 (engineeringValue));
        else
            std::printf ("  !! unknown param id '%s'\n", id);
    }

    /** ALL 5 parameters to the parameter-spec defaults. Trap: neutral is not
        the range minimum (mix 100, crush 50, age 20). */
    void setBaseline (OEmulatorAudioProcessor& proc)
    {
        setParam (proc, "console", 0.0f);    // SNES
        setParam (proc, "crush",   50.0f);
        setParam (proc, "age",     20.0f);   // exists; unwired until Phase 2.4
        setParam (proc, "reverb",  0.0f);    // exists; unwired until Phase 2.2
        setParam (proc, "mix",     100.0f);
    }

    /** Fresh, prepared, baselined processor. */
    std::unique_ptr<OEmulatorAudioProcessor> makeProc()
    {
        auto p = std::make_unique<OEmulatorAudioProcessor>();
        p->setPlayConfigDetails (2, 2, kFs, kMaxBlock);
        p->prepareToPlay (kFs, kMaxBlock);
        setBaseline (*p);
        return p;
    }

    //==========================================================================
    // ── Render helpers ───────────────────────────────────────────────────────

    /** Renders `totalSamples` into `dest`, chopping processBlock calls per the
        cyclically-walked `sizes`. Input is a pure function of the absolute
        sample index, so every size sequence sees the SAME signal. */
    void renderInto (OEmulatorAudioProcessor& proc, juce::AudioBuffer<float>& dest,
                     int totalSamples, const std::vector<int>& sizes, InputFn input)
    {
        juce::MidiBuffer midi;

        dest.setSize (2, totalSamples);
        dest.clear();

        juce::AudioBuffer<float> scratch (2, kMaxBlock);

        int    n  = 0;
        size_t si = 0;

        while (n < totalSamples)
        {
            int chunk = sizes[si % sizes.size()];
            ++si;
            chunk = juce::jmin (chunk, totalSamples - n);
            if (chunk <= 0)
                chunk = 1;

            juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, chunk);

            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < chunk; ++s)
                    block.setSample (ch, s, input (ch, n + s));

            proc.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                dest.copyFrom (ch, n, block, ch, 0, chunk);

            n += chunk;
        }
    }

    /** Canonical vector render (interleaved L,R) — SAME shape as the Stage-1
        harness so the retired-anchor comparison compares like with like. */
    void renderInterleaved (OEmulatorAudioProcessor& proc, std::vector<float>& out,
                            int totalSamples, InputFn input)
    {
        juce::AudioBuffer<float> buf;
        renderInto (proc, buf, totalSamples, { 512 }, input);

        out.clear();
        out.reserve ((size_t) totalSamples * 2);
        for (int n = 0; n < totalSamples; ++n)
            for (int ch = 0; ch < 2; ++ch)
                out.push_back (buf.getSample (ch, n));
    }

    juce::uint64 fnv1a64 (const std::vector<float>& v)
    {
        juce::uint64 h = 1469598103934665603ull;
        const auto* bytes = reinterpret_cast<const unsigned char*> (v.data());
        for (size_t i = 0; i < v.size() * sizeof (float); ++i)
        {
            h ^= bytes[i];
            h *= 1099511628211ull;
        }
        return h;
    }

    //==========================================================================
    // ── Comparison helpers ───────────────────────────────────────────────────

    bool bitExact (float a, float b)
    {
        return std::memcmp (&a, &b, sizeof (float)) == 0;
    }

    /** memcmp, NOT a tolerance — PERF-02 says bit-identical. */
    bool bitIdentical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels()
            || a.getNumSamples() != b.getNumSamples())
            return false;

        const auto bytes = sizeof (float) * (size_t) a.getNumSamples();

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), bytes) != 0)
                return false;

        return true;
    }

    juce::String firstDifference (const juce::AudioBuffer<float>& a,
                                  const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int n = 0; n < a.getNumSamples(); ++n)
                if (! bitExact (a.getSample (ch, n), b.getSample (ch, n)))
                    return juce::String ("first diff ch") + juce::String (ch)
                         + " @" + juce::String (n)
                         + " (" + juce::String (a.getSample (ch, n), 9) + " vs "
                         + juce::String (b.getSample (ch, n), 9) + ")";
        return "identical";
    }

    double maxAbsDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        double m = 0.0;
        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int n = 0; n < a.getNumSamples(); ++n)
                m = juce::jmax (m, std::abs ((double) a.getSample (ch, n)
                                             - (double) b.getSample (ch, n)));
        return m;
    }

    std::vector<float> channelToVector (const juce::AudioBuffer<float>& b, int ch)
    {
        const auto* p = b.getReadPointer (ch);
        return std::vector<float> (p, p + b.getNumSamples());
    }

    double rmsRange (const std::vector<float>& x, int from, int to)
    {
        double acc = 0.0;
        for (int n = from; n < to; ++n)
            acc += (double) x[(size_t) n] * (double) x[(size_t) n];
        return std::sqrt (acc / (double) juce::jmax (1, to - from));
    }

    //==========================================================================
    /** Peak lag of normalized cross-correlation: a[n] vs b[n + lag] over
        [off, off+len), lag in [loLag, hiLag]. Positive lag = b is LATER.
        Caller keeps off+len+hiLag in range. */
    int xcorrBestLag (const std::vector<float>& a, const std::vector<float>& b,
                      int off, int len, int loLag, int hiLag, double& corrOut)
    {
        double ea = 0.0;
        for (int n = 0; n < len; ++n)
            ea += (double) a[(size_t) (off + n)] * a[(size_t) (off + n)];

        double best = -2.0;
        int bestLag = loLag;

        for (int lag = loLag; lag <= hiLag; ++lag)
        {
            double acc = 0.0, eb = 0.0;
            for (int n = 0; n < len; ++n)
            {
                const double bv = (double) b[(size_t) (off + n + lag)];
                acc += (double) a[(size_t) (off + n)] * bv;
                eb  += bv * bv;
            }

            const double c = acc / (std::sqrt (ea * eb) + 1.0e-12);
            if (c > best)
            {
                best = c;
                bestLag = lag;
            }
        }

        corrOut = best;
        return bestLag;
    }

    //==========================================================================
    // ── Spectrum (O-simpleSampler model: flatness; + centroid) ───────────────

    struct Spectrum
    {
        std::vector<float> mag;     // size N/2+1
        double fs = 0;
        int fftSize = 0;

        double binHz() const { return fs / (double) fftSize; }

        double bandEnergy (double loHz, double hiHz) const
        {
            double e = 0.0;
            for (size_t k = 0; k < mag.size(); ++k)
            {
                const double f = (double) k * binHz();
                if (f >= loHz && f <= hiHz)
                    e += (double) mag[k] * mag[k];
            }
            return e;
        }

        /** Spectral flatness (geo/arith mean) over a band: 1.0 = noisy,
            -> 0 = tonal. Quantization noise fills inter-harmonic valleys ->
            flatness rises. */
        double flatness (double loHz, double hiHz) const
        {
            double logSum = 0.0, sum = 0.0;
            int n = 0;
            for (size_t k = 0; k < mag.size(); ++k)
            {
                const double f = (double) k * binHz();
                if (f < loHz || f > hiHz)
                    continue;
                const double m = (double) mag[k] + 1.0e-12;
                logSum += std::log (m);
                sum    += m;
                ++n;
            }
            if (n == 0 || sum <= 0)
                return 0.0;
            return std::exp (logSum / n) / (sum / n);
        }

        /** Power-weighted spectral centroid, Hz. */
        double centroid (double loHz, double hiHz) const
        {
            double num = 0.0, den = 0.0;
            for (size_t k = 0; k < mag.size(); ++k)
            {
                const double f = (double) k * binHz();
                if (f < loHz || f > hiHz)
                    continue;
                const double p = (double) mag[k] * mag[k];
                num += f * p;
                den += p;
            }
            return den > 0.0 ? num / den : 0.0;
        }
    };

    Spectrum analyze (const std::vector<float>& x, int off, double fs, int order = 14)
    {
        const int N = 1 << order;                       // default 16384
        juce::dsp::FFT fft (order);

        std::vector<float> buf ((size_t) (2 * N), 0.0f);
        juce::dsp::WindowingFunction<float> win ((size_t) N,
                                                 juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> seg ((size_t) N, 0.0f);
        for (int n = 0; n < N; ++n)
        {
            const int idx = off + n;
            seg[(size_t) n] = (idx >= 0 && idx < (int) x.size()) ? x[(size_t) idx] : 0.0f;
        }
        win.multiplyWithWindowingTable (seg.data(), (size_t) N);
        for (int n = 0; n < N; ++n)
            buf[(size_t) n] = seg[(size_t) n];

        fft.performRealOnlyForwardTransform (buf.data());

        Spectrum s;
        s.fs = fs;
        s.fftSize = N;
        s.mag.resize ((size_t) (N / 2 + 1));
        for (int k = 0; k <= N / 2; ++k)
        {
            const float re = buf[(size_t) (2 * k)];
            const float im = buf[(size_t) (2 * k + 1)];
            s.mag[(size_t) k] = std::sqrt (re * re + im * im);
        }
        return s;
    }
} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("O-Emulator render-harness (Phase 2.1: SNES end-to-end) — fs=%.0f\n", kFs);

    //==========================================================================
    // P0 — parameter contract (BINDING parameter-spec.md).
    {
        auto p = makeProc();

        auto* console = dynamic_cast<juce::AudioParameterChoice*> (p->apvts.getParameter ("console"));
        check ("P0 console exists+choice", console != nullptr);
        if (console != nullptr)
        {
            check ("P0 console 5 choices", console->choices.size() == 5,
                   juce::String (console->choices.size()));
            check ("P0 console default SNES", console->getIndex() == 0);
        }

        struct { const char* id; float def; } floats[] = {
            { "crush", 50.0f }, { "age", 20.0f }, { "reverb", 0.0f }, { "mix", 100.0f } };

        for (const auto& f : floats)
        {
            auto* fp = dynamic_cast<juce::AudioParameterFloat*> (p->apvts.getParameter (f.id));
            check ((juce::String ("P0 ") + f.id + " exists+float").toRawUTF8(), fp != nullptr);
            if (fp != nullptr)
            {
                check ((juce::String ("P0 ") + f.id + " default").toRawUTF8(),
                       juce::approximatelyEqual (fp->get(), f.def),
                       juce::String (fp->get()));
                check ((juce::String ("P0 ") + f.id + " range 0-100").toRawUTF8(),
                       juce::approximatelyEqual (fp->range.start, 0.0f)
                           && juce::approximatelyEqual (fp->range.end, 100.0f));
            }
        }
    }

    //==========================================================================
    // A — latency reported once, equal to the engine's computed figure. Read
    // through the instrumentation accessor, never a formula mirrored here
    // (pattern_test_fixture_mirrors_drift_silently).
    int kComp = 0;
    {
        auto p = makeProc();
        const int reported = p->getLatencySamples();
        const int computed = p->getComputedLatencyForTest();
        kComp = reported;

        check ("A latency-reported", reported == computed && reported > 32 && reported <= 1024,
               juce::String ("getLatencySamples()=") + juce::String (reported)
                   + " computed=" + juce::String (computed));
    }

    //==========================================================================
    // B — FUNC-02 delay-compensated null at mix 0 %: out[n] bit-equals
    // in[n - kComp]. No tolerance. Warm-up skip > 0.05 s for the DryWetMixer
    // gain smoothers (their ramp starts at the mix 100 -> 0 step).
    {
        auto p = makeProc();
        setParam (*p, "mix", 0.0f);

        const int total = 48000;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseHalfAt);

        const int  startAt = kComp + (int) (0.25 * kFs);
        bool       ok      = true;
        juce::String detail;

        for (int ch = 0; ch < 2 && ok; ++ch)
        {
            const auto* o = out.getReadPointer (ch);
            for (int n = startAt; n < total; ++n)
            {
                if (! bitExact (o[n], noiseHalfAt (ch, n - kComp)))
                {
                    ok = false;
                    detail = juce::String ("first mismatch ch") + juce::String (ch)
                           + " @" + juce::String (n)
                           + " (" + juce::String (o[n], 9) + " vs "
                           + juce::String (noiseHalfAt (ch, n - kComp), 9) + ")";
                    break;
                }
            }
        }

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;
        if (ok && live)
            detail = juce::String ("bit-exact null over [")
                   + juce::String (startAt) + ", " + juce::String (total) + ")";

        check ("B FUNC-02 mix-0 null", ok && live,
               detail + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // AN — PERF-02 block-size invariance: digest sweep, several block-size
    // regimes vs a fixed {512} reference (O-Octagon AN shape). Crush off the
    // default so the shift floor / drive paths are exercised.
    {
        const int total = kRenderSamples;

        auto configure = [] (OEmulatorAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "crush", 65.0f);
        };

        auto ref = makeProc();
        configure (*ref);
        juce::AudioBuffer<float> outRef;
        renderInto (*ref, outRef, total, { 512 }, noiseHalfAt);

        const std::vector<std::vector<int>> sizeSets
            { { 64 }, { 512 }, { 4096 }, { 1, 7, 64, 333, 4096 } };
        const char* setNames[] = { "{64}", "{512}", "{4096}", "{1,7,64,333,4096}" };

        bool ok = true;
        juce::String detail;

        for (size_t s = 0; s < sizeSets.size(); ++s)
        {
            auto proc = makeProc();
            configure (*proc);

            juce::AudioBuffer<float> out;
            renderInto (*proc, out, total, sizeSets[s], noiseHalfAt);

            if (! bitIdentical (outRef, out))
            {
                ok = false;
                detail << setNames[s] << ": " << firstDifference (outRef, out) << "; ";
            }
        }

        const bool live = outRef.getMagnitude (0, 0, total) > 1.0e-4f;
        if (ok && live)
            detail = "4 block-size regimes vs fixed {512}: all bit-identical";

        check ("AN PERF-02 blocksize-sweep", ok && live,
               detail + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // E — determinism on FRESH instances: same config, same input, same bytes.
    {
        const int total = kRenderSamples;

        auto configure = [] (OEmulatorAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "crush", 80.0f);
        };

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);

        juce::AudioBuffer<float> outA, outB;
        renderInto (*a, outA, total, { 512 }, noiseHalfAt);
        renderInto (*b, outB, total, { 512 }, noiseHalfAt);

        const bool identical = bitIdentical (outA, outB);
        const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("E determinism", identical && live,
               (identical ? juce::String ("two fresh instances: bit-identical")
                          : firstDifference (outA, outB))
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // Z — measured wet latency vs reported. Crush 0 (most transparent round
    // trip); xcorr peak of output against input must land within ±15 samples
    // of kComp (grid jitter + 8th-order Butterworth passband group delay +
    // priming-estimate error, L120).
    {
        auto p = makeProc();
        setParam (*p, "crush", 0.0f);

        const int total = kRenderSamples;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseHalfAt);

        std::vector<float> in ((size_t) total);
        for (int n = 0; n < total; ++n)
            in[(size_t) n] = noiseHalfAt (0, n);

        const auto outV = channelToVector (out, 0);

        double corr = 0.0;
        const int off = 8192, len = 8192;
        const int lag = xcorrBestLag (in, outV, off, len,
                                      juce::jmax (0, kComp - 200), kComp + 200, corr);

        const bool aligned = std::abs (lag - kComp) <= 15;
        const bool corrOk  = corr > 0.25;   // degraded but correlated — liveness

        check ("Z latency-xcorr", aligned && corrOk,
               juce::String ("peak lag ") + juce::String (lag)
                   + " vs reported " + juce::String (kComp)
                   + " (budget ±15), corr " + juce::String (corr, 3)
                   + (corrOk ? "" : " — DECORRELATED, probe vacuous"));
    }

    //==========================================================================
    // C — crush min != max liveness, and QUAL-01 boundedness at the extreme.
    {
        const int total = kRenderSamples;

        auto lo = makeProc();  setParam (*lo, "crush", 0.0f);
        auto hi = makeProc();  setParam (*hi, "crush", 100.0f);

        juce::AudioBuffer<float> outLo, outHi;
        renderInto (*lo, outLo, total, { 512 }, noiseHalfAt);
        renderInto (*hi, outHi, total, { 512 }, noiseHalfAt);

        const double diff   = maxAbsDiff (outLo, outHi);
        const bool liveLo   = outLo.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool liveHi   = outHi.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool differs  = diff > 1.0e-3;
        const bool bounded  = outHi.getMagnitude (0, 0, total) < 4.0f
                           && outLo.getMagnitude (0, 0, total) < 4.0f;

        check ("C crush min!=max", differs && liveLo && liveHi && bounded,
               juce::String ("maxAbsDiff(0% vs 100%)=") + juce::String (diff, 6)
                   + (differs ? "" : " — CRUSH DOES NOT REACH THE DSP")
                   + ((liveLo && liveHi) ? "" : " — SILENT, probe vacuous")
                   + (bounded ? "" : " — UNBOUNDED OUTPUT"));
    }

    //==========================================================================
    // S — SNES spectral signature (DSP-02): ratios vs control renders, never
    // absolute magnitudes.
    //   (a) noise in: hi-band (15-21 kHz) energy collapses (console Nyquist
    //       16 kHz + Gaussian rolloff + 10 kHz output LP) and the centroid
    //       darkens vs the input.
    //   (b) sine in: codec quantization noise raises the 2.5-8 kHz band far
    //       above the clean sine's leakage floor.
    {
        const int total = kRenderSamples;
        const int off   = 12288;   // past warm-up; off + 16384 <= total

        // (a) broadband noise
        {
            auto p = makeProc();   // baseline: crush 50, mix 100
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, noiseHalfAt);

            std::vector<float> in ((size_t) total);
            for (int n = 0; n < total; ++n)
                in[(size_t) n] = noiseHalfAt (0, n);

            const auto sWet = analyze (channelToVector (out, 0), off, kFs);
            const auto sIn  = analyze (in, off, kFs);

            const double hiRatio  = sWet.bandEnergy (15000.0, 21000.0)
                                  / (sIn.bandEnergy (15000.0, 21000.0) + 1.0e-12);
            const double midRatio = sWet.bandEnergy (500.0, 3000.0)
                                  / (sIn.bandEnergy (500.0, 3000.0) + 1.0e-12);
            const double centWet  = sWet.centroid (100.0, 21000.0);
            const double centIn   = sIn.centroid (100.0, 21000.0);

            const bool rolledOff = hiRatio < 0.1;
            const bool darkened  = centWet < 0.7 * centIn;
            const bool live      = midRatio > 0.02;   // wet band carries real energy

            check ("S1 SNES rolloff+darkening", rolledOff && darkened && live,
                   juce::String ("hi-band ratio ") + juce::String (hiRatio, 4)
                       + " (< 0.1), centroid " + juce::String (centWet, 0) + " vs "
                       + juce::String (centIn, 0) + " Hz (< 0.7x), mid ratio "
                       + juce::String (midRatio, 3)
                       + (live ? "" : " — WET PATH SILENT, probe vacuous"));
        }

        // (b) sine noise floor
        {
            auto p = makeProc();
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, sineAt);

            std::vector<float> in ((size_t) total);
            for (int n = 0; n < total; ++n)
                in[(size_t) n] = sineAt (0, n);

            const auto sWet = analyze (channelToVector (out, 0), off, kFs);
            const auto sIn  = analyze (in, off, kFs);

            const double floorRatio = sWet.bandEnergy (2500.0, 8000.0)
                                    / (sIn.bandEnergy (2500.0, 8000.0) + 1.0e-12);
            const double toneWet    = sWet.bandEnergy (800.0, 1200.0);
            const double toneIn     = sIn.bandEnergy (800.0, 1200.0);
            const bool   toneLive   = toneWet > 1.0e-4 * toneIn;   // fundamental survives

            check ("S2 SNES codec-noise-floor", floorRatio > 5.0 && toneLive,
                   juce::String ("2.5-8 kHz wet/dry energy ratio ")
                       + juce::String (floorRatio, 2) + " (> 5)"
                       + (toneLive ? "" : " — FUNDAMENTAL MISSING, probe vacuous"));
        }
    }

    //==========================================================================
    // U — QUAL-01 pathological timeline: bounded finite output EVERYWHERE
    // (the NaN stretch is scrubbed at the processBlock boundary, so even the
    // NaN era must render finite), and non-sticky recovery afterwards.
    {
        auto p = makeProc();

        const int total = 216000;   // 4.5 s
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, pathologicalAt);

        bool allFinite = true;
        float peak = 0.0f;
        int firstBad = -1;

        for (int ch = 0; ch < 2 && allFinite; ++ch)
        {
            const auto* o = out.getReadPointer (ch);
            for (int n = 0; n < total; ++n)
            {
                if (! std::isfinite (o[n]))
                {
                    allFinite = false;
                    firstBad = n;
                    break;
                }
                peak = juce::jmax (peak, std::abs (o[n]));
            }
        }

        // Recovery: the clean sine after the NaN era must be audibly present
        // in the last 0.5 s.
        double acc = 0.0;
        {
            const auto* o = out.getReadPointer (0);
            for (int n = total - 24000; n < total; ++n)
                acc += (double) o[n] * (double) o[n];
        }
        const double recoveryRms = std::sqrt (acc / 24000.0);

        const bool bounded   = peak < 4.0f;
        const bool recovered = recoveryRms > 1.0e-3;

        check ("U QUAL-01 pathological", allFinite && bounded && recovered,
               juce::String ("finite ") + (allFinite ? "yes" : juce::String ("NO @") + juce::String (firstBad))
                   + ", peak " + juce::String (peak, 4)
                   + " (< 4), recovery RMS " + juce::String (recoveryRms, 5)
                   + (recovered ? "" : " — STUCK SILENT AFTER NaN ERA"));
    }

    //==========================================================================
    // F — FUNC-01 (partial): PS1 and SNES are measurably distinct on the same
    // material. PS1's 22050 Hz domain + 9.9 kHz AA corner collapse the
    // 9-14 kHz band SNES still carries.
    {
        const int total = kRenderSamples;
        const int off   = 12288;

        auto snes = makeProc();
        auto ps1  = makeProc();
        setParam (*ps1, "console", 1.0f);

        juce::AudioBuffer<float> outS, outP;
        renderInto (*snes, outS, total, { 512 }, noiseHalfAt);
        renderInto (*ps1,  outP, total, { 512 }, noiseHalfAt);

        const double diff = maxAbsDiff (outS, outP);

        const auto sS = analyze (channelToVector (outS, 0), off, kFs);
        const auto sP = analyze (channelToVector (outP, 0), off, kFs);

        const double hiS = sS.bandEnergy (9000.0, 14000.0);
        const double hiP = sP.bandEnergy (9000.0, 14000.0);
        const double hiRatio = hiP / (hiS + 1.0e-12);

        const bool differs = diff > 1.0e-3;
        const bool darker  = hiRatio < 0.5;
        const bool live    = outS.getMagnitude (0, 0, total) > 1.0e-4f
                          && outP.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("F PS1!=SNES", differs && darker && live,
               juce::String ("maxAbsDiff ") + juce::String (diff, 5)
                   + ", 9-14 kHz PS1/SNES energy ratio " + juce::String (hiRatio, 4)
                   + " (< 0.5)"
                   + (differs ? "" : " — CONSOLE DOES NOT REACH THE DSP")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // Z2 — the PS1 pipeline lands on the SAME reported worst-case latency
    // (constant across modes — no PDC renegotiation on console switch).
    {
        auto p = makeProc();
        setParam (*p, "console", 1.0f);
        setParam (*p, "crush", 0.0f);

        const int total = kRenderSamples;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseHalfAt);

        std::vector<float> in ((size_t) total);
        for (int n = 0; n < total; ++n)
            in[(size_t) n] = noiseHalfAt (0, n);

        const auto outV = channelToVector (out, 0);

        double corr = 0.0;
        const int off = 8192, len = 8192;
        const int lag = xcorrBestLag (in, outV, off, len,
                                      juce::jmax (0, kComp - 200), kComp + 200, corr);

        const bool aligned = std::abs (lag - kComp) <= 15;
        const bool corrOk  = corr > 0.2;

        check ("Z2 PS1 latency-xcorr", aligned && corrOk,
               juce::String ("peak lag ") + juce::String (lag)
                   + " vs reported " + juce::String (kComp)
                   + " (budget ±15), corr " + juce::String (corr, 3)
                   + (corrOk ? "" : " — DECORRELATED, probe vacuous"));
    }

    //==========================================================================
    // R0 — `reverb` macro min != max liveness (FUNC-02 clause for the newly
    // wired knob), plus boundedness at full send.
    {
        const int total = kRenderSamples;

        auto dry = makeProc();                          // reverb 0 (baseline)
        auto wet = makeProc();
        setParam (*wet, "reverb", 100.0f);

        juce::AudioBuffer<float> outD, outW;
        renderInto (*dry, outD, total, { 512 }, noiseHalfAt);
        renderInto (*wet, outW, total, { 512 }, noiseHalfAt);

        const double diff = maxAbsDiff (outD, outW);
        const bool differs = diff > 1.0e-3;
        const bool bounded = outW.getMagnitude (0, 0, total) < 4.0f;
        const bool live    = outW.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("R0 reverb min!=max", differs && bounded && live,
               juce::String ("maxAbsDiff(0% vs 100%)=") + juce::String (diff, 5)
                   + (differs ? "" : " — REVERB DOES NOT REACH THE DSP")
                   + (bounded ? "" : " — UNBOUNDED")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // R1 — DSP-04 impulse-response tail structure. Wet-minus-dry isolates the
    // reverb return exactly (the direct path is value-identical in both
    // renders; the return is 0.0 until the first reflection). Criteria:
    //   - first reflection lands in the comb-delay window after the ALIGNED
    //     direct arrival (comb1 = 353 ticks ≈ 16 ms ≈ 768 host samples; the
    //     window [300, 1800] also bounds the L119 alignment error)
    //   - early tail live; late tail (1.6-2.0 s) far below it (short murky
    //     decay, RT60 ≈ 0.9 s)
    {
        auto wet = makeProc();
        setParam (*wet, "crush", 0.0f);
        setParam (*wet, "reverb", 100.0f);

        auto dry = makeProc();
        setParam (*dry, "crush", 0.0f);

        const int total = 120000;   // 2.5 s
        juce::AudioBuffer<float> outW, outD;
        renderInto (*wet, outW, total, { 512 }, impulseAt);
        renderInto (*dry, outD, total, { 512 }, impulseAt);

        std::vector<float> diff ((size_t) total);
        for (int n = 0; n < total; ++n)
            diff[(size_t) n] = outW.getSample (0, n) - outD.getSample (0, n);

        const int n0 = 4800 + kComp;   // aligned direct arrival of the impulse

        int firstTail = -1;
        for (int n = n0 + 150; n < total; ++n)
            if (std::abs (diff[(size_t) n]) > 1.0e-5f)
            {
                firstTail = n;
                break;
            }

        const double early = rmsRange (diff, n0 + 2400,  n0 + 21600);   // 50-450 ms
        const double late  = rmsRange (diff, n0 + 76800, n0 + 96000);   // 1.6-2.0 s

        const bool tailLive  = early > 1.0e-4;
        const bool decays    = late < 0.1 * early;
        const bool combDelay = firstTail >= n0 + 300 && firstTail <= n0 + 1800;

        check ("R1 DSP-04 IR-tail", tailLive && decays && combDelay,
               juce::String ("first reflection @+") + juce::String (firstTail - n0)
                   + " (window [300, 1800]), early RMS " + juce::String (early, 6)
                   + ", late RMS " + juce::String (late, 7)
                   + " (< 0.1x early)"
                   + (tailLive ? "" : " — NO TAIL, probe vacuous"));
    }

    //==========================================================================
    // R2 — QUAL-01/DSP-04 stability: 60 s of noise at reverb 100 % +
    // crush 100 % — finite, bounded, and NO growth (the feedback network's
    // stability-by-construction, measured).
    {
        auto p = makeProc();
        setParam (*p, "console", 1.0f);      // PS1: SPU codec + reverb together
        setParam (*p, "crush", 100.0f);
        setParam (*p, "reverb", 100.0f);

        const int total = 60 * (int) kFs;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseHalfAt);

        bool allFinite = true;
        float peak = 0.0f;
        int firstBad = -1;

        for (int ch = 0; ch < 2 && allFinite; ++ch)
        {
            const auto* o = out.getReadPointer (ch);
            for (int n = 0; n < total; ++n)
            {
                if (! std::isfinite (o[n]))
                {
                    allFinite = false;
                    firstBad = n;
                    break;
                }
                peak = juce::jmax (peak, std::abs (o[n]));
            }
        }

        const auto v = channelToVector (out, 0);
        const double rmsEarly = rmsRange (v, 5 * (int) kFs, 10 * (int) kFs);
        const double rmsLate  = rmsRange (v, 55 * (int) kFs, total);

        const bool bounded  = peak < 4.0f;
        const bool noGrowth = rmsLate < 3.0 * rmsEarly;
        const bool live     = rmsEarly > 1.0e-3;

        check ("R2 60s stability", allFinite && bounded && noGrowth && live,
               juce::String ("finite ") + (allFinite ? "yes" : juce::String ("NO @") + juce::String (firstBad))
                   + ", peak " + juce::String (peak, 4)
                   + ", RMS 5-10s " + juce::String (rmsEarly, 5)
                   + " vs 55-60s " + juce::String (rmsLate, 5)
                   + (noGrowth ? "" : " — GROWING")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // AN2 — PERF-02 block-size invariance RE-RUN with the Phase 2.2 paths
    // engaged: PS1 codec + reverb send/tick/return.
    {
        const int total = kRenderSamples;

        auto configure = [] (OEmulatorAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "console", 1.0f);
            setParam (proc, "crush", 65.0f);
            setParam (proc, "reverb", 60.0f);
        };

        auto ref = makeProc();
        configure (*ref);
        juce::AudioBuffer<float> outRef;
        renderInto (*ref, outRef, total, { 512 }, noiseHalfAt);

        const std::vector<std::vector<int>> sizeSets
            { { 64 }, { 4096 }, { 1, 7, 64, 333, 4096 } };
        const char* setNames2[] = { "{64}", "{4096}", "{1,7,64,333,4096}" };

        bool ok = true;
        juce::String detail;

        for (size_t s = 0; s < sizeSets.size(); ++s)
        {
            auto proc = makeProc();
            configure (*proc);

            juce::AudioBuffer<float> out;
            renderInto (*proc, out, total, sizeSets[s], noiseHalfAt);

            if (! bitIdentical (outRef, out))
            {
                ok = false;
                detail << setNames2[s] << ": " << firstDifference (outRef, out) << "; ";
            }
        }

        const bool live = outRef.getMagnitude (0, 0, total) > 1.0e-4f;
        if (ok && live)
            detail = "PS1 + reverb: 3 block-size regimes vs fixed {512}, all bit-identical";

        check ("AN2 PERF-02 reverb-engaged", ok && live,
               detail + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // D — digest anchors (plan decision #1). Canonical render: baseline
    // (SNES, crush 50, mix 100), flat {512}. Excitation is FULL-SCALE noiseAt
    // — the SAME signal the Stage-1 anchor was recorded with, so the
    // moved-assert compares like with like (with a different input even a
    // passthrough would trivially "move" the digest and the probe would be
    // decoration — pattern_probe_must_target_the_branch_the_fix_changed).
    {
        auto p = makeProc();
        std::vector<float> flat;
        renderInterleaved (*p, flat, kRenderSamples, noiseAt);

        const juce::uint64 digest = fnv1a64 (flat);

        std::printf ("  digest: fnv1a64=%016llx (%d samples x 2ch, flat 512)\n",
                     (unsigned long long) digest, kRenderSamples);

        // Retired Stage-1 anchor must have MOVED — a match means the engine
        // is not in the signal path at mix 100.
        check ("D stage-1 anchor moved", digest != kDigestStage1PassthroughRetired,
               digest != kDigestStage1PassthroughRetired
                   ? juce::String ("passthrough digest retired")
                   : juce::String ("digest UNCHANGED from Stage 1 — wet path inert"));

        if (kDigestAnchor21CanonicalSnes != 0)
        {
            check ("D phase-2.1 anchor", digest == kDigestAnchor21CanonicalSnes,
                   juce::String::formatted ("expected %016llx",
                       (unsigned long long) kDigestAnchor21CanonicalSnes));
        }
        else
        {
            std::printf ("  [NOTE] kDigestAnchor21CanonicalSnes unrecorded — RECORD "
                         "%016llx at the Phase 2.1 commit\n",
                         (unsigned long long) digest);
        }
    }

    //==========================================================================
    // D2 — Phase 2.2 canonical digest: PS1 + reverb engaged (the paths 2.2
    // added), full-scale noise, flat {512}. 0-sentinel RECORD pattern.
    {
        auto p = makeProc();
        setParam (*p, "console", 1.0f);
        setParam (*p, "reverb", 60.0f);

        std::vector<float> flat;
        renderInterleaved (*p, flat, kRenderSamples, noiseAt);

        const juce::uint64 digest = fnv1a64 (flat);

        std::printf ("  digest-2.2: fnv1a64=%016llx (PS1 + reverb 60%%, flat 512)\n",
                     (unsigned long long) digest);

        // The PS1+reverb render must not degenerate to the SNES canonical or
        // the Stage-1 passthrough (liveness of the new paths in the digest).
        check ("D2 phase-2.2 render distinct", digest != kDigestAnchor21CanonicalSnes
                   && digest != kDigestStage1PassthroughRetired,
               "PS1+reverb digest differs from the SNES and passthrough anchors");

        if (kDigestAnchor22Ps1ReverbCanonical != 0)
        {
            check ("D2 phase-2.2 anchor", digest == kDigestAnchor22Ps1ReverbCanonical,
                   juce::String::formatted ("expected %016llx",
                       (unsigned long long) kDigestAnchor22Ps1ReverbCanonical));
        }
        else
        {
            std::printf ("  [NOTE] kDigestAnchor22Ps1ReverbCanonical unrecorded — RECORD "
                         "%016llx at the Phase 2.2 commit\n",
                         (unsigned long long) digest);
        }
    }

    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
