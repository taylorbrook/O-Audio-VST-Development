/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop render harness — the Stage-2 DSP correctness gate.

    Instantiates TapestopProcessor directly and renders offline. No DAW, no
    hardware, no MIDI. Exit 0 iff every probe passes.

    ── Phase 2.1 probes (PLAN Task 7) ──────────────────────────────────────────
      P0  Determinism — same gesture twice on fresh instances, bitwise
          identical (the engine has no RNG and no wall-clock, so this must
          hold EXACTLY).
      NULL Bypass bitwise null — disengaged output memcmp-equal to the input.
      P1a QUAL-01 — 512-vs-4096 bit-identity over a full gesture cycle
          (engage → Stopped hold → release → splice), ENGAGE edges scheduled
          ONLY at multiples of 4096 (edges are block-header detected; identical
          timelines require boundaries common to both partitions). Includes the
          post-gesture return-to-bitwise-dry tail check.
      P1b QUAL-01 ragged variant — block sizes {1,7,64,333,4096} vs {4096} over
          an edge-free steady engaged span (ragged partitions cannot share edge
          positions, so the one edge sits at sample 0, common to both).
      P3  DSP-02 — sine input, autocorrelation pitch trace
          (autocorrPitchHz per O-simpleGrain); at curve 50 % f(t)/f0 tracks
          (1−u)² within tolerance; at 0 %/100 % the trajectories differ from x²
          in the expected directions. Window 1024 ≪ ramp, τ search constrained
          around the expected f (pattern_metric_window_vs_modulation_period;
          octave-latch guard).
      P6  DSP-01 — discontinuity scan over stop times {50 ms, 500 ms, 8 s} ×
          curves {0, 50, 100} %: max per-sample first difference of the output
          bounded by the dry input's own first-difference bound, with a
          liveness gate (pattern_zipper_sweep_probe_needs_liveness_gate). The
          Phase-2.1 SpinUp→Bypassed splice is EXCLUDED (resync replaces it in
          Phase 2.2).
      REV FUNC-01 partial — release mid-spin-down: pitch-trace ratio continuity
          across the reversal edge + no click at the edge.
      SLOW Risk-register spot render at r ≈ 0.02 — finite + live, smoothness
          number printed for eyeballing (upgrade path is 6-point Lagrange; not
          pre-built).

    No wall-clock inside any verdict (pattern_wallclock_inside_a_stability_
    verdict); the excitation is POSITION-deterministic (noiseAt(n) = hash(n),
    never a sequential RNG — pattern_rng_stream_interleave_blocksize).

  ==============================================================================
*/

#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "PluginProcessor.h"

//==============================================================================
namespace
{

int failures = 0;
int probes   = 0;

void check (const char* name, bool ok, const juce::String& detail)
{
    ++probes;

    if (! ok)
        ++failures;

    std::printf ("  [%s] %-36s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
}

/** Bit-exact float comparison via the object representation. No `==`, so no -Wfloat-equal. */
bool bitExact (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof (float)) == 0;
}

constexpr double kFs = 48000.0;

//==============================================================================
/** POSITION-DETERMINISTIC broadband excitation — a hash of the ABSOLUTE sample
    index. The block-size probes depend on this being a function of n and
    NOTHING else (pattern_rng_stream_interleave_blocksize). Modelled on
    O-Octagon's noiseAt(). */
float noiseAt (int n) noexcept
{
    std::uint32_t h = static_cast<std::uint32_t> (n) * 2654435761u + 0x9E3779B9u;
    h ^= h >> 15;  h *= 0x85EBCA6Bu;
    h ^= h >> 13;  h *= 0xC2B2AE35u;
    h ^= h >> 16;

    return 0.5f * (static_cast<float> (h) / 2147483648.0f - 1.0f);
}

/** Position-deterministic sine, amplitude 0.5. */
float sineAt (int n, double f0) noexcept
{
    return (float) (0.5 * std::sin (juce::MathConstants<double>::twoPi * f0 * (double) n / kFs));
}

//==============================================================================
/** Writes a parameter in ENGINEERING UNITS, synchronously.
    setValueNotifyingHost() is fully synchronous — no timer, no message loop.
    Bare setValue() would NOT notify and would leave the cached atomic stale
    (O-Octagon precedent). ENGAGE edges are exactly this call made BETWEEN
    processBlock calls. */
void setParam (TapestopProcessor& proc, const char* id, float engineeringValue)
{
    auto* p = proc.parameters.getParameter (id);
    jassert (p != nullptr);

    if (p != nullptr)
        p->setValueNotifyingHost (p->convertTo0to1 (engineeringValue));
}

/** Resets ALL 14 parameters to the shipped defaults (createParameterLayout()).
    Traps called out by PLAN Task 1: MIX neutral = 100 (not 0), and ENGAGE is
    explicitly forced OFF. Harness state leaking forward masqueraded as DSP
    regressions for four O-ReverseDelay releases — reset at the source. */
void setBaseline (TapestopProcessor& proc)
{
    setParam (proc, "ENGAGE",         0.0f);   // forced off FIRST
    setParam (proc, "MODE",           0.0f);   // Stop
    setParam (proc, "SYNC_MODE",      0.0f);   // Sync (shipped default; 2.1 DSP reads Free only)
    setParam (proc, "STOP_SYNC_DIV",  3.0f);   // 1/2
    setParam (proc, "STOP_FREE_MS", 500.0f);
    setParam (proc, "STOP_CURVE",    50.0f);
    setParam (proc, "START_SYNC_DIV", 2.0f);   // 1/4
    setParam (proc, "START_FREE_MS",250.0f);
    setParam (proc, "START_CURVE",   50.0f);
    setParam (proc, "ENV_SYNC_DIV",   4.0f);   // 1 bar
    setParam (proc, "ENV_FREE_MS", 1000.0f);
    setParam (proc, "TONE_TRACK",    60.0f);
    setParam (proc, "MIX",          100.0f);   // neutral is 100, NOT 0
    setParam (proc, "OUTPUT_GAIN",    0.0f);
}

/** Baseline + prepare + one settle block of silence.

    prepareToPlay resets the transport to Bypassed directly (and, with ENGAGE
    forced off BEFORE the prepare, the restored-engage-is-an-edge rule cannot
    fire), so transport state cannot leak between probes — the settle block
    then runs one Bypassed block so every probe starts from an identical,
    post-header state.

    The settle length is FIXED (independent of the render block size) and it
    must stay that way: the voice's readAbsFrac is an ABSOLUTE double, so two
    renders whose ring offsets differ accumulate `pos += r` at different
    magnitudes and ROUND differently (~1e-8/sample) — a 512-settle vs
    4096-settle pair diverged by 3e-8 mid-ramp and failed P1a bit-identity
    even though every sample of ring CONTENT both renders read was identical.
    Identical absolute offsets make the accumulation bit-identical. */
void prepareAndSettle (TapestopProcessor& proc, int blockSize)
{
    setBaseline (proc);

    proc.setPlayConfigDetails (2, 2, kFs, blockSize);
    proc.prepareToPlay (kFs, blockSize);

    const int settleLen = juce::jmin (512, blockSize);   // fixed for all current probes (blk >= 512)

    juce::AudioBuffer<float> silence (2, settleLen);
    silence.clear();
    juce::MidiBuffer midi;
    proc.processBlock (silence, midi);
}

//==============================================================================
struct StereoOut
{
    std::vector<float> L, R;
};

/** One automation event: write this parameter when the render reaches this
    ABSOLUTE timeline sample. Applied BETWEEN processBlock calls. */
struct Event
{
    int         atSample;
    const char* id;
    float       value;
};

/** Renders `totalSamples`, breaking processBlock calls so that no call spans
    an event and no call exceeds the size the sequence asks for. `sizes` is
    walked cyclically: a single entry is a fixed block size; a ragged sequence
    is P1b's real gate. Events must be sorted by atSample. */
template <typename Fill>
StereoOut renderTimeline (TapestopProcessor& proc, int totalSamples,
                          const std::vector<int>& sizes,
                          const std::vector<Event>& events,
                          Fill&& fill)   // fill(ch, n) -> float
{
    juce::MidiBuffer midi;

    StereoOut out;
    out.L.reserve ((size_t) totalSamples);
    out.R.reserve ((size_t) totalSamples);

    int maxSize = 1;
    for (int s : sizes)
        maxSize = juce::jmax (maxSize, s);

    juce::AudioBuffer<float> block (2, maxSize);

    size_t nextEvent = 0;
    size_t sizeIndex = 0;
    int    n = 0;

    // Events landing at sample 0 apply before the first block.
    while (nextEvent < events.size() && events[nextEvent].atSample <= 0)
    {
        setParam (proc, events[nextEvent].id, events[nextEvent].value);
        ++nextEvent;
    }

    while (n < totalSamples)
    {
        int chunk = sizes[sizeIndex % sizes.size()];
        ++sizeIndex;

        chunk = juce::jmin (chunk, totalSamples - n);

        if (nextEvent < events.size())
            chunk = juce::jmin (chunk, events[nextEvent].atSample - n);

        if (chunk <= 0)
            chunk = 1;

        juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), 2, chunk);

        for (int s = 0; s < chunk; ++s)
        {
            view.setSample (0, s, fill (0, n + s));
            view.setSample (1, s, fill (1, n + s));
        }

        proc.processBlock (view, midi);

        for (int s = 0; s < chunk; ++s)
        {
            out.L.push_back (view.getSample (0, s));
            out.R.push_back (view.getSample (1, s));
        }

        n += chunk;

        while (nextEvent < events.size() && events[nextEvent].atSample <= n)
        {
            setParam (proc, events[nextEvent].id, events[nextEvent].value);
            ++nextEvent;
        }
    }

    return out;
}

//==============================================================================
/** memcmp-grade equality; reports the first differing sample. */
bool identicalVec (const std::vector<float>& a, const std::vector<float>& b, juce::String& diag)
{
    if (a.size() != b.size())
    {
        diag = "size mismatch";
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (! bitExact (a[i], b[i]))
        {
            diag = juce::String ("first diff @") + juce::String ((int) i)
                 + " (" + juce::String (a[i], 9) + " vs " + juce::String (b[i], 9) + ")";
            return false;
        }
    }

    diag = "identical";
    return true;
}

bool allFinite (const std::vector<float>& v) noexcept
{
    for (float s : v)
        if (! std::isfinite (s))
            return false;

    return true;
}

/** max |v[n]| over [lo, hi). */
double maxAbsIn (const std::vector<float>& v, int lo, int hi) noexcept
{
    double m = 0.0;
    hi = juce::jmin (hi, (int) v.size());

    for (int n = juce::jmax (0, lo); n < hi; ++n)
        m = juce::jmax (m, (double) std::abs (v[(size_t) n]));

    return m;
}

/** max |v[n] − v[n−1]| over n in [lo, hi). */
double maxFirstDiff (const std::vector<float>& v, int lo, int hi) noexcept
{
    double m = 0.0;
    hi = juce::jmin (hi, (int) v.size());

    for (int n = juce::jmax (1, lo); n < hi; ++n)
        m = juce::jmax (m, (double) std::abs (v[(size_t) n] - v[(size_t) n - 1]));

    return m;
}

//==============================================================================
/** Fundamental frequency via normalized autocorrelation over [off, off+len),
    searched in [fLo, fHi] Hz (copied from O-simpleGrain's harness). The τ
    search is CONSTRAINED around the expected frequency by the caller to avoid
    the half-period octave latch. Returns 0 if no lag clears `minCorr`. */
double autocorrPitchHz (const std::vector<float>& x, int off, int len,
                        double fs, double fLo, double fHi, double minCorr = 0.3)
{
    if (off < 0 || off + len > (int) x.size())
        return 0.0;

    const int lagMin = juce::jmax (1, (int) (fs / fHi));
    const int lagMax = (int) (fs / fLo);

    double e0 = 0.0;
    for (int n = 0; n < len; ++n) { const double s = x[(size_t) (off + n)]; e0 += s * s; }
    if (e0 < 1.0e-9) return 0.0;

    double bestCorr = 0.0; int bestLag = -1;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        double acc = 0.0, eL = 0.0;
        for (int n = 0; n + lag < len; ++n)
        {
            const double a = x[(size_t) (off + n)];
            const double b = x[(size_t) (off + n + lag)];
            acc += a * b;
            eL  += b * b;
        }
        const double c = acc / (std::sqrt (e0 * eL) + 1.0e-12);
        if (c > bestCorr) { bestCorr = c; bestLag = lag; }
    }
    return (bestLag > 0 && bestCorr >= minCorr) ? fs / (double) bestLag : 0.0;
}

//==============================================================================
// Shared fills.
float noiseFill (int ch, int n) noexcept  { return noiseAt (n + ch * 7919); }
float sine440   (int ch, int n) noexcept  { juce::ignoreUnused (ch); return sineAt (n, 440.0); }

// First-difference bound the DRY 440 Hz sine itself produces at r = 1
// (A·2π·f0/fs); the engine's varispeed read only ever moves SLOWER than that,
// and the 10 ms stopped-fade adds at most A/480 per sample. 1.5× headroom
// covers interpolation overshoot and the fade term.
const double kDrySineDiffBound = 0.5 * juce::MathConstants<double>::twoPi * 440.0 / kFs;
const double kP6Bound          = kDrySineDiffBound * 1.5 + 0.002;

} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("O-Tapestop render harness — Phase 2.1 (core varispeed + stop/start)\n");
    std::printf ("fs = %.0f Hz\n\n", kFs);

    // ── P0: determinism ──────────────────────────────────────────────────────
    // Same gesture twice on FRESH instances → bitwise identical. The release
    // at 20480 lands mid-spin-down (u ≈ 0.68 of the 500 ms default ramp), so
    // the mid-ramp-reversal path is inside the determinism claim too.
    {
        auto run = []
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            return renderTimeline (p, 49152, { 512 },
                                   { { 4096, "ENGAGE", 1.0f }, { 20480, "ENGAGE", 0.0f } },
                                   noiseFill);
        };

        auto a = run();
        auto b = run();

        juce::String dL, dR;
        const bool okL = identicalVec (a.L, b.L, dL);
        const bool okR = identicalVec (a.R, b.R, dR);

        // Liveness: the gesture actually ran (wet != dry inside the ramp).
        double dev = 0.0;
        for (int n = 8000; n < 18000; ++n)
            dev = juce::jmax (dev, (double) std::abs (a.L[(size_t) n] - noiseFill (0, n)));

        check ("P0-determinism", okL && okR && dev > 0.01,
               "L " + dL + "; R " + dR + "; rampDev=" + juce::String (dev, 4));
    }

    // ── Bypass bitwise null ──────────────────────────────────────────────────
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);

        auto y = renderTimeline (p, 48000, { 512 }, {}, noiseFill);

        bool ok = allFinite (y.L) && allFinite (y.R);
        int firstBad = -1;

        for (int n = 0; n < 48000 && ok; ++n)
        {
            if (! bitExact (y.L[(size_t) n], noiseFill (0, n))
             || ! bitExact (y.R[(size_t) n], noiseFill (1, n)))
            {
                ok = false;
                firstBad = n;
            }
        }

        check ("bypass-bitwise-null", ok,
               ok ? "48000 samples memcmp-equal to input"
                  : "first diff @" + juce::String (firstBad));
    }

    // ── P1a: 512-vs-4096 bit-identity, full gesture cycle ────────────────────
    // Edges ONLY at multiples of 4096: engage @8192, release @40960 (release
    // arrives from Stopped — the 500 ms default ramp parks at ~31.4 k). The
    // spin-up splice (~52.96 k) is deterministic, so it is INSIDE the
    // bit-identity claim; the tail check then proves post-gesture output
    // returns to bitwise dry.
    {
        auto run = [] (int blk)
        {
            TapestopProcessor p;
            prepareAndSettle (p, blk);
            return renderTimeline (p, 73728, { blk },
                                   { { 8192, "ENGAGE", 1.0f }, { 40960, "ENGAGE", 0.0f } },
                                   noiseFill);
        };

        auto s = run (512);
        auto l = run (4096);

        juce::String dL, dR;
        const bool okL = identicalVec (s.L, l.L, dL);
        const bool okR = identicalVec (s.R, l.R, dR);

        double dev = 0.0;
        for (int n = 12000; n < 28000; ++n)
            dev = juce::jmax (dev, (double) std::abs (s.L[(size_t) n] - noiseFill (0, n)));

        check ("P1a-blocksize-invariance", okL && okR && dev > 0.01,
               "L " + dL + "; R " + dR + "; rampDev=" + juce::String (dev, 4));

        // Post-gesture tail: splice at 40960 + 12000 = ~52960; from 56064 the
        // transport is Bypassed and the output must be bitwise dry again.
        bool tailOk = true;
        int firstBad = -1;

        for (int n = 56064; n < 73728 && tailOk; ++n)
        {
            if (! bitExact (s.L[(size_t) n], noiseFill (0, n))
             || ! bitExact (s.R[(size_t) n], noiseFill (1, n)))
            {
                tailOk = false;
                firstBad = n;
            }
        }

        check ("P1a-post-gesture-bitwise-dry", tailOk,
               tailOk ? "tail [56064, 73728) memcmp-equal to input"
                      : "first diff @" + juce::String (firstBad));
    }

    // ── P1b: ragged-block invariance over an edge-free steady span ───────────
    // The one edge (engage) sits at sample 0 — applied before the first block
    // in BOTH partitions. The whole 2 s render is inside an 8 s spin-down:
    // steady engaged material, no edges, which is the only timeline a ragged
    // partition can share with a fixed one.
    {
        auto run = [] (const std::vector<int>& sizes)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 4096);
            setParam (p, "STOP_FREE_MS", 8000.0f);
            return renderTimeline (p, 96000, sizes,
                                   { { 0, "ENGAGE", 1.0f } },
                                   noiseFill);
        };

        auto r = run ({ 1, 7, 64, 333, 4096 });
        auto f = run ({ 4096 });

        juce::String dL, dR;
        const bool okL = identicalVec (r.L, f.L, dL);
        const bool okR = identicalVec (r.R, f.R, dR);

        double dev = 0.0;
        for (int n = 48000; n < 90000; ++n)
            dev = juce::jmax (dev, (double) std::abs (f.L[(size_t) n] - noiseFill (0, n)));

        check ("P1b-ragged-blocks", okL && okR && dev > 0.01,
               "L " + dL + "; R " + dR + "; rampDev=" + juce::String (dev, 4));
    }

    // ── P3: ratio trace vs the curve law (DSP-02) ────────────────────────────
    // 4 s stop ramp on a 440 Hz sine, engage @0, MIX = 100 → the engaged
    // output IS the wet path. Expected f(u) = 440·(1−u)^p, p = 2^(2c).
    // Window 1024 (5.3 ms) ≪ ramp; τ constrained to ±~22 % around expected.
    {
        const double rampSamples = 4.0 * kFs;   // 192000

        auto renderCurve = [] (float curve)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "STOP_FREE_MS", 4000.0f);
            setParam (p, "STOP_CURVE",   curve);
            return renderTimeline (p, 96000, { 512 },
                                   { { 0, "ENGAGE", 1.0f } },
                                   sine440);
        };

        const double uPoints[] = { 0.10, 0.25, 0.40 };
        double fAtQuarter[3] = { 0.0, 0.0, 0.0 };   // measured f at u=0.25 per curve
        const float curves[] = { 0.0f, 50.0f, 100.0f };

        for (int ci = 0; ci < 3; ++ci)
        {
            const double pExp = std::exp2 (2.0 * (double) curves[ci] * 0.01);
            auto y = renderCurve (curves[ci]);

            for (double u : uPoints)
            {
                const int    start = (int) (u * rampSamples);
                const double uMid  = u + 512.0 / rampSamples;
                const double fExp  = 440.0 * std::pow (1.0 - uMid, pExp);

                if (fExp < 100.0)
                    continue;   // below the 1024-window's reliable floor

                const double f = autocorrPitchHz (y.L, start, 1024, kFs,
                                                  fExp * 0.78, fExp * 1.28, 0.25);

                const bool ok = f > 0.0 && std::abs (f / fExp - 1.0) <= 0.08;

                if (std::abs (u - 0.25) < 1.0e-9)
                    fAtQuarter[ci] = f;

                check ((juce::String ("P3-curve") + juce::String ((int) curves[ci])
                          + "-u" + juce::String (u, 2)).toRawUTF8(),
                       ok,
                       "f=" + juce::String (f, 2) + " Hz, expected "
                         + juce::String (fExp, 2) + " Hz (tol 8%)");
            }
        }

        // Direction checks at u = 0.25: curve 0 must sit ABOVE the x² law,
        // curve 100 BELOW it (audibly distinct — DSP-02).
        const double fX2 = 440.0 * std::pow (1.0 - (0.25 + 512.0 / rampSamples), 2.0);

        check ("P3-curve0-above-x2",
               fAtQuarter[0] > fX2 * 1.12,
               "f(c=0)=" + juce::String (fAtQuarter[0], 2) + " Hz vs x2="
                 + juce::String (fX2, 2) + " Hz");

        check ("P3-curve100-below-x2",
               fAtQuarter[2] > 0.0 && fAtQuarter[2] < fX2 * 0.88,
               "f(c=100)=" + juce::String (fAtQuarter[2], 2) + " Hz vs x2="
                 + juce::String (fX2, 2) + " Hz");
    }

    // ── P6: discontinuity scan (DSP-01) ──────────────────────────────────────
    // A continuous-position engine adds no step beyond the source material's
    // own. Sine input so the dry bound is closed-form. The known Phase-2.1
    // SpinUp→Bypassed splice (resync replaces it in 2.2) is excluded ±160.
    {
        const double stopTimesMs[] = { 50.0, 500.0, 8000.0 };
        const float  curves[]      = { 0.0f, 50.0f, 100.0f };

        for (double stopMs : stopTimesMs)
        {
            for (float curve : curves)
            {
                TapestopProcessor p;
                prepareAndSettle (p, 512);
                setParam (p, "STOP_FREE_MS", (float) stopMs);
                setParam (p, "STOP_CURVE",   curve);
                setParam (p, "START_CURVE",  curve);

                const int engageAt    = 12000;
                const int stopSamples = (int) std::lround (stopMs * 0.001 * kFs);
                const int releaseAt   = engageAt + stopSamples + 4800;   // release from Stopped
                const int spliceAt    = releaseAt + 12000;               // 250 ms spin-up
                const int total       = spliceAt + 12000;

                auto y = renderTimeline (p, total, { 512 },
                                         { { engageAt,  "ENGAGE", 1.0f },
                                           { releaseAt, "ENGAGE", 0.0f } },
                                         sine440);

                const double d1L = maxFirstDiff (y.L, engageAt + 1, spliceAt - 160);
                const double d2L = maxFirstDiff (y.L, spliceAt + 160, total);
                const double d1R = maxFirstDiff (y.R, engageAt + 1, spliceAt - 160);
                const double d2R = maxFirstDiff (y.R, spliceAt + 160, total);
                const double dMax = juce::jmax (juce::jmax (d1L, d2L), juce::jmax (d1R, d2R));

                // Liveness (pattern_zipper_sweep_probe_needs_liveness_gate):
                // the ramp actually ran, and there is signal to measure.
                const int lo = engageAt + stopSamples / 4;
                const int hi = engageAt + stopSamples / 2;

                double dev = 0.0;
                for (int n = lo; n < hi; ++n)
                    dev = juce::jmax (dev, (double) std::abs (y.L[(size_t) n] - sine440 (0, n)));

                const double mag = maxAbsIn (y.L, lo, hi);

                const bool ok = allFinite (y.L) && allFinite (y.R)
                             && dMax <= kP6Bound && dev > 0.01 && mag > 1.0e-4;

                check ((juce::String ("P6-stop") + juce::String ((int) stopMs)
                          + "ms-curve" + juce::String ((int) curve)).toRawUTF8(),
                       ok,
                       "maxDiff=" + juce::String (dMax, 5) + " (bound "
                         + juce::String (kP6Bound, 5) + "), rampDev="
                         + juce::String (dev, 3) + ", mag=" + juce::String (mag, 3));
            }
        }
    }

    // ── Mid-ramp reversal (FUNC-01 partial) ──────────────────────────────────
    // Release 1000 ms into a 2000 ms spin-down (u = 0.5, r = 0.25 at curve
    // 50 %). The pitch trace must be ratio-CONTINUOUS across the edge (the
    // reversal seeds u0 = r0^(1/p_new)); a broken seed snaps r back toward 1
    // and the post-edge window reads ~4× the pre-edge pitch.
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "STOP_FREE_MS", 2000.0f);

        const int engageAt  = 12000;
        const int releaseAt = engageAt + 48000;          // u = 0.5 exactly
        const int total     = releaseAt + 12000 + 4800;  // spin-up + margin

        auto y = renderTimeline (p, total, { 512 },
                                 { { engageAt,  "ENGAGE", 1.0f },
                                   { releaseAt, "ENGAGE", 0.0f } },
                                 sine440);

        // Expected ~112 Hz just before the edge, ~130 Hz averaged just after
        // (250 ms spin-up from r = 0.25 under p = 2).
        const double fBefore = autocorrPitchHz (y.L, releaseAt - 1056, 1024, kFs,  80.0, 150.0, 0.25);
        const double fAfter  = autocorrPitchHz (y.L, releaseAt + 16,   1024, kFs,  90.0, 190.0, 0.25);

        const double ratio = fBefore > 0.0 ? fAfter / fBefore : 0.0;

        check ("reversal-ratio-continuity",
               fBefore > 0.0 && fAfter > 0.0 && ratio > 0.65 && ratio < 1.60,
               "fBefore=" + juce::String (fBefore, 2) + " Hz, fAfter="
                 + juce::String (fAfter, 2) + " Hz, ratio=" + juce::String (ratio, 3));

        const double edgeDiff = juce::jmax (maxFirstDiff (y.L, releaseAt - 256, releaseAt + 256),
                                            maxFirstDiff (y.R, releaseAt - 256, releaseAt + 256));

        check ("reversal-no-click",
               allFinite (y.L) && allFinite (y.R) && edgeDiff <= kP6Bound,
               "edge maxDiff=" + juce::String (edgeDiff, 5) + " (bound "
                 + juce::String (kP6Bound, 5) + ")");
    }

    // ── Slow-speed quality spot render (risk register) ───────────────────────
    // Linear 8 s stop on a 2 kHz sine; the window [372000, 378000) sits at
    // u ≈ 0.97–0.98 → r ≈ 0.031–0.016. The verdict gates only on finite +
    // live; the smoothness number (max |Δ²|) is printed for eyeballing —
    // the upgrade path, if stair-stepping is audible, is 6-point Lagrange
    // (deliberately not pre-built).
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "STOP_FREE_MS", 8000.0f);
        setParam (p, "STOP_CURVE",   0.0f);   // linear: r = 1 − u

        const int total = 380000;

        auto y = renderTimeline (p, total, { 512 },
                                 { { 0, "ENGAGE", 1.0f } },
                                 [] (int ch, int n) { juce::ignoreUnused (ch); return sineAt (n, 2000.0); });

        const int lo = 372000, hi = 378000;
        const double mag = maxAbsIn (y.L, lo, hi);

        double d2max = 0.0;
        for (int n = lo + 1; n + 1 < hi; ++n)
            d2max = juce::jmax (d2max,
                                (double) std::abs (y.L[(size_t) (n + 1)]
                                                   - 2.0f * y.L[(size_t) n]
                                                   + y.L[(size_t) (n - 1)]));

        check ("slow-speed-spot-r0.02",
               allFinite (y.L) && allFinite (y.R) && mag > 1.0e-4,
               "mag=" + juce::String (mag, 4) + ", max|d2|=" + juce::String (d2max, 6)
                 + " (informational — eyeball for stair-stepping)");
    }

    //==========================================================================
    std::printf ("\n%d probe checks, %d failure(s)\n", probes, failures);
    return failures == 0 ? 0 : 1;
}
