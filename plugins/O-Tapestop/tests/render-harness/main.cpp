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

    ── Phase 2.1 probes ────────────────────────────────────────────────────────
      P0   Determinism — same gesture twice on fresh instances, bitwise.
      NULL Bypass bitwise null — disengaged output memcmp-equal to the input.
      P1a  QUAL-01 — 512-vs-4096 bit-identity over a full gesture cycle
           (engage → Stopped → release → resync), edges ONLY at multiples of
           4096; post-resync return-to-bitwise-dry tail.
      P1b  QUAL-01 ragged variant — {1,7,64,333,4096} vs {4096} over an
           edge-free steady engaged span.
      P3   DSP-02 — autocorrelation pitch trace vs (1−u)^p; 50 % = x² within
           tolerance; 0/100 % differ in the expected directions.
      P6   DSP-01 — discontinuity scan over stop times × curves, THROUGH the
           full resync (no exclusion windows — the crossfade owns every edge).
      REV  FUNC-01 — release mid-spin-down: ratio continuity + no click.
      SLOW Risk-register spot render at r ≈ 0.02.

    ── Phase 2.2 probes (PLAN Task 11) ─────────────────────────────────────────
      P2   DSP-03 — dry reference vs gesture pass on fresh instances, same
           noiseAt input; bitwise memcmp from one full crossfade after Catchup
           ends. Proves the integer-offset live read + per-sample
           write-then-read chain end to end. Negative control: NOT null
           during Catchup.
      RRT  ResyncXfade retrigger — engage DURING the 50 ms skip fade drives the
           new gesture on the rider while the old fade completes as scheduled;
           click-free; final tail bitwise dry.
      TOG  10 Hz toggling stress — ENGAGE edges every 50 ms against 50 ms
           fades (50 ms gestures); click-free (P6 metric), NaN-free, and the
           post-storm tail returns to bitwise dry.
      SYNC Tempo sync — a TestPlayHead feeds BPM; a mid-ramp tempo change does
           NOT retarget the latched ramp; the NEXT gesture tracks the new BPM;
           Free times land within one block of expected.
      P1c  P1 rerun with sync active (playhead attached, Sync divisions).
      AB   Skip-splice A/B — both gain laws over a sustained pad (sine bed +
           low-passed noise); splice-region level bump/dip printed as the
           evidence for the NOTES.md decision.

    ── Phase 2.3 probes (PLAN Task 15) ─────────────────────────────────────────
      P4   Debt bound — worst-case full-reverse envelope (all y = −1) via the
           UI's JSON commit path, ENV_FREE_MS = 8000; per-block debt sampling;
           maxDebt ≤ ringSpan − kInterpGuard with every bound derived from
           COMPILED source constants; output a coherent 880 Hz reverse read.
      P4b  Stopped hold > kCaptureSeconds — debt clamped throughout, silent
           hold, clean resync, tail bitwise dry (specified behavior).
      P5   Pathological input — silence / DC / impulse train / sine, full
           gesture each; finite everywhere; BITWISE dry after resync.
      TONE DSP-05 — centroid falls with |r| at a = 60 with the a = 0 render as
           the negative control (varispeed darkens content by itself); a = 0
           preserves band energy across the sweep. Cutoff-glide zipper is
           covered structurally: every P6 cell runs at the shipped
           TONE_TRACK = 60, and the invariance probes carry the 16-sample
           absolute grid.
      SCR  FUNC-02/DSP-04 — drawn LUT plays ONCE per engage (pitch trace
           follows the two-level envelope, tail bitwise dry); direction flip
           artifact-free (palindrome corner); disengaged MODE switch bitwise
           silent.

    No wall-clock inside any verdict; the excitation is POSITION-deterministic
    (noiseAt(n) = hash(n), never a sequential RNG). The settle pre-roll length
    is FIXED across compared renders (absolute-double accumulation trap — see
    prepareAndSettle).

  ==============================================================================
*/

#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>
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
    NOTHING else (pattern_rng_stream_interleave_blocksize). */
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
/** Minimal host playhead for the tempo-sync probes. BPM only. */
struct TestPlayHead final : juce::AudioPlayHead
{
    double bpm = 120.0;

    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
    {
        juce::AudioPlayHead::PositionInfo info;
        info.setBpm (bpm);
        info.setIsPlaying (true);
        return info;
    }
};

// Timeline events with id "@BPM" write here instead of the APVTS (mid-render
// host-tempo changes for the latch-contract probe).
TestPlayHead* gPlayHead = nullptr;

//==============================================================================
/** Writes a parameter in ENGINEERING UNITS, synchronously.
    setValueNotifyingHost() is fully synchronous; bare setValue() would leave
    the cached atomic stale. ENGAGE edges are exactly this call made BETWEEN
    processBlock calls. */
void setParam (TapestopProcessor& proc, const char* id, float engineeringValue)
{
    auto* p = proc.parameters.getParameter (id);
    jassert (p != nullptr);

    if (p != nullptr)
        p->setValueNotifyingHost (p->convertTo0to1 (engineeringValue));
}

/** Resets ALL 19 parameters to the shipped defaults (createParameterLayout()).
    Traps: MIX neutral = 100 (not 0); ENGAGE explicitly forced OFF; the
    shipped SYNC_MODE default is Sync — probes that assert Free-ms timing must
    set SYNC_MODE = 1 explicitly after this. */
void setBaseline (TapestopProcessor& proc)
{
    setParam (proc, "ENGAGE",         0.0f);   // forced off FIRST
    setParam (proc, "MODE",           0.0f);   // Stop
    setParam (proc, "SYNC_MODE",      0.0f);   // Sync (shipped default)
    setParam (proc, "STOP_SYNC_DIV",  3.0f);   // 1/2
    setParam (proc, "STOP_FREE_MS", 500.0f);
    setParam (proc, "STOP_CURVE",    50.0f);
    setParam (proc, "START_SYNC_DIV", 2.0f);   // 1/4
    setParam (proc, "START_FREE_MS",250.0f);
    setParam (proc, "START_CURVE",   50.0f);
    setParam (proc, "ENV_SYNC_DIV",   4.0f);   // 1 bar
    setParam (proc, "ENV_FREE_MS", 1000.0f);
    setParam (proc, "CHARACTER",      0.0f);   // Wobble (v1.1)
    setParam (proc, "CONT_RATE_SYNC_DIV", 2.0f);   // 1/4
    setParam (proc, "CONT_RATE_HZ",   1.2f);
    setParam (proc, "CONT_DEPTH",    35.0f);
    setParam (proc, "CONT_CHAOS",    20.0f);
    setParam (proc, "TONE_TRACK",    60.0f);
    setParam (proc, "MIX",          100.0f);   // neutral is 100, NOT 0
    setParam (proc, "OUTPUT_GAIN",    0.0f);
}

/** Baseline + prepare + one settle block of silence.

    The settle length is FIXED (independent of the render block size) and must
    stay that way: readAbsFrac is an ABSOLUTE double, so two renders whose
    ring offsets differ accumulate `pos += r` at different magnitudes and
    ROUND differently (~1e-8/sample) — a 512- vs 4096-settle pair diverged by
    3e-8 mid-ramp and failed P1a even though every ring sample both renders
    read was identical. */
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

/** One automation event applied BETWEEN processBlock calls at this ABSOLUTE
    timeline sample. id "@BPM" writes gPlayHead->bpm instead of the APVTS. */
struct Event
{
    int         atSample;
    const char* id;
    float       value;
};

/** Renders `totalSamples`, breaking processBlock calls so that no call spans
    an event and no call exceeds the size the sequence asks for. `sizes` is
    walked cyclically. Events must be sorted by atSample. */
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

    auto applyEvent = [&proc] (const Event& e)
    {
        if (std::strcmp (e.id, "@BPM") == 0)
        {
            jassert (gPlayHead != nullptr);
            if (gPlayHead != nullptr)
                gPlayHead->bpm = (double) e.value;
            return;
        }
        setParam (proc, e.id, e.value);
    };

    size_t nextEvent = 0;
    size_t sizeIndex = 0;
    int    n = 0;

    while (nextEvent < events.size() && events[nextEvent].atSample <= 0)
        applyEvent (events[nextEvent++]);

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
            applyEvent (events[nextEvent++]);
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

/** RMS over [lo, hi). */
double rmsIn (const std::vector<float>& v, int lo, int hi) noexcept
{
    double acc = 0.0;
    lo = juce::jmax (0, lo);
    hi = juce::jmin (hi, (int) v.size());

    if (hi <= lo)
        return 0.0;

    for (int n = lo; n < hi; ++n)
    {
        const double s = v[(size_t) n];
        acc += s * s;
    }

    return std::sqrt (acc / (double) (hi - lo));
}

/** First sample in [from, to) where the output is NOT bitwise the fill, or -1. */
template <typename Fill>
int firstNonDry (const StereoOut& y, Fill&& fill, int from, int to) noexcept
{
    to = juce::jmin (to, (int) y.L.size());

    for (int n = juce::jmax (0, from); n < to; ++n)
        if (! bitExact (y.L[(size_t) n], fill (0, n)) || ! bitExact (y.R[(size_t) n], fill (1, n)))
            return n;

    return -1;
}

//==============================================================================
/** Fundamental frequency via normalized autocorrelation over [off, off+len),
    searched in [fLo, fHi] Hz (O-simpleGrain lineage). The τ search is
    CONSTRAINED around the expected frequency to avoid the octave latch.
    Returns 0 if no lag clears `minCorr`. */
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

// Pathological fills for P5.
float fillSilence (int ch, int n) noexcept { juce::ignoreUnused (ch, n); return 0.0f; }
float fillDC      (int ch, int n) noexcept { juce::ignoreUnused (ch, n); return 0.9f; }
float fillImpulse (int ch, int n) noexcept { juce::ignoreUnused (ch); return (n % 4800 == 0) ? 1.0f : 0.0f; }

/** HF proxy: RMS of the first difference over RMS — falls as an LPF closes. */
double hfRatio (const std::vector<float>& v, int lo, int hi) noexcept
{
    double d = 0.0, e = 0.0;
    int count = 0;
    hi = juce::jmin (hi, (int) v.size());

    for (int n = juce::jmax (1, lo); n < hi; ++n)
    {
        const double s  = v[(size_t) n];
        const double df = s - v[(size_t) n - 1];
        d += df * df;
        e += s * s;
        ++count;
    }

    if (count == 0 || e <= 0.0)
        return 0.0;

    return std::sqrt (d / e);
}

// First-difference bound: the dry 440 Hz sine produces A·2π·f0/fs at r = 1.
// The worst engaged case is the resync crossfade blending the fading voice at
// catchup speed 1.25× against the live rider at 1× under the equal-power law:
// max over φ of (fo·1.25·d + fi·1.0·d) = d·√(1.25² + 1²) ≈ 1.6·d. 1.85×
// covers that plus the fade-slope and stopped-fade terms.
const double kDrySineDiffBound = 0.5 * juce::MathConstants<double>::twoPi * 440.0 / kFs;
const double kP6Bound          = kDrySineDiffBound * 1.85 + 0.002;

// v1.2 Glitch bound: slam holds the ±2 engine rail and a stutter pitch-ramp
// can splice two voices both near |r| = 2 (equal-power sum → √(2²+2²) ≈ 2.83·d).
// 3.0× covers that plus fade-slope margin; a genuine click is first-diff
// ≈ 2A ≈ 1.0, still ~10× above this bound, so detection power is intact.
const double kP6BoundGlitch    = kDrySineDiffBound * 3.0 + 0.004;

// Shared resync timing constants (48 kHz): 250 ms catchup cap, 50 ms fade.
constexpr int kCatchupLen = 12001;   // cap ticks (elapsed > cap → enter resync)
constexpr int kFadeLen    = 2400;

} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("O-Tapestop render harness — Stage 2 complete (2.1 core + 2.2 resync/sync + 2.3 scratch/toneTrack)\n");
    std::printf ("fs = %.0f Hz\n\n", kFs);

    // ── P0: determinism ──────────────────────────────────────────────────────
    // Release at 20480 lands mid-spin-down (mid-ramp reversal inside the
    // claim); the render runs through catchup + resync fade.
    {
        auto run = []
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE", 1.0f);   // Free timing
            return renderTimeline (p, 49152, { 512 },
                                   { { 4096, "ENGAGE", 1.0f }, { 20480, "ENGAGE", 0.0f } },
                                   noiseFill);
        };

        auto a = run();
        auto b = run();

        juce::String dL, dR;
        const bool okL = identicalVec (a.L, b.L, dL);
        const bool okR = identicalVec (a.R, b.R, dR);

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

        const int bad = firstNonDry (y, noiseFill, 0, 48000);

        check ("bypass-bitwise-null", allFinite (y.L) && allFinite (y.R) && bad < 0,
               bad < 0 ? "48000 samples memcmp-equal to input"
                       : "first diff @" + juce::String (bad));
    }

    // ── P1a: 512-vs-4096 bit-identity, full gesture + resync ─────────────────
    // Edges ONLY at multiples of 4096: engage @8192, release @40960 (from
    // Stopped). SpinUp ends 52960 → catchup ~64961 → fade ends ~67361 →
    // Bypassed. Tail from 69632 must be bitwise dry (DSP-03 by construction).
    {
        auto run = [] (int blk)
        {
            TapestopProcessor p;
            prepareAndSettle (p, blk);
            setParam (p, "SYNC_MODE", 1.0f);
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

        const int bad = firstNonDry (s, noiseFill, 69632, 73728);

        check ("P1a-post-resync-bitwise-dry", bad < 0,
               bad < 0 ? "tail [69632, 73728) memcmp-equal to input"
                       : "first diff @" + juce::String (bad));
    }

    // ── P1b: ragged-block invariance over an edge-free steady span ───────────
    {
        auto run = [] (const std::vector<int>& sizes)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 4096);
            setParam (p, "SYNC_MODE", 1.0f);
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
    {
        const double rampSamples = 4.0 * kFs;   // 192000

        auto renderCurve = [] (float curve)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE", 1.0f);
            setParam (p, "STOP_FREE_MS", 4000.0f);
            setParam (p, "STOP_CURVE",   curve);
            return renderTimeline (p, 96000, { 512 },
                                   { { 0, "ENGAGE", 1.0f } },
                                   sine440);
        };

        const double uPoints[] = { 0.10, 0.25, 0.40 };
        double fAtQuarter[3] = { 0.0, 0.0, 0.0 };
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
                    continue;

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

    // ── P6: discontinuity scan (DSP-01) — THROUGH the full resync ────────────
    // No exclusion windows: the crossfade-skip owns every edge now. The scan
    // covers engage → spin-down → Stopped → release → spin-up → catchup →
    // resync fade → Bypassed.
    {
        const double stopTimesMs[] = { 50.0, 500.0, 8000.0 };
        const float  curves[]      = { 0.0f, 50.0f, 100.0f };

        for (double stopMs : stopTimesMs)
        {
            for (float curve : curves)
            {
                TapestopProcessor p;
                prepareAndSettle (p, 512);
                setParam (p, "SYNC_MODE", 1.0f);
                setParam (p, "STOP_FREE_MS", (float) stopMs);
                setParam (p, "STOP_CURVE",   curve);
                setParam (p, "START_CURVE",  curve);

                const int engageAt    = 12000;
                const int stopSamples = (int) std::lround (stopMs * 0.001 * kFs);
                const int releaseAt   = engageAt + stopSamples + 4800;   // release from Stopped
                const int startLen    = 12000;                           // 250 ms spin-up
                const int total       = releaseAt + startLen + kCatchupLen + kFadeLen + 7200;

                auto y = renderTimeline (p, total, { 512 },
                                         { { engageAt,  "ENGAGE", 1.0f },
                                           { releaseAt, "ENGAGE", 0.0f } },
                                         sine440);

                const double dMax = juce::jmax (maxFirstDiff (y.L, engageAt + 1, total),
                                                maxFirstDiff (y.R, engageAt + 1, total));

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

    // ── Mid-ramp reversal (FUNC-01) ──────────────────────────────────────────
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "STOP_FREE_MS", 2000.0f);

        const int engageAt  = 12000;
        const int releaseAt = engageAt + 48000;          // u = 0.5 exactly
        const int total     = releaseAt + 12000 + 4800;  // ends mid-catchup

        auto y = renderTimeline (p, total, { 512 },
                                 { { engageAt,  "ENGAGE", 1.0f },
                                   { releaseAt, "ENGAGE", 0.0f } },
                                 sine440);

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
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
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

    // ── P2: post-resync null vs dry (DSP-03) ─────────────────────────────────
    // Dry reference and gesture pass on FRESH instances, same noiseAt input.
    // Timeline: engage @8192 → Stopped → release @40960 → spin-up ends 52960
    // → catchup ends ~64961 → fade ends ~67361 → Bypassed. The null window
    // starts one full crossfade after Catchup ends (69632). Negative control:
    // NOT null during catchup.
    {
        auto runGesture = []
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE", 1.0f);
            return renderTimeline (p, 81920, { 512 },
                                   { { 8192, "ENGAGE", 1.0f }, { 40960, "ENGAGE", 0.0f } },
                                   noiseFill);
        };

        auto runDry = []
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            return renderTimeline (p, 81920, { 512 }, {}, noiseFill);
        };

        auto yg = runGesture();
        auto yd = runDry();

        juce::String diag = "identical";
        bool nullOk = true;

        for (int n = 69632; n < 81920 && nullOk; ++n)
        {
            if (! bitExact (yg.L[(size_t) n], yd.L[(size_t) n])
             || ! bitExact (yg.R[(size_t) n], yd.R[(size_t) n]))
            {
                nullOk = false;
                diag = "first diff @" + juce::String (n);
            }
        }

        // Negative control: during catchup (58k–62k) the wet playhead lags —
        // output must NOT equal dry (a vacuous null would pass otherwise).
        double dev = 0.0;
        for (int n = 58000; n < 62000; ++n)
            dev = juce::jmax (dev, (double) std::abs (yg.L[(size_t) n] - yd.L[(size_t) n]));

        check ("P2-post-resync-null", nullOk && dev > 0.01,
               "null window [69632, 81920) " + diag
                 + "; catchup dev=" + juce::String (dev, 4));
    }

    // ── RRT: retrigger DURING the resync crossfade ───────────────────────────
    // 50 ms gestures. First cycle: engage @4096 → release @12288 → spin-up
    // ends 14687 → catchup ends ~26688 → fade [26688, 29088). Engage @28160
    // lands INSIDE the fade (φ ≈ 0.61): the new gesture drives the rider
    // while the old fade completes as scheduled. Second cycle releases @34816
    // and resyncs clean → tail bitwise dry.
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "STOP_FREE_MS", 50.0f);
        setParam (p, "START_FREE_MS", 50.0f);

        const int total = 61440;

        auto y = renderTimeline (p, total, { 512 },
                                 { { 4096,  "ENGAGE", 1.0f },
                                   { 12288, "ENGAGE", 0.0f },
                                   { 28160, "ENGAGE", 1.0f },   // mid-fade retrigger
                                   { 34816, "ENGAGE", 0.0f } },
                                 sine440);

        const double dMax = juce::jmax (maxFirstDiff (y.L, 4097, total),
                                        maxFirstDiff (y.R, 4097, total));

        // Liveness: the retriggered gesture ran (wet != dry after 28160).
        double dev = 0.0;
        for (int n = 29000; n < 31000; ++n)
            dev = juce::jmax (dev, (double) std::abs (y.L[(size_t) n] - sine440 (0, n)));

        const int bad = firstNonDry (y, sine440, 57344, total);

        check ("RRT-resync-retrigger",
               allFinite (y.L) && allFinite (y.R)
                 && dMax <= kP6Bound && dev > 0.01 && bad < 0,
               "maxDiff=" + juce::String (dMax, 5) + " (bound " + juce::String (kP6Bound, 5)
                 + "), retriggerDev=" + juce::String (dev, 3)
                 + (bad < 0 ? juce::String ("; tail dry")
                            : juce::String ("; tail diff @") + juce::String (bad)));
    }

    // ── TOG: 10 Hz toggling stress ───────────────────────────────────────────
    // ENGAGE edges every 50 ms (2400 samples) for 2 s against 50 ms gestures
    // and 50 ms fades — exercises reversal, catchup-retrigger and resync
    // paths back to back. Click-free (P6 metric), NaN-free, and the tail
    // returns to bitwise dry.
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "STOP_FREE_MS", 50.0f);
        setParam (p, "START_FREE_MS", 50.0f);

        std::vector<Event> events;
        for (int k = 0; k < 40; ++k)
            events.push_back ({ 2400 * (k + 1), "ENGAGE", (k % 2 == 0) ? 1.0f : 0.0f });

        const int total = 150000;

        auto y = renderTimeline (p, total, { 512 }, events, sine440);

        const double dMax = juce::jmax (maxFirstDiff (y.L, 2401, total),
                                        maxFirstDiff (y.R, 2401, total));

        double dev = 0.0;
        for (int n = 4800; n < 96000; ++n)
            dev = juce::jmax (dev, (double) std::abs (y.L[(size_t) n] - sine440 (0, n)));

        const int bad = firstNonDry (y, sine440, 140000, total);

        check ("TOG-10Hz-toggling",
               allFinite (y.L) && allFinite (y.R)
                 && dMax <= kP6Bound && dev > 0.05 && bad < 0,
               "maxDiff=" + juce::String (dMax, 5) + " (bound " + juce::String (kP6Bound, 5)
                 + "), dev=" + juce::String (dev, 3)
                 + (bad < 0 ? juce::String ("; tail dry")
                            : juce::String ("; tail diff @") + juce::String (bad)));
    }

    // ── SYNC: tempo sync — latch contract + division tracking ────────────────
    // STOP_SYNC_DIV = 1/4 (1 beat). At 120 BPM that is 24000 samples; the
    // Stopped-silence onset (curve 50 %: entry at u > 1−√0.001 = 0.9684, then
    // the 480-sample fade) measures the latched duration. A @BPM change to
    // 240 mid-ramp must NOT retarget (onset stays ~27818); the NEXT gesture
    // must track 240 BPM (12000 samples → onset ~81733).
    {
        TestPlayHead ph;                    // declared before proc — outlives it
        ph.bpm = 120.0;

        TapestopProcessor p;
        prepareAndSettle (p, 512);
        p.setPlayHead (&ph);
        gPlayHead = &ph;

        setParam (p, "STOP_SYNC_DIV", 2.0f);   // 1/4 = 1 beat
        // SYNC_MODE stays at the shipped default (Sync); START_SYNC_DIV = 1/4.

        const int total = 90112;

        auto y = renderTimeline (p, total, { 512 },
                                 { { 4096,  "ENGAGE", 1.0f },
                                   { 8192,  "@BPM",   240.0f },   // mid-ramp tempo change
                                   { 36864, "ENGAGE", 0.0f },
                                   { 69632, "ENGAGE", 1.0f } },
                                 sine440);

        gPlayHead = nullptr;
        p.setPlayHead (nullptr);

        auto silenceOnset = [&y] (int from, int to) -> int
        {
            for (int n = from; n + 960 <= to; n += 64)
                if (maxAbsIn (y.L, n, n + 960) < 1.0e-6)
                    return n;
            return -1;
        };

        const double entryFrac = 1.0 - std::sqrt (0.001);   // curve 50 % Stopped entry

        const int exp1 = 4096  + (int) std::ceil (entryFrac * 24000.0);   // ~27338 (+480 fade)
        const int exp2 = 69632 + (int) std::ceil (entryFrac * 12000.0);   // ~81253 (+480 fade)

        const int onset1 = silenceOnset (4096,  36864);
        const int onset2 = silenceOnset (69632, total);

        check ("SYNC-latch-no-retarget",
               onset1 >= exp1 - 64 && onset1 <= exp1 + 1500,
               "onset=" + juce::String (onset1) + ", expected ~" + juce::String (exp1 + 480)
                 + " (a retargeted ramp would stop ~11 k samples early)");

        check ("SYNC-next-gesture-tracks-bpm",
               onset2 >= exp2 - 64 && onset2 <= exp2 + 1500,
               "onset=" + juce::String (onset2) + ", expected ~" + juce::String (exp2 + 480));
    }

    // ── SYNC-free: Free times within one block of expected ───────────────────
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);   // Free; STOP_FREE_MS default 500 → 24000 samples

        const int total = 36864;

        auto y = renderTimeline (p, total, { 512 },
                                 { { 4096, "ENGAGE", 1.0f } },
                                 sine440);

        const double entryFrac = 1.0 - std::sqrt (0.001);
        const int    expOnset  = 4096 + (int) std::ceil (entryFrac * 24000.0);

        int onset = -1;
        for (int n = 4096; n + 960 <= total; n += 64)
            if (maxAbsIn (y.L, n, n + 960) < 1.0e-6) { onset = n; break; }

        check ("SYNC-free-time-within-one-block",
               onset >= expOnset - 64 && onset <= expOnset + 1500,
               "onset=" + juce::String (onset) + ", expected ~" + juce::String (expOnset + 480));
    }

    // ── P1c: 512-vs-4096 bit-identity with SYNC ACTIVE ───────────────────────
    // Playhead attached at 100 BPM (1 beat = 28800 samples), Sync divisions,
    // edges at multiples of 4096, full gesture + resync.
    {
        TestPlayHead ph;
        ph.bpm = 100.0;

        auto run = [&ph] (int blk)
        {
            TapestopProcessor p;
            prepareAndSettle (p, blk);
            p.setPlayHead (&ph);
            setParam (p, "STOP_SYNC_DIV", 2.0f);   // 1/4 = 1 beat = 28800 @ 100 BPM
            auto y = renderTimeline (p, 90112, { blk },
                                     { { 8192, "ENGAGE", 1.0f }, { 45056, "ENGAGE", 0.0f } },
                                     noiseFill);
            p.setPlayHead (nullptr);
            return y;
        };

        auto s = run (512);
        auto l = run (4096);

        juce::String dL, dR;
        const bool okL = identicalVec (s.L, l.L, dL);
        const bool okR = identicalVec (s.R, l.R, dR);

        double dev = 0.0;
        for (int n = 16000; n < 36000; ++n)
            dev = juce::jmax (dev, (double) std::abs (s.L[(size_t) n] - noiseFill (0, n)));

        check ("P1c-blocksize-invariance-sync", okL && okR && dev > 0.01,
               "L " + dL + "; R " + dR + "; rampDev=" + juce::String (dev, 4));
    }

    // ── AB: skip-splice gain-law A/B (CONTEXT open question) ─────────────────
    // Sustained pad (220 Hz sine bed + low-passed noise, precomputed and
    // identical for both renders). The fade blends material ~0.3 s apart —
    // highly correlated for the sine bed — so equal-power can over-sum (bump)
    // or the phase relation can dip either law. The bump/dip numbers ARE the
    // evidence for the NOTES.md decision; the check itself gates only on
    // sanity (finite, |bump| < 4 dB).
    {
        const int total = 45056;

        std::vector<float> pad ((size_t) total);
        {
            float lp = 0.0f;
            for (int n = 0; n < total; ++n)
            {
                lp += 0.02f * (noiseAt (n) - lp);
                pad[(size_t) n] = (float) (0.35 * std::sin (juce::MathConstants<double>::twoPi
                                                            * 220.0 * (double) n / kFs))
                                + 3.0f * lp;
            }
        }

        auto padFill = [&pad] (int ch, int n) { juce::ignoreUnused (ch); return pad[(size_t) n]; };

        auto run = [&padFill, total] (bool linearLaw)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE", 1.0f);
            setParam (p, "STOP_FREE_MS", 100.0f);
            p.setSpliceLawForTest (linearLaw);
            return renderTimeline (p, total, { 512 },
                                   { { 4096, "ENGAGE", 1.0f }, { 12288, "ENGAGE", 0.0f } },
                                   padFill);
        };

        auto eq = run (false);
        auto ln = run (true);

        // Fade window: release @12288 → spin-up ends 24287 → catchup ends
        // ~36288 → fade [~36288, ~38688).
        const int fadeLo = 36288 - 480;
        const int fadeHi = 38688 + 480;

        auto spliceStats = [fadeLo, fadeHi] (const std::vector<float>& v)
        {
            const double ref = rmsIn (v, fadeHi + 2400, fadeHi + 6000);

            double maxShort = 0.0, minShort = 1.0e9;
            for (int n = fadeLo; n + 480 <= fadeHi; n += 120)
            {
                const double r = rmsIn (v, n, n + 480);
                maxShort = juce::jmax (maxShort, r);
                minShort = juce::jmin (minShort, r);
            }

            const double bump = 20.0 * std::log10 (juce::jmax (1.0e-9, maxShort / juce::jmax (1.0e-12, ref)));
            const double dip  = 20.0 * std::log10 (juce::jmax (1.0e-9, minShort / juce::jmax (1.0e-12, ref)));
            return std::pair<double, double> (bump, dip);
        };

        const auto eqStats = spliceStats (eq.L);
        const auto lnStats = spliceStats (ln.L);

        check ("AB-splice-equal-power",
               allFinite (eq.L) && allFinite (eq.R) && std::abs (eqStats.first) < 4.0,
               "bump=" + juce::String (eqStats.first, 2) + " dB, dip="
                 + juce::String (eqStats.second, 2) + " dB (evidence → NOTES.md)");

        check ("AB-splice-linear",
               allFinite (ln.L) && allFinite (ln.R) && std::abs (lnStats.first) < 4.0,
               "bump=" + juce::String (lnStats.first, 2) + " dB, dip="
                 + juce::String (lnStats.second, 2) + " dB (evidence → NOTES.md)");
    }

    // ── P4: worst-case full-reverse debt bound (DSP-04) ──────────────────────
    // All-y=−1 envelope committed via the SAME JSON path the UI will use,
    // ENV_FREE_MS = 8000 → r = −2 for 8 s → ~1.15 M samples of debt against
    // the ring's 26 s span. Debt sampled per block; the bound derives from
    // the COMPILED source constants (getRingSpanForTest / kInterpGuard /
    // kCaptureSeconds — never a harness literal). Coherence: full-reverse 2×
    // of a 440 Hz sine reads as a clean 880 Hz tone, not garbage.
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "MODE", 1.0f);              // Scratch
        setParam (p, "ENV_FREE_MS", 8000.0f);

        const bool committed = p.commitScratchEnvelopeJson (
            R"({"v":1,"points":[{"x":0,"y":-1,"curve":0},{"x":1,"y":-1,"curve":0}]})");

        // The pass reads back 2×8 s = 16 s of content, so the ring needs
        // ≥ 16 s of REAL pre-roll before the engage — a fresh-load engage
        // would run the reverse into pre-history zeros (safe by design, but
        // silence would make the coherence window vacuous).
        const int engageAt = 816000;   // 17 s pre-roll
        const int passLen  = 384000;
        const int total    = engageAt + passLen + kFadeLen + 7200;

        const double ringSpan = (double) p.getRingSpanForTest();
        const double bound    = ringSpan - (double) VarispeedVoice::kInterpGuard;

        // Ring span vs the source derivation constant (compiled, not parsed).
        const bool spanOk = ringSpan >= kFs * TapestopProcessor::kCaptureSeconds;

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> block (2, 512);
        StereoOut y;
        y.L.reserve ((size_t) total);
        y.R.reserve ((size_t) total);

        double maxDebt = 0.0;
        bool   engaged = false;
        int    n = 0;

        while (n < total)
        {
            if (! engaged && n >= engageAt)
            {
                setParam (p, "ENGAGE", 1.0f);
                engaged = true;
            }

            int chunk = juce::jmin (512, total - n);
            if (! engaged)
                chunk = juce::jmin (chunk, engageAt - n);
            if (chunk <= 0)
                chunk = 1;

            juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), 2, chunk);
            for (int s = 0; s < chunk; ++s)
            {
                view.setSample (0, s, sine440 (0, n + s));
                view.setSample (1, s, sine440 (1, n + s));
            }

            p.processBlock (view, midi);

            for (int s = 0; s < chunk; ++s)
            {
                y.L.push_back (view.getSample (0, s));
                y.R.push_back (view.getSample (1, s));
            }

            n += chunk;
            maxDebt = juce::jmax (maxDebt, p.getDebtSamplesForTest());
        }

        // Coherent 880 Hz mid-pass (reverse 2× of 440).
        const double fRev = autocorrPitchHz (y.L, engageAt + 200000, 2048, kFs, 700.0, 1050.0, 0.25);

        const int tailBad = firstNonDry (y, sine440, engageAt + passLen + kFadeLen + 4000, total);

        check ("P4-full-reverse-debt-bound",
               committed && spanOk
                 && allFinite (y.L) && allFinite (y.R)
                 && maxDebt <= bound && maxDebt >= 1.0e6
                 && fRev > 0.0 && std::abs (fRev / 880.0 - 1.0) <= 0.05
                 && tailBad < 0,
               "maxDebt=" + juce::String (maxDebt, 0) + " (bound " + juce::String (bound, 0)
                 + "), fRev=" + juce::String (fRev, 1) + " Hz (exp 880)"
                 + (tailBad < 0 ? juce::String ("; tail dry")
                                : juce::String ("; tail diff @") + juce::String (tailBad)));
    }

    // ── P4b: Stopped hold > kCaptureSeconds (CONTEXT decision 1) ─────────────
    // Engage → Stopped → hold 27 s (ring laps the frozen playhead) → release.
    // The stored-position clamp keeps the debt accessor bounded THROUGHOUT;
    // resume plays the oldest valid material and resyncs clean — specified
    // behavior, no assert, tail bitwise dry.
    {
        TapestopProcessor p;
        prepareAndSettle (p, 4096);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "STOP_FREE_MS", 50.0f);

        const int engageAt  = 4096;
        const int releaseAt = engageAt + 2400 + 1296000;   // 27 s hold in Stopped
        const int total     = releaseAt + 12000 + kCatchupLen + kFadeLen + 7200;

        const double bound = (double) p.getRingSpanForTest()
                           - (double) VarispeedVoice::kInterpGuard;

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> block (2, 4096);
        StereoOut y;
        y.L.reserve ((size_t) total);
        y.R.reserve ((size_t) total);

        double maxDebt = 0.0;
        int    phase = 0;   // 0: pre-engage, 1: engaged, 2: released
        int    n = 0;

        while (n < total)
        {
            if (phase == 0 && n >= engageAt)  { setParam (p, "ENGAGE", 1.0f); phase = 1; }
            if (phase == 1 && n >= releaseAt) { setParam (p, "ENGAGE", 0.0f); phase = 2; }

            int chunk = juce::jmin (4096, total - n);
            if (phase == 0) chunk = juce::jmin (chunk, engageAt - n);
            if (phase == 1) chunk = juce::jmin (chunk, releaseAt - n);
            if (chunk <= 0) chunk = 1;

            juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), 2, chunk);
            for (int s = 0; s < chunk; ++s)
            {
                view.setSample (0, s, noiseFill (0, n + s));
                view.setSample (1, s, noiseFill (1, n + s));
            }

            p.processBlock (view, midi);

            for (int s = 0; s < chunk; ++s)
            {
                y.L.push_back (view.getSample (0, s));
                y.R.push_back (view.getSample (1, s));
            }

            n += chunk;
            maxDebt = juce::jmax (maxDebt, p.getDebtSamplesForTest());
        }

        // Liveness: the hold was actually silent (Stopped landed).
        const double holdMag = maxAbsIn (y.L, releaseAt - 96000, releaseAt - 48000);

        const int tailBad = firstNonDry (y, noiseFill,
                                         releaseAt + 12000 + kCatchupLen + kFadeLen + 2400, total);

        check ("P4b-long-hold-clamped-resume",
               allFinite (y.L) && allFinite (y.R)
                 && maxDebt <= bound && holdMag < 1.0e-6 && tailBad < 0,
               "maxDebt=" + juce::String (maxDebt, 0) + " (bound " + juce::String (bound, 0)
                 + "), holdMag=" + juce::String (holdMag, 8)
                 + (tailBad < 0 ? juce::String ("; tail dry")
                                : juce::String ("; tail diff @") + juce::String (tailBad)));
    }

    // ── P5: pathological input / sticky state (QUAL-01) ──────────────────────
    // Silence, DC, full-scale impulse train, clean sine — a full gesture cycle
    // on each; no NaN/Inf anywhere; output returns to BITWISE dry after the
    // resync (no sticky transport/TPT state — the Bypassed path is untouched
    // and every engage re-primes the filter state).
    {
        struct PathCase { const char* name; float (*fill) (int, int); };

        const PathCase cases[] = {
            { "P5-silence",       &fillSilence },
            { "P5-dc",            &fillDC },
            { "P5-impulse-train", &fillImpulse },
            { "P5-sine",          &sine440 },
        };

        for (const auto& c : cases)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE", 1.0f);
            setParam (p, "STOP_FREE_MS", 100.0f);

            const int total = 49152;

            auto y = renderTimeline (p, total, { 512 },
                                     { { 4096, "ENGAGE", 1.0f }, { 12288, "ENGAGE", 0.0f } },
                                     c.fill);

            const int bad = firstNonDry (y, c.fill, 45056, total);

            check (c.name,
                   allFinite (y.L) && allFinite (y.R) && bad < 0,
                   bad < 0 ? "finite; tail bitwise dry"
                           : "tail diff @" + juce::String (bad));
        }
    }

    // ── toneTrack: centroid falls with |r|; a = 0 transparent (DSP-05) ───────
    // Two renders of the SAME 8 s spin-down on noise, a = 0 vs a = 60. The
    // varispeed slowdown darkens BOTH (content shifts down) — the a = 0
    // render is the negative control that isolates the FILTER's contribution.
    // Window E: u ≈ 0.06–0.13 (r ≈ 0.88–0.77); window L: u ≈ 0.50–0.56
    // (r ≈ 0.25–0.19, fc(a=0.6) ≈ 2 kHz).
    {
        auto runTone = [] (float amt)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE", 1.0f);
            setParam (p, "STOP_FREE_MS", 8000.0f);
            setParam (p, "TONE_TRACK", amt);
            return renderTimeline (p, 240000, { 512 },
                                   { { 0, "ENGAGE", 1.0f } },
                                   noiseFill);
        };

        auto x0  = runTone (0.0f);
        auto x60 = runTone (60.0f);

        // E early in the ramp (u ≈ 0.01–0.06, r ≈ 0.97–0.88 → fc(a=0.6) is
        // nearly open at ~16 kHz); L deep in it (r ≈ 0.25–0.19 → fc ≈ 2 kHz).
        const int eLo = 4800,   eHi = 24000;
        const int lLo = 192000, lHi = 216000;

        // METRIC NOTE (fixed after a live false-FAIL): the first-difference
        // HF proxy weights spectral energy ∝ f, which nearly CANCELS a
        // one-pole's 1/f rolloff — it is structurally near-blind to a
        // 6 dB/oct filter (measured: hf-drop 0.280 vs 0.341 while the band
        // energy showed a genuine −2.9 dB). The discriminating instrument is
        // the a60-vs-a0 band-energy ATTENUATION and its GROWTH down the
        // sweep; the hf ratios are kept only as a loose direction check.
        const double attE = 20.0 * std::log10 (juce::jmax (1.0e-12, rmsIn (x60.L, eLo, eHi))
                                               / juce::jmax (1.0e-12, rmsIn (x0.L, eLo, eHi)));
        const double attL = 20.0 * std::log10 (juce::jmax (1.0e-12, rmsIn (x60.L, lLo, lHi))
                                               / juce::jmax (1.0e-12, rmsIn (x0.L, lLo, lHi)));

        const double drop0  = hfRatio (x0.L,  lLo, lHi) / juce::jmax (1.0e-12, hfRatio (x0.L,  eLo, eHi));
        const double drop60 = hfRatio (x60.L, lLo, lHi) / juce::jmax (1.0e-12, hfRatio (x60.L, eLo, eHi));

        check ("toneTrack-centroid-falls",
               allFinite (x60.L)
                 && attL < attE - 0.8      // attenuation GROWS as |r| falls
                 && attL < -2.0            // and is unambiguous deep in the ramp
                 && attE > -3.0            // sanity: near-open at speed ≈ 1
                 && drop60 < drop0,        // weak metric agrees on direction
               "att a60-vs-a0: E=" + juce::String (attE, 2) + " dB → L="
                 + juce::String (attL, 2) + " dB (need L < E−0.8 and < −2); hf drop a60="
                 + juce::String (drop60, 3) + " vs a0=" + juce::String (drop0, 3));

        const double rmsE0  = rmsIn (x0.L,  eLo, eHi);
        const double rmsL0  = rmsIn (x0.L,  lLo, lHi);
        const double rmsL60 = rmsIn (x60.L, lLo, lHi);

        const double a0DeltaDb  = 20.0 * std::log10 (juce::jmax (1.0e-12, rmsL0 / juce::jmax (1.0e-12, rmsE0)));
        const double a60DropDb  = 20.0 * std::log10 (juce::jmax (1.0e-12, rmsL60 / juce::jmax (1.0e-12, rmsL0)));

        check ("toneTrack-a0-transparent",
               std::abs (a0DeltaDb) < 2.5 && a60DropDb < -2.0,
               "a0 band-energy delta E→L=" + juce::String (a0DeltaDb, 2)
                 + " dB (|.|<2.5, Catmull loss ~1 dB budgeted); a60 vs a0 at L="
                 + juce::String (a60DropDb, 2) + " dB (<-2, one-pole slope)");

        // No-zipper on the cutoff glide: covered structurally by the P6 grid
        // (TONE_TRACK sits at its shipped default 60 there, so every P6 cell
        // sweeps the cutoff through the full ramp under the first-difference
        // bound) and by the invariance probes (the 16-sample ABSOLUTE grid is
        // exactly their exposure).
    }

    // ── Scratch: drawn LUT plays once per engage; pitch follows (FUNC-02) ────
    // Two flat segments: r = 0.5 (φ < 0.49) then r = 1.5 (φ > 0.51) over a
    // 2 s pass on a 440 Hz sine → 220 Hz then 660 Hz, then ONE resync back to
    // bitwise dry (a second pass would keep the tail wet).
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "MODE", 1.0f);
        setParam (p, "ENV_FREE_MS", 2000.0f);

        const bool committed = p.commitScratchEnvelopeJson (
            R"({"v":1,"points":[{"x":0,"y":0.25},{"x":0.49,"y":0.25},{"x":0.51,"y":0.75},{"x":1,"y":0.75}]})");

        const int engageAt = 4096;
        const int total    = 110592;

        auto y = renderTimeline (p, total, { 512 },
                                 { { engageAt, "ENGAGE", 1.0f } },
                                 sine440);

        const double fLow  = autocorrPitchHz (y.L, engageAt + 24000, 1024, kFs, 175.0, 275.0, 0.25);
        const double fHigh = autocorrPitchHz (y.L, engageAt + 72000, 1024, kFs, 520.0, 820.0, 0.25);

        const int tailBad = firstNonDry (y, sine440, 105000, total);

        check ("scratch-lut-plays-once",
               committed && allFinite (y.L) && allFinite (y.R)
                 && fLow  > 0.0 && std::abs (fLow  / 220.0 - 1.0) <= 0.08
                 && fHigh > 0.0 && std::abs (fHigh / 660.0 - 1.0) <= 0.08
                 && tailBad < 0,
               "f(phi=.25)=" + juce::String (fLow, 1) + " Hz (exp 220), f(phi=.75)="
                 + juce::String (fHigh, 1) + " Hz (exp 660)"
                 + (tailBad < 0 ? juce::String ("; tail dry")
                                : juce::String ("; tail diff @") + juce::String (tailBad)));
    }

    // ── Scratch: direction flip artifact-free (DSP-04) ───────────────────────
    // r runs +1 → −1 linearly over 1 s; the zero crossing at φ = 0.5 is a
    // palindrome corner — position stays continuous, the stop-fade never
    // fires, and the P6 metric must stay clean across the flip.
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE", 1.0f);
        setParam (p, "MODE", 1.0f);
        setParam (p, "ENV_FREE_MS", 1000.0f);

        const bool committed = p.commitScratchEnvelopeJson (
            R"({"v":1,"points":[{"x":0,"y":0.5,"curve":0},{"x":1,"y":-0.5,"curve":0}]})");

        const int engageAt = 4096;
        const int flipAt   = engageAt + 24000;
        const int total    = 61952;

        auto y = renderTimeline (p, total, { 512 },
                                 { { engageAt, "ENGAGE", 1.0f } },
                                 sine440);

        const double flipDiff = juce::jmax (maxFirstDiff (y.L, flipAt - 2400, flipAt + 2400),
                                            maxFirstDiff (y.R, flipAt - 2400, flipAt + 2400));

        double dev = 0.0;
        for (int n = flipAt - 2400; n < flipAt + 2400; ++n)
            dev = juce::jmax (dev, (double) std::abs (y.L[(size_t) n] - sine440 (0, n)));

        check ("scratch-direction-flip",
               committed && allFinite (y.L) && allFinite (y.R)
                 && flipDiff <= kP6Bound && dev > 0.01,
               "flip maxDiff=" + juce::String (flipDiff, 5) + " (bound "
                 + juce::String (kP6Bound, 5) + "), dev=" + juce::String (dev, 3));
    }

    // ── Scratch: disengaged mode switch is bitwise silent (FUNC-02) ──────────
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);

        auto y = renderTimeline (p, 48000, { 512 },
                                 { { 12288, "MODE", 1.0f }, { 24576, "MODE", 0.0f } },
                                 noiseFill);

        const int bad = firstNonDry (y, noiseFill, 0, 48000);

        check ("scratch-mode-switch-silent", bad < 0,
               bad < 0 ? "48000 samples memcmp-equal to input"
                       : "first diff @" + juce::String (bad));
    }

    //==========================================================================
    // ── v1.1 Continuous mode ─────────────────────────────────────────────────

    // C-P0/P1a/P2 per character: determinism (two fresh instances bitwise),
    // 512-vs-4096 bit-identity (this probe IS the enforcement of the seeded
    // per-purpose RNG discipline), liveness (the engaged span must deviate
    // from dry — a generator wired to nothing passes every other bound
    // vacuously), and post-release bitwise dry through the full SpinUp →
    // Catchup → ResyncXfade path. Edges at multiples of 4096.
    {
        const char* charNames[] = { "wobble", "random", "glitch" };

        for (int c = 0; c < 3; ++c)
        {
            auto run = [c] (int blk)
            {
                TapestopProcessor p;
                prepareAndSettle (p, blk);
                setParam (p, "SYNC_MODE",    1.0f);   // Free
                setParam (p, "MODE",         2.0f);   // Continuous
                setParam (p, "CHARACTER",    (float) c);
                setParam (p, "CONT_RATE_HZ", c == 2 ? 8.0f : 2.0f);   // glitch: 6000-sample cells
                setParam (p, "CONT_DEPTH",  60.0f);
                setParam (p, "CONT_CHAOS",  60.0f);
                return renderTimeline (p, 122880, { blk },
                                       { { 8192, "ENGAGE", 1.0f }, { 57344, "ENGAGE", 0.0f } },
                                       noiseFill);
            };

            auto a = run (512);
            auto b = run (512);
            auto l = run (4096);

            juce::String d0, d1;
            const bool det = identicalVec (a.L, b.L, d0) && identicalVec (a.R, b.R, d1);

            juce::String dL, dR;
            const bool inv = identicalVec (a.L, l.L, dL) && identicalVec (a.R, l.R, dR);

            double dev = 0.0;
            for (int n = 16000; n < 50000; ++n)
                dev = juce::jmax (dev, (double) std::abs (a.L[(size_t) n] - noiseFill (0, n)));

            check ((juce::String ("C-P0-determinism-") + charNames[c]).toRawUTF8(),
                   det && dev > 0.01,
                   "runs " + d0 + "; engagedDev=" + juce::String (dev, 4));

            check ((juce::String ("C-P1a-blocksize-") + charNames[c]).toRawUTF8(),
                   inv, "L " + dL + "; R " + dR);

            // Release @57344: spin-up ≤ 12000 (250 ms), catchup capped 12000,
            // fade 2400 → dry well before 57344 + 32768.
            const int bad = firstNonDry (a, noiseFill, 57344 + 32768, 122880);

            check ((juce::String ("C-P2-null-after-release-") + charNames[c]).toRawUTF8(),
                   bad < 0,
                   bad < 0 ? "tail memcmp-equal to input"
                           : "first diff @" + juce::String (bad));
        }
    }

    // C-P0x (v1.2): glitch at CHAOS/DEPTH 100 — the everything-unlocked path
    // (4 slots/cell, freeze/slam/chatter/shuffle, stutter roll + pitch-ramp
    // all reachable). Same three gates as C-P0/P1a/P2: two fresh instances
    // bitwise, 512-vs-4096 bit-identity, post-release bitwise dry. This is
    // the enforcement that every NEW event type keeps all state inside
    // tick() and every draw on the seeded per-purpose streams.
    {
        auto run = [] (int blk)
        {
            TapestopProcessor p;
            prepareAndSettle (p, blk);
            setParam (p, "SYNC_MODE",    1.0f);
            setParam (p, "MODE",         2.0f);
            setParam (p, "CHARACTER",    2.0f);
            setParam (p, "CONT_RATE_HZ", 8.0f);
            setParam (p, "CONT_DEPTH", 100.0f);
            setParam (p, "CONT_CHAOS", 100.0f);
            return renderTimeline (p, 122880, { blk },
                                   { { 8192, "ENGAGE", 1.0f }, { 57344, "ENGAGE", 0.0f } },
                                   noiseFill);
        };

        auto a = run (512);
        auto b = run (512);
        auto l = run (4096);

        juce::String d0, d1;
        const bool det = identicalVec (a.L, b.L, d0) && identicalVec (a.R, b.R, d1);

        juce::String dL, dR;
        const bool inv = identicalVec (a.L, l.L, dL) && identicalVec (a.R, l.R, dR);

        double dev = 0.0;
        for (int n = 16000; n < 50000; ++n)
            dev = juce::jmax (dev, (double) std::abs (a.L[(size_t) n] - noiseFill (0, n)));

        check ("C-P0x-glitch-max-determinism", det && dev > 0.01,
               "runs " + d0 + "; engagedDev=" + juce::String (dev, 4));

        check ("C-P0x-glitch-max-blocksize", inv, "L " + dL + "; R " + dR);

        const int bad = firstNonDry (a, noiseFill, 57344 + 32768, 122880);
        check ("C-P0x-glitch-max-null-after-release", bad < 0,
               bad < 0 ? "tail memcmp-equal to input"
                       : "first diff @" + juce::String (bad));
    }

    // C-P4c: debt stays bounded under a LONG worst-case hold. Random at max
    // depth/chaos random-walks its position debt (rate mean-reverts, the
    // integral does not) — the ±0.2 % servo must pin it; Glitch's one-sided
    // event debt must be recentred by the debt-biased scheduler + soft budget
    // + hard resync-snap. 40 s spans ≫ every modulation period in play
    // (pattern_metric_window_vs_modulation_period). Bound: hard budget (6 s)
    // + one glitch cell + margin, and NEVER the ring rail (the rail is the
    // pitch-snap artifact; measured against the ACTUAL allocated span —
    // pattern_test_fixture_mirrors_drift_silently).
    for (int c = 1; c <= 2; ++c)
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE",    1.0f);
        setParam (p, "MODE",         2.0f);
        setParam (p, "CHARACTER",    (float) c);
        setParam (p, "CONT_RATE_HZ", c == 2 ? 8.0f : 4.0f);
        setParam (p, "CONT_DEPTH", 100.0f);
        setParam (p, "CONT_CHAOS", 100.0f);
        setParam (p, "ENGAGE",       1.0f);

        const int total = (int) (40.0 * kFs);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;

        double maxDebt = 0.0;
        int    n       = 0;
        bool   finite  = true;

        while (n < total)
        {
            const int chunk = juce::jmin (512, total - n);
            juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), 2, chunk);

            for (int s = 0; s < chunk; ++s)
            {
                view.setSample (0, s, noiseAt (n + s));
                view.setSample (1, s, noiseAt (n + s + 7919));
            }

            p.processBlock (view, midi);
            finite = finite && std::isfinite (view.getSample (0, chunk - 1));

            n += chunk;
            maxDebt = juce::jmax (maxDebt, p.getDebtSamplesForTest());
        }

        const double hardBound = (6.0 + 2.0) * kFs;
        const double rail      = (double) p.getRingSpanForTest()
                               - (double) VarispeedVoice::kInterpGuard;

        check ((juce::String ("C-P4c-debt-bound-") + (c == 1 ? "random" : "glitch")).toRawUTF8(),
               finite && maxDebt < hardBound && maxDebt < rail,
               "maxDebt=" + juce::String (maxDebt / kFs, 3) + " s (bound "
                 + juce::String (hardBound / kFs, 1) + " s, rail "
                 + juce::String (rail / kFs, 1) + " s)");
    }

    // C-P6: continuity. Wobble at full depth is C1 by construction (sines +
    // filtered noise + 50 ms depth slew); Glitch may STEP r (2 ms slew) and
    // splice positions (3–50 ms fades) but the WAVEFORM must stay inside a
    // first-difference bound on a 440 Hz probe tone. Wobble uses the scratch
    // r = ±1.8 precedent bound; Glitch (v1.2) uses the ±2-rail bound — slam
    // holds r = 2 and ramped-stutter splices can sum two near-rail voices.
    for (int c = 0; c <= 2; c += 2)
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);
        setParam (p, "SYNC_MODE",    1.0f);
        setParam (p, "MODE",         2.0f);
        setParam (p, "CHARACTER",    (float) c);
        setParam (p, "CONT_RATE_HZ", c == 2 ? 8.0f : 1.2f);
        setParam (p, "CONT_DEPTH",  c == 2 ? 60.0f : 100.0f);
        setParam (p, "CONT_CHAOS",  c == 2 ? 100.0f : 40.0f);

        const int total = 192000;   // 4 s — ≥ 25 glitch cells at 8 Hz
        auto y = renderTimeline (p, total, { 512 },
                                 { { 4096, "ENGAGE", 1.0f } },
                                 sine440);

        const double diff = juce::jmax (maxFirstDiff (y.L, 8192, total),
                                        maxFirstDiff (y.R, 8192, total));

        double dev = 0.0;
        for (int n2 = 48000; n2 < 96000; ++n2)
            dev = juce::jmax (dev, (double) std::abs (y.L[(size_t) n2] - sine440 (0, n2)));

        const double p6bound = c == 2 ? kP6BoundGlitch : kP6Bound;

        check ((juce::String ("C-P6-continuity-") + (c == 0 ? "wobble" : "glitch")).toRawUTF8(),
               allFinite (y.L) && allFinite (y.R) && diff <= p6bound && dev > 0.01,
               "maxDiff=" + juce::String (diff, 5) + " (bound "
                 + juce::String (p6bound, 5) + "), dev=" + juce::String (dev, 3));
    }

    // C-zipper: DEPTH is LIVE (16-sample-grid latch — two renders differing
    // only in depth must diverge while engaged: liveness gate first,
    // pattern_zipper_sweep_probe_needs_liveness_gate), and a mid-engage
    // depth jump must ride the 50 ms slew with no step in the output.
    {
        auto run = [] (float depthLate)
        {
            TapestopProcessor p;
            prepareAndSettle (p, 512);
            setParam (p, "SYNC_MODE",    1.0f);
            setParam (p, "MODE",         2.0f);
            setParam (p, "CHARACTER",    0.0f);
            setParam (p, "CONT_RATE_HZ", 1.2f);
            setParam (p, "CONT_DEPTH",  35.0f);
            setParam (p, "CONT_CHAOS",   0.0f);
            return renderTimeline (p, 144000, { 512 },
                                   { { 4096,  "ENGAGE",     1.0f },
                                     { 72704, "CONT_DEPTH", depthLate } },
                                   sine440);
        };

        auto swept  = run (100.0f);
        auto steady = run (35.0f);

        bool live = false;
        for (int n = 80000; n < 130000 && ! live; ++n)
            live = ! bitExact (swept.L[(size_t) n], steady.L[(size_t) n]);

        const double stepDiff = juce::jmax (maxFirstDiff (swept.L, 72704 - 2400, 72704 + 4800),
                                            maxFirstDiff (swept.R, 72704 - 2400, 72704 + 4800));

        check ("C-zipper-depth-live-and-smooth",
               live && stepDiff <= kP6Bound,
               juce::String (live ? "live" : "DEAD PARAM") + "; step maxDiff="
                 + juce::String (stepDiff, 5) + " (bound " + juce::String (kP6Bound, 5) + ")");
    }

    // C-preset-migration: a pre-1.1 preset stored MODE NORMALIZED over 2
    // choices (Scratch = 1.0); over 3 choices the same fraction decodes as
    // Continuous. The registered migration callback must remap it — and must
    // NOT touch a 1.1.0-format preset (0.5 already = Scratch).
    {
        TapestopProcessor p;
        prepareAndSettle (p, 512);

        auto tmpDir = juce::File::getSpecialLocation (juce::File::tempDirectory);

        auto writePreset = [&tmpDir] (const char* name, const char* version, double modeNorm)
        {
            auto* params = new juce::DynamicObject();
            params->setProperty ("MODE",   modeNorm);
            params->setProperty ("ENGAGE", 0.0);

            auto* preset = new juce::DynamicObject();
            preset->setProperty ("name",       name);
            preset->setProperty ("version",    version);
            preset->setProperty ("parameters", juce::var (params));

            auto f = tmpDir.getChildFile (juce::String (name) + ".json");
            f.replaceWithText (juce::JSON::toString (juce::var (preset), true));
            return f;
        };

        auto oldFile = writePreset ("ot-mig-old", "1.0.0", 1.0);
        auto newFile = writePreset ("ot-mig-new", "1.1.0", 0.5);

        const bool loadedOld = p.presetManager.loadPresetFromFile (oldFile);
        const int  modeOld   = (int) std::lround (
            (double) p.parameters.getRawParameterValue ("MODE")->load());

        const bool loadedNew = p.presetManager.loadPresetFromFile (newFile);
        const int  modeNew   = (int) std::lround (
            (double) p.parameters.getRawParameterValue ("MODE")->load());

        oldFile.deleteFile();
        newFile.deleteFile();

        check ("C-preset-migration-mode",
               loadedOld && modeOld == 1 && loadedNew && modeNew == 1,
               "v1.0.0 n=1.0 -> index " + juce::String (modeOld)
                 + " (want 1=Scratch); v1.1.0 n=0.5 -> index " + juce::String (modeNew)
                 + " (want 1)");
    }

    //==========================================================================
    std::printf ("\n%d probe checks, %d failure(s)\n", probes, failures);
    return failures == 0 ? 0 : 1;
}
