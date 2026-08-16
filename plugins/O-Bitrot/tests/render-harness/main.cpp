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

/** Resets ALL 31 parameters to the parameter-spec defaults. Traps: the
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

    // Tape (4)
    setParam (proc, "TAPE_ENABLE",     1.0f);      // On
    setParam (proc, "TAPE_PROB",       25.0f);
    setParam (proc, "TAPE_STOP_PROB",  10.0f);
    setParam (proc, "TAPE_RAMP",       150.0f);

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

/** Saw period, samples. Power of two so the marker value k/period is EXACT in
    float (k < 2^18 fits the 24-bit mantissa), and > 2x the ring's maximum lag
    (120000 @ 48 kHz) so signed distances never alias. */
constexpr int kSawPeriod = 262144;

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

/** Dual-mono input: same signal on every channel, so a (1,1) run and a (2,2)
    run see literally identical per-sample engine inputs. */
float noiseDualMono (int ch, int n) noexcept
{
    juce::ignoreUnused (ch);
    return noiseAt (n);
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
    // D — DSP-01 stops: forced tape stops. No click anywhere (sample-to-sample
    // delta bounded by ~2x the sine's own max derivative) and a genuine hold:
    // rate 0.0 exactly => a long run of bit-identical output samples.
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
            juce::AudioBuffer<float> out;
            renderSync (120.0, false, out, 48000);
            const int onset = firstDeviation (out, kComp, startAt, 1.0e-3f);
            check ("I FUNC-03 sync-stopped", onset == -1,
                   onset == -1 ? juce::String ("no events while transport stopped (detector sane)")
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

        // Threshold 5e-5: a 4800-sample loop jump moves the saw by only
        // 0.018, spread over the 144-sample fade (~1.3e-4/sample) — the 1e-3
        // vinyl threshold would ride on the chirp alone. skipAfter 600 clears
        // fade (144) + chirp (384).
        const auto events = scanSawEvents (mono, startAt, 600, 5.0e-5);

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

        const int total = 288000;                 // 6 s => 11 ticks
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sawStereo);

        const auto mono   = channelToVector (out, 0);
        const int  startAt = kComp + (int) (0.2 * kFs);
        const auto events  = scanSawEvents (mono, startAt, 160, 1.0e-3);

        constexpr int rev = 64000;

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

        const bool counts = backward >= 2 && forward >= 2 && (int) events.size() >= 8;
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

        const int total = (int) (4.0 * kFs);
        juce::AudioBuffer<float> out;
        renderInto (*p, out, total, { 512 }, sineStereo);

        const auto mono  = channelToVector (out, 0);
        const auto trace = pitchTrace (mono, kSineHz);
        const int  hopStart = 14400 / 256;

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
    // O — DSP-04 Gilbert-Elliott statistics. 60 s render, Silence
    // concealment, transports off: lost packets are detected from the OUTPUT
    // (packet-aligned RMS windows that collapse), never by peeking internal
    // state. With PACKET_LOSS 40 / PACKET_BURST 30: piB = 0.24,
    // E[B] = 3.1 => pBG = 0.3226, and the lost-run continuation probability
    // r = P(next lost | lost) ~= 0.325 (state-weighted: 0.5(1-pBG) + ...).
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
        const bool fracOk  = lostFrac >= 0.08 && lostFrac <= 0.18;          // expected 0.128
        const bool rOk     = rHat >= 0.15 && rHat <= 0.50;                  // expected 0.325
        const bool histOk  = n2 > 0 && n1n2 >= 1.5 && n1n2 <= 6.5;          // expected ~3.1

        check ("O DSP-04 ge-statistics", live && enough && fracOk && rOk && histOk,
               juce::String ((int) lost.size()) + " packets, lostFrac "
                   + juce::String (lostFrac, 3) + " (exp 0.128, [0.08,0.18]), r^ "
                   + juce::String (rHat, 3) + " (exp 0.325, [0.15,0.50]), N1/N2 "
                   + juce::String (n1n2, 2) + " (exp 3.1, [1.5,6.5]), runs "
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
    std::printf ("\n%d/%d probes passed.\n", probes - failures, probes);
    return failures == 0 ? 0 : 1;
}
