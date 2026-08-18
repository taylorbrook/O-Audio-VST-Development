/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot render-harness — Stage-2 DSP correctness gate (Phase 2.1).

    Probes (Phase 2.1: engine core + tape):

      A  latency-reported     — getLatencySamples() == ceil(0.020 * fs).
      B  FUNC-02 null         — all families off => out[n] bit-equals
                                in[n - kCompLatency] (position-deterministic
                                noise input; NO tolerance — the all-off path
                                is a pure integer delay).
      C  DSP-01 bends         — sine input, forced tape bends: tracked
                                autocorrelation pitch trace shows continuous
                                ramps (no steps), bends actually move pitch.
      D  DSP-01 stops         — forced tape stops: no click (bounded
                                sample-to-sample delta) and a genuine hold
                                (a long run of bit-identical samples).
      E  FUNC-04 determinism  — two FRESH instances, same seed => maxAbsDiff
                                == 0.0 (+ liveness); different SEED differs.
      F  QUAL-02 512-vs-4096  — families on, fixed seed, memcmp bit-identity.
      G  QUAL-02 ragged       — {1,7,64,333,4096} vs {4096}, memcmp.
      H  FUNC-03 free rate    — first event onset lands at fs/rate for two
                                different CLOCK_FREE_RATE settings.
      I  FUNC-03 sync         — mock playhead: first event onset follows the
                                BPM (120 vs 240); stopped transport emits
                                nothing (negative control for the detector).

    Probes (Phase 2.2: CD skip + vinyl):

      J  DSP-02 conceal       — severity 0 forces rung 1: HF-energy dip on a
                                noise input, level never collapses (no mute).
      K  DSP-02 mute          — severity 0.45: a 5 ms RMS window collapses
                                (hard mute engaged), level otherwise intact.
      L  DSP-02 loop          — severity 1.0 forces the buffer loop on a
                                position-marker saw: restarts at EXACT
                                sector-quantised intervals, first onset on the
                                clock grid (FUNC-01), chirp energy at every
                                restart, recovery jumps FORWARD to live.
      M  DSP-03 vinyl         — saw marker: backward jump distances are
                                integer revolution multiples, forward jumps
                                land at live, onsets on the clock grid
                                (FUNC-01); sine: pitch never changes across
                                jumps; pops audible (A/B vs VINYL_POP 0).
      N  collision determinism— all three families at 100%: same seed =>
                                bit-identical fresh-instance renders; plus
                                all-fire ragged-blocksize bit-identity.

    Probes (Phase 2.3: packet loss):

      O  DSP-04 GE statistics — 60 s render, Silence concealment: lost
                                packets detected from OUTPUT RMS on the
                                packet grid; run-length statistics match the
                                geometric expectation (ratio bounds).
      P  DSP-04 concealment   — four modes pairwise distinct on a periodic
                                input; Substitute==Decay prints an explicit
                                FLAG (auto-degrade) instead of silently
                                passing.
      Q  QUAL-02 packet grid  — 512-vs-4096 + ragged bit-identity with the
                                packet stage ACTIVE (high loss, fixed seed).
      O2 v1.2 knob-zero clean — PACKET_LOSS 0 while enabled loses NOTHING
                                (Good-state floor now scales with loss01).
      O3 v1.2 full-knob loss  — PACKET_LOSS 100 delivers true >= 93%
                                measured loss (expected ~0.986).
      P2 v1.2 decay mute-out  — twin Silence/Decay renders (same seed =
                                same schedule): in runs >= 5 lost packets,
                                rep 1 audible, reps 4+ exactly silent.

    Probes (Phase 2.4: crush + quant):

      R  DSP-06 crush sweeps  — CRUSH_RATE liveness + hold-interval trace
                                glides (no zipper step) across a full-range
                                param step; CRUSH_BITS liveness + smoothed
                                staircase (>= 3 intermediate levels on DC);
                                fractional rate renders without warble
                                (envelope flatness).
      S  DSP-07 duck vs pump  — burst/tail signal: duck crushes the tail
                                harder, pump crushes the transient harder
                                (normalized quantization-error energy per
                                segment vs an env-amt-0 reference); plus
                                QUAL-02 bit-identity with crush+quant active
                                (per-sample follower is the load-bearing
                                piece).
      T  DSP-08 dither        — CRUSH_DITHER 0 vs 2 renders differ (same
                                seed/schedule — dither draws are
                                unconditional).
      U  QUAL-01 pathological — DC, silence, full-scale square, NaN
                                injection, then clean input: output recovers,
                                never sticky NaN/Inf.

    Probes (Phase 2.5: codec):

      V  GSM round-trip GATE  — standalone libgsm encode/decode of sine
                                frames (no plugin): finite, bounded,
                                correlated. Gates the DSP-05 probes.
      W  DSP-05 mu-law band   — codec on, mu-law: energy above ~8 kHz
                                collapses vs the codec-off control, the
                                300-3400 passband survives, sub-150 Hz is
                                filtered.
      X  DSP-05 mu-law noise  — quantization/distortion noise tracks signal
                                level (companding property).
      Y  DSP-05 GSM alignment — GSM-mode output cross-correlates against the
                                codec-off (plain delay) output with peak lag
                                within +/- fs/8000 samples of 0.
      Z  latency report       — getLatencySamples() == kCompLatency in every
                                codec state (never renegotiated); QUAL-02
                                bit-identity re-run with codec active in both
                                modes.
      P1 PERF-01              — measured render-time ratio, worst case (all
                                families + packet + crush + GSM), printed +
                                asserted <= 0.15 (the one sanctioned
                                wall-clock use).

    Probes (v1.6.0: CD skip authenticity, brief items 14 and 18):

      K2 mute scales          — longest rung-1 mute against the ceiling its
                                severity implies: <= 63 ms at 0.2, > 100 ms
                                at 0.6 (impossible under the old 20 ms cap),
                                <= 150 ms either way.
      L3 sector lock + gate   — CD_SEGMENT 100 ms loops at 5120 (a whole
                                number of 1/75 s sectors) above severity 0.5
                                and at the free 4800 at 0.5. Both halves
                                asserted: the gate is the claim.
      L4 servo seek + gate    — a loop released at severity 1.0 goes hard
                                silent for 100-400 ms, THEN jumps to live;
                                at exactly 0.85 (the strict >) it does not,
                                while still looping up to the release.
      L5 per-jump fade        — ReadHead driven DIRECTLY (the chirp buries
                                the splice in a plugin render): blend width
                                is 144 samples by default and 24 for a loop
                                wrap, and a splice request arriving mid-fade
                                SPENDS the running length rather than
                                rescaling it into a 0.36 output step.

    Each of the five was verified to FAIL against the code it gates, by
    reverting that one behaviour and re-running (see the CHANGELOG).

    Probes (v1.10.0: the Rot family, brief item 8):

      R1 flip impulses        — DEPTH 90 puts > 100 samples more than a bit-11
                                flip away from the reference and DEPTH 0 puts
                                NONE there (so the knob is load-bearing);
                                output never leaves [-1, 1], which is the
                                post-clip.
      R2 sticky hold          — longest BIT-IDENTICAL output run falls inside
                                the specified 10-80 ms, and the STICK 0 control
                                has no plateau at all.
      R3 garble env-match     — twin renders 20 dB apart on the same seed: over
                                the windows that decorrelate from the
                                programme, the loud/quiet RMS ratio tracks the
                                INPUT ratio (~10x). Fixed-level noise reads 1.0
                                here, which is what makes it a real test of the
                                envelope match rather than of noisiness.
      R4 blocksize identity   — all three kinds live: {512}, {4096} and ragged
                                are bit-identical. This is the measurement
                                behind the single-stream decision in RngBank.
      R5 rot-off null         — ROT_ENABLE off with the other four knobs at
                                100: bit-exact null, no tolerance. The
                                containment claim the release rests on.

    R1-R3 and R5 use tape/cd/vinyl DISABLED, which makes the wet path a pure
    integer delay and the reference exactly input[n - kComp] — so the residual
    is the rot bus with nothing else mixed into it.

    Conventions (all have shipped-bug war stories):
      * position-deterministic noiseAt(n) — NEVER a sequential RNG
        (pattern_rng_stream_interleave_blocksize);
      * setBaseline() at the top of EVERY probe (harness param leak);
      * param writes only via setValueNotifyingHost(convertTo0to1(...));
      * every potentially-vacuous probe carries a liveness control;
      * no wall-clock inside any verdict
        (pattern_wallclock_inside_a_stability_verdict).

    Exit 0 iff all checks pass. Off by default; -DOUARICON_BUILD_TESTS=ON.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <gsm.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

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

    std::printf ("  [%s] %-28s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
}

/** Bit-exact float comparison via the object representation. No `==`, so no
    -Wfloat-equal. */
bool bitExact (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof (float)) == 0;
}

constexpr double kFs       = 48000.0;
constexpr int    kMaxBlock = 4096;
constexpr double kSineHz   = 220.0;
constexpr float  kSineAmp  = 0.5f;

//==============================================================================
/** POSITION-DETERMINISTIC broadband excitation — a hash of the ABSOLUTE
    sample index. The block-size probes depend on this being a function of n
    and nothing else: a sequential generator advanced per call would emit a
    different signal for a 512-render than for a 4096-render, so the memcmp
    would compare two different experiments. */
float noiseAt (int n) noexcept
{
    auto h = static_cast<std::uint32_t> (n) * 2654435761u + 0x9E3779B9u;
    h ^= h >> 15;  h *= 0x85EBCA6Bu;
    h ^= h >> 13;  h *= 0xC2B2AE35u;
    h ^= h >> 16;

    return 0.5f * (static_cast<float> (h) / 2147483648.0f - 1.0f);
}

float noiseStereo (int ch, int n) noexcept
{
    return ch == 0 ? noiseAt (n) : noiseAt (n + 1000003);
}

/** Position-computed sine (phase from n directly — never an accumulated
    oscillator, for the same reason as noiseAt). Starts at phase 0 so the
    delayed onset at n = kCompLatency is click-free by construction. */
float sineStereo (int ch, int n) noexcept
{
    juce::ignoreUnused (ch);
    return kSineAmp * static_cast<float> (
               std::sin (2.0 * juce::MathConstants<double>::pi * kSineHz
                         * static_cast<double> (n) / kFs));
}

using InputFn = float (*) (int ch, int n);

//==============================================================================
/** Writes a parameter in ENGINEERING UNITS, synchronously.
    setValueNotifyingHost() is fully synchronous and updates the cached
    atomics; bare setValue() would leave them stale and the harness would
    test the defaults while reporting green. */
void setParam (OBitrotAudioProcessor& proc, const char* id, float engineeringValue)
{
    auto* p = proc.apvts.getParameter (id);
    jassert (p != nullptr);

    if (p != nullptr)
        p->setValueNotifyingHost (p->convertTo0to1 (engineeringValue));
    else
        std::printf ("  !! unknown param id '%s'\n", id);
}

/** Resets ALL 45 parameters to the parameter-spec defaults. Traps: the
    neutral value is often not the range minimum (CRUSH_BITS 16, CRUSH_RATE
    20000, MIX 100, CODEC_AGC 100); the latching params (the seven *_ENABLEs,
    HARD_EDGES) matter most. Called at the top of EVERY probe. */
void setBaseline (OBitrotAudioProcessor& proc)
{
    // Global (6)
    setParam (proc, "CLOCK_MODE",      0.0f);      // Sync
    setParam (proc, "CLOCK_SYNC_DIV",  2.0f);      // 1/8
    setParam (proc, "CLOCK_FREE_RATE", 2.0f);
    setParam (proc, "SEED",            0.0f);
    setParam (proc, "HARD_EDGES",      0.0f);      // Off
    setParam (proc, "MIX",             100.0f);

    // Tape (6)
    setParam (proc, "TAPE_ENABLE",     1.0f);      // On
    setParam (proc, "TAPE_PROB",       25.0f);
    setParam (proc, "TAPE_STOP_PROB",  10.0f);
    setParam (proc, "TAPE_RAMP",       150.0f);
    setParam (proc, "TAPE_DROP",       0.0f);      // v1.4.0, transparent at 0
    setParam (proc, "TAPE_WOW",        0.0f);      // v1.4.0, transparent at 0

    // CD Skip (4)
    setParam (proc, "CD_ENABLE",       1.0f);      // On
    setParam (proc, "CD_PROB",         25.0f);
    setParam (proc, "CD_SEVERITY",     0.5f);
    setParam (proc, "CD_SEGMENT",      100.0f);

    // Vinyl (4)
    setParam (proc, "VINYL_ENABLE",    1.0f);      // On
    setParam (proc, "VINYL_PROB",      25.0f);
    setParam (proc, "VINYL_RPM",       0.0f);      // 33 1/3
    setParam (proc, "VINYL_POP",       50.0f);

    // Packet Loss (4)
    setParam (proc, "PACKET_ENABLE",   0.0f);      // Off
    setParam (proc, "PACKET_LOSS",     20.0f);
    setParam (proc, "PACKET_BURST",    30.0f);
    setParam (proc, "PACKET_CONCEAL",  2.0f);      // Decay

    // Codec (3)
    setParam (proc, "CODEC_ENABLE",    0.0f);      // Off
    setParam (proc, "CODEC_MODE",      0.0f);      // Mu-law
    setParam (proc, "CODEC_MIX",       100.0f);

    // Crush (6)
    setParam (proc, "CRUSH_ENABLE",    0.0f);      // Off
    setParam (proc, "CRUSH_BITS",      16.0f);
    setParam (proc, "CRUSH_RATE",      20000.0f);
    setParam (proc, "CRUSH_JITTER",    0.0f);
    setParam (proc, "CRUSH_ENV_AMT",   0.0f);
    setParam (proc, "CRUSH_DITHER",    0.0f);

    // Media noise beds (5) — v1.5.0, every one transparent at 0
    setParam (proc, "TAPE_HISS",       0.0f);
    setParam (proc, "VINYL_WEAR",      0.0f);
    setParam (proc, "CODEC_NOISE",     0.0f);
    setParam (proc, "CODEC_MAINS",     0.0f);      // 50 Hz
    setParam (proc, "PACKET_COMFORT",  0.0f);

    // Vinyl warp (1) — v1.7.0, transparent at 0
    setParam (proc, "VINYL_WARP",      0.0f);

    // Rot (5) — v1.10.0. ROT_ENABLE off is the transparent state and the other
    // four are inert behind it, which is exactly why the pre-v1.10.0 digest
    // anchors (A3, V1, N7, N8) stay valid with these lines added to the
    // baseline: a fresh processor is already here, so setting them changes
    // nothing, and a probe that turned rot on cannot leak into the next one.
    setParam (proc, "ROT_ENABLE",      0.0f);      // Off
    setParam (proc, "ROT_PROB",        25.0f);
    setParam (proc, "ROT_DEPTH",       50.0f);
    setParam (proc, "ROT_STICK",       25.0f);
    setParam (proc, "ROT_GARBLE",      25.0f);
}

//==============================================================================
/** Renders `totalSamples` into `dest`, chopping processBlock calls per the
    (cyclically-walked) `sizes` sequence. Input is a pure function of the
    absolute sample index, so every size sequence sees the SAME signal. */
void renderInto (OBitrotAudioProcessor& proc, juce::AudioBuffer<float>& dest,
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

/** Layout variant of renderInto (v1.1.0 mono probes): the block carries
    `bufCh` channels, `input` fills the first `inCh`; any remaining channels
    are junk-filled with a constant the processor MUST ignore (mono->stereo
    clears then copies ch0 over ch1). */
void renderIntoLayout (OBitrotAudioProcessor& proc, juce::AudioBuffer<float>& dest,
                       int totalSamples, const std::vector<int>& sizes, InputFn input,
                       int inCh, int bufCh)
{
    juce::MidiBuffer midi;

    dest.setSize (bufCh, totalSamples);
    dest.clear();

    juce::AudioBuffer<float> scratch (bufCh, kMaxBlock);

    int    n  = 0;
    size_t si = 0;

    while (n < totalSamples)
    {
        int chunk = sizes[si % sizes.size()];
        ++si;
        chunk = juce::jmin (chunk, totalSamples - n);
        if (chunk <= 0)
            chunk = 1;

        juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), bufCh, chunk);

        for (int ch = 0; ch < bufCh; ++ch)
            for (int s = 0; s < chunk; ++s)
                block.setSample (ch, s, ch < inCh ? input (ch, n + s) : 0.777f);

        proc.processBlock (block, midi);

        for (int ch = 0; ch < bufCh; ++ch)
            dest.copyFrom (ch, n, block, ch, 0, chunk);

        n += chunk;
    }
}

/** memcmp, NOT a tolerance — QUAL-02 says bit-identical. */
bool bitIdentical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
        return false;

    const auto bytes = sizeof (float) * static_cast<size_t> (a.getNumSamples());

    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), bytes) != 0)
            return false;

    return true;
}

/** Index of the first sample that differs, for a readable diagnostic. */
juce::String firstDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int n = 0; n < a.getNumSamples(); ++n)
            if (! bitExact (a.getSample (ch, n), b.getSample (ch, n)))
                return juce::String ("first diff ch") + juce::String (ch) + " @" + juce::String (n)
                     + " (" + juce::String (a.getSample (ch, n), 9) + " vs "
                     + juce::String (b.getSample (ch, n), 9) + ")";

    return "identical";
}

double maxAbsDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    double m = 0.0;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int n = 0; n < a.getNumSamples(); ++n)
            m = juce::jmax (m, std::abs ((double) a.getSample (ch, n) - (double) b.getSample (ch, n)));
    return m;
}

//==============================================================================
/** Fundamental frequency via normalized autocorrelation over [off, off+len),
    searched in [fLo, fHi] Hz (from O-simpleGrain's harness). Returns 0 if no
    lag clears `minCorr`. */
double autocorrPitchHz (const std::vector<float>& x, int off, int len,
                        double fs, double fLo, double fHi, double minCorr = 0.3)
{
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

/** Tracked pitch trace: window 1024, hop 256 (window MUST stay well under the
    150 ms TAPE_RAMP — pattern_metric_window_vs_modulation_period). The lag
    search is constrained around the previous estimate to dodge the
    half-period octave latch. Invalid hops report 0. */
std::vector<double> pitchTrace (const std::vector<float>& x, double seedHz)
{
    constexpr int win = 1024;
    constexpr int hop = 256;

    std::vector<double> trace;
    double prev = seedHz;

    for (int off = 0; off + win <= (int) x.size(); off += hop)
    {
        const double lo = juce::jmax (60.0,   prev * 0.75);
        const double hi = juce::jmin (1000.0, prev * 1.33);
        const double f  = autocorrPitchHz (x, off, win, kFs, lo, hi, 0.3);
        trace.push_back (f);
        if (f > 0.0)
            prev = f;
    }
    return trace;
}

std::vector<float> channelToVector (const juce::AudioBuffer<float>& b, int ch)
{
    const auto* p = b.getReadPointer (ch);
    return std::vector<float> (p, p + b.getNumSamples());
}

//==============================================================================
// ── Phase 2.2 helpers: position-marker saw + jump-event scanner ──────────────

/** Read head's maximum lag, in samples at kFs — DERIVED from the plugin's own
    ring constants rather than mirrored, so the two cannot drift apart
    (pattern_test_fixture_mirrors_drift_silently). */
constexpr int kMaxLagSamples = (int) ((CaptureRing::kRingSeconds
                                       - CaptureRing::kSafetySeconds) * kFs);

/** Saw period, samples. Power of two so the marker value k/period is EXACT in
    float, and > 2x the read head's maximum lag so signed jump distances never
    alias.

    That second property was stated in this comment but never enforced, and
    v1.3.0's 2.5 s -> 10 s ring quietly broke it: maxLag went 120000 ->
    475200, so the old 262144 could no longer represent a backward jump
    measured from a deeply lagged head — distances wrapped into
    (-131072, 131072] and were misread or discarded as noise, which read as
    "vinyl stopped jumping." The asserts below are the invariant. */
constexpr int kSawPeriod = 1048576;   // 2^20
static_assert ((kSawPeriod & (kSawPeriod - 1)) == 0,
               "sawAt masks with kSawPeriod - 1, so the period must be a power of two");
static_assert (kSawPeriod > 2 * kMaxLagSamples,
               "Saw marker period must exceed 2x the read head's max lag, or signed "
               "jump distances alias — raise it if the capture ring grows again");
static_assert (kSawPeriod <= (1 << 24),
               "Marker values k/kSawPeriod must stay exact in a 24-bit float mantissa");

/** Per-sample detection threshold for a jump of `jumpSamples` in the saw
    marker: the value step is jumpSamples/kSawPeriod, smeared over the
    ReadHead's ~3 ms crossfade, and the scan trips at 40% of that rate.

    DERIVED from kSawPeriod for the same reason as the period itself — the
    literals this replaces (5e-5 for the CD loop, 1e-3 for a vinyl
    revolution) were tuned against the old 262144 and went 4x too coarse to
    see any jump at all once the period grew. Headroom is ample: the saw is
    exact in float and Catmull-Rom reproduces a linear ramp exactly, so the
    between-jump residual sits around 1e-7. */
double sawJumpThresh (double jumpSamples) noexcept
{
    constexpr double kFadeSamples = 0.003 * kFs;   // ReadHead::fadeLenSamples
    return 0.4 * (jumpSamples / (double) kSawPeriod) / kFadeSamples;
}

/** Position-marker saw: value encodes the ABSOLUTE input sample index, so a
    read-head jump of d samples shows up as a value step of d/kSawPeriod
    (mod 1). Linear, so the head's fractional-position lerp is transparent —
    values stay ON the saw line between jumps. Requires n >= 0. */
float sawAt (int n) noexcept
{
    return static_cast<float> (n & (kSawPeriod - 1)) / static_cast<float> (kSawPeriod);
}

float sawStereo (int ch, int n) noexcept
{
    juce::ignoreUnused (ch);
    return sawAt (n);
}

struct SawEvent
{
    int    outIndex;      // output-time sample where the deviation started
    double distSamples;   // + = jumped BACKWARD by dist; - = jumped FORWARD
    int    postIndex;     // where vPost was read (past fades/chirps)
    float  vPost;
};

/** Scans channel 0 of a saw-marker render for read-head jumps. A jump is a
    per-sample delta deviating from the nominal slope by > `thresh`; the
    distance is recovered from the value `skipAfter` samples later (past the
    crossfade and any restart chirp), mod the saw period, mapped into
    (-period/2, period/2]. |dist| <= 100 events (the saw's own wrap maps to
    ~0) are discarded. */
std::vector<SawEvent> scanSawEvents (const std::vector<float>& out,
                                     int startAt, int skipAfter, double thresh)
{
    std::vector<SawEvent> events;
    const double slope = 1.0 / static_cast<double> (kSawPeriod);

    for (int n = startAt; n - 1 + skipAfter < (int) out.size(); ++n)
    {
        const double d = (double) out[(size_t) n] - (double) out[(size_t) (n - 1)];

        if (std::abs (d - slope) > thresh)
        {
            const int   postIndex = n - 1 + skipAfter;
            const float vPre      = out[(size_t) (n - 1)];
            const float vPost     = out[(size_t) postIndex];

            double delta = ((double) vPre + skipAfter * slope) - (double) vPost;
            delta -= std::floor (delta);              // mod 1 -> [0, 1)
            if (delta > 0.5)
                delta -= 1.0;                         // -> (-0.5, 0.5]

            const double dist = delta * (double) kSawPeriod;

            if (std::abs (dist) > 100.0)
                events.push_back ({ n, dist, postIndex, vPost });

            n += skipAfter + 32;                      // continue past the smear
        }
    }
    return events;
}

/** Read-head lag (samples) recovered from a saw-marker value at output index
    n: live material would read sawAt(n - kComp). */
double lagFromSaw (float v, int outIndex, int kComp)
{
    double delta = (double) sawAt (outIndex - kComp) - (double) v;
    delta -= std::floor (delta);
    return delta * (double) kSawPeriod;
}

double medianOf (std::vector<double> v)
{
    if (v.empty())
        return 0.0;
    std::sort (v.begin(), v.end());
    return v[v.size() / 2];
}

/** The loop-window length CDSkip will actually use at kFs for a given
    CD_SEGMENT, above its sector-lock severity (v1.6.0, brief item 14c).
    DERIVED from the plugin's own constants rather than mirrored, so a change
    to either cannot leave a stale literal passing here
    (pattern_test_fixture_mirrors_drift_silently). */
juce::int64 sectorQuantised (double segmentMs) noexcept
{
    const double sector = kFs / CDSkip::kSectorHz;
    const auto   raw    = (juce::int64) juce::roundToIntAccurate (segmentMs * 0.001 * kFs);
    const auto   n      = juce::jmax ((juce::int64) 1,
                                      (juce::int64) juce::roundToIntAccurate ((double) raw / sector));

    return (juce::int64) juce::roundToIntAccurate ((double) n * sector);
}

struct ZeroRun { int start = -1; int length = 0; };

/** Every run of hard-zero output at least `minLen` long inside [from, to), in
    order. The CD mute rung and the servo seek are the only things in this
    engine that take the output to exact silence, which is what makes the seek
    measurable at all — and at severity 1.0 the rung roll never reaches the
    mute, so there the two cannot be confused. */
std::vector<ZeroRun> zeroRuns (const std::vector<float>& v, int from, int to, int minLen)
{
    std::vector<ZeroRun> runs;
    int                  runStart = -1;

    to = juce::jmin (to, (int) v.size());

    for (int n = juce::jmax (0, from); n < to; ++n)
    {
        const bool z    = std::abs ((double) v[(size_t) n]) < 1.0e-7;
        const bool last = (n == to - 1);

        if (z && runStart < 0)
            runStart = n;

        if (runStart >= 0 && (! z || last))
        {
            const int len = n - runStart + ((z && last) ? 1 : 0);
            if (len >= minLen)
                runs.push_back ({ runStart, len });
            runStart = -1;
        }
    }
    return runs;
}

//==============================================================================
// ── Phase 2.4 helpers ─────────────────────────────────────────────────────────

float dcStereo (int ch, int n) noexcept
{
    juce::ignoreUnused (ch, n);
    return 0.3f;
}

/** Burst/tail signal for the duck-vs-pump probe: 0.5 s loud (0.9), 0.5 s
    quiet (0.09 = -20 dB), 220 Hz carrier. Position-deterministic. */
float burstSine (int ch, int n) noexcept
{
    juce::ignoreUnused (ch);
    const float amp = ((n % 48000) < 24000) ? 0.9f : 0.09f;
    return amp * static_cast<float> (
               std::sin (2.0 * juce::MathConstants<double>::pi * kSineHz
                         * static_cast<double> (n) / kFs));
}

/** QUAL-01 pathological input: DC, silence, full-scale square, a NaN
    stretch, then clean sine — recovery is the criterion. */
float pathologicalStereo (int ch, int n) noexcept
{
    juce::ignoreUnused (ch);
    if (n < 24000) return 0.8f;                                   // DC
    if (n < 48000) return 0.0f;                                   // silence
    if (n < 72000) return ((n / 120) & 1) ? -1.0f : 1.0f;         // FS square, ~200 Hz
    if (n < 76800) return std::numeric_limits<float>::quiet_NaN();// 100 ms of NaN
    return 0.5f * static_cast<float> (
               std::sin (2.0 * juce::MathConstants<double>::pi * kSineHz
                         * static_cast<double> (n - 76800) / kFs));
}

/** renderInto with ONE parameter step applied at the first block boundary at
    or after `stepAt` (fixed chunk size). */
void renderWithStep (OBitrotAudioProcessor& proc, juce::AudioBuffer<float>& dest,
                     int totalSamples, int chunk, InputFn input,
                     int stepAt, const char* id, float value)
{
    juce::MidiBuffer midi;

    dest.setSize (2, totalSamples);
    dest.clear();

    juce::AudioBuffer<float> scratch (2, kMaxBlock);

    int  n       = 0;
    bool stepped = false;

    while (n < totalSamples)
    {
        if (! stepped && n >= stepAt)
        {
            setParam (proc, id, value);
            stepped = true;
        }

        const int len = juce::jmin (chunk, totalSamples - n);
        juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, len);

        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < len; ++s)
                block.setSample (ch, s, input (ch, n + s));

        proc.processBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
            dest.copyFrom (ch, n, block, ch, 0, len);

        n += len;
    }
}

bool allFiniteRange (const float* x, int from, int to)
{
    for (int n = from; n < to; ++n)
        if (! std::isfinite (x[n]))
            return false;
    return true;
}

//==============================================================================
// ── Phase 2.5 helpers ─────────────────────────────────────────────────────────

float sineQuietStereo (int ch, int n) noexcept
{
    return 0.1f * sineStereo (ch, n);   // -20 dB versions share the waveform
}

/** Band energy (Hann-windowed, 16384-point juce::dsp::FFT) of x[off ..
    off+16384) between loHz and hiHz. */
double bandEnergy (const std::vector<float>& x, int off, double loHz, double hiHz)
{
    constexpr int order = 14;
    constexpr int N     = 1 << order;

    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> win ((size_t) N,
                                             juce::dsp::WindowingFunction<float>::hann);

    std::vector<float> buf ((size_t) (2 * N), 0.0f);
    for (int n = 0; n < N; ++n)
        buf[(size_t) n] = (off + n < (int) x.size()) ? x[(size_t) (off + n)] : 0.0f;

    win.multiplyWithWindowingTable (buf.data(), (size_t) N);
    fft.performRealOnlyForwardTransform (buf.data());

    const int kLo = juce::jmax (1,     (int) (loHz * N / kFs));
    const int kHi = juce::jmin (N / 2, (int) (hiHz * N / kFs));

    double e = 0.0;
    for (int k = kLo; k <= kHi; ++k)
    {
        const double re = buf[(size_t) (2 * k)];
        const double im = buf[(size_t) (2 * k + 1)];
        e += re * re + im * im;
    }
    return e;
}

/** Peak lag of the normalized cross-correlation of a vs b over
    [off, off+len), lags in [-maxLag, maxLag]. Positive lag = b is LATER.
    Caller must keep off-maxLag and off+len+maxLag in range. */
int xcorrPeakLag (const std::vector<float>& a, const std::vector<float>& b,
                  int off, int len, int maxLag, double& corrOut)
{
    double ea = 0.0, eb = 0.0;
    for (int n = 0; n < len; ++n)
    {
        ea += (double) a[(size_t) (off + n)] * a[(size_t) (off + n)];
        eb += (double) b[(size_t) (off + n)] * b[(size_t) (off + n)];
    }
    const double denom = std::sqrt (ea * eb) + 1.0e-12;

    double best = -2.0;
    int bestLag = 0;

    for (int lag = -maxLag; lag <= maxLag; ++lag)
    {
        double acc = 0.0;
        for (int n = 0; n < len; ++n)
            acc += (double) a[(size_t) (off + n)] * b[(size_t) (off + n + lag)];

        const double c = acc / denom;
        if (c > best)
        {
            best    = c;
            bestLag = lag;
        }
    }

    corrOut = best;
    return bestLag;
}

//==============================================================================
/** First INPUT-TIME sample (>= startAt in output time) where channel 0
    deviates from the expected delayed-sine passthrough by more than `thresh`.
    -1 if none. The all-off / pre-event path is EXACT, so this locates the
    first transport event. A tick at input sample T emerges in the output at
    T + kComp (the compensated wet-path latency), so the raw output index is
    mapped back to input time before returning — probe expectations are
    written on the input/tick timeline. */
int firstDeviation (const juce::AudioBuffer<float>& out, int kComp, int startAt, float thresh)
{
    const auto* p = out.getReadPointer (0);
    for (int n = startAt; n < out.getNumSamples(); ++n)
    {
        const float expected = (n >= kComp) ? sineStereo (0, n - kComp) : 0.0f;
        if (std::abs (p[n] - expected) > thresh)
            return n - kComp;
    }
    return -1;
}

//==============================================================================
/** Mock host playhead for the sync-mode probes. The harness sets `ppq`
    before every processBlock call, computed from the absolute sample
    position, so tick times are consistent regardless of block chopping. */
struct MockPlayHead : public juce::AudioPlayHead
{
    double bpm     = 120.0;
    double ppq     = 0.0;
    bool   playing = true;

    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
    {
        juce::AudioPlayHead::PositionInfo info;
        info.setBpm (bpm);
        info.setPpqPosition (ppq);
        info.setIsPlaying (playing);
        return info;
    }
};

/** Fresh, prepared, baselined processor. */
std::unique_ptr<OBitrotAudioProcessor> makeProc()
{
    auto p = std::make_unique<OBitrotAudioProcessor>();
    p->setPlayConfigDetails (2, 2, kFs, kMaxBlock);
    p->prepareToPlay (kFs, kMaxBlock);
    setBaseline (*p);
    return p;
}

/** As makeProc but on a non-stereo main-bus layout (v1.1.0 mono probes). */
std::unique_ptr<OBitrotAudioProcessor> makeProcLayout (int inCh, int outCh)
{
    auto p = std::make_unique<OBitrotAudioProcessor>();
    p->setPlayConfigDetails (inCh, outCh, kFs, kMaxBlock);
    p->prepareToPlay (kFs, kMaxBlock);
    setBaseline (*p);
    return p;
}

//==============================================================================
// ── v1.4.0 helpers: sub-sample frequency tracking + a cross-version checksum ──

/** Mean frequency over [from, from+len) from LINEARLY INTERPOLATED upward zero
    crossings. Used instead of pitchTrace for the wow bed because the bed's
    whole deviation budget is 2% — at 220 Hz that is 4.4 samples of period, and
    an integer-lag autocorrelation resolves 1 sample (0.46%), so most of the
    quantity under test would land inside the metric's own step. Interpolated
    crossings measure it to ~0.01%.

    Returns 0 when the window holds fewer than two full cycles. */
double meanFreqZeroCross (const float* x, int from, int len)
{
    double firstT = -1.0, lastT = -1.0;
    int    cycles = 0;

    for (int n = from + 1; n < from + len; ++n)
    {
        if (x[n - 1] <= 0.0f && x[n] > 0.0f)
        {
            const double d    = (double) x[n] - (double) x[n - 1];
            const double frac = d > 0.0 ? (double) (-x[n - 1]) / d : 0.0;
            const double t    = (double) (n - 1) + frac;

            if (firstT < 0.0)
            {
                firstT = t;
            }
            else
            {
                lastT = t;
                ++cycles;
            }
        }
    }

    if (cycles < 2 || lastT <= firstT)
        return 0.0;

    return kFs * (double) cycles / (lastT - firstT);
}

/** FNV-1a over the OBJECT REPRESENTATION of every sample. A single flipped
    mantissa bit anywhere in the render changes the digest, which is the point:
    it is the only way to state "bit-identical to the previous version" in a
    harness that cannot link the previous version. */
juce::uint64 renderChecksum (const juce::AudioBuffer<float>& b)
{
    juce::uint64 h = 14695981039346656037ULL;

    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        const auto* p = b.getReadPointer (ch);

        for (int n = 0; n < b.getNumSamples(); ++n)
        {
            juce::uint32 bits = 0;
            std::memcpy (&bits, &p[n], sizeof (bits));

            for (int k = 0; k < 4; ++k)
            {
                h ^= (juce::uint64) ((bits >> (8 * k)) & 0xFFu);
                h *= 1099511628211ULL;
            }
        }
    }

    return h;
}

/** The canonical cross-version render: forced tape BENDS (stop share 0, so the
    v1.4.0 stop-gain law is out of the picture by construction) over CD and
    vinyl at their defaults, both v1.4.0 additions at their transparent 0.
    Every byte of this must survive the v1.3.0 -> v1.4.0 upgrade. */
void configureCanonicalRender (OBitrotAudioProcessor& proc)
{
    setBaseline (proc);
    setParam (proc, "CLOCK_MODE",      1.0f);    // Free
    setParam (proc, "CLOCK_FREE_RATE", 4.0f);
    setParam (proc, "SEED",            4242.0f);
    setParam (proc, "TAPE_PROB",       100.0f);
    setParam (proc, "TAPE_STOP_PROB",  0.0f);    // no stops: see above
    setParam (proc, "CD_PROB",         60.0f);
    setParam (proc, "VINYL_PROB",      60.0f);
}

/** Canonical render #2 (added v1.5.0): the SERIAL POST-STAGES, which the
    v1.3.0 canonical render above leaves switched off entirely. The transport
    families are silent here so the only things under test are the packet
    stage (which v1.5.0 threads comfort noise through) and the codec stage
    (which v1.5.0 now injects a bed after).

    It deliberately touches NO v1.5.0 parameter and relies on their defaults
    being transparent, which is exactly what makes it compile and run against
    the v1.4.0 tree — where the digest asserted in probe N7 came from. */
void configureCanonicalPostRender (OBitrotAudioProcessor& proc)
{
    setBaseline (proc);
    setParam (proc, "CLOCK_MODE",      1.0f);    // Free
    setParam (proc, "CLOCK_FREE_RATE", 3.0f);
    setParam (proc, "SEED",            777.0f);
    setParam (proc, "TAPE_ENABLE",     0.0f);
    setParam (proc, "CD_ENABLE",       0.0f);
    setParam (proc, "VINYL_ENABLE",    0.0f);
    setParam (proc, "PACKET_ENABLE",   1.0f);
    setParam (proc, "PACKET_LOSS",     45.0f);
    setParam (proc, "PACKET_BURST",    70.0f);
    setParam (proc, "PACKET_CONCEAL",  2.0f);    // Decay
    setParam (proc, "CODEC_ENABLE",    1.0f);
    setParam (proc, "CODEC_MODE",      0.0f);    // Mu-law
    setParam (proc, "CODEC_MIX",       100.0f);
}

/** Dual-mono input: same signal on every channel, so a (1,1) run and a (2,2)
    run see literally identical per-sample engine inputs. */
float noiseDualMono (int ch, int n) noexcept
{
    juce::ignoreUnused (ch);
    return noiseAt (n);
}

//==============================================================================
// ── v1.5.0 helpers: the media-noise beds are measured, not eyeballed ─────────

/** Digital silence. The beds are the ONLY thing in the output under this
    input, so their levels can be read directly rather than separated from a
    programme. */
float silentInput (int ch, int n) noexcept
{
    juce::ignoreUnused (ch, n);
    return 0.0f;
}

/** Fresh processor at an arbitrary sample rate — the beds claim sample-rate
    invariance, and that claim needs a second rate to be worth anything. */
std::unique_ptr<OBitrotAudioProcessor> makeProcAtRate (double fs)
{
    auto p = std::make_unique<OBitrotAudioProcessor>();
    p->setPlayConfigDetails (2, 2, fs, kMaxBlock);
    p->prepareToPlay (fs, kMaxBlock);
    setBaseline (*p);
    return p;
}

double rmsOf (const juce::AudioBuffer<float>& b, int ch, int from, int len)
{
    const auto* p = b.getReadPointer (ch);
    double acc = 0.0;

    for (int n = from; n < from + len; ++n)
        acc += (double) p[n] * (double) p[n];

    return std::sqrt (acc / (double) juce::jmax (1, len));
}

/** RMS after two one-pole lowpasses at `hz`, and RMS of the complement.

    Two one-poles are not a brick wall, but the split only has to separate
    55 Hz rumble from 2-6 kHz ticks — six octaves at -12 dB/oct is 72 dB of
    rejection, so each band reads its own contributor and nothing else. */
void splitBandRms (const juce::AudioBuffer<float>& b, int ch, int from, int len,
                   double hz, double fs, double& lowRms, double& highRms,
                   double& highPeak)
{
    const auto* p = b.getReadPointer (ch);
    const double a = 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * hz / fs);

    double lp1 = 0.0, lp2 = 0.0, accLow = 0.0, accHigh = 0.0, peak = 0.0;

    for (int n = from; n < from + len; ++n)
    {
        const double x = (double) p[n];
        lp1 += a * (x   - lp1);
        lp2 += a * (lp1 - lp2);

        const double hi = x - lp2;

        accLow  += lp2 * lp2;
        accHigh += hi * hi;
        peak     = juce::jmax (peak, std::abs (hi));
    }

    const double inv = 1.0 / (double) juce::jmax (1, len);
    lowRms   = std::sqrt (accLow  * inv);
    highRms  = std::sqrt (accHigh * inv);
    highPeak = peak;
}

/** Amplitude of a single frequency by Goertzel. Callers pick `len` so the
    frequency lands on an exact bin (len = fs/2 gives 2 Hz spacing, which is
    integral for 50, 60 and all their harmonics here) — off-bin leakage would
    otherwise be indistinguishable from the partial being absent. */
double toneAmplitude (const juce::AudioBuffer<float>& b, int ch, int from, int len,
                      double hz, double fs)
{
    const auto*  p = b.getReadPointer (ch);
    const double w = 2.0 * juce::MathConstants<double>::pi * hz / fs;
    const double c = 2.0 * std::cos (w);

    double s1 = 0.0, s2 = 0.0;

    for (int n = from; n < from + len; ++n)
    {
        const double s = (double) p[n] + c * s1 - s2;
        s2 = s1;
        s1 = s;
    }

    const double re = s1 - s2 * std::cos (w);
    const double im = s2 * std::sin (w);

    return 2.0 * std::sqrt (re * re + im * im) / (double) juce::jmax (1, len);
}

/** Normalised L/R correlation. Tape hiss is two tracks and must NOT be a
    centred mono buzz. */
double channelCorrelation (const juce::AudioBuffer<float>& b, int from, int len)
{
    const auto* l = b.getReadPointer (0);
    const auto* r = b.getReadPointer (1);

    double sxy = 0.0, sxx = 0.0, syy = 0.0;

    for (int n = from; n < from + len; ++n)
    {
        sxy += (double) l[n] * (double) r[n];
        sxx += (double) l[n] * (double) l[n];
        syy += (double) r[n] * (double) r[n];
    }

    const double denom = std::sqrt (sxx * syy);
    return denom > 0.0 ? sxy / denom : 0.0;
}

double toDb (double linear)
{
    return 20.0 * std::log10 (juce::jmax (1.0e-12, linear));
}

} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const int kComp = (int) std::ceil (0.020 * kFs);   // 960 @ 48 kHz
    std::printf ("O-Bitrot render-harness (Phases 2.1-2.5) — fs=%.0f, kCompLatency=%d\n", kFs, kComp);

    //==========================================================================
    // A — latency reported once, equal to ceil(0.020 * fs).
    {
        auto p = makeProc();
        const int reported = p->getLatencySamples();
        check ("A latency-reported", reported == kComp,
               juce::String ("getLatencySamples()=") + juce::String (reported)
                   + " expected " + juce::String (kComp));
    }

    //==========================================================================
    // B — FUNC-02 delay-compensated null. All families off: the wet path is
    // the ring at rate 1.0 + the plain integer delay => out[n] bit-equals
    // in[n - kComp]. No tolerance. Skips a warmup for the DryWetMixer gain
    // smoothing to settle exactly on its targets.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",  0.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 0.0f);

        const int total = 48000;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const int  startAt = kComp + (int) (0.2 * kFs);
        bool       ok      = true;
        juce::String detail;

        for (int ch = 0; ch < 2 && ok; ++ch)
        {
            const auto* o = out.getReadPointer (ch);
            for (int n = startAt; n < total; ++n)
            {
                if (! bitExact (o[n], noiseStereo (ch, n - kComp)))
                {
                    ok = false;
                    detail = juce::String ("first mismatch ch") + juce::String (ch)
                           + " @" + juce::String (n)
                           + " (" + juce::String (o[n], 9) + " vs "
                           + juce::String (noiseStereo (ch, n - kComp), 9) + ")";
                    break;
                }
            }
        }

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;
        if (ok && live)
            detail = juce::String ("bit-exact null over [")
                   + juce::String (startAt) + ", " + juce::String (total) + ")";

        check ("B FUNC-02 null", ok && live,
               detail + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // C — DSP-01 bends: forced tape bends on a sine. The tracked pitch trace
    // must move (a down-bend lands below 200 Hz) and must never STEP between
    // hops (the 150 ms linear ramp bounds the per-hop ratio well under 1.20).
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 4.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto mono  = channelToVector (out, 0);
        const auto trace = pitchTrace (mono, kSineHz);

        const int hopStart = (14400 - 0) / 256;   // evaluate past warmup (0.3 s)

        int    valid = 0, evaluated = 0;
        double minF = 1.0e9, maxF = 0.0, worstRatio = 1.0;
        double prevF = 0.0;

        for (int i = hopStart; i < (int) trace.size(); ++i)
        {
            ++evaluated;
            const double f = trace[(size_t) i];
            if (f <= 0.0) { prevF = 0.0; continue; }

            ++valid;
            minF = juce::jmin (minF, f);
            maxF = juce::jmax (maxF, f);

            if (prevF > 0.0)
            {
                const double ratio = juce::jmax (f, prevF) / juce::jmin (f, prevF);
                worstRatio = juce::jmax (worstRatio, ratio);
            }
            prevF = f;
        }

        const bool live       = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool bendsLive  = minF < 200.0;                       // a down-bend was tracked
        const bool continuous = worstRatio < 1.20;                  // ramps, never steps
        // Fast ramps smear the autocorr window (corr can dip below 0.3 mid
        // chirp), so require a third of hops valid rather than a majority.
        const bool trackable  = evaluated > 0 && valid > evaluated / 3;

        check ("C DSP-01 bend-ramps", live && bendsLive && continuous && trackable,
               juce::String ("pitch [") + juce::String (minF, 1) + ", " + juce::String (maxF, 1)
                   + "] Hz, worst hop ratio " + juce::String (worstRatio, 4)
                   + " (bound 1.20), valid " + juce::String (valid) + "/" + juce::String (evaluated)
                   + (bendsLive ? "" : " — NO DOWN-BEND TRACKED, probe vacuous")
                   + (live ? "" : " — SILENT"));
    }

    //==========================================================================
    // C2 — v1.3.0 item 12: the read head's fractional interpolator. Unit-level
    // on CaptureRing, because the quantity under test is the interpolator's
    // own frequency response and any transport around it only obscures it.
    //
    // A mid-sample read (frac = 0.5) is the interpolator's worst case. For a
    // sine at frequency f the retained amplitude is:
    //
    //   2-point lerp   |cos(w/2)|                                  w = 2*pi*f/fs
    //   Catmull-Rom    |1.125*cos(w/2) - 0.125*cos(3w/2)|
    //
    // At f = 0.4*fs that is 0.309 (-10.2 dB) against 0.449 (-7.0 dB). The
    // 0.40 bound sits between them, so this probe FAILS against the lerp this
    // version replaces — it is a gate, not a decoration. Tape bends run the
    // whole melodic voice through this path (rates 0.5-2.0), which is why the
    // loss was worth 2 extra ring reads and a few FMAs.
    //
    // Also pins the exact-integer fast path bit-for-bit: FUNC-02's end-to-end
    // nulls (B / M1 / M3) depend on frac <= 0 never touching the polynomial,
    // and this states that requirement where the code is.
    {
        constexpr double kProbeHz  = 0.4 * kFs;
        constexpr int    kPushLen  = 8192;
        constexpr double w         = 2.0 * juce::MathConstants<double>::pi * kProbeHz / kFs;

        CaptureRing ring;
        ring.prepare (kFs);

        for (int n = 0; n < kPushLen; ++n)
        {
            const auto v = (float) std::sin (w * n);
            ring.push (v, v);
        }

        // Exact-integer reads must be the stored sample, bit-for-bit.
        bool fastPathExact = true;
        for (int n = 1024; n < kPushLen - 8; ++n)
            if (! bitExact (ring.readFrac (0, (double) n), ring.readAbs (0, n)))
            {
                fastPathExact = false;
                break;
            }

        // Mid-sample retention: peak of the interpolated sequence against the
        // unit-amplitude source. Kept clear of both ring ends so neither the
        // write-head clamp nor pre-history is in play.
        double peakMid = 0.0;
        for (int n = 1024; n < kPushLen - 8; ++n)
            peakMid = juce::jmax (peakMid, std::abs ((double) ring.readFrac (0, n + 0.5)));

        const bool retains = peakMid >= 0.40 && peakMid <= 1.0;

        check ("C2 item-12 catmull-rom", fastPathExact && retains,
               juce::String ("mid-sample retention at 0.4*fs = ") + juce::String (peakMid, 4)
                   + " (bound [0.40, 1.0]; lerp scores 0.309, Catmull-Rom 0.449)"
                   + (fastPathExact ? ", exact-integer path bit-exact"
                                    : " — EXACT-INTEGER FAST PATH BROKEN, FUNC-02 at risk"));
    }

    //==========================================================================
    // D — DSP-01 stops: forced tape stops. No click anywhere (sample-to-sample
    // delta bounded by ~2x the sine's own max derivative) and a genuine hold:
    // rate 0.0 exactly => a long run of bit-identical output samples.
    //
    // v1.4.0: that run is now a run of ZEROS rather than of a held sample, so
    // this probe alone no longer discriminates a working stop from a silenced
    // one — `live` is its only negative control here. S1 and S2 below are what
    // actually pin the stop's amplitude behaviour; this probe keeps its
    // original job, which is proving the TRANSPORT holds and does not click.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 1.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  100.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (3.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);

        // Max sample-to-sample delta after warmup. Sine max derivative
        // ~ amp * 2*pi*f / fs ~= 0.0144; bound 0.03 allows rate <= ~2x.
        const int    startAt  = kComp + (int) (0.2 * kFs);
        double       maxDelta = 0.0;
        for (int n = startAt + 1; n < total; ++n)
            maxDelta = juce::jmax (maxDelta, std::abs ((double) o[n] - (double) o[n - 1]));

        // Longest run of consecutive bit-identical samples (the DC hold).
        int longestRun = 0, run = 0;
        for (int n = startAt + 1; n < total; ++n)
        {
            run = bitExact (o[n], o[n - 1]) ? run + 1 : 0;
            longestRun = juce::jmax (longestRun, run);
        }

        const bool live    = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool noClick = maxDelta <= 0.03;
        const bool held    = longestRun >= 2000;   // stop actually engaged

        check ("D DSP-01 stop-no-click", live && noClick && held,
               juce::String ("maxDelta=") + juce::String (maxDelta, 5)
                   + " (bound 0.03), longest hold run " + juce::String (longestRun)
                   + " samples (need >= 2000)"
                   + (held ? "" : " — STOP NEVER ENGAGED, probe vacuous")
                   + (live ? "" : " — SILENT"));
    }

    //==========================================================================
    // E — FUNC-04 seeded determinism on FRESH instances.
    {
        auto configure = [] (OBitrotAudioProcessor& proc, int seed)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 4.0f);
            setParam (proc, "TAPE_PROB",       100.0f);
            setParam (proc, "SEED",            (float) seed);
        };

        const int total = 96000;

        auto a = makeProc();  configure (*a, 1234);
        auto b = makeProc();  configure (*b, 1234);
        auto c = makeProc();  configure (*c, 4321);

        juce::AudioBuffer<float> outA, outB, outC;
        renderInto (*a, outA, total, { 512 }, sineStereo);
        renderInto (*b, outB, total, { 512 }, sineStereo);
        renderInto (*c, outC, total, { 512 }, sineStereo);

        const double diffSame = maxAbsDiff (outA, outB);
        const bool   live     = outA.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool   sameOk   = diffSame <= 0.0 && live;

        check ("E FUNC-04 same-seed", sameOk,
               juce::String ("maxAbsDiff=") + juce::String (diffSame, 9)
                   + " (must be 0.0)" + (live ? "" : " — SILENT, probe vacuous"));

        const bool differs = ! bitIdentical (outA, outC);
        check ("E FUNC-04 diff-seed", differs,
               differs ? juce::String ("seed 1234 vs 4321 renders differ")
                       : juce::String ("IDENTICAL — seed does not reach the DSP"));
    }

    //==========================================================================
    // F/G — QUAL-02 block-size invariance. Families on, fixed seed, free
    // clock: memcmp bit-identity across block-size regimes.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 10.0f);
            setParam (proc, "TAPE_PROB",       80.0f);
            setParam (proc, "SEED",            777.0f);
        };

        const int total = 32768;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);

        juce::AudioBuffer<float> outA, outB;
        renderInto (*a, outA, total, { 512 },  noiseStereo);
        renderInto (*b, outB, total, { 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outA, outB);
            const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("F QUAL-02 512-vs-4096", identical && live,
                   (identical ? juce::String ("bit-identical by memcmp over 32768 x 2ch")
                              : firstDifference (outA, outB))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }

        auto cproc = makeProc();  configure (*cproc);
        juce::AudioBuffer<float> outC;
        renderInto (*cproc, outC, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outB, outC);
            const bool live      = outC.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("G QUAL-02 ragged", identical && live,
                   (identical ? juce::String ("1,7,64,333,4096 vs 4096: bit-identical")
                              : firstDifference (outB, outC))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // H — FUNC-03 free mode: the first event onset lands at fs/rate. Stops
    // (share 100%) guarantee the first tick is audible regardless of which
    // interval the tape stream would have rolled.
    {
        const double rates[] = { 4.0, 2.5 };

        for (const double r : rates)
        {
            auto p = makeProc();
            setParam (*p, "CLOCK_MODE",      1.0f);
            setParam (*p, "CLOCK_FREE_RATE", (float) r);
            setParam (*p, "TAPE_PROB",       100.0f);
            setParam (*p, "TAPE_STOP_PROB",  100.0f);
            setParam (*p, "CD_ENABLE",       0.0f);
            setParam (*p, "VINYL_ENABLE",    0.0f);

            const int total = 48000;
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, sineStereo);

            const int expected = (int) std::ceil (kFs / r);
            const int startAt  = kComp + (int) (0.15 * kFs);
            const int onset    = firstDeviation (out, kComp, startAt, 1.0e-3f);

            const bool ok = onset >= expected - 2 && onset <= expected + 600;
            check ("H FUNC-03 free-rate", ok,
                   juce::String (r, 1) + " Hz: onset @" + juce::String (onset)
                       + " expected ~" + juce::String (expected) + " (+600 tol)");
        }
    }

    //==========================================================================
    // I — FUNC-03 sync mode: mock playhead, division 1/4. The first boundary
    // (ppq 1.0) lands at 60/bpm * fs samples — the grid follows the BPM.
    // Negative control: stopped transport never deviates (also proves the
    // deviation detector is not firing spuriously).
    {
        auto renderSync = [&] (double bpm, bool playing, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "CLOCK_MODE",     0.0f);   // Sync
            setParam (*p, "CLOCK_SYNC_DIV", 4.0f);   // 1/4 (ppq 1.0)
            setParam (*p, "TAPE_PROB",      100.0f);
            setParam (*p, "TAPE_STOP_PROB", 100.0f);
            setParam (*p, "CD_ENABLE",      0.0f);
            setParam (*p, "VINYL_ENABLE",   0.0f);

            MockPlayHead mock;
            mock.bpm     = bpm;
            mock.playing = playing;
            p->setPlayHead (&mock);

            juce::MidiBuffer midi;
            out.setSize (2, total);
            out.clear();

            juce::AudioBuffer<float> scratch (2, 512);
            const double pps = bpm / (60.0 * kFs);

            int n = 0;
            while (n < total)
            {
                const int chunk = juce::jmin (512, total - n);
                juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, chunk);

                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < chunk; ++s)
                        block.setSample (ch, s, sineStereo (ch, n + s));

                mock.ppq = n * pps;   // ppq from the ABSOLUTE sample position
                p->processBlock (block, midi);

                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, n, block, ch, 0, chunk);

                n += chunk;
            }

            p->setPlayHead (nullptr);
        };

        const int startAt = kComp + (int) (0.15 * kFs);

        {
            juce::AudioBuffer<float> out;
            renderSync (120.0, true, out, 36000);
            const int expected = 24000;               // 1 quarter @ 120 BPM
            const int onset    = firstDeviation (out, kComp, startAt, 1.0e-3f);
            check ("I FUNC-03 sync-120bpm", onset >= expected - 2 && onset <= expected + 600,
                   juce::String ("onset @") + juce::String (onset)
                       + " expected ~" + juce::String (expected) + " (+600 tol)");
        }

        {
            juce::AudioBuffer<float> out;
            renderSync (240.0, true, out, 24000);
            const int expected = 12000;               // 1 quarter @ 240 BPM
            const int onset    = firstDeviation (out, kComp, startAt, 1.0e-3f);
            check ("I FUNC-03 sync-240bpm", onset >= expected - 2 && onset <= expected + 600,
                   juce::String ("onset @") + juce::String (onset)
                       + " expected ~" + juce::String (expected) + " (+600 tol)");
        }

        {
            // v1.2.1 (item 9): a STOPPED transport now falls back to the free
            // accumulator instead of going inert. renderSync leaves
            // CLOCK_FREE_RATE at the 2 Hz baseline, so the first free tick
            // lands at 0.5 s and the forced stop must deviate shortly after.
            juce::AudioBuffer<float> out;
            renderSync (120.0, false, out, 48000);
            const int onset    = firstDeviation (out, kComp, startAt, 1.0e-3f);
            const int expected = 24000;              // 1 free period @ 2 Hz
            check ("I FUNC-03 sync-stopped-free-runs",
                   onset >= expected - 2 && onset <= expected + 600,
                   onset == -1
                       ? juce::String ("INERT — stopped transport emitted no events")
                       : juce::String ("onset @") + juce::String (onset)
                             + " expected ~" + juce::String (expected) + " (+600 tol)");
        }

        {
            // Replacement NEGATIVE CONTROL for the deviation detector. The
            // old sync-stopped case used to serve this role; item 9 makes it
            // fire by design, so the detector is now proven sane against a
            // transport that is stopped AND has every family disabled — the
            // one configuration that must still be pure passthrough.
            auto p = makeProc();
            setParam (*p, "CLOCK_MODE",     0.0f);   // Sync
            setParam (*p, "CLOCK_SYNC_DIV", 4.0f);
            setParam (*p, "TAPE_ENABLE",    0.0f);
            setParam (*p, "CD_ENABLE",      0.0f);
            setParam (*p, "VINYL_ENABLE",   0.0f);

            MockPlayHead mock;
            mock.bpm     = 120.0;
            mock.playing = false;
            p->setPlayHead (&mock);

            juce::AudioBuffer<float> out;
            renderInto (*p, out, 48000, { 512 }, sineStereo);
            p->setPlayHead (nullptr);

            const int onset = firstDeviation (out, kComp, startAt, 1.0e-3f);
            check ("I FUNC-03 sync-stopped-all-off-silent", onset == -1,
                   onset == -1
                       ? juce::String ("no deviation with every family off (detector sane)")
                       : juce::String ("SPURIOUS deviation @") + juce::String (onset));
        }
    }

    //==========================================================================
    // J — DSP-02 rung 1 (concealment): severity 0 forces the LPF dip. On a
    // noise input the windowed HF/total energy ratio must dip well below its
    // median (dip liveness) while the level never collapses (severity 0 must
    // never mute — negative control for K).
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 4.0f);
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "CD_SEVERITY",     0.0f);
        setParam (*p, "SEED",            42.0f);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const auto* o = out.getReadPointer (0);
        const int startAt = kComp + (int) (0.2 * kFs);
        constexpr int win = 1024;

        std::vector<double> hfRatio, winRms;
        for (int w = startAt; w + win <= total; w += win)
        {
            double eTot = 0.0, eDiff = 0.0;
            for (int i = 1; i < win; ++i)
            {
                const double x = o[w + i];
                const double d = x - (double) o[w + i - 1];
                eTot  += x * x;
                eDiff += d * d;
            }
            hfRatio.push_back (eDiff / juce::jmax (1.0e-12, eTot));
            winRms.push_back (std::sqrt (eTot / (win - 1)));
        }

        const double medRatio = medianOf (hfRatio);
        const double minRatio = *std::min_element (hfRatio.begin(), hfRatio.end());
        const double medRms   = medianOf (winRms);
        const double minRms   = *std::min_element (winRms.begin(), winRms.end());

        const bool live    = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool dipped  = minRatio < 0.5 * medRatio;
        const bool noMute  = minRms > 0.1 * medRms;

        check ("J DSP-02 conceal-dip", live && dipped && noMute,
               juce::String ("HF ratio min/median ") + juce::String (minRatio, 4) + "/"
                   + juce::String (medRatio, 4) + " (need < 0.5x), RMS min/median "
                   + juce::String (minRms, 4) + "/" + juce::String (medRms, 4)
                   + (dipped ? "" : " — NO DIP, probe vacuous")
                   + (noMute ? "" : " — LEVEL COLLAPSED at severity 0"));
    }

    //==========================================================================
    // K — DSP-02 rung 2 (mute): severity 0.45 rolls mostly mutes. Some 5 ms
    // RMS window must collapse (hard mute engaged) while the median stays
    // healthy.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 4.0f);
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "CD_SEVERITY",     0.45f);
        setParam (*p, "SEED",            42.0f);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const auto* o = out.getReadPointer (0);
        const int startAt = kComp + (int) (0.2 * kFs);
        const int win     = (int) (0.005 * kFs);   // 5 ms

        std::vector<double> winRms;
        for (int w = startAt; w + win <= total; w += win)
        {
            double e = 0.0;
            for (int i = 0; i < win; ++i) { const double x = o[w + i]; e += x * x; }
            winRms.push_back (std::sqrt (e / win));
        }

        const double medRms = medianOf (winRms);
        const double minRms = *std::min_element (winRms.begin(), winRms.end());

        const bool live  = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool muted = minRms < 0.1 * medRms;

        check ("K DSP-02 mute-notch", live && muted,
               juce::String ("5ms RMS min/median ") + juce::String (minRms, 5) + "/"
                   + juce::String (medRms, 5) + " (need < 0.1x)"
                   + (muted ? "" : " — NO MUTE ENGAGED, probe vacuous"));
    }

    //==========================================================================
    // K2 — item 14b, the severity-scaled mute ceiling. The rung-1 duration was
    // pinned at 2-20 ms at every setting; a real E32 mute lengthens as the
    // disc worsens, and v1.6.0 scales the SPAN (not the 2 ms floor) to reach
    // 150 ms by the severity where the loop rung takes over.
    //
    // Two severities, and the assertion is the CEILING each one implies rather
    // than a measured constant: at 0.2 no mute may exceed 63 ms, at 0.6 the
    // longest must clear 100 ms — a length the pre-v1.6.0 engine could not
    // produce at ANY severity, which is what makes the probe discriminate. The
    // upper bound on the high render is asserted too, so a runaway scale is
    // caught as readily as a dead one.
    //
    // Both severities keep the mute rung reachable and the seek unreachable
    // (0.6 < kSeekSeverity), so a hard-zero run here is unambiguously a mute:
    // conceal is a filter dip and the loop rung is silent about level.
    {
        auto muteStats = [&] (float severity, int& count, int& longest)
        {
            auto p = makeProc();
            setParam (*p, "CLOCK_MODE",      1.0f);   // Free
            setParam (*p, "CLOCK_FREE_RATE", 4.0f);
            setParam (*p, "TAPE_ENABLE",     0.0f);
            setParam (*p, "VINYL_ENABLE",    0.0f);
            setParam (*p, "CD_PROB",         100.0f);
            setParam (*p, "CD_SEVERITY",     severity);
            setParam (*p, "SEED",            5.0f);

            const int total = (int) (16.0 * kFs);
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, noiseStereo);

            const auto mono = channelToVector (out, 0);
            const auto runs = zeroRuns (mono, kComp + (int) (0.2 * kFs), total, 48);

            count = (int) runs.size();
            longest = 0;
            for (const auto& r : runs)
                longest = juce::jmax (longest, r.length);
        };

        int loCount = 0, loLongest = 0, hiCount = 0, hiLongest = 0;
        muteStats (0.2f, loCount, loLongest);
        muteStats (0.6f, hiCount, hiLongest);

        // Ceilings straight off CDSkip's expression: 2 ms floor + span.
        const double ceilLoMs = 1000.0 * (0.002 + 0.018 + 0.130 * (0.2 / 0.6));   // 63.3
        const double ceilHiMs = 1000.0 * (0.002 + 0.018 + 0.130);                 // 150.0

        const double loMs = 1000.0 * loLongest / kFs;
        const double hiMs = 1000.0 * hiLongest / kFs;

        const bool enough  = loCount >= 8 && hiCount >= 8;
        const bool loUnder = loMs <= ceilLoMs;
        const bool hiOver  = hiMs > 100.0;          // impossible before v1.6.0 (20 ms cap)
        const bool hiUnder = hiMs <= ceilHiMs;

        check ("K2 item-14b mute-scales", enough && loUnder && hiOver && hiUnder,
               juce::String ("longest mute: severity 0.2 ") + juce::String (loMs, 1)
                   + " ms (ceiling " + juce::String (ceilLoMs, 1) + "), severity 0.6 "
                   + juce::String (hiMs, 1) + " ms (need > 100, ceiling "
                   + juce::String (ceilHiMs, 1) + ") over " + juce::String (loCount) + "/"
                   + juce::String (hiCount) + " mutes"
                   + (enough ? "" : " — TOO FEW MUTES, probe vacuous"));
    }

    //==========================================================================
    // L — DSP-02 rung 3 (buffer loop) on the position-marker saw: severity
    // 1.0 forces the loop. Restarts land at EXACT intervals with chirp energy
    // at each restart; the first onset sits on the clock grid (FUNC-01);
    // dropping CD_PROB to 0 releases the loop, which at this severity means
    // the v1.6.0 servo seek — a full mute, THEN the forward recovery jump back
    // to live material.
    //
    // Two v1.6.0 shifts land in this probe and both are asserted, not
    // tolerated. The interval is the SECTOR-QUANTISED window (5120, not the
    // free 4800 the knob asks for — item 14c), and the recovery no longer
    // happens at the release tick (item 18). The clock grid here is 23999 +
    // 24000k, one sample under the round numbers, so the CD_PROB write at
    // n = 96000 misses the 95999 tick and the release lands at 119999 — a
    // documented +/-1 slop in the free-clock accumulator, unchanged by this
    // release and the reason the old window ran to 120100.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 2.0f);   // ticks every 24000
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "CD_SEVERITY",     1.0f);
        setParam (*p, "CD_SEGMENT",      100.0f); // 4800 free -> 5120 sector-locked
        setParam (*p, "SEED",            42.0f);

        // The window CDSkip will actually loop, derived from its own sector
        // constant. The probe asserts THIS number: against v1.5.0 it measures
        // 4800 and every spacing check below fails.
        const int expSeg = (int) sectorQuantised (100.0);

        // 4 s, not 3 s: the servo seek pushes the recovery jump up to 400 ms
        // past the 119999 release tick, and the tail-tracks-live window has to
        // sit entirely after it.
        const int total = 192000;
        juce::AudioBuffer<float> out (2, total);
        out.clear();

        // Chunk 500 divides 24000, so the CD_PROB write at n == 96000 lands
        // exactly on the tick that must see it (release => recovery).
        {
            juce::MidiBuffer midi;
            juce::AudioBuffer<float> scratch (2, 512);
            int n = 0;
            bool dropped = false;
            while (n < total)
            {
                if (! dropped && n >= 96000)
                {
                    setParam (*p, "CD_PROB", 0.0f);
                    dropped = true;
                }

                const int chunk = juce::jmin (500, total - n);
                juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, chunk);
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < chunk; ++s)
                        block.setSample (ch, s, sawStereo (ch, n + s));
                p->processBlock (block, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, n, block, ch, 0, chunk);
                n += chunk;
            }
        }

        const auto mono = channelToVector (out, 0);
        const int  startAt = kComp + (int) (0.2 * kFs);

        // A 5120-sample loop jump is a far smaller marker step than a vinyl
        // revolution, so it gets its own threshold — a revolution-sized one
        // would ride on the chirp alone. skipAfter 600 clears the fade (24
        // samples now that wraps splice hard, 144 before) + chirp (384).
        const auto events = scanSawEvents (mono, startAt, 600, sawJumpThresh ((double) expSeg));

        // Classify BY DISTANCE, not by position: a wrap is a backward jump of
        // one segment, the recovery is the big forward one. Position alone
        // stopped working when the seek moved the recovery clear of the
        // release tick, and the muted stretch in between throws its own pair
        // of marker steps that are neither.
        std::vector<int>    wrapOnsets;    // input-time
        int recoveryOnset = -1; double recoveryDist = 0.0;
        for (const auto& e : events)
        {
            const int onset = e.outIndex - kComp;
            if (std::abs (e.distSamples - (double) expSeg) <= 8.0)
                wrapOnsets.push_back (onset);
            else if (e.distSamples < -1000.0 && recoveryOnset < 0)
            {
                recoveryOnset = onset;
                recoveryDist  = e.distSamples;
            }
        }

        bool spacingOk = wrapOnsets.size() >= 10;
        int  worstGap  = 0;
        for (size_t i = 1; i < wrapOnsets.size(); ++i)
        {
            const int gap = wrapOnsets[i] - wrapOnsets[i - 1];
            worstGap = juce::jmax (worstGap, std::abs (gap - expSeg));
            if (std::abs (gap - expSeg) > 8)
                spacingOk = false;
        }

        const bool firstOnGrid = ! wrapOnsets.empty()
                                 && std::abs (wrapOnsets.front() - 24000) <= 8;

        // Chirp liveness: 2nd-difference energy at each restart vs mid-loop.
        std::vector<double> chirpRatios;
        for (const auto& e : events)
        {
            if (std::abs (e.distSamples - (double) expSeg) > 8.0
                || e.outIndex + 2400 >= (int) mono.size())
                continue;
            auto d2energy = [&] (int from, int len)
            {
                double acc = 0.0;
                for (int i = from; i < from + len; ++i)
                    acc += std::abs ((double) mono[(size_t) i] - 2.0 * mono[(size_t) (i - 1)]
                                     + (double) mono[(size_t) (i - 2)]);
                return acc;
            };
            const double atRestart = d2energy (e.outIndex, 400);
            const double midLoop   = d2energy (e.outIndex + 2000, 400);
            chirpRatios.push_back (atRestart / juce::jmax (1.0e-12, midLoop));
        }
        const double medChirp = medianOf (chirpRatios);

        // Recovery: forward jump (negative dist) at or after the release tick,
        // and the tail of the render tracks LIVE material again. The window
        // spans the release tick plus the seek ceiling plus slack, and so
        // accepts a build with OR without the seek — verified: disabling the
        // seek leaves this check green. That is deliberate. This probe's claim
        // is "the loop ends and the head returns to live"; whether a servo
        // mute precedes the jump is L4's claim, and L4 is what fails when the
        // stage is removed.
        const bool recoveredForward = recoveryOnset >= 119990 && recoveryOnset <= 140000
                                      && recoveryDist < -1000.0;
        double tailErr = 0.0;
        for (int n2 = total - 8000; n2 < total; ++n2)
            tailErr = juce::jmax (tailErr,
                                  std::abs ((double) mono[(size_t) n2] - (double) sawAt (n2 - kComp)));

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("L DSP-02 loop-grid", live && firstOnGrid && wrapOnsets.size() >= 10,
               juce::String ("first restart @") + juce::String (wrapOnsets.empty() ? -1 : wrapOnsets.front())
                   + " expected 24000 +/-8, " + juce::String ((int) wrapOnsets.size()) + " restarts");

        check ("L DSP-02 loop-spacing", spacingOk,
               juce::String ("worst |gap - ") + juce::String (expSeg) + "| = " + juce::String (worstGap)
                   + " samples (bound 8) over " + juce::String ((int) wrapOnsets.size())
                   + " restarts — CD_SEGMENT 100 ms asks for 4800");

        check ("L DSP-02 loop-chirps", medChirp > 5.0 && ! chirpRatios.empty(),
               juce::String ("median restart/mid-loop HF ratio ") + juce::String (medChirp, 2)
                   + " (need > 5) over " + juce::String ((int) chirpRatios.size()) + " restarts");

        check ("L DSP-02 loop-recovery", recoveredForward && tailErr < 1.0e-3,
               juce::String ("recovery @") + juce::String (recoveryOnset) + " dist "
                   + juce::String (recoveryDist, 1) + " (forward), tail err "
                   + juce::String (tailErr, 6) + " (need < 1e-3)");
    }

    //==========================================================================
    // L2 — v1.3.0 item 5, CD half: a sustained loop must run until the RING
    // budget is spent and then recover on its OWN terms.
    //
    // Each pass ages the head by one segment, so at CD_SEGMENT 400 ms the
    // ladder is ~24 passes deep before the budget (maxLag - 50 ms) refuses
    // another. Two things this pins that nothing else does:
    //
    //   1. The loop SUSTAINS. At the old 2.5 s ring the same settings ran 6
    //      passes; a 10 s ring is the whole point of item 5.
    //   2. Exhaustion is ONE intentional forward recovery jump, and the loop
    //      is Idle after it. Previously there was no budget gate here at all:
    //      ReadHead's lag-overflow clamp teleported the head forward while
    //      this state machine still read Loop, and the very next wrap
    //      re-jumped from the teleported position — a slip belonging to no
    //      family. The "no backward jump in the segment after recovery"
    //      clause is what discriminates the two.
    //
    // Ticks every 2 s keep re-winning CD, but a rung-2 win with state == Loop
    // returns early, so the loop is extended rather than restarted.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 0.5f);   // tick every 2 s
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "CD_SEVERITY",     1.0f);   // rungFloat >= 2.25 => always Loop
        setParam (*p, "CD_SEGMENT",      400.0f); // 19200 samples
        setParam (*p, "SEED",            7.0f);

        // 400 ms is already an exact 30 sectors, so v1.6.0's sector lock is a
        // no-op here and this probe still measures what item 5 claimed.
        constexpr int seg   = 19200;
        static_assert (19200 == (int) (30 * 48000 / 75), "CD_SEGMENT 400 ms must be sector-exact at kFs");

        // 14 s, not 12.5: exhaustion now recovers through the v1.6.0 servo
        // seek, so the post-recovery tracking window sits up to 400 ms later.
        // Still only ONE exhaustion in the render — a fresh loop needs another
        // 9.6 s of passes to spend the budget again.
        const int total = (int) (14.0 * kFs);

        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sawStereo);

        const auto mono    = channelToVector (out, 0);
        const int  startAt = kComp + (int) (0.2 * kFs);
        const auto events  = scanSawEvents (mono, startAt, 600, sawJumpThresh ((double) seg));

        // Passes are counted as the longest UNBROKEN run, not as a total.
        // The total does not discriminate: at the 2.5 s ring the loop still
        // racks up ~22 passes over this render, just as five short loops that
        // each exhaust the ring and get restarted by the next tick. What the
        // 10 s ring buys is one loop that runs 26 passes deep, which is the
        // claim item 5 actually makes.
        int wraps = 0, longestRun = 0, run = 0;
        int prevWrapOnset = -1;

        for (const auto& e : events)
        {
            const int onset = e.outIndex - kComp;

            if (std::abs (e.distSamples - (double) seg) <= 8.0)
            {
                ++wraps;
                run = (prevWrapOnset >= 0 && std::abs (onset - prevWrapOnset - seg) <= 8)
                          ? run + 1 : 1;
                longestRun    = juce::jmax (longestRun, run);
                prevWrapOnset = onset;
                continue;
            }

            run = 0;
            prevWrapOnset = -1;
        }

        // The recovery is located by the SERVO MUTE, not by a marker step
        // (v1.6.0). Exhaustion now goes silent for 100-400 ms before jumping,
        // and the marker step at the un-mute is a step out of zero rather than
        // a jump between two saw lines, so its recovered "distance" is an
        // artefact of wherever the saw happened to be. At severity 1.0 the
        // rung roll is >= 2.25 every time, so the mute rung cannot fire and a
        // hard-zero run of this length is unambiguously the seek.
        const auto seeks = zeroRuns (mono, startAt, (int) mono.size(), 4000);

        const int    recoveries       = (int) seeks.size();
        int          recoveryOnset    = -1;
        double       recoveryLagAfter = 0.0;

        if (! seeks.empty())
        {
            const int endsAt = seeks.front().start + seeks.front().length;
            recoveryOnset    = endsAt - kComp;

            // Read the landing position past the un-mute ramp, the recovery
            // fade and the re-lock chirp.
            const int probeAt = juce::jmin ((int) mono.size() - 1, endsAt + 600);
            recoveryLagAfter  = lagFromSaw (mono[(size_t) probeAt], probeAt, kComp);
        }

        // Where the recovery LANDS is the whole discriminator, and it has to
        // be measured rather than inferred from event spacing: the pre-fix
        // teleport-and-re-jump happened within a sample or two of itself, so
        // the scanner's own smear-skip merges the pair into one event. Read
        // the head's material directly instead — live means the render tracks
        // the latency-aligned input; the pre-fix path left it ~2.6 s back
        // (0.5 * maxLag from the clamp, plus one segment from the still-Loop
        // state machine re-jumping off the teleported position).
        //
        // The window sits after the recovery and before the next 2 s tick can
        // start a fresh loop. An earlier draft asserted "no backward jump
        // within one segment of the recovery" and flagged exactly that next
        // tick: recovery lands at 11.6 s and the tick is at 12.0 s, one
        // 400 ms segment later to the sample. Coincidence, not a defect.
        //
        // v1.6.0 makes the window EXPLICIT rather than lucky. The servo seek
        // delays the recovery by 100-400 ms, so it now lands ~1300 samples
        // before that same tick and a fixed 10000-sample window measured the
        // next loop's entry jump (0.018311 — exactly one 19200 segment over
        // the saw period, i.e. the probe reading a working engine as broken).
        // The window is clipped to the tick and its WIDTH is asserted, so it
        // can never shrink to nothing and pass by measuring no samples.
        constexpr int kTickPeriod = (int) (2.0 * kFs);   // CLOCK_FREE_RATE 0.5

        double postErr    = 0.0;
        int    postWindow = 0;
        if (recoveryOnset >= 0)
        {
            const int nextTick = ((recoveryOnset / kTickPeriod) + 1) * kTickPeriod;
            const int from     = recoveryOnset + kComp + 600;   // past fade + chirp
            const int to       = juce::jmin (juce::jmin (from + 10000, nextTick + kComp - 200),
                                             (int) mono.size());

            postWindow = to - from;
            for (int n2 = from; n2 < to; ++n2)
                postErr = juce::jmax (postErr,
                                      std::abs ((double) mono[(size_t) n2]
                                                - (double) sawAt (n2 - kComp)));
        }

        const bool live       = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool sustained  = longestRun >= 18;           // 26 measured; 2.5 s ring gives 5
        const bool recovered  = recoveries == 1 && recoveryLagAfter < 1000.0;
        const bool tracksLive = recoveryOnset >= 0 && postErr < 1.0e-3 && postWindow >= 400;

        check ("L2 item-5 cd-loop-budget", live && sustained && recovered && tracksLive,
               juce::String (longestRun) + " passes in the longest unbroken loop (need >= 18), "
                   + juce::String (wraps) + " total, " + juce::String (recoveries)
                   + " recovery jump(s) (need exactly 1) @" + juce::String (recoveryOnset)
                   + " landing at lag " + juce::String (recoveryLagAfter, 1)
                   + " (need < 1000), post-recovery err vs live "
                   + juce::String (postErr, 6) + " (need < 1e-3) over "
                   + juce::String (postWindow) + " samples (need >= 400)"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // ── v1.6.0: improvement brief items 14 and 18 ────────────────────────────
    //==========================================================================

    //==========================================================================
    // L3 — item 14c, the sector lock AND its gate. Two renders that differ in
    // ONE parameter, measured the same way: above kSectorSeverity the loop
    // window snaps to a multiple of fs/75, at or below it the free CD_SEGMENT
    // is used verbatim.
    //
    // The gate is the whole point, so both halves are asserted. A build that
    // quantised unconditionally passes the first half and fails the second; a
    // build that never quantises (v1.5.0) fails the first. CD_SEGMENT 100 ms
    // is deliberately NOT sector-aligned — 4800 vs 5120 at kFs — so the two
    // outcomes are 320 samples apart and cannot be confused by the +/-8
    // detection slop.
    //
    // The low-severity render needs the loop rung to fire at all, and at
    // severity exactly 0.5 it takes r >= 0.833 on the cd stream to reach it
    // (~17% of wins) — so the clock runs at 1 Hz for 40 s rather than the 2 s
    // grid the other CD probes use. At 15 attempts a seed missing the rung
    // entirely is a 6% event and this probe drew one; at 40 it is 0.07%. The
    // wrap COUNT is asserted either way, so a seed that never loops reports
    // vacuous instead of passing on an empty set.
    {
        const int expHi = (int) sectorQuantised (100.0);   // 5120
        constexpr int expLo = 4800;                        // the free knob value

        auto measure = [&] (float severity, int& hiCount, int& loCount, int& other)
        {
            auto p = makeProc();
            setParam (*p, "CLOCK_MODE",      1.0f);   // Free
            setParam (*p, "CLOCK_FREE_RATE", 1.0f);   // tick every 1 s
            setParam (*p, "TAPE_ENABLE",     0.0f);
            setParam (*p, "VINYL_ENABLE",    0.0f);
            setParam (*p, "CD_PROB",         100.0f);
            setParam (*p, "CD_SEVERITY",     severity);
            setParam (*p, "CD_SEGMENT",      100.0f);
            setParam (*p, "SEED",            11.0f);

            const int total = (int) (40.0 * kFs);
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, sawStereo);

            const auto mono   = channelToVector (out, 0);
            const auto events = scanSawEvents (mono, kComp + (int) (0.2 * kFs), 600,
                                               sawJumpThresh ((double) expLo));

            hiCount = loCount = other = 0;
            for (const auto& e : events)
            {
                if (std::abs (e.distSamples - (double) expHi) <= 8.0)       ++hiCount;
                else if (std::abs (e.distSamples - (double) expLo) <= 8.0)  ++loCount;
                else if (e.distSamples > 100.0)                             ++other;
            }
        };

        int hiQ = 0, hiF = 0, hiX = 0, loQ = 0, loF = 0, loX = 0;
        measure (1.0f, hiQ, hiF, hiX);
        measure (0.5f, loQ, loF, loX);

        check ("L3 item-14c sector-lock", hiQ >= 10 && hiF == 0,
               juce::String ("severity 1.0: ") + juce::String (hiQ) + " wraps at "
                   + juce::String (expHi) + " (sector-locked), " + juce::String (hiF)
                   + " at the free 4800 (need 0), " + juce::String (hiX) + " other"
                   + (hiQ >= 10 ? "" : " — TOO FEW WRAPS, probe vacuous"));

        check ("L3 item-14c gate-negative", loF >= 10 && loQ == 0,
               juce::String ("severity 0.5: ") + juce::String (loF) + " wraps at the free "
                   + juce::String (expLo) + ", " + juce::String (loQ)
                   + " sector-locked (need 0), " + juce::String (loX) + " other"
                   + (loF >= 10 ? "" : " — TOO FEW WRAPS, probe vacuous"));
    }

    //==========================================================================
    // L4 — item 18, the servo-seek terminal stage. A loop released above
    // kSeekSeverity goes FULLY SILENT for 100-400 ms before the recovery jump;
    // at or below it the recovery is immediate, as it has always been.
    //
    // Measured as a hard-zero run after the release tick, which is
    // unambiguous at these settings: severity 1.0 and 0.85 both roll rung 2 on
    // essentially every win, and once CD_PROB is 0 no new mute rung can start,
    // so nothing else in the engine can take the output to exact zero.
    //
    // The negative control at exactly 0.85 is the gate: kSeekSeverity is a
    // strict >, so this is the highest severity that must NOT seek. It runs
    // the same render and requires the loop to have been wrapping right up to
    // the release, so "no mute" cannot be explained by "no loop".
    {
        auto run = [&] (float severity, int& lastWrapOnset, ZeroRun& seek, double& tailErr)
        {
            auto p = makeProc();
            setParam (*p, "CLOCK_MODE",      1.0f);
            setParam (*p, "CLOCK_FREE_RATE", 2.0f);   // ticks every 24000
            setParam (*p, "TAPE_ENABLE",     0.0f);
            setParam (*p, "VINYL_ENABLE",    0.0f);
            setParam (*p, "CD_PROB",         100.0f);
            setParam (*p, "CD_SEVERITY",     severity);
            setParam (*p, "CD_SEGMENT",      100.0f);
            setParam (*p, "SEED",            42.0f);

            const int total = 192000;
            juce::AudioBuffer<float> out (2, total);
            out.clear();

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> scratch (2, 512);
            int  n = 0;
            bool dropped = false;
            while (n < total)
            {
                if (! dropped && n >= 120000)      // past the 119999 release tick
                {
                    setParam (*p, "CD_PROB", 0.0f);
                    dropped = true;
                }

                const int chunk = juce::jmin (500, total - n);
                juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, chunk);
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < chunk; ++s)
                        block.setSample (ch, s, sawStereo (ch, n + s));
                p->processBlock (block, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, n, block, ch, 0, chunk);
                n += chunk;
            }

            const auto mono   = channelToVector (out, 0);
            const auto events = scanSawEvents (mono, kComp + (int) (0.2 * kFs), 600,
                                               sawJumpThresh (4800.0));

            lastWrapOnset = -1;
            for (const auto& e : events)
                if (e.distSamples > 1000.0)
                    lastWrapOnset = e.outIndex - kComp;

            // Only silence AFTER the release tick counts: a mute rung before
            // it would be a different artifact answering a different question.
            const auto runs = zeroRuns (mono, 143999 + kComp, (int) mono.size(), 480);
            seek = runs.empty() ? ZeroRun{} : runs.front();

            tailErr = 0.0;
            for (int n2 = total - 8000; n2 < total; ++n2)
                tailErr = juce::jmax (tailErr,
                                      std::abs ((double) mono[(size_t) n2]
                                                - (double) sawAt (n2 - kComp)));
        };

        int     wrapHi = -1, wrapLo = -1;
        ZeroRun seekHi, seekLo;
        double  tailHi = 0.0, tailLo = 0.0;

        run (1.0f,  wrapHi, seekHi, tailHi);
        run (0.85f, wrapLo, seekLo, tailLo);

        const double seekMs = 1000.0 * seekHi.length / kFs;

        // Bounds are the declared window plus the two 1 ms gain ramps.
        const bool inWindow = seekMs >= 1000.0 * CDSkip::kSeekMinSeconds - 3.0
                              && seekMs <= 1000.0 * CDSkip::kSeekMaxSeconds + 3.0;

        check ("L4 item-18 servo-seek", inWindow && tailHi < 1.0e-3 && wrapHi > 0,
               juce::String ("severity 1.0: mute ") + juce::String (seekMs, 1) + " ms (window "
                   + juce::String (1000.0 * CDSkip::kSeekMinSeconds, 0) + "-"
                   + juce::String (1000.0 * CDSkip::kSeekMaxSeconds, 0)
                   + " ms), then tail err " + juce::String (tailHi, 6) + " (need < 1e-3)"
                   + (wrapHi > 0 ? "" : " — NO LOOP RAN, probe vacuous"));

        // 0.85 is not > 0.85: no seek. The loop must still have been running
        // at the release, or the absence of a mute proves nothing.
        const bool loopedToTheEnd = wrapLo > 100000;

        check ("L4 item-18 gate-negative", seekLo.length == 0 && loopedToTheEnd && tailLo < 1.0e-3,
               juce::String ("severity 0.85: longest post-release silence ")
                   + juce::String (seekLo.length) + " samples (need 0), last wrap @"
                   + juce::String (wrapLo) + ", tail err " + juce::String (tailLo, 6)
                   + (loopedToTheEnd ? "" : " — LOOP ENDED EARLY, probe vacuous"));
    }

    //==========================================================================
    // L5 — item 14a, the per-jump fade length, measured on ReadHead directly.
    //
    // It cannot be measured through the plugin: every CD loop wrap fires a
    // chirp over exactly the window the fade occupies, and the chirp is three
    // orders of magnitude louder than the marker step being faded. Driving the
    // read head and its ring straight from the harness removes the chirp, the
    // clock and the rung roll, and leaves the one number in question — over
    // how many samples does the two-head blend run.
    //
    // The saw is read at integer positions throughout, so CaptureRing takes
    // its exact-integer path and the expected value of each head is exact.
    // "Blend width" is therefore the count of samples whose output differs
    // from the incoming head's line at all.
    {
        const int    ringSize = (int) (kFs * CaptureRing::kRingSeconds) + 1;
        const int    filled   = 500000;
        const double hi       = (double) (filled - 1);

        CaptureRing ring;
        ReadHead    head;
        ring.prepare (kFs);
        head.prepare (kFs, ringSize);

        for (int n = 0; n < filled; ++n)
            ring.push (sawAt (n), sawAt (n));

        // Renders one sample and returns the deviation from the line the
        // INCOMING head is reading — 0 exactly once the fade has finished.
        auto step = [&] (double expectedPos)
        {
            float l = 0.0f, r = 0.0f;
            head.renderSample (ring, 1.0, false, false, 0.0, l, r);
            return std::abs ((double) l - (double) sawAt ((int) expectedPos));
        };

        auto blendWidth = [&] (double from, double to, int fadeSamples)
        {
            head.reset();
            head.clampAndScheduleJump (from, filled, true);              // instant, no fade
            head.clampAndScheduleJump (to,   filled, false, fadeSamples);

            int width = 0;
            for (int i = 0; i < 4000; ++i)
            {
                if (step (to + i) <= 1.0e-9)
                    break;
                ++width;
            }
            return width;
        };

        // The fade runs fadeLen samples and reaches unity ON the last one, so
        // the count of non-zero deviations is fadeLen - 1.
        const int expDefault = head.getDefaultFadeSamples() - 1;    // 143 @ 48 kHz
        const int expSplice  = head.getSpliceFadeSamples()  - 1;    // 23  @ 48 kHz

        const int gotDefault = blendWidth (hi, hi - 300000.0, 0);
        const int gotSplice  = blendWidth (hi, hi - 300000.0, head.getSpliceFadeSamples());

        check ("L5 item-14a fade-lengths",
               gotDefault == expDefault && gotSplice == expSplice && expSplice < expDefault,
               juce::String ("default ") + juce::String (gotDefault) + " (expect "
                   + juce::String (expDefault) + " = 3 ms), splice " + juce::String (gotSplice)
                   + " (expect " + juce::String (expSplice) + " = "
                   + juce::String (1000.0 * ReadHead::kLoopSpliceFadeSeconds, 2) + " ms)");

        // The fold guard. A jump arriving while the OUTGOING head still
        // dominates must spend the running fade's length, not the one it asks
        // for: rescaling t from fadeCount/144 to fadeCount/24 would collapse
        // the outgoing head's weight from 0.861 to 0 in one sample
        // (pattern_splice_jump_guard_previous_fade). Both halves are checked —
        // the residual step AND the fact that the fade still ends at 144.
        head.reset();
        head.clampAndScheduleJump (hi, filled, true);
        head.clampAndScheduleJump (hi - 300000.0, filled, false);        // 3 ms fade

        double lastOut = 0.0;
        for (int i = 0; i < 20; ++i)
        {
            float l = 0.0f, r = 0.0f;
            head.renderSample (ring, 1.0, false, false, 0.0, l, r);
            lastOut = (double) l;
        }

        head.clampAndScheduleJump (hi - 600000.0, filled, false,
                                   head.getSpliceFadeSamples());        // asks for 0.5 ms

        float fl = 0.0f, fr = 0.0f;
        head.renderSample (ring, 1.0, false, false, 0.0, fl, fr);
        const double foldStep = std::abs ((double) fl - lastOut);

        // Where the second jump LANDS after clamping — 600000 back is outside
        // the ring window, so the choke point pins it at the lag ceiling.
        const double landed = hi - juce::jmin (600000.0, head.getMaxLag());

        int remaining = 1;                       // the render above is sample 1
        for (int i = 1; i < 4000; ++i)
        {
            if (step (landed + i) <= 1.0e-9)
                break;
            ++remaining;
        }

        const int    expRemaining = head.getDefaultFadeSamples() - 20 - 1;
        const double jumpDelta    = std::abs ((double) sawAt ((int) landed)
                                              - (double) sawAt ((int) (hi - 300000.0)));

        check ("L5 item-14a fold-keeps-length",
               remaining == expRemaining && foldStep < 0.25 * jumpDelta,
               juce::String ("fade ran ") + juce::String (20 + remaining) + " of "
                   + juce::String (head.getDefaultFadeSamples())
                   + " samples after a 0.5 ms request 20 in (must keep the running length), "
                   + "step " + juce::String (foldStep, 5) + " (bound "
                   + juce::String (0.25 * jumpDelta, 5)
                   + "; adopting the short length gives ~0.69 x delta)");
    }

    //==========================================================================
    // M — DSP-03 vinyl on the saw marker: backward jumps are integer
    // revolution multiples (45 RPM => 64000 samples @ 48 kHz), every onset
    // sits on the clock grid (FUNC-01). Pops are silenced (VINYL_POP 0) so the
    // marker stays clean — the pop draws are still consumed, so M2/M3 share
    // this event schedule.
    //
    // The FORWARD half changed shape in v1.7.0 (brief item 27a). It used to
    // assert "every forward event lands at live", which was the whole of the
    // old behaviour; a forward event is now EITHER exactly one revolution
    // forward (whenever the head has that much recorded past to spend) OR the
    // return-to-live fallback. Both shapes are asserted, and the probe
    // additionally requires that the revolution-quantized forward actually
    // fires — under the pre-v1.7.0 engine forwardRev is 0 and this fails.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 2.0f);   // ticks every 24000
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_PROB",      100.0f);
        setParam (*p, "VINYL_RPM",       1.0f);   // 45 => 4/3 s = 64000 samples
        setParam (*p, "VINYL_POP",       0.0f);
        setParam (*p, "SEED",            99.0f);

        // v1.3.0 geometry. The ring is 10 s, so the head can legitimately sit
        // seconds behind live — and a backward jump taken before the ring has
        // FILLED lands in pre-history, where readAbs returns the cleared
        // buffer's zeros and the marker measures nothing at all. The old 6 s
        // render was sized for the 2.5 s ring, and under a 10 s one it spent
        // its first four ticks throwing the head into that dead zone: the
        // scan saw 5 events where the transport had made 11. Fill the ring
        // first, then measure over a window where every jump is meaningful.
        const int warmup = (int) (12.0 * kFs);
        const int total  = warmup + (int) (8.0 * kFs);   // 20 s => 40 ticks
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sawStereo);

        const auto mono   = channelToVector (out, 0);
        // warmup lands exactly ON a clock tick, so a jump fires at the first
        // sample of the window and scanSawEvents reads its vPre from inside its
        // own smear — that one event measures ~444 samples short of the
        // revolution while every other reads 64000 +/- 3.2 (the +2% re-approach
        // trim over the 160-sample read-ahead). Start clear of the tick: the
        // event is skipped rather than mis-measured.
        const int  startAt = kComp + warmup + 400;
        constexpr int rev = 64000;

        const auto events  = scanSawEvents (mono, startAt, 160, sawJumpThresh ((double) rev));

        int  backward = 0, forward = 0, forwardRev = 0, forwardLive = 0;
        bool distOk = true, gridOk = true, fwdShapeOk = true;
        juce::String detail;

        for (const auto& e : events)
        {
            const int onset = e.outIndex - kComp;
            const int mod   = ((onset % 24000) + 24000) % 24000;
            if (! (mod <= 8 || mod >= 24000 - 8))
            {
                gridOk = false;
                detail << " OFF-GRID @" << onset;
            }

            if (e.distSamples > 100.0)
            {
                ++backward;
                const int k = juce::jmax (1, juce::roundToInt (e.distSamples / rev));
                if (std::abs (e.distSamples - (double) k * rev) > 8.0)
                {
                    distOk = false;
                    detail << " NON-REV back dist " << juce::String (e.distSamples, 1);
                }
            }
            else
            {
                ++forward;

                // Exactly +1 revolution (dist is negative for a forward move),
                // or the fallback, which must land ON live. A forward event of
                // any other size is a move this transport may not make.
                if (std::abs (e.distSamples + (double) rev) <= 8.0)
                {
                    ++forwardRev;
                }
                else
                {
                    const double lagAfter = lagFromSaw (e.vPost, e.postIndex, kComp);
                    if (lagAfter > 100.0)
                    {
                        fwdShapeOk = false;
                        detail << " FORWARD NEITHER +rev NOR LIVE: dist "
                               << juce::String (e.distSamples, 1) << " lag "
                               << juce::String (lagAfter, 1);
                    }
                    else
                    {
                        ++forwardLive;
                    }
                }
            }
        }

        // At VINYL_PROB 100 a win lands every 24000 samples and supersedes any
        // locked groove long before its 64000-sample re-pass could fire, so
        // every event here is win-driven and on the clock grid. What the 10 s
        // ring changed is how DEEP the backward ladder gets before the room
        // gate hands over to a forward recovery: the lag now stacks past
        // 7 revolutions (~9.3 s), where the 2.5 s ring refused the second
        // jump outright. M4 covers the multi-pass groove itself.
        const bool counts = backward >= 8 && forward >= 2 && (int) events.size() >= 12;
        const bool live   = out.getMagnitude (0, 0, total) > 1.0e-4f;

        // The item-27a discriminator: the ladder here descends past 7
        // revolutions, so every forward win has a revolution to spend. Against
        // the pre-v1.7.0 engine all 8 of these were return-to-live teleports
        // and forwardRev is 0.
        const bool revFwdLive = forwardRev >= 4;

        check ("M DSP-03 vinyl-jumps",
               live && counts && distOk && gridOk && fwdShapeOk && revFwdLive,
               juce::String ((int) events.size()) + " events (" + juce::String (backward)
                   + " back / " + juce::String (forwardRev) + " fwd+rev / "
                   + juce::String (forwardLive) + " fwd-live), dists = k*64000 +/-8, "
                   + "onsets on 24000-grid +/-8" + detail
                   + (counts ? "" : " — TOO FEW EVENTS, probe vacuous")
                   + (revFwdLive ? "" : " — NO REVOLUTION-QUANTIZED FORWARD (item 27a dead)"));
    }

    //==========================================================================
    // M2 — DSP-03 pitch: same schedule (same seed/params) on a sine — the
    // revolution jumps must never move the pitch (rate stays 1.0; only the
    // <= +2% re-approach trim is allowed). Pops ON for realism.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 2.0f);
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_PROB",      100.0f);
        setParam (*p, "VINYL_RPM",       1.0f);
        setParam (*p, "VINYL_POP",       50.0f);
        setParam (*p, "SEED",            99.0f);

        // Same 12 s ring-fill as M, and for the same reason: before the 10 s
        // ring has filled, the backward ladder walks the head into
        // pre-history, where the reads are zeros and autocorrelation has no
        // pitch to track. That is a measurement artefact, not a pitch defect
        // — the band below was already in spec at 4 s (220.2-226.4 Hz), it
        // was the trackable-hop count that collapsed (277/599).
        const int warmup = (int) (12.0 * kFs);
        const int total  = warmup + (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto mono  = channelToVector (out, 0);
        const auto trace = pitchTrace (mono, kSineHz);
        const int  hopStart = (kComp + warmup) / 256;

        /* DSP-03's criterion is STEADY-STATE pitch across jumps. A window that
           straddles the jump instant itself sees a phase-discontinuous
           crossfade plus the pop transient and reads a false pitch — exclude
           windows overlapping [tick - window, tick + 2400) around each clock
           tick (ticks every 24000 input samples, emerging at +kComp; jumps
           happen only at ticks, so the exclusion is exact and deterministic). */
        constexpr int kTickPeriod = 24000;
        constexpr int kWin        = 1024;
        constexpr int kHop        = 256;
        const auto nearTick = [&] (int winStartOut)
        {
            const int rel = ((winStartOut - kComp) % kTickPeriod + kTickPeriod) % kTickPeriod;
            // window [rel, rel + kWin) vs exclusion (tick - kWin, tick + 2400)
            return rel < 2400 || rel > kTickPeriod - kWin;
        };

        int valid = 0, evaluated = 0;
        double minF = 1.0e9, maxF = 0.0;
        for (int i = hopStart; i < (int) trace.size(); ++i)
        {
            if (nearTick (i * kHop))
                continue;
            ++evaluated;
            const double f = trace[(size_t) i];
            if (f <= 0.0) continue;
            ++valid;
            minF = juce::jmin (minF, f);
            maxF = juce::jmax (maxF, f);
        }

        const bool live      = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool inBand    = minF >= 213.0 && maxF <= 227.0;
        const bool trackable = evaluated > 0 && valid > (evaluated * 3) / 5;

        check ("M2 DSP-03 vinyl-pitch", live && inBand && trackable,
               juce::String ("pitch [") + juce::String (minF, 1) + ", " + juce::String (maxF, 1)
                   + "] Hz (bounds [213, 227]), valid " + juce::String (valid) + "/"
                   + juce::String (evaluated));
    }

    //==========================================================================
    // M3 — vinyl pops audible: identical seed/schedule, VINYL_POP 50 vs 0.
    // triggerPop consumes its RNG draws at level 0 too, so ONLY the pop audio
    // differs between the renders.
    {
        auto configure = [] (OBitrotAudioProcessor& proc, float pop)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 2.0f);
            setParam (proc, "TAPE_ENABLE",     0.0f);
            setParam (proc, "CD_ENABLE",       0.0f);
            setParam (proc, "VINYL_PROB",      100.0f);
            setParam (proc, "VINYL_RPM",       1.0f);
            setParam (proc, "VINYL_POP",       pop);
            setParam (proc, "SEED",            99.0f);
        };

        const int total = 96000;
        auto a = makeProc();  configure (*a, 50.0f);
        auto b = makeProc();  configure (*b, 0.0f);

        juce::AudioBuffer<float> outA, outB;
        renderInto (*a, outA, total, { 512 }, sineStereo);
        renderInto (*b, outB, total, { 512 }, sineStereo);

        const double diff = maxAbsDiff (outA, outB);
        const bool   live = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("M3 vinyl-pop-audible", live && diff > 1.0e-3,
               juce::String ("maxAbsDiff pop-50-vs-0 = ") + juce::String (diff, 5)
                   + " (need > 1e-3)");
    }

    //==========================================================================
    // M4 — v1.3.0 item 5, vinyl half: the locked groove finally LOOPS.
    //
    // BRIEF.md's headline is "locked groove loops with a pop per pass", and
    // until now it could not: a re-pass needs
    // lag + revolution <= maxLag - 50 ms, and at kRingSeconds = 2.5 that
    // inequality required a NEGATIVE starting lag, so the groove released on
    // its first return every single time. No probe caught it because M runs
    // VINYL_PROB 100 with ticks every 0.5 s, where a fresh win supersedes the
    // groove long before its 1.333 s return — every event there is win-driven
    // and on the clock grid.
    //
    // So this probe makes the groove the only thing happening: ONE tick at
    // 10 s (by which point the 10 s ring is full, so the backward ladder
    // reads real material rather than pre-history), then nothing. Re-passes
    // fire when the head returns to lockedEndAbs — exactly one revolution
    // apart, and deliberately NOT on the clock grid. Total stays under one
    // saw period (1048576) so the marker cannot wrap.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 0.1f);   // one tick at 10 s
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_PROB",      100.0f);
        setParam (*p, "VINYL_RPM",       1.0f);   // 45 => 4/3 s = 64000 samples
        setParam (*p, "VINYL_POP",       0.0f);   // keep the marker clean
        setParam (*p, "SEED",            99.0f);

        constexpr int rev   = 64000;
        const int     total = (int) (21.0 * kFs);

        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sawStereo);

        const auto mono    = channelToVector (out, 0);
        const int  startAt = kComp + (int) (0.2 * kFs);
        const auto events  = scanSawEvents (mono, startAt, 160, sawJumpThresh ((double) rev));

        // Re-passes: consecutive one-revolution backward jumps spaced one
        // revolution apart in TIME. The initial win-driven jump seeds the
        // chain; each link after it is a pass the 2.5 s ring could not buy.
        int    rePasses = 0, worstSpacing = 0;
        int    prevBackOnset = -1;
        double deepestLag = 0.0;

        for (const auto& e : events)
        {
            const int onset = e.outIndex - kComp;
            deepestLag = juce::jmax (deepestLag, lagFromSaw (e.vPost, e.postIndex, kComp));

            if (std::abs (e.distSamples - (double) rev) > 8.0)
            {
                prevBackOnset = -1;                  // not a revolution jump
                continue;
            }

            if (prevBackOnset >= 0)
            {
                const int spacing = onset - prevBackOnset;
                worstSpacing = juce::jmax (worstSpacing, std::abs (spacing - rev));
                if (std::abs (spacing - rev) <= 8)
                    ++rePasses;
            }
            prevBackOnset = onset;
        }

        const bool live     = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool loops    = rePasses >= 4;             // 2.5 s ring scored 0
        const bool onRevGrid = rePasses > 0 && worstSpacing <= 8;
        const bool deep     = deepestLag > 4.0 * rev;    // the ladder really descends

        check ("M4 item-5 locked-groove-multipass", live && loops && onRevGrid && deep,
               juce::String (rePasses) + " revolution-spaced re-passes (need >= 4; the 2.5 s "
                   "ring allowed 0), worst |spacing - 64000| = " + juce::String (worstSpacing)
                   + " (bound 8), deepest lag " + juce::String (deepestLag, 0) + " samples"
                   + (deep ? "" : " — LADDER TOO SHALLOW, probe vacuous")
                   + (live ? "" : " — SILENT"));
    }

    //==========================================================================
    // M5 — item 27a's GATE, on VinylTransport driven directly.
    //
    // M proves the revolution-quantized forward fires when the lag affords it.
    // The other half of the claim is that it does NOT fire when the lag does
    // not — and that half cannot be reached from a plugin render, because the
    // lag there is whatever the ladder happens to have accumulated. Driving the
    // transport straight from the harness sets the lag to an exact number on
    // either side of the threshold, which is the only way the gate is the
    // thing being measured rather than the seed.
    //
    // Both halves are asserted. A build that always jumps +rev passes the
    // first and fails the second; the pre-v1.7.0 build fails the first.
    {
        const int    ringSize = (int) (kFs * CaptureRing::kRingSeconds) + 1;
        const int    filled   = 800000;
        const double hi       = (double) (filled - 1);
        constexpr int rev     = 64000;                  // 45 RPM @ 48 kHz
        const double minLag   = 0.05 * kFs;             // 2400

        // A win draws (rType, rDir) from the vinyl stream in that fixed order.
        // The forward path needs rType >= 0.30 (not a locked groove) and
        // rDir >= 0.65 (wantForward). juce::Random is copyable, so a candidate
        // seed can be inspected without consuming the stream it seeds.
        int seed = -1;
        for (int s = 0; s < 4096 && seed < 0; ++s)
        {
            RngBank probe;
            probe.reseed (s);
            juce::Random peek = probe.get (RngBank::vinyl);
            const float rType = peek.nextFloat();
            const float rDir  = peek.nextFloat();
            if (rType >= 0.30f && rDir >= 0.65f)
                seed = s;
        }

        // Runs ONE win with the head placed at `lag` behind live, and returns
        // the distance the head moved (+ = forward).
        auto forwardJumpFrom = [&] (double lag)
        {
            CaptureRing    ring;
            ReadHead       head;
            RngBank        rng;
            ArtifactSynth  art;
            VinylTransport vt;

            ring.prepare (kFs);
            head.prepare (kFs, ringSize);
            art.prepare (kFs);
            vt.prepare (kFs);
            rng.reseed (seed);

            for (int n = 0; n < filled; ++n)
                ring.push (0.0f, 0.0f);

            head.clampAndScheduleJump (hi - lag, filled, true);   // instant placement
            const double before = head.getPosition();

            // v1.9.0 split onWin into rollKind (the two draws + the lag test)
            // and applyOwner (the install). Called back to back with nothing
            // advancing the head between them, this is the same call the probe
            // has always made.
            vt.rollKind (rng, 1 /* 45 RPM */, 0.0f, head, ring);
            vt.applyOwner (head, ring, true /* hardEdges */, art, rng);

            return head.getPosition() - before;
        };

        // Deep: a whole revolution of past to spend, plus margin.
        const double deep    = forwardJumpFrom (3.0 * rev);
        // Shallow: past minLag (so the win is not skipped) but short of a
        // revolution — deliberately just under the threshold.
        const double shallow = forwardJumpFrom (rev + minLag - 1000.0);

        const bool seedOk    = seed >= 0;
        const bool deepIsRev = std::abs (deep - (double) rev) <= 1.0;
        // The fallback lands ON live, i.e. it moves the head the whole lag.
        const bool shallowIsLive = std::abs (shallow - (rev + minLag - 1000.0)) <= 1.0;

        check ("M5 item-27a forward-gate", seedOk && deepIsRev && shallowIsLive,
               juce::String ("seed ") + juce::String (seed) + ": lag 3 rev -> moved "
                   + juce::String (deep, 1) + " (expect exactly " + juce::String (rev)
                   + "), lag " + juce::String (rev + minLag - 1000.0, 0) + " (just under the "
                   + juce::String (rev + minLag, 0) + " threshold) -> moved "
                   + juce::String (shallow, 1) + " (expect return-to-live)"
                   + (seedOk ? "" : " — NO FORWARD-PATH SEED FOUND, probe vacuous"));
    }

    //==========================================================================
    // M6 — item 27b, the warp. Two claims, and the second is the one worth
    // having.
    //
    // (a) VINYL_WARP 0 is EXACTLY transparent: the offset is 0.0, `pos - 0.0`
    //     is bit-identical to `pos`, and the render must match a build with the
    //     warp absent. Asserted here as bit-identity against VINYL_ENABLE off
    //     with every other vinyl control already inert — see below.
    // (b) A warped LOCKED GROOVE repeats EXACTLY. The groove jumps back
    //     revSamples every pass and the warp LFO's period is the same integer,
    //     so pass N and pass N+1 read the same positions and must be
    //     sample-identical. If the LFO were derived from the un-rounded
    //     seconds, or free-run at a frequency unrelated to the quantum, the
    //     passes would drift apart.
    //
    // The M4 geometry: ONE tick at 10 s, then nothing, so the groove is the
    // only thing happening and the passes are one revolution apart.
    {
        auto configure = [] (OBitrotAudioProcessor& proc, float warp)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);   // Free
            setParam (proc, "CLOCK_FREE_RATE", 0.1f);   // one tick at 10 s
            setParam (proc, "TAPE_ENABLE",     0.0f);
            setParam (proc, "CD_ENABLE",       0.0f);
            setParam (proc, "VINYL_PROB",      100.0f);
            setParam (proc, "VINYL_RPM",       1.0f);   // 45 => 64000 samples
            setParam (proc, "VINYL_POP",       0.0f);   // keep the passes clean
            setParam (proc, "VINYL_WARP",      warp);
            setParam (proc, "SEED",            99.0f);
        };

        constexpr int rev   = 64000;
        const int     total = (int) (21.0 * kFs);

        auto a = makeProc();  configure (*a, 60.0f);
        auto b = makeProc();  configure (*b,  0.0f);

        juce::AudioBuffer<float> warped, flat;
        renderInto (*a, warped, total, { 512 }, sineStereo);
        renderInto (*b, flat,   total, { 512 }, sineStereo);

        // (a) The warp must actually be DOING something, or (b) is vacuous:
        // two identical renders repeat trivially.
        const double warpDiff = maxAbsDiff (warped, flat);

        // (b) Compare two consecutive groove passes. The groove locks on the
        // 10 s tick; start well inside it and take a whole revolution.
        const auto mono = channelToVector (warped, 0);
        const int  passA = kComp + (int) (13.0 * kFs);
        double     worstPassDiff = 0.0;
        for (int i = 0; i < rev; ++i)
            worstPassDiff = juce::jmax (worstPassDiff,
                                        std::abs ((double) mono[(size_t) (passA + i)]
                                                  - (double) mono[(size_t) (passA + rev + i)]));

        // Same window on the UNWARPED render: the reference the warped pass
        // must be no worse than. Both should be exactly 0 — the point of
        // measuring it is that a non-zero result here would mean the window
        // is not inside a locked groove at all, and (b) would be vacuous.
        const auto monoFlat = channelToVector (flat, 0);
        double     flatPassDiff = 0.0;
        for (int i = 0; i < rev; ++i)
            flatPassDiff = juce::jmax (flatPassDiff,
                                       std::abs ((double) monoFlat[(size_t) (passA + i)]
                                                 - (double) monoFlat[(size_t) (passA + rev + i)]));

        const bool live     = warped.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool audible  = warpDiff > 1.0e-3;
        const bool inGroove = flatPassDiff <= 1.0e-6;
        const bool repeats  = worstPassDiff <= 1.0e-6;

        check ("M6 item-27b warp-groove", live && audible && inGroove && repeats,
               juce::String ("warp-60-vs-0 maxAbsDiff ") + juce::String (warpDiff, 5)
                   + " (need > 1e-3), pass-to-pass |diff| warped "
                   + juce::String (worstPassDiff, 9) + " / unwarped "
                   + juce::String (flatPassDiff, 9) + " (bound 1e-6)"
                   + (audible   ? "" : " — WARP INAUDIBLE, repeat claim vacuous")
                   + (inGroove  ? "" : " — WINDOW NOT IN A LOCKED GROOVE, probe vacuous")
                   + (live      ? "" : " — SILENT"));
    }

    //==========================================================================
    // M7 — item 17, the pop taxonomy. Two things are measured, both against
    // the shape of the OLD code rather than against a restated constant.
    //
    // (a) The classes are drawn at ~70/25/5. Counted on the artifactSynth
    //     stream directly rather than inferred from audio: the class draw is
    //     the first of the five, so replaying that stream's draw sequence in
    //     groups of five reproduces exactly the classification triggerPop
    //     performs. This asserts the DRAW DISCIPLINE too — five per call.
    // (b) A pop RINGS. The old code could not: a first-order lowpass has no
    //     complex pole pair, so its doublet response is monotone after the
    //     second sample. The resonator's is not. Measured as the number of
    //     zero crossings in the 5 ms after a triggered pop, on the synth
    //     driven directly (a plugin render buries it under the programme).
    {
        // (a) class weights over a long run of the stream
        RngBank rng;
        rng.reseed (2024);
        juce::Random& s = rng.get (RngBank::artifactSynth);

        constexpr int kDraws = 20000;
        int nTick = 0, nPop = 0, nScratch = 0;
        for (int i = 0; i < kDraws; ++i)
        {
            const float rClass = s.nextFloat();
            s.nextFloat(); s.nextFloat(); s.nextFloat(); s.nextFloat();   // the other four

            if      (rClass < 0.70f) ++nTick;
            else if (rClass < 0.95f) ++nPop;
            else                     ++nScratch;
        }

        const double pTick    = (double) nTick    / kDraws;
        const double pPop     = (double) nPop     / kDraws;
        const double pScratch = (double) nScratch / kDraws;

        const bool weightsOk = std::abs (pTick - 0.70) < 0.02
                            && std::abs (pPop  - 0.25) < 0.02
                            && std::abs (pScratch - 0.05) < 0.01;

        // (b) does a pop-class hit ring? Force the class by seeding the synth's
        // stream from a draw sequence whose first value lands in [0.70, 0.95).
        int popSeed = -1;
        for (int k = 0; k < 4096 && popSeed < 0; ++k)
        {
            RngBank probe;
            probe.reseed (k);
            juce::Random peek = probe.get (RngBank::artifactSynth);
            const float rClass = peek.nextFloat();
            if (rClass >= 0.70f && rClass < 0.95f)
                popSeed = k;
        }

        ArtifactSynth art;
        RngBank        popRng;
        art.prepare (kFs);
        popRng.reseed (popSeed);

        juce::Random& scratchStream = popRng.get (RngBank::scratch);
        art.triggerPop (1.0f, popRng.get (RngBank::artifactSynth));

        const int    window = (int) (0.005 * kFs);      // 5 ms
        int          crossings = 0;
        double       peak = 0.0;
        float        prev = 0.0f;
        for (int n = 0; n < window; ++n)
        {
            const float v = art.renderSample (scratchStream);
            peak = juce::jmax (peak, (double) std::abs (v));
            if (n > 2 && ((v > 0.0f) != (prev > 0.0f)))
                ++crossings;
            prev = v;
        }

        // A 900-1800 Hz ring over 5 ms is 9-18 zero crossings. The old
        // first-order lowpass produced 1 (the doublet's own sign flip) and
        // nothing after it, so the bound discriminates rather than decorates.
        const bool rings   = crossings >= 6;
        const bool sane    = peak > 0.02 && peak < 1.0;

        check ("M7 item-17 pop-taxonomy", weightsOk && rings && sane && popSeed >= 0,
               juce::String ("weights tick/pop/scratch ") + juce::String (pTick, 3) + "/"
                   + juce::String (pPop, 3) + "/" + juce::String (pScratch, 3)
                   + " (target .70/.25/.05), pop-class ring: " + juce::String (crossings)
                   + " zero crossings in 5 ms (need >= 6; a one-pole gives 1), peak "
                   + juce::String (peak, 4)
                   + (rings ? "" : " — POP DOES NOT RING")
                   + (sane  ? "" : " — PEAK OUT OF RANGE"));
    }

    //==========================================================================
    // M8 — item 27c, 78 RPM. The revolution quantum is 0.769 s, so backward
    // jumps land on 36923 samples at 48 kHz rather than 86400 or 64000. Same
    // saw-marker method as M, one speed over.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 2.0f);
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_PROB",      100.0f);
        setParam (*p, "VINYL_RPM",       2.0f);   // 78
        setParam (*p, "VINYL_POP",       0.0f);
        setParam (*p, "SEED",            99.0f);

        // The quantum comes from the SAME function the engine uses, not from a
        // literal mirrored here (pattern_test_fixture_mirrors_drift_silently).
        const int rev = VinylGeometry::revolutionSamples (kFs, 2);

        const int warmup = (int) (12.0 * kFs);
        const int total  = warmup + (int) (8.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sawStereo);

        const auto mono    = channelToVector (out, 0);
        // warmup lands exactly ON a clock tick, so a jump fires at the first
        // sample of the window and scanSawEvents reads its vPre from inside its
        // own smear — that one event measures ~444 samples short of the
        // revolution while every other reads 64000 +/- 3.2 (the +2% re-approach
        // trim over the 160-sample read-ahead). Start clear of the tick: the
        // event is skipped rather than mis-measured.
        const int  startAt = kComp + warmup + 400;
        const auto events  = scanSawEvents (mono, startAt, 160, sawJumpThresh ((double) rev));

        int  onQuantum = 0, offQuantum = 0;
        for (const auto& e : events)
        {
            const double mag = std::abs (e.distSamples);
            const int    k   = juce::jmax (1, juce::roundToInt (mag / rev));
            if (std::abs (mag - (double) k * rev) <= 8.0) ++onQuantum;
            else                                          ++offQuantum;
        }

        const bool live    = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool enough  = onQuantum >= 8;
        const bool allOn   = offQuantum == 0;
        // Discriminator: 36923 is not a multiple of 64000 or 86400, and the
        // 45 RPM quantum is not a multiple of it, so a build that ignored
        // index 2 and fell back would land off-quantum here.
        const bool distinct = rev != 64000 && rev != 86400;

        check ("M8 item-27c 78-rpm-quantum", live && enough && allOn && distinct,
               juce::String ("quantum ") + juce::String (rev) + " samples ("
                   + juce::String (1000.0 * rev / kFs, 1) + " ms), " + juce::String (onQuantum)
                   + " on-quantum / " + juce::String (offQuantum) + " off"
                   + (enough ? "" : " — TOO FEW EVENTS, probe vacuous"));
    }

    //==========================================================================
    // M9 — item 27c's preset migration, end to end through the module.
    //
    // Appending "78" repoints every NORMALIZED VINYL_RPM fraction a pre-1.7.0
    // preset saved: "45" went out as 1.0 over an end of 1, and 1.0 over an end
    // of 2 decodes as "78". The v1.0.6 hook remaps it. The probe writes the
    // JSON a v1.6.0 save would have produced and loads it through the public
    // loadPresetFromFile(), so the module's own version gate, reset pass and
    // apply pass are all in the path.
    //
    // The negative control is the point: a preset stamped 1.7.0 with the SAME
    // 1.0 must load as "78", because at that version 1.0 genuinely means 78. A
    // migration that ran unconditionally would pass the first half and fail
    // this one.
    {
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("obitrot-migration-probe");
        tmp.createDirectory();

        auto writePreset = [&] (const juce::String& version, double rpmNormalised)
        {
            auto* params = new juce::DynamicObject();
            params->setProperty ("VINYL_RPM", rpmNormalised);

            auto* preset = new juce::DynamicObject();
            preset->setProperty ("parameters", juce::var (params));
            preset->setProperty ("version", version);
            preset->setProperty ("plugin", "Ouaricon Bitrot");

            auto file = tmp.getChildFile ("probe-" + version + ".json");
            file.replaceWithText (juce::JSON::toString (juce::var (preset), true));
            return file;
        };

        auto loadAndReadRpm = [] (OBitrotAudioProcessor& proc, const juce::File& f)
        {
            proc.presetManager.loadPresetFromFile (f);
            auto* param = dynamic_cast<juce::AudioParameterChoice*> (
                              proc.apvts.getParameter ("VINYL_RPM"));
            return param != nullptr ? param->getIndex() : -1;
        };

        auto old45 = makeProc();
        auto new78 = makeProc();
        auto old33 = makeProc();

        const int gotOld45 = loadAndReadRpm (*old45, writePreset ("1.6.0", 1.0));
        const int gotNew78 = loadAndReadRpm (*new78, writePreset ("1.7.0", 1.0));
        const int gotOld33 = loadAndReadRpm (*old33, writePreset ("1.5.0", 0.0));

        tmp.deleteRecursively();

        const bool migrated = gotOld45 == 1;    // pre-1.7.0 "45" stays 45
        const bool untouched = gotNew78 == 2;   // 1.7.0 "78" stays 78
        const bool zeroSafe  = gotOld33 == 0;   // 33 1/3 is 0.0 under both encodings

        check ("M9 item-27c preset-migration", migrated && untouched && zeroSafe,
               juce::String ("v1.6.0 rpm=1.0 -> index ") + juce::String (gotOld45)
                   + " (expect 1 = 45), v1.7.0 rpm=1.0 -> index " + juce::String (gotNew78)
                   + " (expect 2 = 78), v1.5.0 rpm=0.0 -> index " + juce::String (gotOld33)
                   + " (expect 0)"
                   + (untouched ? "" : " — MIGRATION RAN ON A CURRENT PRESET"));
    }

    //==========================================================================
    // N — collision determinism + all-fire block-size invariance: all three
    // families at 100% => arbitration collisions every tick. Same seed on
    // fresh instances must render bit-identically, and the ragged block
    // chopping must not move a single bit (loop wraps, jumps and artifact
    // synthesis all under split blocks).
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 4.0f);
            setParam (proc, "TAPE_PROB",       100.0f);
            setParam (proc, "CD_PROB",         100.0f);
            setParam (proc, "VINYL_PROB",      100.0f);
            setParam (proc, "SEED",            555.0f);
        };

        const int total = 96000;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);
        auto c = makeProc();  configure (*c);

        juce::AudioBuffer<float> outA, outB, outC;
        renderInto (*a, outA, total, { 512 }, noiseStereo);
        renderInto (*b, outB, total, { 512 }, noiseStereo);
        renderInto (*c, outC, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outA, outB);
            const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("N collision-determinism", identical && live,
                   (identical ? juce::String ("all-fire same-seed fresh instances: bit-identical")
                              : firstDifference (outA, outB))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }

        {
            const bool identical = bitIdentical (outA, outC);
            const bool live      = outC.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("N all-fire-ragged", identical && live,
                   (identical ? juce::String ("512 vs 1,7,64,333,4096: bit-identical under all-fire")
                              : firstDifference (outA, outC))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // N2 — v1.2.1 item 13a: a jump arriving MID-FADE folds into the running
    // crossfade instead of restarting it. Driven directly on CaptureRing +
    // ReadHead: end-to-end, every jump instant also fires an ArtifactSynth pop
    // or chirp, whose deliberate impulses swamp any output-delta bound.
    //
    // Geometry: the ring holds the 220 Hz sine, and each jump steps back HALF
    // a period, so the outgoing head (B) and the jump-1 head (A) sit in
    // ANTIPHASE — the raw discontinuity the crossfade must hide. Jump 2 lands
    // a full period behind B, i.e. back in phase with it.
    //
    //   gap 0   = the Arbitration collision that motivated the fix: kVinyl
    //             runs cd.release() (recovery jump) then vinyl.onWin() with NO
    //             render between, so the fade is 0% done. Pre-fix, oldPos was
    //             overwritten with the jump-1 head and the output stepped the
    //             FULL antiphase discontinuity in one sample.
    //   gap 40  = fade 28% done — still inside the branch the fix changed
    //             (below the 50% dominance switch the OUTGOING head and the
    //             fade counter are both preserved). Above 50% the fold is
    //             deliberately identical to the old behaviour, which is
    //             already the bounded case, so probing there proves nothing.
    {
        const double kHalfPeriod = 0.5 * kFs / kSineHz;   // ~109.1 samples

        struct Collision { double maxDelta = 0.0; double discontinuity = 0.0; };

        auto runCollision = [&] (int gapSamples) -> Collision
        {
            CaptureRing ring;
            ReadHead    head;
            ring.prepare (kFs);
            head.prepare (kFs, ring.getSize());

            std::vector<float> out;
            out.reserve (4096);

            int n = 0;
            auto stepOne = [&] ()
            {
                ring.push (sineStereo (0, n), sineStereo (1, n));
                float l = 0.0f, r = 0.0f;
                // hardEdges off, no loop owner, no wow offset
                head.renderSample (ring, 1.0, false, false, 0.0, l, r);
                out.push_back (l);
                ++n;
            };

            for (int i = 0; i < 2000; ++i)                    // warm up
                stepOne();

            head.clampAndScheduleJump (head.getPosition() - kHalfPeriod,
                                       ring.getTotalWritten(), false);

            for (int i = 0; i < gapSamples; ++i)
                stepOne();

            Collision c;

            // The antiphase pair the jump-2 crossfade has to hide. Sampled one
            // slot back so both reads land on written material.
            const double pA = head.getPosition() - 1.0;       // jump-1 head
            const double pB = pA + kHalfPeriod;               // outgoing head
            c.discontinuity = std::abs ((double) ring.readFrac (0, pA)
                                        - (double) ring.readFrac (0, pB));

            const int markAt = (int) out.size();

            head.clampAndScheduleJump (head.getPosition() - kHalfPeriod,
                                       ring.getTotalWritten(), false);

            for (int i = 0; i < 1000; ++i)
                stepOne();

            for (size_t i = (size_t) markAt; i < out.size(); ++i)
                c.maxDelta = juce::jmax (c.maxDelta,
                                         std::abs ((double) out[i] - (double) out[i - 1]));

            return c;
        };

        {
            const auto c = runCollision (0);
            // Sine max derivative 0.0144/sample; 0.05 leaves room for the
            // 3 ms fade ramp and cleanly rejects the pre-fix full-amplitude
            // step. Liveness: the hidden discontinuity must be real.
            const bool live   = c.discontinuity >= 0.5;
            const bool smooth = c.maxDelta <= 0.05;
            check ("N2 jump-fade-collision same-tick", live && smooth,
                   juce::String ("maxDelta=") + juce::String (c.maxDelta, 5)
                       + " (bound 0.05) over a discontinuity of "
                       + juce::String (c.discontinuity, 4)
                       + (live ? "" : " — DISCONTINUITY TOO SMALL, probe vacuous"));
        }

        {
            const auto c = runCollision (40);
            const bool live    = c.discontinuity >= 0.5;
            const bool bounded = c.maxDelta <= 0.5 * c.discontinuity + 0.05;
            check ("N2 jump-fade-collision mid-fade", live && bounded,
                   juce::String ("maxDelta=") + juce::String (c.maxDelta, 5)
                       + " (bound " + juce::String (0.5 * c.discontinuity + 0.05, 5)
                       + " = half of " + juce::String (c.discontinuity, 4) + " + slack)"
                       + (live ? "" : " — DISCONTINUITY TOO SMALL, probe vacuous"));
        }
    }

    //==========================================================================
    // N3 — v1.2.1 item 13b: a tape release landing back on NORMAL with more
    // than 250 ms of lag takes ONE intentional crossfaded jump to live.
    //
    // Forced stop at the first tick, then TAPE_ENABLE off so the NEXT tick
    // releases it (a no-firer tick calls tape.release). Input is NOISE, not
    // the sine: a 220 Hz sine re-aligns at every period, so only decorrelated
    // material can prove the head is genuinely back at the write head rather
    // than an arbitrary distance behind it.
    //
    // Verdict = correlation of the render's tail against the latency-aligned
    // input. Recovered => ~1. Pre-fix the +2% trim needed ~50x the stall
    // duration, leaving the tail reading stale material => ~0.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 1.0f);   // tick every 1 s
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  100.0f);
        setParam (*p, "TAPE_RAMP",       150.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int stallTotal = (int) (1.8 * kFs);   // tick @ 1.0 s, stopped from ~1.15 s
        const int tailTotal  = (int) (1.2 * kFs);

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> scratch (2, 512);
        std::vector<float> out;
        out.reserve ((size_t) (stallTotal + tailTotal));

        auto pump = [&] (int from, int count)
        {
            int n = from;
            const int end = from + count;
            while (n < end)
            {
                const int chunk = juce::jmin (512, end - n);
                juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, chunk);

                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < chunk; ++s)
                        block.setSample (ch, s, noiseStereo (ch, n + s));

                p->processBlock (block, midi);

                for (int s = 0; s < chunk; ++s)
                    out.push_back (block.getSample (0, s));

                n += chunk;
            }
        };

        pump (0, stallTotal);

        // Disable tape: the next tick has no firer, so tape.release() runs and
        // the ramp lands back on NORMAL still ~0.65 s behind.
        setParam (*p, "TAPE_ENABLE", 0.0f);
        pump (stallTotal, tailTotal);

        // Correlate the last 0.5 s against the latency-aligned input.
        const int    winLen = (int) (0.5 * kFs);
        const int    winAt  = (int) out.size() - winLen;
        double       num = 0.0, dOut = 0.0, dIn = 0.0;

        for (int i = 0; i < winLen; ++i)
        {
            const double o = out[(size_t) (winAt + i)];
            const double x = noiseStereo (0, winAt + i - kComp);
            num  += o * x;
            dOut += o * o;
            dIn  += x * x;
        }

        const double corr = (dOut > 0.0 && dIn > 0.0)
                                ? num / std::sqrt (dOut * dIn) : 0.0;

        const bool live      = dOut > 1.0e-6;
        const bool recovered = corr > 0.9;

        check ("N3 post-stop recovery-jump", live && recovered,
               juce::String ("tail correlation vs live input = ") + juce::String (corr, 4)
                   + " (need > 0.9)"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }


    //==========================================================================
    // O — DSP-04 Gilbert-Elliott statistics. 60 s render, Silence
    // concealment, transports off: lost packets are detected from the OUTPUT
    // (packet-aligned RMS windows that collapse), never by peeking internal
    // state. v1.2 mapping — with PACKET_LOSS 40 / PACKET_BURST 30:
    // piB = 0.95*0.4 = 0.38, E[B] = 3.1 => pBG = 0.3226,
    // pGB = 0.38*0.3226/0.62 = 0.1977 (unclamped, stationary piB holds),
    // pBad = 0.7, pGood = 0.004 => E[lostFrac] = 0.38*0.7 + 0.62*0.004
    // = 0.268. Lost-run continuation r = P(next lost | lost) ~= 0.47
    // (state-weighted: P(B|lost) = 0.991, then 0.677*0.7 + 0.323*0.004 ...).
    // Ratio bounds are generous but reject gross mapping errors (an inverted
    // burst mapping reads r ~= 0.9; no burstiness reads N2 ~= 0).
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",    0.0f);
        setParam (*p, "CD_ENABLE",      0.0f);
        setParam (*p, "VINYL_ENABLE",   0.0f);
        setParam (*p, "PACKET_ENABLE",  1.0f);
        setParam (*p, "PACKET_CONCEAL", 0.0f);   // Silence
        setParam (*p, "PACKET_LOSS",    40.0f);
        setParam (*p, "PACKET_BURST",   30.0f);
        setParam (*p, "SEED",           4242.0f);

        const int packetLen = (int) std::ceil (0.020 * kFs);   // grid, 960 @ 48k
        const int total     = 3000 * packetLen;                // 60 s
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 4096 }, noiseStereo);

        const auto* o = out.getReadPointer (0);

        // Packet k occupies output [kComp + k*packetLen, ...); measure the
        // middle (clear of the 3 ms boundary crossfades). Skip 25 packets of
        // warmup (enable fade + latency).
        std::vector<bool> lost;
        for (int k = 25; ; ++k)
        {
            const int start = kComp + k * packetLen + 200;
            const int len   = packetLen - 400;
            if (start + len >= total)
                break;

            double e = 0.0;
            for (int i = 0; i < len; ++i) { const double x = o[start + i]; e += x * x; }
            lost.push_back (std::sqrt (e / len) < 0.05);
        }

        int numLost = 0, numRuns = 0, n1 = 0, n2 = 0, runLen = 0;
        for (size_t i = 0; i < lost.size(); ++i)
        {
            if (lost[i])
            {
                ++numLost;
                ++runLen;
                if (runLen == 1)
                    ++numRuns;
            }
            if ((! lost[i] || i + 1 == lost.size()) && runLen > 0)
            {
                if (runLen == 1) ++n1;
                if (runLen == 2) ++n2;
                runLen = 0;
            }
        }

        const double lostFrac = (double) numLost / (double) juce::jmax ((size_t) 1, lost.size());
        const double rHat     = numLost > 0 ? (double) (numLost - numRuns) / (double) numLost : -1.0;
        const double n1n2     = n2 > 0 ? (double) n1 / (double) n2 : -1.0;

        const bool live    = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool enough  = numRuns >= 100 && numLost >= 150;
        const bool fracOk  = lostFrac >= 0.19 && lostFrac <= 0.35;          // expected 0.268
        const bool rOk     = rHat >= 0.30 && rHat <= 0.63;                  // expected 0.472
        const bool histOk  = n2 > 0 && n1n2 >= 1.2 && n1n2 <= 4.5;          // expected ~2.1

        check ("O DSP-04 ge-statistics", live && enough && fracOk && rOk && histOk,
               juce::String ((int) lost.size()) + " packets, lostFrac "
                   + juce::String (lostFrac, 3) + " (exp 0.268, [0.19,0.35]), r^ "
                   + juce::String (rHat, 3) + " (exp 0.472, [0.30,0.63]), N1/N2 "
                   + juce::String (n1n2, 2) + " (exp 2.1, [1.2,4.5]), runs "
                   + juce::String (numRuns)
                   + (enough ? "" : " — TOO FEW LOSSES, probe vacuous"));
    }

    //==========================================================================
    // P — DSP-04 concealment distinctness. Same seed/schedule (the loss
    // pattern is independent of PACKET_CONCEAL — the chain draws
    // unconditionally), periodic input: the four modes must render pairwise
    // distinct. Substitute may auto-degrade to Decay when AMDF finds no
    // periodicity — on a clean sine it must not, but if it does, FLAG
    // explicitly and pass on 3-distinct (PLAN: never silently pass).
    {
        auto render = [&] (int concealMode, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",    0.0f);
            setParam (*p, "CD_ENABLE",      0.0f);
            setParam (*p, "VINYL_ENABLE",   0.0f);
            setParam (*p, "PACKET_ENABLE",  1.0f);
            setParam (*p, "PACKET_CONCEAL", (float) concealMode);
            setParam (*p, "PACKET_LOSS",    60.0f);
            setParam (*p, "PACKET_BURST",   60.0f);
            setParam (*p, "SEED",           4242.0f);
            renderInto (*p, out, total, { 512 }, sineStereo);
        };

        const int total = 192000;   // 4 s
        juce::AudioBuffer<float> modes[4];
        for (int m = 0; m < 4; ++m)
            render (m, modes[m], total);

        bool   pairOk[4][4] = {};
        double subVsDecay   = 0.0;
        bool   allOthers    = true;

        for (int a2 = 0; a2 < 4; ++a2)
        {
            for (int b2 = a2 + 1; b2 < 4; ++b2)
            {
                const double d = maxAbsDiff (modes[a2], modes[b2]);
                pairOk[a2][b2] = d > 1.0e-3;

                if (a2 == 2 && b2 == 3)
                    subVsDecay = d;
                else if (! pairOk[a2][b2])
                    allOthers = false;
            }
        }

        const bool live       = modes[0].getMagnitude (0, 0, total) > 1.0e-4f;
        const bool subDistinct = pairOk[2][3];

        if (! subDistinct)
            std::printf ("  [FLAG] Substitute degraded to Decay on the sine test signal "
                         "(AMDF found no periodicity) — DSP-04 'four audibly distinct "
                         "modes' needs verify re-scope.\n");

        check ("P DSP-04 conceal-distinct", live && allOthers,
               juce::String (subDistinct ? "4 modes pairwise distinct"
                                         : "3 modes distinct + Substitute==Decay FLAGGED")
                   + ", Substitute-vs-Decay maxAbsDiff " + juce::String (subVsDecay, 5)
                   + (allOthers ? "" : " — NON-SUBSTITUTE PAIR IDENTICAL (loss dead or "
                                       "modes not reaching DSP)"));
    }

    //==========================================================================
    // Q — QUAL-02 with the packet grid ACTIVE: high loss + Decay concealment
    // + all transport families rolling, across block-size regimes.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 10.0f);
            setParam (proc, "TAPE_PROB",       50.0f);
            setParam (proc, "CD_PROB",         50.0f);
            setParam (proc, "VINYL_PROB",      50.0f);
            setParam (proc, "PACKET_ENABLE",   1.0f);
            setParam (proc, "PACKET_LOSS",     60.0f);
            setParam (proc, "PACKET_BURST",    50.0f);
            setParam (proc, "PACKET_CONCEAL",  2.0f);   // Decay
            setParam (proc, "SEED",            777.0f);
        };

        const int total = 32768;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);
        auto c = makeProc();  configure (*c);

        juce::AudioBuffer<float> outA, outB, outC;
        renderInto (*a, outA, total, { 512 },              noiseStereo);
        renderInto (*b, outB, total, { 4096 },             noiseStereo);
        renderInto (*c, outC, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outA, outB);
            const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("Q QUAL-02 packet-512-4096", identical && live,
                   (identical ? juce::String ("packet grid active: bit-identical by memcmp")
                              : firstDifference (outA, outB))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }

        {
            const bool identical = bitIdentical (outB, outC);
            const bool live      = outC.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("Q QUAL-02 packet-ragged", identical && live,
                   (identical ? juce::String ("packet grid active, ragged: bit-identical")
                              : firstDifference (outB, outC))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // O2 — v1.2 knob-zero clean: PACKET_ENABLE on with PACKET_LOSS 0 must
    // lose NOTHING (the old mapping's unscaled 1% Good-state floor fired one
    // uninvited dropout every ~2 s while merely enabled). Silence conceal so
    // any loss collapses an RMS window; 30 s, zero lost windows required.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",    0.0f);
        setParam (*p, "CD_ENABLE",      0.0f);
        setParam (*p, "VINYL_ENABLE",   0.0f);
        setParam (*p, "PACKET_ENABLE",  1.0f);
        setParam (*p, "PACKET_CONCEAL", 0.0f);   // Silence
        setParam (*p, "PACKET_LOSS",    0.0f);
        setParam (*p, "PACKET_BURST",   30.0f);
        setParam (*p, "SEED",           4242.0f);

        const int packetLen = (int) std::ceil (0.020 * kFs);
        const int total     = 1500 * packetLen;                // 30 s
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 4096 }, noiseStereo);

        const auto* o = out.getReadPointer (0);

        int numLost = 0, numPackets = 0;
        for (int k = 25; ; ++k)
        {
            const int start = kComp + k * packetLen + 200;
            const int len   = packetLen - 400;
            if (start + len >= total)
                break;

            double e = 0.0;
            for (int i = 0; i < len; ++i) { const double x = o[start + i]; e += x * x; }
            if (std::sqrt (e / len) < 0.05)
                ++numLost;
            ++numPackets;
        }

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;
        check ("O2 packet-loss-zero clean", live && numPackets > 1400 && numLost == 0,
               juce::String (numLost) + " lost of " + juce::String (numPackets)
                   + " packets at PACKET_LOSS=0 (must be 0)"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // O3 — v1.2 full-knob true failure: PACKET_LOSS 100 / BURST 70 must
    // actually drop the call. New mapping: piB = 0.95 -> pGB clamps at 1 so
    // Bad occupancy = 1/(1+pBG) = 0.855; pBad = 1.0, pGood = 0.90 =>
    // E[lostFrac] = 0.855*1.0 + 0.145*0.90 = 0.986 (the old mapping read
    // ~0.30 here). Require >= 0.93; require < 1.0 so the probe stays honest
    // about the surviving fragments (P(all 3000 lost) ~ e^-43).
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",    0.0f);
        setParam (*p, "CD_ENABLE",      0.0f);
        setParam (*p, "VINYL_ENABLE",   0.0f);
        setParam (*p, "PACKET_ENABLE",  1.0f);
        setParam (*p, "PACKET_CONCEAL", 0.0f);   // Silence
        setParam (*p, "PACKET_LOSS",    100.0f);
        setParam (*p, "PACKET_BURST",   70.0f);
        setParam (*p, "SEED",           4242.0f);

        const int packetLen = (int) std::ceil (0.020 * kFs);
        const int total     = 3000 * packetLen;                // 60 s
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 4096 }, noiseStereo);

        const auto* o = out.getReadPointer (0);

        int numLost = 0, numPackets = 0;
        for (int k = 25; ; ++k)
        {
            const int start = kComp + k * packetLen + 200;
            const int len   = packetLen - 400;
            if (start + len >= total)
                break;

            double e = 0.0;
            for (int i = 0; i < len; ++i) { const double x = o[start + i]; e += x * x; }
            if (std::sqrt (e / len) < 0.05)
                ++numLost;
            ++numPackets;
        }

        const double lostFrac = (double) numLost / (double) juce::jmax (1, numPackets);
        const bool   live     = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("O3 packet-loss-full ~95%", live && lostFrac >= 0.93 && lostFrac < 1.0,
               "lostFrac " + juce::String (lostFrac, 3)
                   + " (exp 0.986, require [0.93,1.0)) over "
                   + juce::String (numPackets) + " packets"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // P2 — v1.2 Decay mutes out: twin renders, same seed, so the Silence
    // render's loss schedule is a mask for the Decay render (the GE chain
    // draws unconditionally, independent of PACKET_CONCEAL — proven by P).
    // Within every lost run of length >= 5: packet 1 must be audible
    // (concealment ramping 0 -> -6 dB) and packets 4+ must be SILENT (the
    // -6 dB/rep ramp hard-floors to 0 by the end of rep 3, ~60 ms). The old
    // -3 dB/rep floorless decay reads packet 6 at ~ -18 dB, loudly nonzero.
    {
        auto render = [&] (int concealMode, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",    0.0f);
            setParam (*p, "CD_ENABLE",      0.0f);
            setParam (*p, "VINYL_ENABLE",   0.0f);
            setParam (*p, "PACKET_ENABLE",  1.0f);
            setParam (*p, "PACKET_CONCEAL", (float) concealMode);
            setParam (*p, "PACKET_LOSS",    60.0f);
            setParam (*p, "PACKET_BURST",   100.0f);   // E[B] = 8: long bursts
            setParam (*p, "SEED",           4242.0f);
            renderInto (*p, out, total, { 4096 }, sineStereo);
        };

        const int packetLen = (int) std::ceil (0.020 * kFs);
        const int total     = 3000 * packetLen;                // 60 s
        juce::AudioBuffer<float> maskBuf, decayBuf;
        render (0, maskBuf, total);
        render (2, decayBuf, total);

        const auto* om = maskBuf.getReadPointer (0);
        const auto* od = decayBuf.getReadPointer (0);

        auto winRms = [&] (const float* o, int k)
        {
            const int start = kComp + k * packetLen + 200;
            const int len   = packetLen - 400;
            double e = 0.0;
            for (int i = 0; i < len; ++i) { const double x = o[start + i]; e += x * x; }
            return std::sqrt (e / len);
        };

        const int lastK = (total - kComp - 200) / packetLen - 1;

        int runs = 0, badFirst = 0, badTail = 0;
        for (int k = 25; k <= lastK; ++k)
        {
            if (winRms (om, k) >= 0.05)
                continue;

            int runLen = 0;                        // measure the lost run at k
            while (k + runLen <= lastK && winRms (om, k + runLen) < 0.05)
                ++runLen;

            // Only count runs whose START is proven (previous window good) —
            // otherwise k may be mid-run and the rep indices misalign.
            const bool startKnown = winRms (om, k - 1) >= 0.05;

            if (startKnown && runLen >= 5)
            {
                ++runs;
                if (winRms (od, k) <= 0.02)        // rep 1: must be audible
                    ++badFirst;
                for (int j = 3; j < runLen; ++j)   // reps 4+: must be silent
                    if (winRms (od, k + j) > 1.0e-4)
                        { ++badTail; break; }
            }

            k += runLen - 1;
        }

        const bool live   = decayBuf.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool enough = runs >= 10;

        check ("P2 decay-mutes-by-60ms", live && enough && badFirst == 0 && badTail == 0,
               juce::String (runs) + " runs >= 5 lost packets; audible-rep-1 failures "
                   + juce::String (badFirst) + ", silent-rep-4+ failures "
                   + juce::String (badTail)
                   + (enough ? "" : " — TOO FEW LONG RUNS, probe vacuous")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // R — DSP-06 CRUSH_RATE: liveness + zipper-free sweep. A crushed signal
    // is inherently steppy, so a raw sample-delta bound is vacuous — instead
    // the HOLD-INTERVAL TRACE (spacing between output value changes) must
    // GLIDE across a full-range param step. The 50 ms per-sample-smoothed
    // target bounds consecutive interval ratios (< ~1.7 analytically); an
    // unsmoothed 20 kHz -> 500 Hz step reads ratio ~40.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",   0.0f);
        setParam (*p, "CD_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",  0.0f);
        setParam (*p, "CRUSH_ENABLE",  1.0f);
        setParam (*p, "CRUSH_RATE",    20000.0f);

        const int total = 96000;
        juce::AudioBuffer<float> out;
        renderWithStep (*p, out, total, 512, sineStereo, 24000, "CRUSH_RATE", 500.0f);

        const auto* o = out.getReadPointer (0);
        const int startAt = kComp + (int) (0.2 * kFs);

        std::vector<int> intervals;
        int lastChange = -1;
        for (int n = startAt + 1; n < total; ++n)
        {
            if (! bitExact (o[n], o[n - 1]))
            {
                if (lastChange >= 0)
                    intervals.push_back (n - lastChange);
                lastChange = n;
            }
        }

        int    minIv = 1 << 30, maxIv = 0;
        double worstRatio = 1.0;
        for (size_t i = 1; i < intervals.size(); ++i)
        {
            minIv = juce::jmin (minIv, intervals[i]);
            maxIv = juce::jmax (maxIv, intervals[i]);

            const int a2 = intervals[i - 1], b2 = intervals[i];
            if (a2 >= 4 && a2 <= 300 && b2 >= 4 && b2 <= 300)
                worstRatio = juce::jmax (worstRatio,
                                         (double) juce::jmax (a2, b2) / (double) juce::jmin (a2, b2));
        }

        const bool live      = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool spanned   = minIv <= 4 && maxIv >= 80;    // sweep actually moved the DSP
        const bool noZipper  = worstRatio <= 2.5;

        check ("R DSP-06 rate-sweep", live && spanned && noZipper,
               juce::String ("hold intervals [") + juce::String (minIv) + ", " + juce::String (maxIv)
                   + "] samples, worst consecutive ratio " + juce::String (worstRatio, 3)
                   + " (bound 2.5; unsmoothed step ~40)"
                   + (spanned ? "" : " — PARAM DID NOT MOVE THE DSP, probe vacuous"));
    }

    //==========================================================================
    // R2 — DSP-06 CRUSH_BITS: liveness + smoothed staircase. On DC input a
    // hard (unsmoothed) 16 -> 2 bits step yields exactly TWO output values;
    // the per-sample-smoothed target glides delta through many intermediate
    // quantization levels.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",   0.0f);
        setParam (*p, "CD_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",  0.0f);
        setParam (*p, "CRUSH_ENABLE",  1.0f);
        setParam (*p, "CRUSH_BITS",    16.0f);

        const int total = 48000;
        juce::AudioBuffer<float> out;
        renderWithStep (*p, out, total, 512, dcStereo, 24000, "CRUSH_BITS", 2.0f);

        const auto* o = out.getReadPointer (0);

        const float before = o[24000 + kComp - 200];
        const float after  = o[total - 100];

        int changes = 0;
        for (int n = 24000 + kComp - 100 + 1; n < 24000 + kComp + 9600; ++n)
            if (! bitExact (o[n], o[n - 1]))
                ++changes;

        const bool moved    = std::abs ((double) after - (double) before) > 0.05;   // liveness
        const bool staired  = changes >= 2;                                          // >= 3 levels
        const bool finiteOk = allFiniteRange (o, 0, total);

        check ("R2 DSP-06 bits-staircase", moved && staired && finiteOk,
               juce::String ("DC 0.3: ") + juce::String (before, 5) + " -> " + juce::String (after, 5)
                   + " through " + juce::String (changes + 1) + " levels (hard step = 2)"
                   + (moved ? "" : " — BITS DID NOT MOVE THE DSP, probe vacuous"));
    }

    //==========================================================================
    // R3 — DSP-06 fractional rate, no warble: at 6857 Hz (fs/7.0001 — the
    // classic integer-latch beat position) the interpolated hold keeps the
    // steady-state envelope flat.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",   0.0f);
        setParam (*p, "CD_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",  0.0f);
        setParam (*p, "CRUSH_ENABLE",  1.0f);
        setParam (*p, "CRUSH_RATE",    6857.0f);

        const int total = 72000;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);
        const int win = (int) (0.025 * kFs);   // 25 ms (5.5 sine cycles)

        std::vector<double> winRms;
        for (int w = (int) (0.3 * kFs); w + win <= total; w += win)
        {
            double e = 0.0;
            for (int i = 0; i < win; ++i) { const double x = o[w + i]; e += x * x; }
            winRms.push_back (std::sqrt (e / win));
        }

        const double mn = *std::min_element (winRms.begin(), winRms.end());
        const double mx = *std::max_element (winRms.begin(), winRms.end());
        const bool   live   = medianOf (winRms) > 0.1;
        const bool   flat   = mn > 0.0 && mx / mn <= 1.3;

        check ("R3 DSP-06 fractional-rate", live && flat,
               juce::String ("25ms RMS envelope max/min ") + juce::String (mx / juce::jmax (1.0e-12, mn), 3)
                   + " (bound 1.3 — integer-latch warble beats)"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // S — DSP-07 duck vs pump. Burst/tail signal, quantization-error energy
    // per segment measured against the env-amt-0 reference render. Duck (-)
    // concentrates error in the QUIET tails (errQ/errL >> 1); pump (+)
    // concentrates it in the LOUD transients (errQ/errL << 1). The
    // cross-segment ratio is used (not energy-normalized rates) because
    // quantization error does not scale with signal level.
    {
        auto render = [&] (float envAmt, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",   0.0f);
            setParam (*p, "CD_ENABLE",     0.0f);
            setParam (*p, "VINYL_ENABLE",  0.0f);
            setParam (*p, "CRUSH_ENABLE",  1.0f);
            setParam (*p, "CRUSH_BITS",    12.0f);
            setParam (*p, "CRUSH_ENV_AMT", envAmt);
            renderInto (*p, out, total, { 512 }, burstSine);
        };

        const int total = 192000;   // 4 x 1 s burst/tail periods
        juce::AudioBuffer<float> outRef, outDuck, outPump;
        render (0.0f,    outRef,  total);
        render (-100.0f, outDuck, total);
        render (100.0f,  outPump, total);

        auto segErr = [&] (const juce::AudioBuffer<float>& a, bool loud)
        {
            const auto* xa = a.getReadPointer (0);
            const auto* xr = outRef.getReadPointer (0);
            double acc = 0.0;
            int    cnt = 0;

            for (int period = 1; period < 4; ++period)
            {
                // Second half of each segment (follower most settled).
                const int segBase = period * 48000 + (loud ? 12000 : 36000);
                for (int i = 0; i < 12000; ++i)
                {
                    const int n = kComp + segBase + i;
                    const double d = (double) xa[n] - (double) xr[n];
                    acc += d * d;
                    ++cnt;
                }
            }
            return acc / (double) juce::jmax (1, cnt);   // mean squared error
        };

        const double duckL = segErr (outDuck, true),  duckQ = segErr (outDuck, false);
        const double pumpL = segErr (outPump, true),  pumpQ = segErr (outPump, false);

        const double duckRatio = duckQ / juce::jmax (1.0e-18, duckL);
        const double pumpRatio = pumpQ / juce::jmax (1.0e-18, pumpL);

        const bool liveD = duckQ > 1.0e-12;   // duck actually crushed the tails
        const bool liveP = pumpL > 1.0e-12;   // pump actually crushed the bursts
        const bool ok    = duckRatio > 4.0 && pumpRatio < 0.5;

        check ("S DSP-07 duck-vs-pump", liveD && liveP && ok,
               juce::String ("duck errQ/errL ") + juce::String (duckRatio, 2)
                   + " (need > 4), pump errQ/errL " + juce::String (pumpRatio, 4)
                   + " (need < 0.5)"
                   + ((liveD && liveP) ? "" : " — ENV DEPTH INERT, probe vacuous"));
    }

    //==========================================================================
    // S2 — QUAL-02 with crush + quant ACTIVE (jitter + dither streams drawing
    // per sample, per-sample follower engaged) plus transports rolling.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 10.0f);
            setParam (proc, "TAPE_PROB",       50.0f);
            setParam (proc, "CD_PROB",         50.0f);
            setParam (proc, "VINYL_PROB",      50.0f);
            setParam (proc, "CRUSH_ENABLE",    1.0f);
            setParam (proc, "CRUSH_BITS",      6.0f);
            setParam (proc, "CRUSH_RATE",      6000.0f);
            setParam (proc, "CRUSH_JITTER",    40.0f);
            setParam (proc, "CRUSH_DITHER",    1.0f);
            setParam (proc, "CRUSH_ENV_AMT",   -50.0f);
            setParam (proc, "SEED",            777.0f);
        };

        const int total = 32768;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);
        auto c = makeProc();  configure (*c);

        juce::AudioBuffer<float> outA, outB, outC;
        renderInto (*a, outA, total, { 512 },              noiseStereo);
        renderInto (*b, outB, total, { 4096 },             noiseStereo);
        renderInto (*c, outC, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outA, outB);
            const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("S2 QUAL-02 crush-512-4096", identical && live,
                   (identical ? juce::String ("crush+quant active: bit-identical by memcmp")
                              : firstDifference (outA, outB))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }

        {
            const bool identical = bitIdentical (outB, outC);
            const bool live      = outC.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("S2 QUAL-02 crush-ragged", identical && live,
                   (identical ? juce::String ("crush+quant active, ragged: bit-identical")
                              : firstDifference (outB, outC))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // T — DSP-08 dither functional: CRUSH_DITHER 0 vs 2 differ. The dither
    // draws are unconditional, so both renders share the identical stream —
    // ONLY the dither amplitude differs.
    {
        auto render = [&] (float dither, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "CRUSH_ENABLE", 1.0f);
            setParam (*p, "CRUSH_BITS",   8.0f);
            setParam (*p, "CRUSH_DITHER", dither);
            renderInto (*p, out, total, { 512 }, sineStereo);
        };

        const int total = 48000;
        juce::AudioBuffer<float> outA, outB;
        render (0.0f, outA, total);
        render (2.0f, outB, total);

        const double diff = maxAbsDiff (outA, outB);
        const bool   live = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("T DSP-08 dither-audible", live && diff > 0.003,
               juce::String ("maxAbsDiff dither-0-vs-2 = ") + juce::String (diff, 5)
                   + " (need > 0.003 = ~0.4 LSB @ 8 bits)");
    }

    //==========================================================================
    // U — QUAL-01 pathological input with crush + quant fully engaged
    // (env-driven depth ON — the follower is the sticky-NaN target): DC,
    // silence, full-scale square, 100 ms of NaN, then clean sine. The output
    // must be finite before the NaN window and must RECOVER after it —
    // finite, bounded and live.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",   0.0f);
        setParam (*p, "CD_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",  0.0f);
        setParam (*p, "CRUSH_ENABLE",  1.0f);
        setParam (*p, "CRUSH_BITS",    5.0f);
        setParam (*p, "CRUSH_RATE",    8000.0f);
        setParam (*p, "CRUSH_JITTER",  30.0f);
        setParam (*p, "CRUSH_DITHER",  1.0f);
        setParam (*p, "CRUSH_ENV_AMT", 80.0f);

        const int total = 144000;
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, pathologicalStereo);

        bool preOk = true, postOk = true;
        double postPeak = 0.0, postMag = 0.0;

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* o = out.getReadPointer (ch);

            // Before the NaN window reaches the output (input NaN starts at
            // 72000; +kComp latency; stop a little early for safety).
            preOk = preOk && allFiniteRange (o, 0, 72000 + kComp - 64);

            // Recovery region: 0.4 s after clean input resumed.
            postOk = postOk && allFiniteRange (o, 96000, total);
            for (int n = 96000; n < total; ++n)
            {
                postPeak = juce::jmax (postPeak, (double) std::abs (o[n]));
                postMag  = juce::jmax (postMag,  (double) std::abs (o[n]));
            }
        }

        const bool bounded = postPeak <= 2.0;
        const bool live    = postMag > 1.0e-4;

        check ("U QUAL-01 pathological", preOk && postOk && bounded && live,
               juce::String ("pre-NaN finite: ") + (preOk ? "yes" : "NO")
                   + ", recovery finite: " + (postOk ? "yes" : "NO")
                   + ", post peak " + juce::String (postPeak, 4) + " (bound 2.0)"
                   + (live ? "" : " — RECOVERY SILENT (sticky state?)"));
    }

    //==========================================================================
    // V — GSM ROUND-TRIP GATE (Task 16, runs BEFORE the DSP-05 probes).
    // Standalone libgsm exercise, no plugin: two handles, two continuous
    // 160-sample sine frames at the 8 kHz grid; scaling per RESEARCH 4
    // ((x*4096)<<3 masked 0xFFF8 in; (s>>3)/4096 out). Correlation is
    // measured on frame 2 (past the encoder's adaptation), generous bound —
    // GSM is lossy by design.
    {
        gsm enc = gsm_create();
        gsm dec = gsm_create();

        const bool created = enc != nullptr && dec != nullptr;
        bool   decodeOk = created;
        double maxAbs   = 0.0;
        double peakCorr = 0.0;

        if (created)
        {
            gsm_signal in[2][160];
            gsm_signal out[2][160];
            float      fin[320], fout[320];

            for (int k = 0; k < 320; ++k)
            {
                const float x = 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                                         * 440.0 * (double) k / 8000.0);
                const int v = juce::jlimit (-4095, 4095, juce::roundToIntAccurate (x * 4096.0f));
                in[k / 160][k % 160] = (gsm_signal) (v * 8);   // low 3 bits zero (0xFFF8)
                fin[k] = (float) (in[k / 160][k % 160] >> 3) / 4096.0f;
            }

            gsm_frame frm;
            for (int f = 0; f < 2; ++f)
            {
                gsm_encode (enc, in[f], frm);
                if (gsm_decode (dec, frm, out[f]) != 0)
                    decodeOk = false;                          // -1 = malformed (impossible here)
            }

            for (int k = 0; k < 320; ++k)
            {
                fout[k] = (float) (out[k / 160][k % 160] >> 3) / 4096.0f;
                maxAbs  = juce::jmax (maxAbs, (double) std::abs (fout[k]));
            }

            // Normalized correlation, frame 2, peak over lags 0..40.
            double e0 = 0.0;
            for (int n = 160; n < 280; ++n) e0 += (double) fin[n] * fin[n];

            for (int lag = 0; lag <= 40; ++lag)
            {
                double acc = 0.0, e1 = 0.0;
                for (int n = 160; n < 280; ++n)
                {
                    acc += (double) fin[n] * fout[n + lag];
                    e1  += (double) fout[n + lag] * fout[n + lag];
                }
                peakCorr = juce::jmax (peakCorr, acc / (std::sqrt (e0 * e1) + 1.0e-12));
            }
        }

        if (enc != nullptr) gsm_destroy (enc);
        if (dec != nullptr) gsm_destroy (dec);

        const bool bounded = maxAbs > 0.01 && maxAbs <= 1.5 && std::isfinite (maxAbs);

        check ("V GSM round-trip gate", created && decodeOk && bounded && peakCorr > 0.4,
               juce::String ("handles ") + (created ? "ok" : "NULL")
                   + ", decode " + (decodeOk ? "ok" : "FAILED")
                   + ", |out| max " + juce::String (maxAbs, 3)
                   + ", peak corr " + juce::String (peakCorr, 3) + " (need > 0.4)");
    }

    //==========================================================================
    // W — DSP-05 mu-law bandwidth: noise through the phone chain vs the
    // codec-off control on the same input. Energy above 8 kHz collapses
    // (post-LPF kills the 8 kHz hold images), the 300-3400 passband
    // survives, sub-150 Hz is HPF-filtered.
    {
        auto render = [&] (bool codecOn, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "CODEC_ENABLE", codecOn ? 1.0f : 0.0f);
            setParam (*p, "CODEC_MODE",   0.0f);    // Mu-law
            setParam (*p, "CODEC_MIX",    100.0f);
            renderInto (*p, out, total, { 512 }, noiseStereo);
        };

        const int total = 48000;
        juce::AudioBuffer<float> outOn, outOff;
        render (true,  outOn,  total);
        render (false, outOff, total);

        const auto on  = channelToVector (outOn, 0);
        const auto off = channelToVector (outOff, 0);
        const int  seg = 24000;

        const double hiRatio   = bandEnergy (on, seg, 8000.0, 20000.0)
                               / juce::jmax (1.0e-12, bandEnergy (off, seg, 8000.0, 20000.0));
        const double passRatio = bandEnergy (on, seg, 500.0, 3000.0)
                               / juce::jmax (1.0e-12, bandEnergy (off, seg, 500.0, 3000.0));
        const double loRatio   = bandEnergy (on, seg, 30.0, 150.0)
                               / juce::jmax (1.0e-12, bandEnergy (off, seg, 30.0, 150.0));

        const bool live = outOn.getMagnitude (0, 0, total) > 1.0e-3f;

        check ("W DSP-05 mulaw-band", live && hiRatio < 0.05 && passRatio > 0.1 && loRatio < 0.2,
               juce::String (">8k ratio ") + juce::String (hiRatio, 4) + " (<0.05), passband "
                   + juce::String (passRatio, 3) + " (>0.1), <150Hz " + juce::String (loRatio, 4)
                   + " (<0.2)" + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // X — DSP-05 mu-law level-dependent noise: distortion/quantization
    // energy outside the 220 Hz fundamental tracks the signal level (the
    // companding property; a LINEAR quantizer's noise floor would not move).
    {
        auto render = [&] (InputFn input, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "CODEC_ENABLE", 1.0f);
            setParam (*p, "CODEC_MODE",   0.0f);
            setParam (*p, "CODEC_MIX",    100.0f);
            renderInto (*p, out, total, { 512 }, input);
        };

        const int total = 48000;
        juce::AudioBuffer<float> outLoud, outQuiet;
        render (sineStereo,      outLoud,  total);   // amp 0.5
        render (sineQuietStereo, outQuiet, total);   // amp 0.05 (-20 dB)

        const auto loud  = channelToVector (outLoud, 0);
        const auto quiet = channelToVector (outQuiet, 0);

        const double nLoud  = bandEnergy (loud,  24000, 1000.0, 3800.0);
        const double nQuiet = bandEnergy (quiet, 24000, 1000.0, 3800.0);

        const bool live = nLoud > 1.0e-10;

        check ("X DSP-05 mulaw-level-noise", live && nQuiet < 0.3 * nLoud,
               juce::String ("noise-band energy quiet/loud ")
                   + juce::String (nQuiet / juce::jmax (1.0e-18, nLoud), 4)
                   + " (need < 0.3 — noise must track level)"
                   + (live ? "" : " — NO DISTORTION ENERGY, probe vacuous"));
    }

    //==========================================================================
    // Y — DSP-05 GSM alignment: both the GSM chain and the plain delay
    // present kCompLatency, so the cross-correlation peak between the two
    // renders must sit within +/- one 8 kHz grid period (fs/8000 = 6) of 0.
    // Input is a PASSBAND multi-tone, not broadband noise: the reference is
    // full-band, and GSM keeps only ~300-3400 Hz of it, so noise starves the
    // normalized correlation (~0.1) without any alignment defect. Three
    // non-commensurate tones give a unique xcorr peak inside the +/-60-lag
    // search window (a single sine's periodic peaks would alias the lag).
    {
        InputFn tones = [] (int ch, int n) noexcept -> float
        {
            juce::ignoreUnused (ch);
            const double t = static_cast<double> (n) / kFs;
            const double twoPi = 2.0 * juce::MathConstants<double>::pi;
            return 0.2f * static_cast<float> (std::sin (twoPi * 600.0  * t)
                                            + std::sin (twoPi * 1450.0 * t)
                                            + std::sin (twoPi * 3100.0 * t));
        };

        auto render = [&] (bool gsmOn, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "CODEC_ENABLE", gsmOn ? 1.0f : 0.0f);
            setParam (*p, "CODEC_MODE",   1.0f);    // GSM
            setParam (*p, "CODEC_MIX",    100.0f);
            renderInto (*p, out, total, { 512 }, tones);
        };

        const int total = 72000;
        juce::AudioBuffer<float> outGsm, outRef;
        render (true,  outGsm, total);
        render (false, outRef, total);

        const auto a = channelToVector (outGsm, 0);
        const auto b = channelToVector (outRef, 0);

        double corr = 0.0;
        const int lag = xcorrPeakLag (a, b, 36000, 24000, 60, corr);

        /* Bound: one 8 kHz grid period (fs/8000 = 6) of latch jitter PLUS
           the codec path's IIR group delay — three 4-pole Butterworth
           cascades contribute ~12-15 samples of passband group delay the
           plain-delay reference does not have (minimum-phase filter delay,
           not reported latency). +/-24 tolerates that while still catching
           every frame-bookkeeping error, which manifests at +/-160 grid
           samples (+/-960) or as a 0..959-varying smear that kills the
           correlation peak. */
        const int  gridPeriod = (int) std::ceil (kFs / 8000.0);
        const int  bound      = 4 * gridPeriod;
        const bool aligned    = std::abs (lag) <= bound;
        const bool corrLive   = corr > 0.1;

        check ("Y DSP-05 gsm-alignment", aligned && corrLive,
               juce::String ("xcorr peak lag ") + juce::String (lag) + " (bound +/-"
                   + juce::String (bound) + " = grid jitter + IIR group delay), corr "
                   + juce::String (corr, 3)
                   + (corrLive ? "" : " — CORRELATION DEAD, probe vacuous"));
    }

    //==========================================================================
    // Z — latency report never renegotiates: kCompLatency in every codec
    // state, after audio has run.
    {
        bool ok = true;
        juce::String detail;

        const float modes[3][2] = { { 0.0f, 0.0f },    // off
                                    { 1.0f, 0.0f },    // mu-law
                                    { 1.0f, 1.0f } };  // gsm
        const char* names[3] = { "off", "mulaw", "gsm" };

        for (int m = 0; m < 3; ++m)
        {
            auto p = makeProc();
            setParam (*p, "CODEC_ENABLE", modes[m][0]);
            setParam (*p, "CODEC_MODE",   modes[m][1]);

            juce::AudioBuffer<float> out;
            renderInto (*p, out, 8192, { 512 }, sineStereo);

            const int reported = p->getLatencySamples();
            if (reported != kComp)
                ok = false;
            detail << names[m] << "=" << reported << " ";
        }

        check ("Z latency-all-modes", ok,
               detail + "(expected " + juce::String (kComp) + " everywhere)");
    }

    //==========================================================================
    // Z2 — QUAL-02 with the codec ACTIVE, both modes: 512-vs-4096 memcmp
    // bit-identity (the 8 kHz latch phase, frame slots and fades are all
    // pure functions of the sample count).
    for (int mode = 0; mode < 2; ++mode)
    {
        auto configure = [mode] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 10.0f);
            setParam (proc, "TAPE_PROB",       50.0f);
            setParam (proc, "CD_PROB",         50.0f);
            setParam (proc, "VINYL_PROB",      50.0f);
            setParam (proc, "CODEC_ENABLE",    1.0f);
            setParam (proc, "CODEC_MODE",      (float) mode);
            setParam (proc, "CODEC_MIX",       100.0f);
            setParam (proc, "SEED",            777.0f);
        };

        const int total = 32768;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);

        juce::AudioBuffer<float> outA, outB;
        renderInto (*a, outA, total, { 512 },  noiseStereo);
        renderInto (*b, outB, total, { 4096 }, noiseStereo);

        const bool identical = bitIdentical (outA, outB);
        const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check (mode == 0 ? "Z2 QUAL-02 codec-mulaw" : "Z2 QUAL-02 codec-gsm",
               identical && live,
               (identical ? juce::String ("codec active: bit-identical by memcmp")
                          : firstDifference (outA, outB))
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // P1 — PERF-01, MEASURED (the one sanctioned wall-clock use, per PLAN:
    // the measurement is printed; the verdict is a single ratio bound).
    // Worst case: all transport families at 100%, packet loss with the AMDF
    // Substitute concealment, crush + quant with jitter/dither/env, GSM
    // codec. 10 s of audio at 48 kHz, 512-sample blocks. NOTE: only
    // meaningful in a Release build.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 10.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "VINYL_PROB",      100.0f);
        setParam (*p, "PACKET_ENABLE",   1.0f);
        setParam (*p, "PACKET_LOSS",     60.0f);
        setParam (*p, "PACKET_BURST",    50.0f);
        setParam (*p, "PACKET_CONCEAL",  3.0f);    // Substitute (AMDF, worst)
        setParam (*p, "CRUSH_ENABLE",    1.0f);
        setParam (*p, "CRUSH_BITS",      6.0f);
        setParam (*p, "CRUSH_RATE",      6000.0f);
        setParam (*p, "CRUSH_JITTER",    50.0f);
        setParam (*p, "CRUSH_DITHER",    1.0f);
        setParam (*p, "CRUSH_ENV_AMT",   -50.0f);
        setParam (*p, "CODEC_ENABLE",    1.0f);
        setParam (*p, "CODEC_MODE",      1.0f);    // GSM
        setParam (*p, "CODEC_MIX",       100.0f);
        setParam (*p, "SEED",            999.0f);

        const int total = 480000;                  // 10 s @ 48 kHz
        juce::AudioBuffer<float> out;

        const auto t0 = std::chrono::steady_clock::now();
        renderInto (*p, out, total, { 512 }, noiseStereo);
        const auto t1 = std::chrono::steady_clock::now();

        const double renderSeconds = std::chrono::duration<double> (t1 - t0).count();
        const double audioSeconds  = total / kFs;
        const double ratio         = renderSeconds / audioSeconds;

        std::printf ("  [PERF] worst-case render: %.3f s for %.1f s of audio — ratio %.4f "
                     "(includes harness input-gen/copy overhead)\n",
                     renderSeconds, audioSeconds, ratio);

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("P1 PERF-01 cpu-ratio", live && ratio <= 0.15,
               juce::String ("ratio ") + juce::String (ratio, 4) + " (bound 0.15)"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // M1 — v1.1.0 mono->mono FUNC-02 null. Same delay-compensated bit-exact
    // null as B, on a 1-channel buffer through the mono main-bus layout.
    {
        auto p = makeProcLayout (1, 1);
        setParam (*p, "TAPE_ENABLE",  0.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 0.0f);

        const int total = 48000;
        juce::AudioBuffer<float> out;
        renderIntoLayout (*p, out, total, { 512 }, noiseDualMono, 1, 1);

        const int  startAt = kComp + (int) (0.2 * kFs);
        bool       ok      = true;
        juce::String detail;

        const auto* o = out.getReadPointer (0);
        for (int n = startAt; n < total; ++n)
        {
            if (! bitExact (o[n], noiseDualMono (0, n - kComp)))
            {
                ok = false;
                detail = juce::String ("first mismatch @") + juce::String (n)
                       + " (" + juce::String (o[n], 9) + " vs "
                       + juce::String (noiseDualMono (0, n - kComp), 9) + ")";
                break;
            }
        }

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;
        if (ok && live)
            detail = juce::String ("bit-exact null over [")
                   + juce::String (startAt) + ", " + juce::String (total) + ")";

        check ("M1 mono-null", ok && live,
               detail + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // M2 — mono->mono engine equivalence: ch0 of a (1,1) run must be
    // BIT-IDENTICAL to ch0 of a (2,2) run fed dual-mono input. The mono path
    // still computes the full L/R engine (wetR discarded), so every RNG draw
    // aligns; any divergence means the mono adapter changed the engine.
    // Heavy settings: all transport families forced, packet Substitute
    // concealment, crush + jitter/dither/env, GSM codec.
    {
        auto setHeavy = [] (OBitrotAudioProcessor& p)
        {
            setParam (p, "CLOCK_MODE",      1.0f);
            setParam (p, "CLOCK_FREE_RATE", 10.0f);
            setParam (p, "TAPE_PROB",       100.0f);
            setParam (p, "CD_PROB",         100.0f);
            setParam (p, "VINYL_PROB",      100.0f);
            setParam (p, "PACKET_ENABLE",   1.0f);
            setParam (p, "PACKET_LOSS",     60.0f);
            setParam (p, "PACKET_BURST",    50.0f);
            setParam (p, "PACKET_CONCEAL",  3.0f);
            setParam (p, "CRUSH_ENABLE",    1.0f);
            setParam (p, "CRUSH_BITS",      6.0f);
            setParam (p, "CRUSH_RATE",      6000.0f);
            setParam (p, "CRUSH_JITTER",    50.0f);
            setParam (p, "CRUSH_DITHER",    1.0f);
            setParam (p, "CRUSH_ENV_AMT",   -50.0f);
            setParam (p, "CODEC_ENABLE",    1.0f);
            setParam (p, "CODEC_MODE",      1.0f);
            setParam (p, "CODEC_MIX",       100.0f);
            setParam (p, "SEED",            999.0f);
        };

        auto pm = makeProcLayout (1, 1);
        auto ps = makeProc();
        setHeavy (*pm);
        setHeavy (*ps);

        const int total = 96000;                   // 2 s @ 48 kHz
        juce::AudioBuffer<float> outM, outS;
        renderIntoLayout (*pm, outM, total, { 512 }, noiseDualMono, 1, 1);
        renderInto       (*ps, outS, total, { 512 }, noiseDualMono);

        const bool identical = std::memcmp (outM.getReadPointer (0), outS.getReadPointer (0),
                                            sizeof (float) * (size_t) total) == 0;
        const bool live = outM.getMagnitude (0, 0, total) > 1.0e-4f;

        juce::String detail = identical
            ? juce::String ("mono ch0 bit-identical to dual-mono stereo ch0 by memcmp")
            : [&]
              {
                  for (int n = 0; n < total; ++n)
                      if (! bitExact (outM.getSample (0, n), outS.getSample (0, n)))
                          return juce::String ("first diff @") + juce::String (n)
                               + " (" + juce::String (outM.getSample (0, n), 9) + " vs "
                               + juce::String (outS.getSample (0, n), 9) + ")";
                  return juce::String ("identical");
              }();

        check ("M2 mono-vs-stereo-engine", identical && live,
               detail + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // M3 — mono->stereo FUNC-02 null. Buffer is 2-wide but the INPUT bus is
    // mono; the harness junk-fills ch1 (0.777f), which the processor must
    // discard (clear + copy ch0). BOTH output channels bit-equal the delayed
    // mono input.
    {
        auto p = makeProcLayout (1, 2);
        setParam (*p, "TAPE_ENABLE",  0.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 0.0f);

        const int total = 48000;
        juce::AudioBuffer<float> out;
        renderIntoLayout (*p, out, total, { 512 }, noiseDualMono, 1, 2);

        const int  startAt = kComp + (int) (0.2 * kFs);
        bool       ok      = true;
        juce::String detail;

        for (int ch = 0; ch < 2 && ok; ++ch)
        {
            const auto* o = out.getReadPointer (ch);
            for (int n = startAt; n < total; ++n)
            {
                if (! bitExact (o[n], noiseDualMono (0, n - kComp)))
                {
                    ok = false;
                    detail = juce::String ("first mismatch ch") + juce::String (ch)
                           + " @" + juce::String (n)
                           + " (" + juce::String (o[n], 9) + " vs "
                           + juce::String (noiseDualMono (0, n - kComp), 9) + ")";
                    break;
                }
            }
        }

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;
        if (ok && live)
            detail = juce::String ("both channels bit-exact null over [")
                   + juce::String (startAt) + ", " + juce::String (total) + ")";

        check ("M3 mono-to-stereo-null", ok && live,
               detail + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // ── v1.4.0: improvement brief items 2, 3 and 11 ──────────────────────────
    //==========================================================================

    //==========================================================================
    // V1 — the cross-version bit-identity gate. Both v1.4.0 additions default
    // to 0, and at 0 they are claimed to be EXACTLY transparent: the wow bed
    // returns an offset of 0.0 (and `pos - 0.0` is bit-identical to `pos`, so
    // the exact-integer read survives), and the dropout roll is short-circuited
    // before it can consume a draw from the tape stream, so the bend sequence
    // is unchanged.
    //
    // The digest below was produced by THIS probe compiled against the v1.3.0
    // tree (git a22ff7c3) — see the CHANGELOG's "Render-affecting" note. It is
    // the only way a harness that cannot link the old engine can assert
    // "bit-identical to the previous version"; a probe that merely re-rendered
    // the new engine twice would pass no matter what changed.
    //
    // RE-ANCHORED at v1.6.0, and the reason is the finding, not a workaround.
    // v1.6.0 is the first release that changes the render at a DEFAULT: the
    // CD mute rung's ceiling now scales with CD_SEVERITY, and the baseline
    // severity is 0.5, so the old canonical render legitimately moved
    // (0x3ee4e028900e47ca -> 0xef4d37a476695d6a). The probe therefore pins
    // CD_SEVERITY 0 — the one severity at which v1.6.0 claims to be exactly
    // transparent, because there the rung roll can only reach conceal, the
    // sector lock and the servo seek are both below their thresholds, and
    // every jump still takes ReadHead's default fade.
    //
    // RE-ANCHORED AGAIN at v1.7.0, and narrowed a second time, for the same
    // kind of reason: v1.7.0 deliberately moves the VINYL family's render at
    // its defaults. Two independent changes do it, and neither is a
    // regression — the pop taxonomy widened triggerPop from two artifactSynth
    // draws to five (so every pop after the first differs even at an unchanged
    // level), and forward groove jumps became revolution-quantized. The
    // canonical render runs VINYL_PROB 60, so it legitimately moved
    // (0x972a5d3807538393 -> 0xf8c2080db4e69ec1). The probe therefore pins
    // VINYL_ENABLE 0 as well.
    //
    // What survives is still the claim worth having, now over four releases:
    // the tape bends, the post-stop recovery jump, the CD conceal rung and the
    // lag-overflow clamp are bit-unchanged, and — since VINYL_WARP is the
    // v1.7.0 addition that is claimed to be exactly transparent at its default
    // — so is the read path with the warp wired into it but silent. The vinyl
    // transport's own invariants move to M/M4/M5/M8, which assert the
    // revolution grid directly rather than through a digest.
    //
    // The digest was recorded the same way both times: by compiling THIS probe
    // — with the CD_SEVERITY 0 and VINYL_ENABLE 0 lines — against the previous
    // tree (16e63620, v1.6.0) in a detached worktree and reading the number it
    // printed. That run scored 86/87: the one failure is V1 itself, measuring
    // the freshly narrowed render against the anchor the narrowing had just
    // superseded, which is precisely the run that produces the new number. All
    // 86 other probes passing is what says the old tree was otherwise intact.
    //
    // RE-ANCHORED A THIRD TIME at v1.9.0 (brief item 6, overlay-class
    // arbitration), and this time the config is NOT narrowed — the move is the
    // feature. This render runs TAPE_PROB 100 against CD_PROB 60, so ~60% of
    // its ticks have two firers, and at CD_SEVERITY 0 the rung roll can only
    // ever reach rung 0: CD's event here is ALWAYS a conceal, which v1.9.0
    // classifies as OVERLAY. Under single-winner arbitration that conceal
    // landed on roughly half the collision ticks; it now lands on all of them,
    // under a tape bend that keeps ramping underneath. The digest therefore
    // moves by exactly the mechanism item 6 describes.
    //
    // The v1.3.0 number is kept and asserted NEGATIVELY, the discipline N7
    // uses: a probe that only checked the new anchor would pass just as
    // happily if the overlay split had silently failed to land.
    //
    // What V1 still buys, and it is now a two-sided gate: the v1.9.0 collision
    // render is pinned for every release after this one. The claim that the
    // arbitration rewrite left SINGLE-firer ticks bit-identical is a different
    // claim and is carried by A3 below, which is the probe that actually
    // constrains this change.
    {
        constexpr juce::uint64 kV190CanonicalDigest = 0x70e744c93cbcc2a3ULL;
        constexpr juce::uint64 kV130CanonicalDigest = 0x44a5de77d572facdULL;   // retired, kept for provenance

        auto p = makeProc();
        configureCanonicalRender (*p);
        setParam (*p, "CD_SEVERITY",   0.0f);
        setParam (*p, "VINYL_ENABLE",  0.0f);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const juce::uint64 digest = renderChecksum (out);
        const bool         live   = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool         match  = digest == kV190CanonicalDigest;
        const bool         moved  = digest != kV130CanonicalDigest;

        check ("V1 v1.9.0-collision-identity", match && live && moved,
               juce::String ("digest 0x") + juce::String::toHexString ((juce::int64) digest)
                   + " vs v1.9.0 0x"
                   + juce::String::toHexString ((juce::int64) kV190CanonicalDigest)
                   + (match ? " — collision render pinned" : " — COLLISION RENDER DRIFTED")
                   + (moved ? "" : " — STILL EQUALS v1.3.0: the overlay split DID NOT LAND")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // W1 — item 3: the wow bed is live, and bounded by its own budget.
    //
    // TAPE_ENABLE stays ON but TAPE_PROB is 0, so no tape EVENT can fire and
    // the only thing modulating the transport is the bed. Measured past 4 s
    // because the depth ramp itself takes kDepthRampSeconds (3 s) — a window
    // opened during the ramp would report a fraction of the real deviation.
    //
    // Both bounds discriminate: below the floor the bed is dead or inaudible,
    // above the ceiling it has exceeded the deviation budget the partial table
    // is supposed to enforce.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_PROB",    0.0f);      // tape enabled, no events
        setParam (*p, "TAPE_WOW",     100.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 0.0f);

        const int total = (int) (9.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);

        const int win      = 2048;
        const int startAt  = (int) (4.0 * kFs);
        double    maxDev   = 0.0;
        int       measured = 0;

        for (int n = startAt; n + win <= total; n += win)
        {
            const double f = meanFreqZeroCross (o, n, win);
            if (f > 0.0)
            {
                maxDev = juce::jmax (maxDev, std::abs (f / kSineHz - 1.0));
                ++measured;
            }
        }

        const bool enough  = measured >= 20;
        const bool livebed = maxDev >= 0.004;    // the bed actually modulates
        const bool bounded = maxDev <= 0.030;    // and stays inside its budget

        check ("W1 wow-live-and-bounded", enough && livebed && bounded,
               juce::String ("max deviation ") + juce::String (100.0 * maxDev, 3)
                   + "% over " + juce::String (measured) + " windows "
                   + "(bound [0.4%, 3.0%])"
                   + (enough  ? "" : " — TOO FEW WINDOWS, probe vacuous")
                   + (livebed ? "" : " — BED IS FLAT")
                   + (bounded ? "" : " — BUDGET EXCEEDED"));
    }

    //==========================================================================
    // W2 — item 3 negative control, and the reason W1's bounds mean anything:
    // the SAME configuration at TAPE_WOW = 0 must be flat AND bit-exactly the
    // delayed input. This is what proves the 0 case still takes CaptureRing's
    // exact-integer path rather than a fractional read that merely rounds
    // close.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_PROB",    0.0f);
        setParam (*p, "TAPE_WOW",     0.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 0.0f);

        const int total = (int) (9.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const int  startAt = kComp + (int) (0.2 * kFs);
        bool       exact   = true;
        juce::String detail;

        for (int ch = 0; ch < 2 && exact; ++ch)
        {
            const auto* o = out.getReadPointer (ch);
            for (int n = startAt; n < total; ++n)
            {
                if (! bitExact (o[n], noiseStereo (ch, n - kComp)))
                {
                    exact  = false;
                    detail = juce::String ("first mismatch ch") + juce::String (ch)
                           + " @" + juce::String (n);
                    break;
                }
            }
        }

        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("W2 wow-zero-bit-exact", exact && live,
               (exact ? juce::String ("9 s bit-exact with the tape family ENABLED")
                      : detail)
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // D1 — item 2: the dropout dips PARTWAY and comes back.
    //
    // Every tape win is forced to be a dropout (stop share 0, drop share 100),
    // so the read rate is exactly 1.0 for the whole render and the only
    // difference from passthrough is the dropout's gain and cutoff dip. The
    // 220 Hz sine sits far below the 2 kHz trough cutoff, so a windowed RMS
    // ratio against the delayed input reads the GAIN envelope directly.
    //
    // The floor bound is two-sided on purpose: a dropout that mutes is an
    // edit, not tape, and one that never dips below 0.75 is inaudible.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 3.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  0.0f);
        setParam (*p, "TAPE_DROP",       100.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (6.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);

        const int win     = 256;
        const int startAt = kComp + (int) (0.2 * kFs);

        double minRatio = 10.0, maxRatio = 0.0, maxDelta = 0.0;

        for (int n = startAt; n + win <= total; n += win)
        {
            double so = 0.0, si = 0.0;
            for (int k = 0; k < win; ++k)
            {
                const double a = (double) o[n + k];
                const double b = (double) sineStereo (0, n + k - kComp);
                so += a * a;
                si += b * b;
            }

            if (si > 1.0e-9)
            {
                const double r = std::sqrt (so / si);
                minRatio = juce::jmin (minRatio, r);
                maxRatio = juce::jmax (maxRatio, r);
            }
        }

        for (int n = startAt + 1; n < total; ++n)
            maxDelta = juce::jmax (maxDelta, std::abs ((double) o[n] - (double) o[n - 1]));

        const bool dips     = minRatio <= 0.75;    // an event actually fired
        const bool notMute  = minRatio >= 0.05;    // and never muted
        const bool recovers = maxRatio >= 0.98 && maxRatio <= 1.02;
        const bool noClick  = maxDelta <= 0.03;    // same bound as probe D

        check ("D1 dropout-dips-and-returns", dips && notMute && recovers && noClick,
               juce::String ("gain ratio [") + juce::String (minRatio, 3) + ", "
                   + juce::String (maxRatio, 3) + "] (floor bound [0.05, 0.75], "
                   + "recovery 1.00), maxDelta " + juce::String (maxDelta, 5)
                   + " (bound 0.03)"
                   + (dips     ? "" : " — NO DROPOUT FIRED, probe vacuous")
                   + (notMute  ? "" : " — MUTED, not a dropout")
                   + (recovers ? "" : " — NEVER RETURNS TO UNITY")
                   + (noClick  ? "" : " — CLICK"));
    }

    //==========================================================================
    // D2 — item 2 negative control: the same forced-tape render at
    // TAPE_DROP = 0 never dips at all. Without this, D1's floor bound would
    // pass just as happily against an engine that attenuated for some entirely
    // different reason.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 3.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  0.0f);
        setParam (*p, "TAPE_DROP",       0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (6.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);

        // Peak per window, not an RMS ratio: with dropouts off the tape BENDS
        // are free to run, and a bent sine is time-warped against the
        // reference — but its amplitude is still kSineAmp. Peak is the metric
        // that is blind to the bend and sensitive to a gain.
        const int win     = 1024;
        const int startAt = kComp + (int) (0.3 * kFs);
        double    minPeak = 10.0;

        for (int n = startAt; n + win <= total; n += win)
        {
            double peak = 0.0;
            for (int k = 0; k < win; ++k)
                peak = juce::jmax (peak, std::abs ((double) o[n + k]));

            minPeak = juce::jmin (minPeak, peak);
        }

        const bool flat = minPeak >= 0.95 * (double) kSineAmp;

        check ("D2 dropout-zero-no-dip", flat,
               juce::String ("min windowed peak ") + juce::String (minPeak, 4)
                   + " (need >= " + juce::String (0.95 * (double) kSineAmp, 4) + ")"
                   + (flat ? "" : " — SOMETHING ATTENUATES AT TAPE_DROP=0"));
    }

    //==========================================================================
    // S1 — item 11: the stop dies with speed instead of freezing to DC.
    //
    // Before this the read head re-read one held sample forever at FULL
    // amplitude, so the deep-stop plateau sat at whatever the sine happened to
    // be worth at freeze time and fed that as a DC step into Codec and Crush.
    // It must now reach silence, and it must get there without a click.
    //
    // The bound is 1e-3 rather than something near kSineAmp because the held
    // value is NOT the sine's peak — it is its instantaneous level, and across
    // the three stops in this render the quietest freeze measured 0.0216 with
    // the gain law reverted. 1e-3 sits an order of magnitude below even that
    // luckiest case, so the probe cannot pass on a favourable freeze.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 1.0f);   // stop every 1 s
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  100.0f);
        setParam (*p, "TAPE_RAMP",       150.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (3.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);

        // Deepest silence reached anywhere past warmup, measured as the
        // quietest 32 ms window.
        const int win     = (int) (0.032 * kFs);
        const int startAt = kComp + (int) (0.3 * kFs);

        double minPeak  = 10.0;
        double maxDelta = 0.0;

        for (int n = startAt; n + win <= total; n += win / 2)
        {
            double peak = 0.0;
            for (int k = 0; k < win; ++k)
                peak = juce::jmax (peak, std::abs ((double) o[n + k]));

            minPeak = juce::jmin (minPeak, peak);
        }

        for (int n = startAt + 1; n < total; ++n)
            maxDelta = juce::jmax (maxDelta, std::abs ((double) o[n] - (double) o[n - 1]));

        const bool dies    = minPeak <= 1.0e-3;                       // was ~0.5 (held DC)
        const bool live    = out.getMagnitude (0, 0, total) > 0.1f;   // signal outside the stops
        const bool noClick = maxDelta <= 0.03;

        check ("S1 stop-dies-with-speed", dies && live && noClick,
               juce::String ("quietest 32 ms peak ") + juce::String (minPeak, 6)
                   + " (need <= 1e-3; measured 0.0216 with the gain law"
                   " reverted), maxDelta "
                   + juce::String (maxDelta, 5) + " (bound 0.03)"
                   + (dies    ? "" : " — STOP STILL FREEZES TO DC")
                   + (live    ? "" : " — SILENT EVERYWHERE, probe vacuous")
                   + (noClick ? "" : " — CLICK"));
    }

    //==========================================================================
    // S2 — item 11 negative control, and the one that pins the whole reason the
    // gain law is armed by installStop rather than by a rate test: the bend
    // table's 0.5x interval sits BELOW the law's 0.9 threshold, so a law that
    // keyed on rate alone would quietly take 6 dB off every down-bend. Bends
    // are the tape family's melodic voice; a bent sine is time-warped but its
    // amplitude is still kSineAmp.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 2.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  0.0f);   // bends only
        setParam (*p, "TAPE_DROP",       0.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (8.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto* o = out.getReadPointer (0);

        const int win     = 1024;
        const int startAt = kComp + (int) (0.3 * kFs);
        double    minPeak = 10.0;

        for (int n = startAt; n + win <= total; n += win)
        {
            double peak = 0.0;
            for (int k = 0; k < win; ++k)
                peak = juce::jmax (peak, std::abs ((double) o[n + k]));

            minPeak = juce::jmin (minPeak, peak);
        }

        // Prove the render actually VISITED the sub-threshold rates the law
        // would have caught — otherwise "bends were not attenuated" is a claim
        // about a render that never contained a down-bend.
        const auto   mono  = channelToVector (out, 0);
        const auto   trace = pitchTrace (mono, kSineHz);
        double       lowestRatio = 1.0;
        for (size_t i = trace.size() / 8; i < trace.size(); ++i)
            if (trace[i] > 0.0)
                lowestRatio = juce::jmin (lowestRatio, trace[i] / kSineHz);

        const bool wentLow = lowestRatio <= 0.85;   // below the 0.9 threshold
        const bool flat    = minPeak >= 0.95 * (double) kSineAmp;

        check ("S2 bends-keep-loudness", flat && wentLow,
               juce::String ("min windowed peak ") + juce::String (minPeak, 4)
                   + " (need >= " + juce::String (0.95 * (double) kSineAmp, 4)
                   + "), lowest bend ratio " + juce::String (lowestRatio, 3)
                   + " (need <= 0.85, i.e. under the 0.9 gain-law threshold)"
                   + (flat    ? "" : " — GAIN LAW LEAKED ONTO BENDS")
                   + (wentLow ? "" : " — NO SUB-THRESHOLD BEND OCCURRED, probe vacuous"));
    }

    //==========================================================================
    // S3 — v1.4.0 all-on block-size invariance. Every new subsystem is in the
    // path at once: the wow bed draws from its own stream on a sample counter
    // (the one RNG consumer in this engine that is not tick-aligned), the
    // dropout draws at ticks, and the stop gain law runs its own latches. Any
    // of those tied to a block boundary rather than a sample count shows up
    // here as a mismatch under ragged chopping.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 4.0f);
            setParam (proc, "TAPE_PROB",       100.0f);
            setParam (proc, "TAPE_STOP_PROB",  30.0f);
            setParam (proc, "TAPE_DROP",       50.0f);
            setParam (proc, "TAPE_WOW",        75.0f);
            setParam (proc, "CD_PROB",         100.0f);
            setParam (proc, "VINYL_PROB",      100.0f);
            setParam (proc, "SEED",            909.0f);
        };

        const int total = 96000;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);
        auto c = makeProc();  configure (*c);

        juce::AudioBuffer<float> outA, outB, outC;
        renderInto (*a, outA, total, { 512 }, noiseStereo);
        renderInto (*b, outB, total, { 512 }, noiseStereo);
        renderInto (*c, outC, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outA, outB);
            const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("S3 v1.4-determinism", identical && live,
                   (identical ? juce::String ("wow+drop+stop same-seed fresh instances: bit-identical")
                              : firstDifference (outA, outB))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }

        {
            const bool identical = bitIdentical (outA, outC);
            const bool live      = outC.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("S3 v1.4-ragged", identical && live,
                   (identical ? juce::String ("512 vs 1,7,64,333,4096: bit-identical with the wow bed on")
                              : firstDifference (outA, outC))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // ── v1.5.0: improvement brief items 4 and 19 ─────────────────────────────
    //==========================================================================

    //==========================================================================
    // N1 — item 4, vinyl: rumble level and the micro-tick rain, measured on
    // DIGITAL SILENCE so the bed is the only thing in the output.
    //
    // VINYL_PROB is 0, so no jump event — and therefore no ArtifactSynth pop —
    // can fire. The 80 Hz band split then separates the bed's two contributors
    // (55 Hz rumble below, 2-6 kHz ticks above) so neither can stand in for the
    // other: a probe on total RMS alone would pass on rumble with no ticks.
    //
    // The high-band CREST FACTOR is the assertion that the amplitude law is
    // actually a power law. Gaussian noise crests around 4; a rain of ticks
    // whose amplitude is a cubed uniform — many tiny, rare large — crests far
    // higher. A bed that emitted uniform-amplitude clicks would pass a rate
    // check and fail this.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",  0.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 1.0f);
        setParam (*p, "VINYL_PROB",   0.0f);
        setParam (*p, "VINYL_POP",    0.0f);
        setParam (*p, "VINYL_WEAR",   100.0f);

        const int total = (int) (8.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, silentInput);

        const int startAt = (int) (0.5 * kFs);          // past the level ramp
        const int len     = total - startAt;

        double lowRms = 0.0, highRms = 0.0, highPeak = 0.0;
        splitBandRms (out, 0, startAt, len, 80.0, kFs, lowRms, highRms, highPeak);

        const double targetDb = toDb (VinylBed::kRumbleFullRms);
        const double crest    = highRms > 0.0 ? highPeak / highRms : 0.0;

        const bool rumbleOk = std::abs (toDb (lowRms) - targetDb) <= 1.0;
        const bool ticksOk  = highRms > 1.0e-5;
        const bool powerLaw = crest >= 8.0;

        check ("N1 vinyl-bed-levels", rumbleOk && ticksOk && powerLaw,
               juce::String ("rumble ") + juce::String (toDb (lowRms), 2)
                   + " dBFS (target " + juce::String (targetDb, 2) + " +/- 1), ticks "
                   + juce::String (toDb (highRms), 2) + " dBFS, crest "
                   + juce::String (crest, 2)
                   + (rumbleOk ? "" : " — RUMBLE OFF TARGET")
                   + (ticksOk  ? "" : " — NO TICKS, probe half vacuous")
                   + (powerLaw ? "" : " — TICK AMPLITUDES ARE NOT POWER-LAW"));
    }

    //==========================================================================
    // N1b — the vinyl bed's transparency rail. Same render at VINYL_WEAR 0 must
    // be EXACTLY digital zero, not merely quiet: that exactness is what keeps
    // the FUNC-02 null intact at the shipped default.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",  0.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 1.0f);
        setParam (*p, "VINYL_PROB",   0.0f);
        setParam (*p, "VINYL_WEAR",   0.0f);

        const int total = (int) (2.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, silentInput);

        bool allZero = true;
        int  firstNz = -1;

        for (int ch = 0; ch < 2 && allZero; ++ch)
            for (int n = 0; n < total; ++n)
                if (! bitExact (out.getSample (ch, n), 0.0f))
                {
                    allZero = false;
                    firstNz = n;
                    break;
                }

        check ("N1b vinyl-bed-zero-rail", allZero,
               allZero ? juce::String ("bit-exact zero over the whole render")
                       : juce::String ("first non-zero @") + juce::String (firstNz)
                             + " — BED LEAKS AT WEAR 0");
    }

    //==========================================================================
    // N2 — item 4, tape: hiss level and stereo width, again on silence with
    // TAPE_PROB 0 so no tape event can fire.
    //
    // The correlation bound is the load-bearing half. Two tape tracks carry two
    // independent noise sources; a mono bed added to both channels would hit
    // the level target exactly and still sound like a centred buzz, so level
    // alone cannot tell the two implementations apart.
    {
        auto p = makeProc();
        setParam (*p, "TAPE_ENABLE",  1.0f);
        setParam (*p, "TAPE_PROB",    0.0f);
        setParam (*p, "TAPE_HISS",    100.0f);
        setParam (*p, "CD_ENABLE",    0.0f);
        setParam (*p, "VINYL_ENABLE", 0.0f);

        const int total = (int) (8.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, silentInput);

        const int startAt = (int) (0.5 * kFs);
        const int len     = total - startAt;

        const double rms      = rmsOf (out, 0, startAt, len);
        const double corr     = channelCorrelation (out, startAt, len);
        const double targetDb = toDb (TapeBed::kHissFullRms);

        const bool levelOk = std::abs (toDb (rms) - targetDb) <= 1.0;
        const bool wideOk  = std::abs (corr) < 0.15;

        check ("N2 tape-hiss-level", levelOk && wideOk,
               juce::String ("hiss ") + juce::String (toDb (rms), 2)
                   + " dBFS (target " + juce::String (targetDb, 2) + " +/- 1), L/R corr "
                   + juce::String (corr, 3)
                   + (levelOk ? "" : " — HISS OFF TARGET")
                   + (wideOk  ? "" : " — CHANNELS CORRELATED, hiss is mono"));
    }

    //==========================================================================
    // N2c — hiss rides tape speed: a full stop mutes the bed along with the
    // programme, because hiss is recorded material and not a synthetic layer
    // bolted on top.
    //
    // Its DISCRIMINATOR is N2d below, not a liveness check: on silence the bed
    // is the only signal, so "it got quiet somewhere" is worthless without a
    // render that must NOT get quiet.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);   // Free
        setParam (*p, "CLOCK_FREE_RATE", 1.0f);   // a stop every second
        setParam (*p, "TAPE_ENABLE",     1.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  100.0f);
        setParam (*p, "TAPE_RAMP",       150.0f);
        setParam (*p, "TAPE_HISS",       100.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (5.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, silentInput);

        const auto* o   = out.getReadPointer (0);
        const int   win = (int) (0.032 * kFs);
        const int   startAt = (int) (0.5 * kFs);

        double minPeak = 10.0, maxPeak = 0.0;

        for (int n = startAt; n + win <= total; n += win / 2)
        {
            double peak = 0.0;
            for (int k = 0; k < win; ++k)
                peak = juce::jmax (peak, std::abs ((double) o[n + k]));

            minPeak = juce::jmin (minPeak, peak);
            maxPeak = juce::jmax (maxPeak, peak);
        }

        const double ratio = maxPeak > 0.0 ? minPeak / maxPeak : 1.0;

        const bool mutes = ratio <= 0.10;
        const bool live  = maxPeak > 1.0e-4;

        check ("N2c hiss-dies-with-speed", mutes && live,
               juce::String ("quietest/loudest 32 ms peak ") + juce::String (ratio, 4)
                   + " (need <= 0.10), loudest " + juce::String (maxPeak, 6)
                   + (mutes ? "" : " — HISS RUNS THROUGH THE STOP")
                   + (live  ? "" : " — SILENT EVERYWHERE, probe vacuous"));
    }

    //==========================================================================
    // N2d — N2c's discriminator. Same forced tape events with the stop share at
    // 0, so only BENDS fire and TapeStopGain is never armed. Without this, N2c
    // would pass just as happily against a bed that ducked for any reason at
    // all — or against one that simply ran out of level.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 1.0f);
        setParam (*p, "TAPE_ENABLE",     1.0f);
        setParam (*p, "TAPE_PROB",       100.0f);
        setParam (*p, "TAPE_STOP_PROB",  0.0f);   // bends only
        setParam (*p, "TAPE_HISS",       100.0f);
        setParam (*p, "CD_ENABLE",       0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);

        const int total = (int) (5.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, silentInput);

        const auto* o   = out.getReadPointer (0);
        const int   win = (int) (0.032 * kFs);
        const int   startAt = (int) (0.5 * kFs);

        double minPeak = 10.0, maxPeak = 0.0;

        for (int n = startAt; n + win <= total; n += win / 2)
        {
            double peak = 0.0;
            for (int k = 0; k < win; ++k)
                peak = juce::jmax (peak, std::abs ((double) o[n + k]));

            minPeak = juce::jmin (minPeak, peak);
            maxPeak = juce::jmax (maxPeak, peak);
        }

        const double ratio = maxPeak > 0.0 ? minPeak / maxPeak : 0.0;
        const bool   flat  = ratio >= 0.30;

        check ("N2d bends-keep-hiss", flat,
               juce::String ("quietest/loudest 32 ms peak ") + juce::String (ratio, 4)
                   + " (need >= 0.30 — bends must NOT mute the bed)"
                   + (flat ? "" : " — SPEED COUPLING LEAKED ONTO BENDS"));
    }

    //==========================================================================
    // N3 — item 4, codec: the mains hum is at the mains frequency, carries its
    // two harmonics, and FOLLOWS CODEC_MAINS.
    //
    // Goertzel windows are sized so 50, 60 and every harmonic land on exact
    // bins (fs/2 samples = 2 Hz spacing); off-bin leakage would otherwise be
    // indistinguishable from a partial that is not there. The off-partial
    // control at 137 Hz is what makes "there is energy at 50 Hz" mean
    // something more than "there is energy".
    {
        auto measure = [&] (int mainsIndex, double& f0, double& h2, double& h3,
                            double& off, double& other)
        {
            auto p = makeProc();
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "CODEC_ENABLE", 1.0f);
            setParam (*p, "CODEC_MIX",    100.0f);
            setParam (*p, "CODEC_NOISE",  100.0f);
            setParam (*p, "CODEC_MAINS",  (float) mainsIndex);

            const int total = (int) (4.0 * kFs);
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, silentInput);

            const int startAt = (int) (1.0 * kFs);
            const int len     = (int) (kFs / 2.0);          // 2 Hz bins

            const double mains = mainsIndex == 1 ? CodecBed::kHum60Hz : CodecBed::kHum50Hz;
            const double alt   = mainsIndex == 1 ? CodecBed::kHum50Hz : CodecBed::kHum60Hz;

            f0    = toneAmplitude (out, 0, startAt, len, mains,       kFs);
            h2    = toneAmplitude (out, 0, startAt, len, 2.0 * mains, kFs);
            h3    = toneAmplitude (out, 0, startAt, len, 3.0 * mains, kFs);
            off   = toneAmplitude (out, 0, startAt, len, 137.0,       kFs);
            other = toneAmplitude (out, 0, startAt, len, alt,         kFs);
        };

        double f0 = 0, h2 = 0, h3 = 0, off = 0, other = 0;
        measure (0, f0, h2, h3, off, other);

        // Expected fundamental amplitude: the partial sum is normalised to
        // unity RMS, so the fundamental's own amplitude is kHumNorm * kHumFullRms.
        const double expected = (double) CodecBed::kHumNorm * (double) CodecBed::kHumFullRms;

        const bool level50   = std::abs (toDb (f0) - toDb (expected)) <= 2.0;
        const bool harmonics = h2 > 0.25 * f0 && h3 > 0.12 * f0;
        const bool clean50   = f0 > 20.0 * off;
        const bool picked50  = f0 > 8.0 * other;

        check ("N3 codec-hum-50", level50 && harmonics && clean50 && picked50,
               juce::String ("50 Hz ") + juce::String (toDb (f0), 2)
                   + " dBFS (expected " + juce::String (toDb (expected), 2) + " +/- 2), 100/150 Hz "
                   + juce::String (h2 / juce::jmax (1.0e-12, f0), 3) + "/"
                   + juce::String (h3 / juce::jmax (1.0e-12, f0), 3)
                   + " of f0, 137 Hz control " + juce::String (toDb (off), 1) + " dBFS"
                   + (level50   ? "" : " — HUM OFF TARGET")
                   + (harmonics ? "" : " — HARMONICS MISSING")
                   + (clean50   ? "" : " — BROADBAND, NOT A HUM")
                   + (picked50  ? "" : " — 60 Hz PRESENT AT THE 50 Hz SETTING"));

        double g0 = 0, g2 = 0, g3 = 0, goff = 0, gother = 0;
        measure (1, g0, g2, g3, goff, gother);

        const bool picked60 = g0 > 8.0 * gother;

        check ("N3b codec-hum-60", picked60,
               juce::String ("60 Hz ") + juce::String (toDb (g0), 2)
                   + " dBFS vs 50 Hz " + juce::String (toDb (gother), 2) + " dBFS"
                   + (picked60 ? "" : " — CODEC_MAINS DOES NOT MOVE THE HUM"));
    }

    //==========================================================================
    // N4 — every bed at 96 kHz sits where it sits at 48 kHz.
    //
    // This is the probe the beds' whole normalisation scheme exists for.
    // Filtering white noise to a fixed bandwidth in Hz gives output power
    // proportional to that bandwidth over fs, so a bed calibrated with a bare
    // constant is 3 dB quieter every time the rate doubles — inaudible in the
    // one render anybody tests, and wrong for every user at 96 kHz.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "TAPE_ENABLE",  1.0f);
            setParam (proc, "TAPE_PROB",    0.0f);
            setParam (proc, "TAPE_HISS",    100.0f);
            setParam (proc, "CD_ENABLE",    0.0f);
            setParam (proc, "VINYL_ENABLE", 1.0f);
            setParam (proc, "VINYL_PROB",   0.0f);
            setParam (proc, "VINYL_POP",    0.0f);
            setParam (proc, "VINYL_WEAR",   100.0f);
            setParam (proc, "CODEC_ENABLE", 1.0f);
            setParam (proc, "CODEC_NOISE",  100.0f);
        };

        auto measureAt = [&] (double fs)
        {
            auto p = makeProcAtRate (fs);
            configure (*p);

            const int total = (int) (8.0 * fs);
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, silentInput);

            const int startAt = (int) (1.0 * fs);
            return rmsOf (out, 0, startAt, total - startAt);
        };

        const double rms48 = measureAt (48000.0);
        const double rms96 = measureAt (96000.0);
        const double delta = toDb (rms96) - toDb (rms48);

        const bool invariant = std::abs (delta) <= 1.5;
        const bool live      = rms48 > 1.0e-5;

        check ("N4 bed-rate-invariance", invariant && live,
               juce::String ("48 kHz ") + juce::String (toDb (rms48), 2)
                   + " dBFS, 96 kHz " + juce::String (toDb (rms96), 2)
                   + " dBFS, delta " + juce::String (delta, 2) + " dB (bound 1.5)"
                   + (invariant ? "" : " — BEDS ARE SAMPLE-RATE DEPENDENT")
                   + (live      ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // N5 — item 19: comfort noise fills the hole that extended concealment
    // leaves.
    //
    // The measurement is a DIFFERENCE of two renders at the same seed, one at
    // PACKET_COMFORT 0 and one at 100, so what is measured is the bed itself
    // and not the concealment around it. Silence mode makes the negative
    // control exact: at comfort 0 a lost packet is bit-exact digital zero, and
    // the length of the longest zero run is the size of the hole the item
    // exists to fill. If that run were short the probe would be measuring
    // nothing, so it is asserted too.
    {
        auto configure = [] (OBitrotAudioProcessor& proc, float comfort)
        {
            setBaseline (proc);
            setParam (proc, "SEED",            2024.0f);
            setParam (proc, "TAPE_ENABLE",     0.0f);
            setParam (proc, "CD_ENABLE",       0.0f);
            setParam (proc, "VINYL_ENABLE",    0.0f);
            setParam (proc, "PACKET_ENABLE",   1.0f);
            setParam (proc, "PACKET_LOSS",     85.0f);
            setParam (proc, "PACKET_BURST",    100.0f);
            setParam (proc, "PACKET_CONCEAL",  0.0f);   // Silence
            setParam (proc, "PACKET_COMFORT",  comfort);
        };

        const int total = (int) (8.0 * kFs);

        auto a = makeProc();  configure (*a, 0.0f);
        auto b = makeProc();  configure (*b, 100.0f);

        juce::AudioBuffer<float> off, on;
        renderInto (*a, off, total, { 512 }, sineStereo);
        renderInto (*b, on,  total, { 512 }, sineStereo);

        const auto* zo = off.getReadPointer (0);
        const auto* zn = on.getReadPointer (0);

        const int startAt = kComp + (int) (0.5 * kFs);

        // Longest exact-zero run, and the RMS of the bed measured only DEEP
        // inside such runs — the ramp takes one packet to arrive, so sampling
        // the first 40 ms of a hole would report a number the law never claims.
        int    run = 0, longestRun = 0;
        double acc = 0.0;
        int    cnt = 0;

        const int deepAt = (int) (0.040 * kFs);

        for (int n = startAt; n < total; ++n)
        {
            if (bitExact (zo[n], 0.0f))
            {
                ++run;
                longestRun = juce::jmax (longestRun, run);

                if (run >= deepAt)
                {
                    const double d = (double) zn[n] - (double) zo[n];
                    acc += d * d;
                    ++cnt;
                }
            }
            else
            {
                run = 0;
            }
        }

        const double cnRms = cnt > 0 ? std::sqrt (acc / (double) cnt) : 0.0;

        // Expected: kMaxRelative (the knob is at 100%, so the squaring law is
        // the identity) below the tracked programme RMS, which for a full-scale
        // sine at kSineAmp is kSineAmp / sqrt(2).
        const double programRms = (double) kSineAmp / juce::MathConstants<double>::sqrt2;
        const double expected   = programRms * (double) ComfortNoise::kMaxRelative;

        const bool holeExists = longestRun >= 3 * (int) (0.020 * kFs);
        const bool measured   = cnt > 1000;
        const bool levelOk    = std::abs (toDb (cnRms) - toDb (expected)) <= 4.0;

        check ("N5 comfort-noise-level", holeExists && measured && levelOk,
               juce::String ("longest silent run ") + juce::String (longestRun)
                   + " samples, bed " + juce::String (toDb (cnRms), 2)
                   + " dBFS over " + juce::String (cnt) + " samples (expected "
                   + juce::String (toDb (expected), 2) + " +/- 4)"
                   + (holeExists ? "" : " — NO EXTENDED BURST, probe vacuous")
                   + (measured   ? "" : " — TOO FEW DEEP SAMPLES, probe vacuous")
                   + (levelOk    ? "" : " — COMFORT BED OFF TARGET"));

        // The negative control is already in hand: `off` must be bit-exact
        // zero across those runs (it is, by construction of the loop above),
        // and `on` must NOT be. Stating it makes the pass mean something.
        bool onIsQuiet = true;
        for (int n = startAt; n < total && onIsQuiet; ++n)
            if (bitExact (zo[n], 0.0f) && ! bitExact (zn[n], 0.0f))
                onIsQuiet = false;

        check ("N5b comfort-fills-the-hole", ! onIsQuiet,
               onIsQuiet ? juce::String ("holes are STILL digital zero at PACKET_COMFORT 100"
                                         " — THE BED NEVER FIRES")
                         : juce::String ("holes carry the bed at comfort 100 and are"
                                         " bit-exact zero at comfort 0"));
    }

    //==========================================================================
    // N5c — the additive choice, stated as a probe: comfort noise arrives under
    // Repeat as well, which is the mode whose conceal output never decays.
    // Replacing rather than adding would have dissolved that mode's
    // machine-gun identity after 60 ms.
    {
        auto configure = [] (OBitrotAudioProcessor& proc, float comfort)
        {
            setBaseline (proc);
            setParam (proc, "SEED",           2024.0f);
            setParam (proc, "TAPE_ENABLE",    0.0f);
            setParam (proc, "CD_ENABLE",      0.0f);
            setParam (proc, "VINYL_ENABLE",   0.0f);
            setParam (proc, "PACKET_ENABLE",  1.0f);
            setParam (proc, "PACKET_LOSS",    85.0f);
            setParam (proc, "PACKET_BURST",   100.0f);
            setParam (proc, "PACKET_CONCEAL", 1.0f);   // Repeat
            setParam (proc, "PACKET_COMFORT", comfort);
        };

        const int total = (int) (6.0 * kFs);

        auto a = makeProc();  configure (*a, 0.0f);
        auto b = makeProc();  configure (*b, 100.0f);

        juce::AudioBuffer<float> off, on;
        renderInto (*a, off, total, { 512 }, sineStereo);
        renderInto (*b, on,  total, { 512 }, sineStereo);

        const int startAt = kComp + (int) (0.5 * kFs);
        const int len     = total - startAt;

        const auto* zo = off.getReadPointer (0);
        const auto* zn = on.getReadPointer (0);

        double acc = 0.0;
        for (int n = startAt; n < total; ++n)
        {
            const double d = (double) zn[n] - (double) zo[n];
            acc += d * d;
        }

        const double diffRms = std::sqrt (acc / (double) juce::jmax (1, len));
        const double offRms  = rmsOf (off, 0, startAt, len);

        // The bed is present, and it is a FLOOR rather than a replacement: the
        // repeated packets still dominate by a wide margin.
        const bool present = diffRms > 1.0e-5;
        const bool under   = diffRms < 0.1 * offRms;

        check ("N5c comfort-under-repeat", present && under,
               juce::String ("bed ") + juce::String (toDb (diffRms), 2)
                   + " dBFS under a Repeat output at " + juce::String (toDb (offRms), 2)
                   + " dBFS"
                   + (present ? "" : " — NO BED UNDER REPEAT")
                   + (under   ? "" : " — BED SWAMPS THE REPEAT, not a floor"));
    }

    //==========================================================================
    // N6 — v1.5.0 all-on determinism and block-size invariance. Four new RNG
    // streams are now drawing per sample on their own schedules, two of them
    // taking CONDITIONAL extra draws when a tick or a crackle burst fires. Any
    // of that keyed to a block boundary rather than a sample count shows up
    // here as a mismatch under ragged chopping.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);
            setParam (proc, "CLOCK_FREE_RATE", 4.0f);
            setParam (proc, "SEED",            1505.0f);
            setParam (proc, "TAPE_PROB",       100.0f);
            setParam (proc, "TAPE_STOP_PROB",  30.0f);
            setParam (proc, "TAPE_DROP",       50.0f);
            setParam (proc, "TAPE_WOW",        75.0f);
            setParam (proc, "TAPE_HISS",       80.0f);
            setParam (proc, "CD_PROB",         100.0f);
            setParam (proc, "VINYL_PROB",      100.0f);
            setParam (proc, "VINYL_WEAR",      90.0f);
            setParam (proc, "PACKET_ENABLE",   1.0f);
            setParam (proc, "PACKET_LOSS",     60.0f);
            setParam (proc, "PACKET_COMFORT",  70.0f);
            setParam (proc, "CODEC_ENABLE",    1.0f);
            setParam (proc, "CODEC_NOISE",     85.0f);
        };

        const int total = 96000;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);
        auto c = makeProc();  configure (*c);

        juce::AudioBuffer<float> outA, outB, outC;
        renderInto (*a, outA, total, { 512 }, noiseStereo);
        renderInto (*b, outB, total, { 512 }, noiseStereo);
        renderInto (*c, outC, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        {
            const bool identical = bitIdentical (outA, outB);
            const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("N6 v1.5-determinism", identical && live,
                   (identical ? juce::String ("all beds + comfort, same-seed fresh"
                                              " instances: bit-identical")
                              : firstDifference (outA, outB))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }

        {
            const bool identical = bitIdentical (outA, outC);
            const bool live      = outC.getMagnitude (0, 0, total) > 1.0e-4f;
            check ("N6 v1.5-ragged", identical && live,
                   (identical ? juce::String ("512 vs 1,7,64,333,4096: bit-identical"
                                              " with every bed live")
                              : firstDifference (outA, outC))
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // C1 — ITEM 16: the codec AGC drags quiet material UP and holds loud
    // material roughly flat. Measured as the band gain at 1 kHz between
    // CODEC_AGC 0 and 100 on the same input, so the codec's own colour
    // cancels and only the compressor's gain survives.
    //
    // Design targets (threshold -20 dBFS, 4:1, +10 dB makeup):
    //   a -26 dBFS tone sits below threshold -> the full +10 dB makeup;
    //   a  -6 dBFS tone sits 14 dB over      -> ~10.5 dB of reduction,
    //                                           landing near unity.
    // The DIFFERENCE between the two is the whole point of the stage: at
    // CODEC_AGC 0 both gains are exactly 0 dB, so a plumbing failure that
    // leaves the AGC out of circuit reads as 0/0 here, not as a near miss.
    {
        InputFn quietTone = [] (int ch, int n) noexcept -> float
        {
            juce::ignoreUnused (ch);
            return 0.05f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                             * 1000.0 * (double) n / kFs);
        };

        InputFn loudTone = [] (int ch, int n) noexcept -> float
        {
            juce::ignoreUnused (ch);
            return 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                            * 1000.0 * (double) n / kFs);
        };

        auto render = [&] (InputFn input, float agc, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setBaseline (*p);
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "CODEC_ENABLE", 1.0f);
            setParam (*p, "CODEC_MODE",   0.0f);      // mu-law
            setParam (*p, "CODEC_MIX",    100.0f);
            setParam (*p, "CODEC_AGC",    agc);
            renderInto (*p, out, total, { 512 }, input);
        };

        const int total = 48000;
        juce::AudioBuffer<float> qOff, qOn, lOff, lOn;
        render (quietTone, 0.0f,   qOff, total);
        render (quietTone, 100.0f, qOn,  total);
        render (loudTone,  0.0f,   lOff, total);
        render (loudTone,  100.0f, lOn,  total);

        auto bandGainDb = [] (const juce::AudioBuffer<float>& on,
                              const juce::AudioBuffer<float>& off)
        {
            const auto a = channelToVector (on,  0);
            const auto b = channelToVector (off, 0);
            return 10.0 * std::log10 (bandEnergy (a, 24000, 800.0, 1200.0)
                                      / juce::jmax (1.0e-18,
                                                    bandEnergy (b, 24000, 800.0, 1200.0)));
        };

        const double quietGain = bandGainDb (qOn, qOff);
        const double loudGain  = bandGainDb (lOn, lOff);

        // A 1 ms attack meeting a COLD envelope sits at full makeup, so the
        // codec's first signal — arriving ~20 ms in, past the enable fade —
        // used to overshoot by the whole +10 dB (peak 1.54 on a -6 dBFS
        // tone). The envelope is primed at its unity-gain level for exactly
        // that reason; what remains is the ordinary compressor transient,
        // fed by the 20 ms of structural-delay silence the envelope releases
        // through before signal arrives.
        //
        // Bound stated as a principle, not fitted to the measurement: a
        // 1 ms-attack transient is worth a few dB, never the whole makeup.
        // 3 dB passes comfortably today and fails hard on an unprimed
        // envelope (+9.2 dB), which is the regression this guards.
        const float peakOn   = lOn.getMagnitude  (0, 0, total);
        const float peakOff  = lOff.getMagnitude (0, 0, total);
        const float peakCeil = 1.41254f * peakOff;   // +3 dB

        const bool lifts   = quietGain > 8.0 && quietGain < 11.5;
        const bool tames   = loudGain  > -3.0 && loudGain < 1.5;
        const bool spreads = (quietGain - loudGain) > 6.0;
        const bool bounded = peakOn < peakCeil;
        const bool live    = qOn.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("C1 codec-agc-compresses", lifts && tames && spreads && bounded && live,
               juce::String ("-26 dBFS tone ") + juce::String (quietGain, 2)
                   + " dB (need 8..11.5), -6 dBFS tone " + juce::String (loudGain, 2)
                   + " dB (need -3..1.5), spread "
                   + juce::String (quietGain - loudGain, 2) + " dB (need > 6), peak "
                   + juce::String (peakOn, 4) + " vs AGC-off " + juce::String (peakOff, 4)
                   + " (ceiling " + juce::String (peakCeil, 4) + ")"
                   + (bounded ? "" : " — AGC OVERSHOOTS ON COLD START")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // C2 — ITEM 29: the mu-law codec is SEGMENTED, not a sampled log curve.
    //
    // Read the encoder and the decode table directly rather than inferring
    // them from a render: the post-LPF smears the quantiser lattice, so a
    // signal-domain probe could only ever measure "something changed". What
    // makes G.711 the real codec is structural — 8 chords of 16 UNIFORM
    // steps, each chord's step exactly double the one below it. A continuous
    // log curve has neither uniform runs nor exact doublings, so this
    // separates the two implementations rather than merely detecting an edit.
    {
        auto decodePos = [] (int s, int q)
        {
            return g711::kMuDecode[(std::size_t) (((s << 4) | q) ^ 0xFF)];
        };

        bool   uniform = true, doubling = true;
        int    badSeg = -1;

        for (int s = 0; s < 8 && uniform && doubling; ++s)
        {
            const float expected = (float) (8 << s) / 32768.0f;

            for (int q = 0; q < 15; ++q)
            {
                const float step = decodePos (s, q + 1) - decodePos (s, q);

                if (! bitExact (step, expected))
                {
                    // Which invariant failed: a wrong-but-constant step is a
                    // chord-scaling error, a varying step is not a chord.
                    const float first = decodePos (s, 1) - decodePos (s, 0);
                    if (bitExact (step, first)) doubling = false;
                    else                        uniform  = false;
                    badSeg = s;
                    break;
                }
            }
        }

        const bool zeroExact = bitExact (g711::kMuDecode[0xFF], 0.0f)
                            && g711::linearToMuLaw (0) == 0xFF;
        const bool clipExact = bitExact (g711::kMuDecode[0x80],  32124.0f / 32768.0f)
                            && bitExact (g711::kMuDecode[0x00], -32124.0f / 32768.0f)
                            && g711::linearToMuLaw (32767)  == 0x80
                            && g711::linearToMuLaw (-32768) == 0x00;

        check ("C2 mulaw-segmented-chords", uniform && doubling && zeroExact && clipExact,
               juce::String (uniform  ? "" : "chord steps NOT uniform in seg "
                                             + juce::String (badSeg) + "; ")
                   + (doubling ? "" : "chord step does not double at seg "
                                      + juce::String (badSeg) + "; ")
                   + (zeroExact ? "" : "silence does not encode to 0xFF/0.0; ")
                   + (clipExact ? "" : "full scale does not clip at 32124/32768; ")
                   + (uniform && doubling && zeroExact && clipExact
                          ? "8 chords x 16 uniform steps, step doubles per chord, "
                            "0 -> 0xFF -> 0.0, FS -> 0x80 -> -0.17 dBFS"
                          : ""));
    }

    //==========================================================================
    // C3 — ITEM 7: codec-domain frame loss reaches the GSM 06.11 mute point.
    //
    // Config chosen so that silence can come from ONE place only. The input
    // is a constant-amplitude 1 kHz tone and PACKET_CONCEAL is Repeat, which
    // replays the last good packet verbatim and never decays — so the PCM
    // stage feeds the encoder a full-level tone for the entire render. In
    // v1.7.0 the codec re-encoded that repeat and the output never went
    // quiet. A silent stretch here can therefore only be the new decoder-side
    // path muting after 16 consecutive lost frames.
    //
    // At PACKET_LOSS 100 the GE chain gives pGB = 1 and pLossBad = 1, so
    // per-packet loss is ~98.75% and runs past 16 are routine across a 4 s
    // render — but the bound asked for is ONE frame of confirmed mute
    // (10 ms), not a lucky long one.
    {
        InputFn tone = [] (int ch, int n) noexcept -> float
        {
            juce::ignoreUnused (ch);
            return 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                            * 1000.0 * (double) n / kFs);
        };

        auto render = [&] (bool packetOn, juce::AudioBuffer<float>& out, int total)
        {
            auto p = makeProc();
            setBaseline (*p);
            setParam (*p, "SEED",           4242.0f);
            setParam (*p, "TAPE_ENABLE",    0.0f);
            setParam (*p, "CD_ENABLE",      0.0f);
            setParam (*p, "VINYL_ENABLE",   0.0f);
            setParam (*p, "PACKET_ENABLE",  packetOn ? 1.0f : 0.0f);
            setParam (*p, "PACKET_LOSS",    90.0f);
            setParam (*p, "PACKET_BURST",   100.0f);
            setParam (*p, "PACKET_CONCEAL", 1.0f);    // Repeat — never decays
            setParam (*p, "CODEC_ENABLE",   1.0f);
            setParam (*p, "CODEC_MODE",     1.0f);    // GSM
            setParam (*p, "CODEC_MIX",      100.0f);
            renderInto (*p, out, total, { 512 }, tone);
        };

        // Longest contiguous run below -100 dBFS, past the codec warmup.
        auto longestSilentRun = [] (const juce::AudioBuffer<float>& b, int total)
        {
            const auto* o   = b.getReadPointer (0);
            int         run = 0, best = 0;

            for (int n = (int) (0.25 * kFs); n < total; ++n)
            {
                run  = std::abs (o[n]) < 1.0e-5f ? run + 1 : 0;
                best = juce::jmax (best, run);
            }

            return best;
        };

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> outOn, outOff;
        render (true,  outOn,  total);
        render (false, outOff, total);

        const int  bound   = (int) (0.010 * kFs);
        const int  runOn   = longestSilentRun (outOn,  total);
        const int  runOff  = longestSilentRun (outOff, total);
        const bool mutes   = runOn >= bound;
        const bool control = runOff == 0;       // PACKET off: the coupling cannot fire
        const bool live    = outOn.getMagnitude (0, 0, total) > 0.05f;

        check ("C3 gsm-frame-loss-mutes", mutes && control && live,
               juce::String ("longest silence ") + juce::String (runOn)
                   + " samples (need >= " + juce::String (bound) + "), peak "
                   + juce::String (outOn.getMagnitude (0, 0, total), 5)
                   + " vs off-peak " + juce::String (outOff.getMagnitude (0, 0, total), 5)
                   + (control ? ", none at all with PACKET off"
                              : ", but PACKET-off render is ALSO silent for "
                                + juce::String (runOff) + " — not the coupling")
                   + (live ? "" : " — SILENT THROUGHOUT, probe vacuous"));
    }

    //==========================================================================
    // C4 — ITEM 7: the frame-indexed coupling survives ragged block sizes.
    //
    // This is the probe the coupling exists for. A shared packet/frame
    // counter would still pass C3 at a fixed block size and fall apart here:
    // the 8 kHz latch is FRACTIONAL, so the GSM frame boundary lands on a
    // different host sample depending on how the blocks are cut, and any
    // scheme that indexes loss by counting rather than by looking it up at
    // the boundary drifts. Same seed, same params, five block sizes, one
    // answer or it is broken.
    {
        auto configure = [] (OBitrotAudioProcessor& proc)
        {
            setBaseline (proc);
            setParam (proc, "SEED",           4242.0f);
            setParam (proc, "TAPE_ENABLE",    0.0f);
            setParam (proc, "CD_ENABLE",      0.0f);
            setParam (proc, "VINYL_ENABLE",   0.0f);
            setParam (proc, "PACKET_ENABLE",  1.0f);
            setParam (proc, "PACKET_LOSS",    60.0f);
            setParam (proc, "PACKET_BURST",   80.0f);
            setParam (proc, "PACKET_CONCEAL", 1.0f);
            setParam (proc, "CODEC_ENABLE",   1.0f);
            setParam (proc, "CODEC_MODE",     1.0f);   // GSM
            setParam (proc, "CODEC_MIX",      100.0f);
            setParam (proc, "CODEC_AGC",      100.0f);
        };

        const int total = 96000;

        auto a = makeProc();  configure (*a);
        auto b = makeProc();  configure (*b);

        juce::AudioBuffer<float> outA, outB;
        renderInto (*a, outA, total, { 512 }, noiseStereo);
        renderInto (*b, outB, total, { 1, 7, 64, 333, 4096 }, noiseStereo);

        const bool identical = bitIdentical (outA, outB);
        const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("C4 frame-loss-ragged", identical && live,
               (identical ? juce::String ("GSM + packet loss coupled, 512 vs "
                                          "1,7,64,333,4096: bit-identical")
                          : firstDifference (outA, outB))
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // N7 — the serial-post-stage anchor, RE-ANCHORED at v1.8.0.
    //
    // History, because a silently re-recorded digest is worthless: this probe
    // was introduced at v1.5.0 carrying 0x1cf2f80d1f71674c, produced against
    // the v1.4.0 tree (git 2160dd66), to prove the media-noise beds had not
    // disturbed the packet/codec chain. It held through v1.5.0, v1.6.0 and
    // v1.7.0. v1.8.0 breaks it ON PURPOSE and by exactly three mechanisms,
    // all inside CodecStage, all reachable from this config (CODEC_ENABLE on,
    // mu-law, CODEC_MIX 100):
    //
    //   * item 29 — muLawRoundTrip is now the segmented G.711 codec, not the
    //     continuous log curve. Unconditional: no parameter opts out.
    //   * item 16 — CODEC_AGC defaults to 100, so the AGC is in circuit.
    //   * item 7  — inert HERE (mu-law, not GSM) but it shares the call.
    //
    // The v1.4.0 claim is therefore retired rather than re-fitted. What N7
    // still buys is a forward anchor: the v1.8.0 codec chain is pinned for
    // every release after this one. The half of the old claim that survives
    // untouched — everything OUTSIDE the codec — is now carried by N8 below,
    // which anchors to v1.7.0 and must NOT move.
    {
        constexpr juce::uint64 kV180PostDigest = 0xb473105611e3ea78ULL;
        constexpr juce::uint64 kV140PostDigest = 0x1cf2f80d1f71674cULL;   // retired, kept for provenance

        auto p = makeProc();
        configureCanonicalPostRender (*p);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const juce::uint64 digest = renderChecksum (out);
        const bool         live   = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool         match  = digest == kV180PostDigest;
        const bool         moved  = digest != kV140PostDigest;

        check ("N7 v1.8.0-post-identity", match && live && moved,
               juce::String ("digest 0x") + juce::String::toHexString ((juce::int64) digest)
                   + " vs v1.8.0 0x"
                   + juce::String::toHexString ((juce::int64) kV180PostDigest)
                   + (match ? " — codec chain pinned" : " — CODEC CHAIN DRIFTED")
                   + (moved ? "" : " — STILL EQUALS v1.4.0: items 16/29 DID NOT LAND")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // N8 — the v1.7.0 cross-version gate for EVERYTHING EXCEPT THE CODEC.
    //
    // v1.8.0 (brief items 16, 29, 7) deliberately moves the codec render, so
    // N7 above can no longer carry the "nothing downstream changed" claim for
    // this release — it renders WITH the codec on. N8 is the half of N7 that
    // v1.8.0 must not touch: the identical canonical post-render config with
    // CODEC_ENABLE off, which exercises the packet stage (GE chain,
    // concealment, comfort noise) and the whole transport engine in front of
    // it while CodecStage sits on its bit-transparent bypass rail.
    //
    // The digest was produced by THIS probe compiled against the v1.7.0 tree
    // (git 4d52377e), BEFORE any v1.8.0 edit — an anchor recorded after the
    // change would only prove the new engine equals itself.
    {
        constexpr juce::uint64 kV170PacketDigest = 0x8eb6e29da5ec2692ULL;

        auto p = makeProc();
        configureCanonicalPostRender (*p);
        setParam (*p, "CODEC_ENABLE", 0.0f);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const juce::uint64 digest = renderChecksum (out);
        const bool         live   = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool         match  = digest == kV170PacketDigest;

        check ("N8 v1.7.0-packet-identity", match && live,
               juce::String ("digest 0x") + juce::String::toHexString ((juce::int64) digest)
                   + " vs v1.7.0 0x"
                   + juce::String::toHexString ((juce::int64) kV170PacketDigest)
                   + (match ? " — non-codec engine unchanged by v1.8.0"
                            : " — THE CODEC EDIT LEAKED OUTSIDE THE CODEC")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // A1 — ITEM 6: a CD overlay fires under a FOREIGN owner, on every tick.
    //
    // The config is chosen so the classification is forced rather than
    // sampled, which is what lets the pass bar sit at "every tick" instead of
    // at a statistical margin:
    //
    //   * VINYL_PROB 100 — vinyl fires every tick, and both of its event kinds
    //     (jump, locked groove) are OWNER. So vinyl owns every tick.
    //     Deliberately vinyl and not tape: a groove jump leaves the read RATE
    //     at exactly 1.0 (BRIEF:16, "pitch never changes"), so it cannot tilt
    //     the spectrum of a noise input and confound the measurement below.
    //     A tape bend would.
    //   * CD_PROB 100 at CD_SEVERITY 0 — rungFloat = 0*3 + (r-0.5)*1.5 lands in
    //     [-0.75, 0.75] and clamps at 0, so the rung is ALWAYS 0: conceal.
    //     Conceal is OVERLAY (filter domain only, orthogonal to head position),
    //     so CD never contends and its dip must land on all 32 ticks.
    //   * VINYL_POP 0 — pops are broadband transients that RAISE the HF ratio
    //     this probe reads as a dip. Silencing them isolates the conceal;
    //     triggerPop still consumes its five draws at level 0, so the event
    //     schedule is untouched.
    //
    // Under the single-winner arbitration this replaces, both families fired
    // every tick and the arbitration stream picked one: the conceal reached
    // the output on roughly HALF the ticks. So the 0.9 bar is not decoration —
    // this probe FAILS against v1.8.0 at ~0.5, which is the only thing that
    // makes it a gate (pattern_probe_must_target_the_branch_the_fix_changed).
    //
    // Measurement is probe J's HF-ratio-per-window, re-tiled onto the tick
    // grid, with one config detail that is NOT cosmetic: the scan does not
    // start until 12 s in.
    //
    // VINYL_PROB 100 is a deliberately punishing transport setting. Every tick
    // that rolls a backward groove jump ages the head by a whole revolution
    // while the write head advances only one tick period, so the lag walks out
    // to the ring's budget within a couple of seconds and parks there (past
    // that point backRoom fails and the family alternates forward and backward
    // revolutions around the ceiling). The head is then reading material 8-9 s
    // old — which, early in a render, is the ring's PRE-HISTORY, i.e. zeros.
    // Measured on the v1.8.0 tree over an 8 s render: median HF ratio 0.00000
    // across all 341 windows, because the output was silent. The dip bar was
    // being applied to silence.
    //
    // So: render 20 s, and start the scan at 12 s, by which time the ring holds
    // 10 s of real noise and the parked head is reading it. The start offset is
    // also tick-aligned (12 s is exactly 48 tick periods, plus kComp for the
    // reported latency), so each tick sits at the START of its period and its
    // conceal — 30-80 ms into a 250 ms period — cannot straddle a boundary.
    {
        auto p = makeProc();
        setBaseline (*p);
        setParam (*p, "CLOCK_MODE",      1.0f);    // Free
        setParam (*p, "CLOCK_FREE_RATE", 4.0f);
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "CD_SEVERITY",     0.0f);
        setParam (*p, "VINYL_PROB",      100.0f);
        setParam (*p, "VINYL_POP",       0.0f);
        setParam (*p, "SEED",            31337.0f);

        const int total = (int) (20.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const auto*   o          = out.getReadPointer (0);
        const int     tickPeriod = (int) (kFs / 4.0);
        const int     scanFrom   = kComp + 48 * tickPeriod;   // 12 s, tick-aligned
        constexpr int win        = 1024;

        // Global median of the HF ratio, over every window in the scan region.
        std::vector<double> allRatios;
        std::vector<std::vector<double>> perTick;

        for (int t = scanFrom; t + tickPeriod <= total; t += tickPeriod)
        {
            std::vector<double> thisTick;

            for (int w = t; w + win <= t + tickPeriod; w += win)
            {
                double eTot = 0.0, eDiff = 0.0;
                for (int i = 1; i < win; ++i)
                {
                    const double x = o[w + i];
                    const double d = x - (double) o[w + i - 1];
                    eTot  += x * x;
                    eDiff += d * d;
                }
                const double ratio = eDiff / juce::jmax (1.0e-12, eTot);
                thisTick.push_back (ratio);
                allRatios.push_back (ratio);
            }
            perTick.push_back (std::move (thisTick));
        }

        const double median = medianOf (allRatios);
        int dipped = 0;

        for (const auto& tick : perTick)
        {
            const double lo = *std::min_element (tick.begin(), tick.end());
            if (lo < 0.5 * median)
                ++dipped;
        }

        const double frac = perTick.empty()
                                ? 0.0
                                : (double) dipped / (double) perTick.size();

        // Liveness is asserted on the SCAN REGION and on the median ratio, not
        // on the whole buffer: the starved-head failure above left the tail
        // silent while the first second still carried signal, so a whole-buffer
        // magnitude check would have called that render live and applied the
        // dip bar to zeros.
        const bool live   = out.getMagnitude (scanFrom, total - scanFrom) > 1.0e-4f
                            && median > 0.1;
        const bool always = frac >= 0.9;

        check ("A1 cd-overlay-under-foreign-owner", live && always,
               juce::String ("conceal dip present in ") + juce::String (dipped) + "/"
                   + juce::String ((int) perTick.size()) + " tick periods (frac "
                   + juce::String (frac, 3) + ", need >= 0.900), median ratio "
                   + juce::String (median, 4)
                   + (always ? " — overlay fires regardless of the owner"
                             : " — CD STILL LOSES TICKS TO VINYL (single-winner behaviour)")
                   + (live ? "" : " — SCAN REGION SILENT, probe vacuous"));
    }

    //==========================================================================
    // A2 — ITEM 6: the standalone vinyl pop, the overlay residue of a groove
    // jump that LOST the tick.
    //
    // TAPE_PROB 100 with TAPE_STOP_PROB 0 makes every tape event a bend, and a
    // bend is OWNER, so tape owns all 32 ticks. VINYL_PROB 100 means vinyl
    // fires every one of them and loses every one of them. Before v1.9.0 that
    // meant silence from the vinyl family; now the roll still succeeded, so the
    // stylus still hits the debris — the pop fires, the transport just is not
    // vinyl's to move.
    //
    // Twin renders differing ONLY in VINYL_POP (50 vs 0), M3's method: the head
    // schedule, the tape ramps and every RNG stream are identical between them
    // because triggerPop consumes its five draws at level 0 too, so the
    // difference signal is the pop bus and nothing else.
    //
    // Against v1.8.0 this reads ~0.5: vinyl won about half the ticks and popped
    // only there.
    {
        auto configure = [] (OBitrotAudioProcessor& proc, float pop)
        {
            setBaseline (proc);
            setParam (proc, "CLOCK_MODE",      1.0f);    // Free
            setParam (proc, "CLOCK_FREE_RATE", 4.0f);
            setParam (proc, "TAPE_PROB",       100.0f);
            setParam (proc, "TAPE_STOP_PROB",  0.0f);    // bends only: always OWNER
            setParam (proc, "CD_ENABLE",       0.0f);
            setParam (proc, "VINYL_PROB",      100.0f);
            setParam (proc, "VINYL_POP",       pop);
            setParam (proc, "SEED",            31337.0f);
        };

        const int total = (int) (8.0 * kFs);

        auto a = makeProc();  configure (*a, 50.0f);
        auto b = makeProc();  configure (*b, 0.0f);

        juce::AudioBuffer<float> outA, outB;
        renderInto (*a, outA, total, { 512 }, sineStereo);
        renderInto (*b, outB, total, { 512 }, sineStereo);

        // The pop bus, isolated.
        std::vector<float> diff ((size_t) total);
        const auto* pa = outA.getReadPointer (0);
        const auto* pb = outB.getReadPointer (0);
        float peak = 0.0f;

        for (int n = 0; n < total; ++n)
        {
            diff[(size_t) n] = pa[n] - pb[n];
            peak = juce::jmax (peak, std::abs (diff[(size_t) n]));
        }

        const int    tickPeriod = (int) (kFs / 4.0);
        const double thresh     = 0.1 * (double) peak;

        int periods = 0, popped = 0;

        for (int t = kComp; t + tickPeriod <= total; t += tickPeriod)
        {
            ++periods;
            for (int i = t; i < t + tickPeriod; ++i)
            {
                if (std::abs (diff[(size_t) i]) > thresh)
                {
                    ++popped;
                    break;
                }
            }
        }

        const double frac = periods == 0 ? 0.0 : (double) popped / (double) periods;

        const bool live   = peak > 1.0e-3f;
        const bool always = frac >= 0.9;

        check ("A2 standalone-vinyl-pop", live && always,
               juce::String ("pop present in ") + juce::String (popped) + "/"
                   + juce::String (periods) + " tick periods (frac "
                   + juce::String (frac, 3) + ", need >= 0.900), bus peak "
                   + juce::String (peak, 5)
                   + (always ? " — a lost groove jump still leaves its pop"
                             : " — VINYL SILENT ON LOST TICKS (single-winner behaviour)")
                   + (live ? "" : " — POP BUS SILENT, probe vacuous"));
    }

    //==========================================================================
    // A3 — ITEM 6's containment claim, and the probe that actually constrains
    // the rewrite: on any tick with AT MOST ONE firer, v1.9.0 is bit-identical
    // to v1.8.0.
    //
    // This is what makes the overlay split safe to ship without a toggle. The
    // rewrite moves each family's sub-rolls out of the winner-only branch and
    // into a classification step that runs for every FIRER, and it replaces the
    // winner's blanket "release the other two" with a class-aware rule. Neither
    // is observable unless two families fire on the same tick:
    //
    //   * a lone firer either owns the tick (identical install, identical
    //     draws, identical releases on the two silent families) or is a lone
    //     overlay (identical install; the two silent families release exactly
    //     as the old winner released them, and the overlay's own transport is
    //     left alone — which is what v1.8.0 already did for a tape dropout
    //     win, Arbitration.h:182-192);
    //   * a tick with no firer at all releases all three, unchanged.
    //
    // Three single-family renders, one per family, each exercising the branch
    // that moved:
    //
    //   a  tape  — TAPE_DROP 40 puts the dropout roll (the conditional second
    //              draw) and the bend-interval draw on the same stream, which
    //              is the ordering the rollKind/apply split had to preserve.
    //   b  cd    — CD_SEVERITY 0.5 makes all three rungs reachable, so the
    //              bounded-event guard, the rung-change recovery jump and the
    //              loop entry are all in play.
    //   c  vinyl — both kinds plus the pop draws.
    //
    // All three digests were produced by compiling THIS probe against the
    // v1.8.0 tree (git 627f8afb) in a detached worktree, BEFORE any v1.9.0
    // edit. An anchor recorded after the change would only prove the new engine
    // equals itself.
    {
        constexpr juce::uint64 kV180TapeOnly  = 0x24fd1e9c6fae03aaULL;
        constexpr juce::uint64 kV180CdOnly    = 0x8eb70326e6ce1d95ULL;
        constexpr juce::uint64 kV180VinylOnly = 0x9a54f4f8c9ad6a9fULL;

        struct Case
        {
            const char*  name;
            juce::uint64 anchor;
            void (*configure) (OBitrotAudioProcessor&);
        };

        const Case cases[3] = {
            { "tape", kV180TapeOnly, [] (OBitrotAudioProcessor& proc)
              {
                  setBaseline (proc);
                  setParam (proc, "CLOCK_MODE",      1.0f);
                  setParam (proc, "CLOCK_FREE_RATE", 4.0f);
                  setParam (proc, "TAPE_PROB",       100.0f);
                  setParam (proc, "TAPE_STOP_PROB",  10.0f);
                  setParam (proc, "TAPE_DROP",       40.0f);
                  setParam (proc, "CD_ENABLE",       0.0f);
                  setParam (proc, "VINYL_ENABLE",    0.0f);
                  setParam (proc, "SEED",            2024.0f);
              } },
            { "cd", kV180CdOnly, [] (OBitrotAudioProcessor& proc)
              {
                  setBaseline (proc);
                  setParam (proc, "CLOCK_MODE",      1.0f);
                  setParam (proc, "CLOCK_FREE_RATE", 4.0f);
                  setParam (proc, "TAPE_ENABLE",     0.0f);
                  setParam (proc, "CD_PROB",         100.0f);
                  setParam (proc, "CD_SEVERITY",     0.5f);
                  setParam (proc, "VINYL_ENABLE",    0.0f);
                  setParam (proc, "SEED",            2024.0f);
              } },
            { "vinyl", kV180VinylOnly, [] (OBitrotAudioProcessor& proc)
              {
                  setBaseline (proc);
                  setParam (proc, "CLOCK_MODE",      1.0f);
                  setParam (proc, "CLOCK_FREE_RATE", 4.0f);
                  setParam (proc, "TAPE_ENABLE",     0.0f);
                  setParam (proc, "CD_ENABLE",       0.0f);
                  setParam (proc, "VINYL_PROB",      100.0f);
                  setParam (proc, "VINYL_POP",       50.0f);
                  setParam (proc, "SEED",            2024.0f);
              } }
        };

        for (const auto& c : cases)
        {
            auto p = makeProc();
            c.configure (*p);

            const int total = (int) (4.0 * kFs);
            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, noiseStereo);

            const juce::uint64 digest = renderChecksum (out);
            const bool         live   = out.getMagnitude (0, 0, total) > 1.0e-4f;
            const bool         match  = digest == c.anchor;

            const juce::String probeName = juce::String ("A3 single-firer/") + c.name;

            check (probeName.toRawUTF8(), match && live,
                   juce::String ("digest 0x") + juce::String::toHexString ((juce::int64) digest)
                       + " vs v1.8.0 0x" + juce::String::toHexString ((juce::int64) c.anchor)
                       + (match ? " — single-firer ticks bit-unchanged"
                                : " — THE OVERLAY SPLIT LEAKED INTO A SINGLE-FAMILY RENDER")
                       + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    // R1-R5 — the Rot family (v1.10.0, improvement brief item 8).
    //
    // Every one of these runs with tape, cd and vinyl DISABLED, which makes the
    // measurement unusually direct: with the transport families off the wet
    // path is a pure integer delay (that is what probe B asserts), so the
    // reference signal is exactly input[n - kComp] and the difference between
    // the render and that reference IS the rot artifact, with nothing else in
    // it. No twin-render subtraction is needed to isolate the bus.
    {
        // Free-clock at 8 Hz: 125 ms between ticks, comparable to the event
        // durations, so events land often without every tick being swallowed
        // as a discarded retrigger.
        auto configureRot = [] (OBitrotAudioProcessor& proc, float prob, float depth,
                                float stick, float garble)
        {
            setBaseline (proc);
            setParam (proc, "TAPE_ENABLE",     0.0f);
            setParam (proc, "CD_ENABLE",       0.0f);
            setParam (proc, "VINYL_ENABLE",    0.0f);
            setParam (proc, "CLOCK_MODE",      1.0f);    // Free
            setParam (proc, "CLOCK_FREE_RATE", 8.0f);
            setParam (proc, "SEED",            4242.0f);
            setParam (proc, "ROT_ENABLE",      1.0f);
            setParam (proc, "ROT_PROB",        prob);
            setParam (proc, "ROT_DEPTH",       depth);
            setParam (proc, "ROT_STICK",       stick);
            setParam (proc, "ROT_GARBLE",      garble);
        };

        const int total   = (int) (8.0 * kFs);
        const int startAt = kComp + (int) (0.2 * kFs);   // past mixer smoothing

        //======================================================================
        // R1 — bit flips are impulses, and the post-clip actually holds.
        //
        // Three claims, all three two-sided:
        //   * at DEPTH 100 the flip window produces MANY samples that depart
        //     from the reference by more than a bit-11 flip (0.0625 FS);
        //   * at DEPTH 0 it produces NONE of them — the reachable bit field
        //     stops at bit 3, i.e. 0.0001 FS, so DEPTH is doing real work
        //     rather than just existing;
        //   * no sample anywhere leaves [-1, 1].
        //
        // DEPTH 100 rather than 90 for the third clause specifically, and the
        // reason is worth recording because the first version of this probe got
        // it wrong: only at DEPTH 1 does the bit field reach bit 15. Below it
        // every reachable flip moves the word by at most 16384, which from
        // inside +/-32767 cannot escape +/-32768 — so with the clip DELETED
        // the bound still held and the clause passed against the code it was
        // supposed to gate. The sign bit is the only one that can reach 2.0 FS,
        // and with it in range deleting the clip takes the peak to ~1.5.
        {
            auto countDepartures = [&] (float depth, float& peakOut) -> int
            {
                auto p = makeProc();
                configureRot (*p, 100.0f, depth, 0.0f, 0.0f);   // flips only

                juce::AudioBuffer<float> out;
                renderInto (*p, out, total, { 512 }, sineStereo);

                const auto* o = out.getReadPointer (0);
                int   n90     = 0;
                float peak    = 0.0f;

                for (int n = startAt; n < total; ++n)
                {
                    peak = juce::jmax (peak, std::abs (o[n]));
                    if (std::abs (o[n] - sineStereo (0, n - kComp)) > 0.0625f)
                        ++n90;
                }

                peakOut = peak;
                return n90;
            };

            float peakHi = 0.0f, peakLo = 0.0f;
            const int hitsFull = countDepartures (100.0f, peakHi);
            const int hits0    = countDepartures (0.0f,   peakLo);

            const bool loud    = hitsFull > 100;
            const bool quiet   = hits0 == 0;
            const bool bounded = peakHi <= 1.0f && peakLo <= 1.0f;

            check ("R1 rot-flip-impulses", loud && quiet && bounded,
                   juce::String ("depth 100: ") + juce::String (hitsFull)
                       + " samples > 0.0625 from reference (need > 100); depth 0: "
                       + juce::String (hits0) + " (need 0); peak "
                       + juce::String (peakHi, 4)
                       + (bounded ? " — post-clip holds"
                                  : " — OUTPUT LEFT [-1,1], THE XOR OVERFLOWED")
                       + (loud ? "" : " — NO FLIPS LANDED, probe vacuous"));
        }

        //======================================================================
        // R2 — the sticky hold is a real hold, of the length the brief asked
        // for (10-80 ms).
        //
        // Measured as the longest run of BIT-IDENTICAL consecutive output
        // samples. A held sample is DC, and at MIX 100 the mixer gain has
        // settled to exactly 1 by startAt, so the plateau survives to the
        // output bit-for-bit.
        //
        // The window is asserted at both ends and both ends discriminate:
        //   * lower bound — the 1.5 ms entry and exit blends eat 2 x 72 samples
        //     off the shortest possible 10 ms hold, leaving ~336, so anything
        //     under 300 means no plateau ever formed;
        //   * upper bound — 80 ms is the ceiling the kind is specified at, and
        //     3840 samples is where it sits at 48 kHz;
        //   * the STICK 0 control renders the same schedule as bit flips
        //     instead, and a flip window has no plateau at all.
        {
            auto longestFlatRun = [&] (float stick) -> int
            {
                auto p = makeProc();
                configureRot (*p, 100.0f, 50.0f, stick, 0.0f);

                juce::AudioBuffer<float> out;
                renderInto (*p, out, total, { 512 }, sineStereo);

                const auto* o   = out.getReadPointer (0);
                int longest     = 0;
                int run         = 1;

                for (int n = startAt + 1; n < total; ++n)
                {
                    run = bitExact (o[n], o[n - 1]) ? run + 1 : 1;
                    longest = juce::jmax (longest, run);
                }

                return longest;
            };

            const int held = longestFlatRun (100.0f);
            const int flip = longestFlatRun (0.0f);

            const int loBound = 300;
            const int hiBound = (int) (RotStage::kStickMaxSec * kFs);

            const bool inRange = held >= loBound && held <= hiBound;
            const bool control = flip < 100;

            check ("R2 rot-sticky-hold", inRange && control,
                   juce::String ("longest bit-identical run ") + juce::String (held)
                       + " samples (" + juce::String (1000.0 * held / kFs, 1)
                       + " ms), need [" + juce::String (loBound) + ", "
                       + juce::String (hiBound) + "]; STICK 0 control "
                       + juce::String (flip) + " (need < 100)"
                       + (inRange ? "" : " — HOLD MISSING OR OUT OF SPEC")
                       + (control ? "" : " — CONTROL ALSO HOLDS, probe vacuous"));
        }

        //======================================================================
        // R3 — the wrong-decode stretch is ENVELOPE-MATCHED, which is the whole
        // claim of the kind and the one thing that separates it from "a noise
        // gate got stuck open".
        //
        // Method: two renders at the SAME seed and therefore the same event
        // schedule, differing only in input amplitude (0.5 vs 0.05, i.e. 20 dB
        // apart). Over the windows that actually garbled — detected as windows
        // whose output decorrelates from the reference — the ratio of the two
        // renders' RMS must track the ratio of the two INPUTS' RMS.
        //
        // This is the discriminator a plain "is it noisy" check does not give:
        // white noise at a FIXED level passes any noisiness test and fails this
        // one outright, because its ratio would be 1 instead of 10.
        {
            InputFn loudTone  = [] (int ch, int n) noexcept -> float
            {
                return sineStereo (ch, n);            // amplitude 0.5
            };
            InputFn quietTone = [] (int ch, int n) noexcept -> float
            {
                return 0.1f * sineStereo (ch, n);     // 20 dB down
            };

            auto render = [&] (InputFn in, juce::AudioBuffer<float>& out)
            {
                auto p = makeProc();
                configureRot (*p, 100.0f, 50.0f, 0.0f, 100.0f);   // garble only
                renderInto (*p, out, total, { 512 }, in);
            };

            juce::AudioBuffer<float> outLoud, outQuiet;
            render (loudTone,  outLoud);
            render (quietTone, outQuiet);

            const int win = (int) (0.020 * kFs);   // 20 ms
            std::vector<double> ratios;
            double loudRmsSum = 0.0;
            int    garbled    = 0;

            const auto* oL = outLoud.getReadPointer (0);
            const auto* oQ = outQuiet.getReadPointer (0);

            for (int w = startAt; w + win <= total; w += win)
            {
                // Correlate the loud render against its own reference. A
                // window that is passing the programme through correlates ~1;
                // a garbled one is uncorrelated noise.
                double sxy = 0.0, sxx = 0.0, syy = 0.0;
                double rmsL = 0.0, rmsQ = 0.0;

                for (int i = 0; i < win; ++i)
                {
                    const double x = (double) oL[w + i];
                    const double y = (double) sineStereo (0, w + i - kComp);

                    sxy += x * y;
                    sxx += x * x;
                    syy += y * y;

                    rmsL += x * x;
                    rmsQ += (double) oQ[w + i] * (double) oQ[w + i];
                }

                const double denom = std::sqrt (sxx * syy);
                const double corr  = denom > 0.0 ? sxy / denom : 0.0;

                if (std::abs (corr) > 0.5)
                    continue;                       // not garbled — skip

                rmsL = std::sqrt (rmsL / (double) win);
                rmsQ = std::sqrt (rmsQ / (double) win);

                if (rmsQ > 1.0e-6)
                {
                    ratios.push_back (rmsL / rmsQ);
                    loudRmsSum += rmsL;
                    ++garbled;
                }
            }

            double median = 0.0;
            if (! ratios.empty())
            {
                std::sort (ratios.begin(), ratios.end());
                median = ratios[ratios.size() / 2];
            }

            // The inputs are exactly 10x apart. Allow a wide band: the follower
            // is a smoothed RMS, the fades at each end of a stretch are partly
            // programme, and the noise itself is stochastic over a 20 ms window.
            const bool enough  = garbled >= 20;
            const bool tracks  = median > 5.0 && median < 20.0;
            const bool audible = garbled > 0 && (loudRmsSum / juce::jmax (1, garbled)) > 0.05;

            check ("R3 rot-garble-env-match", enough && tracks && audible,
                   juce::String ("garbled windows ") + juce::String (garbled)
                       + " (need >= 20), median loud/quiet RMS ratio "
                       + juce::String (median, 2) + " (need 5-20; inputs are 10x apart"
                       + ", a FIXED-level noise would read 1.0), mean garble RMS "
                       + juce::String (garbled > 0 ? loudRmsSum / garbled : 0.0, 3)
                       + (tracks ? "" : " — NOISE DOES NOT TRACK THE PROGRAMME LEVEL")
                       + (enough ? "" : " — TOO FEW GARBLED WINDOWS, probe vacuous"));
        }

        //======================================================================
        // R4 — QUAL-02 with all three rot kinds live: the single-stream claim.
        //
        // RotStage draws from ONE stream at tick instants AND per sample, which
        // is the shape `scratch` was split out to avoid. The argument for why
        // it is safe here (one subsystem, one loop iteration, fixed order) is
        // in RngBank; this is the measurement. If the tick draws and the
        // per-sample draws could interleave differently at different block
        // sizes, these three renders would diverge.
        {
            auto render = [&] (const std::vector<int>& sizes, juce::AudioBuffer<float>& out)
            {
                auto p = makeProc();
                configureRot (*p, 100.0f, 70.0f, 30.0f, 35.0f);   // all three kinds
                renderInto (*p, out, total, sizes, noiseStereo);
            };

            juce::AudioBuffer<float> a, b, c;
            render ({ 512 }, a);
            render ({ 4096 }, b);
            render ({ 1, 7, 64, 333, 4096 }, c);

            const size_t bytes = sizeof (float) * (size_t) total;
            bool same = true;

            for (int ch = 0; ch < 2 && same; ++ch)
            {
                same = same && std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), bytes) == 0
                            && std::memcmp (a.getReadPointer (ch), c.getReadPointer (ch), bytes) == 0;
            }

            // Liveness: rot must actually be doing something, or three
            // identical passthroughs would pass this trivially.
            bool moved = false;
            {
                const auto* o = a.getReadPointer (0);
                for (int n = startAt; n < total && ! moved; ++n)
                    if (! bitExact (o[n], noiseStereo (0, n - kComp)))
                        moved = true;
            }

            check ("R4 rot-blocksize-identity", same && moved,
                   juce::String (same ? "512 == 4096 == ragged, bit-identical"
                                      : "RENDERS DIVERGE — the rot stream interleaves with block size")
                       + (moved ? " (rot active)" : " — ROT INERT, probe vacuous"));
        }

        //======================================================================
        // R5 — FUNC-02 with the rot knobs at maximum and the family OFF.
        //
        // The containment claim this whole release rests on: while ROT_ENABLE
        // is false the gate short-circuits before its draw, so rot costs
        // nothing, perturbs no stream, and touches no sample — regardless of
        // where the other four knobs sit. This is what keeps the A3/V1/N7/N8
        // cross-version anchors green, and it is asserted with NO tolerance.
        {
            auto p = makeProc();
            setBaseline (*p);
            setParam (*p, "TAPE_ENABLE",  0.0f);
            setParam (*p, "CD_ENABLE",    0.0f);
            setParam (*p, "VINYL_ENABLE", 0.0f);
            setParam (*p, "ROT_ENABLE",   0.0f);      // the one that matters
            setParam (*p, "ROT_PROB",     100.0f);
            setParam (*p, "ROT_DEPTH",    100.0f);
            setParam (*p, "ROT_STICK",    100.0f);
            setParam (*p, "ROT_GARBLE",   100.0f);

            juce::AudioBuffer<float> out;
            renderInto (*p, out, total, { 512 }, noiseStereo);

            bool         ok = true;
            juce::String detail;

            for (int ch = 0; ch < 2 && ok; ++ch)
            {
                const auto* o = out.getReadPointer (ch);
                for (int n = startAt; n < total; ++n)
                {
                    if (! bitExact (o[n], noiseStereo (ch, n - kComp)))
                    {
                        ok = false;
                        detail = juce::String ("first mismatch ch") + juce::String (ch)
                               + " @" + juce::String (n);
                        break;
                    }
                }
            }

            const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

            if (ok && live)
                detail = "bit-exact null with all four ROT knobs at 100";

            check ("R5 rot-off-null", ok && live,
                   detail + (live ? "" : " — SILENT, probe vacuous"));
        }
    }

    //==========================================================================
    std::printf ("\n%d/%d probes passed.\n", probes - failures, probes);
    return failures == 0 ? 0 : 1;
}
