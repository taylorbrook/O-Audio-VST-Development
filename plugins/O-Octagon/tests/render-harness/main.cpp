/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

    O-Octagon render harness — the PLUGIN-level gate for Phases 2.1, 2.2 and 2.3.

    Instantiates OOctagonProcessor directly, negotiates bus layouts programmatically, and renders
    offline. No DAW, no hardware, no MIDI.

    ── Phase 4.1 ────────────────────────────────────────────────────────────────────────────────
      CP N5's fix — a factory preset load leaves THE TWELVE (srcX/srcY/srcZ/decorr + w1..w8) BIT-
         unchanged. The six-changed clause passes with the bug present; clause 3b IS the probe.
         Hermetic: it deletes and regenerates its own probe-scoped store, because WR-04's
         .factory-version sentinel would otherwise let it read a file from an earlier iteration
         and PASS. Uses "Distant Field", never "Concert Default" — the latter's six values ARE the
         defaults, so it cannot distinguish "applied" from "reset".
      CQ COMPAT-04/2's render clause — full-scale signal through the SAFE fold at both widths
         (mono-out and stereo-out), parameters at their range extremes, every sample finite

    ── Phase 2.3 ────────────────────────────────────────────────────────────────────────────────
      AY DSP-06/1 — width = 0 gives gL == gR BITWISE, with a width = 4 negative control
      AZ QUAL-04/3 + QUAL-01/1 — the five parameters AS does not sweep (11 + 5 + BC's 1 = 17 of
         18; v1.5.0's decorr is covered by CV/CW/CX instead — see AZ's note on why not here)
      BA DSP-06/4 — a puck swept THROUGH the centroid at width = 6 stays continuous (risk R5)
      BB QUAL-01/2 — the hull crossing: DC, 1 kHz and 8 kHz; entry BIT-EXACT (P27), exit measured
         against the predicted A·|H_20k(f) − 1| (H2)
      BC QUAL-01/1 for airAmount — the two-render differential, bounds derived in-probe (P34)
      BD DSP-07/2,5,6,7 — bit-transparency proven STRUCTURALLY, each half with a non-vacuity
         control (P33)
      BE DSP-07/8 — NaN recovery, INCLUDING the skipped-filter hole a per-block output guard
         structurally cannot see (risk R6)
      BF FUNC-07/1-4 — ±dB on one lane only, through a NON-IDENTITY map, venue-scoped
      BG QUAL-01 under D4's scope — one representative live venue edit during playback
      BH P29/H5 — the permanent-silence latch FUNC-07's multiply would otherwise have armed
      BI QUAL-03 with width, trim and BOTH filters live — AL's and AM's shapes, re-run
      BJ PERF-01/02 as numbers — airCutoffUpdates == solveRuns·2, and powCalls STILL exactly 32

    ── Phase 2.1 ────────────────────────────────────────────────────────────────────────────────
      Q' Lane speakerToBuffer[j] reproduces the input at unity with w = δ_ij — RE-SPECIFIED at 2.2
      R  1, 2 and 8 output channels: finite, non-crashing, correct SAFE/REAL selection
      S  The F3 hazard DIRECTLY — 7.1 layout, buffer of 3..7 channels, getTotalNumOutputChannels()
         still reporting 8. The state G1 describes, now a tested path rather than a reasoned one
      T  Session round-trip with a NON-DEFAULT venue — edited coordinates and a permuted label map
      U  A Stage-1-shaped session (no VENUE child at all) restores to the §OQ4 defaults

    ── Phase 2.2 ────────────────────────────────────────────────────────────────────────────────
      AI FUNC-01/3 — 8 pairwise-distinct lanes for an OFF-CENTRE source under a non-identity map
      AJ Channel-map Layer 3 — tone per speaker, FFT per lane, non-identity map (MANDATORY)
      AK DSP-04/3 — rakeRear moves a REAR source's gain vector AND leaves a srcY=0 source alone
      AL QUAL-03 mandated — 512 vs 4096, events at 4096 multiples, memcmp
      AM QUAL-03 real gate — RAGGED block sizes vs fixed 4096, arbitrary event offsets, memcmp
      AN QUAL-03 — parameters held constant, several block-size pairs, memcmp
      AO PERF-01/1 — the whole replaced operator new/delete family counts ZERO in processBlock
      AP PERF-01/3 + PERF-02/1,2 — hullProjections and solveRuns as numbers
      AQ H1 regression — a venue edit published BETWEEN two processBlock calls is picked up
      AR QUAL-02/4 — a PARAMETER NaN (not only an input NaN) does not latch
      AS QUAL-04/1,2 — no zipper on a full-speed sweep, WITH A NEGATIVE CONTROL
      AT SAFE mode + the exactly-once invariant at all four auval configs

    Exit 0 iff every probe passes.

  ==============================================================================
*/
#include <JuceHeader.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>
#include <utility>
#include <vector>

#include "PluginProcessor.h"

// Phase 3.2. Probes BW / BX drive the preset manager directly — the harness has no editor, and the
// FUNC-05 guarantee lives entirely in this header: applyPresetJson iterates
// processor.getParameters() and resolves via parameters.getParameter(id), so it can never walk
// apvts.state's children, where the VENUE node lives.
#include "Data/VenueFile.h"
#include "OuariconPresetManager.h"

// Phase 4.1. The six factory definitions and the preserving load. Deliberately NOT in
// PluginEditor.cpp — that TU is permanently excluded from this target, and a preset rule written
// there would be unreachable by any probe (PLAN-4.1 P92).
#include "Data/PresetPolicy.h"

//==============================================================================
// ── PROBE AO: the allocation counter (PERF-01 criterion 1) ────────────────────────────────────
//
// -fsanitize=realtime is UNSUPPORTED by Apple clang 17.0.0 — verified by running it, not assumed
// (RESEARCH-2.2 H8). Allocation is therefore MEASURED here by replacing the global operator new
// family; locks and file I/O remain grep + inspection, and SUMMARY-2.2 says so rather than claiming
// an "RT-safety harness pass".
//
// EVERY variant is replaced, including both std::align_val_t overloads and every matching delete.
// An un-replaced aligned-new is silently uncounted, and a probe that counts nothing passes.

namespace rtcheck
{
    std::atomic<long long> allocations { 0 };
    std::atomic<bool>      armed { false };

    // ── THE COUNTER IS THREAD-FILTERED, AND IT HAD TO BECOME SO (VERIFY-3.3) ──────────────────
    // `armed` is a PROCESS-WIDE flag, so before this filter existed note() counted an allocation
    // made by ANY thread inside the window — including the JUCE message thread and the macOS
    // runtime threads a console app keeps alive. That is not a hypothesis: probe AO reported
    // "1 allocation" once in 35 runs at execute, did not reproduce, and was carried to verify
    // UNATTRIBUTED. Verify reproduced it 4 times in 40 runs under 8-way CPU load and attributed
    // EVERY ONE of the four to a thread other than the one that armed the counter and called
    // processBlock — 4/4, no exceptions. Contention does not make processBlock allocate; it
    // widens the window in which some other thread's allocation lands inside it.
    //
    // So the measurement is scoped to the arming thread, which is the thread processBlock runs on
    // in this harness and is exactly what PERF-01/1 is a claim about. The foreign count is still
    // TAKEN and still REPORTED beside the verdict — a filter that made the artifact invisible
    // would trade a flaky probe for a silent one.
    std::atomic<long long> foreignAllocations { 0 };
    std::atomic<bool>      ownerValid { false };
    std::thread::id        ownerThread {};

    inline void note() noexcept
    {
        if (! armed.load (std::memory_order_relaxed))
            return;

        if (ownerValid.load (std::memory_order_relaxed)
            && std::this_thread::get_id() != ownerThread)
        {
            foreignAllocations.fetch_add (1, std::memory_order_relaxed);
            return;
        }

        allocations.fetch_add (1, std::memory_order_relaxed);
    }

    /** Arms the counter ON THE CALLING THREAD and zeroes both tallies. Every arm site goes
        through this, so a later probe cannot re-introduce the unattributed form by writing
        armed.store (true) directly.
    */
    inline void arm (bool resetForeign = true) noexcept
    {
        allocations.store (0);

        // A probe that arms a SECOND window inside the same verdict passes false, so the foreign
        // tally it finally reports covers every window it measured rather than only the last.
        if (resetForeign)
            foreignAllocations.store (0);
        ownerThread = std::this_thread::get_id();
        ownerValid.store (true);
        armed.store (true);
    }

    inline void disarm() noexcept
    {
        armed.store (false);
    }

    /** " [+N foreign-thread]" when a background thread allocated inside the window, empty
        otherwise. Appended to every allocation verdict so the artifact stays visible.
    */
    inline juce::String foreignNote()
    {
        const auto f = foreignAllocations.load();

        if (f == 0)
            return {};

        juce::String s;
        s << " [+" << static_cast<int> (f) << " foreign-thread, NOT counted — see rtcheck]";
        return s;
    }
}

void* operator new (std::size_t size)
{
    rtcheck::note();

    if (void* p = std::malloc (size == 0 ? 1 : size))
        return p;

    throw std::bad_alloc();
}

void* operator new[] (std::size_t size)                              { return ::operator new (size); }

void* operator new (std::size_t size, std::align_val_t alignment)
{
    rtcheck::note();

    // posix_memalign rather than std::aligned_alloc: libc++ only exposes the latter behind
    // _LIBCPP_HAS_ALIGNED_ALLOC, and a probe that fails to BUILD on one toolchain is a probe that
    // silently stops running. free() is the correct deallocator for both.
    std::size_t a = static_cast<std::size_t> (alignment);

    if (a < sizeof (void*))
        a = sizeof (void*);

    void* p = nullptr;

    if (posix_memalign (&p, a, size == 0 ? a : size) == 0 && p != nullptr)
        return p;

    throw std::bad_alloc();
}

void* operator new[] (std::size_t size, std::align_val_t alignment)
{
    return ::operator new (size, alignment);
}

void operator delete (void* p) noexcept                              { std::free (p); }
void operator delete[] (void* p) noexcept                            { std::free (p); }
void operator delete (void* p, std::size_t) noexcept                 { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept               { std::free (p); }
void operator delete (void* p, std::align_val_t) noexcept            { std::free (p); }
void operator delete[] (void* p, std::align_val_t) noexcept          { std::free (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept   { std::free (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }

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

    std::printf ("  [%s] %-30s %s\n", ok ? "PASS" : "FAIL", name, detail.toRawUTF8());
}

bool near (float a, float b, float tol) noexcept
{
    return std::abs (a - b) <= tol;
}

/** Bit-exact float comparison via the object representation. No `==`, so no -Wfloat-equal. */
bool bitExact (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof (float)) == 0;
}

constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;

/** Negotiates a layout on a fresh processor and prepares it. Returns false if the predicate
    rejected the layout, which is itself a useful assertion. */
bool negotiate (OOctagonProcessor& proc,
                const juce::AudioChannelSet& in,
                const juce::AudioChannelSet& out)
{
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (in);
    layout.outputBuses.add (out);

    if (! proc.setBusesLayout (layout))
        return false;

    proc.setRateAndBufferSizeDetails (kSampleRate, kBlockSize);
    proc.prepareToPlay (kSampleRate, kBlockSize);
    return true;
}

/** A deterministic, non-trivial test signal — a sine at an irrational-ish period so no block
    boundary lands on a zero crossing. */
float testSample (int n) noexcept
{
    return 0.5f * std::sin (static_cast<float> (n) * 0.0731f);
}

bool allFinite (const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int n = 0; n < b.getNumSamples(); ++n)
            if (! std::isfinite (b.getSample (ch, n)))
                return false;

    return true;
}

//==============================================================================
// ── Phase 2.2 helpers ─────────────────────────────────────────────────────────────────────────

/** POSITION-DETERMINISTIC broadband excitation — a hash of the ABSOLUTE sample index.

    THE BLOCK-SIZE PROBES DEPEND ON THIS BEING A FUNCTION OF n AND NOTHING ELSE. A sequential
    generator advanced per call would emit a different signal for a 512-render than for a
    4096-render, so the two runs would not share an input at all and the memcmp would be comparing
    two different experiments (pattern_rng_stream_interleave_blocksize). Modelled on
    O-ReverseDelay's noiseAt().
*/
float noiseAt (int n) noexcept
{
    std::uint32_t h = static_cast<std::uint32_t> (n) * 2654435761u + 0x9E3779B9u;
    h ^= h >> 15;  h *= 0x85EBCA6Bu;
    h ^= h >> 13;  h *= 0xC2B2AE35u;
    h ^= h >> 16;

    return 0.5f * (static_cast<float> (h) / 2147483648.0f - 1.0f);
}

/** Writes a parameter in ENGINEERING UNITS, synchronously.

    setValueNotifyingHost() is fully synchronous — no timer, no message loop (RESEARCH-2.2 Q1). Bare
    setValue() would NOT notify and would leave the cached atomic stale, so a harness written against
    it would render every block size against the DEFAULT gain vector and report QUAL-03 green having
    tested nothing.
*/
void setParam (OOctagonProcessor& proc, const char* id, float engineeringValue)
{
    auto* p = proc.getAPVTS().getParameter (id);
    jassert (p != nullptr);

    if (p != nullptr)
        p->setValueNotifyingHost (p->convertTo0to1 (engineeringValue));
}

/** Sets all 8 weights at once. */
void setWeights (OOctagonProcessor& proc, const std::array<float, 8>& w)
{
    for (int i = 0; i < 8; ++i)
        setParam (proc, oo::params::id (oo::params::w1 + i), w[(size_t) i]);
}

/** One automation event: write these parameters when the render reaches this ABSOLUTE sample. */
struct Event
{
    int         atSample;
    const char* id;
    float       value;
};

/** v1.8.0 — a host transport for the harness. NOT INSTALLED BY DEFAULT: every probe that predates
    the motion engine renders with proc.getPlayHead() == nullptr, exactly as before (RESEARCH Risk
    7 — the full suite was re-run with this class present but uninstalled before any motion probe
    was added). A probe that wants tempo installs it with proc.setPlayHead (&ph) and renderInto()
    advances ppq per block while `playing`. */
struct HarnessPlayHead : public juce::AudioPlayHead
{
    double bpm      { 120.0 };
    double ppqStart { 0.0 };
    bool   playing  { true };
    bool   valid    { true };    // false: report no position at all (a host with no transport)

    /// Samples rendered while playing. The PPQ is DERIVED from this, never accumulated in
    /// beats — an accumulated ppq would carry the harness's own rounding into probe DH.
    std::int64_t samplesElapsed { 0 };

    double ppq() const noexcept
    {
        return ppqStart + static_cast<double> (samplesElapsed) / kSampleRate * (bpm / 60.0);
    }

    void advance (int samples) noexcept
    {
        if (playing)
            samplesElapsed += samples;
    }

    juce::Optional<PositionInfo> getPosition() const override
    {
        if (! valid)
            return {};

        PositionInfo info;
        info.setBpm (bpm);
        info.setPpqPosition (ppq());
        info.setIsPlaying (playing);
        return info;
    }
};

/** Renders `totalSamples` into `dest`, breaking processBlock calls so that:
      - no call spans an event, and
      - no call exceeds the size the sequence asks for.

    `sizes` is walked cyclically. A single-entry sequence is a fixed block size; a ragged sequence is
    P21's real gate. Both renders in a QUAL-03 pair therefore see every event at the SAME absolute
    sample while chopping the buffer completely differently, which is exactly the variable under test.
*/
void renderInto (OOctagonProcessor& proc, juce::AudioBuffer<float>& dest,
                 int totalSamples, const std::vector<int>& sizes,
                 const std::vector<Event>& events, HarnessPlayHead* ph = nullptr)
{
    juce::MidiBuffer midi;

    const int numOut = dest.getNumChannels();
    dest.clear();

    size_t nextEvent = 0;
    size_t sizeIndex = 0;
    int    n = 0;

    // Events landing at sample 0 must be applied before the first block.
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

        juce::AudioBuffer<float> block (numOut, chunk);
        block.clear();

        for (int s = 0; s < chunk; ++s)
            block.setSample (0, s, noiseAt (n + s));

        proc.processBlock (block, midi);

        // The transport advances AFTER the block, so the ppq the block saw was its START ppq —
        // which is what a host reports, and what GainStage extrapolates from.
        if (ph != nullptr)
            ph->advance (chunk);

        for (int ch = 0; ch < numOut; ++ch)
            dest.copyFrom (ch, n, block, ch, 0, chunk);

        n += chunk;

        while (nextEvent < events.size() && events[nextEvent].atSample <= n)
        {
            setParam (proc, events[nextEvent].id, events[nextEvent].value);
            ++nextEvent;
        }
    }
}

/** v1.8.0 — renders `numBlocks` blocks of `blockSize` and returns the motion offset GainStage
    published at each block's first control boundary (liveMotionOffset() read after the block).
    `onBlock (i)` runs BEFORE block i, so a probe can move a parameter or stop the transport at an
    exact block. The audio goes to `dest` when non-null. */
using OffsetSeries = std::vector<std::array<float, 3>>;

template <typename OnBlock>
OffsetSeries renderOffsets (OOctagonProcessor& proc, int numBlocks, int blockSize,
                            HarnessPlayHead* ph, juce::AudioBuffer<float>* dest, OnBlock onBlock)
{
    juce::MidiBuffer midi;
    OffsetSeries     out;
    out.reserve (static_cast<size_t> (numBlocks));

    const int numOut = dest != nullptr ? dest->getNumChannels() : 8;

    for (int i = 0; i < numBlocks; ++i)
    {
        onBlock (i);

        juce::AudioBuffer<float> block (numOut, blockSize);
        block.clear();

        for (int s = 0; s < blockSize; ++s)
            block.setSample (0, s, noiseAt (i * blockSize + s));

        proc.processBlock (block, midi);

        if (ph != nullptr)
            ph->advance (blockSize);

        if (dest != nullptr)
            for (int ch = 0; ch < numOut; ++ch)
                dest->copyFrom (ch, i * blockSize, block, ch, 0, blockSize);

        out.push_back (proc.liveMotionOffset());
    }

    return out;
}

bool sameSeries (const OffsetSeries& a, const OffsetSeries& b)
{
    return a.size() == b.size()
        && std::memcmp (a.data(), b.data(), sizeof (std::array<float, 3>) * a.size()) == 0;
}

/** memcmp, NOT a tolerance. QUAL-03 says bit-identical and a tolerance would hide the very
    divergence the control grid exists to prevent. */
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

/** FNV-1a over the RAW FLOAT BYTES of every lane, for the cross-version bit-identity anchor.

    A DIGEST AND NOT A memcmp BECAUSE THE REFERENCE LIVES IN ANOTHER BINARY. bitIdentical() compares
    two buffers rendered in ONE process, which is the right tool for QUAL-03 and useless for "v1.5.0
    at decorr = 0 renders what v1.4.0 rendered" — there is no v1.4.0 buffer to hold. A digest
    captured from the v1.4.0 binary and transcribed into the probe is the only form that claim can
    take (pattern_reanchor_cross_version_digest_probe: re-anchor deliberately, never re-record
    silently).

    Raw bytes, not values: a float-by-float accumulate would let a compiler contract or reorder the
    arithmetic and change the digest without the audio changing.
*/
std::uint64_t bufferDigest (const juce::AudioBuffer<float>& b)
{
    std::uint64_t h = 1469598103934665603ull;

    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        const auto* bytes = reinterpret_cast<const unsigned char*> (b.getReadPointer (ch));

        for (std::size_t i = 0; i < sizeof (float) * static_cast<std::size_t> (b.getNumSamples()); ++i)
        {
            h ^= bytes[i];
            h *= 1099511628211ull;
        }
    }

    return h;
}

/** A weight vector with a single live speaker — w = δ_ij. */
std::array<float, 8> deltaWeights (int j)
{
    std::array<float, 8> w {};
    w[(size_t) j] = 1.0f;
    return w;
}

/** THE INDEPENDENT GROUND TRUTH for "which buffer channel does speaker j own?".

    Deliberately NOT proc's speakerToBuffer, and deliberately not ochan::buildSpeakerToBuffer either:
    both are the thing under test. This asks JUCE directly — the label resolves to a ChannelType and
    AudioChannelSet says where that type sits in the buffer. If the plugin's map disagrees, the audio
    lands in the wrong lane and the probe fails, which is the entire point of Layer 3.
*/
int expectedLane (const OOctagonProcessor& proc, const juce::AudioChannelSet& set, int speaker)
{
    return set.getChannelIndexForType (proc.getVenue().labelType (speaker));
}

/** Rotates the venue's labels by one so the channel map is NOT the identity.

    C1: all three accepted 8-channel containers have initializer order == enum-bit order, so the
    SHIPPED default map IS the identity and a probe driven by it is byte-identical to a hardcoded
    0..7. Only a permuted label assignment can tell the two apart.
*/
void applyRotatedLabels (OOctagonProcessor& proc)
{
    oo::VenueModel v = proc.getVenue();

    const std::array<const char*, 8> rotated { "R", "C", "Lfe", "Lss", "Rss", "Lrs", "Rrs", "L" };

    for (int i = 0; i < 8; ++i)
        v.setSpeakerLabel (i, rotated[(size_t) i]);

    proc.applyVenueEdit (v);
}

/** Renders `samples` of steady state, leaving the LAST block in `dest` so the smoothers have
    arrived (H10: a 5 ms ramp is 240 samples and must not be measured mid-flight).

    @returns the ABSOLUTE sample index at which the block left in `dest` begins. Callers comparing
             against noiseAt() need it — dest is overwritten each chunk, so it holds the tail of the
             render rather than its head.
*/
int renderSteady (OOctagonProcessor& proc, juce::AudioBuffer<float>& dest, int samples,
                  float dcLevel = 0.0f, bool useDc = false)
{
    juce::MidiBuffer midi;
    const int numOut = dest.getNumChannels();

    juce::AudioBuffer<float> block (numOut, kBlockSize);

    int rendered = 0;
    int lastStart = 0;

    while (rendered < samples)
    {
        lastStart = rendered;
        const int chunk = juce::jmin (kBlockSize, samples - rendered);

        block.clear();

        for (int s = 0; s < chunk; ++s)
            block.setSample (0, s, useDc ? dcLevel : noiseAt (rendered + s));

        juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), numOut, chunk);
        proc.processBlock (view, midi);

        for (int ch = 0; ch < numOut; ++ch)
            dest.copyFrom (ch, 0, view, ch, 0, chunk);

        rendered += chunk;
    }

    return lastStart;
}

/** The gain vector as it appears at the OUTPUT, measured with a DC input after the smoothers have
    settled. Requires >= 240 still samples first (H10). */
std::array<float, 8> measureGainVector (OOctagonProcessor& proc)
{
    juce::AudioBuffer<float> out (8, kBlockSize);
    out.clear();

    renderSteady (proc, out, 4 * kBlockSize, 1.0f, true);

    std::array<float, 8> g {};

    for (int ch = 0; ch < 8; ++ch)
        g[(size_t) ch] = out.getSample (ch, kBlockSize - 1);

    return g;
}

bool vectorsDiffer (const std::array<float, 8>& a, const std::array<float, 8>& b, float tol)
{
    for (int i = 0; i < 8; ++i)
        if (std::abs (a[(size_t) i] - b[(size_t) i]) > tol)
            return true;

    return false;
}

//==============================================================================
// ── Phase 2.3 helpers ─────────────────────────────────────────────────────────────────────────

/** renderInto() with a caller-supplied excitation instead of noiseAt().

    A separate function rather than a defaulted parameter on renderInto: the seventeen 2.1/2.2
    probes drive that one and none of them should change shape at this phase.
*/
template <typename Gen>
void renderGenInto (OOctagonProcessor& proc, juce::AudioBuffer<float>& dest,
                    int totalSamples, const std::vector<int>& sizes,
                    const std::vector<Event>& events, Gen&& gen)
{
    juce::MidiBuffer midi;

    const int numOut = dest.getNumChannels();
    dest.clear();

    size_t nextEvent = 0;
    size_t sizeIndex = 0;
    int    n = 0;

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

        juce::AudioBuffer<float> block (numOut, chunk);
        block.clear();

        for (int s = 0; s < chunk; ++s)
            block.setSample (0, s, gen (n + s));

        proc.processBlock (block, midi);

        for (int ch = 0; ch < numOut; ++ch)
            dest.copyFrom (ch, n, block, ch, 0, chunk);

        n += chunk;

        while (nextEvent < events.size() && events[nextEvent].atSample <= n)
        {
            setParam (proc, events[nextEvent].id, events[nextEvent].value);
            ++nextEvent;
        }
    }
}

/** A pure sine of the absolute sample index — deterministic, so two renders share it exactly. */
struct Sine
{
    float freqHz, amplitude;

    float operator() (int n) const noexcept
    {
        return amplitude * std::sin (juce::MathConstants<float>::twoPi * freqHz
                                     * static_cast<float> (n) / static_cast<float> (kSampleRate));
    }
};

/** The normalised puck position FARTHEST outside the plugin's own hull.

    SEARCHED FOR, NEVER GUESSED — the reasoning is probe AP's and it still applies: srcX/srcY are
    normalised into the speaker BOUNDING BOX, so only its corners fall outside the hull, and the
    obvious guess (0,0) denormalises to exactly speaker 1, which is a hull VERTEX that isInside()
    correctly reports as inside. A guessed position yields a silently vacuous probe.

    ── FARTHEST, NOT MERELY THE FIRST ONE FOUND ─────────────────────────────────────────────────
    Returning the first hit walks out at nx = 0.0, where d_hull is a few centimetres — the trim is
    then ~0.3 dB and the cutoff barely leaves its ceiling, so every DSP-07 probe driven from it
    would be technically non-vacuous and practically asleep. AND nx = 0.0 is a trap of its own: a
    probe that perturbs the position by SCALING it (`nx * 0.95f`) moves nothing at all, which is how
    a counter identity can pass as 0 == 0.

    @returns false when no outside position exists, which callers must assert on.
*/
bool findOutside (const OOctagonProcessor& proc, float& nx, float& ny)
{
    const auto& venue   = proc.getVenue();
    const auto& theHull = proc.getHull();

    float best = 0.0f;
    bool  any  = false;

    for (int ix = 0; ix <= 40; ++ix)
        for (int iy = 0; iy <= 40; ++iy)
        {
            const float tx = 0.025f * static_cast<float> (ix);
            const float ty = 0.025f * static_cast<float> (iy);
            const auto  m  = venue.normToMetres (tx, ty);

            if (theHull.isInside ({ m.x, m.y }))
                continue;

            const float d = theHull.project ({ m.x, m.y }).distance;

            if (d > best)
            {
                best = d;
                nx   = tx;
                ny   = ty;
                any  = true;
            }
        }

    return any;
}

/** The AMPLITUDE of a steady sine in one channel over [from, to), via RMS.

    NOT max|x|, AND THE DIFFERENCE IS A REAL DEFECT RATHER THAN A REFINEMENT. At 8 kHz on a 48 kHz
    grid there are six samples per cycle, so depending on phase the sampled maximum can sit at
    sin 60° = 0.866 of the true amplitude — a 13.4% under-read, which is exactly the margin by which
    a peak-based bound was first violated here. RMS is phase-independent, and over an integer number
    of cycles it is exact.
*/
float sineAmplitude (const juce::AudioBuffer<float>& b, int ch, int from, int to) noexcept
{
    double sum = 0.0;

    for (int n = from; n < to; ++n)
    {
        const double s = b.getSample (ch, n);
        sum += s * s;
    }

    const int count = to - from;

    return count > 0 ? static_cast<float> (std::sqrt (2.0 * sum / count)) : 0.0f;
}

/** As above, for the sample-wise difference of two buffers. */
float sineAmplitudeOfDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b,
                           int ch, int from, int to) noexcept
{
    double sum = 0.0;

    for (int n = from; n < to; ++n)
    {
        const double d = static_cast<double> (a.getSample (ch, n)) - b.getSample (ch, n);
        sum += d * d;
    }

    const int count = to - from;

    return count > 0 ? static_cast<float> (std::sqrt (2.0 * sum / count)) : 0.0f;
}

/** d_hull in metres for a normalised puck position — the same quantity solveSubPoint returns.

    Uses the plugin's own hull deliberately: this is not testing the hull (probes I/J/K/L do that),
    it is deriving the bound a FILTER probe asserts against, and that bound must be the one the
    filter actually saw.
*/
float dHullAtNorm (const OOctagonProcessor& proc, float nx, float ny)
{
    const auto m = proc.getVenue().normToMetres (nx, ny);

    if (proc.getHull().isInside ({ m.x, m.y }))
        return 0.0f;

    return proc.getHull().project ({ m.x, m.y }).distance;
}

/** Walks from an inside point toward an outside one and returns the normalised position whose
    d_hull first reaches `targetMetres`. Keeps hull-crossing probes near the boundary, which is
    both the musically realistic gesture and the case the |H − 1| bound is derived for. */
std::pair<float, float> normAtHullDistance (const OOctagonProcessor& proc,
                                            float inNx, float inNy,
                                            float outNx, float outNy,
                                            float targetMetres)
{
    float lo = 0.0f, hi = 1.0f;

    for (int it = 0; it < 60; ++it)
    {
        const float mid = 0.5f * (lo + hi);
        const float nx  = inNx + mid * (outNx - inNx);
        const float ny  = inNy + mid * (outNy - inNy);

        if (dHullAtNorm (proc, nx, ny) < targetMetres)
            lo = mid;
        else
            hi = mid;
    }

    return { inNx + hi * (outNx - inNx), inNy + hi * (outNy - inNy) };
}

//==============================================================================
// ── The TPT one-pole's transfer function, derived rather than tabulated ───────────────────────
//
// From juce_FirstOrderTPTFilter::processSample:  v = G(x − s);  y = v + s;  s = y + v
// which is  H(z) = G(1 + z^-1) / (1 + (2G − 1)z^-1),  the bilinear transform of 1/(1 + s/wc).
//
// Written out here rather than mirrored from a table so the bound moves if the filter does
// (pattern_test_fixture_mirrors_drift_silently).

double tptG (double fcHz, double fs) noexcept
{
    const double g = std::tan (juce::MathConstants<double>::pi * fcHz / fs);

    return g / (1.0 + g);
}

/** |H(f) − 1| — THE FULL COMPLEX STEP, not | |H| − 1 |.

    RESEARCH-2.3 H2: the step at a hull crossing is the difference between two signal PATHS, so the
    quantity is complex, and the filter's PHASE LAG dominates everywhere below ~15 kHz — by 114x at
    1 kHz and 13x at 8 kHz. §3.5.2's magnitude-only figures understate the accepted cost by 5–190x.
*/
double magHMinusOne (double fcHz, double fs, double freqHz) noexcept
{
    const double G = tptG (fcHz, fs);

    const std::complex<double> z = std::exp (std::complex<double> (
                                       0.0, -2.0 * juce::MathConstants<double>::pi * freqHz / fs));

    const std::complex<double> H = G * (1.0 + z) / (1.0 + (2.0 * G - 1.0) * z);

    return std::abs (H - 1.0);
}

/** The largest per-sample |Δ| in one channel over [from, to). */
float worstStepIn (const juce::AudioBuffer<float>& b, int ch, int from, int to) noexcept
{
    float worst = 0.0f;

    for (int n = juce::jmax (1, from); n < to; ++n)
        worst = std::max (worst, std::abs (b.getSample (ch, n) - b.getSample (ch, n - 1)));

    return worst;
}

/** Sets one speaker's calibration trim, preserving every other venue value. */
void applyTrim (OOctagonProcessor& proc, int speakerIndex, float trimDb)
{
    oo::VenueModel v = proc.getVenue();
    v.setSpeakerTrimDb (speakerIndex, trimDb);
    proc.applyVenueEdit (v);
}

/** THE 5 ms SMOOTHER'S MAXIMUM PER-SAMPLE DELTA — probe AS's bound, reused unchanged.

    v_i ∈ [0,1] because Σv² = 1, the hull trim only attenuates, and outputGain defaults to 0 dB, so
    no smoothed gain can move faster than 1/240 per sample at 48 kHz.
*/
constexpr float kSmootherBound = 1.0f / 240.0f + 1.0e-6f;

/** H9 — every filter-path probe discards this many samples of lead-in.

    A filter starting from s = 0 produces a step at n = 2 LARGER than the signal's steady-state
    maximum slew (0.067264 vs 0.065304 at 1 kHz). That produced a 3% over-reading during research
    before it was traced. Probe AS is unaffected because its excitation is DC; every 2.3 probe that
    puts a sine through the filter is affected.
*/
constexpr int kFilterLeadIn = 2048;

static_assert (kFilterLeadIn >= 2000, "RESEARCH-2.3 H9 requires at least 2000 samples");

//==============================================================================
// ── Phase 3.2 helpers ─────────────────────────────────────────────────────────────────────────

/** Per-lane energy and peak over a render.

    The ping OVERWRITES the eight mapped outputs, so feeding SILENCE means everything measured here
    is the ping and nothing else — which is what lets "seven lanes are EXACT zero" be an assertion
    about the routine rather than about the material.
*/
struct LaneStats
{
    std::array<double, 8> sumSq {};
    std::array<float, 8>  peak {};
    std::array<bool, 8>   anyNonZero {};
    long long             samples { 0 };

    float rmsDb (int lane) const noexcept
    {
        if (samples <= 0)
            return -200.0f;

        const auto rms = std::sqrt (sumSq[static_cast<std::size_t> (lane)]
                                    / static_cast<double> (samples));
        return juce::Decibels::gainToDecibels (static_cast<float> (rms), -200.0f);
    }

    float peakDb (int lane) const noexcept
    {
        return juce::Decibels::gainToDecibels (peak[static_cast<std::size_t> (lane)], -200.0f);
    }

    int loudestLane() const noexcept
    {
        int best = 0;
        for (int ch = 1; ch < 8; ++ch)
            if (sumSq[static_cast<std::size_t> (ch)] > sumSq[static_cast<std::size_t> (best)])
                best = ch;
        return best;
    }

    int soundingLanes() const noexcept
    {
        int n = 0;
        for (int ch = 0; ch < 8; ++ch)
            if (anyNonZero[static_cast<std::size_t> (ch)])
                ++n;
        return n;
    }
};

/** Renders SILENCE in fixed blocks, discarding `warmupSamples` before accumulating.

    The warm-up exists so BR measures the SUSTAIN rather than the 20 ms raised-cosine fade-in: an
    RMS taken across the fade reads low by an amount that depends on the measurement length, which
    would make the assertion a function of the probe rather than of the signal.

    Deliberately allocates its buffer ONCE, outside the loop — the RT-allocation counter is armed
    around a call in probe BQ.
*/
LaneStats renderPingSilence (OOctagonProcessor& proc, int warmupSamples, int measureSamples,
                             int blockSize = kBlockSize)
{
    LaneStats stats {};

    juce::AudioBuffer<float> block (8, blockSize);
    juce::MidiBuffer midi;

    const int total = warmupSamples + measureSamples;

    for (int done = 0; done < total; done += blockSize)
    {
        block.clear();
        proc.processBlock (block, midi);

        if (done < warmupSamples)
            continue;

        for (int ch = 0; ch < 8; ++ch)
            for (int n = 0; n < blockSize; ++n)
            {
                const float v = block.getSample (ch, n);
                const auto  c = static_cast<std::size_t> (ch);

                stats.sumSq[c] += static_cast<double> (v) * static_cast<double> (v);
                stats.peak[c]   = juce::jmax (stats.peak[c], std::abs (v));

                // EXACT zero, via the object representation — a tolerance here would let a
                // -80 dBFS leak into the other seven lanes pass as "silent".
                if (! bitExact (v, 0.0f))
                    stats.anyNonZero[c] = true;
            }

        stats.samples += blockSize;
    }

    return stats;
}

/** Applies a venue edit through applyVenueEdit() — the UNCHECKED path — so a probe can put the
    processor into the state a session restore or a host renegotiation would.

    Deliberately not applyVenueEditChecked: BU needs mapInvalid to actually be raised, and the
    guard exists precisely to stop the editor doing this. */
void forceDuplicateLabel (OOctagonProcessor& proc)
{
    oo::VenueModel v = proc.getVenue();
    v.setSpeakerLabel (5, v.labelAbbreviation (0));
    proc.applyVenueEdit (v);
}

/** A venue with all 50 values distinct and none of them the §OQ4 default. Mirrors the unit
    target's makeMeasuredVenue so BN and BW/BY are talking about the same thing.

    TWO COPIES OF ONE FIXTURE IS pattern_test_fixture_mirrors_drift_silently BY CONSTRUCTION, and
    it is tolerated here only because the two targets cannot share a TU (the unit target links
    without PluginProcessor.cpp, by design — PLAN-2.1 P7/D3). The delay row below was added to BOTH
    at v1.4.0; if a third value ever joins, it goes in both or the two probes stop agreeing about
    what "the measured hall" is while both keep passing. */
oo::VenueModel makeMeasuredVenue()
{
    oo::VenueModel v;

    for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
    {
        const float f = static_cast<float> (i);

        v.setSpeakerPosition (i, { 1.125f + f * 1.375f, 2.250f + f * 2.125f, 3.375f + f * 0.125f });
        v.setSpeakerTrimDb (i, -5.5f + f * 1.25f);
        v.setSpeakerDelayMs (i, 0.375f + f * 1.625f);   // v1.4.0 — matches the unit target exactly
    }

    v.setRake (0.875f, 2.625f);
    v.setName ("Measured hall");
    return v;
}

/** The 50-value bit-compare. Same predicate shape as the unit target's. */
bool sameFifty (const oo::VenueModel& a, const oo::VenueModel& b, int& firstBad)
{
    firstBad = -1;

    for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
    {
        const auto pa = a.speaker (i);
        const auto pb = b.speaker (i);

        if (! (bitExact (pa.x, pb.x) && bitExact (pa.y, pb.y) && bitExact (pa.z, pb.z)
               && bitExact (a.trimDb (i), b.trimDb (i))
               && bitExact (a.delayMs (i), b.delayMs (i))
               && a.labelAbbreviation (i) == b.labelAbbreviation (i)))
        {
            firstBad = i;
            return false;
        }
    }

    return bitExact (a.rakeFront(), b.rakeFront()) && bitExact (a.rakeRear(), b.rakeRear());
}

} // namespace

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("\nO-Octagon render harness — Phases 2.1 (Geometry Core) + 2.2 (DBAP Solve)\n");
    std::printf ("========================================================================\n\n");

    const auto mono = juce::AudioChannelSet::mono();
    const auto set71 = juce::AudioChannelSet::create7point1();

    //==========================================================================
    // Q' — UNITY THROUGH THE ADDRESSED LANE. RE-SPECIFIED AT PHASE 2.2.
    //
    // ── Why the original statement had to change, predicted at PLAN before it failed ──────────
    // Q used to assert unity through ALL 8 outputs, and measured exactly 0.000000000 at Phase 2.1
    // because the placeholder wrote the same mono sum everywhere. Once DBAP is live, lane i carries
    // v_i·in and Σ v_i² = 1, so NO INDIVIDUAL LANE IS AT UNITY. Q's original claim is now false BY
    // DESIGN, not by regression.
    //
    // The replacement is the stronger statement. With w = δ_ij the denominator is w_j²·d_j^(−2a), so
    // k = d_j^a and v_j = 1 ANALYTICALLY, for any position, any rolloff and any blur. Lane
    // speakerToBuffer[j] must therefore reproduce the input exactly, and the other seven must be
    // silent.
    //
    // Asserted at 1e-6 RELATIVE rather than bit-exact: the k = 1/sqrt(t²) round trip is not
    // guaranteed exact in single precision, and a bit-exact gate here would be flaky rather than
    // strict.
    {
        OOctagonProcessor proc;
        const bool negotiated = negotiate (proc, mono, set71);

        // A non-identity map, so the probe is not byte-identical to a hardcoded 0..7 (C1).
        applyRotatedLabels (proc);

        constexpr int liveSpeaker = 3;
        setWeights (proc, deltaWeights (liveSpeaker));

        juce::AudioBuffer<float> out (8, kBlockSize);
        out.clear();

        // Discard the lead-in: setWeights arrives after prepareToPlay's initial solve, so the
        // smoothers need >= 240 samples to reach the new target (H10). `out` holds the LAST block,
        // so the input to compare against starts at `offset`, not at 0.
        const int offset = renderSteady (proc, out, 8 * kBlockSize);

        const int lane = expectedLane (proc, set71, liveSpeaker);

        float worstErr = 0.0f;
        float loudestOther = 0.0f;

        for (int n = 0; n < kBlockSize; ++n)
            worstErr = std::max (worstErr,
                                 std::abs (out.getSample (lane, n) - noiseAt (offset + n)));

        for (int ch = 0; ch < 8; ++ch)
            if (ch != lane)
                loudestOther = std::max (loudestOther, out.getMagnitude (ch, 0, kBlockSize));

        const bool ok = negotiated && worstErr <= 1.0e-6f && loudestOther <= 1.0e-6f
                     && allFinite (out);

        check ("Q' unity-through-addressed-lane", ok,
               juce::String (negotiated ? "7.1 negotiated, " : "LAYOUT REJECTED, ")
                   + "w = delta_" + juce::String (liveSpeaker + 1) + " -> lane "
                   + juce::String (lane) + ", max |out - in| = " + juce::String (worstErr, 9)
                   + ", loudest other lane " + juce::String (loudestOther, 9));
    }

    //==========================================================================
    // R — 1, 2 and 8 output channels, constructed programmatically. No hardware needed.
    //
    // Closes Stage-1 issue 4. JUCE derives the AU channel-config set from
    // isBusesLayoutSupported(), so auval exercises (1,1), (1,2), (1,8), (2,1), (2,2) and (2,8) —
    // the SAFE path is load-bearing for AU, not only for Standalone on a stereo interface.
    {
        struct Case { const char* name; juce::AudioChannelSet out; int channels; bool expectMapped; };

        const std::array<Case, 3> cases
            { { { "mono",   juce::AudioChannelSet::mono(),   1, false },
                { "stereo", juce::AudioChannelSet::stereo(), 2, false },
                { "7.1",    set71,                           8, true  } } };

        bool ok = true;
        juce::String detail;

        for (const auto& c : cases)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, mono, c.out))
            {
                ok = false;
                detail << c.name << ": REJECTED; ";
                continue;
            }

            juce::AudioBuffer<float> buffer (c.channels, kBlockSize);
            juce::MidiBuffer midi;
            buffer.clear();

            for (int n = 0; n < kBlockSize; ++n)
                buffer.setSample (0, n, testSample (n));

            proc.processBlock (buffer, midi);

            // The REAL path is selected iff the buffer is 8 channels AND the map is valid; the
            // mapped case must additionally be a permutation, i.e. every lane written.
            const bool mapValid = ! proc.isChannelMapInvalid();
            bool everyLaneWritten = true;

            for (int ch = 0; ch < c.channels; ++ch)
                if (buffer.getMagnitude (ch, 0, kBlockSize) < 1.0e-6f)
                    everyLaneWritten = false;

            const bool good = allFinite (buffer) && everyLaneWritten && mapValid == c.expectMapped;

            ok = ok && good;
            detail << c.name << "(" << c.channels << "ch) " << (good ? "ok" : "BAD") << "; ";
        }

        check ("R bus-layouts-1-2-8", ok, detail);
    }

    //==========================================================================
    // S — THE F3 HAZARD, DIRECTLY.
    //
    // Standalone on a 3-7 output device: canonicalChannelSet(n) is rejected by the predicate, Debug
    // asserts, and RELEASE KEEPS THE 7.1 LAYOUT while the buffer arrives with n channels. In that
    // state mapInvalid is false and speakerToBuffer holds indices up to 7 — a valid map is NOT
    // evidence of an 8-channel buffer (G1).
    //
    // Both halves are asserted: that the hazardous state is genuinely reproduced (the accessor
    // really does report 8 while the buffer does not), and that nothing writes out of bounds.
    {
        bool ok = true;
        bool reproducedHazard = true;
        juce::String detail;

        for (int n = 3; n <= 7; ++n)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, mono, set71))
            {
                ok = false;
                detail << "7.1 rejected; ";
                break;
            }

            // Two extra canary channels beyond what the AudioBuffer will reference. A per-sample
            // overrun inside a channel, or a stray write through a stale pointer, disturbs them.
            std::vector<std::vector<float>> storage (static_cast<size_t> (n) + 2,
                                                     std::vector<float> (kBlockSize, 0.0f));
            constexpr float canary = -12345.0f;

            for (size_t c = static_cast<size_t> (n); c < storage.size(); ++c)
                std::fill (storage[c].begin(), storage[c].end(), canary);

            std::vector<float*> pointers;

            for (int c = 0; c < n; ++c)
                pointers.push_back (storage[(size_t) c].data());

            for (int s = 0; s < kBlockSize; ++s)
                storage[0][(size_t) s] = testSample (s);

            juce::AudioBuffer<float> buffer (pointers.data(), n, kBlockSize);
            juce::MidiBuffer midi;

            // THE HAZARD, stated as an assertion rather than a comment: the accessor lies here.
            if (! (proc.getTotalNumOutputChannels() == 8 && buffer.getNumChannels() == n))
                reproducedHazard = false;

            proc.processBlock (buffer, midi);

            bool canariesIntact = true;

            for (size_t c = static_cast<size_t> (n); c < storage.size(); ++c)
                for (float v : storage[c])
                    if (! bitExact (v, canary))
                        canariesIntact = false;

            const bool good = allFinite (buffer) && canariesIntact;

            ok = ok && good;

            if (! good)
                detail << n << "ch BAD; ";
        }

        ok = ok && reproducedHazard;

        if (ok)
            detail = "3..7 ch buffers under a 7.1 layout: no OOB, finite, hazard state confirmed "
                     "(getTotalNumOutputChannels()==8 while the buffer is narrower)";
        else if (! reproducedHazard)
            detail << "the F3 state was NOT reproduced — this probe would be vacuous";

        check ("S f3-narrow-buffer-hazard", ok, detail);
    }

    //==========================================================================
    // T — Session round-trip with a NON-DEFAULT venue.
    //
    // Edited coordinates AND a permuted label map, because the identity map round-trips
    // identically whether or not the label layer works at all.
    {
        OOctagonProcessor source;
        negotiate (source, mono, set71);

        oo::VenueModel edited = source.getVenue();

        for (int i = 0; i < 8; ++i)
            edited.setSpeakerPosition (i, { 1.5f + static_cast<float> (i) * 1.375f,
                                            2.25f + static_cast<float> (i) * 2.125f,
                                            3.875f + static_cast<float> (i) * 0.0625f });

        edited.setRake (0.875f, 4.625f);
        edited.setName ("Round-trip fixture");

        for (int i = 0; i < 8; ++i)
            edited.setSpeakerTrimDb (i, -3.5f + static_cast<float> (i) * 0.75f);

        // Rotate the label map by one — a non-identity assignment under create7point1().
        const std::array<const char*, 8> rotated
            { "R", "C", "Lfe", "Lss", "Rss", "Lrs", "Rrs", "L" };

        for (int i = 0; i < 8; ++i)
            edited.setSpeakerLabel (i, rotated[(size_t) i]);

        source.applyVenueEdit (edited);

        juce::MemoryBlock blob;
        source.getStateInformation (blob);

        OOctagonProcessor restored;
        negotiate (restored, mono, set71);
        restored.setStateInformation (blob.getData(), static_cast<int> (blob.getSize()));

        const auto& a = source.getVenue();
        const auto& b = restored.getVenue();

        bool same = bitExact (a.rakeFront(), b.rakeFront()) && bitExact (a.rakeRear(), b.rakeRear());
        int  mismatches = 0;

        for (int i = 0; i < 8; ++i)
        {
            const auto pa = a.speaker (i);
            const auto pb = b.speaker (i);

            const bool speakerSame = bitExact (pa.x, pb.x) && bitExact (pa.y, pb.y)
                                  && bitExact (pa.z, pb.z)
                                  && bitExact (a.trimDb (i), b.trimDb (i))
                                  && a.labelAbbreviation (i) == b.labelAbbreviation (i);

            if (! speakerSame)
                ++mismatches;

            same = same && speakerSame;
        }

        // The restored map must be the rotated one, not the identity — proving the label layer
        // survived the round trip rather than the venue silently falling back to defaults.
        const bool mapValid = ! restored.isChannelMapInvalid();
        const bool nonDefault = ! bitExact (b.speaker (0).x, oo::VenueModel {}.speaker (0).x);

        const bool ok = same && mismatches == 0 && mapValid && nonDefault;

        check ("T venue-session-round-trip", ok,
               juce::String ("50 values, ") + juce::String (mismatches) + " speaker mismatch(es)"
                   + (mapValid ? ", map valid" : ", MAP INVALID")
                   + (nonDefault ? ", venue is non-default" : ", VENUE FELL BACK TO DEFAULTS"));
    }

    //==========================================================================
    // U — A Stage-1-shaped session: the parameters and NO VENUE child at all.
    //
    // "The parameters" and not a count: the blob is manufactured from a CURRENT tree by stripping
    // the venue child, so it carries however many the build has (18 since v1.5.0). What makes it
    // Stage-1-shaped is the missing child, which is the only thing this probe is about.
    //
    // This is not an edge case. Every project saved between Stage 1 and now takes exactly this
    // path, and it must produce the §OQ4 default venue silently, without error.
    {
        OOctagonProcessor source;
        negotiate (source, mono, set71);

        juce::MemoryBlock blob;
        source.getStateInformation (blob);

        // Strip the VENUE child to manufacture a Stage-1-shaped blob from a current one.
        auto xml = juce::AudioProcessor::getXmlFromBinary (blob.getData(),
                                                           static_cast<int> (blob.getSize()));

        bool strippedOne = false;

        if (xml != nullptr)
        {
            if (auto* venueElement = xml->getChildByName (oo::VenueModel::venueTag.toString()))
            {
                xml->removeChildElement (venueElement, true);
                strippedOne = true;
            }
        }

        juce::MemoryBlock stage1Blob;

        if (xml != nullptr)
            juce::AudioProcessor::copyXmlToBinary (*xml, stage1Blob);

        OOctagonProcessor restored;
        negotiate (restored, mono, set71);
        restored.setStateInformation (stage1Blob.getData(), static_cast<int> (stage1Blob.getSize()));

        const oo::VenueModel reference;
        const auto& v = restored.getVenue();

        bool same = near (v.rakeFront(), reference.rakeFront(), 1.0e-6f)
                 && near (v.rakeRear(),  reference.rakeRear(),  1.0e-6f);

        for (int i = 0; i < 8; ++i)
        {
            const auto pa = v.speaker (i);
            const auto pb = reference.speaker (i);

            same = same && bitExact (pa.x, pb.x) && bitExact (pa.y, pb.y) && bitExact (pa.z, pb.z)
                        && v.labelAbbreviation (i) == reference.labelAbbreviation (i);
        }

        const bool ok = strippedOne && same && ! restored.isChannelMapInvalid();

        check ("U stage1-session-defaults", ok,
               juce::String (strippedOne ? "VENUE stripped, " : "NOTHING TO STRIP — probe vacuous, ")
                   + (same ? "restored to §OQ4 defaults" : "DID NOT restore to defaults")
                   + (restored.isChannelMapInvalid() ? ", MAP INVALID" : ", map valid"));
    }

    //==========================================================================
    //
    //  ═══ PHASE 2.2 — DBAP SOLVE AND GAIN APPLICATION ═══════════════════════════════════════
    //
    //==========================================================================

    std::printf ("\n  ── Phase 2.2 (DBAP Solve + Gain Application) ─────────────────────\n");

    //==========================================================================
    // AI — FUNC-01 CRITERION 3: all 8 outputs carry independent, non-duplicated signal.
    //
    // ── The source position is chosen, not arbitrary (RESEARCH-2.2 H4) ───────────────────────
    // The §OQ4 rig is mirror-symmetric about x = 6.5 m, so at the DEFAULT srcX = 0.5 four speaker
    // pairs receive IDENTICAL gains — on correct code. A naive "all 8 differ" probe placed at the
    // default position would fail a correct implementation. srcX = 0.18 / srcY = 0.72 avoids the
    // mirror axis and every speaker coordinate.
    //
    // Driven under a NON-IDENTITY label map, so this also exercises the routing rather than only the
    // arithmetic.
    {
        OOctagonProcessor proc;
        const bool negotiated = negotiate (proc, mono, set71);

        applyRotatedLabels (proc);

        setParam (proc, "srcX", 0.18f);
        setParam (proc, "srcY", 0.72f);

        const auto gains = measureGainVector (proc);

        int    duplicatePairs = 0;
        float  closest = 1.0e9f;

        for (int i = 0; i < 8; ++i)
            for (int j = i + 1; j < 8; ++j)
            {
                const float d = std::abs (gains[(size_t) i] - gains[(size_t) j]);

                closest = std::min (closest, d);

                if (d <= 1.0e-6f)
                    ++duplicatePairs;
            }

        bool allLive = true;

        for (int i = 0; i < 8; ++i)
            if (! (std::isfinite (gains[(size_t) i]) && std::abs (gains[(size_t) i]) > 1.0e-4f))
                allLive = false;

        const bool ok = negotiated && duplicatePairs == 0 && allLive;

        check ("AI eight-independent-lanes", ok,
               juce::String ("off-centre source, non-identity map: ")
                   + juce::String (duplicatePairs) + " duplicate pair(s) of 28, closest lanes differ by "
                   + juce::String (closest, 8)
                   + (allLive ? "" : ", A LANE IS SILENT OR NON-FINITE"));
    }

    //==========================================================================
    // AJ — CHANNEL-MAP LAYER 3. MANDATORY per ROADMAP, and the gate for FUNC-03 criterion 3.
    //
    // Eight renders, w = δ_ij, a unique BIN-CENTRED tone per speaker. N = 4096 at 48 kHz gives a bin
    // spacing of 11.71875 Hz, so k ∈ {64,128,...,512} lands exactly on a bin and "the dominant bin"
    // is an exact statement rather than a three-bin argument.
    //
    // RECTANGULAR WINDOW, deliberately: a Hann window spreads energy into k±1, which would turn the
    // assertion into "roughly there". With a bin-centred tone and no window the leakage is zero.
    //
    // C1 — THE VENUE CARRIES A NON-IDENTITY LABEL MAP. All three accepted 8-channel containers have
    // initializer order == enum-bit order, so with the shipped default labels this probe would be
    // byte-identical to one run against a hardcoded 0..7 map and would prove nothing.
    {
        constexpr int fftOrder = 12;
        constexpr int fftSize  = 1 << fftOrder;          // 4096

        juce::dsp::FFT fft (fftOrder);

        OOctagonProcessor proc;
        const bool negotiated = negotiate (proc, mono, set71);

        applyRotatedLabels (proc);

        bool         ok = negotiated;
        juce::String detail;
        int          correct = 0;

        // Accumulated across all eight renders, then analysed once — the second half of the probe.
        juce::AudioBuffer<float> combined (8, fftSize);
        combined.clear();

        for (int speaker = 0; speaker < 8; ++speaker)
        {
            const int   bin  = 64 * (speaker + 1);       // 64, 128, ... 512
            const float freq = static_cast<float> (bin) * static_cast<float> (kSampleRate)
                             / static_cast<float> (fftSize);

            OOctagonProcessor one;
            negotiate (one, mono, set71);
            applyRotatedLabels (one);
            setWeights (one, deltaWeights (speaker));

            const int lane = expectedLane (one, set71, speaker);

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> captured (8, fftSize);
            captured.clear();

            // Discard >= one control block of start transient before capturing (H10): the weight
            // change lands after prepareToPlay's solve and the ramp is 240 samples.
            {
                juce::AudioBuffer<float> warm (8, 1024);
                warm.clear();
                one.processBlock (warm, midi);
            }

            int rendered = 0;

            while (rendered < fftSize)
            {
                const int chunk = juce::jmin (kBlockSize, fftSize - rendered);

                juce::AudioBuffer<float> block (8, chunk);
                block.clear();

                for (int s = 0; s < chunk; ++s)
                    block.setSample (0, s,
                                     std::sin (juce::MathConstants<float>::twoPi * freq
                                               * static_cast<float> (rendered + s)
                                               / static_cast<float> (kSampleRate)));

                one.processBlock (block, midi);

                for (int ch = 0; ch < 8; ++ch)
                    captured.copyFrom (ch, rendered, block, ch, 0, chunk);

                rendered += chunk;
            }

            // Level: the addressed lane loud, the other seven below a floor. THIS is the assertion
            // that catches a permutation error — a swapped map moves the energy to another lane.
            const float addressed = captured.getMagnitude (lane, 0, fftSize);
            float       loudestOther = 0.0f;

            for (int ch = 0; ch < 8; ++ch)
                if (ch != lane)
                    loudestOther = std::max (loudestOther, captured.getMagnitude (ch, 0, fftSize));

            const bool routed = addressed > 0.9f && loudestOther < 1.0e-5f;

            ok = ok && routed;

            if (! routed)
                detail << "spk" << (speaker + 1) << "->lane" << lane
                       << " addressed " << juce::String (addressed, 4)
                       << " other " << juce::String (loudestOther, 6) << "; ";

            for (int ch = 0; ch < 8; ++ch)
                combined.addFrom (ch, 0, captured, ch, 0, fftSize);
        }

        // One 8-channel buffer carrying all eight tones. Each lane must show ITS OWN speaker's
        // frequency as the dominant bin.
        for (int speaker = 0; speaker < 8; ++speaker)
        {
            const int lane        = expectedLane (proc, set71, speaker);
            const int expectedBin = 64 * (speaker + 1);

            std::vector<float> fftData (static_cast<size_t> (fftSize) * 2, 0.0f);
            std::memcpy (fftData.data(), combined.getReadPointer (lane),
                         sizeof (float) * static_cast<size_t> (fftSize));

            fft.performFrequencyOnlyForwardTransform (fftData.data());

            int   dominant = 0;
            float peak     = 0.0f;

            for (int b = 1; b < fftSize / 2; ++b)
                if (fftData[(size_t) b] > peak)
                {
                    peak     = fftData[(size_t) b];
                    dominant = b;
                }

            if (dominant == expectedBin)
                ++correct;
            else
                detail << "lane" << lane << " dominant bin " << dominant
                       << " != " << expectedBin << "; ";
        }

        ok = ok && correct == 8;

        if (ok)
            detail << "8 renders, non-identity map: each speaker's tone lands ONLY in its own lane, "
                      "and all 8 dominant bins are exact";

        check ("AJ channel-map-layer-3", ok, detail);
    }

    //==========================================================================
    // AK — DSP-04 CRITERION 3, BOTH HALVES (RESEARCH-2.2 H5).
    //
    // The positive half alone is weak: any unrelated recompute triggered by a venue edit would
    // satisfy "the gain vector changed". The negative half is what makes it a real test —
    // earHeight(bbMinY) == rakeFront for ANY rakeRear, so a source at srcY = 0 provably must NOT
    // move. An implementation that recomputed something unrelated fails that half.
    {
        const auto vectorAfterRakeRear = [&] (float srcY, float rakeRear)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);

            oo::VenueModel v = proc.getVenue();
            v.setRake (v.rakeFront(), rakeRear);
            proc.applyVenueEdit (v);

            setParam (proc, "srcY", srcY);
            setParam (proc, "blur", 0.0f);           // maximise positional sensitivity

            return measureGainVector (proc);
        };

        // REAR source: bbMaxY, where earHeight == rakeRear. Must move.
        const auto rearLow  = vectorAfterRakeRear (1.0f, 3.20f);
        const auto rearHigh = vectorAfterRakeRear (1.0f, 7.50f);

        // FRONT source: bbMinY, where earHeight == rakeFront for any rakeRear. Must NOT move.
        const auto frontLow  = vectorAfterRakeRear (0.0f, 3.20f);
        const auto frontHigh = vectorAfterRakeRear (0.0f, 7.50f);

        const bool rearMoved   = vectorsDiffer (rearLow, rearHigh, 1.0e-4f);
        const bool frontStayed = ! vectorsDiffer (frontLow, frontHigh, 1.0e-6f);

        float rearDelta = 0.0f, frontDelta = 0.0f;

        for (int i = 0; i < 8; ++i)
        {
            rearDelta  = std::max (rearDelta,  std::abs (rearLow[(size_t) i]  - rearHigh[(size_t) i]));
            frontDelta = std::max (frontDelta, std::abs (frontLow[(size_t) i] - frontHigh[(size_t) i]));
        }

        const bool ok = rearMoved && frontStayed;

        check ("AK rakerear-moves-gain-vector", ok,
               juce::String ("rear source delta ") + juce::String (rearDelta, 8)
                   + (rearMoved ? " (moved)" : " — DID NOT MOVE")
                   + ", front source delta " + juce::String (frontDelta, 10)
                   + (frontStayed ? " (unchanged, as the plane requires)"
                                  : " — MOVED, so something unrelated is recomputing"));
    }

    //==========================================================================
    // AL — QUAL-03 AS MANDATED. 512 vs 4096, automation at multiples of 4096, memcmp.
    //
    // Literal ROADMAP compliance, and still meaningful: the 512-render sets control targets eight
    // times more often BETWEEN events than the 4096-render does, which is exactly the divergence
    // QUAL-03 exists to catch. AM is the stronger version.
    {
        constexpr int total = 4096 * 6;

        const std::vector<Event> events
            { { 4096 * 1, "srcX", 0.20f }, { 4096 * 2, "srcY", 0.85f },
              { 4096 * 3, "w3",   0.10f }, { 4096 * 4, "rolloff", 5.5f },
              { 4096 * 5, "srcZ", 3.25f } };

        OOctagonProcessor a, b;
        negotiate (a, mono, set71);
        negotiate (b, mono, set71);
        applyRotatedLabels (a);
        applyRotatedLabels (b);

        juce::AudioBuffer<float> outA (8, total), outB (8, total);

        renderInto (a, outA, total, { 512 },  events);
        renderInto (b, outB, total, { 4096 }, events);

        const bool identical = bitIdentical (outA, outB);
        const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("AL blocksize-512-vs-4096", identical && live,
               juce::String (identical ? "bit-identical by memcmp over 24576 samples x 8 lanes"
                                       : firstDifference (outA, outB))
                   + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // AM — QUAL-03's REAL GATE. Ragged block sizes, ARBITRARY event offsets, memcmp.
    //
    // §3.6.3's protocol is only executable with events at multiples of the LARGER block size: a
    // write at absolute sample 1024 cannot be performed between processBlock calls of a fixed-4096
    // render without splitting the call, and splitting it changes the variable under test.
    //
    // A ragged sequence removes that restriction and demonstrates invariance over a far wider space.
    // The grid walk is what makes it hold: the first control boundary at or after absolute sample S
    // is the SAME absolute sample in both renders, even when S is not 64-aligned.
    {
        constexpr int total = 20000;

        // Deliberately not multiples of anything.
        const std::vector<Event> events
            { { 1,     "srcX", 0.31f }, { 337,   "srcY", 0.66f },
              { 1025,  "w5",   0.25f }, { 4097,  "blur", 0.72f },
              { 6151,  "srcZ", -1.40f }, { 11113, "srcX", 0.83f },
              { 15999, "outputGain", -6.5f } };

        OOctagonProcessor a, b;
        negotiate (a, mono, set71);
        negotiate (b, mono, set71);
        applyRotatedLabels (a);
        applyRotatedLabels (b);

        juce::AudioBuffer<float> outA (8, total), outB (8, total);

        renderInto (a, outA, total, { 1, 7, 64, 333, 4096 }, events);
        renderInto (b, outB, total, { 4096 },                events);

        const bool identical = bitIdentical (outA, outB);
        const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("AM ragged-blocksizes", identical && live,
               juce::String (identical ? "1,7,64,333,4096 vs 4096: bit-identical, 7 events at "
                                         "non-aligned offsets"
                                       : firstDifference (outA, outB))
                   + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // AN — QUAL-03 criteria 2 and 3: parameters held constant, several block-size pairs.
    {
        constexpr int total = 12288;

        const std::vector<std::vector<int>> sizeSets
            { { 32 }, { 64 }, { 100 }, { 1024 }, { 2048 }, { 3, 5, 4096 } };

        OOctagonProcessor reference;
        negotiate (reference, mono, set71);
        applyRotatedLabels (reference);
        setParam (reference, "srcX", 0.37f);
        setParam (reference, "srcY", 0.61f);

        juce::AudioBuffer<float> outRef (8, total);
        renderInto (reference, outRef, total, { 4096 }, {});

        bool         ok = true;
        juce::String detail;

        for (const auto& sizes : sizeSets)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);
            setParam (proc, "srcX", 0.37f);
            setParam (proc, "srcY", 0.61f);

            juce::AudioBuffer<float> out (8, total);
            renderInto (proc, out, total, sizes, {});

            if (! bitIdentical (outRef, out))
            {
                ok = false;
                detail << "size " << sizes[0] << ": " << firstDifference (outRef, out) << "; ";
            }
        }

        if (ok)
            detail << "6 block-size regimes vs fixed 4096, parameters constant: all bit-identical";

        check ("AN constant-params-blocksize", ok, detail);
    }

    //==========================================================================
    // AO — PERF-01 CRITERION 1, THE ALLOCATION HALF, MEASURED.
    //
    // ── State the method, not just the verdict (RESEARCH-2.2 H8) ─────────────────────────────
    // -fsanitize=realtime is UNSUPPORTED by Apple clang 17.0.0 (verified by running it). What this
    // probe measures is ALLOCATION, via the replaced global operator new family at the top of this
    // file — every variant, including both align_val_t overloads and every matching delete, because
    // an un-replaced aligned-new is silently uncounted and a probe that counts nothing passes.
    // LOCKS AND FILE I/O REMAIN GREP + INSPECTION, and SUMMARY-2.2 says so.
    //
    // WARM UP FIRST: the first processBlock triggers libc++/JUCE first-touch initialisation that has
    // nothing to do with the audio path, and attributing it here would produce a false failure.
    {
        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.42f);

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> block (8, kBlockSize);

        const auto renderOne = [&] (int n)
        {
            block.clear();

            for (int s = 0; s < kBlockSize; ++s)
                block.setSample (0, s, noiseAt (n * kBlockSize + s));

            proc.processBlock (block, midi);
        };

        renderOne (0);                                   // warm-up, NOT counted

        rtcheck::arm();

        for (int n = 1; n <= 64; ++n)
            renderOne (n);

        // A buffer LARGER than the prepared block size — pluginval at strictness 10 issues these,
        // and it is the case a naive implementation would handle by allocating.
        {
            juce::AudioBuffer<float> big (8, 8192);
            big.clear();
            proc.processBlock (big, midi);
        }

        // A venue edit mid-stream must not make the AUDIO thread allocate either.
        {
            juce::AudioBuffer<float> b2 (8, kBlockSize);
            b2.clear();
            proc.processBlock (b2, midi);
        }

        rtcheck::disarm();

        const long long counted = rtcheck::allocations.load();

        check ("AO no-allocation-in-processblock", counted == 0,
               juce::String (static_cast<int> (counted))
                   + " allocation(s) across 66 processBlock calls incl. an 8192-sample over-size "
                     "block (RTSan unavailable on this toolchain; locks/file I/O are grep)"
                   + rtcheck::foreignNote());
    }

    //==========================================================================
    // AP — PERF-01/3 and PERF-02/1,2 AS NUMBERS.
    //
    // "Hull projection only when outside" and "skipped when unchanged" become instrumentation
    // readings rather than arguments about where a call sits in the source.
    {
        OOctagonProcessor proc;
        negotiate (proc, mono, set71);

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> block (8, kBlockSize);

        const auto renderBlocks = [&] (int count)
        {
            for (int i = 0; i < count; ++i)
            {
                block.clear();
                proc.processBlock (block, midi);
            }
        };

        // 1. INSIDE the hull -> zero projections.
        setParam (proc, "srcX", 0.5f);
        setParam (proc, "srcY", 0.5f);
        renderBlocks (2);

        oo::instr::resetCounters();
        renderBlocks (4);
        const auto insideProjections = oo::instr::get (oo::instr::hullProjections);

        // 2. Nothing changed -> the solve is SKIPPED entirely.
        oo::instr::resetCounters();
        renderBlocks (8);
        const auto solvesWhenIdle = oo::instr::get (oo::instr::solveRuns);
        const auto powWhenIdle    = oo::instr::get (oo::instr::powCalls);

        // 3. A parameter change -> exactly one solve, and 32 pow with it (v1.3.0: 16 for the
        //    two sub-point solves + 16 for their z-cue reference solves in GainStage).
        oo::instr::resetCounters();
        setParam (proc, "srcX", 0.22f);
        renderBlocks (1);
        const auto solvesAfterParam = oo::instr::get (oo::instr::solveRuns);
        const auto powAfterParam    = oo::instr::get (oo::instr::powCalls);

        // 4. OUTSIDE the hull -> projections fire (two per solve, one per sub-point).
        //
        // THE POSITION IS SEARCHED FOR, NOT GUESSED. srcX/srcY are normalised into the SPEAKER
        // BOUNDING BOX, so the puck can never leave it — and the bbox corners are the only region of
        // it that falls outside the hull. Worse, the obvious guess (0,0) denormalises to exactly
        // speaker 1, which is a hull VERTEX: hull::isInside is an inside-OR-ON test, so it reports
        // inside and no projection ever fires. That is correct behaviour and a silently vacuous
        // probe.
        //
        // So: scan for a normalised position the plugin's own hull calls outside, and ASSERT one was
        // found before relying on it.
        float outsideNx = -1.0f, outsideNy = -1.0f;

        {
            const auto& venue = proc.getVenue();
            const auto& theHull = proc.getHull();

            for (int ix = 0; ix <= 20 && outsideNx < 0.0f; ++ix)
                for (int iy = 0; iy <= 20 && outsideNx < 0.0f; ++iy)
                {
                    const float nx = 0.05f * static_cast<float> (ix);
                    const float ny = 0.05f * static_cast<float> (iy);

                    const auto m = venue.normToMetres (nx, ny);

                    if (! theHull.isInside ({ m.x, m.y }))
                    {
                        outsideNx = nx;
                        outsideNy = ny;
                    }
                }
        }

        const bool foundOutside = outsideNx >= 0.0f;

        setParam (proc, "srcX", foundOutside ? outsideNx : 0.5f);
        setParam (proc, "srcY", foundOutside ? outsideNy : 0.5f);
        renderBlocks (2);

        oo::instr::resetCounters();
        setParam (proc, "srcZ", 1.5f);
        renderBlocks (1);
        const auto outsideProjections = oo::instr::get (oo::instr::hullProjections);

        // 5. A VENUE EDIT -> a solve, without any parameter having changed.
        oo::instr::resetCounters();
        {
            oo::VenueModel v = proc.getVenue();
            v.setRake (1.4f, 4.8f);
            proc.applyVenueEdit (v);
        }
        renderBlocks (1);
        const auto solvesAfterVenue = oo::instr::get (oo::instr::solveRuns);

        const bool ok = foundOutside
                     && insideProjections == 0
                     && solvesWhenIdle == 0 && powWhenIdle == 0
                     && solvesAfterParam == 1 && powAfterParam == 32
                     && outsideProjections >= 2
                     && solvesAfterVenue == 1;

        check ("AP solve-and-projection-counts", ok,
               juce::String ("inside proj ") + juce::String ((int) insideProjections)
                   + ", idle solves " + juce::String ((int) solvesWhenIdle)
                   + " (pow " + juce::String ((int) powWhenIdle) + ")"
                   + ", param-change solves " + juce::String ((int) solvesAfterParam)
                   + " (pow " + juce::String ((int) powAfterParam) + ")"
                   + ", outside proj " + juce::String ((int) outsideProjections)
                   + " at norm (" + juce::String (outsideNx, 2) + "," + juce::String (outsideNy, 2) + ")"
                   + ", venue-edit solves " + juce::String ((int) solvesAfterVenue)
                   + (foundOutside ? "" : " — NO OUTSIDE POSITION FOUND, probe would be vacuous"));
    }

    //==========================================================================
    // AQ — THE H1 REGRESSION, MEASURED.
    //
    // The venue generation used to be published as a SEPARATE atomic from the snapshot, so a
    // publish() landing between the audio thread's two acquires handed the control block the NEW
    // geometry with the OLD generation. That generation was stored as lastSolvedGeneration and every
    // subsequent block compared EQUAL — the edit was present in the snapshot and the solve never ran
    // against it, PERMANENTLY.
    //
    // THIS IS INVISIBLE TO ANY PROBE THAT EDITS THE VENUE WHILE AUDIO IS STOPPED. The edit must land
    // BETWEEN two processBlock calls, which is what this does.
    {
        OOctagonProcessor proc;
        negotiate (proc, mono, set71);

        setParam (proc, "srcX", 0.30f);
        setParam (proc, "srcY", 0.65f);
        setParam (proc, "blur", 0.0f);

        const auto before = measureGainVector (proc);

        // The edit, mid-stream. No parameter changes at all, so ONLY the generation can trigger the
        // re-solve.
        {
            oo::VenueModel v = proc.getVenue();

            for (int i = 0; i < 8; ++i)
            {
                const auto p = v.speaker (i);
                v.setSpeakerPosition (i, { p.x, p.y, p.z + 2.5f });
            }

            proc.applyVenueEdit (v);
        }

        const auto after = measureGainVector (proc);

        const bool moved = vectorsDiffer (before, after, 1.0e-4f);

        float delta = 0.0f;

        for (int i = 0; i < 8; ++i)
            delta = std::max (delta, std::abs (before[(size_t) i] - after[(size_t) i]));

        check ("AQ venue-edit-between-blocks", moved,
               juce::String ("gain vector moved by ") + juce::String (delta, 8)
                   + (moved ? " after a mid-stream venue edit"
                            : " — THE DIRTY CHECK IS STALE (H1 has regressed)"));
    }

    //==========================================================================
    // AR — QUAL-02 criterion 4, AND the H2 latch.
    //
    // ── A PARAMETER NaN, not only an input NaN ───────────────────────────────────────────────
    // The input path self-heals: the next block overwrites the buffer. The PARAMETER path does not.
    // jlimit passes NaN straight through, the jassert inside clampTo0To1 is Debug-only, §3.3.4's
    // guard misses it (NaN < kDenomEpsilon is FALSE), and SmoothedValue::setTargetValue(NaN) sets
    // step = NaN — after which currentValue is NaN for the life of the object. ARCHITECTURE §3.5.2's
    // claim that the TPT filter is "the only recursive element" is wrong.
    {
        OOctagonProcessor proc;
        negotiate (proc, mono, set71);

        juce::MidiBuffer midi;

        const auto renderNoise = [&] (int n)
        {
            juce::AudioBuffer<float> b (8, kBlockSize);
            b.clear();

            for (int s = 0; s < kBlockSize; ++s)
                b.setSample (0, s, noiseAt (n * kBlockSize + s));

            proc.processBlock (b, midi);
            return allFinite (b);
        };

        bool         ok = renderNoise (0);
        juce::String detail;

        // 1. NaN written straight into the parameter atomics, bypassing the range clamp exactly as a
        //    misbehaving host would.
        {
            const auto poison = [&] (const char* id)
            {
                proc.getAPVTS().getRawParameterValue (id)
                    ->store (std::numeric_limits<float>::quiet_NaN());
            };

            poison ("srcX");
            poison ("w4");
            poison ("rolloff");
            poison ("outputGain");

            const bool duringNaN = renderNoise (1);

            // And it must RECOVER: a real value afterwards produces correct audio again, which a
            // latched SmoothedValue could not.
            setParam (proc, "srcX", 0.44f);
            setParam (proc, "w4", 0.8f);
            setParam (proc, "rolloff", 4.5f);
            setParam (proc, "outputGain", 0.0f);

            bool recovered = true;

            for (int n = 2; n <= 6; ++n)
                recovered = recovered && renderNoise (n);

            juce::AudioBuffer<float> probe (8, kBlockSize);
            probe.clear();
            renderSteady (proc, probe, 4 * kBlockSize, 1.0f, true);

            bool producesSignal = false;

            for (int ch = 0; ch < 8; ++ch)
                if (probe.getMagnitude (ch, 0, kBlockSize) > 1.0e-3f)
                    producesSignal = true;

            ok = ok && duringNaN && recovered && producesSignal;
            detail << "param-NaN " << (duringNaN ? "finite" : "NON-FINITE")
                   << "/" << (recovered && producesSignal ? "recovered" : "LATCHED") << "; ";
        }

        // 2. Pathological INPUT: silence, DC, full scale, denormals, and NaN itself.
        {
            struct Case { const char* name; float value; };

            const std::array<Case, 5> cases
                { { { "silence", 0.0f }, { "DC", 1.0f }, { "full-scale", 1.0f },
                    { "denormal", 1.0e-40f },
                    { "input-NaN", std::numeric_limits<float>::quiet_NaN() } } };

            for (const auto& c : cases)
            {
                juce::AudioBuffer<float> b (8, kBlockSize);
                b.clear();

                for (int s = 0; s < kBlockSize; ++s)
                    b.setSample (0, s, c.value);

                proc.processBlock (b, midi);

                // The NaN input is EXPECTED to produce NaN this block — the requirement is that it
                // does not STICK.
                if (std::isfinite (c.value) && ! allFinite (b))
                {
                    ok = false;
                    detail << c.name << " NON-FINITE; ";
                }
            }

            // The block after the NaN input must be clean again.
            const bool healed = renderNoise (7);

            ok = ok && healed;
            detail << "input-NaN " << (healed ? "does not stick" : "STUCK");
        }

        check ("AR nan-does-not-latch", ok, detail);
    }

    //==========================================================================
    // AS — QUAL-04 criteria 1 and 2: no zipper noise on a full-speed sweep.
    //
    // ── Measured against a DC input, necessarily ─────────────────────────────────────────────
    // The rendered sample must BE the gain, or the test signal's own per-sample steps swamp the
    // measurement entirely.
    //
    // Bound: |Δout| <= 1/240 + 1e-6. v_i ∈ [0,1] because Σv² = 1, and a 5 ms ramp at 48 kHz is 240
    // steps, so no smoothed gain can move faster than 1/240 per sample.
    //
    // ── AND IT CARRIES A NEGATIVE CONTROL ────────────────────────────────────────────────────
    // A probe never observed to fail is a probe whose failure path is untested. The same sweep is
    // re-measured with the solved vector HELD PER CONTROL BLOCK instead of ramped — reconstructed
    // here by sampling the output only at control boundaries — and that series MUST exceed the
    // bound. If it does not, the sweep is too slow to be a test at all.
    {
        constexpr int total = 48000;                  // 1 second
        constexpr float bound = 1.0f / 240.0f + 1.0e-6f;

        const auto sweep = [&] (bool sweepWeights)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);
            setParam (proc, "blur", 0.0f);

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> out (8, total);
            out.clear();

            int n = 0;

            while (n < total)
            {
                const int chunk = juce::jmin (kBlockSize, total - n);
                const float t = static_cast<float> (n) / static_cast<float> (total);

                if (sweepWeights)
                {
                    // Through EXACT zero on every weight, at different phases.
                    std::array<float, 8> w {};

                    for (int i = 0; i < 8; ++i)
                    {
                        const float phase = t * 4.0f + static_cast<float> (i) * 0.125f;
                        w[(size_t) i] = std::max (0.0f, std::sin (juce::MathConstants<float>::twoPi
                                                                  * phase));
                    }

                    setWeights (proc, w);
                }
                else
                {
                    setParam (proc, "srcX", 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * t * 3.0f));
                    setParam (proc, "srcY", 0.5f + 0.5f * std::cos (juce::MathConstants<float>::twoPi * t * 2.0f));
                    setParam (proc, "srcZ", 3.0f * std::sin (juce::MathConstants<float>::twoPi * t * 5.0f));
                }

                juce::AudioBuffer<float> block (8, chunk);

                // DC input, so the rendered sample IS the gain.
                for (int ch = 0; ch < 8; ++ch)
                    block.clear (ch, 0, chunk);

                for (int s = 0; s < chunk; ++s)
                    block.setSample (0, s, 1.0f);

                proc.processBlock (block, midi);

                for (int ch = 0; ch < 8; ++ch)
                    out.copyFrom (ch, n, block, ch, 0, chunk);

                n += chunk;
            }

            return out;
        };

        const auto worstStep = [] (const juce::AudioBuffer<float>& b, int stride)
        {
            float worst = 0.0f;

            for (int ch = 0; ch < b.getNumChannels(); ++ch)
                for (int n = stride; n < b.getNumSamples(); n += stride)
                    worst = std::max (worst, std::abs (b.getSample (ch, n)
                                                     - b.getSample (ch, n - stride)));

            return worst;
        };

        const auto positionSweep = sweep (false);
        const auto weightSweep   = sweep (true);

        const float positionWorst = worstStep (positionSweep, 1);
        const float weightWorst   = worstStep (weightSweep, 1);

        // NEGATIVE CONTROL: sample the SAME rendered series only at 64-sample control boundaries.
        // That is what the output would look like with the vector held per control block instead of
        // ramped, and it must BREAK the bound — otherwise the sweep is too gentle to prove anything.
        const float positionHeld = worstStep (positionSweep, 64);
        const float weightHeld   = worstStep (weightSweep, 64);

        const bool ramped   = positionWorst <= bound && weightWorst <= bound;
        const bool controlFires = positionHeld > bound && weightHeld > bound;

        const bool ok = ramped && controlFires && allFinite (positionSweep)
                     && allFinite (weightSweep);

        check ("AS no-zipper-on-sweep", ok,
               juce::String ("bound ") + juce::String (bound, 7)
                   + "; per-sample: position " + juce::String (positionWorst, 7)
                   + ", weights " + juce::String (weightWorst, 7)
                   + (ramped ? " (both under)" : " — OVER THE BOUND")
                   + "; negative control (held per control block): "
                   + juce::String (positionHeld, 7) + " / " + juce::String (weightHeld, 7)
                   + (controlFires ? " (both over, so the probe CAN fail)"
                                   : " — UNDER THE BOUND, THIS PROBE CANNOT FAIL"));
    }

    //==========================================================================
    // AT — SAFE MODE AND THE EXACTLY-ONCE INVARIANT, at all four auval configs.
    //
    // sampleAdvances == totalSamples in BOTH modes is the executable form of §3.6.4's "every
    // getNextValue() called exactly once per sample, unconditionally". Without the counter, PERF-02
    // criterion 4 is unmeasurable.
    //
    // The Output knob being INERT in SAFE mode is CONTRACT-MANDATED (§5: the per-sample stage writes
    // "the dry input at unity"). It surprises a reader, so it is asserted rather than left as a
    // comment — if it is to change, it changes at a discuss boundary.
    {
        struct Config { const char* name; juce::AudioChannelSet in; juce::AudioChannelSet out;
                        int channels; bool mapped; };

        const std::array<Config, 5> configs
            { { { "(1,1)", mono, juce::AudioChannelSet::mono(),   1, false },
                { "(1,2)", mono, juce::AudioChannelSet::stereo(), 2, false },
                { "(2,1)", juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono(),   1, false },
                { "(2,2)", juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo(), 2, false },
                { "(1,8)", mono, set71, 8, true } } };

        bool         ok = true;
        juce::String detail;

        for (const auto& c : configs)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, c.in, c.out))
            {
                ok = false;
                detail << c.name << " REJECTED; ";
                continue;
            }

            // v1.11.0: the stereo-bus binaural arm is ON by default and would take the (1,2) and
            // (2,2) configs. This probe is about the DRY SAFE fold, which is what the preference
            // OFF restores; probe DP owns the arm itself, including that OFF is this exact fold.
            proc.setStereoBinauralEnabled (false);

            // A non-default Output gain, so "not applied" is distinguishable from "applied at 0 dB".
            setParam (proc, "outputGain", -12.0f);

            juce::MidiBuffer midi;

            // JUCE sizes the block buffer at max(totalIn, totalOut).
            const int bufferChannels = juce::jmax (c.in.size(), c.channels);

            oo::instr::resetCounters();

            constexpr int blocks = 8;
            int totalRendered = 0;

            juce::AudioBuffer<float> block (bufferChannels, kBlockSize);

            for (int i = 0; i < blocks; ++i)
            {
                block.clear();

                for (int ch = 0; ch < c.in.size(); ++ch)
                    for (int s = 0; s < kBlockSize; ++s)
                        block.setSample (ch, s, 0.25f);

                proc.processBlock (block, midi);
                totalRendered += kBlockSize;
            }

            const auto advances = oo::instr::get (oo::instr::sampleAdvances);
            const auto solves   = oo::instr::get (oo::instr::solveRuns);

            const bool advancedOnce = advances == static_cast<std::uint64_t> (totalRendered);
            const bool solved       = solves >= 1;             // steps 1-7 ran in BOTH modes

            // All 17 smoothers must have ARRIVED after >= 240 still samples. A desynchronised
            // smoother has not.
            const auto smoothed = proc.currentSmoothedGains();
            bool allFiniteGains = true;

            for (float g : smoothed)
                if (! std::isfinite (g))
                    allFiniteGains = false;

            bool good = advancedOnce && solved && allFiniteGains && allFinite (block);

            if (! c.mapped)
            {
                // SAFE mode: dry input at unity, outputGain NOT applied. -12 dB would be 0.0629.
                const float written = block.getSample (0, kBlockSize - 1);
                const bool unity = std::abs (written - 0.25f) <= 1.0e-6f;

                good = good && unity;

                if (! unity)
                    detail << c.name << " wrote " << juce::String (written, 6)
                           << " (expected the dry 0.25 with outputGain INERT); ";
            }
            else
            {
                // REAL mode: outGain IS applied, so the lanes must be quieter than the dry input.
                const float peak = block.getMagnitude (0, 0, kBlockSize);
                const bool attenuated = peak < 0.25f;

                good = good && attenuated;

                if (! attenuated)
                    detail << c.name << " outputGain not applied in REAL mode; ";
            }

            ok = ok && good;

            if (! advancedOnce)
                detail << c.name << " advances " << juce::String ((int) advances) << " != "
                       << juce::String (totalRendered) << "; ";
        }

        if (ok)
            detail << "5 configs incl. all four auval pairs: sampleAdvances == totalSamples in both "
                      "modes, steps 1-7 run in both, Output inert in SAFE as §5 mandates";

        check ("AT safe-mode-and-exactly-once", ok, detail);
    }

    //==========================================================================
    // AY — DSP-06 CRITERION 1, THE REGRESSION GATE.
    //
    // width's DEFAULT IS 0.0, so this is the default patch: it is the criterion most likely to
    // break when width goes live and the least likely to be noticed, because every other probe
    // would still pass. At width = 0 the two sub-points coincide, both solves run over identical
    // inputs, and v_L must be BIT-FOR-BIT v_R — with NO branch eliding the second solve (§3.4.3).
    {
        bool         ok = true;
        juce::String detail;

        // 1. width = 0 -> the eight pairs are bit-identical.
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            setParam (proc, "srcX", 0.30f);          // off-centre, so nothing is masked by symmetry
            setParam (proc, "srcY", 0.65f);
            setParam (proc, "width", 0.0f);

            juce::AudioBuffer<float> settle (8, kBlockSize);
            renderSteady (proc, settle, 4 * kBlockSize, 1.0f, true);

            const auto g = proc.currentSmoothedGains();

            int identical = 0;

            for (int i = 0; i < 8; ++i)
                if (bitExact (g[(size_t) i], g[(size_t) (i + 8)]))
                    ++identical;

            ok = ok && identical == 8;
            detail << "width=0: " << juce::String (identical) << "/8 gL==gR bitwise"
                   << (identical == 8 ? "" : " — DSP-06/1 HAS REGRESSED") << "; ";
        }

        // 2. NEGATIVE CONTROL, in the same probe. An off-centre puck at width = 4 must SEPARATE the
        //    pairs — otherwise the bit-identity above is being produced by width never reaching the
        //    shaper at all, which is precisely the pre-2.3 state.
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            setParam (proc, "srcX", 0.30f);
            setParam (proc, "srcY", 0.65f);
            setParam (proc, "width", 4.0f);

            juce::AudioBuffer<float> settle (8, kBlockSize);
            renderSteady (proc, settle, 4 * kBlockSize, 1.0f, true);

            const auto g = proc.currentSmoothedGains();

            int separated = 0;
            float worst = 0.0f;

            for (int i = 0; i < 8; ++i)
            {
                const float d = std::abs (g[(size_t) i] - g[(size_t) (i + 8)]);

                worst = std::max (worst, d);

                if (d > 1.0e-5f)
                    ++separated;
            }

            ok = ok && separated >= 6;
            detail << "width=4: " << juce::String (separated) << "/8 pairs separated, worst "
                   << juce::String (worst, 7)
                   << (separated >= 6 ? " (so the probe CAN fail)"
                                      : " — WIDTH IS NOT REACHING THE SHAPER");
        }

        check ("AY width-zero-is-bit-identical", ok, detail);
    }

    //==========================================================================
    // AZ — QUAL-04 CRITERION 3 (which was PARTIAL at the 2.2 boundary) AND QUAL-01/1.
    //
    // Probe AS's proven DC construction, applied to the five parameters AS does not cover. With
    // AS's eleven (srcX, srcY, srcZ and the eight weights) and BC's airAmount, SEVENTEEN OF THE
    // EIGHTEEN ARE COVERED: 11 + 5 + 1 = 17.
    //
    // ── AND THE EIGHTEENTH IS DELIBERATELY NOT HERE (v1.5.0) ──────────────────────────────────
    //
    // `decorr` CANNOT be swept by this construction, and adding it would be worse than leaving it
    // out. Every section of the decorrelation network is an all-pass with H(1) = (1-g)/(1-g) = 1
    // EXACTLY, so a DC input passes through unchanged at every depth — probe CV measures the
    // residual at 3.0e-7 across a full sweep. A decorr sweep on AS's DC signal would therefore
    // move NOTHING, report a per-sample delta of zero, and pass its bound by a mile while testing
    // the parameter not at all (pattern_zipper_sweep_probe_needs_liveness_gate). It would read as
    // coverage and be decoration.
    //
    // The zipper risk it would be pretending to cover is closed structurally and measured
    // elsewhere: decorrMix and decorrDepth are 5 ms SmoothedValues advanced once per sample
    // unconditionally, the ring reads are fractional and interpolated, and probe CX sweeps decorr
    // across block boundaries at five block sizes and requires bit-identity.
    //
    // Two placement requirements, or the sweep is vacuous rather than clean:
    //   - the puck sits OFF-CENTRE, so width is outside the rFade collapse and actually spreads;
    //   - the hullAtten sweep runs OUTSIDE the hull, where d_hull > 0. Inside, hullAtten is a
    //     provable no-op (AV) and its sweep would move nothing at all.
    {
        // FULL RANGE MEANS THE DECLARED RANGE, READ FROM THE PARAMETER OBJECT. Transcribing the
        // endpoints here would be pattern_test_fixture_mirrors_drift_silently, and getting them
        // wrong is not hypothetical: a first draft of this probe swept rolloff to 9.0 against a
        // declared 3.0–6.0 and outputGain to +6 against a declared +12, so setValueNotifyingHost
        // clamped and the sweep silently covered less than the criterion claims.
        struct Sweep { const char* id; bool outside; };

        const std::array<Sweep, 5> sweeps
            { { { "width",      false },
                { "rolloff",    false },
                { "blur",       false },
                { "hullAtten",  true  },
                { "outputGain", false } } };

        constexpr int total = 24000;                  // 0.5 s per parameter

        bool         ok = true;
        juce::String detail;

        for (const auto& s : sweeps)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);

            float onx = 0.0f, ony = 0.0f;
            const bool found = findOutside (proc, onx, ony);

            if (s.outside && ! found)
            {
                ok = false;
                detail << s.id << " NO OUTSIDE POSITION; ";
                continue;
            }

            // Off-centre in both cases; outside the hull when the parameter needs it.
            setParam (proc, "srcX", s.outside ? onx : 0.30f);
            setParam (proc, "srcY", s.outside ? ony : 0.65f);

            const auto& range = proc.getAPVTS().getParameterRange (s.id);
            const float lo = range.start;
            const float hi = range.end;

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> out (8, total);
            out.clear();

            int n = 0;

            while (n < total)
            {
                const int chunk = juce::jmin (kBlockSize, total - n);
                const float t   = static_cast<float> (n) / static_cast<float> (total);

                // Full range and back, so both directions are exercised.
                const float tri = t < 0.5f ? 2.0f * t : 2.0f * (1.0f - t);

                setParam (proc, s.id, lo + tri * (hi - lo));

                juce::AudioBuffer<float> block (8, chunk);

                for (int ch = 0; ch < 8; ++ch)
                    block.clear (ch, 0, chunk);

                for (int k = 0; k < chunk; ++k)      // DC, so the rendered sample IS the gain
                    block.setSample (0, k, 1.0f);

                proc.processBlock (block, midi);

                for (int ch = 0; ch < 8; ++ch)
                    out.copyFrom (ch, n, block, ch, 0, chunk);

                n += chunk;
            }

            // outputGain multiplies the whole vector, so its bound is the OUTPUT range's ramp rate
            // rather than 1/240 — DERIVED from the swept range, not tuned.
            float bound = kSmootherBound;

            if (std::strcmp (s.id, "outputGain") == 0)
                bound = (juce::Decibels::decibelsToGain (hi)
                         - juce::Decibels::decibelsToGain (lo)) / 240.0f + 1.0e-6f;

            float perSample = 0.0f, held = 0.0f;

            for (int ch = 0; ch < 8; ++ch)
            {
                perSample = std::max (perSample, worstStepIn (out, ch, 1, total));

                // NEGATIVE CONTROL: the same series sampled only at 64-sample control boundaries —
                // what the output would look like held per control block instead of ramped. It must
                // BREAK the bound, or the sweep is too gentle to prove anything.
                for (int k = 64; k < total; k += 64)
                    held = std::max (held, std::abs (out.getSample (ch, k)
                                                   - out.getSample (ch, k - 64)));
            }

            const bool ramped  = perSample <= bound;
            const bool control = held > bound;

            ok = ok && ramped && control && allFinite (out);

            detail << s.id << " [" << juce::String (lo, 1) << ".." << juce::String (hi, 1) << "] "
                   << juce::String (perSample, 7) << "/" << juce::String (bound, 7)
                   << (ramped ? "" : " OVER")
                   << " (held " << juce::String (held, 5)
                   << (control ? ")" : " — CONTROL DOES NOT FIRE)") << "; ";
        }

        if (ok)
            detail << "all 5 ramped, all 5 controls fire; with AS's 11 and BC's airAmount that is "
                      "11 + 5 + 1 = 17 of 18 (decorr is CV/CW/CX's — see the note above)";

        check ("AZ no-zipper-on-remaining", ok, detail);
    }

    //==========================================================================
    // BA — DSP-06/4 AND RISK R5: the gate on §3.4.2's design-time rFade fix.
    //
    // Without the collapse, a puck sweeping through the centroid flips b̂ by 180°, n̂ with it, and
    // L and R swap sides INSTANTANEOUSLY — a 6 m jump of both sub-points in one control block at
    // width = 6. Probe AX asserts the collapse arithmetically; this asserts that the RENDER stays
    // continuous through it, which is what the requirement actually says.
    {
        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "width", 6.0f);
        setParam (proc, "blur", 0.0f);

        // The centroid in normalised coordinates — swept THROUGH, not merely approached.
        const auto& v = proc.getVenue();
        const float cx = (v.centroid().x - v.bbMinX()) / (v.bbMaxX() - v.bbMinX());
        const float cy = (v.centroid().y - v.bbMinY()) / (v.bbMaxY() - v.bbMinY());

        setParam (proc, "srcY", cy);

        constexpr int total = 24000;

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> out (8, total);
        out.clear();

        int n = 0;

        while (n < total)
        {
            const int chunk = juce::jmin (kBlockSize, total - n);
            const float t   = static_cast<float> (n) / static_cast<float> (total);

            // 0.30 -> 0.70 in normalised x, passing exactly through cx partway.
            setParam (proc, "srcX", juce::jlimit (0.0f, 1.0f, cx - 0.2f + 0.4f * t));

            juce::AudioBuffer<float> block (8, chunk);

            for (int ch = 0; ch < 8; ++ch)
                block.clear (ch, 0, chunk);

            for (int k = 0; k < chunk; ++k)
                block.setSample (0, k, 1.0f);        // DC

            proc.processBlock (block, midi);

            for (int ch = 0; ch < 8; ++ch)
                out.copyFrom (ch, n, block, ch, 0, chunk);

            n += chunk;
        }

        float worst = 0.0f, held = 0.0f;

        for (int ch = 0; ch < 8; ++ch)
        {
            worst = std::max (worst, worstStepIn (out, ch, 1, total));

            for (int k = 64; k < total; k += 64)
                held = std::max (held, std::abs (out.getSample (ch, k) - out.getSample (ch, k - 64)));
        }

        const bool smooth  = worst <= kSmootherBound;
        const bool control = held > kSmootherBound;
        const bool ok = smooth && control && allFinite (out);

        check ("BA centroid-crossing-at-width-6", ok,
               juce::String ("worst per-sample step ") + juce::String (worst, 7)
                   + " vs bound " + juce::String (kSmootherBound, 7)
                   + (smooth ? " (continuous through the centroid)"
                             : " — DISCONTINUITY, THE rFade COLLAPSE IS NOT WORKING")
                   + "; negative control " + juce::String (held, 5)
                   + (control ? " (fires)" : " — DOES NOT FIRE, sweep too gentle"));
    }

    //==========================================================================
    // BB — QUAL-01 CRITERION 2, THE HULL CROSSING. P27's entry seed and H3's asymmetric exit.
    //
    // ── THE MEASUREMENT ISOLATES THE FILTER BY DIFFERENCING TWO RENDERS ──────────────────────
    // A single render's step at the crossing mixes the filter switch with the gain vector's 5 ms
    // ramp. But airAmount does not enter the DBAP solve at all, so a render at airAmount = 0.35 and
    // one at airAmount = 0 have BIT-IDENTICAL gain trajectories. Their difference D = A − B is
    // therefore exactly the filter's contribution, gain-weighted, with the sine and the ramp
    // removed by construction rather than by tolerance.
    //
    // ── AND BOTH EXCITATIONS, ALWAYS (H2/D3) ─────────────────────────────────────────────────
    // "0 dB at DC" remains true — the phase lag also vanishes at DC — so a DC hull-crossing probe
    // is EXACTLY BLIND to the one discontinuity the D2 amendment introduced. DC measures the gain
    // vector; sine measures the filter. 8 kHz is included because that is where the cost is large
    // enough to matter musically, and it is the tone D5's listening check should use.
    {
        constexpr int total      = 24576;
        constexpr int entryAt    = 8192;              // both multiples of 64, so the parameter
        constexpr int exitAt     = 16384;             // takes effect at EXACTLY these samples

        static_assert (entryAt % 64 == 0 && exitAt % 64 == 0,
                       "the crossing must land on a control boundary or its sample index is only "
                       "known to within 63 samples and the edge assertions read the wrong sample");
        static_assert (entryAt >= kFilterLeadIn, "H9: discard the filter's cold start");

        bool         ok = true;
        juce::String detail;

        // Locate a crossing NEAR the boundary: 0.25 m outside. That is the realistic gesture, and
        // it is the case the |H − 1| bound is derived for (d_hull small -> fc near the ceiling).
        OOctagonProcessor scout;
        negotiate (scout, mono, set71);

        float onx = 0.0f, ony = 0.0f;
        const bool found = findOutside (scout, onx, ony);

        const auto& sv = scout.getVenue();
        const float inNx = (sv.centroid().x - sv.bbMinX()) / (sv.bbMaxX() - sv.bbMinX());
        const float inNy = (sv.centroid().y - sv.bbMinY()) / (sv.bbMaxY() - sv.bbMinY());

        const auto outN = found ? normAtHullDistance (scout, inNx, inNy, onx, ony, 0.25f)
                                : std::pair<float, float> { inNx, inNy };

        const float dHullOut = dHullAtNorm (scout, outN.first, outN.second);
        const float fcOut    = oo::hullproc::airCutoffHz (0.35f, dHullOut, kSampleRate);

        if (! found || dHullOut <= 0.0f)
        {
            check ("BB hull-crossing-continuity", false,
                   "NO OUTSIDE POSITION FOUND — the probe would be vacuous");
        }
        else
        {
            const std::vector<Event> crossing
                { { entryAt, "srcX", outN.first },  { entryAt, "srcY", outN.second },
                  { exitAt,  "srcX", inNx },        { exitAt,  "srcY", inNy } };

            const auto renderCrossing = [&] (float airAmount, float freqHz,
                                             juce::AudioBuffer<float>& dest)
            {
                OOctagonProcessor proc;
                negotiate (proc, mono, set71);
                setParam (proc, "srcX", inNx);
                setParam (proc, "srcY", inNy);
                setParam (proc, "airAmount", airAmount);
                setParam (proc, "blur", 0.0f);

                if (freqHz > 0.0f)
                    renderGenInto (proc, dest, total, { kBlockSize }, crossing,
                                   Sine { freqHz, 0.5f });
                else
                    renderGenInto (proc, dest, total, { kBlockSize }, crossing,
                                   [] (int) { return 1.0f; });
            };

            // ── 1. DC — the GAIN VECTOR's continuity across the crossing ──────────────────────
            {
                juce::AudioBuffer<float> dc (8, total);
                renderCrossing (0.35f, 0.0f, dc);

                float worst = 0.0f;

                for (int ch = 0; ch < 8; ++ch)
                {
                    worst = std::max (worst, worstStepIn (dc, ch, entryAt - 256, entryAt + 512));
                    worst = std::max (worst, worstStepIn (dc, ch, exitAt  - 256, exitAt  + 512));
                }

                const bool smooth = worst <= kSmootherBound && allFinite (dc);

                ok = ok && smooth;
                detail << "DC step " << juce::String (worst, 7) << "/"
                       << juce::String (kSmootherBound, 7) << (smooth ? "; " : " OVER; ");
            }

            // ── 2. SINE at 1 kHz and 8 kHz — the FILTER's step ────────────────────────────────
            const std::array<float, 2> tones { { 1000.0f, 8000.0f } };

            for (float f : tones)
            {
                juce::AudioBuffer<float> wet (8, total), dry (8, total);

                renderCrossing (0.35f, f, wet);
                renderCrossing (0.0f,  f, dry);

                // The predicted bound, computed from the transfer function at the ACTUAL cutoff the
                // filter ran at, and the ACTUAL per-lane amplitude of the dry render. Nothing here
                // is a tolerance: |D| = |gain·(filtered − dry)| <= peak_dry · |H(f) − 1|.
                const double hMinus1 = magHMinusOne (fcOut, kSampleRate, f);

                // An INTEGER number of cycles at both tones — 2016 = 48 x 42 = 6 x 336 — so the
                // RMS below is exact rather than rippling by a partial cycle.
                constexpr int measureWindow = 2016;

                static_assert (measureWindow % 48 == 0 && measureWindow % 6 == 0,
                               "the window must span whole cycles of 1 kHz (48 samples) and 8 kHz "
                               "(6 samples) at 48 kHz, or the RMS amplitude ripples");
                static_assert (exitAt - measureWindow > entryAt,
                               "the measurement window must lie entirely inside the outside-hull "
                               "excursion, after the gains have settled");

                bool  entryExact = true;
                bool  filterRuns = false;
                bool  exitBounded = true;
                float worstExit = 0.0f;
                float worstBound = 0.0f;
                float worstRatio = 0.0f;

                for (int ch = 0; ch < 8; ++ch)
                {
                    // ENTRY — P27 makes this BIT-EXACT: the filter is seeded with the incoming
                    // sample, so v = G*(x − s) = 0.0f and y == x. This is the assertion that fails
                    // loudly if the seed is ever dropped; a resident-state entry is ~20x the exit
                    // step at 1 kHz.
                    const float dEntry = wet.getSample (ch, entryAt) - dry.getSample (ch, entryAt);

                    entryExact = entryExact && bitExact (dEntry, 0.0f);

                    // NON-VACUITY: the filter must actually be running just after, or "entry is
                    // bit-exact" is only saying the stage never engaged.
                    for (int n = entryAt + 1; n < entryAt + 64; ++n)
                        if (std::abs (wet.getSample (ch, n) - dry.getSample (ch, n)) > 1.0e-7f)
                            filterRuns = true;

                    // EXIT — inherent to D2 and NOT removable: there is no state on the dry path to
                    // seed (H3).
                    //
                    // MEASURED AS AN AMPLITUDE, NOT AS THE STEP AT THIS PARTICULAR SAMPLE. The
                    // realised step depends on where in the sine's cycle the crossing happens, so a
                    // single-sample reading can land near a zero crossing and report a tiny number
                    // for a large discontinuity — which is how this probe first "passed" at 1 kHz
                    // with a ratio of 0.365. The amplitude of D over the settled window is the
                    // worst step an arbitrarily-timed crossing could produce, and it is the number
                    // D5 listens to.
                    //
                    // AND IT IS ASSERTED AS AN EQUALITY, NOT A CEILING (P34). D is exactly the
                    // signal (H − 1)·x scaled by the lane's gain, so its amplitude must MATCH
                    // ampDry·|H(f) − 1| — which makes this a statement that the filter really is
                    // the specified one-pole at the specified cutoff, and makes it fail loudly if
                    // either ever changes.
                    const float ampDry = sineAmplitude (dry, ch, exitAt - measureWindow, exitAt);
                    const float ampD   = sineAmplitudeOfDiff (wet, dry, ch,
                                                              exitAt - measureWindow, exitAt);

                    const float predicted = ampDry * static_cast<float> (hMinus1);
                    const float ratio = predicted > 1.0e-9f ? ampD / predicted : 0.0f;

                    worstExit  = std::max (worstExit,  ampD);
                    worstBound = std::max (worstBound, predicted);
                    worstRatio = std::max (worstRatio, std::abs (ratio - 1.0f));

                    // 1e-3 is a float-precision allowance, not a fitted tolerance: over whole
                    // cycles the two RMS figures are analytically equal.
                    exitBounded = exitBounded && std::abs (ratio - 1.0f) <= 1.0e-3f;
                }

                ok = ok && entryExact && filterRuns && exitBounded;

                detail << juce::String (static_cast<int> (f)) << " Hz: entry "
                       << (entryExact ? "BIT-EXACT" : "NOT BIT-EXACT — P27's SEED IS GONE")
                       << (filterRuns ? "" : " (VACUOUS — filter never ran)")
                       << ", exit amp " << juce::String (worstExit, 6) << " vs predicted "
                       << juce::String (worstBound, 6) << " (err "
                       << juce::String (worstRatio * 100.0f, 3) << "%)"
                       << (exitBounded ? "; " : " — DOES NOT MATCH |H−1|; ");
            }

            detail << "d_hull " << juce::String (dHullOut, 3) << " m, fc "
                   << juce::String (fcOut, 0) << " Hz";

            // Q9 — sub-points STRADDLING the boundary are NOT probed empirically, and that is a
            // decision rather than an omission. Each feed enters at 0.5 (§3.4.3's level
            // convention), so a straddling crossing is at worst HALF a coincident one; and the wEff
            // collapse region is a disc interior to the hull, so on a non-degenerate rig the
            // sub-points cannot be separated AND at the boundary at the same time. The argument
            // bounds the case more tightly than a probe would.

            check ("BB hull-crossing-continuity", ok, detail);
        }
    }

    //==========================================================================
    // BC — QUAL-01/1 FOR airAmount, THE SEVENTEENTH PARAMETER. P34.
    //
    // ── THE METHOD REQUIREMENTS.md ORIGINALLY SPECIFIED IS VACUOUS, AND WAS REPLACED AT PLAN ──
    // "max |out[n] − out[n−1]| swept vs held" measures +0.00000% excess at 1 kHz — identically
    // zero to nine decimals — because the render's maximum slew is set by the sine's own zero
    // crossing, which occurs early, where the swept and held renders still share a cutoff. It
    // would have passed for a reason unrelated to the code. (RESEARCH-2.3 Q6.)
    //
    // ── WHAT IS MEASURED INSTEAD ──────────────────────────────────────────────────────────────
    // Q3 gives the perturbation in closed form: Δy = (G_new − G_old)(x − s), so a cutoff staircase
    // of step ΔG perturbs the output by at most ΔG · 2 · peak. Render the SAME sweep with the
    // cutoff quantised to the 64-sample control grid and to a 4096-sample grid, and difference
    // them: D = A − B is the perturbation, with the sine removed by construction.
    //
    //   UPPER BOUND   max|D| <= maxΔG(4096) · 2 · peak   — the analytic model bounds what is
    //                                                      actually observed, at both tones.
    //   SEPARATION    max|D| >  maxΔG(64)   · 2 · peak   — the coarse grid diverges by far more
    //                                                      than any 64-grid update could, so the
    //                                                      measurement demonstrably CAN see a
    //                                                      regression that coarsened the grid.
    //
    // Both bounds are computed IN-PROBE from the sweep schedule the probe itself drives. Nothing is
    // tuned and nothing mirrors a constant (pattern_test_fixture_mirrors_drift_silently).
    //
    // The separation is asserted at 8 kHz and only REPORTED at 1 kHz: per H2 the coefficient effect
    // at 1 kHz is 114x smaller relative to the signal, so the margin there is thin enough that
    // asserting it would make the probe fragile rather than strict.
    {
        constexpr int total = 48000 + kFilterLeadIn;   // 1 s of sweep after the H9 discard
        constexpr float peakAmp = 0.5f;

        OOctagonProcessor scout;
        negotiate (scout, mono, set71);

        float onx = 0.0f, ony = 0.0f;
        const bool found = findOutside (scout, onx, ony);
        const float dHull = found ? dHullAtNorm (scout, onx, ony) : 0.0f;

        if (! found || dHull <= 0.0f)
        {
            check ("BC air-sweep-differential", false,
                   "NO OUTSIDE POSITION FOUND — airAmount would do nothing and the probe would be "
                   "vacuous");
        }
        else
        {
            // The sweep schedule, as a function of the absolute sample. 0.02 -> 1.0 over one
            // second, starting after the lead-in.
            const auto airAt = [&] (int n)
            {
                const float t = juce::jlimit (0.0f, 1.0f,
                                              static_cast<float> (n - kFilterLeadIn) / 48000.0f);
                return 0.02f + 0.98f * t;
            };

            const auto eventsOnGrid = [&] (int grid)
            {
                std::vector<Event> e;

                for (int n = 0; n < total; n += grid)
                    e.push_back ({ n, "airAmount", airAt (n) });

                return e;
            };

            // maxΔG for a grid, from the SAME schedule and the SAME cutoff law the plugin uses.
            const auto maxDeltaG = [&] (int grid)
            {
                double worst = 0.0, previous = -1.0;

                for (int n = 0; n < total; n += grid)
                {
                    const double G = tptG (oo::hullproc::airCutoffHz (airAt (n), dHull, kSampleRate),
                                           kSampleRate);

                    if (previous >= 0.0)
                        worst = std::max (worst, std::abs (G - previous));

                    previous = G;
                }

                return worst;
            };

            const double dG64   = maxDeltaG (64);
            const double dG4096 = maxDeltaG (4096);

            const auto fine   = eventsOnGrid (64);
            const auto coarse = eventsOnGrid (4096);

            bool         ok = true;
            juce::String detail;

            const std::array<float, 2> tones { { 1000.0f, 8000.0f } };

            for (float f : tones)
            {
                const auto renderGrid = [&] (const std::vector<Event>& ev,
                                             juce::AudioBuffer<float>& dest)
                {
                    OOctagonProcessor proc;
                    negotiate (proc, mono, set71);
                    setParam (proc, "srcX", onx);
                    setParam (proc, "srcY", ony);
                    setParam (proc, "blur", 0.0f);

                    renderGenInto (proc, dest, total, { kBlockSize }, ev, Sine { f, peakAmp });
                };

                juce::AudioBuffer<float> a (8, total), b (8, total);

                renderGrid (fine,   a);
                renderGrid (coarse, b);

                float worstD = 0.0f, peakLane = 0.0f;

                for (int ch = 0; ch < 8; ++ch)
                    for (int n = kFilterLeadIn; n < total; ++n)
                    {
                        worstD   = std::max (worstD, std::abs (a.getSample (ch, n)
                                                             - b.getSample (ch, n)));
                        peakLane = std::max (peakLane, std::abs (b.getSample (ch, n)));
                    }

                const float upper = static_cast<float> (dG4096) * 2.0f * peakLane;
                const float lower = static_cast<float> (dG64)   * 2.0f * peakLane;

                const bool bounded   = worstD <= upper;
                const bool separated = worstD > lower;

                ok = ok && bounded && allFinite (a) && allFinite (b);

                if (f > 4000.0f)
                    ok = ok && separated;

                detail << juce::String (static_cast<int> (f)) << " Hz: |D| "
                       << juce::String (worstD, 6) << " <= " << juce::String (upper, 6)
                       << (bounded ? "" : " OVER")
                       << ", vs 64-grid bound " << juce::String (lower, 6) << " ("
                       << juce::String (worstD / juce::jmax (lower, 1.0e-12f), 1) << "x"
                       << (separated ? "" : ", NOT SEPARATED")
                       << (f > 4000.0f ? ", asserted" : ", reported only") << "); ";
            }

            detail << "maxΔG 64-grid " << juce::String (dG64, 7)
                   << " vs 4096-grid " << juce::String (dG4096, 7)
                   << " (" << juce::String (dG4096 / juce::jmax (dG64, 1.0e-12), 1) << "x); "
                   << "the 64-grid per-update step is therefore bounded by "
                   << juce::String (dG64 * 2.0 * static_cast<double> (peakAmp), 7)
                   << " at the source feed";

            check ("BC air-sweep-differential", ok, detail);
        }
    }

    //==========================================================================
    // BD — DSP-07 CRITERIA 2, 5, 6 AND 7, BY P33's STRUCTURAL METHOD.
    //
    // THERE IS NO "FILTER ABSENT" BUILD AND NO "TRIM ABSENT" BUILD, and fabricating one would mean
    // a second arithmetic path selected by a compile flag — the class of thing §3.4.3 forbids, and
    // a path the shipping binary would never take. So bit-transparency is proven where it lives:
    // the branch is COUNTED as never taken, and the multiply is proven to be by exactly 1.0f
    // (probe AV). Each half carries a non-vacuity control showing the stage DOES change the render
    // when enabled. SUMMARY-2.3 states this as a method, not as "bit-transparency ✓".
    //
    // ── ONE STATED PRECONDITION, NOT A LOOPHOLE (RESEARCH-2.3 Q5) ────────────────────────────
    // processBlock runs under juce::ScopedNoDenormals, so a DENORMAL v_i (< 1.18e-38) would flush
    // to zero under v_i * 1.0f where 2.2 stored the denormal. Reaching one requires a weight around
    // 1e-38 — unreachable from the exposed 0–1 range with any non-degenerate geometry — and this
    // probe uses default weights. An unstated precondition is how a bit-identity claim quietly
    // becomes false.
    {
        constexpr int total = 8192;

        bool         ok = true;
        juce::String detail;

        OOctagonProcessor scout;
        negotiate (scout, mono, set71);

        float onx = 0.0f, ony = 0.0f;
        const bool found = findOutside (scout, onx, ony);

        const auto& sv = scout.getVenue();
        const float inNx = (sv.centroid().x - sv.bbMinX()) / (sv.bbMaxX() - sv.bbMinX());
        const float inNy = (sv.centroid().y - sv.bbMinY()) / (sv.bbMaxY() - sv.bbMinY());

        // Renders `total` samples of noise at a fixed position and air setting, and returns the
        // filtered-sample count alongside the audio.
        const auto renderAt = [&] (float nx, float ny, float air, float hullAtten,
                                   juce::AudioBuffer<float>& dest)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            setParam (proc, "srcX", nx);
            setParam (proc, "srcY", ny);
            setParam (proc, "airAmount", air);
            setParam (proc, "hullAtten", hullAtten);
            setParam (proc, "blur", 0.0f);

            oo::instr::resetCounters();
            renderInto (proc, dest, total, { kBlockSize }, {});

            return oo::instr::get (oo::instr::airSamplesFiltered);
        };

        if (! found)
        {
            check ("BD air-and-trim-bit-transparency", false,
                   "NO OUTSIDE POSITION FOUND — every non-vacuity control would be vacuous");
        }
        else
        {
            // 1. DSP-07/6 — INSIDE the hull, bit-identical at ANY airAmount. This is the D2
            //    amendment's whole point: the SHIPPING DEFAULT PATCH (hullAtten 1.0, airAmount
            //    0.35, puck centred) is bit-transparent.
            {
                juce::AudioBuffer<float> r0 (8, total), r35 (8, total), r100 (8, total);

                const auto c0   = renderAt (inNx, inNy, 0.0f,  1.0f, r0);
                const auto c35  = renderAt (inNx, inNy, 0.35f, 1.0f, r35);
                const auto c100 = renderAt (inNx, inNy, 1.0f,  1.0f, r100);

                const bool identical = bitIdentical (r0, r35) && bitIdentical (r0, r100);
                const bool counted   = c0 == 0 && c35 == 0 && c100 == 0;

                ok = ok && identical && counted;
                detail << "inside: air 0/0.35/1.0 "
                       << (identical ? "bit-identical" : firstDifference (r0, r35))
                       << ", filtered samples " << juce::String ((int) c35)
                       << (counted ? "" : " — FILTER RAN INSIDE THE HULL") << "; ";
            }

            // 2. DSP-07/5 — OUTSIDE the hull at airAmount = 0, the branch is never taken; and the
            //    NON-VACUITY control: at 0.35 it is taken on every sample of both sub-points, and
            //    the render CHANGES.
            {
                juce::AudioBuffer<float> off (8, total), on (8, total);

                const auto cOff = renderAt (onx, ony, 0.0f,  1.0f, off);
                const auto cOn  = renderAt (onx, ony, 0.35f, 1.0f, on);

                const bool defeated = cOff == 0;
                const bool active   = cOn == static_cast<std::uint64_t> (total) * 2;
                const bool changes  = ! bitIdentical (off, on);

                ok = ok && defeated && active && changes;
                detail << "outside: air=0 filtered " << juce::String ((int) cOff)
                       << (defeated ? "" : " — NOT DEFEATED")
                       << ", air=0.35 filtered " << juce::String ((int) cOn) << "/"
                       << juce::String (total * 2)
                       << (active ? "" : " — WRONG COUNT")
                       << (changes ? " and the render differs; " : " — RENDER UNCHANGED, VACUOUS; ");
            }

            // 3. The TRIM's non-vacuity half (its identity half is AV's bitExact against 1.0f).
            {
                juce::AudioBuffer<float> t0 (8, total), t1 (8, total);

                renderAt (onx, ony, 0.0f, 0.0f, t0);
                renderAt (onx, ony, 0.0f, 0.5f, t1);

                const bool changes = ! bitIdentical (t0, t1);

                ok = ok && changes;
                detail << "hullAtten 0 vs 0.5 outside "
                       << (changes ? "differs" : "IDENTICAL — THE TRIM IS INERT") << "; ";
            }

            // 4. DSP-07/7 — a puck OSCILLATING across the hull edge.
            //
            //    HONEST SCOPE NOTE. Under P27 the two reset policies §3.5.2 debated are
            //    OBSERVATIONALLY EQUIVALENT at the boundary: a spurious reset(0) taken while the
            //    filter is inactive is overwritten by the entry re-seed before a single sample goes
            //    through it. So this cannot distinguish them by measurement, and claiming it does
            //    would be false. The criterion is met STRUCTURALLY — the reset is guarded by an
            //    airAmount edge, not a d_hull test (see GainStage::updateControl) — and what is
            //    measured here is the property the criterion exists to protect: repeated crossings
            //    produce no discontinuity and no state corruption.
            {
                OOctagonProcessor proc;
                negotiate (proc, mono, set71);
                setParam (proc, "airAmount", 0.35f);
                setParam (proc, "blur", 0.0f);
                setParam (proc, "srcX", inNx);
                setParam (proc, "srcY", inNy);

                std::vector<Event> osc;

                for (int k = 1; k <= 12; ++k)
                {
                    const bool outward = (k % 2) == 1;

                    osc.push_back ({ 1024 * k, "srcX", outward ? onx  : inNx });
                    osc.push_back ({ 1024 * k, "srcY", outward ? ony  : inNy });
                }

                juce::AudioBuffer<float> out (8, 16384);
                renderGenInto (proc, out, 16384, { kBlockSize }, osc, [] (int) { return 1.0f; });

                float worst = 0.0f;

                for (int ch = 0; ch < 8; ++ch)
                    worst = std::max (worst, worstStepIn (out, ch, kFilterLeadIn, 16384));

                const bool smooth = worst <= kSmootherBound && allFinite (out);

                ok = ok && smooth;
                detail << "12 crossings: worst DC step " << juce::String (worst, 7)
                       << (smooth ? " (no click, state intact)" : " — DISCONTINUOUS");
            }

            check ("BD air-and-trim-bit-transparency", ok, detail);
        }
    }

    //==========================================================================
    // BE — DSP-07/8 AND RISK R6. The filter is the only recursive element on the signal path.
    //
    // Its state is STICKY BY CONSTRUCTION: s = y + v re-derives s from a value that is already
    // non-finite, so once poisoned it never recovers on its own
    // (pattern_envelope_follower_state_sticky_nan).
    {
        bool         ok = true;
        juce::String detail;

        OOctagonProcessor scout;
        negotiate (scout, mono, set71);

        float onx = 0.0f, ony = 0.0f;
        const bool found = findOutside (scout, onx, ony);

        const auto& sv = scout.getVenue();
        const float inNx = (sv.centroid().x - sv.bbMinX()) / (sv.bbMaxX() - sv.bbMinX());
        const float inNy = (sv.centroid().y - sv.bbMinY()) / (sv.bbMaxY() - sv.bbMinY());

        if (! found)
        {
            check ("BE air-filter-nan-recovery", false, "NO OUTSIDE POSITION FOUND — vacuous");
        }
        else
        {
            juce::MidiBuffer midi;

            const auto renderBlock = [&] (OOctagonProcessor& proc, float value, bool poison)
            {
                juce::AudioBuffer<float> b (8, kBlockSize);
                b.clear();

                for (int s = 0; s < kBlockSize; ++s)
                    b.setSample (0, s, value);

                if (poison)
                    b.setSample (0, kBlockSize / 2, std::numeric_limits<float>::quiet_NaN());

                proc.processBlock (b, midi);

                return allFinite (b);
            };

            // 1. Poison the filter while it is ACTIVE -> finite output within one block.
            {
                OOctagonProcessor proc;
                negotiate (proc, mono, set71);
                setParam (proc, "srcX", onx);
                setParam (proc, "srcY", ony);
                setParam (proc, "airAmount", 0.35f);

                renderBlock (proc, 0.25f, false);
                renderBlock (proc, 0.25f, true);              // NaN expected THIS block

                bool recovered = true;

                for (int k = 0; k < 3; ++k)
                    recovered = recovered && renderBlock (proc, 0.25f, false);

                ok = ok && recovered;
                detail << "active-filter NaN " << (recovered ? "recovers in one block"
                                                             : "LATCHED") << "; ";
            }

            // 2. THE SKIPPED-FILTER HOLE — the third payoff of P27, and it needs its own assertion
            //    or nobody will know it works.
            //
            //    A NaN parked in a filter that is currently SKIPPED is invisible to a per-block
            //    OUTPUT guard: the filter produces no output to inspect. Under D2 as written the
            //    filter could stay skipped indefinitely and re-enter poisoned. P27's unconditional
            //    re-seed at the false->true edge overwrites the state before it can be used.
            {
                OOctagonProcessor proc;
                negotiate (proc, mono, set71);
                setParam (proc, "srcX", onx);
                setParam (proc, "srcY", ony);
                setParam (proc, "airAmount", 0.35f);

                renderBlock (proc, 0.25f, false);
                renderBlock (proc, 0.25f, true);              // poison while active

                // Move INSIDE — the filter is now skipped, so the output guard cannot see it.
                setParam (proc, "srcX", inNx);
                setParam (proc, "srcY", inNy);

                bool insideFinite = true;

                for (int k = 0; k < 4; ++k)
                    insideFinite = insideFinite && renderBlock (proc, 0.25f, false);

                // Back OUTSIDE — the re-entry must be clean AND bit-exact.
                setParam (proc, "srcX", onx);
                setParam (proc, "srcY", ony);

                bool outsideFinite = true;

                for (int k = 0; k < 4; ++k)
                    outsideFinite = outsideFinite && renderBlock (proc, 0.25f, false);

                ok = ok && insideFinite && outsideFinite;
                detail << "skipped-filter hole: inside "
                       << (insideFinite ? "finite" : "NON-FINITE") << ", re-entry "
                       << (outsideFinite ? "clean" : "RE-ENTERED POISONED");
            }

            check ("BE air-filter-nan-recovery", ok, detail);
        }
    }

    //==========================================================================
    // BF — FUNC-07, ALL FOUR CRITERIA.
    //
    // THROUGH A NON-IDENTITY LABEL MAP, ALWAYS. All three accepted 8-channel containers have
    // initializer order == enum-bit order, so the SHIPPED default map IS the identity and a probe
    // driven by it is byte-identical to a hardcoded 0..7
    // (critical_audiochannelset_is_a_bitset_not_an_order). Only a permuted assignment can tell a
    // real map from a hardcoded one.
    {
        bool         ok = true;
        juce::String detail;

        constexpr int kSpeaker = 3;                   // arbitrary, but NOT 0 and not the last

        const auto gainsWithTrim = [&] (float trimDb, float srcX)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);
            setParam (proc, "srcX", srcX);
            setParam (proc, "srcY", 0.62f);
            setParam (proc, "blur", 0.0f);

            if (std::abs (trimDb) > 0.0f)
                applyTrim (proc, kSpeaker, trimDb);

            return std::make_pair (measureGainVector (proc), expectedLane (proc, set71, kSpeaker));
        };

        // 1 & 2. −12 dB then +6 dB at speaker 3: exactly that change on ITS lane, and the other
        //        seven BITWISE unchanged (the DBAP solve never sees trims, so bitwise is the right
        //        assertion, not "within a tolerance").
        for (float db : { -12.0f, 6.0f })
        {
            const auto base = gainsWithTrim (0.0f, 0.34f);
            const auto trim = gainsWithTrim (db,   0.34f);

            const int lane = base.second;

            const float want = juce::Decibels::decibelsToGain (db);
            const float got  = base.first[(size_t) lane] > 1.0e-6f
                                 ? trim.first[(size_t) lane] / base.first[(size_t) lane]
                                 : 0.0f;

            const bool exact = near (got, want, 1.0e-4f);

            int untouched = 0;

            for (int ch = 0; ch < 8; ++ch)
                if (ch != lane && bitExact (base.first[(size_t) ch], trim.first[(size_t) ch]))
                    ++untouched;

            ok = ok && exact && untouched == 7 && lane == trim.second;

            detail << juce::String (db, 0) << " dB on lane " << juce::String (lane) << ": x"
                   << juce::String (got, 5) << " (want x" << juce::String (want, 5) << ")"
                   << (exact ? "" : " WRONG") << ", " << juce::String (untouched)
                   << "/7 others bitwise unchanged" << (untouched == 7 ? "; " : " — LEAKED; ");
        }

        // 3. FUNC-07/3 — the trim STEP is smoothed like everything else (QUAL-01's D4 scope). The
        //    trim rides the same 17 targets, so a venue edit must not click.
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);
            setParam (proc, "srcX", 0.34f);
            setParam (proc, "srcY", 0.62f);

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> out (8, 2048);
            out.clear();

            const auto renderInto2048 = [&] (int at, int count)
            {
                juce::AudioBuffer<float> b (8, count);

                for (int ch = 0; ch < 8; ++ch)
                    b.clear (ch, 0, count);

                for (int s = 0; s < count; ++s)
                    b.setSample (0, s, 1.0f);         // DC

                proc.processBlock (b, midi);

                for (int ch = 0; ch < 8; ++ch)
                    out.copyFrom (ch, at, b, ch, 0, count);
            };

            renderInto2048 (0, 1024);
            applyTrim (proc, kSpeaker, -9.0f);        // BETWEEN two processBlock calls
            renderInto2048 (1024, 1024);

            float worst = 0.0f;

            for (int ch = 0; ch < 8; ++ch)
                worst = std::max (worst, worstStepIn (out, ch, 512, 2048));

            const bool smooth = worst <= kSmootherBound;

            ok = ok && smooth;
            detail << "trim step " << juce::String (worst, 7) << "/"
                   << juce::String (kSmootherBound, 7) << (smooth ? "; " : " OVER; ");
        }

        // 4. FUNC-07/4 — VENUE-SCOPED. A musical preset load writes all 17 parameters; the trims
        //    must be untouched, which is the FUNC-05 store separation exercised on this field.
        //    Asserted on the RENDERED ratio, not on the model: the model obviously does not change,
        //    and what the requirement is about is whether the trim still reaches the audio.
        {
            const auto before = gainsWithTrim (-12.0f, 0.34f);
            const auto beforeBase = gainsWithTrim (0.0f, 0.34f);

            // A different "musical preset": every position parameter moved.
            const auto after = gainsWithTrim (-12.0f, 0.58f);
            const auto afterBase = gainsWithTrim (0.0f, 0.58f);

            const int lane = before.second;

            const float r1 = before.first[(size_t) lane] / beforeBase.first[(size_t) lane];
            const float r2 = after.first[(size_t) lane]  / afterBase.first[(size_t) lane];

            const bool preserved = near (r1, r2, 1.0e-4f)
                                && near (r1, juce::Decibels::decibelsToGain (-12.0f), 1.0e-4f);

            ok = ok && preserved;
            detail << "venue-scoped: ratio " << juce::String (r1, 6) << " -> "
                   << juce::String (r2, 6)
                   << (preserved ? " (survives a musical preset load)"
                                 : " — THE TRIM MOVED WITH THE MUSICAL STATE");
        }

        check ("BF per-speaker-trim", ok, detail);
    }

    //==========================================================================
    // BG — QUAL-01 UNDER D4's SCOPE: one representative LIVE venue edit during playback.
    //
    // Venue values are message-thread edits, not automation lanes, so a full-speed sweep of all 50
    // stresses a path no user can drive. What a user CAN do is type a number while audio is
    // running, and everything a venue edit touches lands on the same smoothed targets.
    //
    // WINDOW: starts at a block boundary and runs at least 64 + 240 samples. The edit takes effect
    // at the NEXT control boundary — up to 63 samples later — and then ramps for 240. A narrower
    // window can miss the event entirely and pass vacuously. Alignment is otherwise free here,
    // because the effect is always control-grid-aligned; that is why this needs less care than
    // QUAL-03's protocol. (Probe AQ's mid-stream edit rig, unmodified — Q10.)
    {
        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.34f);
        setParam (proc, "srcY", 0.62f);

        juce::MidiBuffer midi;

        constexpr int settle = 1024;
        constexpr int window = 1024;                  // >> 64 + 240

        static_assert (window >= 64 + 240, "the edit can land up to 63 samples late and then ramps "
                                           "for 240; a shorter window can miss it entirely");

        juce::AudioBuffer<float> out (8, settle + window);
        out.clear();

        const auto renderChunkAt = [&] (int at, int count)
        {
            juce::AudioBuffer<float> b (8, count);

            for (int ch = 0; ch < 8; ++ch)
                b.clear (ch, 0, count);

            for (int s = 0; s < count; ++s)
                b.setSample (0, s, 1.0f);

            proc.processBlock (b, midi);

            for (int ch = 0; ch < 8; ++ch)
                out.copyFrom (ch, at, b, ch, 0, count);
        };

        renderChunkAt (0, settle);

        const auto beforeEdit = out.getSample (0, settle - 1);

        // THE EDIT, between two processBlock calls, with no parameter change at all.
        {
            oo::VenueModel v = proc.getVenue();
            v.setSpeakerTrimDb (5, -7.5f);
            v.setRake (1.25f, 4.10f);
            proc.applyVenueEdit (v);
        }

        renderChunkAt (settle, window);

        float worst = 0.0f;
        bool  moved = false;

        for (int ch = 0; ch < 8; ++ch)
        {
            worst = std::max (worst, worstStepIn (out, ch, settle - 64, settle + window));

            if (std::abs (out.getSample (ch, settle + window - 1)
                        - out.getSample (ch, settle - 1)) > 1.0e-4f)
                moved = true;
        }

        const bool smooth = worst <= kSmootherBound;
        const bool ok = smooth && moved && allFinite (out);

        check ("BG live-venue-edit-during-playback", ok,
               juce::String ("worst step ") + juce::String (worst, 7) + "/"
                   + juce::String (kSmootherBound, 7) + (smooth ? " (smoothed)" : " — CLICK")
                   + ", gain " + juce::String (beforeEdit, 6) + " -> "
                   + juce::String (out.getSample (0, settle + window - 1), 6)
                   + (moved ? " (the edit LANDED)"
                            : " — NOTHING MOVED, THE PROBE IS VACUOUS"));
    }

    //==========================================================================
    // BH — P29 / RESEARCH-2.3 H5: THE DEFECT FUNC-07's MULTIPLY WOULD OTHERWISE HAVE CREATED.
    //
    // Both hazards below were LIVE before this phase's fix, and the reason is worth stating rather
    // than implying: an unsanitised venue value reaches setTargetValue through §3.3.4's guards
    // untouched, because `dRaw < kMinDistance` is FALSE for NaN and `denom < kDenomEpsilon` is
    // FALSE for NaN. Neither guard is a NaN guard.
    //
    //   trimDb = 1e30  ->  decibelsToGain -> +inf  ->  trimLin = inf
    //   v_i = 0.0f     (EXACTLY, whenever w_i == 0 — DSP-05/1)
    //   0.0f * inf     =  NaN  ->  SmoothedValue latches  ->  PERMANENT SILENCE
    {
        bool         ok = true;
        juce::String detail;

        juce::MidiBuffer midi;

        const auto renderNoiseBlock = [&] (OOctagonProcessor& proc, int n)
        {
            juce::AudioBuffer<float> b (8, kBlockSize);
            b.clear();

            for (int s = 0; s < kBlockSize; ++s)
                b.setSample (0, s, noiseAt (n * kBlockSize + s));

            proc.processBlock (b, midi);

            return b;
        };

        // 1. THE EXACT 0.0f * inf PATH, driven deliberately: w_i = 0 at the poisoned speaker.
        for (float poison : { 1.0e30f, -1.0e30f, std::numeric_limits<float>::quiet_NaN() })
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);

            auto w = deltaWeights (0);                // w[0] = 1, every other weight EXACTLY 0
            setWeights (proc, w);

            applyTrim (proc, 4, poison);              // a speaker whose weight is exactly zero

            bool finite = true;

            for (int n = 0; n < 4; ++n)
                finite = finite && allFinite (renderNoiseBlock (proc, n));

            // AND IT MUST NOT HAVE LATCHED: restore a sane state and the plugin produces audio
            // again, which a poisoned SmoothedValue could never do.
            applyTrim (proc, 4, 0.0f);
            setWeights (proc, { 1, 1, 1, 1, 1, 1, 1, 1 });

            juce::AudioBuffer<float> settleBuf (8, kBlockSize);
            renderSteady (proc, settleBuf, 4 * kBlockSize, 1.0f, true);

            bool signalBack = false;

            for (int ch = 0; ch < 8; ++ch)
                if (settleBuf.getMagnitude (ch, 0, kBlockSize) > 1.0e-3f)
                    signalBack = true;

            ok = ok && finite && signalBack;

            detail << "trimDb " << juce::String (poison, 1) << " w=0: "
                   << (finite ? "finite" : "NON-FINITE") << "/"
                   << (signalBack ? "no latch" : "PERMANENT SILENCE") << "; ";
        }

        // 2. The trim is CLAMPED, not merely finite — asserted against the source constant rather
        //    than a transcribed 24.0f (pattern_test_fixture_mirrors_drift_silently).
        {
            OOctagonProcessor loud, ref;
            negotiate (loud, mono, set71);
            negotiate (ref,  mono, set71);
            setParam (loud, "blur", 0.0f);
            setParam (ref,  "blur", 0.0f);

            applyTrim (loud, 2, 1.0e30f);

            const auto gLoud = measureGainVector (loud);
            const auto gRef  = measureGainVector (ref);

            const float ceiling = juce::Decibels::decibelsToGain (
                                      OOctagonProcessor::kVenueTrimClampDb);

            const float ratio = gRef[2] > 1.0e-6f ? gLoud[2] / gRef[2] : 0.0f;
            const bool  clamped = std::isfinite (ratio) && ratio <= ceiling * 1.001f
                               && ratio > ceiling * 0.999f;

            ok = ok && clamped;
            detail << "clamp: x" << juce::String (ratio, 4) << " vs ±"
                   << juce::String (OOctagonProcessor::kVenueTrimClampDb, 0) << " dB (x"
                   << juce::String (ceiling, 4) << ")" << (clamped ? "; " : " — NOT CLAMPED; ");
        }

        // 3. A NaN speaker COORDINATE — the pre-existing 2.2 hazard folded in at the same site, and
        //    recorded as a scope addition rather than slipped in.
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);

            {
                oo::VenueModel v = proc.getVenue();
                const auto p = v.speaker (6);
                v.setSpeakerPosition (6, { std::numeric_limits<float>::quiet_NaN(), p.y, p.z });
                proc.applyVenueEdit (v);
            }

            bool finite = true;

            for (int n = 0; n < 4; ++n)
                finite = finite && allFinite (renderNoiseBlock (proc, n));

            ok = ok && finite;
            detail << "NaN coordinate " << (finite ? "finite output" : "NON-FINITE");
        }

        check ("BH venue-value-sanitisation", ok, detail);
    }

    //==========================================================================
    // BI — QUAL-03 WITH EVERYTHING LIVE. The ROADMAP criterion, re-run against the 2.3 surface.
    //
    // AL's 512/4096 pair and AM's ragged sequence proved block-size invariance over the 2.2
    // surface. This re-runs both with width = 4, hullAtten = 1.0, airAmount = 0.35 and the source
    // OUTSIDE the hull, so the trim, BOTH filters and two DISTINCT sub-point distances are all
    // live. H10 is the argument that the airActive skip cannot break QUAL-03 — the skip gates
    // processSample and nothing else, and every getNextValue() still runs unconditionally. This
    // probe is the evidence. memcmp, never a tolerance.
    {
        OOctagonProcessor scout;
        negotiate (scout, mono, set71);

        float onx = 0.0f, ony = 0.0f;
        const bool found = findOutside (scout, onx, ony);

        if (! found)
        {
            check ("BI blocksize-invariance-everything-live", false,
                   "NO OUTSIDE POSITION FOUND — the filters would never run and the probe would "
                   "reduce to AL");
        }
        else
        {
            const auto arm = [&] (OOctagonProcessor& p)
            {
                negotiate (p, mono, set71);
                applyRotatedLabels (p);
                setParam (p, "srcX", onx);
                setParam (p, "srcY", ony);
                setParam (p, "width", 4.0f);
                setParam (p, "hullAtten", 1.0f);
                setParam (p, "airAmount", 0.35f);
            };

            bool         ok = true;
            juce::String detail;

            // 1. AL's shape: fixed 512 vs fixed 4096, events at multiples of 4096.
            {
                constexpr int total = 4096 * 5;

                // srcZ rather than a scaled srcX/srcY: the farthest-outside position can sit on a
                // bbox edge at 0.0, where scaling is a no-op and the "event" would change nothing.
                const std::vector<Event> events
                    { { 4096 * 1, "airAmount", 0.80f }, { 4096 * 2, "width", 6.0f },
                      { 4096 * 3, "hullAtten", 2.4f },  { 4096 * 4, "srcZ", -1.75f } };

                OOctagonProcessor a, b;
                arm (a); arm (b);

                juce::AudioBuffer<float> outA (8, total), outB (8, total);

                renderInto (a, outA, total, { 512 },  events);
                renderInto (b, outB, total, { 4096 }, events);

                const bool identical = bitIdentical (outA, outB);
                const bool live = outA.getMagnitude (0, 0, total) > 1.0e-4f;

                ok = ok && identical && live;
                detail << "512 vs 4096 "
                       << (identical ? "bit-identical" : firstDifference (outA, outB))
                       << (live ? "; " : " — SILENT, VACUOUS; ");
            }

            // 2. AM's shape: ragged sizes, events at arbitrary non-aligned offsets.
            {
                constexpr int total = 20000;

                const std::vector<Event> events
                    { { 1,     "airAmount", 0.15f }, { 337,   "width", 2.5f },
                      { 1025,  "hullAtten", 0.4f },  { 4097,  "airAmount", 0.95f },
                      { 6151,  "srcZ", 2.40f },      { 11113, "width", 6.0f },
                      { 15999, "outputGain", -6.5f } };

                OOctagonProcessor a, b;
                arm (a); arm (b);

                juce::AudioBuffer<float> outA (8, total), outB (8, total);

                renderInto (a, outA, total, { 1, 7, 64, 333, 4096 }, events);
                renderInto (b, outB, total, { 4096 },                events);

                const bool identical = bitIdentical (outA, outB);
                const bool live = outA.getMagnitude (0, 0, total) > 1.0e-4f;

                ok = ok && identical && live;
                detail << "ragged 1,7,64,333,4096 vs 4096 "
                       << (identical ? "bit-identical, 7 non-aligned events"
                                     : firstDifference (outA, outB))
                       << (live ? "" : " — SILENT, VACUOUS");
            }

            check ("BI blocksize-invariance-everything-live", ok, detail);
        }
    }

    //==========================================================================
    // BJ — PERF-01/PERF-02 AS NUMBERS, AND THE REGRESSION GUARD ON PROBE AE.
    //
    // airCutoffUpdates == solveRuns * 2 asserts PLACEMENT, not cost: it is the executable form of
    // "setCutoffFrequency is called at the control boundary and never per sample". And powCalls
    // must STILL be exactly 16 — the new exp2 and the two Decibels conversions deliberately do not
    // route through countedPow, and confirming that here is what stops a green PERF gate from
    // turning red for a reason unrelated to the change.
    {
        OOctagonProcessor scout;
        negotiate (scout, mono, set71);

        float onx = 0.0f, ony = 0.0f;
        const bool found = findOutside (scout, onx, ony);

        bool         ok = found;
        juce::String detail;

        if (! found)
        {
            detail << "NO OUTSIDE POSITION FOUND — the filtered-sample count would be vacuously 0";
        }
        else
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            setParam (proc, "srcX", onx);
            setParam (proc, "srcY", ony);
            setParam (proc, "airAmount", 0.35f);

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> block (8, kBlockSize);

            const auto renderBlocks = [&] (int count)
            {
                for (int i = 0; i < count; ++i)
                {
                    block.clear();
                    proc.processBlock (block, midi);
                }
            };

            renderBlocks (2);                         // settle, so the dirty check is quiescent

            // 1. A parameter change -> exactly one solve, two cutoff updates, and STILL 32 pow
            //    (v1.3.0: two sub-point solves + two z-cue reference solves, 8 each).
            oo::instr::resetCounters();
            setParam (proc, "srcZ", 1.25f);
            renderBlocks (1);

            const auto solves1 = oo::instr::get (oo::instr::solveRuns);
            const auto cutoffs1 = oo::instr::get (oo::instr::airCutoffUpdates);
            const auto pow1 = oo::instr::get (oo::instr::powCalls);

            // 2. Over a longer run with continuous automation, the identity must still hold
            //    exactly — this is what would break if the call migrated into the sample loop.
            oo::instr::resetCounters();

            constexpr int blocks = 24;

            for (int i = 0; i < blocks; ++i)
            {
                // srcZ, NOT a scaled srcX. The farthest-outside position can legitimately sit at
                // nx = 0.0, where `onx * k` is a no-op — every block would then be a cache hit, the
                // solve would never run, and `cutoffs == solves * 2` would pass as 0 == 0. srcZ
                // also leaves d_hull untouched (the hull is 2D on the floor), so the filter stays
                // active throughout and the filtered-sample count stays exact.
                setParam (proc, "srcZ", -2.0f + 0.25f * static_cast<float> (i % 9));
                block.clear();

                for (int s = 0; s < kBlockSize; ++s)
                    block.setSample (0, s, noiseAt (i * kBlockSize + s));

                proc.processBlock (block, midi);
            }

            const auto solvesN   = oo::instr::get (oo::instr::solveRuns);
            const auto cutoffsN  = oo::instr::get (oo::instr::airCutoffUpdates);
            const auto filteredN = oo::instr::get (oo::instr::airSamplesFiltered);
            const auto advancesN = oo::instr::get (oo::instr::sampleAdvances);

            const auto totalSamples = static_cast<std::uint64_t> (blocks * kBlockSize);

            // NON-VACUITY, AND IT IS NOT OPTIONAL: `cutoffs == solves * 2` is satisfied by 0 == 0,
            // so a run in which nothing re-solved would pass this probe while measuring nothing.
            const bool solvesRan = solves1 == 1 && solvesN >= static_cast<std::uint64_t> (blocks);

            const bool placement = cutoffs1 == solves1 * 2 && cutoffsN == solvesN * 2;
            const bool powIntact = pow1 == 32;
            const bool filtered  = filteredN == totalSamples * 2;
            const bool advanced  = advancesN == totalSamples;

            ok = solvesRan && placement && powIntact && filtered && advanced;

            if (! solvesRan)
                detail << "ONLY " << juce::String ((int) solvesN) << " SOLVES IN "
                       << juce::String (blocks) << " AUTOMATED BLOCKS — the identity below would "
                          "pass as 0 == 0; ";

            detail << "cutoffs " << juce::String ((int) cutoffsN) << " == solves "
                   << juce::String ((int) solvesN) << " x2"
                   << (placement ? "" : " — setCutoffFrequency HAS MOVED INTO THE SAMPLE LOOP")
                   << ", pow " << juce::String ((int) pow1)
                   << (powIntact ? " (AE intact)" : " — AE's EXACT-16 BUDGET IS DISTURBED")
                   << ", filtered " << juce::String ((int) filteredN) << "/"
                   << juce::String ((int) (totalSamples * 2)) << (filtered ? "" : " WRONG")
                   << ", advances " << juce::String ((int) advancesN) << "/"
                   << juce::String ((int) totalSamples) << (advanced ? "" : " WRONG");
        }

        check ("BJ control-rate-counters", ok, detail);
    }

    //==========================================================================
    // ── Phase 3.1 (GUI) — the C++ halves of three UI claims ───────────────────────────────────
    //
    // These three exist because the JS gates cannot reach the plugin. ui_layout_check.js drives a
    // real browser against the ui-stub, which is a fixture; BK, BL and BM assert the same three
    // facts against the LIVE processor, so a stub that drifted from the plugin cannot let a UI-02
    // criterion close against nothing.
    //
    // BK — the hull the overlay draws. UI-02 criterion 2 says speakers 3 and 8 render ON_EDGE
    //      rather than as vertices; that is only true if ConvexHull2D says so, and the overlay
    //      calls this same classify(). Made a first-class return value at Phase 2.1 (P11) exactly
    //      so the picture and the solve could not disagree.
    {
        OOctagonProcessor proc;
        const bool negotiated = negotiate (proc, mono, set71);

        const auto& hull = proc.getHull();

        const int hullCount = hull.getNumHullPoints();

        using Cls = oo::ConvexHull2D::Classification;

        // 0-based indices: speakers 3 and 8 in the UI's 1-based numbering.
        const bool onEdge = hull.classify (2) == Cls::ON_EDGE
                         && hull.classify (7) == Cls::ON_EDGE;

        bool othersVertex = true;

        for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
            if (i != 2 && i != 7 && hull.classify (i) != Cls::VERTEX)
                othersVertex = false;

        const bool ok = negotiated && hullCount == 6 && onEdge && othersVertex;

        // The measured classifications are PRINTED, so a future venue change reads as a diff
        // rather than as a bare FAIL with nothing to compare against.
        juce::String detail;
        detail << "hullCount " << juce::String (hullCount) << " (expect 6), classes ";

        for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
        {
            const auto c = hull.classify (i);
            detail << juce::String (i + 1) << ":"
                   << (c == Cls::VERTEX ? "V" : c == Cls::ON_EDGE ? "E" : "I")
                   << (i + 1 < oo::VenueModel::kNumSpeakers ? " " : "");
        }

        if (! onEdge)
            detail << " — SPEAKERS 3 AND 8 ARE NOT BOTH ON_EDGE; the UI-02/2 overlay claim is stale";

        check ("BK hull-classification-for-overlay", ok, detail);
    }

    //==========================================================================
    // BL — UI-02 criterion 5's C++ half (PLAN-3.1 P45b). "The metres readout is resolved against
    //      the live venue" decomposes into: the page asks the plugin, and the plugin's answer
    //      MOVES when the venue moves. This is the second half, without needing a WebView: read
    //      the same accessors getVenueGeometry reads, apply a venue edit, and read them again.
    //
    //      The generation must advance too — that counter is the ONLY signal the editor has that a
    //      cached envelope has gone stale, and a venue edit that did not advance it would leave the
    //      page showing the previous room indefinitely.
    {
        OOctagonProcessor proc;
        const bool negotiated = negotiate (proc, mono, set71);

        const float beforeMaxX = proc.getVenue().bbMaxX();
        const float beforeMinY = proc.getVenue().bbMinY();
        const auto  beforeGen  = proc.getVenueGeneration();

        // Speaker 2 is the front-right corner at (12.50, 4.50) — a bbox EXTREME on both axes, so
        // moving it outward must move both bounds. Moving an interior speaker would leave the bbox
        // untouched and the probe would assert nothing.
        constexpr float kPushX = 3.0f;
        constexpr float kPullY = 2.0f;

        oo::VenueModel edited = proc.getVenue();
        const auto p2 = edited.speaker (1);
        edited.setSpeakerPosition (1, oo::Vec3 { p2.x + kPushX, p2.y - kPullY, p2.z });

        proc.applyVenueEdit (edited);

        const float afterMaxX = proc.getVenue().bbMaxX();
        const float afterMinY = proc.getVenue().bbMinY();
        const auto  afterGen  = proc.getVenueGeneration();

        const bool xMoved = near (afterMaxX, beforeMaxX + kPushX, 1.0e-4f);
        const bool yMoved = near (afterMinY, beforeMinY - kPullY, 1.0e-4f);
        const bool genAdvanced = afterGen > beforeGen;

        const bool ok = negotiated && xMoved && yMoved && genAdvanced;

        juce::String detail;
        detail << "bbMaxX " << juce::String (beforeMaxX, 3) << " -> " << juce::String (afterMaxX, 3)
               << " (expect +" << juce::String (kPushX, 3) << ")"
               << (xMoved ? "" : " WRONG")
               << ", bbMinY " << juce::String (beforeMinY, 3) << " -> " << juce::String (afterMinY, 3)
               << " (expect -" << juce::String (kPullY, 3) << ")"
               << (yMoved ? "" : " WRONG")
               << ", gen " << juce::String ((int) beforeGen) << " -> " << juce::String ((int) afterGen)
               << (genAdvanced ? "" : " — DID NOT ADVANCE; a cached envelope would never refresh");

        check ("BL venue-edit-moves-envelope", ok, detail);
    }

    //==========================================================================
    // BM — the SAFE banner's C++ half. ROADMAP Phase 3.1 asks that the banner appear on a stereo
    //      track AND ONLY THERE, which is two claims: it raises on the fold, and it does NOT raise
    //      on any of the three 8-channel containers Logic exposes. Both halves are driven here.
    //
    //      isSafeMode() is written in prepareToPlay() as the complement of the three REAL
    //      containers, so this probe is also what would catch that predicate drifting away from
    //      isBusesLayoutSupported()'s.
    {
        struct Case { juce::AudioChannelSet out; bool expectSafe; const char* label; };

        const Case cases[] = {
            { juce::AudioChannelSet::mono(),             true,  "mono"       },
            { juce::AudioChannelSet::stereo(),           true,  "stereo"     },
            { juce::AudioChannelSet::create7point1(),    false, "7.1"        },
            { juce::AudioChannelSet::create7point1SDDS(), false, "7.1-SDDS"  },
            { juce::AudioChannelSet::create5point1point2(), false, "5.1.2"   },
        };

        bool         ok = true;
        juce::String detail;

        for (const auto& c : cases)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, mono, c.out))
            {
                ok = false;
                detail << c.label << ":REJECTED ";
                continue;
            }

            const bool safe = proc.isSafeMode();

            if (safe != c.expectSafe)
                ok = false;

            detail << c.label << ":" << (safe ? "SAFE" : "REAL")
                   << (safe == c.expectSafe ? "" : "(WRONG)") << " ";
        }

        check ("BM safe-mode-tracks-layout", ok, detail);
    }

    // ══════════════════════════════════════════════════════════════════════════════════════════
    // PHASE 3.2 — BP, BQ, BR, BS, BT, BU, BW, BX, BY, BZ.
    // ══════════════════════════════════════════════════════════════════════════════════════════

    const auto set8      = juce::AudioChannelSet::create7point1();
    const auto monoIn    = juce::AudioChannelSet::mono();
    const auto scratch32 = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getChildFile ("OOctagon-32-probes");
    scratch32.createDirectory();

    //==========================================================================
    // BP — N8 / P52, and ROADMAP orphan 4: applyVenueEditChecked REJECTS a duplicate label,
    //      NAMES the row, and leaves the venue completely untouched.
    //
    //      The failure this closes is audible, not cosmetic. mappedOutputAvailable() false sends
    //      GainStage to its else arm, which writes out[ch][n] = ch == 0 ? sL : sR with numWrite 8
    //      — speaker 1 gets the left input and speakers 2 through 8 all get the right one, at
    //      unity. Under commit-on-blur that state would hold for as long as an operator takes to
    //      type the second label of a swap.
    //
    //      NC6 removes the guard and this probe must fire.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            const auto before = proc.getVenue();

            oo::VenueModel bad = before;
            bad.setSpeakerLabel (3, before.labelAbbreviation (1));   // row 4 duplicates row 2
            bad.setSpeakerPosition (0, { 99.0f, 99.0f, 99.0f });     // and moves a coordinate too

            ochan::MapDiagnosis whyNot {};
            const bool applied = proc.applyVenueEditChecked (bad, &whyNot);

            int firstBad = -1;
            const bool untouched = sameFifty (before, proc.getVenue(), firstBad);

            // mapInvalid stays FALSE because nothing was applied — the guard removed the
            // transient, and the backstop was never reached.
            const bool stillValid = ! proc.isChannelMapInvalid();

            const bool named = whyNot.reason == ochan::MapFailure::duplicateLabel
                               && whyNot.speakerIndex == 3;

            // The positive control: the SAME venue with the duplicate resolved must apply, or the
            // rejection above could be "applyVenueEditChecked always says no".
            oo::VenueModel good = bad;
            good.setSpeakerLabel (3, before.labelAbbreviation (3));
            ochan::MapDiagnosis okWhy {};
            const bool goodApplied = proc.applyVenueEditChecked (good, &okWhy);
            const bool moved = ! bitExact (proc.getVenue().speaker (0).x, before.speaker (0).x);

            ok = ! applied && untouched && stillValid && named && goodApplied && moved;

            detail << "duplicate: " << (applied ? "APPLIED" : "rejected")
                   << ", reason " << static_cast<int> (whyNot.reason)
                   << " row " << (whyNot.speakerIndex + 1)
                   << ", venue " << (untouched ? "untouched" : "MODIFIED")
                   << ", mapInvalid " << (stillValid ? "false" : "RAISED")
                   << "; positive control " << (goodApplied && moved ? "applied and moved" : "DID NOT APPLY");
        }

        check ("BP checked-venue-edit-guard", ok, detail);
    }

    //==========================================================================
    // BQ — FUNC-04/1 and ROADMAP orphan 5: the ping sounds from EXACTLY ONE lane, the other seven
    //      are EXACT zero, on a NON-IDENTITY map — and a label edit MOVES the sounding lane.
    //
    //      The non-identity map is the whole probe. All three accepted 8-channel containers have
    //      initializer order == enum-bit order, so the shipped default map IS the identity and a
    //      probe driven by it is byte-identical to one driven by a hardcoded out[i] (C1, and the
    //      reason probe D was built this way at 2.1).
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            applyRotatedLabels (proc);

            int wrongLane = 0, leaked = 0, silent = 0;

            for (int speaker = 1; speaker <= 8; ++speaker)
            {
                if (! proc.startVerifyPing (speaker))
                {
                    ok = false;
                    detail << "start(" << speaker << ") REFUSED; ";
                    continue;
                }

                const auto stats = renderPingSilence (proc, 4 * kBlockSize, 8 * kBlockSize);
                const int  expect = expectedLane (proc, set8, speaker - 1);

                if (stats.soundingLanes() != 1) ++leaked;
                if (stats.loudestLane() != expect) ++wrongLane;
                if (! stats.anyNonZero[static_cast<std::size_t> (expect)]) ++silent;

                proc.stopVerifyPing();
                renderPingSilence (proc, 2 * kBlockSize, 0);   // let the release finish
            }

            const bool routing = wrongLane == 0 && leaked == 0 && silent == 0;

            // ── ROADMAP orphan 5: a label-row change moves audio, CONFIRMED BY PING ──
            // AJ closed the rendered-tone half at 2.2; this is the ping-confirmed half.
            const int laneBefore = expectedLane (proc, set8, 0);

            oo::VenueModel swapped = proc.getVenue();
            const auto l0 = swapped.labelAbbreviation (0);
            const auto l1 = swapped.labelAbbreviation (1);
            swapped.setSpeakerLabel (0, l1);
            swapped.setSpeakerLabel (1, l0);

            ochan::MapDiagnosis why {};
            const bool swapOk = proc.applyVenueEditChecked (swapped, &why);

            proc.startVerifyPing (1);
            const auto afterStats = renderPingSilence (proc, 4 * kBlockSize, 8 * kBlockSize);
            const int  laneAfter  = expectedLane (proc, set8, 0);
            proc.stopVerifyPing();
            renderPingSilence (proc, 2 * kBlockSize, 0);

            const bool followed = swapOk && laneAfter != laneBefore
                                  && afterStats.loudestLane() == laneAfter
                                  && afterStats.soundingLanes() == 1;

            // ── RT-safety of the new audio-thread code, measured rather than asserted ──
            proc.startVerifyPing (3);
            rtcheck::arm();
            renderPingSilence (proc, 0, 4 * kBlockSize);
            rtcheck::disarm();
            const auto allocs = rtcheck::allocations.load();
            proc.stopVerifyPing();
            renderPingSilence (proc, 2 * kBlockSize, 0);

            ok = ok && routing && followed && allocs == 0;

            detail << "8 targets on a NON-IDENTITY map: "
                   << (routing ? "one lane each, seven EXACT zero" : "FAILED")
                   << " (wrongLane " << wrongLane << ", leaked " << leaked << ", silent " << silent << ")"
                   << "; label swap: lane " << laneBefore << " -> " << laneAfter << " "
                   << (followed ? "FOLLOWED" : "DID NOT FOLLOW")
                   << "; allocations while pinging " << allocs;
        }

        check ("BQ ping-routes-through-map", ok, detail);
    }

    //==========================================================================
    // BR — FUNC-04/3 and ROADMAP orphan 2: the ceiling holds at outputGain +12 dB AND trim +6 dB
    //      SIMULTANEOUSLY.
    //
    //      Either one alone leaves the other multiply untested, and the criterion's own wording
    //      ("regardless of outputGain") does not reach the trims at all — FUNC-07's per-speaker
    //      trim is a separate multiply, live since 2.3. Both are bypassed by construction because
    //      the ping is a POST-WRITE OVERWRITE, and this is what measures that claim.
    //
    //      ASSERTS THE RESULT, NOT THE CONSTANT. kPinkNormScalar was calibrated by measurement at
    //      execute; a probe written against it would agree with any value it was given.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            setParam (proc, "outputGain", 12.0f);
            applyTrim (proc, 0, 6.0f);

            const int lane = expectedLane (proc, set8, 0);

            ok = proc.startVerifyPing (1);

            // Warm past the 20 ms fade-in (960 samples at 48 kHz) before measuring the sustain.
            const auto stats = renderPingSilence (proc, 4 * kBlockSize, 32 * kBlockSize);
            proc.stopVerifyPing();
            renderPingSilence (proc, 2 * kBlockSize, 0);

            const float rms  = stats.rmsDb (lane);
            const float peak = stats.peakDb (lane);

            // +/- 1.5 dB on the RMS: the calibration is a statistical property of a noise source
            // and the measurement window is finite, so a tighter bound would be measuring the
            // window rather than the level.
            const bool level   = std::abs (rms - oo::VerifyPing::kTargetRmsDb) <= 1.5f;
            const bool ceiling = peak <= oo::VerifyPing::kPeakCeilDb + 0.01f;

            ok = ok && level && ceiling;

            detail << "at outputGain +12 dB AND trim +6 dB: RMS "
                   << juce::String (rms, 2) << " dBFS (target "
                   << juce::String (oo::VerifyPing::kTargetRmsDb, 1) << ") "
                   << (level ? "OK" : "OUT OF BOUND")
                   << ", peak " << juce::String (peak, 2) << " dBFS (ceiling "
                   << juce::String (oo::VerifyPing::kPeakCeilDb, 1) << ") "
                   << (ceiling ? "OK" : "EXCEEDED")
                   << ", crest " << juce::String (peak - rms, 2) << " dB";
        }

        check ("BR ping-ceiling-holds", ok, detail);
    }

    //==========================================================================
    // BS — FUNC-04/2 and ROADMAP orphan 3: the auto cycle runs 1 -> 8 at 1.2 s on / 0.4 s gap and
    //      completes in 12.8 s, MEASURED IN SAMPLES DERIVED FROM THE PREPARED RATE.
    //
    //      Never against a transcribed 614400 (pattern_test_fixture_mirrors_drift_silently). The
    //      expected figure below is kAutoCycleSeconds x kSampleRate, so a rate change or a timing
    //      change moves the expectation with the code.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            constexpr int kStep = 64;                       // one control block: the sampling grain

            const auto expectedTotal = static_cast<long long> (
                std::lround (oo::VerifyPing::kAutoCycleSeconds * kSampleRate));
            const auto expectedOn  = static_cast<long long> (
                std::lround (oo::VerifyPing::kOnSeconds * kSampleRate));
            const auto expectedGap = static_cast<long long> (
                std::lround (oo::VerifyPing::kGapSeconds * kSampleRate));

            ok = proc.startVerifyPing (oo::VerifyPing::kAuto);

            juce::AudioBuffer<float> block (8, kStep);
            juce::MidiBuffer midi;

            std::vector<int>       order;
            std::vector<long long> onRuns, gapRuns;
            int       lastSpeaker = 0;
            long long runStart = 0, n = 0;
            long long stoppedAt = -1;

            const long long cap = expectedTotal + 4 * kStep;

            while (n < cap)
            {
                block.clear();
                proc.processBlock (block, midi);
                n += kStep;

                const auto state = proc.verifyPingState();

                if (state.speaker != lastSpeaker)
                {
                    const long long len = n - kStep - runStart;

                    if (lastSpeaker == 0 && runStart > 0) gapRuns.push_back (len);
                    else if (lastSpeaker > 0)             onRuns.push_back (len);

                    if (state.speaker > 0) order.push_back (state.speaker);

                    lastSpeaker = state.speaker;
                    runStart    = n - kStep;
                }

                if (! state.active && stoppedAt < 0 && n > kStep)
                {
                    stoppedAt = n;
                    break;
                }
            }

            bool sequence = order.size() == 8;
            for (std::size_t i = 0; i < order.size() && sequence; ++i)
                sequence = order[i] == static_cast<int> (i) + 1;

            const auto within = [] (long long v, long long target, long long tol)
            {
                return std::llabs (v - target) <= tol;
            };

            bool onOk = onRuns.size() >= 7;
            for (std::size_t i = 0; i < onRuns.size() && onOk; ++i)
                onOk = within (onRuns[i], expectedOn, 3 * kStep);

            bool gapOk = gapRuns.size() >= 6;
            for (std::size_t i = 0; i < gapRuns.size() && gapOk; ++i)
                gapOk = within (gapRuns[i], expectedGap, 3 * kStep);

            const bool totalOk = stoppedAt > 0 && within (stoppedAt, expectedTotal, 4 * kStep);

            // ── The MANUAL half: start advances 1 -> 8, one speaker at a time. ──
            bool manual = true;
            for (int s = 1; s <= 8 && manual; ++s)
            {
                manual = proc.startVerifyPing (s);
                block.clear();
                proc.processBlock (block, midi);
                manual = manual && proc.verifyPingState().speaker == s;
            }
            proc.stopVerifyPing();

            ok = ok && sequence && onOk && gapOk && totalOk && manual;

            juce::String orderStr;
            for (const int s : order) orderStr << s;

            detail << "order " << (orderStr.isEmpty() ? juce::String ("-") : orderStr) << " "
                   << (sequence ? "1->8" : "WRONG")
                   << "; on " << (onRuns.empty() ? 0 : onRuns[0]) << " smp (expect " << expectedOn
                   << ") " << (onOk ? "ok" : "OUT")
                   << "; gap " << (gapRuns.empty() ? 0 : gapRuns[0]) << " (expect " << expectedGap
                   << ") " << (gapOk ? "ok" : "OUT")
                   << "; total " << stoppedAt << " (expect " << expectedTotal << " = "
                   << juce::String (oo::VerifyPing::kAutoCycleSeconds, 1) << " s) "
                   << (totalOk ? "ok" : "OUT")
                   << "; manual 1->8 " << (manual ? "ok" : "FAILED");
        }

        check ("BS ping-auto-cycle-12.8s", ok, detail);
    }

    //==========================================================================
    // BT — ROADMAP orphan 1: a LATCHED ping self-stops at 120 s, sample-counted.
    //
    //      No FUNC-04 line mentions the timeout at all — it is carried here as a named gate rather
    //      than dropped because dropping it turns nothing yellow. It is also the reason both clocks
    //      are counted in samples: a juce::Timer would be unmeasurable from this target.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            const auto expected = static_cast<long long> (
                std::lround (oo::VerifyPing::kLatchSeconds * kSampleRate));

            ok = proc.startVerifyPing (4);

            juce::AudioBuffer<float> block (8, kBlockSize);
            juce::MidiBuffer midi;

            long long n = 0, stoppedAt = -1;
            const long long cap = expected + 200 * kBlockSize;

            while (n < cap)
            {
                block.clear();
                proc.processBlock (block, midi);
                n += kBlockSize;

                if (! proc.verifyPingState().active)
                {
                    stoppedAt = n;
                    break;
                }
            }

            // Half a second either side: the stop is sample-accurate but observed at block
            // granularity, and the assertion that matters is "it stopped at 120 s, not at 119 or
            // never".
            const bool onTime = stoppedAt > 0
                                && std::llabs (stoppedAt - expected) <= static_cast<long long> (kSampleRate / 2);

            // It must still have been sounding well before the latch, or "stopped at 120 s" would
            // also be true of a ping that never started.
            ok = ok && onTime;

            detail << "latched ping stopped at " << stoppedAt << " smp, expected " << expected
                   << " (" << juce::String (oo::VerifyPing::kLatchSeconds, 0) << " s x "
                   << juce::String (kSampleRate, 0) << " Hz) — "
                   << (onTime ? "on time" : (stoppedAt < 0 ? "NEVER STOPPED" : "WRONG TIME"));
        }

        check ("BT ping-latch-self-stop", ok, detail);
    }

    //==========================================================================
    // BU — Q5 / P60: the ping REFUSES to start on an invalid map, and a mapped -> not-mapped flip
    //      STOPS a running one.
    //
    //      Pinging "speaker 5" on a stereo fold names a speaker that does not exist, during the
    //      one procedure whose entire purpose is confirming that speaker N is speaker N — R1
    //      reproduced inside its own diagnostic tool. The two halves arrive at the same rule from
    //      opposite directions and BOTH are needed: the precondition runs on the message thread,
    //      where getTotalNumOutputChannels() lies under F3, and the flip is caught on the audio
    //      thread with the real buffer width in hand.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            // 1. REFUSES when the map is invalid.
            forceDuplicateLabel (proc);
            const bool invalid  = proc.isChannelMapInvalid();
            const bool refused  = ! proc.startVerifyPing (5);
            const bool notActive = ! proc.verifyPingState().active;

            // 2. And ACCEPTS once it is valid again — or "refuses" would be "never starts".
            oo::VenueModel fixed = proc.getVenue();
            fixed.setSpeakerLabel (5, oo::VenueModel().labelAbbreviation (5));
            proc.applyVenueEdit (fixed);

            const bool nowValid = ! proc.isChannelMapInvalid();
            const bool accepted = proc.startVerifyPing (5);

            juce::AudioBuffer<float> block (8, kBlockSize);
            juce::MidiBuffer midi;
            block.clear();
            proc.processBlock (block, midi);

            const bool running = proc.verifyPingState().active;

            // 3. THE FLIP. mapInvalid goes true WHILE the ping is running, between two blocks, with
            //    no intervening prepareToPlay — exactly the F3 window.
            forceDuplicateLabel (proc);
            block.clear();
            proc.processBlock (block, midi);

            const bool stopped = ! proc.verifyPingState().active;

            ok = invalid && refused && notActive && nowValid && accepted && running && stopped;

            detail << "invalid map: " << (refused ? "REFUSED" : "started anyway")
                   << "/" << (notActive ? "silent" : "ACTIVE")
                   << "; valid again: " << (accepted && running ? "accepted and running" : "REFUSED")
                   << "; mid-ping flip: " << (stopped ? "STOPPED" : "KEPT RUNNING");
        }

        check ("BU ping-refuses-invalid-map", ok, detail);
    }

    //==========================================================================
    // BW — FUNC-05/1: a preset load leaves all 50 venue values BIT-IDENTICAL.
    //
    //      The criterion holds BY CONSTRUCTION — applyPresetJson iterates
    //      processor.getParameters() and resolves via parameters.getParameter(id), so it can never
    //      walk apvts.state's children, where VENUE lives — and this measures it anyway. Section
    //      27 of the static gate carries the other half: setCustomStateCallbacks, the ONE path by
    //      which a preset could reach non-parameter state, is registered nowhere in this plugin.
    //
    //      savePresetToFile / loadPresetFromFile rather than the named pair: both funnel into the
    //      same createPresetJson / applyPresetJson bodies, and a temp file keeps the probe off the
    //      user's ~/Library.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            proc.applyVenueEdit (makeMeasuredVenue());
            const auto venueBefore = proc.getVenue();

            OuariconPresetManager presets (proc.getAPVTS(), "O-Octagon-probe");

            setParam (proc, "blur", 0.62f);
            setParam (proc, "rolloff", 5.25f);
            setParam (proc, "outputGain", -7.5f);

            const auto file = scratch32.getChildFile ("bw.json");
            file.deleteFile();
            const bool saved = presets.savePresetToFile (file);

            // Move every one of the three away, so a load that did nothing at all would fail the
            // positive control below.
            setParam (proc, "blur", 0.05f);
            setParam (proc, "rolloff", 3.10f);
            setParam (proc, "outputGain", 4.0f);

            const bool loaded = presets.loadPresetFromFile (file);

            int firstBad = -1;
            const bool venueIntact = sameFifty (venueBefore, proc.getVenue(), firstBad);

            const auto readParam = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
            };

            const bool paramsRestored = near (readParam ("blur"), 0.62f, 1.0e-3f)
                                     && near (readParam ("rolloff"), 5.25f, 1.0e-3f)
                                     && near (readParam ("outputGain"), -7.5f, 1.0e-2f);

            ok = saved && loaded && venueIntact && paramsRestored;

            detail << "save " << (saved ? "ok" : "FAILED") << ", load " << (loaded ? "ok" : "FAILED")
                   << ", 50 venue values " << (venueIntact ? "BIT-IDENTICAL" : "CHANGED")
                   << (firstBad >= 0 ? " (speaker " + juce::String (firstBad + 1) + ")" : "")
                   << ", parameters restored " << (paramsRestored ? "yes" : "NO — the load did nothing")
                   << " (blur " << juce::String (readParam ("blur"), 3) << ")";

            file.deleteFile();
        }

        check ("BW preset-load-leaves-venue", ok, detail);
    }

    //==========================================================================
    // BX — FUNC-05/2: a preset saved under venue A recalls under venue B, with srcX/srcY unchanged
    //      NORMALISED and the resolved metres following venue B's bbox.
    //
    //      This is the property that makes the venue/preset split worth having: the musical
    //      gesture is "70 % of the way across the rig", not "at 9.1 m", so the same preset means
    //      the same thing in a different hall.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            OuariconPresetManager presets (proc.getAPVTS(), "O-Octagon-probe");

            setParam (proc, "srcX", 0.3f);
            setParam (proc, "srcY", 0.7f);

            const auto metresA = proc.getVenue().normToMetres (0.3f, 0.7f);

            const auto file = scratch32.getChildFile ("bx.json");
            file.deleteFile();
            const bool saved = presets.savePresetToFile (file);

            // Venue B: a DIFFERENT bounding box, so the same normalised pair must resolve
            // somewhere else in metres.
            proc.applyVenueEdit (makeMeasuredVenue());
            const auto metresB = proc.getVenue().normToMetres (0.3f, 0.7f);

            setParam (proc, "srcX", 0.9f);
            setParam (proc, "srcY", 0.1f);

            const bool loaded = presets.loadPresetFromFile (file);

            const auto readNorm = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->getValue() : -1.0f;
            };

            const bool normSame = near (readNorm ("srcX"), 0.3f, 1.0e-3f)
                               && near (readNorm ("srcY"), 0.7f, 1.0e-3f);

            const bool metresMoved = ! near (metresA.x, metresB.x, 1.0e-3f)
                                  || ! near (metresA.y, metresB.y, 1.0e-3f);

            ok = saved && loaded && normSame && metresMoved;

            detail << "srcX/srcY normalised " << juce::String (readNorm ("srcX"), 4) << "/"
                   << juce::String (readNorm ("srcY"), 4) << " "
                   << (normSame ? "UNCHANGED" : "DRIFTED")
                   << "; metres A (" << juce::String (metresA.x, 2) << ", "
                   << juce::String (metresA.y, 2) << ") -> B ("
                   << juce::String (metresB.x, 2) << ", " << juce::String (metresB.y, 2) << ") "
                   << (metresMoved ? "followed venue B" : "DID NOT MOVE");

            file.deleteFile();
        }

        check ("BX preset-recalls-under-venue-b", ok, detail);
    }

    //==========================================================================
    // BY — FUNC-05/3: session state round-trips BOTH stores together.
    //
    //      N2 stands and is what this measures: O-Octagon keeps its OWN
    //      getStateInformation/setStateInformation. OuariconPresetManager::setStateFromXml calls
    //      replaceState() and nothing else, which would bypass §4.1's readVenueFromState() ->
    //      rebuildChannelMap() ordering — so the module is adopted for PRESETS only.
    {
        OOctagonProcessor source;
        bool         ok = negotiate (source, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            source.applyVenueEdit (makeMeasuredVenue());
            setParam (source, "blur", 0.44f);
            setParam (source, "airAmount", 0.81f);
            setParam (source, "w5", 0.25f);

            juce::MemoryBlock state;
            source.getStateInformation (state);

            OOctagonProcessor restored;
            ok = negotiate (restored, monoIn, set8);
            restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

            int firstBad = -1;
            const bool venueOk = sameFifty (source.getVenue(), restored.getVenue(), firstBad);

            int paramsBad = 0;
            for (int i = 0; i < static_cast<int> (oo::params::kCount); ++i)
            {
                auto* a = source.getAPVTS().getParameter (oo::params::id (i));
                auto* b = restored.getAPVTS().getParameter (oo::params::id (i));

                if (a == nullptr || b == nullptr || ! near (a->getValue(), b->getValue(), 1.0e-6f))
                    ++paramsBad;
            }

            // Not the default venue, or "identical" would be trivially true of two fresh
            // processors that never restored anything.
            int ignored = -1;
            const bool notDefault = ! sameFifty (oo::VenueModel(), restored.getVenue(), ignored);

            ok = ok && venueOk && paramsBad == 0 && notDefault && state.getSize() > 0;

            detail << state.getSize() << " bytes; 50 venue values "
                   << (venueOk ? "identical" : "DIFFER")
                   << (firstBad >= 0 ? " (speaker " + juce::String (firstBad + 1) + ")" : "")
                   << "; 17 parameters " << (paramsBad == 0 ? "identical" : juce::String (paramsBad) + " DIFFER")
                   << "; restored venue " << (notDefault ? "is the measured one" : "IS THE DEFAULT");
        }

        check ("BY session-roundtrips-both", ok, detail);
    }

    //==========================================================================
    // BZ — FUNC-02/3: the DBAP solve USES the entered values.
    //
    //      Driven through applyVenueEditChecked(), which is the function setVenue calls. Probe BL
    //      already drives applyVenueEdit directly and therefore does NOT exercise 3.2's path — the
    //      distinction matters because the guard sits between them and a guard that rejected
    //      everything would leave BL green.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            setParam (proc, "srcX", 0.35f);
            setParam (proc, "srcY", 0.55f);

            const auto before = measureGainVector (proc);

            oo::VenueModel moved = proc.getVenue();
            const auto p2 = moved.speaker (2);
            moved.setSpeakerPosition (2, { p2.x + 4.5f, p2.y - 2.0f, p2.z });

            ochan::MapDiagnosis why {};
            const bool applied = proc.applyVenueEditChecked (moved, &why);

            const auto after = measureGainVector (proc);
            const bool changed = vectorsDiffer (before, after, 1.0e-4f);

            float biggest = 0.0f;
            int   at = 0;
            for (int i = 0; i < 8; ++i)
            {
                const float d = std::abs (after[(std::size_t) i] - before[(std::size_t) i]);
                if (d > biggest) { biggest = d; at = i; }
            }

            ok = applied && changed;

            detail << "one coordinate moved through applyVenueEditChecked: "
                   << (applied ? "applied" : "REJECTED")
                   << ", gain vector " << (changed ? "CHANGED" : "IDENTICAL")
                   << " (largest delta " << juce::String (biggest, 6) << " at speaker " << (at + 1) << ")";
        }

        check ("BZ solve-uses-entered-values", ok, detail);
    }

    //==========================================================================
    //== PHASE 3.3 — CI-CN. Weight scenes (FUNC-06) and the eight meters (UI-03). ==============
    //==========================================================================
    //
    // CM IS THE OTHER PROBE THAT CARRIES THE PHASE. It is the ONLY assertion in 3.3 that a meter
    // driven by the SOLVE VECTOR rather than by the written buffer fails; every other meter probe
    // passes under that defect. (The first is CG, in the unit target: the only probe a fixed-index
    // scene implementation fails.)

    // Reads the eight weights back the way a HOST sees them — through the parameter objects, in
    // engineering units — rather than through any cached copy the plugin might hold. FUNC-06
    // criterion 1 says "read back host-side" and this is what that means.
    const auto readWeights = [] (OOctagonProcessor& p)
    {
        std::array<float, 8> w {};

        for (int i = 0; i < 8; ++i)
            if (auto* param = p.getAPVTS().getParameter (oo::params::id (oo::params::w1 + i)))
                w[(std::size_t) i] = param->convertFrom0to1 (param->getValue());

        return w;
    };

    //==========================================================================
    // CI — FUNC-06/1: EIGHT VALUES WRITTEN IN ONE GESTURE, AND EIGHT CLOSED BRACKET PAIRS.
    //
    //      MEASURED THROUGH REAL PARAMETER LISTENERS, not grepped. That is the whole reason
    //      applySceneWeights lives on the processor rather than at the editor's call site: this
    //      target never compiles PluginEditor.cpp, so on the editor the brackets could only ever
    //      have been asserted statically.
    //
    //      setValueNotifyingHost is setValue + sendValueChangedMessageToListeners AND NOTHING
    //      ELSE. Without begin/endChangeGesture, Logic with a lane in Latch or Touch MOVES THE
    //      SOUND AND DOES NOT RECORD IT — and build, auval and pluginval are all blind to it.
    //      This is the third and final site of that obligation.
    {
        struct GestureSpy : juce::AudioProcessorParameter::Listener
        {
            std::array<int, 8> starts {}, ends {}, values {};
            std::array<int, 8> paramIndex { -1, -1, -1, -1, -1, -1, -1, -1 };

            int slotFor (int hostIndex) const noexcept
            {
                for (int i = 0; i < 8; ++i)
                    if (paramIndex[(std::size_t) i] == hostIndex)
                        return i;

                return -1;
            }

            void parameterValueChanged (int hostIndex, float) override
            {
                if (const int s = slotFor (hostIndex); s >= 0)
                    ++values[(std::size_t) s];
            }

            void parameterGestureChanged (int hostIndex, bool starting) override
            {
                const int s = slotFor (hostIndex);

                if (s < 0)
                    return;

                if (starting) ++starts[(std::size_t) s];
                else          ++ends[(std::size_t) s];
            }
        };

        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            GestureSpy spy;
            std::array<juce::RangedAudioParameter*, 8> params {};

            for (int i = 0; i < 8; ++i)
            {
                params[(std::size_t) i] =
                    proc.getAPVTS().getParameter (oo::params::id (oo::params::w1 + i));

                if (params[(std::size_t) i] != nullptr)
                {
                    spy.paramIndex[(std::size_t) i] = params[(std::size_t) i]->getParameterIndex();
                    params[(std::size_t) i]->addListener (&spy);
                }
            }

            // A NON-UNIFORM set, and not the defaults: the eight weights all default to 1.0, so a
            // write of all-ones would be indistinguishable from no write at all.
            const std::array<float, 8> target { 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };

            proc.applySceneWeights (target);

            const auto after = readWeights (proc);

            int wrongValue = 0, unbracketed = 0, unclosed = 0, notWritten = 0;

            for (int i = 0; i < 8; ++i)
            {
                if (! near (after[(std::size_t) i], target[(std::size_t) i], 1.0e-4f)) ++wrongValue;
                if (spy.starts[(std::size_t) i] != 1)                                  ++unbracketed;
                if (spy.ends[(std::size_t) i] != spy.starts[(std::size_t) i])          ++unclosed;
                if (spy.values[(std::size_t) i] < 1)                                   ++notWritten;
            }

            for (auto* p : params)
                if (p != nullptr)
                    p->removeListener (&spy);

            ok = wrongValue == 0 && unbracketed == 0 && unclosed == 0 && notWritten == 0;

            detail << "8 host values: " << (wrongValue == 0 ? "all match" : juce::String (wrongValue) + " WRONG")
                   << "; begin gestures " << (unbracketed == 0 ? "8/8" : juce::String (8 - unbracketed) + "/8 — UNBRACKETED WRITES")
                   << "; unclosed " << unclosed
                   << "; valueChanged notifications " << (notWritten == 0 ? "8/8" : juce::String (8 - notWritten) + "/8");
        }

        check ("CI scene-writes-eight-bracketed", ok, detail);
    }

    //==========================================================================
    // CJ — FUNC-06/6: THE FADE GATE. The probe that catches an implementation that LATCHES.
    //
    //      Apply A, read 8; apply B, read 8; then write the MIDPOINT DIRECTLY to the parameters,
    //      bypassing the scene path entirely — and assert the render matches a processor that only
    //      ever saw the blend, AND that no subsequent block re-asserts A or B.
    //
    //      An implementation that stored "the current scene" and re-applied it in updateControl
    //      would pass every other scene probe in this file and FAIL HERE, on the last clause. One
    //      that only writes parameters passes by construction — which is the point: FUNC-06/6 asks
    //      for evidence that scenes are a WRITE and not a MODE.
    //
    //      THE REFERENCE IS A SECOND PROCESSOR, not a recomputed solve. Re-deriving the expected
    //      gains here would put a copy of the DBAP chain in the test file, over the highest-risk
    //      arithmetic in the plugin (pattern_test_fixture_mirrors_drift_silently).
    {
        OOctagonProcessor proc;
        OOctagonProcessor reference;

        bool         ok = negotiate (proc, monoIn, set8) && negotiate (reference, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            // Off-centre, so the eight lanes carry genuinely different gains and a blend is
            // distinguishable from either endpoint.
            for (auto* p : { &proc, &reference })
            {
                setParam (*p, "srcX", 0.38f);
                setParam (*p, "srcY", 0.62f);
            }

            const std::array<float, 8> sceneA { 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
            const std::array<float, 8> sceneB { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };

            proc.applySceneWeights (sceneA);
            const auto a = readWeights (proc);

            proc.applySceneWeights (sceneB);
            const auto b = readWeights (proc);

            // Step 3: THE MIDPOINT, WRITTEN DIRECTLY. Not through applySceneWeights — the whole
            // question is whether the plugin remembers having been given a scene.
            std::array<float, 8> blend {};

            for (int i = 0; i < 8; ++i)
            {
                blend[(std::size_t) i] = 0.5f * (a[(std::size_t) i] + b[(std::size_t) i]);
                setParam (proc, oo::params::id (oo::params::w1 + i), blend[(std::size_t) i]);
                setParam (reference, oo::params::id (oo::params::w1 + i), blend[(std::size_t) i]);
            }

            const auto got      = measureGainVector (proc);
            const auto expected = measureGainVector (reference);

            const bool matchesDirectSolve = ! vectorsDiffer (got, expected, 1.0e-5f);

            // Step 4's second clause. Render several more control blocks and re-read the HOST-SIDE
            // values: an implementation re-applying a stored scene would have overwritten them by
            // now, and the gains would have snapped back to A or B.
            juce::AudioBuffer<float> spin (8, kBlockSize);
            juce::MidiBuffer midi;

            for (int n = 0; n < 8; ++n)
            {
                spin.clear();
                proc.processBlock (spin, midi);
            }

            const auto held  = readWeights (proc);
            const auto after = measureGainVector (proc);

            int drifted = 0;
            for (int i = 0; i < 8; ++i)
                if (! near (held[(std::size_t) i], blend[(std::size_t) i], 1.0e-5f))
                    ++drifted;

            const bool stillBlend  = drifted == 0 && ! vectorsDiffer (got, after, 1.0e-6f);

            // NON-VACUITY: A and B must actually differ from the blend, or "no drift" is trivially
            // true and step 4 proves nothing.
            const bool endpointsDiffer = vectorsDiffer (a, blend, 1.0e-3f)
                                      && vectorsDiffer (b, blend, 1.0e-3f);

            ok = matchesDirectSolve && stillBlend && endpointsDiffer;

            detail << "blend vs a reference processor given only the blend: "
                   << (matchesDirectSolve ? "IDENTICAL" : "DIFFER")
                   << "; after 8 further blocks the parameters "
                   << (drifted == 0 ? "still hold the blend" : juce::String (drifted) + " RE-ASSERTED A SCENE")
                   << "; endpoints distinguishable from the blend: " << (endpointsDiffer ? "yes" : "NO — VACUOUS");
        }

        check ("CJ scene-fade-not-latched", ok, detail);
    }

    //==========================================================================
    // CK — FUNC-06/4: SCENES ROUND-TRIPS, AND A PRE-3.3 SESSION NORMALISES EXACTLY ONCE.
    //
    //      The round trip is STRUCTURAL — getStateInformation is apvts.copyState() -> XML and a
    //      SCENES child of apvts.state rides along with no new code (D17). What is NOT free is the
    //      NORMALISATION: without a write-back at setStateInformation, every session saved before
    //      Phase 3.3 restores with NO SCENES NODE AT ALL and the four slots read as ABSENT rather
    //      than EMPTY (N13). Silent, and only on upgrade.
    //
    //      So the pre-3.3 session is DRIVEN EXPLICITLY here — the SCENES element is stripped out of
    //      a real serialised state and fed back in — rather than argued about.
    {
        OOctagonProcessor source;
        bool         ok = negotiate (source, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            const std::array<float, 8> slot0 { 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.25f, 1.0f };
            const std::array<float, 8> slot2 { 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  1.0f };

            source.applySceneWeights (slot0);
            source.captureScene (0);
            source.applySceneWeights (slot2);
            source.captureScene (2);

            juce::MemoryBlock state;
            source.getStateInformation (state);

            // ── (a) THE ROUND TRIP ──
            OOctagonProcessor restored;
            negotiate (restored, monoIn, set8);
            restored.setStateInformation (state.getData(), (int) state.getSize());

            const auto& rs = restored.getScenes();

            int slotsBad = 0;

            for (int i = 0; i < 8; ++i)
            {
                if (! near (rs.weights (0)[(std::size_t) i], slot0[(std::size_t) i], 1.0e-4f)) ++slotsBad;
                if (! near (rs.weights (2)[(std::size_t) i], slot2[(std::size_t) i], 1.0e-4f)) ++slotsBad;
            }

            const bool occupancy = rs.isOccupied (0) && ! rs.isOccupied (1)
                                && rs.isOccupied (2) && ! rs.isOccupied (3);

            // ── (b) A PRE-3.3 SESSION: the SAME state with its SCENES element removed ──
            auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());

            bool hadScenes = false;
            juce::MemoryBlock pre33;

            if (xml != nullptr)
            {
                hadScenes = xml->getChildByName (oo::SceneStore::scenesTag.toString()) != nullptr;
                xml->deleteAllChildElementsWithTagName (oo::SceneStore::scenesTag.toString());
                juce::AudioProcessor::copyXmlToBinary (*xml, pre33);
            }

            OOctagonProcessor upgraded;
            negotiate (upgraded, monoIn, set8);
            upgraded.setStateInformation (pre33.getData(), (int) pre33.getSize());

            // EMPTY, NOT ABSENT, and not populated with invented weights either.
            int   ghosts = 0;
            for (int s = 0; s < oo::SceneStore::kNumSlots; ++s)
                if (upgraded.getScenes().isOccupied (s))
                    ++ghosts;

            // And the node is WRITTEN BACK, so the next save is self-describing.
            juce::MemoryBlock resaved;
            upgraded.getStateInformation (resaved);

            auto reXml = juce::AudioProcessor::getXmlFromBinary (resaved.getData(), (int) resaved.getSize());
            const bool nodeRestored = reXml != nullptr
                                   && reXml->getChildByName (oo::SceneStore::scenesTag.toString()) != nullptr;

            // The venue must be untouched by all of this — the two children are siblings and
            // neither read may disturb the other.
            const bool venueSurvived = reXml != nullptr
                                    && reXml->getChildByName (oo::VenueModel::venueTag.toString()) != nullptr;

            ok = slotsBad == 0 && occupancy && hadScenes && ghosts == 0
                 && nodeRestored && venueSurvived;

            detail << "round trip: " << (slotsBad == 0 ? "16/16 weights identical" : juce::String (slotsBad) + " DIFFER")
                   << ", occupancy " << (occupancy ? "{1,3} as stored" : "WRONG")
                   << "; the saved state carried SCENES: " << (hadScenes ? "yes" : "NO — THE STRIP WAS VACUOUS")
                   << "; pre-3.3 restore -> " << ghosts << " ghost slot(s), node "
                   << (nodeRestored ? "WRITTEN BACK" : "STILL ABSENT")
                   << ", VENUE " << (venueSurvived ? "intact" : "LOST");
        }

        check ("CK scenes-roundtrip-and-upgrade", ok, detail);
    }

    //==========================================================================
    // CL — FUNC-06/5: THE 42-VALUE BIT-COMPARE, RE-RUN NOW THAT THE SCENES NODE EXISTS.
    //
    //      RE-MEASURED, NOT INHERITED. Probe BW established FUNC-05 at 3.2 when
    //      setCustomStateCallbacks was registered NOWHERE — the guarantee then rested on the
    //      absence of any route from a preset to non-parameter state. Phase 3.3 registers the
    //      plugin's first and only such callback, so the TREE SHAPE THE GUARANTEE HOLDS OVER HAS
    //      CHANGED and inheriting BW's pass would be inheriting a claim about a different tree.
    //
    //      The callback is exercised HERE, live, rather than the probe merely noting it exists:
    //      the preset is saved and loaded through a manager with the SAME registration the editor
    //      makes, so if that callback could reach VENUE this probe would see it.
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            proc.applyVenueEdit (makeMeasuredVenue());
            const auto venueBefore = proc.getVenue();

            OuariconPresetManager presets (proc.getAPVTS(), "O-Octagon-probe");

            // THE SAME REGISTRATION PluginEditor.cpp MAKES, and it touches only SCENES.
            presets.setCustomStateCallbacks (
                [&proc] { return proc.scenesToVar(); },
                [&proc] (const juce::var& v) { proc.scenesFromVar (v); });

            const std::array<float, 8> stored { 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
            proc.applySceneWeights (stored);
            proc.captureScene (1);

            setParam (proc, "blur", 0.44f);

            const auto file = scratch32.getChildFile ("cl.json");
            file.deleteFile();
            const bool saved = presets.savePresetToFile (file);

            // Move the venue AND the musical parameter away, so a load that did nothing would fail
            // the positive control below and a load that did too much would fail the venue one.
            oo::VenueModel moved = proc.getVenue();
            const auto p3 = moved.speaker (3);
            moved.setSpeakerPosition (3, { p3.x + 2.5f, p3.y - 1.25f, p3.z + 0.4f });
            proc.applyVenueEdit (moved);

            setParam (proc, "blur", 0.05f);

            const bool loaded = presets.loadPresetFromFile (file);

            // THE 42, BIT-COMPARED. The venue the preset load left behind must be the one that was
            // live BEFORE the load — the MOVED one — and not the one that was live when the preset
            // was written.
            int firstBad = -1;
            const bool venueIsTheMovedOne = sameFifty (moved, proc.getVenue(), firstBad);

            int ignored = -1;
            const bool venueIsNotTheSaved = ! sameFifty (venueBefore, proc.getVenue(), ignored);

            auto* blurParam = proc.getAPVTS().getParameter ("blur");
            const bool paramRestored = blurParam != nullptr
                                    && near (blurParam->convertFrom0to1 (blurParam->getValue()), 0.44f, 1.0e-3f);

            // And the callback DID run — otherwise "the venue is intact" would be true of a load
            // that reached nothing at all, and the probe would pass while asserting nothing.
            const bool sceneRestored = proc.getScenes().isOccupied (1);

            ok = saved && loaded && venueIsTheMovedOne && venueIsNotTheSaved
                 && paramRestored && sceneRestored;

            detail << "save " << (saved ? "ok" : "FAILED") << ", load " << (loaded ? "ok" : "FAILED")
                   << "; 50 venue values " << (venueIsTheMovedOne ? "BIT-IDENTICAL to the live venue" : "CHANGED")
                   << (firstBad >= 0 ? " (speaker " + juce::String (firstBad + 1) + ")" : "")
                   << ", and demonstrably not the saved one: " << (venueIsNotTheSaved ? "yes" : "NO — VACUOUS")
                   << "; blur restored " << (paramRestored ? "yes" : "NO")
                   << "; the SCENES callback ran: " << (sceneRestored ? "yes" : "NO — THE PROBE TESTED NOTHING");

            file.deleteFile();
        }

        check ("CL func05-remeasured-with-scenes", ok, detail);
    }

    //==========================================================================
    // CM — UI-03/1,2: **THE PROBE THAT CARRIES THIS HALF OF THE PHASE.**
    //
    //      THE SPEAKER THAT LIGHTS IS THE SPEAKER THAT SOUNDS, on a NON-IDENTITY MAP, cross-checked
    //      against the verify ping stepping 1 -> 8.
    //
    //      A METER DRIVEN BY THE SOLVE VECTOR v_i FAILS HERE AND PASSES EVERYWHERE ELSE. §R7 names
    //      UI-03 a second human line of defence on R1 — the channel map — and R1's defining
    //      property is silence: a meter on v_i would light correctly under a BYPASSED map and
    //      report a working rig while the hall hears the wrong speakers. That is the exact NC3
    //      failure caught at Phase 2.2, and NC4 reproduces it here.
    //
    //      THE ROTATED LABELS ARE WHAT MAKE IT A TEST. All three accepted 8-channel containers have
    //      initializer order == enum-bit order, so the SHIPPED default map IS the identity and a
    //      probe driven by it would be byte-identical to one run against a hardcoded 0..7 (C1).
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            applyRotatedLabels (proc);

            // NON-VACUITY, FIRST AND EXPLICITLY: if the map were the identity, every assertion
            // below would hold for an implementation that ignored it entirely.
            int identityLanes = 0;
            for (int i = 0; i < 8; ++i)
                if (expectedLane (proc, set8, i) == i)
                    ++identityLanes;

            const bool mapIsPermuted = identityLanes == 0;

            juce::MidiBuffer midi;
            juce::AudioBuffer<float> block (8, kBlockSize);

            const auto renderSilentBlocks = [&] (int n)
            {
                for (int k = 0; k < n; ++k)
                {
                    block.clear();
                    proc.processBlock (block, midi);
                }
            };

            // Silence the programme material entirely, so the ONLY thing in the buffer is the
            // ping's post-write overwrite. The eight weights go to zero, which is DSP-05's exact
            // silence — v_i is then EXACTLY 0.0f on every speaker, and a v_i meter reads eight
            // zeros for the whole probe while the buffer plainly does not.
            proc.applySceneWeights ({ 0, 0, 0, 0, 0, 0, 0, 0 });
            renderSilentBlocks (6);
            (void) proc.readAndZeroMeters();

            int wrongIndex = 0, leaked = 0, silent = 0;
            juce::String walk;

            for (int speaker = 1; speaker <= 8; ++speaker)
            {
                proc.startVerifyPing (speaker);
                renderSilentBlocks (6);

                const auto peaks = proc.readAndZeroMeters();

                int   lit = 0, loudest = 0;
                float best = 0.0f;

                for (int i = 0; i < 8; ++i)
                {
                    if (peaks[(std::size_t) i] > 1.0e-6f)
                        ++lit;

                    if (peaks[(std::size_t) i] > best)
                    {
                        best = peaks[(std::size_t) i];
                        loudest = i;
                    }
                }

                if (loudest != speaker - 1)                         ++wrongIndex;
                if (lit != 1)                                       ++leaked;
                if (! (peaks[(std::size_t) (speaker - 1)] > 1.0e-6f)) ++silent;

                if (speaker <= 3)
                    walk << "spk" << speaker << "->meter" << (loudest + 1)
                         << "(buf " << expectedLane (proc, set8, speaker - 1) << ") ";

                proc.stopVerifyPing();
                renderSilentBlocks (6);
                (void) proc.readAndZeroMeters();
            }

            // exchange(0) really zeroes: a second read with nothing sounding must be silent, or
            // "the other seven read zero" above would be true of a meter that never decayed.
            renderSilentBlocks (4);
            const auto quiet = proc.readAndZeroMeters();

            int residue = 0;
            for (int i = 0; i < 8; ++i)
                if (quiet[(std::size_t) i] > 1.0e-6f)
                    ++residue;

            ok = mapIsPermuted && wrongIndex == 0 && leaked == 0 && silent == 0 && residue == 0;

            detail << "map is permuted: " << (mapIsPermuted ? "yes" : "NO — THIS PROBE IS VACUOUS")
                   << "; ping 1->8: " << walk
                   << "wrongIndex " << wrongIndex << ", leaked " << leaked << ", silent " << silent
                   << "; read-and-zero residue " << residue;
        }

        check ("CM meters-follow-the-map", ok, detail);
    }

    //==========================================================================
    // CN — UI-03/4: PROBE AO RE-RUN WITH METERING LIVE. PERF-01 IS A MEASUREMENT, NOT AN ARGUMENT.
    //
    //      buffer.getMagnitude() resolves to FloatVectorOperations::findMinAndMax on a raw pointer
    //      and allocates nothing — but the criterion says the allocation count must be re-measured
    //      with the loop in place, and AO's own comment is that a probe which counts nothing
    //      passes. The metering loop is unconditional in processBlock, so this IS AO's setup with
    //      eight more reads per block; what makes it a separate probe is the non-vacuity clause
    //      below, which proves the loop actually ran.
    {
        OOctagonProcessor proc;
        negotiate (proc, monoIn, set8);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.42f);

        juce::MidiBuffer midi;
        juce::AudioBuffer<float> block (8, kBlockSize);

        const auto renderOne = [&] (int n)
        {
            block.clear();

            for (int s = 0; s < kBlockSize; ++s)
                block.setSample (0, s, noiseAt (n * kBlockSize + s));

            proc.processBlock (block, midi);
        };

        renderOne (0);                                   // warm-up, NOT counted
        (void) proc.readAndZeroMeters();

        rtcheck::arm();

        for (int n = 1; n <= 64; ++n)
            renderOne (n);

        // The over-size block pluginval issues at strictness 10 — the case a naive metering
        // implementation would handle by allocating a scratch buffer.
        {
            juce::AudioBuffer<float> big (8, 8192);
            big.clear();
            proc.processBlock (big, midi);
        }

        rtcheck::disarm();

        const long long counted = proc.readAndZeroMeters().size() > 0 ? rtcheck::allocations.load() : -1;

        // NON-VACUITY: the meters must have SEEN the render. A zero here would mean the loop never
        // ran, and "0 allocations" would be a statement about code that was not executing.
        rtcheck::arm (false);
        renderOne (99);
        rtcheck::disarm();

        const auto peaks = proc.readAndZeroMeters();
        const long long secondCount = rtcheck::allocations.load();

        int metered = 0;
        for (int i = 0; i < 8; ++i)
            if (peaks[(std::size_t) i] > 1.0e-6f)
                ++metered;

        const bool clean = counted == 0 && secondCount == 0;
        const bool live  = metered > 0;

        check ("CN no-allocation-with-metering", clean && live,
               juce::String (static_cast<int> (counted))
                   + " allocation(s) across 65 processBlock calls incl. an 8192-sample over-size "
                     "block, with the eight meters live; "
                   + juce::String (metered) + "/8 meters registered the render"
                   + rtcheck::foreignNote()
                   + (live ? "" : " — THE METERING LOOP DID NOT RUN, THIS PROBE IS VACUOUS"));
    }

    // ══════════════════════════════════════════════════════════════════════════════════════════
    // PHASE 4.1 — CP, CQ.
    // ══════════════════════════════════════════════════════════════════════════════════════════

    //==========================================================================
    // CP — N5's fix: a factory preset load leaves THE TWELVE BIT-UNCHANGED.
    //
    //      ── WHY THE OBVIOUS PROBE IS VACUOUS ────────────────────────────────────────────────
    //      "Assert the six moved to the preset's values" PASSES WITH THE BUG PRESENT. WR-01 resets
    //      all eighteen to their defaults before applying anything, so the six arrive correctly
    //      either way; what the bug does is silently take the OTHER twelve — the source position,
    //      v1.5.0's decorr, and the eight scene weights — down to their defaults with them.
    //      Clause 3's SECOND half
    //      is the probe. NC2 (delete the restore) fails that half while the six-changed half still
    //      passes, which is how that is known rather than asserted.
    //
    //      ── AND IT MUST NOT USE "Concert Default" ───────────────────────────────────────────
    //      That preset's six values ARE the shipped defaults — that is its entire point — so after
    //      WR-01's reset the six read correct EVEN IF THE APPLY IS STUBBED OUT ENTIRELY. A probe
    //      that cannot distinguish "applied" from "reset to defaults" is vacuous, and it is the one
    //      preset in the set that guarantees it. DISTANT FIELD is used instead: all six of its
    //      values differ from default, in both directions. NC3 demonstrates the difference.
    //
    //      ── HERMETIC BY CONSTRUCTION (N7) ───────────────────────────────────────────────────
    //      initializeFactoryPresets returns EARLY when Factory/.factory-version already matches
    //      JucePlugin_VersionString, and O-Octagon ships 1.0.0 and stays there. A probe that read
    //      whatever JSON happened to be on disk could therefore be reading a file from an earlier
    //      authoring iteration AND PASS (NC4 demonstrates exactly that). So this probe deletes the
    //      store and regenerates it from oo::presets::factoryDefs itself.
    //
    //      The manager is scoped to a PROBE-ONLY plugin name, so the tree it touches is
    //      ~/Library/O-Octagon-CP-probe/ and the shipping ~/Library/O-Octagon/Presets/User/ is not
    //      merely left alone — it is unreachable from here (gate 17).
    {
        OOctagonProcessor proc;
        bool         ok = negotiate (proc, monoIn, set8);
        juce::String detail;

        if (ok)
        {
            const juce::String probePlugin { "O-Octagon-CP-probe" };

            OuariconPresetManager presets (proc.getAPVTS(), probePlugin);

            // Hermetic: nothing from a previous run survives into this one.
            presets.getPresetsDirectory().getParentDirectory().deleteRecursively();
            presets.initializeFactoryPresets (oo::presets::factoryDefs (proc.getAPVTS()));

            // The twelve are given values the WR-01 reset would visibly destroy: a source well
            // off centre and lifted, decorrelation dialled up, and a FRONT-like scene rather than
            // the all-1.0 default.
            setParam (proc, "srcX", 0.17f);
            setParam (proc, "srcY", 0.83f);
            setParam (proc, "srcZ", 3.4f);
            //
            // v1.5.0. Off its 0.0f default for the reason the weights are off 1.0f: a preserved
            // parameter sitting at its default is one the WR-01 reset cannot be OBSERVED to move,
            // and the liveness gate below is what turns that from a comment into a failure. It
            // fired on this exact parameter the moment decorr joined kPreserved.
            setParam (proc, "decorr", 0.55f);
            //
            // NOT 1.0f ANYWHERE. 1.0 is the weight default, and a weight sitting at its default is
            // one the WR-01 reset cannot be observed to move — the liveness gate below caught
            // exactly that when this line began with 1.0f.
            setWeights (proc, { 0.95f, 0.85f, 0.7f, 0.0f, 0.0f, 0.0f, 0.25f, 0.9f });

            const auto readNorm = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->getValue() : -1.0f;
            };

            const auto defaultNorm = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->getDefaultValue() : -1.0f;
            };

            // Captured NORMALISED, which is the representation loadPreserving restores in — so
            // "unchanged" below is an exact float compare and not a tolerance.
            std::array<float, oo::presets::kPreserved.size()> before {};

            for (std::size_t i = 0; i < oo::presets::kPreserved.size(); ++i)
                before[i] = readNorm (oo::presets::kPreserved[i]);

            // THE LIVENESS GATE. Every one of the twelve must currently sit AWAY from its default,
            // or "bit-unchanged after a reset-to-defaults" is a claim about nothing.
            int atDefault = 0;
            for (std::size_t i = 0; i < oo::presets::kPreserved.size(); ++i)
                if (bitExact (before[i], defaultNorm (oo::presets::kPreserved[i])))
                    ++atDefault;

            const bool armed = atDefault == 0;

            const bool loaded = oo::presets::loadPreserving (presets, proc.getAPVTS(), "Distant Field");

            // Clause 3a — the six moved to the preset's ENGINEERING values. Passes with the bug
            // present; here for coverage, and as NC2's control.
            const auto readEng = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
            };

            const bool sixApplied = near (readEng ("width"),      4.5f,  1.0e-3f)
                                 && near (readEng ("rolloff"),    3.0f,  1.0e-3f)
                                 && near (readEng ("blur"),       0.18f, 1.0e-3f)   // v1.3.0 rescale (was 0.55)
                                 && near (readEng ("hullAtten"),  0.4f,  1.0e-3f)
                                 && near (readEng ("airAmount"),  0.85f, 1.0e-3f)
                                 && near (readEng ("outputGain"), -3.0f, 1.0e-2f);

            // Clause 3b — THE PROBE. Bit-exact, not near().
            int         moved     = 0;
            const char* firstMoved = nullptr;

            for (std::size_t i = 0; i < oo::presets::kPreserved.size(); ++i)
            {
                if (! bitExact (readNorm (oo::presets::kPreserved[i]), before[i]))
                {
                    ++moved;

                    if (firstMoved == nullptr)
                        firstMoved = oo::presets::kPreserved[i];
                }
            }

            const bool twelveHeld = moved == 0;

            ok = loaded && armed && sixApplied && twelveHeld;

            detail << "load " << (loaded ? "ok" : "FAILED")
                   << "; the SIX -> width " << juce::String (readEng ("width"), 2)
                   << " rolloff " << juce::String (readEng ("rolloff"), 2)
                   << " blur " << juce::String (readEng ("blur"), 2)
                   << " hullAtten " << juce::String (readEng ("hullAtten"), 2)
                   << " air " << juce::String (readEng ("airAmount"), 2)
                   << " gain " << juce::String (readEng ("outputGain"), 2)
                   << (sixApplied ? " (applied)" : " — DID NOT MATCH Distant Field")
                   << "; the TWELVE " << (twelveHeld ? "BIT-UNCHANGED" : "MOVED")
                   << (firstMoved != nullptr ? juce::String (" (first: ") + firstMoved
                                                   + " -> reset to default)"
                                             : juce::String())
                   << "; armed ";

            // Built with << onto the named local rather than juce::String("...") + ... : that
            // constructor is ASCII-ONLY and silently mangles the em-dash
            // (critical_juce_string_char_ctor_is_ascii_only). It did, and this is the fix.
            if (armed)
                detail << "yes (11/11 away from default)";
            else
                detail << "NO — " << atDefault
                       << " of 12 SAT AT THEIR DEFAULT, THIS PROBE IS VACUOUS";

            presets.getPresetsDirectory().getParentDirectory().deleteRecursively();
        }

        check ("CP preset-load-preserves-the-twelve", ok, detail);
    }

    //==========================================================================
    // CQ — COMPAT-04 criterion 2's RENDER clause: instantiation on a 2-channel output is
    //      non-destructive, and the SAFE fold produces finite samples.
    //
    //      Criterion 2's first half ("defined and non-destructive") closed at Stage 1 on
    //      instantiation alone. Nothing had yet pushed full-scale signal THROUGH the fold with the
    //      parameters at their range extremes, which is where a divide-by-a-collapsed-denominator
    //      would surface as NaN rather than as a refusal to load.
    //
    //      BOTH SAFE outputs are driven — stereo and mono. They are different fold widths, and the
    //      mono one is the narrower of the two.
    {
        struct Case { juce::AudioChannelSet out; const char* label; };

        const Case cases[] = {
            { juce::AudioChannelSet::stereo(), "mono-in/stereo-out" },
            { juce::AudioChannelSet::mono(),   "mono-in/mono-out"   },
        };

        // Range extremes, taken off the LIVE ranges rather than transcribed — the values that
        // maximise the trim, the blur radius and the output gain all at once.
        struct Extreme { const char* id; bool high; };

        const Extreme extremes[] = {
            { "srcX", false }, { "srcY", true }, { "srcZ", true }, { "width", true },
            { "rolloff", true }, { "blur", true },
            { "hullAtten", true }, { "airAmount", true },
            { "outputGain", true },
        };

        bool         ok = true;
        juce::String detail;

        for (const auto& c : cases)
        {
            OOctagonProcessor proc;

            if (! negotiate (proc, monoIn, c.out))
            {
                ok = false;
                detail << c.label << ":REJECTED ";
                continue;
            }

            if (! proc.isSafeMode())
            {
                ok = false;
                detail << c.label << ":NOT-SAFE(so this is not the fold) ";
                continue;
            }

            for (const auto& e : extremes)
            {
                const auto& r = proc.getAPVTS().getParameterRange (e.id);
                setParam (proc, e.id, e.high ? r.end : r.start);
            }

            // Weights at both rails at once — an all-zero subset is DSP-05's silence path and a
            // full-scale subset is the loudest the fold can be asked for.
            setWeights (proc, { 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f });

            juce::AudioBuffer<float> buffer (juce::jmax (1, c.out.size()), kBlockSize);
            juce::MidiBuffer         midi;

            bool  finite = true;
            float peak   = 0.0f;

            for (int block = 0; block < 8; ++block)
            {
                buffer.clear();

                // FULL SCALE, not the 0.5 test signal — the input the fold is least likely to
                // survive.
                for (int n = 0; n < kBlockSize; ++n)
                    buffer.setSample (0, n, std::sin (static_cast<float> (block * kBlockSize + n) * 0.0731f));

                proc.processBlock (buffer, midi);

                if (! allFinite (buffer))
                    finite = false;

                peak = juce::jmax (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
            }

            if (! finite)
                ok = false;

            detail << c.label << ":" << (finite ? "finite" : "NaN/Inf")
                   << "(peak " << juce::String (peak, 3) << ") ";
        }

        check ("CQ safe-fold-render-is-finite", ok, detail);
    }

    //==========================================================================
    // CR — setStateInformation() PUBLISHES EXACTLY ONCE (CODE_REVIEW WR-01).
    //
    //      VenueSnapshotPublisher is a 2-slot double buffer: publish() writes `1 - activeSlot`
    //      with no knowledge of which slot a READER holds, and processBlock() binds the active
    //      slot BY REFERENCE once and holds it for the whole callback. ONE publish inside that
    //      window is safe — it writes the other slot. TWO are not: the second computes
    //      `1 - (the slot the first just activated)` and lands in the slot the audio thread is
    //      reading. A data race on ~276 bytes of non-atomic floats and ints, reachable whenever a
    //      host restores a session or switches a preset with the transport rolling.
    //
    //      Until v1.3.2 setStateInformation() did exactly that: readVenueFromState() published,
    //      and rebuildChannelMap() published again microseconds later.
    //
    //      THE PROPERTY IS A COUNT, so this probe counts it. getVenueGeneration() advances by one
    //      per publish, which makes the count observable from outside without instrumenting the
    //      publisher. An attribute check — grepping for `publish=false` — would pass with the
    //      argument threaded to a call site that never runs.
    //
    //      Both branches are driven, because the suppression is CONDITIONAL and getting that
    //      condition wrong is the failure mode the fix could plausibly introduce:
    //
    //        prepared    -> rebuildChannelMap() runs, so readVenueFromState() must NOT publish.
    //                       Expect exactly +1.
    //        unprepared  -> rebuildChannelMap() is skipped (`if (preparedYet)`), so
    //                       readVenueFromState() MUST publish or the restored geometry never
    //                       reaches the audio thread. Expect exactly +1 here too — the same
    //                       number, for the opposite reason, which is why the second clause below
    //                       also checks the venue actually ARRIVED.
    {
        juce::MemoryBlock state;

        {
            OOctagonProcessor source;
            if (negotiate (source, monoIn, set8))
            {
                source.applyVenueEdit (makeMeasuredVenue());
                source.getStateInformation (state);
            }
        }

        bool         ok = state.getSize() > 0;
        juce::String detail;

        if (! ok)
        {
            detail << "NO STATE CAPTURED — vacuous";
        }
        else
        {
            // Branch 1 — PREPARED. The reachable racy path.
            {
                OOctagonProcessor prepared;
                const bool ready = negotiate (prepared, monoIn, set8);

                const auto before = prepared.getVenueGeneration();
                prepared.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
                const auto delta = prepared.getVenueGeneration() - before;

                int ignored = -1;
                const bool arrived = ! sameFifty (oo::VenueModel(), prepared.getVenue(), ignored);

                ok = ok && ready && delta == 1u && arrived;
                detail << "prepared: " << juce::String (static_cast<int> (delta)) << " publish"
                       << (delta == 1u ? "" : " — RACY") << ", venue "
                       << (arrived ? "arrived; " : "DID NOT ARRIVE; ");
            }

            // Branch 2 — UNPREPARED. A host that restores before prepareToPlay(). The rebuild is
            // skipped here, so an UNCONDITIONAL suppression would publish ZERO times and leave the
            // audio thread on the default geometry.
            {
                OOctagonProcessor unprepared;

                const auto before = unprepared.getVenueGeneration();
                unprepared.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
                const auto delta = unprepared.getVenueGeneration() - before;

                int ignored = -1;
                const bool arrived = ! sameFifty (oo::VenueModel(), unprepared.getVenue(), ignored);

                ok = ok && delta == 1u && arrived;
                detail << "unprepared: " << juce::String (static_cast<int> (delta)) << " publish"
                       << (delta == 1u ? "" : " — SUPPRESSED TOO FAR") << ", venue "
                       << (arrived ? "arrived" : "DID NOT ARRIVE");
            }
        }

        check ("CR setstate-publishes-once", ok, detail);
    }

    //==========================================================================

    std::printf ("\n  ── v1.4.0 (Per-Speaker Alignment Delay) ──────────────────────────\n");

    //==========================================================================
    // CS — THE DELAY MOVES EXACTLY ONE LANE, BY EXACTLY THE RIGHT NUMBER OF SAMPLES.
    //
    // ── WHY BOTH HALVES ARE IN ONE PROBE ──────────────────────────────────────────────────────
    //
    // The two claims v1.4.0 makes are "a zero delay changes nothing" and "a nonzero delay shifts
    // that lane". Written as separate probes, the first PASSES WITH THE FEATURE DELETED — it is a
    // claim about an absence, and a build with no delay code at all satisfies it perfectly
    // (pattern_probe_must_target_the_branch_the_fix_changed). Together they cannot both hold on
    // any implementation but the right one: seven lanes bit-identical proves the zero path is
    // untouched IN THIS BUILD, and the eighth shifted by a measured 480 samples proves the code
    // that leaves it untouched is the same code that can move it.
    //
    // ── THE DELAY IS APPLIED BEFORE negotiate() ───────────────────────────────────────────────
    //
    // applyVenueEdit() writes through to apvts.state, and prepareToPlay() reads the venue back
    // from there — so a delay set first arrives via prepare()'s TELEPORT rather than via a 5 ms
    // ramp. That is what makes the shift exact from sample 0 and the comparison a bit-compare
    // instead of a tolerance. A probe that edited mid-render would be measuring the ramp.
    //
    // ── NO ASSUMPTION ABOUT WHICH BUFFER CHANNEL A SPEAKER REACHES ────────────────────────────
    //
    // The probe never indexes speakerToBuffer. It delays ONE speaker and then asks which lanes
    // moved; "exactly one" is a stronger statement than "lane 3 moved" and it is true under any
    // map, which is what lets this run under the rotated labels the routing probes use.
    {
        constexpr int   kSpeaker = 3;
        constexpr float kDelayMs = 10.0f;
        constexpr int   total    = 4096 * 3;

        // 10 ms at 48 kHz is 480 samples EXACTLY, and it stays exact through the float conversion
        // GainStage performs (10.0f * 0.001f * 48000.0f rounds to 480.0f), so delayFrac is 0 and
        // Linear interpolation returns the stored sample unchanged. Derived here rather than
        // written as 480 so the constant follows kSampleRate if that ever moves.
        const int expectedShift = static_cast<int> (kDelayMs * 0.001 * kSampleRate);

        OOctagonProcessor a, b;

        // b's delay goes on BEFORE prepareToPlay — see above.
        {
            oo::VenueModel v = b.getVenue();
            v.setSpeakerDelayMs (kSpeaker, kDelayMs);
            b.applyVenueEdit (v);
        }

        const bool negotiated = negotiate (a, mono, set71) && negotiate (b, mono, set71);

        applyRotatedLabels (a);
        applyRotatedLabels (b);

        // Off the rig's mirror axis, for AI's reason: at the default srcX four speaker pairs
        // receive identical gains and "exactly one lane moved" would be ambiguous.
        for (auto* p : { &a, &b })
        {
            setParam (*p, "srcX", 0.18f);
            setParam (*p, "srcY", 0.72f);
        }

        juce::AudioBuffer<float> outA (8, total), outB (8, total);

        renderInto (a, outA, total, { 512 }, {});
        renderInto (b, outB, total, { 512 }, {});

        // ── 1. EXACTLY ONE LANE DIFFERS ──
        int  moved = 0, movedLane = -1;

        for (int ch = 0; ch < 8; ++ch)
        {
            bool same = true;

            for (int n = 0; n < total && same; ++n)
                same = bitExact (outA.getSample (ch, n), outB.getSample (ch, n));

            if (! same) { ++moved; movedLane = ch; }
        }

        const bool exactlyOne = moved == 1;

        // ── 2. THE MOVED LANE IS THE UNDELAYED ONE, SHIFTED, BIT-EXACTLY ──
        bool shifted = exactlyOne;

        if (exactlyOne)
            for (int n = expectedShift; n < total && shifted; ++n)
                shifted = bitExact (outB.getSample (movedLane, n),
                                    outA.getSample (movedLane, n - expectedShift));

        // ── 3. AND ITS FIRST expectedShift SAMPLES ARE EXACTLY SILENT ──
        //
        // The line is reset() on the engage edge, so what comes out before the signal arrives is
        // zero rather than whatever the buffer last held. Bit-exact zero, not "small".
        bool silentHead = exactlyOne;

        if (exactlyOne)
            for (int n = 0; n < expectedShift && silentHead; ++n)
                silentHead = bitExact (outB.getSample (movedLane, n), 0.0f);

        // ── 4. NON-VACUITY ──
        //
        // If the moved lane were silent in BOTH renders, claims 2 and 3 would hold trivially. And
        // if expectedShift were 0 the shift comparison would degenerate into claim 1's negation.
        const bool live = exactlyOne
                          && outA.getMagnitude (movedLane, 0, total) > 1.0e-4f
                          && expectedShift > 0;

        const bool ok = negotiated && exactlyOne && shifted && silentHead && live;

        juce::String detail;
        detail << "delayed speaker " << kSpeaker << " by " << juce::String (kDelayMs, 1)
               << " ms (" << expectedShift << " samples): " << moved << " lane(s) moved";

        if (exactlyOne)
            detail << " (lane " << movedLane << "), shift "
                   << (shifted ? "bit-exact" : "WRONG OR INEXACT")
                   << ", head " << (silentHead ? "exactly silent" : "NOT ZERO")
                   << ", source lane peak "
                   << juce::String (outA.getMagnitude (movedLane, 0, total), 6);
        else
            detail << " — expected exactly 1"
                   << (moved == 0 ? " (THE DELAY DID NOTHING)" : " (IT LEAKED ACROSS LANES)");

        check ("CS delay-shifts-one-lane-exactly", ok, detail);
    }

    //==========================================================================
    // CT — QUAL-03 STILL HOLDS WITH THE DELAY LINES CLOCKING.
    //
    // AL and AM prove block-size invariance for the seventeen gain smoothers. v1.4.0 adds eight
    // more smoothers AND eight stateful delay lines, and the delay lines are the sharper risk:
    // their read position is driven by a ramp, so a ramp that advanced per BLOCK instead of per
    // SAMPLE would slide the read pointer by a different amount at 512 than at 4096 and the two
    // renders would diverge (pattern_block_rate_envelope_breaks_blocksize_invariance).
    //
    // Deliberately AL's shape, with events at multiples of 4096 and a memcmp, so a reader can see
    // that the only variable added is the delay.
    {
        constexpr int total = 4096 * 6;

        const std::vector<Event> events
            { { 4096 * 1, "srcX", 0.20f }, { 4096 * 2, "srcY", 0.85f },
              { 4096 * 3, "w3",   0.10f }, { 4096 * 4, "rolloff", 5.5f },
              { 4096 * 5, "srcZ", 3.25f } };

        OOctagonProcessor a, b;

        // ALL EIGHT delayed, and all eight DIFFERENTLY — a single shared value would be invariant
        // to a bug that mixed up which lane read which ramp.
        for (auto* p : { &a, &b })
        {
            oo::VenueModel v = p->getVenue();

            for (int i = 0; i < oo::VenueModel::kNumSpeakers; ++i)
                v.setSpeakerDelayMs (i, 1.5f + static_cast<float> (i) * 3.25f);

            p->applyVenueEdit (v);
        }

        negotiate (a, mono, set71);
        negotiate (b, mono, set71);
        applyRotatedLabels (a);
        applyRotatedLabels (b);

        juce::AudioBuffer<float> outA (8, total), outB (8, total);

        renderInto (a, outA, total, { 512 },  events);
        renderInto (b, outB, total, { 4096 }, events);

        const bool identical = bitIdentical (outA, outB);

        // NON-VACUITY, IN THE ONE PLACE IT COULD HIDE: two silent renders are bit-identical. The
        // longest delay here is 24.25 ms — about 1164 samples — so a lane checked only over its
        // first block would look silent for a reason that has nothing to do with a bug.
        const bool live = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("CT blocksize-invariance-with-delay", identical && live,
               juce::String (identical
                                 ? "8 lanes delayed 1.50..24.25 ms: bit-identical by memcmp over "
                                   "24576 samples x 8 lanes"
                                 : firstDifference (outA, outB))
                   + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // CU — v1.5.0's BIT-IDENTITY ANCHOR, CAPTURED FROM THE v1.4.0 BINARY.
    //
    // v1.5.0 adds an 18th parameter and a stateful all-pass network on the two sub-point feeds.
    // The feature's whole compatibility claim is that at `decorr` = 0 NOTHING CHANGES — not
    // "changes below audibility", not "changes only in the last bit". This probe is that claim,
    // and it is the one assertion in the feature that CANNOT be made by comparing two buffers
    // rendered in one process: the reference lives in the previous binary.
    //
    // ── THE SCENARIO IS CHOSEN TO BE WHERE THE DECORRELATOR WOULD ENGAGE ──────────────────────
    // A digest taken at width = 0 would be worthless: the decorrelator is gated on wEff and does
    // not run there, so it would pass with the gate wired backwards. The puck is OFF-CENTRE (so
    // the rFade collapse does not fire and wEff is the full dialled width), width is 6 m, and air
    // is up — every condition the decorrelator needs, with only the parameter itself at zero.
    //
    // ── RE-ANCHORING ──────────────────────────────────────────────────────────────────────────
    // If a LATER version deliberately changes this render, the constant is re-derived by running
    // the previous release's harness and transcribing the number, WITH the changelog entry that
    // says why. It is never re-recorded from the failing build — that turns the one probe that
    // watches for silent drift into a probe that ratifies it.
    {
        constexpr int total = 4096 * 4;

        // v1.4.0 CAPTURE, 2026-08-26, from commit 0c7154f2 (the v1.4.0 release build), rendered
        // by this exact scenario before a line of v1.5.0's DSP existed.
        constexpr std::uint64_t kV140Digest = 0xe25f022c8ce71dc9ull;

        const std::vector<Event> events
            { { 4096 * 1, "srcX",    0.72f },
              { 4096 * 2, "blur",    0.22f },
              { 4096 * 3, "rolloff", 5.0f } };

        OOctagonProcessor proc;

        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);

        // ARMED AFTER negotiate(), so prepareToPlay()'s own work is not counted.
        oo::instr::resetCounters();

        // OFF-CENTRE AND WIDE — see the note above. srcY moves too, so the spread axis is not the
        // degenerate fore-aft fallback either.
        setParam (proc, "srcX",      0.28f);
        setParam (proc, "srcY",      0.66f);
        setParam (proc, "srcZ",      1.40f);
        setParam (proc, "width",     6.0f);
        setParam (proc, "airAmount", 0.60f);
        setParam (proc, "hullAtten", 1.60f);
        setWeights (proc, { 1.0f, 0.85f, 0.6f, 1.0f, 0.4f, 0.9f, 1.0f, 0.75f });

        juce::AudioBuffer<float> out (8, total);
        renderInto (proc, out, total, { 4096 }, events);

        const auto digest = bufferDigest (out);

        // THE SECOND HALF OF THE CLAIM, AND IT IS NOT THE SAME HALF. The digest says the audio is
        // v1.4.0's audio; this says the network never executed. Either alone leaves a hole — a
        // decorrelator that ran and happened to be transparent would pass the digest, and a
        // counter that stayed at zero because the render was silent would pass this.
        const auto decorrRan = oo::instr::get (oo::instr::decorrSamples);

        // NON-VACUITY. A silent render has a digest too, and it would be a stable one.
        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

        const bool ok = digest == kV140Digest && decorrRan == 0 && live;

        juce::String detail;
        detail << "width 6 m off-centre, air 0.60, 4 x 4096 samples x 8 lanes: digest 0x"
               << juce::String::toHexString (static_cast<juce::int64> (digest))
               << " vs v1.4.0 0x" << juce::String::toHexString (static_cast<juce::int64> (kV140Digest))
               << "; decorrSamples " << juce::String (decorrRan) << " (expect 0)"
               << (live ? "" : " — SIGNAL IS SILENT, probe vacuous");

        check ("CU decorr-zero-matches-v1.4.0", ok, detail);
    }

    //==========================================================================
    // CV — THE DECORRELATOR ITSELF: ALL-PASS AT EVERY DEPTH, DECORRELATING, CONVERGENT AT ZERO.
    //
    // Driven DIRECTLY rather than through a render, because the claims Decorrelator.h makes are
    // claims about the network and a render can only show their consequences.
    //
    // ── THE SWEEP IS THE PROBE, AND THE FIRST DRAFT OF THIS PROBE PROVED IT ───────────────────
    //
    // This originally measured depth 1.0 ONLY, and it passed against an implementation that was
    // 3 to 4.6 dB down at every other depth. The bases are integers, so depth 1.0 is the one
    // value in the range where a fractional read needs no interpolation — the single point at
    // which the broken code was correct. The defect was found by measuring the SUMMED level
    // (-6.94 dB against a coherent sum, where incoherent addition predicts -3.01), not by this
    // probe, which is exactly the shape of pattern_test_fixture_mirrors_drift_silently: a fixture
    // that samples the one operating point where the bug is invisible.
    //
    // So clause 1 now sweeps NINE depths spanning integer, half-sample and quarter-sample
    // positions of the shortest base, and asserts the gain at every one. Restore the fractional
    // read and this fails at eight of them.
    {
        constexpr double sr      = 48000.0;
        constexpr int    measure = 65536;

        /*  ── THE WARM-UP IS 32768 AND THE FIRST DRAFT'S 8192 WAS NOT ENOUGH ───────────────────
            Filling the rings is not the constraint — the longest chain holds 1252 samples at
            depth 1. SETTLING is: each section's DC state converges to 1/(1-g) geometrically at
            g per round trip, so reaching 1e-6 needs ~40 trips of a 449-sample section, and the
            four sections settle IN SERIES because each one's input is the previous one's output.

            Measured rather than reasoned, because the two are easy to confuse here: at 8192 the
            settled DC reads 0.93180668 and a depth sweep swings it by 6.8e-2, which looks
            exactly like a real artefact. At 32768 it reads 0.99999970 and the sweep swings it by
            3.0e-7, and it does not improve at 524288 — so 32768 is settled, not merely closer.
            A tolerance loosened to accommodate the first number would have certified a
            decorrelator that thumps.                                                            */
        constexpr int warmUp = 32768;

        // Chosen so that base 113 x depth lands on integers (1.000), halves (0.500), quarters
        // (0.250, 0.750) and assorted thirds/eighths — every interpolation phase a fractional
        // read would have had to handle.
        const std::array<float, 9> depths
            { 1.0f, 0.9f, 0.75f, 0.5f, 0.375f, 0.25f, 0.125f, 0.05f, 0.01f };

        oo::Decorrelator dL, dR;

        const auto runPair = [&] (float depth, std::vector<float>& outL, std::vector<float>& outR)
        {
            dL.prepare (sr, oo::decorr::kBasesLeft);
            dR.prepare (sr, oo::decorr::kBasesRight);

            for (int n = 0; n < warmUp; ++n)
            {
                dL.process (noiseAt (n), depth);
                dR.process (noiseAt (n), depth);
            }

            outL.resize (measure);
            outR.resize (measure);

            for (int n = 0; n < measure; ++n)
            {
                const float x = noiseAt (warmUp + n);
                outL[(std::size_t) n] = dL.process (x, depth);
                outR[(std::size_t) n] = dR.process (x, depth);
            }
        };

        // Clause 1 — ALL-PASS AT EVERY DEPTH, and clause 2 — DECORRELATED where the chains have
        // separated. Both fall out of one pass per depth.
        bool  allPass       = true;
        bool  decorrelated  = true;
        float worstGainDb   = 0.0f;
        float worstDepth    = 0.0f;
        float corrAtFull    = 0.0f;
        float summedDb      = 0.0f;

        std::vector<float> outL, outR;

        for (std::size_t i = 0; i < depths.size(); ++i)
        {
            const float depth = depths[i];
            runPair (depth, outL, outR);

            double eIn = 0.0, eL = 0.0, eR = 0.0, xy = 0.0, eSum = 0.0, eCoh = 0.0;

            for (int n = 0; n < measure; ++n)
            {
                const double x = noiseAt (warmUp + n);
                const double l = outL[(std::size_t) n];
                const double r = outR[(std::size_t) n];

                eIn += x * x;   eL += l * l;   eR += r * r;   xy += l * r;

                // THE SUM THAT FOUND THE BUG. Where both sub-points feed one speaker the hall
                // hears l + r, and THAT is where a non-all-pass chain shows up as cancellation
                // rather than as a quiet feed.
                eSum += (l + r) * (l + r);
                eCoh += (2.0 * x) * (2.0 * x);
            }

            const auto dB = [] (double ratio) { return 10.0 * std::log10 (ratio); };

            const float gainLDb = static_cast<float> (dB (eL / eIn));
            const float gainRDb = static_cast<float> (dB (eR / eIn));

            if (std::abs (gainLDb) > std::abs (worstGainDb)) { worstGainDb = gainLDb; worstDepth = depth; }
            if (std::abs (gainRDb) > std::abs (worstGainDb)) { worstGainDb = gainRDb; worstDepth = depth; }

            // 0.1 dB. The measured figure is 0.00 at all nine; the broken fractional read was
            // 3.22 to 4.63 down, so this discriminates by a factor of thirty.
            allPass = allPass && std::abs (gainLDb) < 0.1f && std::abs (gainRDb) < 0.1f;

            const float corr = static_cast<float> (xy / std::sqrt (eL * eR));

            // ONLY ABOVE THE CONVERGENCE REGION. At depth 0.01 every section has clamped to the
            // one-sample floor in BOTH chains, so a high correlation there is the design working
            // (clause 4), not failing — asserting decorrelation there would contradict clause 4.
            if (depth >= 0.05f)
                decorrelated = decorrelated && std::abs (corr) < 0.35f;

            if (i == 0)
            {
                corrAtFull = corr;
                summedDb   = static_cast<float> (dB (eSum / eCoh));
            }
        }

        // Clause 3 — DC IS UNTOUCHED, AT EVERY DEPTH. Each section's H(1) = (1-g)/(1-g) = 1
        // exactly, so a DC input leaves a DC output and sweeping depth cannot produce a thump.
        // This is also WHY probe AZ's DC-based zipper construction cannot cover `decorr` — see
        // the note there.
        float dcWorst = 0.0f;
        {
            oo::Decorrelator dc;
            dc.prepare (sr, oo::decorr::kBasesLeft);

            for (int n = 0; n < warmUp; ++n)
                dc.process (1.0f, 1.0f);

            for (int n = 0; n < 512; ++n)
            {
                const float depth = 1.0f - static_cast<float> (n) / 512.0f;
                dcWorst = juce::jmax (dcWorst, std::abs (dc.process (1.0f, depth) - 1.0f));
            }
        }

        // 1e-5 against a measured 3.0e-7 — two orders of headroom over the noise floor of a
        // settled chain, and three below the 6.8e-2 an unsettled one produces.
        const bool dcHeld = dcWorst < 1.0e-5f;

        // Clause 4 — AT DEPTH 0 THE TWO CHAINS ARE ONE CHAIN. Every section clamps to
        // kMinDelaySamples, so left and right become the same filter and their outputs are
        // BIT-IDENTICAL. That is what makes the bottom of the control a common phase colour on a
        // still-correlated pair rather than a comb.
        std::vector<float> zeroL, zeroR;
        runPair (0.0f, zeroL, zeroR);

        bool converged = true;
        for (int n = 0; n < measure && converged; ++n)
            converged = bitExact (zeroL[(std::size_t) n], zeroR[(std::size_t) n]);

        const bool ok = allPass && decorrelated && dcHeld && converged;

        juce::String detail;
        detail << "9 depths 1.00..0.01: worst chain gain " << juce::String (worstGainDb, 3)
               << " dB at depth " << juce::String (worstDepth, 3) << " (expect |dB| < 0.1)"
               << "; cross-correlation at full depth " << juce::String (corrAtFull, 4)
               << " (expect |r| < 0.35), summed vs coherent " << juce::String (summedDb, 2)
               << " dB (incoherent addition predicts -3.01)"
               << "; DC through a depth sweep deviates " << juce::String (dcWorst, 9)
               << " (expect < 1e-5); depth 0 collapses both chains onto one: "
               << (converged ? "bit-identical" : "DIVERGED");

        check ("CV decorrelator-is-allpass-at-every-depth", ok, detail);
    }

    //==========================================================================
    // CW — THE wEff GATE: AT WIDTH 0, decorr = 1 IS BIT-IDENTICAL TO decorr = 0.
    //
    // THE PROBE THAT CARRIES THE FEATURE'S CORRECTNESS ARGUMENT. At width 0 the two sub-points
    // coincide and probe AY asserts v_L is bit-for-bit v_R, so §3.4.3's degenerate path is a clean
    // mono sum. Decorrelating there would make that sum incoherent — 3 dB down and phasey — which
    // is the exact defect the feature exists to remove, delivered by the feature. A gate written
    // on p[width] instead of on wEff would ALSO pass this, which is why the second half exists.
    //
    // ── AND THE SECOND HALF IS NOT DECORATION ─────────────────────────────────────────────────
    // A decorrelator wired to nothing passes clause 1 perfectly. Clause 2 re-runs the identical
    // comparison at width 6 and requires it to FAIL — the same shape as every other non-vacuity
    // gate in this file, and the only thing separating "correctly gated" from "inert"
    // (pattern_probe_must_target_the_branch_the_fix_changed).
    {
        constexpr int total = 4096 * 3;

        const auto renderAt = [&] (float widthMetres, float decorrAmount,
                                   juce::AudioBuffer<float>& dest)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);

            // OFF-CENTRE, so at width 6 the rFade collapse is not what is holding wEff down.
            setParam (proc, "srcX",   0.30f);
            setParam (proc, "srcY",   0.70f);
            setParam (proc, "width",  widthMetres);
            setParam (proc, "decorr", decorrAmount);

            dest.setSize (8, total);
            renderInto (proc, dest, total, { 4096 }, {});
        };

        juce::AudioBuffer<float> zeroWidthOff, zeroWidthOn, wideOff, wideOn;

        renderAt (0.0f, 0.0f, zeroWidthOff);
        renderAt (0.0f, 1.0f, zeroWidthOn);
        renderAt (6.0f, 0.0f, wideOff);
        renderAt (6.0f, 1.0f, wideOn);

        // Clause 1 — THE GATE HOLDS.
        const bool gated = bitIdentical (zeroWidthOff, zeroWidthOn);

        // Clause 2 — AND IT IS A GATE, NOT AN OFF SWITCH.
        const bool audibleWhenWide = ! bitIdentical (wideOff, wideOn);

        const bool live = wideOn.getMagnitude (0, 0, total) > 1.0e-4f;

        const bool ok = gated && audibleWhenWide && live;

        juce::String detail;
        detail << "width 0: decorr 0 vs 1 is "
               << (gated ? "BIT-IDENTICAL" : juce::String ("DIFFERENT — ") + firstDifference (zeroWidthOff, zeroWidthOn))
               << "; width 6 m: decorr 0 vs 1 "
               << (audibleWhenWide ? "differ (the control is live)"
                                   : "ARE IDENTICAL — DECORR IS WIRED TO NOTHING, PROBE VACUOUS")
               << (live ? "" : "; SIGNAL IS SILENT, probe vacuous");

        check ("CW decorr-gated-on-effective-width", ok, detail);
    }

    //==========================================================================
    // CX — QUAL-03 STILL HOLDS WITH THE DECORRELATION CHAINS CLOCKING.
    //
    // CT's shape, one feature later. The eight rings are the sharper risk for the reason the eight
    // delay lines were: their read positions are driven by a ramp, so a depth ramp that advanced
    // per BLOCK rather than per SAMPLE would read a different fractional position at 512 than at
    // 4096 and the two renders would diverge
    // (pattern_block_rate_envelope_breaks_blocksize_invariance).
    //
    // The events INCLUDE a decorr move, so the ramp is in motion across a block boundary rather
    // than parked — a smoother that is never smoothing is invariant to the bug under test.
    {
        constexpr int total = 4096 * 6;

        const std::vector<Event> events
            { { 4096 * 1, "decorr", 0.85f }, { 4096 * 2, "srcX",   0.22f },
              { 4096 * 3, "width",  9.0f  }, { 4096 * 4, "decorr", 0.25f },
              { 4096 * 5, "blur",   0.30f } };

        OOctagonProcessor a, b;

        for (auto* p : { &a, &b })
        {
            negotiate (*p, mono, set71);
            applyRotatedLabels (*p);

            setParam (*p, "srcX",   0.30f);
            setParam (*p, "srcY",   0.70f);
            setParam (*p, "width",  6.0f);
            setParam (*p, "decorr", 0.60f);
        }

        juce::AudioBuffer<float> outA (8, total), outB (8, total);

        // ARMED HERE, NOT READ RAW AT THE END. decorrSamples is a running total since the last
        // reset, and probes CU/CV/CW ran before this one — so an unreset read would report "the
        // chains clocked" on the strength of ANOTHER probe's render and the liveness gate below
        // would be measuring nothing. This is the counter equivalent of an unarmed probe.
        oo::instr::resetCounters();

        renderInto (a, outA, total, { 1, 7, 64, 333, 4096 }, events);
        renderInto (b, outB, total, { 4096 },                events);

        const bool identical = bitIdentical (outA, outB);

        // NON-VACUITY, IN THE PLACE IT COULD HIDE: a render with the chains disengaged is a render
        // of v1.4.0, and it would be invariant for reasons that have nothing to do with this probe.
        const bool ran  = oo::instr::get (oo::instr::decorrSamples) > 0;
        const bool live = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("CX blocksize-invariance-with-decorr", identical && ran && live,
               juce::String (identical
                                 ? "ragged {1,7,64,333,4096} vs 4096, decorr swept 0.60 -> 0.85 -> "
                                   "0.25 across boundaries: bit-identical by memcmp over 24576 "
                                   "samples x 8 lanes"
                                 : firstDifference (outA, outB))
                   + (ran  ? "" : " — THE CHAINS NEVER CLOCKED, probe vacuous")
                   + (live ? "" : " — SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // CY — THE STRUCTURAL BYPASS. A SESSION THAT NEVER ARMS THE MONITOR CLOCKS NOTHING.
    //
    // This is the mechanism the bit-identity claim rests on, asserted directly rather than
    // inferred: fold() is the only thing that writes a monitor sample, isRunning() gates every
    // call to it, and monitorSamples counts every sample it processes. Zero means the v1.6.0
    // signal path was reached LITERALLY, not approximately.
    //
    // CROSS-VERSION IDENTITY IS A DIFFERENT TEST AND THIS IS NOT IT. That one compares against the
    // v1.6.0 backup's own render digest (pattern_reanchor_cross_version_digest_probe). Saying so
    // here stops this probe from being read as more than it proves.
    {
        constexpr int total = 4096 * 4;

        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.30f);
        setParam (proc, "srcY", 0.70f);

        juce::AudioBuffer<float> out (8, total);

        // ARMED HERE. monitorSamples is a running total since the last reset and probes CZ/DA/DB
        // have not run yet, but CU's decorr counter proves the general hazard: an unreset read
        // reports on ANOTHER probe's render and the gate below measures nothing. This is the
        // counter equivalent of an unarmed probe.
        oo::instr::resetCounters();
        renderInto (proc, out, total, { 1, 7, 64, 333, 4096 }, {});

        const bool clean = oo::instr::get (oo::instr::monitorSamples) == 0;
        const bool live  = out.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("CY monitor-disarmed-is-structural-bypass", clean && live,
               juce::String ("monitorSamples = ")
                   + juce::String ((juce::int64) oo::instr::get (oo::instr::monitorSamples))
                   + (clean ? " — the fold never ran" : " — THE FOLD CLOCKED WHILE DISARMED")
                   + (live ? "" : "; SIGNAL IS SILENT, probe vacuous"));
    }

    //==========================================================================
    // CZ — THE PRIMARY CONSTRAINT. AN OFFLINE RENDER IS BIT-IDENTICAL TO A NEVER-ARMED ONE.
    //
    // "Must not silently contaminate a render" is the requirement this whole feature was shaped
    // around, and this is the probe that holds it. Three processors, identical material:
    //
    //   a — never armed                   the reference
    //   b — ARMED, setNonRealtime (true)  must equal a, BIT FOR BIT
    //   c — ARMED, realtime               must DIFFER from a, or b's match proves nothing
    //
    // c IS NOT DECORATION. Without it a fold that was broken, silent, refused, or wired to nothing
    // would pass this probe perfectly — the probe would be measuring the absence of a feature
    // rather than the presence of a guard (pattern_probe_must_target_the_branch_the_fix_changed).
    {
        constexpr int total = 4096 * 4;

        auto build = [&] (OOctagonProcessor& p)
        {
            negotiate (p, mono, set71);
            applyRotatedLabels (p);
            setParam (p, "srcX", 0.22f);
            setParam (p, "srcY", 0.63f);
        };

        OOctagonProcessor a, b, c;
        build (a); build (b); build (c);

        // BEFORE the arm and before the render: setNonRealtime is what the wrapper calls on an
        // offline bounce, and processBlock reads it every block.
        b.setNonRealtime (true);

        const bool armedB = b.setMonitorArmed (true);
        const bool armedC = c.setMonitorArmed (true);

        juce::AudioBuffer<float> outA (8, total), outB (8, total), outC (8, total);

        renderInto (a, outA, total, { 4096 }, {});
        renderInto (b, outB, total, { 4096 }, {});
        renderInto (c, outC, total, { 4096 }, {});

        const bool offlineClean  = bitIdentical (outA, outB);
        const bool realtimeFolds = ! bitIdentical (outA, outC);

        check ("CZ monitor-cannot-contaminate-offline-render",
               offlineClean && realtimeFolds && armedB && armedC,
               juce::String (offlineClean
                                 ? "armed + isNonRealtime: bit-identical to never-armed over "
                                   "16384 samples x 8 lanes"
                                 : juce::String ("OFFLINE RENDER WAS CONTAMINATED — ")
                                       + firstDifference (outA, outB))
                   + (realtimeFolds ? "; realtime arm DOES fold (so the probe can fail)"
                                    : "; REALTIME ARM CHANGED NOTHING — probe vacuous")
                   + (armedB && armedC ? "" : "; AN ARM WAS REFUSED — probe vacuous"));
    }

    //==========================================================================
    // DA — TWO LANES CARRY, SIX ARE HARD ZERO, AND THE TWO ARE left/right.
    //
    // ON A NON-IDENTITY MAP, which is the whole point: applyRotatedLabels puts "L" on speaker 8
    // and "R" on speaker 1, so the monitor pair resolves to out[] slots 7 and 0 while landing in
    // the BUFFER channels left/right occupy. A probe on the default map would be byte-identical to
    // one asserting "channels 0 and 1" and would test nothing at all (the C1 argument, reused).
    {
        constexpr int total = 4096 * 3;

        OOctagonProcessor proc;
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        setParam (proc, "srcX", 0.35f);

        const bool armed = proc.setMonitorArmed (true);

        juce::AudioBuffer<float> out (8, total);
        renderInto (proc, out, total, { 4096 }, {});

        const auto set  = proc.getBusesLayout().getMainOutputChannelSet();
        const int  bufL = set.getChannelIndexForType (juce::AudioChannelSet::left);
        const int  bufR = set.getChannelIndexForType (juce::AudioChannelSet::right);

        // The SETTLED TAIL only. The first 5 ms is the engage crossfade, during which the six rig
        // lanes are legitimately still fading and are NOT yet zero — measuring from sample 0 would
        // fail on correct behaviour.
        const int from = total / 2;

        int   sounding = 0;
        float worstRigLane = 0.0f;

        for (int ch = 0; ch < 8; ++ch)
        {
            const float mag = out.getMagnitude (ch, from, total - from);

            if (ch == bufL || ch == bufR) { if (mag > 1.0e-4f) ++sounding; }
            else                          worstRigLane = juce::jmax (worstRigLane, mag);
        }

        check ("DA monitor-writes-left-right-mutes-six",
               armed && sounding == 2 && worstRigLane == 0.0f,
               juce::String ("monitor pair = buffer ch ") + juce::String (bufL) + "/"
                   + juce::String (bufR) + " under a ROTATED map; " + juce::String (sounding)
                   + " of 2 sounding; worst rig-lane magnitude "
                   + juce::String (worstRigLane, 9)
                   + (armed ? "" : " — ARM REFUSED, probe vacuous"));
    }

    //==========================================================================
    // DB — THE FOLD IS LIVE, POSITION-DEPENDENT, AND INSIDE A PLAUSIBLE ILD BAND.
    //
    // BOTH RAILS ON PURPOSE, and the upper one is not defensive padding. During development a ">"
    // comparison passed at an ILD of 108 dB — which was the far ear being SILENT. An uncompressed
    // constant-power pan had driven it to exactly zero at +/-90 degrees, taking the inter-aural
    // delay and the head shadow out of the signal path with it, and the NEAR ear looked perfect
    // throughout. Asserting a BAND is what makes that visible; asserting the MIRROR is what
    // catches a flipped atan2 convention, which is otherwise inaudible without a reference.
    {
        auto ildDbForSource = [&] (float srcX)
        {
            constexpr int total = 4096 * 4;

            OOctagonProcessor proc;
            negotiate (proc, mono, set71);
            applyRotatedLabels (proc);
            setParam (proc, "srcX", srcX);
            setParam (proc, "srcY", 0.5f);
            proc.setMonitorArmed (true);

            juce::AudioBuffer<float> out (8, total);
            renderInto (proc, out, total, { 4096 }, {});

            const auto set  = proc.getBusesLayout().getMainOutputChannelSet();
            const int  bufL = set.getChannelIndexForType (juce::AudioChannelSet::left);
            const int  bufR = set.getChannelIndexForType (juce::AudioChannelSet::right);

            const int from = total / 2;

            const double eL = juce::jmax (1.0e-12f, out.getRMSLevel (bufL, from, total - from));
            const double eR = juce::jmax (1.0e-12f, out.getRMSLevel (bufR, from, total - from));

            return 20.0 * std::log10 (eR / eL);
        };

        const double right = ildDbForSource (0.95f);
        const double left  = ildDbForSource (0.05f);

        const bool banded   = right > 3.0 && right < 25.0 && left < -3.0 && left > -25.0;
        const bool mirrored = std::fabs (right + left) < 4.0;

        check ("DB monitor-fold-is-position-dependent", banded && mirrored,
               juce::String ("srcX 0.95 -> ") + juce::String (right, 1)
                   + " dB, srcX 0.05 -> " + juce::String (left, 1)
                   + " dB (want +/-3..25, mirrored within 4 dB)"
                   + (banded ? "" : " — OUT OF BAND: 0 dB means the fold is wired to nothing, "
                                    "> 25 dB means the FAR EAR IS SILENT")
                   + (mirrored ? "" : " — NOT MIRRORED: the azimuth convention is flipped"));
    }

    //==========================================================================
    // DC — v1.8.0's BIT-IDENTITY ANCHOR, CAPTURED FROM THE v1.7.0 BINARY (R3).
    //
    // CU's shape, one feature later. v1.8.0 adds ten motion parameters and a metric offset applied
    // downstream of srcX/srcY/srcZ. The feature's compatibility claim is that at `motionOn` = 0
    // NOTHING CHANGES: the v1.7.0 shape() call and the v1.7.0 dirty-check predicate run verbatim
    // on the off branch. This probe is that claim against the previous binary's own digest.
    //
    // ── THE SCENARIO IS CHOSEN TO BE WHERE MOTION WOULD ENGAGE ────────────────────────────────
    // Off-centre puck, srcZ raised (so BOTH Z consumers — shape() and the z-cue solve — carry a
    // non-trivial height), width 6 m, air up, weights non-uniform, RAGGED block sizes so the grid
    // is walked from every phase, and events on srcX / blur / rolloff so the dirty check fires
    // mid-render. Only motionOn itself is at zero.
    //
    // ── RE-ANCHORING ──────────────────────────────────────────────────────────────────────────
    // Same rule as CU: re-derived only by running the PREVIOUS release's harness and transcribing
    // the number with the changelog entry that says why. Never re-recorded from a failing build.
    {
        constexpr int total = 4096 * 4;

        // v1.7.0 CAPTURE, 2026-08-27, from commit 2e03020e (working tree == v1.7.0-O-Octagon for
        // plugins/O-Octagon/Source, verified by `git diff --stat v1.7.0-O-Octagon -- Source` being
        // empty), rendered by this exact scenario before a line of v1.8.0's DSP existed.
        constexpr std::uint64_t kV170Digest = 0xb8c5a2d0c7518204ull;

        // NOT CU's event list. CU's scenario at ragged sizes digests to CU's own constant (QUAL-03
        // makes the chop invisible), which would make this probe a second copy of one number. The
        // srcZ step at a NON-grid offset is what gives DC an independent anchor — and it moves
        // BOTH Z consumers (shape() and the z-cue solve) under motion-off, which is exactly the
        // pair Task 4 routes an effective Z through.
        const std::vector<Event> events
            { { 4096 * 1,        "srcX",    0.72f },
              { 4096 * 2,        "blur",    0.22f },
              { 4096 * 2 + 1000, "srcZ",    3.00f },
              { 4096 * 3,        "rolloff", 5.0f } };

        OOctagonProcessor proc;

        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);

        // ARMED AFTER negotiate(), so prepareToPlay()'s own work is not counted.
        oo::instr::resetCounters();

        setParam (proc, "srcX",      0.28f);
        setParam (proc, "srcY",      0.66f);
        setParam (proc, "srcZ",      1.40f);
        setParam (proc, "width",     6.0f);
        setParam (proc, "airAmount", 0.60f);
        setParam (proc, "hullAtten", 1.60f);
        setWeights (proc, { 1.0f, 0.85f, 0.6f, 1.0f, 0.4f, 0.9f, 1.0f, 0.75f });

        juce::AudioBuffer<float> out (8, total);
        renderInto (proc, out, total, { 1, 7, 64, 333, 4096 }, events);

        const auto digest = bufferDigest (out);

        // THE SECOND HALF OF THE CLAIM. The digest says the audio is v1.7.0's audio; this says
        // the motion branch of updateControl() never executed.
        const auto motionRan = oo::instr::get (oo::instr::motionSolves);

        // NON-VACUITY. A silent render has a digest too, and it would be a stable one.
        const bool live = out.getMagnitude (0, 0, total) > 1.0e-4f;

        const bool ok = digest == kV170Digest && motionRan == 0 && live;

        juce::String detail;
        detail << "width 6 m off-centre, srcZ 1.4, air 0.60, ragged sizes, 4 x 4096 samples x 8 "
                  "lanes: digest 0x"
               << juce::String::toHexString (static_cast<juce::int64> (digest))
               << " vs v1.7.0 0x" << juce::String::toHexString (static_cast<juce::int64> (kV170Digest))
               << "; motionSolves " << juce::String (motionRan) << " (expect 0)"
               << (live ? "" : " — SIGNAL IS SILENT, probe vacuous");

        check ("DC motion-off-matches-v1.7.0", ok, detail);
    }

    //==========================================================================
    // DK — A v1.7.0-SHAPED PRESET (18 keys, no motion) LOADS MOTION-OFF WITH THE 18 UNCHANGED (R7).
    //
    // No migration hook exists for v1.8.0 — no range moved — so THIS is the compatibility gate.
    // WR-01 resets omitted keys to their defaults, and the ten motion defaults are "off"; the
    // probe writes the file by hand in the module's own JSON shape (parameters object of
    // NORMALISED values + version string), dials motion fully ON first, and asserts the load
    // (a) turned it off, (b) left the other nine at default, (c) restored the 18 BIT-EXACT.
    {
        OOctagonProcessor proc;
        bool ok = negotiate (proc, mono, set71);
        juce::String detail;

        if (ok)
        {
            OuariconPresetManager presets (proc.getAPVTS(), "O-Octagon-DK-probe");

            // The 18 pre-1.8.0 ids, each with a NON-DEFAULT normalised value.
            auto* params = new juce::DynamicObject();
            std::array<float, 18> want {};

            for (int i = 0; i < 18; ++i)
            {
                // DYADIC (k/64) so the JSON text round-trip is exact and "bit-exact" tests the
                // load, not juce::JSON's decimal printing. None is a default: the defaults are
                // 0.5 (= 32/64, never hit), 0.2 (srcZ), 0 and 1, 1/9 (rolloff), 0.03, 1/3, 0.35, 2/3.
                want[(size_t) i] = (float) (3 + 2 * i) / 64.0f;      // 3/64 .. 37/64
                params->setProperty (oo::params::id (i), want[(size_t) i]);
            }

            auto* preset = new juce::DynamicObject();
            preset->setProperty ("parameters", juce::var (params));
            preset->setProperty ("version", "1.7.0");
            preset->setProperty ("plugin", "O-Octagon");

            const auto file = scratch32.getChildFile ("dk-v170.json");
            file.replaceWithText (juce::JSON::toString (juce::var (preset)));

            // Motion fully on and every motion knob away from default, so a load that did nothing
            // to the group would fail every clause.
            setParam (proc, "motionOn",     1.0f);
            setParam (proc, "motionPath",   3.0f);
            setParam (proc, "motionSync",   8.0f);
            setParam (proc, "motionRate",   1.5f);
            setParam (proc, "motionSize",   12.0f);
            setParam (proc, "motionRatio",  0.4f);
            setParam (proc, "motionAngle",  45.0f);
            setParam (proc, "motionHeight", 2.0f);
            setParam (proc, "motionPhase",  90.0f);
            setParam (proc, "motionSeed",   9.0f);

            const bool loaded = presets.loadPresetFromFile (file);

            const auto readNorm = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->getValue() : -1.0f;
            };
            const auto defaultNorm = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->getDefaultValue() : -2.0f;
            };

            int          mismatched = 0;
            juce::String firstMismatch;
            for (int i = 0; i < 18; ++i)
                if (! bitExact (readNorm (oo::params::id (i)), want[(size_t) i]))
                {
                    if (mismatched == 0)
                        firstMismatch = juce::String (" (first: ") + oo::params::id (i) + " "
                                      + juce::String (readNorm (oo::params::id (i)), 9) + " vs "
                                      + juce::String (want[(size_t) i], 9) + ")";
                    ++mismatched;
                }

            int motionNotDefault = 0;
            for (int i = oo::params::motionOn; i < (int) oo::params::kCount; ++i)
                if (! bitExact (readNorm (oo::params::id (i)), defaultNorm (oo::params::id (i))))
                    ++motionNotDefault;

            const bool motionOff = readNorm ("motionOn") < 0.5f;

            ok = loaded && mismatched == 0 && motionNotDefault == 0 && motionOff;

            detail << "v1.7.0-shaped 18-key preset: load " << (loaded ? "ok" : "FAILED")
                   << ", 18 restored bit-exact: " << (18 - mismatched) << "/18" << firstMismatch
                   << ", motionOn " << (motionOff ? "OFF" : "ON (WRONG)")
                   << ", motion params at default " << (10 - motionNotDefault) << "/10";
        }
        else
            detail = "negotiate failed";

        check ("DK pre-1.8.0-preset-loads-motion-off", ok, detail);
    }

    //==========================================================================
    // DN — EIGHT FACTORY PRESETS; THE TWO NEW ONES CARRY MOTION, THE SIX OLD ONES LAND IT OFF.
    //
    // factoryDefs() is what initializeFactoryPresets() writes and getPresetList() lists, so its
    // names ARE the list. "Wander" loaded through the preserving path must start Drift at seed 7
    // (Path index 3, Free, 0.05 Hz, 6 m); a v1.7.0 row loaded after it must switch motion OFF
    // again — WR-01's reset through a row that names no motion key, which is the whole
    // compatibility argument for putting the ten in kAuthored rather than kPreserved.
    {
        OOctagonProcessor proc;
        bool ok = negotiate (proc, mono, set71);
        juce::String detail;

        if (ok)
        {
            const auto defs = oo::presets::factoryDefs (proc.getAPVTS());

            juce::StringArray names;
            for (const auto& d : defs)
                names.add (d.name);

            const bool eight = defs.size() == 8
                            && names[6] == "Slow Orbit" && names[7] == "Wander"
                            && names[0] == "Dry Point" && names[5] == "Enveloping";

            OuariconPresetManager presets (proc.getAPVTS(), "O-Octagon-DN-probe");
            presets.getPresetsDirectory().getParentDirectory().deleteRecursively();
            presets.initializeFactoryPresets (defs);

            const auto eng = [&proc] (const char* id)
            {
                auto* p = proc.getAPVTS().getParameter (id);
                return p != nullptr ? p->convertFrom0to1 (p->getValue()) : -1.0f;
            };

            const bool loadedW = oo::presets::loadPreserving (presets, proc.getAPVTS(), "Wander");
            const bool wander  = eng ("motionOn") > 0.5f && near (eng ("motionPath"), 3.0f, 1e-4f)
                              && near (eng ("motionSync"), 0.0f, 1e-4f) && near (eng ("motionRate"), 0.05f, 1e-3f)
                              && near (eng ("motionSize"), 6.0f, 1e-3f) && near (eng ("motionSeed"), 7.0f, 1e-4f);

            const bool loadedS = oo::presets::loadPreserving (presets, proc.getAPVTS(), "Slow Orbit");
            const bool orbit   = eng ("motionOn") > 0.5f && near (eng ("motionPath"), 0.0f, 1e-4f)
                              && near (eng ("motionSync"), 14.0f, 1e-4f) && near (eng ("motionSize"), 8.0f, 1e-3f)
                              && near (eng ("motionRatio"), 0.8f, 1e-3f) && near (eng ("motionHeight"), 1.0f, 1e-3f);

            const bool loadedC = oo::presets::loadPreserving (presets, proc.getAPVTS(), "Chamber");
            const bool off     = eng ("motionOn") < 0.5f && near (eng ("motionSeed"), 1.0f, 1e-4f);

            presets.getPresetsDirectory().getParentDirectory().deleteRecursively();

            ok = eight && loadedW && wander && loadedS && orbit && loadedC && off;

            detail << (eight ? "8 factory names" : "factory count " + juce::String ((int) defs.size()) + " (expect 8)")
                   << "; Wander -> " << (loadedW && wander ? "Drift, Free 0.05 Hz, 6 m, seed 7" : "WRONG")
                   << "; Slow Orbit -> " << (loadedS && orbit ? "Orbit, 4 Bars, 8 m, ratio 0.8, height 1 m" : "WRONG")
                   << "; Chamber after it -> motion " << (loadedC && off ? "OFF, seed back to 1" : "STILL ON");
        }
        else
            detail = "negotiate failed";

        check ("DN factory-presets-carry-motion", ok, detail);
    }

    //==========================================================================
    // ── v1.8.0 MOTION PROBES ──────────────────────────────────────────────────────────────────
    // Common setup: an off-centre puck with width, air and non-uniform weights so the whole chain
    // is live under the moving source, then motion parameters per probe.
    auto motionSetup = [&] (OOctagonProcessor& proc, HarnessPlayHead* ph)
    {
        negotiate (proc, mono, set71);
        applyRotatedLabels (proc);
        if (ph != nullptr)
            proc.setPlayHead (ph);
        setParam (proc, "srcX",      0.35f);
        setParam (proc, "srcY",      0.62f);
        setParam (proc, "srcZ",      0.8f);
        setParam (proc, "width",     4.0f);
        setParam (proc, "airAmount", 0.5f);
        setParam (proc, "hullAtten", 1.2f);
        setWeights (proc, { 1.0f, 0.85f, 0.6f, 1.0f, 0.4f, 0.9f, 1.0f, 0.75f });
    };
    auto motionShape = [&] (OOctagonProcessor& proc, int path, float rateHz, int sync)
    {
        setParam (proc, "motionOn",     1.0f);
        setParam (proc, "motionPath",   (float) path);
        setParam (proc, "motionSync",   (float) sync);
        setParam (proc, "motionRate",   rateHz);
        setParam (proc, "motionSize",   6.0f);
        setParam (proc, "motionRatio",  0.6f);
        setParam (proc, "motionAngle",  30.0f);
        setParam (proc, "motionHeight", 1.0f);
        setParam (proc, "motionPhase",  15.0f);
        setParam (proc, "motionSeed",   7.0f);
    };
    static const char* const kPathNames[] = { "Orbit", "Figure-8", "Sweep", "Drift", "Pendulum", "Spiral" };

    //==========================================================================
    // DD — THE ANCHOR IS NEVER WRITTEN (R2 / D1). Motion on, Orbit 1 Hz, 4 s at 64-sample blocks:
    // srcX/srcY/srcZ raw values BIT-unchanged at every one of the 3000 blocks, while the render
    // is live and the motion branch ran. The companion clause (Risk 6): a srcZ of 2 m with
    // motion off renders bit-identically to srcZ 0 + Height 2 m held at sin t = 1 (a stopped
    // synced transport at ppq 1 with "1 Bar" -> 0.25 cycles -> t = pi/2; v1.10.0 re-fixtured
    // from index 8 when WR-01 made 1/4 one cycle per BEAT), which is only true if the
    // effective Z reached BOTH the shaper and the z-cue solve.
    {
        OOctagonProcessor proc;
        motionSetup (proc, nullptr);
        motionShape (proc, 0, 1.0f, 0);

        const float x0 = proc.getAPVTS().getRawParameterValue ("srcX")->load();
        const float y0 = proc.getAPVTS().getRawParameterValue ("srcY")->load();
        const float z0 = proc.getAPVTS().getRawParameterValue ("srcZ")->load();

        oo::instr::resetCounters();

        juce::AudioBuffer<float> out (8, 64 * 3000);
        int moved = 0;

        const auto series = renderOffsets (proc, 3000, 64, nullptr, &out, [&] (int)
        {
            if (! bitExact (proc.getAPVTS().getRawParameterValue ("srcX")->load(), x0)
             || ! bitExact (proc.getAPVTS().getRawParameterValue ("srcY")->load(), y0)
             || ! bitExact (proc.getAPVTS().getRawParameterValue ("srcZ")->load(), z0))
                ++moved;
        });

        const bool ran    = oo::instr::get (oo::instr::motionSolves) > 0;
        const bool live   = out.getMagnitude (0, 0, out.getNumSamples()) > 1.0e-4f;
        const bool moving = ! sameSeries (OffsetSeries (series.begin(), series.begin() + 100),
                                          OffsetSeries (series.begin() + 100, series.begin() + 200));

        // Companion: both Z consumers.
        constexpr int total = 4096 * 2;
        OOctagonProcessor a, b;
        HarnessPlayHead   stopped;
        stopped.ppqStart = 1.0; stopped.playing = false;
        motionSetup (a, nullptr);
        motionSetup (b, &stopped);
        setParam (a, "srcZ", 2.0f);
        setParam (b, "srcZ", 0.0f);
        setParam (b, "motionOn", 1.0f);
        setParam (b, "motionPath", 0.0f);
        setParam (b, "motionSync", 12.0f);         // 1 Bar -> 0.25 cycles/beat
        setParam (b, "motionSize", 0.0f);
        setParam (b, "motionHeight", 2.0f);
        setParam (b, "motionPhase", 0.0f);
        juce::AudioBuffer<float> outA (8, total), outB (8, total);
        renderInto (a, outA, total, { 4096 }, {});
        renderInto (b, outB, total, { 4096 }, {}, &stopped);
        const bool bothZ = bitIdentical (outA, outB);

        check ("DD anchor-never-written", moved == 0 && ran && live && moving && bothZ,
               juce::String ("srcX/srcY/srcZ bit-unchanged at ") + juce::String (3000 - moved)
                   + "/3000 blocks; motionSolves " + juce::String (oo::instr::get (oo::instr::motionSolves))
                   + (moving ? "; offset moving" : "; OFFSET STATIC")
                   + (live ? "" : " — SILENT")
                   + (bothZ ? "; srcZ 2 == srcZ 0 + Height 2 at sin t = 1: bit-identical (both Z consumers)"
                            : "; srcZ 2 vs Height 2 DIFFER — " + firstDifference (outA, outB)));
    }

    //==========================================================================
    // DI — A HOST srcX STEP MOVES THE WHOLE PATH (D1). The offset series is a function of time
    // only: identical with and without the step, while the two renders differ (the step is live).
    {
        constexpr int blocks = 1500;
        OOctagonProcessor a, b;
        motionSetup (a, nullptr); motionShape (a, 0, 0.5f, 0);
        motionSetup (b, nullptr); motionShape (b, 0, 0.5f, 0);

        juce::AudioBuffer<float> outA (8, 64 * blocks), outB (8, 64 * blocks);

        const auto sa = renderOffsets (a, blocks, 64, nullptr, &outA, [] (int) {});
        const auto sb = renderOffsets (b, blocks, 64, nullptr, &outB, [&] (int i)
        {
            if (i == 700)
                setParam (b, "srcX", 0.78f);
        });

        const bool same    = sameSeries (sa, sb);
        const bool differs = ! bitIdentical (outA, outB);

        check ("DI anchor-step-moves-whole-path", same && differs,
               juce::String ("offset series with vs without a srcX step at block 700: ")
                   + (same ? "bit-identical (1500 x 3 floats)" : "DIFFER")
                   + (differs ? "; renders differ (step is live)" : "; RENDERS IDENTICAL — step inert"));
    }

    //==========================================================================
    // DE — BLOCK-SIZE INVARIANCE, FREE MODE, EVERY PATH (R5), plus a rate-change event on the
    // last pass (the re-base is the one place an accumulator could creep back in). {64} vs {256}
    // vs {1024} vs ragged {1,7,64,333,4096}: memcmp-identical, motionSolves > 0, live.
    // Negative control NC2 replaces cyclesAt with an accumulator and this probe must fail.
    {
        constexpr int total = 4096 * 4;
        int failedPaths = 0;
        juce::String detail;

        for (int path = 0; path < 6; ++path)
        {
            const std::vector<Event> events
                { { 4096 * 2 + 100, "motionRate", 1.7f },
                  { 4096 * 3,       "srcY",       0.4f } };

            std::vector<juce::AudioBuffer<float>> outs;
            const std::vector<std::vector<int>> sizes { { 64 }, { 256 }, { 1024 }, { 1, 7, 64, 333, 4096 } };

            oo::instr::resetCounters();

            for (const auto& sz : sizes)
            {
                OOctagonProcessor proc;
                motionSetup (proc, nullptr);
                motionShape (proc, path, 1.0f, 0);
                outs.emplace_back (8, total);
                renderInto (proc, outs.back(), total, sz, events);
            }

            bool identical = true;
            for (size_t k = 1; k < outs.size(); ++k)
                identical = identical && bitIdentical (outs[0], outs[k]);

            // EVERY boundary must have solved, not merely some: with the dirty-check bypass
            // deleted (NC1) the two events still force a solve each, so "> 0" passed on the
            // strength of a frozen offset. 4 renders x total/64 boundaries, exactly.
            const auto solves = oo::instr::get (oo::instr::motionSolves);
            const bool ran    = solves == 4ull * (total / 64);
            const bool live   = outs[0].getMagnitude (0, 0, total) > 1.0e-4f;

            if (! (identical && ran && live))
            {
                ++failedPaths;
                detail << kPathNames[path] << ": " << (identical ? "" : firstDifference (outs[0], outs[3]))
                       << (ran ? "" : " motionSolves " + juce::String (solves) + " != " + juce::String (4 * (total / 64)))
                       << (live ? "" : " SILENT") << "; ";
            }
        }

        check ("DE blocksize-invariance-free", failedPaths == 0,
               failedPaths == 0 ? "6 paths x {64}/{256}/{1024}/ragged, rate 1.0 -> 1.7 Hz mid-render: "
                                  "all bit-identical, every boundary solved (1024/1024), live"
                                : detail);
    }

    //==========================================================================
    // DF — DE's shape, SYNC 1/4 @ 120 BPM with the transport ROLLING (R5 + R4). Each render gets
    // its own playhead from ppq 0, so the grid boundaries see extrapolated PPQ from different
    // block starts — a 4096 block extrapolates 63 boundaries, a 64 block none.
    {
        constexpr int total = 4096 * 4;
        int failedPaths = 0;
        juce::String detail;

        for (int path = 0; path < 6; ++path)
        {
            const std::vector<Event> events { { 4096 * 2 + 100, "motionSize", 9.0f } };
            std::vector<juce::AudioBuffer<float>> outs;
            const std::vector<std::vector<int>> sizes { { 64 }, { 256 }, { 1024 }, { 1, 7, 64, 333, 4096 } };

            oo::instr::resetCounters();

            for (const auto& sz : sizes)
            {
                OOctagonProcessor proc;
                HarnessPlayHead   ph;
                motionSetup (proc, &ph);
                motionShape (proc, path, 1.0f, 8);
                outs.emplace_back (8, total);
                renderInto (proc, outs.back(), total, sz, events, &ph);
                proc.setPlayHead (nullptr);
            }

            bool identical = true;
            for (size_t k = 1; k < outs.size(); ++k)
                identical = identical && bitIdentical (outs[0], outs[k]);

            const bool ran  = oo::instr::get (oo::instr::motionSolves) > 0;
            const bool live = outs[0].getMagnitude (0, 0, total) > 1.0e-4f;

            if (! (identical && ran && live))
            {
                ++failedPaths;
                detail << kPathNames[path] << ": " << (identical ? "" : firstDifference (outs[0], outs[3]))
                       << (ran ? "" : " NEVER RAN") << (live ? "" : " SILENT") << "; ";
            }
        }

        check ("DF blocksize-invariance-synced", failedPaths == 0,
               failedPaths == 0 ? "6 paths, 1/4 @ 120 BPM rolling, {64}/{256}/{1024}/ragged: all "
                                  "bit-identical, motion ran, live"
                                : detail);
    }

    //==========================================================================
    // DG — TWO BOUNCES OF ONE SESSION ARE IDENTICAL, DRIFT INCLUDED (R4 / D7); seed 8 differs.
    {
        constexpr int total = 4096 * 4;
        juce::AudioBuffer<float> o7a (8, total), o7b (8, total), o8 (8, total);

        auto render = [&] (juce::AudioBuffer<float>& out, float seed)
        {
            OOctagonProcessor proc;
            HarnessPlayHead   ph;
            motionSetup (proc, &ph);
            motionShape (proc, 3, 0.5f, 8);
            setParam (proc, "motionSeed", seed);
            renderInto (proc, out, total, { 1, 7, 64, 333, 4096 }, {}, &ph);
            proc.setPlayHead (nullptr);
        };

        render (o7a, 7.0f);
        render (o7b, 7.0f);
        render (o8,  8.0f);

        const bool same    = bitIdentical (o7a, o7b);
        const bool differs = ! bitIdentical (o7a, o8);
        const bool live    = o7a.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("DG drift-bounce-deterministic", same && differs && live,
               juce::String ("Drift seed 7 twice: ") + (same ? "bit-identical" : firstDifference (o7a, o7b))
                   + "; seed 8: " + (differs ? "differs" : "IDENTICAL — seed inert")
                   + (live ? "" : " — SILENT"));
    }

    //==========================================================================
    // DH — A BOUNCE STARTED MID-TIMELINE LANDS AT THE CORRECT PHASE (R4). Sync 1/4 @ 120 BPM: the
    // offset series of a render starting at ppq 38.4 equals the tail of a render from ppq 0
    // (38.4 beats = 921600 samples in). Compared to 1e-6 m rather than bit-exact: the two
    // derive the boundary PPQ from different block starts, and the claim is the PHASE, not the
    // rounding (bit-identity of one session is DG's claim).
    //
    // 38.4 AND NOT THE PLAN'S 37.5, FOR A REASON WORTH KEEPING: the control grid is aligned to the
    // ABSOLUTE SAMPLE COUNTER, deliberately not to the playhead (GainStage.h — a locate must not
    // jump the grid). 37.5 beats is 900000 samples, which 64 does not divide, so a bounce from 0
    // and a bounce from 37.5 evaluate the path at instants 32 samples apart: a real, designed 9 mm
    // difference that the 5 ms gain ramps absorb and that no probe should call a phase error.
    // 921600 = 14400 x 64 puts both renders' boundaries on the same instants.
    {
        constexpr int tailBlocks = 128;
        constexpr int lead       = 921600 / 64;
        static_assert (lead * 64 == 921600, "the lead must be a whole number of grid blocks");

        OOctagonProcessor a, b;
        HarnessPlayHead   pa, pb;
        pb.ppqStart = 38.4;
        motionSetup (a, &pa); motionShape (a, 1, 1.0f, 8);
        motionSetup (b, &pb); motionShape (b, 1, 1.0f, 8);

        const auto sa = renderOffsets (a, lead + tailBlocks, 64, &pa, nullptr, [] (int) {});
        const auto sb = renderOffsets (b, tailBlocks,        64, &pb, nullptr, [] (int) {});

        float worst = 0.0f;
        for (int i = 0; i < tailBlocks; ++i)
            for (int k = 0; k < 3; ++k)
                worst = std::max (worst, std::abs (sa[(size_t) (lead + i)][(size_t) k] - sb[(size_t) i][(size_t) k]));

        const bool moving = ! sameSeries (OffsetSeries (sb.begin(), sb.begin() + 64),
                                          OffsetSeries (sb.begin() + 64, sb.end()));

        a.setPlayHead (nullptr); b.setPlayHead (nullptr);

        check ("DH mid-timeline-start-phase", worst < 1.0e-6f && moving,
               "start at ppq 38.4 vs tail of a render from 0: worst offset diff "
                   + juce::String (worst, 9) + " m over 128 boundaries"
                   + (moving ? "" : " — OFFSET STATIC, probe vacuous"));
    }

    //==========================================================================
    // DJ — SYNCED + STOPPED HOLDS; FREE + STOPPED KEEPS MOVING (RESEARCH Q7).
    {
        constexpr int blocks = 400;

        auto run = [&] (int sync)
        {
            OOctagonProcessor proc;
            HarnessPlayHead   ph;
            motionSetup (proc, &ph);
            motionShape (proc, 0, 1.0f, sync);
            const auto s = renderOffsets (proc, blocks, 64, &ph, nullptr, [&] (int i)
            {
                if (i == blocks / 2)
                    ph.playing = false;
            });
            proc.setPlayHead (nullptr);
            return s;
        };

        const auto synced = run (8);
        const auto free   = run (0);

        auto constantFrom = [] (const OffsetSeries& s, size_t from)
        {
            for (size_t i = from + 1; i < s.size(); ++i)
                if (std::memcmp (&s[i], &s[from], sizeof (s[i])) != 0)
                    return false;
            return true;
        };

        const bool syncedMovedBefore = ! constantFrom (OffsetSeries (synced.begin(), synced.begin() + blocks / 2), 0);
        const bool syncedHolds       = constantFrom (synced, blocks / 2 + 1);
        const bool freeMoves         = ! constantFrom (free, blocks / 2 + 1);

        check ("DJ stopped-transport-behaviour", syncedMovedBefore && syncedHolds && freeMoves,
               juce::String ("1/4 @ 120: moving while rolling ") + (syncedMovedBefore ? "yes" : "NO")
                   + ", holds after stop " + (syncedHolds ? "yes" : "NO")
                   + "; Free: keeps moving after stop " + (freeMoves ? "yes" : "NO"));
    }

    //==========================================================================
    // SAFE MODE (RESEARCH Q3) — a stereo negotiation is the SAFE fold, which never reads a
    // position: motion on renders bit-identically to motion off, WITH the motion branch running
    // (so the map still animates). Inaudible by construction, not by a gate.
    {
        constexpr int total = 4096 * 3;
        OOctagonProcessor a, b;

        for (auto* p : { &a, &b })
        {
            negotiate (*p, mono, juce::AudioChannelSet::stereo());
            p->setStereoBinauralEnabled (false);   // v1.11.0: this probe is about the DRY fold (see AT)
            setParam (*p, "srcX", 0.3f);
            setParam (*p, "width", 4.0f);
        }
        motionShape (b, 0, 1.0f, 0);

        juce::AudioBuffer<float> outA (2, total), outB (2, total);
        renderInto (a, outA, total, { 1, 7, 64, 333, 4096 }, {});
        oo::instr::resetCounters();
        renderInto (b, outB, total, { 1, 7, 64, 333, 4096 }, {});

        const bool identical = bitIdentical (outA, outB);
        const bool ran       = oo::instr::get (oo::instr::motionSolves) > 0;
        const bool live      = outA.getMagnitude (0, 0, total) > 1.0e-4f;

        check ("DM safe-mode-motion-inaudible", identical && ran && live,
               juce::String ("stereo (SAFE) fold, motion on vs off: ")
                   + (identical ? "bit-identical" : firstDifference (outA, outB))
                   + "; motionSolves " + juce::String (oo::instr::get (oo::instr::motionSolves))
                   + (ran ? " (branch ran, map animates)" : " — BRANCH NEVER RAN")
                   + (live ? "" : " — SILENT"));
    }

    //==========================================================================
    // DL — CX AND CW RE-RUN WITH MOTION ON. (1) CX's decorr scenario at ragged vs 4096 stays
    // bit-identical with the source orbiting and the chains clocking; (2) CW's claim: at width 0
    // decorr is inert (decorrSamples == 0) even though motion moves the puck every grid.
    {
        constexpr int total = 4096 * 6;
        const std::vector<Event> events
            { { 4096 * 1, "decorr", 0.85f }, { 4096 * 2, "srcX",   0.22f },
              { 4096 * 3, "width",  9.0f  }, { 4096 * 4, "decorr", 0.25f },
              { 4096 * 5, "blur",   0.30f } };

        OOctagonProcessor a, b;
        for (auto* p : { &a, &b })
        {
            motionSetup (*p, nullptr);
            motionShape (*p, 5, 0.8f, 0);
            setParam (*p, "width",  6.0f);
            setParam (*p, "decorr", 0.60f);
        }

        juce::AudioBuffer<float> outA (8, total), outB (8, total);
        oo::instr::resetCounters();
        renderInto (a, outA, total, { 1, 7, 64, 333, 4096 }, events);
        renderInto (b, outB, total, { 4096 },                events);

        const bool identical = bitIdentical (outA, outB);
        const bool chains    = oo::instr::get (oo::instr::decorrSamples) > 0;
        const bool ran       = oo::instr::get (oo::instr::motionSolves) > 0;

        OOctagonProcessor c;
        motionSetup (c, nullptr);
        motionShape (c, 0, 1.0f, 0);
        setParam (c, "width",  0.0f);
        setParam (c, "decorr", 0.8f);
        juce::AudioBuffer<float> outC (8, 4096 * 2);
        oo::instr::resetCounters();
        renderInto (c, outC, 4096 * 2, { 4096 }, {});
        const bool inert = oo::instr::get (oo::instr::decorrSamples) == 0
                        && oo::instr::get (oo::instr::motionSolves) > 0;

        check ("DL decorr-with-motion", identical && chains && ran && inert,
               juce::String ("CX shape + Spiral 0.8 Hz: ") + (identical ? "bit-identical" : firstDifference (outA, outB))
                   + (chains ? ", chains clocked" : ", CHAINS NEVER CLOCKED")
                   + (ran ? ", motion ran" : ", MOTION NEVER RAN")
                   + "; width 0 + motion: decorr " + (inert ? "inert (decorrSamples 0)" : "RAN at wEff 0"));
    }

    scratch32.deleteRecursively();

    //==========================================================================
    // DO — VERSION HINTS ARE MONOTONE BY GENERATION (v1.10.0 / WR-02). The AU wrapper sorts the
    // parameter list by ID hash then STABLE-sorts by version hint, and Logic keys automation lanes
    // by index in that list. Every parameter added after a release must carry a hint higher than
    // everything before it. Evaluated on the LIVE parameter objects, not a mirrored count.
    {
        OOctagonProcessor proc;
        static const char* const originals[] = { "srcX", "srcY", "srcZ", "width", "rolloff", "blur",
                                                 "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
                                                 "hullAtten", "airAmount", "outputGain" };
        auto expectedHint = [] (const juce::String& id)
        {
            for (auto* o : originals) if (id == o) return 1;
            if (id == "decorr") return 2;
            if (id.startsWith ("motion")) return 3;
            return -1;                                   // unknown id: a NEW parameter with no generation
        };

        int seen = 0, wrong = 0;
        juce::String detail;
        for (auto* p : proc.getParameters())
        {
            auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p);
            if (withId == nullptr) continue;
            ++seen;
            const int want = expectedHint (withId->paramID);
            if (want < 0 || withId->getVersionHint() != want)
            {
                ++wrong;
                detail << withId->paramID << " hint " << withId->getVersionHint() << " (want " << want << ") ";
            }
        }
        check ("DO version-hints-monotone", seen == 28 && wrong == 0,
               juce::String (seen) + " parameters, " + juce::String (wrong) + " off-generation"
                   + (wrong > 0 ? ": " + detail : juce::String ("; originals 1, decorr 2, motion* 3")));
    }


    //==========================================================================
    // DP — v1.11.0: THE STEREO-BUS BINAURAL ARM. Six clauses, each with the failure it names.
    //
    //   (a) A stereo negotiation, preference at its default, takes the arm: isBinauralActive()
    //       is true after a block and the output is NOT the dry input (v1.10.1 wrote 0.25 back).
    //   (b) POSITION-DEPENDENT, AND THE CONVENTION. Source hard left -> the LEFT ear is louder;
    //       hard right -> the RIGHT ear. A mirrored fold or a fold of a constant would pass a
    //       "not silent" check and fail here. Measured after the engage ramp and the delay ramps.
    //   (c) Preference OFF is the v1.10.1 dry fold EXACTLY: output == input at unity.
    //   (d) EXCLUSION with the 8-channel arm: a 7.1 negotiation with the preference ON never
    //       reports the arm active, and renders BIT-IDENTICALLY to the preference OFF. This is
    //       the clause that keeps v1.11.0 out of every 8-channel delivery.
    //   (e) THE (2,1) TRAP: stereo-in / mono-out hands processBlock a 2-channel buffer on a MONO
    //       bus. The arm must not take it (first run of AT caught a width-only rule doing so).
    //   (f) The eight meters read the PRE-FOLD lanes: a wide source on the stereo bus lights more
    //       than the two indicators the host buffer could ever account for.
    {
        const auto stereoOut = juce::AudioChannelSet::stereo();
        bool         ok = true;
        juce::String detail;

        // (a)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, stereoOut);

            juce::AudioBuffer<float> block (2, kBlockSize);
            juce::MidiBuffer         midi;
            bool finite = true;
            float written = 0.0f;

            for (int i = 0; i < 8; ++i)
            {
                block.clear();
                for (int n = 0; n < kBlockSize; ++n) block.setSample (0, n, 0.25f);
                proc.processBlock (block, midi);
                finite = finite && allFinite (block);
                written = block.getSample (0, kBlockSize - 1);
            }

            const bool active  = proc.isBinauralActive();
            const bool notDry  = std::abs (written - 0.25f) > 1.0e-3f;
            const bool live    = block.getMagnitude (0, 0, kBlockSize) > 1.0e-4f
                              && block.getMagnitude (1, 0, kBlockSize) > 1.0e-4f;

            ok = ok && active && notDry && live && finite;
            detail << "(a) active " << (active ? "yes" : "NO") << ", ch0 wrote " << juce::String (written, 4)
                   << (notDry ? " (not the dry 0.25)" : " — THE DRY INPUT")
                   << (live ? ", both ears live" : " — AN EAR IS SILENT")
                   << (finite ? "" : " — NaN/Inf") << "; ";
        }

        // (b)
        {
            constexpr int total = 4096 * 3;
            constexpr int skip  = 4096;                                // engage + delay ramps
            OOctagonProcessor left, right;
            const auto range = OOctagonProcessor().getAPVTS().getParameterRange ("srcX");

            negotiate (left,  mono, stereoOut);  setParam (left,  "srcX", range.start);
            negotiate (right, mono, stereoOut);  setParam (right, "srcX", range.end);

            juce::AudioBuffer<float> outL (2, total), outR (2, total);
            renderInto (left,  outL, total, { 1, 7, 64, 333, 4096 }, {});
            renderInto (right, outR, total, { 1, 7, 64, 333, 4096 }, {});

            const float lL = outL.getRMSLevel (0, skip, total - skip), lR = outL.getRMSLevel (1, skip, total - skip);
            const float rL = outR.getRMSLevel (0, skip, total - skip), rR = outR.getRMSLevel (1, skip, total - skip);

            const bool leftIsLeft   = lL > lR * 1.2f;
            const bool rightIsRight = rR > rL * 1.2f;
            const bool differs      = ! bitIdentical (outL, outR);

            ok = ok && leftIsLeft && rightIsRight && differs;
            detail << "(b) src left L/R " << juce::String (lL, 4) << "/" << juce::String (lR, 4)
                   << (leftIsLeft ? "" : " — LEFT EAR NOT LOUDER")
                   << ", src right L/R " << juce::String (rL, 4) << "/" << juce::String (rR, 4)
                   << (rightIsRight ? "" : " — RIGHT EAR NOT LOUDER")
                   << (differs ? "" : " — POSITION-INDEPENDENT") << "; ";
        }

        // (c)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, stereoOut);
            proc.setStereoBinauralEnabled (false);

            juce::AudioBuffer<float> block (2, kBlockSize);
            juce::MidiBuffer         midi;
            bool dry = true;

            for (int i = 0; i < 4; ++i)
            {
                block.clear();
                for (int n = 0; n < kBlockSize; ++n) block.setSample (0, n, testSample (n));
                proc.processBlock (block, midi);
                for (int n = 0; n < kBlockSize; ++n)
                    if (! juce::exactlyEqual (block.getSample (0, n), testSample (n))) dry = false;
            }

            const bool inactive = ! proc.isBinauralActive();
            ok = ok && dry && inactive;
            detail << "(c) OFF: " << (dry ? "bit-exact dry fold" : "NOT THE DRY FOLD")
                   << (inactive ? "" : ", STILL REPORTS ACTIVE") << "; ";
        }

        // (d)
        {
            constexpr int total = 4096 * 2;
            OOctagonProcessor on, off;
            negotiate (on,  mono, set71);
            negotiate (off, mono, set71);  off.setStereoBinauralEnabled (false);
            setParam (on, "srcX", 0.4f);   setParam (off, "srcX", 0.4f);

            juce::AudioBuffer<float> outOn (8, total), outOff (8, total);
            renderInto (on,  outOn,  total, { 1, 7, 64, 333, 4096 }, {});
            renderInto (off, outOff, total, { 1, 7, 64, 333, 4096 }, {});

            const bool identical = bitIdentical (outOn, outOff);
            const bool inactive  = ! on.isBinauralActive();
            const bool live      = outOn.getMagnitude (0, total) > 1.0e-4f;

            ok = ok && identical && inactive && live;
            detail << "(d) 7.1 pref on vs off: " << (identical ? "bit-identical" : firstDifference (outOn, outOff))
                   << (inactive ? "" : " — ARM REPORTED ACTIVE ON 8ch") << (live ? "" : " — SILENT") << "; ";
        }

        // (e)
        {
            OOctagonProcessor proc;
            negotiate (proc, juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono());

            juce::AudioBuffer<float> block (2, kBlockSize);      // max(totalIn, totalOut) = 2
            juce::MidiBuffer         midi;
            bool dry = true;

            for (int i = 0; i < 4; ++i)
            {
                block.clear();
                for (int n = 0; n < kBlockSize; ++n) { block.setSample (0, n, 0.25f); block.setSample (1, n, 0.25f); }
                proc.processBlock (block, midi);
                if (std::abs (block.getSample (0, kBlockSize - 1) - 0.25f) > 1.0e-6f) dry = false;
            }

            const bool inactive = ! proc.isBinauralActive();
            ok = ok && dry && inactive;
            detail << "(e) (2,1): " << (dry ? "dry" : "FOLDED ON A MONO BUS") << (inactive ? "" : ", REPORTS ACTIVE") << "; ";
        }

        // (f)
        {
            OOctagonProcessor proc;
            negotiate (proc, mono, stereoOut);
            setParam (proc, "width", proc.getAPVTS().getParameterRange ("width").end);
            setParam (proc, "blur",  proc.getAPVTS().getParameterRange ("blur").end);

            juce::AudioBuffer<float> block (2, kBlockSize);
            juce::MidiBuffer         midi;
            (void) proc.readAndZeroMeters();

            for (int i = 0; i < 8; ++i)
            {
                block.clear();
                for (int n = 0; n < kBlockSize; ++n) block.setSample (0, n, testSample (n));
                proc.processBlock (block, midi);
            }

            const auto peaks = proc.readAndZeroMeters();
            int lit = 0;
            for (float v : peaks) if (v > 1.0e-4f) ++lit;

            ok = ok && lit >= 3;
            detail << "(f) meters lit on the stereo bus: " << lit << " of 8" << (lit >= 3 ? "" : " — ONLY THE HOST PAIR");
        }

        check ("DP stereo-bus-binaural-arm", ok, detail);
    }

    //==========================================================================
    std::printf ("\n----------------------------------------------------\n");
    std::printf ("  %d probe(s), %d failure(s)\n\n", probes, failures);

    return failures == 0 ? 0 : 1;
}
