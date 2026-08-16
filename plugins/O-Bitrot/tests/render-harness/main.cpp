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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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

} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const int kComp = (int) std::ceil (0.020 * kFs);   // 960 @ 48 kHz
    std::printf ("O-Bitrot render-harness (Phases 2.1 + 2.2) — fs=%.0f, kCompLatency=%d\n", kFs, kComp);

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
    std::printf ("\n%d/%d probes passed.\n", probes - failures, probes);
    return failures == 0 ? 0 : 1;
}
