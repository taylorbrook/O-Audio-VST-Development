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
                                CD_SEGMENT intervals, first onset on the
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

/** Resets ALL 38 parameters to the parameter-spec defaults. Traps: the
    neutral value is often not the range minimum (CRUSH_BITS 16, CRUSH_RATE
    20000, MIX 100); the latching params (the six *_ENABLEs, HARD_EDGES)
    matter most. Called at the top of EVERY probe. */
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
    // L — DSP-02 rung 3 (buffer loop) on the position-marker saw: severity
    // 1.0 forces the loop. Restarts land at EXACT CD_SEGMENT intervals with
    // chirp energy at each restart; the first onset sits on the clock grid
    // (FUNC-01); dropping CD_PROB to 0 releases the loop with a FORWARD
    // recovery jump back to live material.
    {
        auto p = makeProc();
        setParam (*p, "CLOCK_MODE",      1.0f);
        setParam (*p, "CLOCK_FREE_RATE", 2.0f);   // ticks every 24000
        setParam (*p, "TAPE_ENABLE",     0.0f);
        setParam (*p, "VINYL_ENABLE",    0.0f);
        setParam (*p, "CD_PROB",         100.0f);
        setParam (*p, "CD_SEVERITY",     1.0f);
        setParam (*p, "CD_SEGMENT",      100.0f); // 4800 samples
        setParam (*p, "SEED",            42.0f);

        const int total = 144000;
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

        // A 4800-sample loop jump is a far smaller marker step than a vinyl
        // revolution, so it gets its own threshold — a revolution-sized one
        // would ride on the chirp alone. skipAfter 600 clears fade (144) +
        // chirp (384).
        const auto events = scanSawEvents (mono, startAt, 600, sawJumpThresh (4800.0));

        // Classify: wrap-region events (< 95990 input-time) and the recovery
        // (the first FORWARD event after the release region — a +/-1 sample
        // slop in the free-clock accumulator can let one more wrap land at
        // ~96000 before the release tick, so position alone is not enough).
        std::vector<int>    wrapOnsets;    // input-time
        int recoveryOnset = -1; double recoveryDist = 0.0;
        for (const auto& e : events)
        {
            const int onset = e.outIndex - kComp;
            if (onset < 95990)
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
            worstGap = juce::jmax (worstGap, std::abs (gap - 4800));
            if (std::abs (gap - 4800) > 8)
                spacingOk = false;
        }

        const bool firstOnGrid = ! wrapOnsets.empty()
                                 && std::abs (wrapOnsets.front() - 24000) <= 8;

        // Chirp liveness: 2nd-difference energy at each restart vs mid-loop.
        std::vector<double> chirpRatios;
        for (const auto& e : events)
        {
            const int onset = e.outIndex - kComp;
            if (onset >= 95000 || e.outIndex + 2400 >= (int) mono.size())
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

        // Recovery: forward jump (negative dist) near the release tick, and
        // the tail of the render tracks LIVE material again.
        const bool recoveredForward = recoveryOnset >= 95990 && recoveryOnset <= 120100
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
               juce::String ("worst |gap - 4800| = ") + juce::String (worstGap)
                   + " samples (bound 8) over " + juce::String ((int) wrapOnsets.size()) + " restarts");

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

        constexpr int seg   = 19200;
        const int     total = (int) (12.5 * kFs);

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
        int    wraps = 0, recoveries = 0, longestRun = 0, run = 0;
        int    prevWrapOnset = -1;
        int    recoveryOnset = -1;
        double recoveryLagAfter = 0.0;

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

            if (e.distSamples < -(double) seg * 4.0)
            {
                ++recoveries;
                if (recoveryOnset < 0)
                {
                    recoveryOnset    = onset;
                    recoveryLagAfter = lagFromSaw (e.vPost, e.postIndex, kComp);
                }
            }
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
        double postErr = 0.0;
        if (recoveryOnset >= 0)
        {
            const int from = recoveryOnset + kComp + 2000;
            const int to   = juce::jmin (from + 10000, (int) mono.size());
            for (int n2 = from; n2 < to; ++n2)
                postErr = juce::jmax (postErr,
                                      std::abs ((double) mono[(size_t) n2]
                                                - (double) sawAt (n2 - kComp)));
        }

        const bool live       = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool sustained  = longestRun >= 18;           // 26 measured; 2.5 s ring gives 5
        const bool recovered  = recoveries == 1 && recoveryLagAfter < 1000.0;
        const bool tracksLive = recoveryOnset >= 0 && postErr < 1.0e-3;

        check ("L2 item-5 cd-loop-budget", live && sustained && recovered && tracksLive,
               juce::String (longestRun) + " passes in the longest unbroken loop (need >= 18), "
                   + juce::String (wraps) + " total, " + juce::String (recoveries)
                   + " recovery jump(s) (need exactly 1) @" + juce::String (recoveryOnset)
                   + " landing at lag " + juce::String (recoveryLagAfter, 1)
                   + " (need < 1000), post-recovery err vs live "
                   + juce::String (postErr, 6) + " (need < 1e-3)"
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    // M — DSP-03 vinyl on the saw marker: backward jumps are integer
    // revolution multiples (45 RPM => 64000 samples @ 48 kHz), forward jumps
    // land at live, every onset sits on the clock grid (FUNC-01). Pops are
    // silenced (VINYL_POP 0) so the marker stays clean — the pop draws are
    // still consumed, so M2/M3 share this event schedule.
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
        const int  startAt = kComp + warmup;
        constexpr int rev = 64000;

        const auto events  = scanSawEvents (mono, startAt, 160, sawJumpThresh ((double) rev));

        int  backward = 0, forward = 0;
        bool distOk = true, gridOk = true, liveOk = true;
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
                const double lagAfter = lagFromSaw (e.vPost, e.postIndex, kComp);
                if (lagAfter > 100.0)
                {
                    liveOk = false;
                    detail << " FORWARD NOT LIVE lag " << juce::String (lagAfter, 1);
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

        check ("M DSP-03 vinyl-jumps", live && counts && distOk && gridOk && liveOk,
               juce::String ((int) events.size()) + " events (" + juce::String (backward)
                   + " back / " + juce::String (forward) + " fwd), back dists = k*64000 +/-8, "
                   + "onsets on 24000-grid +/-8" + detail
                   + (counts ? "" : " — TOO FEW EVENTS, probe vacuous"));
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
    {
        constexpr juce::uint64 kV130CanonicalDigest = 0x3ee4e028900e47caULL;

        auto p = makeProc();
        configureCanonicalRender (*p);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const juce::uint64 digest = renderChecksum (out);
        const bool         live   = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool         match  = digest == kV130CanonicalDigest;

        check ("V1 v1.3.0-bit-identity", match && live,
               juce::String ("digest 0x") + juce::String::toHexString ((juce::int64) digest)
                   + " vs v1.3.0 0x"
                   + juce::String::toHexString ((juce::int64) kV130CanonicalDigest)
                   + (match ? " — bends/CD/vinyl unchanged at TAPE_DROP=0, TAPE_WOW=0"
                            : " — DEFAULTS ARE NOT TRANSPARENT")
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
    // N7 — the v1.4.0 cross-version gate for the SERIAL POST-STAGES.
    //
    // V1 above pins the transport families back to v1.3.0, but it runs with
    // packet and codec switched off, so it says nothing about the two places
    // v1.5.0 actually touched downstream code: comfort noise threaded into
    // PacketLossStage, and the codec bed added after CodecStage. This render
    // turns both on and touches no v1.5.0 parameter at all.
    //
    // The digest was produced by THIS probe compiled against the v1.4.0 tree
    // (git 2160dd66). Re-rendering the new engine twice would prove nothing.
    {
        constexpr juce::uint64 kV140PostDigest = 0x1cf2f80d1f71674cULL;

        auto p = makeProc();
        configureCanonicalPostRender (*p);

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, noiseStereo);

        const juce::uint64 digest = renderChecksum (out);
        const bool         live   = out.getMagnitude (0, 0, total) > 1.0e-4f;
        const bool         match  = digest == kV140PostDigest;

        check ("N7 v1.4.0-post-identity", match && live,
               juce::String ("digest 0x") + juce::String::toHexString ((juce::int64) digest)
                   + " vs v1.4.0 0x"
                   + juce::String::toHexString ((juce::int64) kV140PostDigest)
                   + (match ? " — packet+codec unchanged at every v1.5.0 default"
                            : " — DEFAULTS ARE NOT TRANSPARENT")
                   + (live ? "" : " — SILENT, probe vacuous"));
    }

    //==========================================================================
    std::printf ("\n%d/%d probes passed.\n", probes - failures, probes);
    return failures == 0 ? 0 : 1;
}
