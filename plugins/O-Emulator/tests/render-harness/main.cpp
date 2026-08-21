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

    Phase 2.3 probes (NES/GB/Genesis + console switching):

      M1 mode matrix  — all five consoles pairwise distinct on the same
                        material: time-domain maxAbsDiff AND a normalized
                        6-band spectral-profile L1 distance per pair.
      M2 corners      — per-console output-corner evidence: RATIOS against
                        the input render (extreme-top energy halved or
                        better, centroid darkened, mid band live) + the five
                        DAC-LP corners read from the LIVE prepared filters
                        against the ARCHITECTURE table. (An earlier
                        GB<NES centroid-ordering clause was retired: DPCM's
                        slew-limiter character dominates the NES spectrum
                        regardless of its 14 kHz corner — see the in-probe
                        comment.)
      M3 switch       — mid-render SNES -> NES switch via a timeline event on
                        a sine (no processBlock spans the event): reported
                        latency unchanged, no clicks (fade-region max delta
                        bounded by the steady regions' own staircase deltas),
                        equal-power fade-region RMS neither dips nor doubles.
      M4 NES DC       — NES DPCM's unipolar offset fully blocked at the mixer
                        boundary: output mean ~ 0 while live.
      M5 reverb x5    — reverb 0 vs 100 changes the render in ALL five modes.
      AN3 invariance  — block-size digest sweep re-run per new mode
                        (NES/GB/Genesis, crush + reverb engaged).
      D3 digest       — Phase 2.3 switch-matrix anchor (timeline render
                        fading through all five consoles), 0-sentinel RECORD
                        pattern. Anchors 2.1/2.2 SURVIVE structurally — see
                        the constants block for the two survival arguments
                        (idle-crossfader branch; first-chunk instant switch).

    Phase 2.4 probes (age model + crush polish + PERF audit):

      G1 age x5       — age 0 vs 100 changes the render in every mode.
      G2 bed scaling  — silence in: bed RMS strictly rising over age
                        10/40/70/100, absent at 0.
      G3 bed rates    — bed level within ±1.5 dB of 48 kHz at 44.1/96/192 kHz
                        (white-fed-stage normalization, makeProcAtRate).
      G4 drift        — interpolated zero-crossing pitch: < 3 cents wobble at
                        age 0, alive (> 1 cent) but bounded (< 25 cents) at
                        age 100; no resampler instability.
      G5 offline      — bit-identical under setNonRealtime(true).
      G6 macros x5    — all four knobs min != max in all five modes.
      G7 sweeps       — every knob swept 0 -> 100 through timeline steps
                        (crush on SNES/NES/GB crosses every integer step +
                        the AA-open breakpoints): finite, bounded, and the
                        20 ms windowed RMS never collapses (no dropout; the
                        5 ms micro-fade dips stay inside the floor).
      P1 cpu          — render/realtime ratio <= 0.15 (the one sanctioned
                        wall-clock probe).
      D/D2/D3         — RE-ANCHORED: the age bed (default 20 %) rides on
                        every wet render, so all three prior anchors are
                        retired + moved-asserted; new 0-sentinel anchors
                        recorded at the 2.4 commit.

    Conventions (O-Bitrot harness header): setBaseline() first in every
    probe; setValueNotifyingHost only; position-hashed excitation; liveness
    clauses on every potentially-vacuous probe; no wall-clock in verdicts;
    fixed settle lengths.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <algorithm>
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
        on the SNES path, not a re-anchor event.

        Phase 2.3 status: SURVIVES, structurally. The canonical render never
        changes console, so the ConsoleCrossfader's idle branch renders the
        single active pipeline through the exact 2.2 sequence (the fade
        scratch copies and gain math are behind isFading()); the SNES row,
        Gaussian branch, and priming formula are unchanged (prime stays 33 at
        the anchored rates). */
    constexpr juce::uint64 kDigestAnchor21CanonicalSnes = 0x59d72af3f1b80676ULL;

    /** Phase 2.2 canonical PS1+reverb digest (recorded at the 2.2 commit).

        Phase 2.3 status: SURVIVES, structurally. The render selects PS1 on a
        fresh instance, which takes the FIRST-CHUNK INSTANT SWITCH path —
        before any chunk has rendered there is nothing to crossfade, and the
        instant path reproduces 2.2's chunk-0 hard switch bit-exactly (the
        2.2-era reverb.reset() it drops is a structural no-op on the still
        all-zero reverb state). Mid-stream switches always fade; only renders
        that switch consoles AFTER audio has flowed produce new output, and
        those carry their own Phase 2.3 anchor below. */
    constexpr juce::uint64 kDigestAnchor22Ps1ReverbCanonical = 0x58cb7f909f6a6e30ULL;

    /** Phase 2.3 canonical switch-matrix digest (a timeline render fading
        through all five consoles mid-stream). Recorded from the FIRST 2.3
        harness run — valid despite that run's M2 failure because the M2 fix
        was MEASUREMENT-ONLY (a retired centroid-ordering clause replaced by
        a live-corner contract read; zero DSP changes), so the rendered bytes
        are unchanged. */
    constexpr juce::uint64 kDigestAnchor23SwitchMatrix = 0x03ca7037593af84aULL;

    //==========================================================================
    // ── Phase 2.4 re-anchor (Task 19, retire/moved-assert discipline) ────────
    //
    // The age bed (default age = 20 %) now injects noise + hum into EVERY wet
    // render, so ALL THREE prior canonical anchors MOVED, structurally:
    //   - 2.1 SNES canonical:  bed audible at −73-ish dB on the wet path.
    //   - 2.2 PS1 + reverb:    bed + PS1's upsample priming grew a drift-
    //                          headroom floor (prime 4 -> 8 console samples).
    //   - 2.3 switch matrix:   bed + the priming change + age dulling.
    // The retired values are kept below and asserted MOVED
    // (pattern_reanchor_cross_version_digest_probe); the new anchors follow
    // the 0-sentinel RECORD pattern.

    constexpr juce::uint64 kDigestAnchor21CanonicalSnesRetired24  = kDigestAnchor21CanonicalSnes;
    constexpr juce::uint64 kDigestAnchor22Ps1ReverbRetired24      = kDigestAnchor22Ps1ReverbCanonical;
    constexpr juce::uint64 kDigestAnchor23SwitchMatrixRetired24   = kDigestAnchor23SwitchMatrix;

    constexpr juce::uint64 kDigestAnchor24CanonicalSnes = 0x9cf6baa8d3b61b14ULL;   // recorded at the 2.4 commit
    constexpr juce::uint64 kDigestAnchor24Ps1Reverb     = 0xb23fe10b74526fabULL;   // recorded at the 2.4 commit
    constexpr juce::uint64 kDigestAnchor24SwitchMatrix  = 0xdad157a01f7c393fULL;   // recorded at the 2.4 commit

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

    /** Digital silence: under it the AGE BED is the ONLY thing in the wet
        output, so its level can be read directly (O-Bitrot bed model). */
    float silentAt (int ch, int n)
    {
        juce::ignoreUnused (ch, n);
        return 0.0f;
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

    /** Fresh processor at an arbitrary sample rate — the noise bed claims
        rate invariance, and that claim needs other rates to mean anything. */
    std::unique_ptr<OEmulatorAudioProcessor> makeProcAtRate (double fs)
    {
        auto p = std::make_unique<OEmulatorAudioProcessor>();
        p->setPlayConfigDetails (2, 2, fs, kMaxBlock);
        p->prepareToPlay (fs, kMaxBlock);
        setBaseline (*p);
        return p;
    }

    /** Mean frequency over [from, from+len) from LINEARLY INTERPOLATED upward
        zero crossings (O-Bitrot model — resolves ~0.01 %, needed because the
        whole drift budget is ±15 cents ≈ ±0.87 %). 0 if < 2 full cycles. */
    double meanFreqZeroCross (const std::vector<float>& x, int from, int len, double fs)
    {
        double firstT = -1.0, lastT = -1.0;
        int cycles = 0;

        for (int n = from + 1; n < from + len; ++n)
        {
            if (x[(size_t) (n - 1)] <= 0.0f && x[(size_t) n] > 0.0f)
            {
                const double d = (double) x[(size_t) n] - (double) x[(size_t) (n - 1)];
                const double frac = d > 0.0 ? (double) (-x[(size_t) (n - 1)]) / d : 0.0;
                const double t = (double) (n - 1) + frac;

                if (firstT < 0.0)
                    firstT = t;
                else
                {
                    lastT = t;
                    ++cycles;
                }
            }
        }

        if (cycles < 2 || lastT <= firstT)
            return 0.0;

        return fs * (double) cycles / (lastT - firstT);
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

    /** Timeline event: a parameter write applied when the render reaches
        `sample`. renderTimeline clamps its processBlock calls so NO call
        spans an event (O-Tapestop Event/renderTimeline model) — which is
        what makes automation-carrying probes legal at arbitrary offsets. */
    struct TimelineEvent
    {
        int sample;
        const char* id;
        float value;
    };

    void renderTimeline (OEmulatorAudioProcessor& proc, juce::AudioBuffer<float>& dest,
                         int totalSamples, const std::vector<int>& sizes, InputFn input,
                         const std::vector<TimelineEvent>& events)
    {
        juce::MidiBuffer midi;

        dest.setSize (2, totalSamples);
        dest.clear();

        juce::AudioBuffer<float> scratch (2, kMaxBlock);

        int    n  = 0;
        size_t si = 0, ei = 0;

        while (n < totalSamples)
        {
            while (ei < events.size() && events[ei].sample <= n)
            {
                setParam (proc, events[ei].id, events[ei].value);
                ++ei;
            }

            int chunk = sizes[si % sizes.size()];
            ++si;
            chunk = juce::jmin (chunk, totalSamples - n);
            if (ei < events.size())
                chunk = juce::jmin (chunk, events[ei].sample - n);   // never span an event
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
    // trip) and AGE 0 (drift is a bounded time-offset walk — tens of host
    // samples at full depth — and the reported figure is deliberately the
    // NOMINAL one, so the measurement must run drift-free); xcorr peak within
    // ±15 samples of kComp (grid jitter + IIR group delay + priming-estimate
    // error, L120).
    {
        auto p = makeProc();
        setParam (*p, "crush", 0.0f);
        setParam (*p, "age", 0.0f);

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
        setParam (*p, "age", 0.0f);   // drift-free — see probe Z

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

        // PS1's drift-headroom priming floor (2.4) sits ~+9 host samples
        // above its exact alignment — still inside the ±15 budget.
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
    // M1/M2 — FUNC-01 five-mode spectral distinctness matrix + DSP-03
    // per-console output-corner evidence. One render per console on the same
    // material, everything measured as ratios/distances, never absolutes.
    {
        const int total = kRenderSamples;
        const int off   = 12288;
        const char* names[5] = { "SNES", "PS1", "NES", "GB", "GEN" };

        juce::AudioBuffer<float> outs[5];
        Spectrum spectra[5];

        for (int c = 0; c < 5; ++c)
        {
            auto p = makeProc();
            setParam (*p, "console", (float) c);
            renderInto (*p, outs[c], total, { 512 }, noiseHalfAt);
            spectra[c] = analyze (channelToVector (outs[c], 0), off, kFs);
        }

        std::vector<float> in ((size_t) total);
        for (int n = 0; n < total; ++n)
            in[(size_t) n] = noiseHalfAt (0, n);
        const auto sIn = analyze (in, off, kFs);

        // Normalized 6-band energy profile for the pairwise matrix.
        auto profileOf = [] (const Spectrum& s)
        {
            const double edges[7] = { 200.0, 1000.0, 3000.0, 6000.0,
                                      10000.0, 16000.0, 22000.0 };
            std::array<double, 6> p {};
            double tot = 0.0;
            for (int k = 0; k < 6; ++k)
            {
                p[(size_t) k] = s.bandEnergy (edges[k], edges[k + 1]);
                tot += p[(size_t) k];
            }
            for (auto& v : p)
                v /= (tot + 1.0e-12);
            return p;
        };

        std::array<double, 6> profiles[5];
        for (int c = 0; c < 5; ++c)
            profiles[c] = profileOf (spectra[c]);

        // ── M1: pairwise distinctness ───────────────────────────────────────
        bool ok = true;
        juce::String detail;

        for (int a = 0; a < 5; ++a)
        {
            if (outs[a].getMagnitude (0, 0, total) <= 1.0e-4f)
            {
                ok = false;
                detail << names[a] << " SILENT; ";
            }

            for (int b = a + 1; b < 5; ++b)
            {
                const double diff = maxAbsDiff (outs[a], outs[b]);

                double dist = 0.0;
                for (int k = 0; k < 6; ++k)
                    dist += std::abs (profiles[a][(size_t) k] - profiles[b][(size_t) k]);

                if (diff <= 1.0e-3 || dist <= 0.02)
                {
                    ok = false;
                    detail << names[a] << "/" << names[b]
                           << " diff " << juce::String (diff, 5)
                           << " dist " << juce::String (dist, 4) << "; ";
                }
            }
        }

        if (ok)
            detail = "10 pairs distinct (time-domain + 6-band profile)";

        check ("M1 FUNC-01 mode-matrix", ok, detail);

        // ── M2: per-console corners as ratios vs the input ──────────────────
        bool ok2 = true;
        juce::String detail2;

        const double centIn = sIn.centroid (100.0, 21000.0);
        const double topIn  = sIn.bandEnergy (18000.0, 22000.0);

        double cent[5] {};

        for (int c = 0; c < 5; ++c)
        {
            cent[c] = spectra[c].centroid (100.0, 21000.0);

            const double topRatio = spectra[c].bandEnergy (18000.0, 22000.0)
                                  / (topIn + 1.0e-12);
            const double midRatio = spectra[c].bandEnergy (500.0, 3000.0)
                                  / (sIn.bandEnergy (500.0, 3000.0) + 1.0e-12);

            if (! (topRatio < 0.5 && cent[c] < 0.85 * centIn && midRatio > 0.02))
            {
                ok2 = false;
                detail2 << names[c] << " top " << juce::String (topRatio, 3)
                        << " cent " << juce::String (cent[c], 0)
                        << " mid " << juce::String (midRatio, 3) << "; ";
            }
        }

        // ── RETIRED CLAUSE (first 2.3 run, measured): `centroid GB < NES`.
        // The premise ("domain 33.1 kHz + 14 kHz corner => NES brighter")
        // ignored the CODEC: DPCM's ±2-step counter is a slew-rate limiter,
        // and at the baseline crush 50 the timer is walked to index 7
        // (8363.4 Hz), a slew ceiling of 2·8363/64 ≈ 261 FS/s — broadband
        // near-FS noise is therefore ~1/f²-shaped far below the output
        // corner (measured NES centroid 602 Hz; consistency cross-checks:
        // M4 RMS 0.0406 vs the ~0.03-0.05 slew-ceiling prediction, M3
        // post-switch delta 0.0179 ≈ one ±2 step through the 14 kHz TPT).
        // The codec character, not the output stage, sets the NES centroid
        // on this excitation — the clause measured the wrong observable and
        // NO time-domain excitation can see the NES corner through the slew
        // limiter. DSP-03's actual claim (per-console output-stage corners)
        // is asserted below from the LIVE prepared filters instead
        // (pattern_probe_must_target_the_branch_the_fix_changed: the corner
        // claim now has a probe that fails if a corner is mis-set, which the
        // centroid ordering never cleanly did).
        {
            auto probe = makeProc();

            // ARCHITECTURE Pipeline Manager table (the binding contract this
            // checks, same role as P0 for parameter-spec.md).
            const float expectedLpHz[5] = { 10000.0f, 12000.0f, 14000.0f,
                                            8000.0f, 12000.0f };

            for (int c = 0; c < 5; ++c)
            {
                const float actual = probe->getOutputLpHzForTest (c);
                if (! juce::approximatelyEqual (actual, expectedLpHz[c]))
                {
                    ok2 = false;
                    detail2 << names[c] << " LP " << juce::String (actual, 0)
                            << " != " << juce::String (expectedLpHz[c], 0) << "; ";
                }
            }
        }

        if (ok2)
            detail2 = juce::String ("all corners dark vs input (centroids ")
                    + juce::String (cent[0], 0) + "/" + juce::String (cent[1], 0) + "/"
                    + juce::String (cent[2], 0) + "/" + juce::String (cent[3], 0) + "/"
                    + juce::String (cent[4], 0) + " vs in " + juce::String (centIn, 0) + ")";

        check ("M2 DSP-03 output-corners", ok2, detail2);
    }

    //==========================================================================
    // M3 — FUNC-04 mid-render console switch (SNES -> NES) via a timeline
    // event at a non-aligned sample. Criteria: reported latency unchanged
    // (no PDC renegotiation), no clicks (the fade region's max sample delta
    // bounded by the steady regions' own intrinsic staircase deltas), and
    // the equal-power fade neither dips nor doubles the RMS.
    {
        auto p = makeProc();
        const int latBefore = p->getLatencySamples();

        const int total = 96000;
        const int eventAt = 43211;
        juce::AudioBuffer<float> out;
        renderTimeline (*p, out, total, { 512 }, sineAt,
                        { { eventAt, "console", 2.0f } });

        const int latAfter = p->getLatencySamples();
        const auto v = channelToVector (out, 0);

        auto maxDeltaIn = [&v] (int from, int to)
        {
            double m = 0.0;
            for (int n = from + 1; n < to; ++n)
                m = juce::jmax (m, std::abs ((double) v[(size_t) n]
                                             - (double) v[(size_t) (n - 1)]));
            return m;
        };

        const double dBefore = maxDeltaIn (24000, 43000);
        const double dAfter  = maxDeltaIn (60000, 90000);
        const double dSwitch = maxDeltaIn (43150, 46600);

        const double rmsBefore = rmsRange (v, 24000, 43000);
        const double rmsAfter  = rmsRange (v, 60000, 90000);
        const double rmsFade   = rmsRange (v, 43250, 43250 + 1440);

        const bool latencySame = latBefore == latAfter && latBefore == kComp;
        const bool noClick = dSwitch <= 2.5 * juce::jmax (dBefore, dAfter) + 1.0e-4;
        const bool noDip   = rmsFade >= 0.4 * juce::jmin (rmsBefore, rmsAfter)
                          && rmsFade <= 2.5 * juce::jmax (rmsBefore, rmsAfter);
        const bool live    = rmsBefore > 1.0e-3 && rmsAfter > 1.0e-3;

        check ("M3 FUNC-04 switch", latencySame && noClick && noDip && live,
               juce::String ("latency ") + juce::String (latBefore) + "->" + juce::String (latAfter)
                   + ", deltas before/switch/after "
                   + juce::String (dBefore, 4) + "/" + juce::String (dSwitch, 4) + "/"
                   + juce::String (dAfter, 4)
                   + ", RMS before/fade/after "
                   + juce::String (rmsBefore, 4) + "/" + juce::String (rmsFade, 4) + "/"
                   + juce::String (rmsAfter, 4)
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // M4 — the NES DPCM unipolar offset is fully blocked at the mixer
    // boundary (structural 10 Hz DC blocker): live output, mean ~ 0.
    {
        auto p = makeProc();
        setParam (*p, "console", 2.0f);
        setParam (*p, "crush", 70.0f);   // reduced timer rate: maximum DPCM character

        const int total = kRenderSamples;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineAt);

        const auto v = channelToVector (out, 0);

        double mean = 0.0;
        for (int n = 8192; n < total; ++n)
            mean += (double) v[(size_t) n];
        mean /= (double) (total - 8192);

        const double rms = rmsRange (v, 8192, total);

        const bool blocked = std::abs (mean) < 0.005;
        const bool live    = rms > 1.0e-3;

        check ("M4 NES DC-blocked", blocked && live,
               juce::String ("mean ") + juce::String (mean, 6)
                   + " (|.| < 0.005), RMS " + juce::String (rms, 4)
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // M5 — FUNC-03: the reverb send is functional in ALL five modes.
    {
        const int total = kRenderSamples;
        const char* names[5] = { "SNES", "PS1", "NES", "GB", "GEN" };

        bool ok = true;
        juce::String detail;

        for (int c = 0; c < 5; ++c)
        {
            auto dry = makeProc();
            setParam (*dry, "console", (float) c);

            auto wet = makeProc();
            setParam (*wet, "console", (float) c);
            setParam (*wet, "reverb", 100.0f);

            juce::AudioBuffer<float> outD, outW;
            renderInto (*dry, outD, total, { 512 }, noiseHalfAt);
            renderInto (*wet, outW, total, { 512 }, noiseHalfAt);

            const double diff = maxAbsDiff (outD, outW);
            const bool live = outW.getMagnitude (0, 0, total) > 1.0e-4f;

            if (diff <= 1.0e-3 || ! live)
            {
                ok = false;
                detail << names[c] << " diff " << juce::String (diff, 5)
                       << (live ? "" : " SILENT") << "; ";
            }
        }

        if (ok)
            detail = "reverb 0 vs 100 changes the render in all 5 modes";

        check ("M5 FUNC-03 reverb-all-modes", ok, detail);
    }

    //==========================================================================
    // AN3 — PERF-02 block-size invariance re-run per NEW mode (NES/GB/GEN),
    // crush + reverb engaged.
    {
        const int total = kRenderSamples;
        const char* names[3] = { "NES", "GB", "GEN" };

        bool ok = true;
        juce::String detail;

        for (int m = 0; m < 3; ++m)
        {
            const float consoleIdx = (float) (m + 2);

            auto configure = [consoleIdx] (OEmulatorAudioProcessor& proc)
            {
                setBaseline (proc);
                setParam (proc, "console", consoleIdx);
                setParam (proc, "crush", 65.0f);
                setParam (proc, "reverb", 40.0f);
            };

            auto ref = makeProc();
            configure (*ref);
            juce::AudioBuffer<float> outRef;
            renderInto (*ref, outRef, total, { 512 }, noiseHalfAt);

            if (outRef.getMagnitude (0, 0, total) <= 1.0e-4f)
            {
                ok = false;
                detail << names[m] << " SILENT; ";
            }

            const std::vector<std::vector<int>> sizeSets
                { { 64 }, { 1, 7, 64, 333, 4096 } };

            for (const auto& sizes : sizeSets)
            {
                auto proc = makeProc();
                configure (*proc);

                juce::AudioBuffer<float> out;
                renderInto (*proc, out, total, sizes, noiseHalfAt);

                if (! bitIdentical (outRef, out))
                {
                    ok = false;
                    detail << names[m] << " size " << sizes[0] << ": "
                           << firstDifference (outRef, out) << "; ";
                }
            }
        }

        if (ok)
            detail = "NES/GB/GEN x {64},{ragged} vs {512}: all bit-identical";

        check ("AN3 PERF-02 per-mode", ok, detail);
    }

    //==========================================================================
    // G1 — DSP-05/FUNC-02: age min != max in ALL five modes.
    {
        const int total = kRenderSamples / 2;
        const char* names[5] = { "SNES", "PS1", "NES", "GB", "GEN" };

        bool ok = true;
        juce::String detail;

        for (int c = 0; c < 5; ++c)
        {
            auto lo = makeProc();
            setParam (*lo, "console", (float) c);
            setParam (*lo, "age", 0.0f);

            auto hi = makeProc();
            setParam (*hi, "console", (float) c);
            setParam (*hi, "age", 100.0f);

            juce::AudioBuffer<float> outLo, outHi;
            renderInto (*lo, outLo, total, { 512 }, noiseHalfAt);
            renderInto (*hi, outHi, total, { 512 }, noiseHalfAt);

            const double diff = maxAbsDiff (outLo, outHi);
            const bool live = outHi.getMagnitude (0, 0, total) > 1.0e-4f;

            if (diff <= 1.0e-3 || ! live)
            {
                ok = false;
                detail << names[c] << " diff " << juce::String (diff, 5)
                       << (live ? "" : " SILENT") << "; ";
            }
        }

        if (ok)
            detail = "age 0 vs 100 changes the render in all 5 modes";

        check ("G1 age min!=max x5", ok, detail);
    }

    //==========================================================================
    // G2 — DSP-05 continuous scaling: under SILENCE the bed is the ONLY
    // output, so its level is read directly; strictly rising across ages,
    // and effectively absent at age 0.
    {
        const int total = kRenderSamples;
        const float ages[4] = { 10.0f, 40.0f, 70.0f, 100.0f };
        double rms[4] {};

        for (int a = 0; a < 4; ++a)
        {
            auto p = makeProc();
            setParam (*p, "age", ages[a]);

            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, silentAt);
            rms[a] = rmsRange (channelToVector (out, 0), 12000, total);
        }

        auto z = makeProc();
        setParam (*z, "age", 0.0f);
        juce::AudioBuffer<float> outZ;
        renderInto (*z, outZ, total, { 512 }, silentAt);
        const double rmsZero = rmsRange (channelToVector (outZ, 0), 12000, total);

        const bool rising = rms[0] < rms[1] && rms[1] < rms[2] && rms[2] < rms[3];
        const bool offAt0 = rmsZero < 1.0e-6;
        const bool live   = rms[3] > 1.0e-4;

        check ("G2 age-bed scaling", rising && offAt0 && live,
               juce::String ("bed RMS 10/40/70/100% = ")
                   + juce::String (rms[0], 7) + "/" + juce::String (rms[1], 7) + "/"
                   + juce::String (rms[2], 7) + "/" + juce::String (rms[3], 7)
                   + ", age0 " + juce::String (rmsZero, 9)
                   + (rising ? "" : " — NOT MONOTONIC")
                   + (live ? "" : " — BED SILENT, probe vacuous"));
    }

    //==========================================================================
    // G3 — DSP-05 noise-bed level RATE invariance (the white-fed stage is
    // normalized; pattern_noise_bed_level_is_rate_dependent). Silence input,
    // age 60, bed RMS within ±1.5 dB of the 48 kHz figure at 44.1/96/192 kHz.
    {
        auto bedRmsAt = [] (double fs)
        {
            auto p = makeProcAtRate (fs);
            setParam (*p, "age", 60.0f);

            const int total = (int) fs;   // 1 s
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, silentAt);
            return rmsRange (channelToVector (out, 0), total / 2, total);
        };

        const double ref = bedRmsAt (48000.0);
        const double rates[3] = { 44100.0, 96000.0, 192000.0 };

        bool ok = ref > 1.0e-4;   // liveness
        juce::String detail = juce::String ("48k ") + juce::String (ref, 6);

        for (const double fs : rates)
        {
            const double v = bedRmsAt (fs);
            const double dB = 20.0 * std::log10 (juce::jmax (1.0e-12, v / ref));
            detail << ", " << juce::String (fs / 1000.0, 1) << "k "
                   << juce::String (dB, 2) << "dB";
            if (std::abs (dB) > 1.5)
                ok = false;
        }

        check ("G3 bed rate-invariance", ok, detail
                   + (ref > 1.0e-4 ? "" : " — BED SILENT, probe vacuous"));
    }

    //==========================================================================
    // G4 — DSP-05 drift: bounded (±15 cents + measurement slack) and ALIVE at
    // age 100, absent at age 0. Interpolated zero-crossing mean frequency per
    // 0.25 s window (integer-lag methods can't resolve a sub-percent wobble).
    {
        auto renderPitch = [] (float agePct, std::vector<double>& centsDev)
        {
            auto p = makeProc();
            setParam (*p, "crush", 0.0f);
            setParam (*p, "age", agePct);

            const int total = 4 * (int) kFs;
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, sineAt);
            const auto v = channelToVector (out, 0);

            // 0.125 s windows: short enough that a drift episode between two
            // offset-rail flips is not diluted away
            // (pattern_metric_window_vs_modulation_period).
            centsDev.clear();
            for (int off = 24000; off + 6000 <= total; off += 6000)
            {
                const double f = meanFreqZeroCross (v, off, 6000, kFs);
                if (f > 0.0)
                    centsDev.push_back (1200.0 * std::log2 (f / kSigHz));
            }
        };

        std::vector<double> dev0, dev100;
        renderPitch (0.0f, dev0);
        renderPitch (100.0f, dev100);

        double max0 = 0.0, max100 = 0.0;
        for (const double d : dev0)   max0   = juce::jmax (max0,   std::abs (d));
        for (const double d : dev100) max100 = juce::jmax (max100, std::abs (d));

        const bool stable0  = ! dev0.empty() && max0 < 3.0;
        const bool bounded  = ! dev100.empty() && max100 < 25.0;
        const bool alive    = max100 > 1.0;

        check ("G4 drift bounded+live", stable0 && bounded && alive,
               juce::String ("age0 max |dev| ") + juce::String (max0, 2)
                   + "c, age100 " + juce::String (max100, 2)
                   + "c (need 1..25)"
                   + (alive ? "" : " — DRIFT DOES NOT REACH THE RESAMPLER")
                   + (stable0 ? "" : " — WOBBLE AT AGE 0"));
    }

    //==========================================================================
    // G5 — offline == real-time: the engine has no wall-clock/thread
    // dependence, and the host's non-realtime flag must not grow one.
    {
        auto rt = makeProc();
        setParam (*rt, "crush", 65.0f);
        setParam (*rt, "reverb", 40.0f);

        auto off = makeProc();
        off->setNonRealtime (true);
        setParam (*off, "crush", 65.0f);
        setParam (*off, "reverb", 40.0f);

        juce::AudioBuffer<float> outRt, outOff;
        renderInto (*rt, outRt, kRenderSamples, { 512 }, noiseHalfAt);
        renderInto (*off, outOff, kRenderSamples, { 512 }, noiseHalfAt);

        const bool identical = bitIdentical (outRt, outOff);
        const bool live = outRt.getMagnitude (0, 0, kRenderSamples) > 1.0e-4f;

        check ("G5 offline==realtime", identical && live,
               (identical ? juce::String ("bit-identical with setNonRealtime(true)")
                          : firstDifference (outRt, outOff))
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // G6 — FUNC-02: ALL four macro knobs min != max in ALL five modes.
    {
        const int total = kRenderSamples / 2;
        const char* names[5] = { "SNES", "PS1", "NES", "GB", "GEN" };
        const char* knobs[4] = { "crush", "age", "reverb", "mix" };

        bool ok = true;
        juce::String detail;

        for (int c = 0; c < 5; ++c)
        {
            for (int k = 0; k < 4; ++k)
            {
                auto lo = makeProc();
                setParam (*lo, "console", (float) c);
                setParam (*lo, knobs[k], 0.0f);

                auto hi = makeProc();
                setParam (*hi, "console", (float) c);
                setParam (*hi, knobs[k], 100.0f);

                juce::AudioBuffer<float> outLo, outHi;
                renderInto (*lo, outLo, total, { 512 }, noiseHalfAt);
                renderInto (*hi, outHi, total, { 512 }, noiseHalfAt);

                if (maxAbsDiff (outLo, outHi) <= 1.0e-3)
                {
                    ok = false;
                    detail << names[c] << "/" << knobs[k] << " INERT; ";
                }
            }
        }

        if (ok)
            detail = "20 knob/mode pairs all live (min != max)";

        check ("G6 macros x modes", ok, detail);
    }

    //==========================================================================
    // G7 — smoothing audit (L114): sweep every knob 0 -> 100 in 20 timeline
    // steps; the crush sweeps cross EVERY integer step (shift floor, NES rate
    // walk, GB 16->8->4, AA-open) so the 5 ms micro-fades are in the render.
    // Bounds are on the CONTROL TRAJECTORY's audible symptoms, not raw sample
    // deltas (the crush IS a stepper): bounded, finite, and no dropout — the
    // 20 ms windowed RMS never collapses below 5 % of its median (a stuck
    // fade or a hard mute would).
    {
        struct Sweep { const char* knob; float console; };
        const Sweep sweeps[] = {
            { "crush", 0.0f }, { "crush", 2.0f }, { "crush", 3.0f },
            { "age", 0.0f }, { "reverb", 1.0f }, { "mix", 0.0f },
        };

        bool ok = true;
        juce::String detail;

        for (const auto& sw : sweeps)
        {
            auto p = makeProc();
            setParam (*p, "console", sw.console);

            const int total = kRenderSamples;
            std::vector<TimelineEvent> events;
            for (int s = 0; s < 20; ++s)
                events.push_back ({ 1024 + s * 1500, sw.knob, (float) (s + 1) * 5.0f });

            juce::AudioBuffer<float> out;
            renderTimeline (*p, out, total, { 512 }, noiseHalfAt, events);

            const auto v = channelToVector (out, 0);

            bool finiteOk = true;
            float peak = 0.0f;
            for (const float x : v)
            {
                if (! std::isfinite (x)) { finiteOk = false; break; }
                peak = juce::jmax (peak, std::abs (x));
            }

            // Windowed RMS floor vs median (mix sweep excluded from the
            // dropout clause: mix 0 near the start legitimately nulls the
            // early wet/dry blend against the delayed dry — it is not a
            // dropout; its liveness is covered by G6).
            std::vector<double> wins;
            for (int off = 4096; off + 960 <= total; off += 960)
                wins.push_back (rmsRange (v, off, off + 960));

            auto sorted = wins;
            std::sort (sorted.begin(), sorted.end());
            const double median = sorted[sorted.size() / 2];
            double minWin = 1.0e9;
            for (const double wv : wins)
                minWin = juce::jmin (minWin, wv);

            const bool noDropout = std::strcmp (sw.knob, "mix") == 0
                                       ? true
                                       : minWin > 0.05 * median;
            const bool bounded = peak < 4.0f;
            const bool live = median > 1.0e-3;

            if (! (finiteOk && bounded && noDropout && live))
            {
                ok = false;
                detail << sw.knob << "@c" << (int) sw.console
                       << (finiteOk ? "" : " NONFINITE")
                       << (bounded ? "" : " UNBOUNDED")
                       << (noDropout ? "" : juce::String (" DROPOUT min/med ")
                                                + juce::String (minWin / juce::jmax (1.0e-12, median), 4))
                       << (live ? "" : " SILENT") << "; ";
            }
        }

        if (ok)
            detail = "6 sweeps: finite, bounded, no dropout (micro-fades in-band)";

        check ("G7 sweep smoothing-audit", ok, detail);
    }

    //==========================================================================
    // P1 — PERF-01 CPU ratio (the ONE sanctioned wall-clock use, O-Bitrot P1
    // shape): 5 s canonical render must take <= 0.15x realtime.
    {
        auto p = makeProc();
        setParam (*p, "reverb", 40.0f);   // reverb ticking included in the cost

        const int total = 5 * (int) kFs;
        juce::AudioBuffer<float> out;

        const double t0 = juce::Time::getMillisecondCounterHiRes();
        renderInto (*p, out, total, { 512 }, noiseHalfAt);
        const double elapsedSec = (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001;

        const double ratio = elapsedSec / ((double) total / kFs);
        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("P1 PERF-01 cpu-ratio", ratio <= 0.15 && live,
               juce::String ("render/realtime ratio ") + juce::String (ratio, 3)
                   + " (<= 0.15)" + (live ? "" : " — SILENT, probe vacuous"));
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

        // Retired anchors must have MOVED: Stage-1 (wet path in the signal
        // path at all) and the 2.1 value (the Phase 2.4 age bed at default
        // age 20 % now rides on every wet render).
        check ("D retired anchors moved",
               digest != kDigestStage1PassthroughRetired
                   && digest != kDigestAnchor21CanonicalSnesRetired24,
               digest != kDigestAnchor21CanonicalSnesRetired24
                   ? juce::String ("Stage-1 and 2.1 anchors both retired+moved")
                   : juce::String ("digest UNCHANGED from 2.1 — AGE BED NOT IN THE WET PATH"));

        if (kDigestAnchor24CanonicalSnes != 0)
        {
            check ("D phase-2.4 anchor", digest == kDigestAnchor24CanonicalSnes,
                   juce::String::formatted ("expected %016llx",
                       (unsigned long long) kDigestAnchor24CanonicalSnes));
        }
        else
        {
            std::printf ("  [NOTE] kDigestAnchor24CanonicalSnes unrecorded — RECORD "
                         "%016llx at the Phase 2.4 commit\n",
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

        // Moved-asserts: the 2.2 value retired (age bed + PS1's new priming
        // floor), and the render must stay distinct from the SNES canonical.
        check ("D2 retired anchor moved",
               digest != kDigestAnchor22Ps1ReverbRetired24
                   && digest != kDigestStage1PassthroughRetired,
               digest != kDigestAnchor22Ps1ReverbRetired24
                   ? juce::String ("2.2 anchor retired+moved")
                   : juce::String ("digest UNCHANGED from 2.2 — bed/priming changes inert"));

        if (kDigestAnchor24Ps1Reverb != 0)
        {
            check ("D2 phase-2.4 anchor", digest == kDigestAnchor24Ps1Reverb,
                   juce::String::formatted ("expected %016llx",
                       (unsigned long long) kDigestAnchor24Ps1Reverb));
        }
        else
        {
            std::printf ("  [NOTE] kDigestAnchor24Ps1Reverb unrecorded — RECORD "
                         "%016llx at the Phase 2.4 commit\n",
                         (unsigned long long) digest);
        }
    }

    //==========================================================================
    // D3 — Phase 2.3 canonical switch-matrix digest: a timeline render fading
    // through all five consoles MID-STREAM (the crossfader math is inside
    // this digest), crush + reverb engaged, full-scale noise, flat {512}.
    // 0-sentinel RECORD pattern.
    {
        auto p = makeProc();
        setParam (*p, "crush", 65.0f);
        setParam (*p, "reverb", 40.0f);

        juce::AudioBuffer<float> buf;
        renderTimeline (*p, buf, kRenderSamples, { 512 }, noiseAt,
                        { { 6000,  "console", 2.0f },     // SNES -> NES (fade)
                          { 12000, "console", 3.0f },     // -> GB
                          { 18000, "console", 4.0f },     // -> Genesis
                          { 24000, "console", 1.0f } });  // -> PS1

        std::vector<float> flat;
        flat.reserve ((size_t) kRenderSamples * 2);
        for (int n = 0; n < kRenderSamples; ++n)
            for (int ch = 0; ch < 2; ++ch)
                flat.push_back (buf.getSample (ch, n));

        const juce::uint64 digest = fnv1a64 (flat);

        std::printf ("  digest-2.3: fnv1a64=%016llx (5-console switch matrix, flat 512)\n",
                     (unsigned long long) digest);

        // Moved-asserts: the 2.3 value retired (age bed + dulling + priming),
        // liveness, and distinctness from every other anchor.
        const bool live = buf.getMagnitude (0, 0, kRenderSamples) > 1.0e-4f;
        check ("D3 retired anchor moved", live
                   && digest != kDigestAnchor23SwitchMatrixRetired24
                   && digest != kDigestAnchor21CanonicalSnesRetired24
                   && digest != kDigestAnchor22Ps1ReverbRetired24
                   && digest != kDigestStage1PassthroughRetired,
               live ? juce::String (digest != kDigestAnchor23SwitchMatrixRetired24
                                        ? "2.3 anchor retired+moved"
                                        : "digest UNCHANGED from 2.3 — age paths inert")
                    : juce::String ("SILENT, probe vacuous"));

        if (kDigestAnchor24SwitchMatrix != 0)
        {
            check ("D3 phase-2.4 anchor", digest == kDigestAnchor24SwitchMatrix,
                   juce::String::formatted ("expected %016llx",
                       (unsigned long long) kDigestAnchor24SwitchMatrix));
        }
        else
        {
            std::printf ("  [NOTE] kDigestAnchor24SwitchMatrix unrecorded — RECORD "
                         "%016llx at the Phase 2.4 commit\n",
                         (unsigned long long) digest);
        }
    }

    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
