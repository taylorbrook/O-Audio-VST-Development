/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics render-harness — Stage 2 / Phase 2.1 DSP gate.

    Discipline (CONTEXT-2.1 D9 / PLAN-2.1 A10–A11): every probe renders on a
    FRESH instance; verdicts are ratios or exact anchors (no wall-clock, no
    slack-on-a-slack); goldens are analytic or the double-precision
    ReferenceModel (never stored renders); every literal is printed; each
    click / allocation detector carries a negative control
    (checkFailsAsExpected) and a liveness check; engine-direct renders run
    under juce::ScopedNoDenormals (F6).

    Probes:

      P0 contract       — 12 APVTS parameters with the ARCHITECTURE.md types /
                          defaults / skews; getLatencySamples() == kPartition == 64.
      P1 null           — processor at az 37°, el −12°, d 3.3 m, order 3, Stereo
                          vs ReferenceModel on the same noise: max |err| < 1e-5.
      P1 anchor         — front / 1 m / order 1 in BOTH decoder modes and
                          engine-direct: out == 0.75f·s[n−64] BIT-EQUAL,
                          out[0..63] == 0, out[64] exact (no fade-in).
      P1b               — Left only → 0.75f·L[n−64]; mono-in bus → 0.75f·in0[n−64].
      P2 / P3           — defaults, orbit Off: ragged ≡ flat; prep-64 + 4096
                          feed ≡ prep-4096 ≡ flat, bit-equal.
      P2b / P3b         — orbit On 2 Hz, order 3, Stereo, az 45°, d 2.5 m: the
                          same slicings BIT-EQUAL to the flat render (pins the
                          D5 amendment: the 64-sample control grid).
      P4 latency        — impulse at 100 → first nonzero at 100 + 64, amplitude
                          exactly 0.75f.
      P5 silence floor  — outputLevel −60 before prepare → exact silence.
      DSP-01            — encodeSH16 (float) vs the generic SN3D table (double)
                          on a 13 × 7 grid: max |err| < 1e-5; W ≡ 1; Y > 0 at
                          +90°; Appendix-B spot vectors.
      DSP-04            — ±60° cardioid pair: front L == R; +90° ≥ 20 dB with
                          L > R; e = ±90° L == R == 0.5f; non-negative over 72
                          directions; processor Stereo at +90° rms ≥ 20 dB;
                          correlation with the delayed source ≥ 0.
      DSP-05            — two-tone 200 Hz + 12 kHz, 25 log-spaced d + 1 m:
                          level strictly decreasing; HF/LF bit-exactly g·x for
                          d ≤ 1 m and strictly decreasing on (1, 20]; anchors
                          −10.79 / +8.87 / 0 / −24.19 dB; liveness; clamp TARGET
                          at 44.1 k; audible clamp at fs 22050.
      DSP-06a           — +170° → −170° trajectory through 180°, never near 0°,
                          converged by 0.2 s; orbit Off→On resets to the puck;
                          On→Off glides through 180°.
      DSP-06b           — full-circle sweep click gate on a 100 Hz tone: worst
                          second difference vs a matched parked control, ratio
                          ≤ 2.0, hook-driven at 64 blocks and orbit-driven at
                          64 / 4096 blocks (bit-equal); snapped-model negative
                          control; liveness ≥ 6 dB.
      order fade        — 1 → 3 → 1 mid-tone on the 16-channel BUS TAP (the
                          stereo output cannot see bands 2–3): exact 1.0f / 0.0f
                          arrival within 16 steps, monotonic; bus-tap click ratio
                          ≤ 2.0 vs the model; snapped-model negative control;
                          shPeak[4..15] == 0 after.
      meter frame       — orderActive 4 / 9 / 16 with the bands above exactly 0;
                          azEff follows the orbit; peaks accumulate until read,
                          then reset; latencySamples == 64; azRot == azEff.
      extremes          — 16 corner settings × fs 48 k / 96 k: finite, not
                          silenced.
      PERF-01           — global operator new/delete counter: 0 allocations
                          across 200 processBlock calls with every fade armed
                          and one 4096-sample block; liveness new == 1.
      digest            — FNV-1a 64 of the P1-null render (informational).

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/SpatialEngine.h"
#include "dsp/ShEncoder.h"
#include "dsp/DistanceModel.h"
#include "dsp/StereoDecoder.h"
#include "ReferenceModel.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <thread>
#include <vector>

//==============================================================================
// ── PERF-01 allocation counter ────────────────────────────────────────────────
//
// PROVENANCE: copied VERBATIM from plugins/O-Octagon/tests/render-harness/main.cpp:111–241
// (probe AO; RESEARCH-2.1 R2 / PLAN-2.1 A11), including the VERIFY-3.3 thread filter. No
// ASan compile-out: ASan hangs at init on this toolchain anyway
// (critical_asan_runtime_hangs_at_init_on_macos26).
//
// Coverage statement (SUMMARY-2.1 repeats it): covered = every new-expression on the audio
// thread; NOT covered = HeapBlock/malloc paths (AudioBuffer, Array, MidiBuffer, MemoryBlock)
// and the AsyncUpdater post path — grep + inspection. Nothing in the 2.1 engine lives in the
// uncovered set (its only dynamic storage is std::vector sized in prepareToPlay, which IS
// operator new and would be counted if it resized in process).
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
    namespace osa = OSimpleAmbisonics::dsp;

    constexpr double kFs            = 48000.0;
    constexpr int    kDefaultBlock  = 512;
    constexpr int    kRenderSamples = 8192;
    constexpr int    kLatency       = osa::kPartition;
    constexpr double kPi            = 3.14159265358979323846;

    int failures = 0;

    void check (const char* name, bool ok, const juce::String& detail = {})
    {
        std::printf ("  [%s] %s%s%s\n", ok ? "PASS" : "FAIL", name,
                     detail.isEmpty() ? "" : " — ",
                     detail.isEmpty() ? "" : detail.toRawUTF8());
        if (! ok)
            ++failures;
    }

    // A negative control: the inner measurement MUST fail; the verdict line passes when it
    // does (O-Strata idiom). A control that passes means the probe is vacuous.
    void checkFailsAsExpected (bool innerPassed, const char* name, const juce::String& detail = {})
    {
        std::printf ("  [%s] %s%s%s%s\n", innerPassed ? "FAIL" : "PASS", name,
                     detail.isEmpty() ? "" : " — ",
                     detail.isEmpty() ? "" : detail.toRawUTF8(),
                     innerPassed ? "  (control PASSED — the probe is vacuous)" : "  FAIL-as-expected");
        if (innerPassed)
            ++failures;
    }

    juce::String fmt (const char* f, ...)
    {
        char buf[1024];
        va_list ap;
        va_start (ap, f);
        std::vsnprintf (buf, sizeof (buf), f, ap);
        va_end (ap);
        return juce::String (buf);
    }

    double dB (double ratio) { return 20.0 * std::log10 (ratio); }

    using Proc = OSimpleAmbisonicsAudioProcessor;

    // Set a parameter by its plain (denormalised) value, through the host path.
    void setParam (Proc& p, const char* id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->convertTo0to1 (plainValue));
    }

    float rawParam (Proc& p, const char* id)
    {
        return p.apvts.getRawParameterValue (id)->load();
    }

    // Fresh instance at an explicit sample rate: optional pre-prepare parameter
    // setup, then setPlayConfigDetails + prepareToPlay.
    std::unique_ptr<Proc> makeProcAt (double sampleRate, int numIn, int maxBlock,
                                      const std::function<void (Proc&)>& preSetup = {})
    {
        auto p = std::make_unique<Proc>();
        if (preSetup)
            preSetup (*p);
        p->setPlayConfigDetails (numIn, 2, sampleRate, maxBlock);
        p->prepareToPlay (sampleRate, maxBlock);
        return p;
    }

    std::unique_ptr<Proc> makeProc (int numIn, int maxBlock,
                                    const std::function<void (Proc&)>& preSetup = {})
    {
        return makeProcAt (kFs, numIn, maxBlock, preSetup);
    }

    // The reference model's parameters, read back from the processor's raw
    // atomics so both see the same (range-snapped) values.
    RefParams refParamsFrom (Proc& p)
    {
        using namespace OSimpleAmbisonics;
        RefParams r;
        r.azimuthDeg   = rawParam (p, ParamIDs::azimuth);
        r.elevationDeg = rawParam (p, ParamIDs::elevation);
        r.distanceM    = rawParam (p, ParamIDs::distance);
        r.order        = (int) rawParam (p, ParamIDs::order) + 1;
        r.decoder      = (int) rawParam (p, ParamIDs::decoder);
        r.orbit        = rawParam (p, ParamIDs::orbit) > 0.5f;
        r.orbitRateHz  = rawParam (p, ParamIDs::orbitRate);
        return r;
    }

    RefParams refParamsFrom (const osa::Params& p)
    {
        RefParams r;
        r.azimuthDeg   = p.azimuthDeg;
        r.elevationDeg = p.elevationDeg;
        r.distanceM    = p.distanceM;
        r.order        = p.order;
        r.decoder      = p.decoder;
        r.orbit        = p.orbit;
        r.orbitRateHz  = p.orbitRateHz;
        return r;
    }

    // Position-deterministic noise: the sample at absolute index n on channel
    // ch is a pure function of (ch, n), independent of block slicing.
    float noiseAt (int ch, int n)
    {
        uint32_t h = (uint32_t) n * 2654435761u ^ (uint32_t) (ch + 1) * 40503u;
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        return (float) ((double) h / 2147483648.0 - 1.0);   // [-1, 1)
    }

    float monoNoiseAt (int n) { return 0.5f * (noiseAt (0, n) + noiseAt (1, n)); }

    // Sine generator. 100 Hz at 48 kHz = exactly 480 samples per cycle; 200 Hz =
    // 240; 12 kHz = 4 (pattern_exact_cycle_gate_needs_exactly_representable_f0).
    struct Sine
    {
        double freqHz, amp, sampleRate = kFs;
        float operator() (int n) const { return (float) (amp * std::sin (2.0 * kPi * freqHz * (double) n / sampleRate)); }
    };

    const Sine tone100 { 100.0, 0.5, kFs };

    float twoTone (int n)
    {
        return Sine { 200.0, 0.25, kFs }(n) + Sine { 12000.0, 0.25, kFs }(n);
    }

    // Render `total` samples through `p` with the given block slicing, feeding
    // `fill(ch, n)` on `numIn` input legs; output interleaved L,R per sample.
    // The buffer is always 2 wide (max(in, out)), as the host passes it.
    template <typename Fill>
    void renderWith (Proc& p, int numIn, const std::vector<int>& blockSizes,
                     Fill fill, std::vector<float>& out, int total = kRenderSamples)
    {
        juce::MidiBuffer midi;
        const int maxLen = *std::max_element (blockSizes.begin(), blockSizes.end());
        juce::AudioBuffer<float> buf (2, maxLen);

        int pos = 0, sizeIdx = 0;
        while (pos < total)
        {
            const int want = blockSizes[(size_t) sizeIdx % blockSizes.size()];
            const int len  = juce::jmin (want, total - pos);
            ++sizeIdx;

            buf.setSize (2, len, false, false, true);
            buf.clear();
            for (int ch = 0; ch < numIn; ++ch)
            {
                auto* w = buf.getWritePointer (ch);
                for (int i = 0; i < len; ++i)
                    w[i] = fill (ch, pos + i);
            }

            p.processBlock (buf, midi);

            for (int i = 0; i < len; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    out.push_back (buf.getSample (ch, i));

            pos += len;
        }
    }

    // Fixed-size blocks (default 64 — every call lands on the control grid),
    // calling hook (n) BEFORE the block that starts at absolute sample n. The
    // hook may set parameters; they take effect at that block's control step.
    // Stereo input, both legs from gen (Mono sum → exactly gen).
    template <typename Gen, typename Hook>
    std::vector<float> renderHooked (Proc& proc, int total, Gen gen, Hook hook, int block = 64)
    {
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> buf (2, block);
        std::vector<float> out;
        out.reserve ((size_t) total * 2);

        for (int n = 0; n < total; n += block)
        {
            hook (n);

            const int count = juce::jmin (block, total - n);
            buf.setSize (2, count, false, false, true);

            for (int s = 0; s < count; ++s)
            {
                const float v = gen (n + s);
                buf.setSample (0, s, v);
                buf.setSample (1, s, v);
            }

            proc.processBlock (buf, midi);

            for (int s = 0; s < count; ++s)
            {
                out.push_back (buf.getSample (0, s));
                out.push_back (buf.getSample (1, s));
            }
        }

        return out;
    }

    // Engine-direct render under ScopedNoDenormals (F6). hook (n, params) runs
    // before every block; with recordTaps the block is processed one sample at
    // a time (the grid persists across calls) and busTap[4] / [9] are logged.
    struct EngineRender
    {
        std::vector<float> lr, tap4, tap9;
    };

    template <typename Gen, typename Hook>
    EngineRender renderEngine (const osa::Params& initial, double sampleRate, int maxBlock, int total,
                               const std::vector<int>& blockSizes, Gen gen, Hook hook,
                               bool recordTaps = false)
    {
        juce::ScopedNoDenormals noDenormals;

        osa::SpatialEngine e;
        osa::Params p = initial;
        e.setParams (p);
        e.prepare (sampleRate, maxBlock);

        EngineRender r;
        r.lr.reserve ((size_t) total * 2);
        std::vector<float> mono ((size_t) maxBlock), L ((size_t) maxBlock), R ((size_t) maxBlock);

        int pos = 0, idx = 0;
        while (pos < total)
        {
            const int len = juce::jmin (blockSizes[(size_t) idx % blockSizes.size()], total - pos, maxBlock);
            ++idx;

            hook (pos, p);
            e.setParams (p);

            for (int i = 0; i < len; ++i)
                mono[(size_t) i] = gen (pos + i);

            if (recordTaps)
            {
                for (int i = 0; i < len; ++i)
                {
                    e.process (&mono[(size_t) i], &L[(size_t) i], &R[(size_t) i], 1);
                    r.tap4.push_back (e.busTap()[4]);
                    r.tap9.push_back (e.busTap()[9]);
                }
            }
            else
            {
                e.process (mono.data(), L.data(), R.data(), len);
            }

            for (int i = 0; i < len; ++i)
            {
                r.lr.push_back (L[(size_t) i]);
                r.lr.push_back (R[(size_t) i]);
            }

            pos += len;
        }

        return r;
    }

    // The same driver for the double-precision reference model.
    struct RefRender
    {
        std::vector<double> lr, tap4, tap9;
    };

    template <typename Gen, typename Hook>
    RefRender renderRef (ReferenceModel& model, const RefParams& initial, double sampleRate, int total,
                         int block, Gen gen, Hook hook, bool recordTaps = false)
    {
        RefParams p = initial;
        model.setParams (p);
        model.prepare (sampleRate);

        RefRender r;
        r.lr.reserve ((size_t) total * 2);
        std::vector<float>  mono ((size_t) block);
        std::vector<double> L ((size_t) block), R ((size_t) block);

        for (int pos = 0; pos < total; pos += block)
        {
            const int len = juce::jmin (block, total - pos);

            hook (pos, p);
            model.setParams (p);

            for (int i = 0; i < len; ++i)
                mono[(size_t) i] = gen (pos + i);

            if (recordTaps)
            {
                for (int i = 0; i < len; ++i)
                {
                    model.process (&mono[(size_t) i], &L[(size_t) i], &R[(size_t) i], 1);
                    r.tap4.push_back (model.busTap()[4]);
                    r.tap9.push_back (model.busTap()[9]);
                }
            }
            else
            {
                model.process (mono.data(), L.data(), R.data(), len);
            }

            for (int i = 0; i < len; ++i)
            {
                r.lr.push_back (L[(size_t) i]);
                r.lr.push_back (R[(size_t) i]);
            }
        }

        return r;
    }

    bool bitEqual (float a, float b)
    {
        return std::memcmp (&a, &b, sizeof (float)) == 0;
    }

    bool buffersBitEqual (const std::vector<float>& a, const std::vector<float>& b)
    {
        return a.size() == b.size()
            && std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0;
    }

    // Compare an interleaved stereo render against expected(n) on both channels,
    // with the 64-sample latency offset; also gates out[0..63] == 0 and out[64]
    // exact. Returns the first mismatching sample index or -1.
    template <typename Expected>
    int firstMismatch (const std::vector<float>& out, Expected expected, int total = kRenderSamples)
    {
        for (int n = 0; n < total; ++n)
        {
            const float e = n < kLatency ? 0.0f : expected (n - kLatency);
            for (int ch = 0; ch < 2; ++ch)
                if (! bitEqual (out[(size_t) n * 2 + (size_t) ch], e))
                    return n;
        }
        return -1;
    }

    uint64_t fnv1a64 (const std::vector<float>& v)
    {
        uint64_t h = 1469598103934665603ull;
        const auto* bytes = reinterpret_cast<const unsigned char*> (v.data());
        for (size_t i = 0; i < v.size() * sizeof (float); ++i)
        {
            h ^= bytes[i];
            h *= 1099511628211ull;
        }
        return h;
    }

    bool allFinite (const std::vector<float>& v)
    {
        for (float x : v)
            if (! std::isfinite (x))
                return false;
        return true;
    }

    //==========================================================================
    // ── Click helpers (RESEARCH-2.1 R1, O-Octagon v1.15.0 lineage) ────────────

    /** The CLICK METRIC: the worst |second difference| of one channel of a strided
        vector over [from, to).

        Second difference, not first: a sine's own per-sample step is A·ω and dwarfs a small
        discontinuity, but its curvature is A·ω² — 8.6e-5 for a 0.5 lane at 100 Hz — while a
        step of size s puts s straight into the second difference. That is what lets a low tone
        expose a click that a first-difference reading would bury under the programme's own
        slope (adapted from O-Octagon main.cpp:973 to the harness's interleaved vectors). */
    template <typename T>
    double worstCurvature (const std::vector<T>& x, int ch, int stride, int from, int to) noexcept
    {
        double worst = 0.0;
        const int count = (int) x.size() / stride;

        for (int n = std::max (2, from); n < std::min (to, count); ++n)
        {
            const double a = (double) x[(size_t) (n * stride + ch)];
            const double b = (double) x[(size_t) ((n - 1) * stride + ch)];
            const double c = (double) x[(size_t) ((n - 2) * stride + ch)];
            worst = std::max (worst, std::abs (a - 2.0 * b + c));
        }

        return worst;
    }

    /** A click ratio: the event render's worst curvature over the event window, against a
        MATCHED CONTROL render — the same tone and gain staging with the stage under test
        defeated (parked at the direction of maximum level, or the reference model's own
        ramp). A ratio near 1 says the stage adds nothing of its own. */
    struct ClickReading
    {
        double event { 0.0 }, control { 0.0 };

        double ratio() const noexcept { return control > 1.0e-300 ? event / control : 0.0; }
    };

    /** The ceiling every click probe asserts (O-Octagon kClickRatioCeiling). The stage under
        test may add AT MOST as much curvature again as the matched control already carries —
        a snapped (per-grid, un-lerped) coefficient vector exceeds it by 24× on a 100 Hz tone
        at 64-sample blocks (RESEARCH-2.1 R1 table), and a fixed transition sits close to 1. */
    constexpr double kClickRatioCeiling = 2.0;

    juce::String clickDetail (const ClickReading& r, const juce::String& extra = {})
    {
        return fmt ("event %.4e, control %.4e, ratio %.4f (ceiling %.1f)",
                    r.event, r.control, r.ratio(), kClickRatioCeiling) + extra;
    }

    void clickVerdict (const char* name, const ClickReading& r, const juce::String& extra = {})
    {
        check (name, r.ratio() <= kClickRatioCeiling, clickDetail (r, extra));
    }

    // Exact-bin DFT magnitude, normalised so a sine of amplitude A reads A:
    // (2/len)·|Σ x[from+i]·e^{−2πi·cycles·i/len}|. `cycles` must be an integer
    // number of cycles over `len` samples for a leakage-free reading.
    template <typename T>
    double binMagnitude (const std::vector<T>& x, int ch, int stride, int from, int len, int cycles)
    {
        double re = 0.0, im = 0.0;
        for (int i = 0; i < len; ++i)
        {
            const double v  = (double) x[(size_t) ((from + i) * stride + ch)];
            const double ph = -2.0 * kPi * (double) cycles * (double) i / (double) len;
            re += v * std::cos (ph);
            im += v * std::sin (ph);
        }
        return 2.0 * std::sqrt (re * re + im * im) / (double) len;
    }

    template <typename T>
    double energy (const std::vector<T>& x, int ch, int stride, int from, int to)
    {
        double e = 0.0;
        for (int n = from; n < to; ++n)
        {
            const double v = (double) x[(size_t) (n * stride + ch)];
            e += v * v;
        }
        return e;
    }

    template <typename T>
    double rms (const std::vector<T>& x, int ch, int stride, int from, int to)
    {
        return std::sqrt (energy (x, ch, stride, from, to) / (double) (to - from));
    }

    // Allocation-liveness sink: the pointer escapes through an atomic so the
    // new/delete pair cannot be elided.
    std::atomic<void*> livenessSink { nullptr };
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    using namespace OSimpleAmbisonics;

    std::printf ("O-simpleAmbisonics render-harness (Stage 2 / Phase 2.1) — fs=%.0f, latency=%d\n",
                 kFs, kLatency);

    //==========================================================================
    // P0 — parameter contract (ARCHITECTURE.md §Parameter Mapping).
    {
        auto p = makeProc (2, kDefaultBlock);

        struct FloatSpec { const char* id; float lo, hi, def; bool skewed; };
        const FloatSpec floats[] = {
            { ParamIDs::azimuth,     -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::elevation,    -90.0f,  90.0f, 0.0f, false },
            { ParamIDs::distance,       0.2f,  20.0f, 1.0f, true  },
            { ParamIDs::yaw,         -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::pitch,       -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::roll,        -180.0f, 180.0f, 0.0f, false },
            { ParamIDs::orbitRate,     0.02f,   2.0f, 0.1f, true  },
            { ParamIDs::outputLevel,  -60.0f,   0.0f, 0.0f, false },
        };

        for (const auto& f : floats)
        {
            auto* fp = dynamic_cast<juce::AudioParameterFloat*> (p->apvts.getParameter (f.id));
            check ((juce::String ("P0 ") + f.id + " exists+float").toRawUTF8(), fp != nullptr);
            if (fp == nullptr)
                continue;

            check ((juce::String ("P0 ") + f.id + " range").toRawUTF8(),
                   juce::approximatelyEqual (fp->range.start, f.lo)
                       && juce::approximatelyEqual (fp->range.end, f.hi),
                   juce::String (fp->range.start) + ".." + juce::String (fp->range.end));
            check ((juce::String ("P0 ") + f.id + " default").toRawUTF8(),
                   juce::approximatelyEqual (fp->get(), f.def), juce::String (fp->get()));
            check ((juce::String ("P0 ") + f.id + (f.skewed ? " skewed" : " linear")).toRawUTF8(),
                   f.skewed ? ! juce::approximatelyEqual (fp->range.skew, 1.0f)
                            :   juce::approximatelyEqual (fp->range.skew, 1.0f),
                   juce::String (fp->range.skew));
        }

        struct ChoiceSpec { const char* id; int count; const char* first; };
        const ChoiceSpec choices[] = {
            { ParamIDs::order,     3, "1" },
            { ParamIDs::decoder,   2, "Binaural" },
            { ParamIDs::inputMode, 2, "Mono sum" },
        };

        for (const auto& c : choices)
        {
            auto* cp = dynamic_cast<juce::AudioParameterChoice*> (p->apvts.getParameter (c.id));
            check ((juce::String ("P0 ") + c.id + " exists+choice").toRawUTF8(), cp != nullptr);
            if (cp == nullptr)
                continue;
            check ((juce::String ("P0 ") + c.id + " choices").toRawUTF8(),
                   cp->choices.size() == c.count, juce::String (cp->choices.size()));
            check ((juce::String ("P0 ") + c.id + " default").toRawUTF8(),
                   cp->getIndex() == 0 && cp->choices[0] == c.first, cp->getCurrentChoiceName());
        }

        auto* orbit = dynamic_cast<juce::AudioParameterBool*> (p->apvts.getParameter (ParamIDs::orbit));
        check ("P0 orbit exists+bool", orbit != nullptr);
        if (orbit != nullptr)
            check ("P0 orbit default off", orbit->get() == false);

        int count = 0;
        for (auto* param : p->getParameters())
            if (dynamic_cast<juce::RangedAudioParameter*> (param) != nullptr)
                ++count;
        check ("P0 exactly 12 parameters", count == 12, juce::String (count));

        check ("P0 latency == kPartition == 64",
               p->getLatencySamples() == kLatency && kLatency == 64,
               juce::String (p->getLatencySamples()));
    }

    //==========================================================================
    // P1 null — processor vs the double-precision ReferenceModel at a static,
    // non-trivial setting (D8 / F9): az 37°, el −12°, d 3.3 m, order 3, Stereo.
    std::vector<float> nullRender;
    nullRender.reserve ((size_t) kRenderSamples * 2);
    {
        auto p = makeProc (2, kDefaultBlock, [] (Proc& q)
        {
            setParam (q, ParamIDs::azimuth,   37.0f);
            setParam (q, ParamIDs::elevation, -12.0f);
            setParam (q, ParamIDs::distance,  3.3f);
            setParam (q, ParamIDs::order,     2.0f);   // index 2 = "3"
            setParam (q, ParamIDs::decoder,   1.0f);   // Stereo
        });
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, nullRender);

        ReferenceModel model;
        model.setParams (refParamsFrom (*p));
        model.prepare (kFs);

        std::vector<float>  mono ((size_t) kRenderSamples);
        std::vector<double> L ((size_t) kRenderSamples), R ((size_t) kRenderSamples);
        for (int n = 0; n < kRenderSamples; ++n)
            mono[(size_t) n] = monoNoiseAt (n);
        model.process (mono.data(), L.data(), R.data(), kRenderSamples);

        double maxErr = 0.0, maxAbs = 0.0;
        for (int n = 0; n < kRenderSamples; ++n)
        {
            maxErr = std::max (maxErr, std::abs ((double) nullRender[(size_t) n * 2]     - L[(size_t) n]));
            maxErr = std::max (maxErr, std::abs ((double) nullRender[(size_t) n * 2 + 1] - R[(size_t) n]));
            maxAbs = std::max (maxAbs, std::abs (L[(size_t) n]));
        }
        check ("P1 null: processor vs ReferenceModel max|err| < 1e-5 (az 37, el -12, d 3.3, order 3, Stereo)",
               maxErr < 1.0e-5 && maxAbs > 0.01,
               fmt ("max|err| %.3e over %d samples x 2 ch, peak |L| %.3f", maxErr, kRenderSamples, maxAbs));
    }

    //==========================================================================
    // P1 anchor — front / 1 m / order 1: out == 0.75f·s[n−64] BIT-EQUAL in both
    // decoder modes (F2 / F3) and engine-direct. `flat` (defaults, Binaural)
    // is the P2 / P3 reference.
    std::vector<float> flat;
    flat.reserve ((size_t) kRenderSamples * 2);
    const auto anchorExpected = [] (int n) { return 0.75f * monoNoiseAt (n); };
    {
        auto p = makeProc (2, kDefaultBlock);

        // Evidence for the fillParams snap: the APVTS raw default of the skewed,
        // stepped distance range is NOT snapped on construction (JUCE legacy).
        {
            auto* dp = dynamic_cast<juce::AudioParameterFloat*> (p->apvts.getParameter (ParamIDs::distance));
            const float raw = rawParam (*p, ParamIDs::distance);
            std::printf ("  info: distance raw default %.9g (APVTS, unsnapped), snapToLegalValue → %.9g, g(raw) %.9g\n",
                         (double) raw, (double) dp->range.snapToLegalValue (raw), (double) osa::DistanceModel::gainFor (raw));
        }

        renderWith (*p, 2, { kDefaultBlock }, noiseAt, flat);

        const int mm = firstMismatch (flat, anchorExpected);
        check ("P1 anchor Binaural (default): out == 0.75f*s[n-64] bit-equal both ch", mm < 0, juce::String (mm));

        bool headZero = true;
        for (int n = 0; n < kLatency && headZero; ++n)
            headZero = flat[(size_t) n * 2] == 0.0f && flat[(size_t) n * 2 + 1] == 0.0f;
        check ("P1 anchor out[0..63] == 0", headZero);

        check ("P1 anchor out[64] exact (no fade-in)",
               bitEqual (flat[(size_t) kLatency * 2], anchorExpected (0))
                   && bitEqual (flat[(size_t) kLatency * 2 + 1], anchorExpected (0)),
               juce::String (flat[(size_t) kLatency * 2], 7) + " vs " + juce::String (anchorExpected (0), 7));
    }
    {
        auto p = makeProc (2, kDefaultBlock, [] (Proc& q) { setParam (q, ParamIDs::decoder, 1.0f); });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, anchorExpected);
        check ("P1 anchor Stereo: out == 0.75f*s[n-64] bit-equal both ch", mm < 0, juce::String (mm));
        check ("P1 anchor Stereo == Binaural render bit-equal (A14 placeholder exact in both modes)",
               buffersBitEqual (out, flat));
    }
    {
        osa::Params defaults;
        const auto r = renderEngine (defaults, kFs, kDefaultBlock, kRenderSamples, { kDefaultBlock },
                                     monoNoiseAt, [] (int, osa::Params&) {});
        const int mm = firstMismatch (r.lr, anchorExpected);
        check ("P1 anchor engine-direct: out == 0.75f*s[n-64] bit-equal", mm < 0, juce::String (mm));
    }

    //==========================================================================
    // P1b — Left only (stereo in), then a mono input bus — both with the 0.75 factor.
    {
        auto p = makeProc (2, kDefaultBlock,
                           [] (Proc& q) { setParam (q, ParamIDs::inputMode, 1.0f); });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, [] (int n) { return 0.75f * noiseAt (0, n); });
        check ("P1b Left only == 0.75f*L[n-64] on both ch", mm < 0, juce::String (mm));
    }
    {
        auto p = makeProc (1, kDefaultBlock);   // mono input bus, stereo out
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 1, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, [] (int n) { return 0.75f * noiseAt (0, n); });
        check ("P1b mono-in == 0.75f*in0[n-64] on both ch", mm < 0, juce::String (mm));
    }

    //==========================================================================
    // P2 — ragged block sizes bit-identical to the flat 512 feed (defaults).
    {
        auto p = makeProc (2, 4096);
        std::vector<float> ragged;
        ragged.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { 1, 7, 64, 333, 4096, 13, 512, 2 }, noiseAt, ragged);
        check ("P2 ragged == flat", buffersBitEqual (ragged, flat));
    }

    //==========================================================================
    // P2b — the D5 amendment pinned: orbit On 2 Hz, order 3, Stereo, az 45°,
    // d 2.5 m — parameters MOVING, still bit-equal across ragged slicing.
    const auto movingSetup = [] (Proc& q)
    {
        setParam (q, ParamIDs::azimuth,   45.0f);
        setParam (q, ParamIDs::distance,  2.5f);
        setParam (q, ParamIDs::order,     2.0f);
        setParam (q, ParamIDs::decoder,   1.0f);
        setParam (q, ParamIDs::orbit,     1.0f);
        setParam (q, ParamIDs::orbitRate, 2.0f);
    };
    std::vector<float> flatMoving;
    flatMoving.reserve ((size_t) kRenderSamples * 2);
    {
        auto p = makeProc (2, kDefaultBlock, movingSetup);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, flatMoving);

        // Liveness: the orbit must actually move the image within the render.
        const double e1 = energy (flatMoving, 0, 2, 64, 4096), e2 = energy (flatMoving, 0, 2, 4096, 8192);
        check ("P2b liveness: orbit changes L energy between halves", std::abs (10.0 * std::log10 (e2 / e1)) > 0.5,
               fmt ("%.2f dB", 10.0 * std::log10 (e2 / e1)));

        auto q = makeProc (2, 4096, movingSetup);
        std::vector<float> ragged;
        ragged.reserve ((size_t) kRenderSamples * 2);
        renderWith (*q, 2, { 1, 7, 64, 333, 4096, 13, 512, 2 }, noiseAt, ragged);
        check ("P2b ragged == flat with orbit On 2 Hz / order 3 (control grid)", buffersBitEqual (ragged, flatMoving));
    }

    //==========================================================================
    // P3 — oversized host block: prepared for 64, fed 4096-sample blocks.
    {
        auto small = makeProc (2, 64);
        std::vector<float> outSmall;
        outSmall.reserve ((size_t) kRenderSamples * 2);
        renderWith (*small, 2, { 4096 }, noiseAt, outSmall);

        auto big = makeProc (2, 4096);
        std::vector<float> outBig;
        outBig.reserve ((size_t) kRenderSamples * 2);
        renderWith (*big, 2, { 4096 }, noiseAt, outBig);

        check ("P3 oversized block (prep 64, feed 4096) == prep 4096",
               buffersBitEqual (outSmall, outBig));
        check ("P3 oversized == flat", buffersBitEqual (outBig, flat));
    }

    //==========================================================================
    // P3b — the same with parameters moving (orbit On 2 Hz, order 3).
    {
        auto small = makeProc (2, 64, movingSetup);
        std::vector<float> outSmall;
        outSmall.reserve ((size_t) kRenderSamples * 2);
        renderWith (*small, 2, { 4096 }, noiseAt, outSmall);

        auto big = makeProc (2, 4096, movingSetup);
        std::vector<float> outBig;
        outBig.reserve ((size_t) kRenderSamples * 2);
        renderWith (*big, 2, { 4096 }, noiseAt, outBig);

        check ("P3b oversized (prep 64, feed 4096) == prep 4096 with orbit On", buffersBitEqual (outSmall, outBig));
        check ("P3b oversized == flat with orbit On", buffersBitEqual (outBig, flatMoving));
    }

    //==========================================================================
    // P4 — latency measured: impulse at 100 → first nonzero at 100 + latency,
    // amplitude exactly 0.75f (front / 1 m / order 1 anchor).
    {
        auto p = makeProc (2, kDefaultBlock);
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock },
                    [] (int, int n) { return n == 100 ? 1.0f : 0.0f; }, out);

        int firstNonZero = -1;
        for (int n = 0; n < kRenderSamples && firstNonZero < 0; ++n)
            if (out[(size_t) n * 2] != 0.0f || out[(size_t) n * 2 + 1] != 0.0f)
                firstNonZero = n;

        const int reported = p->getLatencySamples();
        check ("P4 measured latency == reported", firstNonZero == 100 + reported,
               juce::String ("first nonzero ") + juce::String (firstNonZero)
                   + ", reported " + juce::String (reported));
        check ("P4 impulse amplitude exactly 0.75f", firstNonZero >= 0
                   && bitEqual (out[(size_t) firstNonZero * 2], 0.75f)
                   && bitEqual (out[(size_t) firstNonZero * 2 + 1], 0.75f),
               firstNonZero >= 0 ? juce::String (out[(size_t) firstNonZero * 2], 7) : juce::String ("n/a"));
    }

    //==========================================================================
    // P5 — silence floor: outputLevel = −60 before prepare → all zeros.
    {
        auto p = makeProc (2, kDefaultBlock,
                           [] (Proc& q) { setParam (q, ParamIDs::outputLevel, -60.0f); });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out);

        bool allZero = true;
        for (float v : out)
            if (v != 0.0f) { allZero = false; break; }
        check ("P5 -60 dB == exact silence", allZero);
    }

    //==========================================================================
    // DSP-01 — encoder vs the generic SN3D table (R4): 13 × 7 grid.
    {
        double maxErr = 0.0;
        bool   wIsOne = true;
        for (int ia = 0; ia <= 12; ++ia)
            for (int ie = 0; ie <= 6; ++ie)
            {
                const float azDeg = -180.0f + 30.0f * (float) ia;
                const float elDeg = -90.0f + 30.0f * (float) ie;
                float  y[16];
                double ref[16];
                osa::encodeSH16 (azDeg * osa::kDegToRad, elDeg * osa::kDegToRad, y);
                ReferenceModel::refSh16 ((double) azDeg, (double) elDeg, ref);
                for (int i = 0; i < 16; ++i)
                    maxErr = std::max (maxErr, std::abs ((double) y[i] - ref[i]));
                wIsOne = wIsOne && y[0] == 1.0f;
            }
        check ("DSP-01 encodeSH16 vs generic SN3D table: max|err| < 1e-5 on 13x7 grid", maxErr < 1.0e-5,
               fmt ("max|err| %.3e (gate 1e-5)", maxErr));
        check ("DSP-01 W == 1.0f at every grid point", wIsOne);

        float left[16], front[16];
        osa::encodeSH16 (90.0f * osa::kDegToRad, 0.0f, left);
        osa::encodeSH16 (0.0f, 0.0f, front);
        check ("DSP-01 sign anchor: ACN 1 (Y) > 0 at az +90 (AmbiX left-positive)", left[1] > 0.0f,
               fmt ("Y %.6f", (double) left[1]));
        check ("DSP-01 Appendix-B front: ACN 13 = -0.6124, ACN 15 = +0.7906 (1e-4)",
               std::abs (front[13] + 0.6124f) < 1.0e-4f && std::abs (front[15] - 0.7906f) < 1.0e-4f,
               fmt ("%.4f / %.4f", (double) front[13], (double) front[15]));
        check ("DSP-01 Appendix-B left: ACN 9 = -0.7906, ACN 11 = -0.6124 (1e-4)",
               std::abs (left[9] + 0.7906f) < 1.0e-4f && std::abs (left[11] + 0.6124f) < 1.0e-4f,
               fmt ("%.4f / %.4f", (double) left[9], (double) left[11]));
    }

    //==========================================================================
    // DSP-04 — ±60° virtual-cardioid pair.
    {
        float y[16], L, R;

        osa::encodeSH16 (0.0f, 0.0f, y);
        osa::StereoDecoder::decode (y, L, R);
        check ("DSP-04 front: L == R (bit)", bitEqual (L, R), fmt ("L %.7f R %.7f", (double) L, (double) R));

        osa::encodeSH16 (90.0f * osa::kDegToRad, 0.0f, y);
        osa::StereoDecoder::decode (y, L, R);
        const double sep = dB ((double) L / (double) R);
        check ("DSP-04 +90 deg: 20log10(L/R) >= 20 dB and L > R", sep >= 20.0 && L > R,
               fmt ("L %.4f R %.4f = %.2f dB (expect 22.9)", (double) L, (double) R, sep));

        bool poles = true;
        juce::String poleDetail;
        for (float e : { 90.0f, -90.0f })
            for (float a : { 0.0f, 180.0f })
            {
                osa::encodeSH16 (a * osa::kDegToRad, e * osa::kDegToRad, y);
                osa::StereoDecoder::decode (y, L, R);
                poles = poles && L == 0.5f && R == 0.5f;
                poleDetail << fmt ("(az %.0f el %.0f: %.8f/%.8f) ", (double) a, (double) e, (double) L, (double) R);
            }
        check ("DSP-04 e = +/-90 deg: L == R == 0.5f (bit, az 0 / 180)", poles, poleDetail);

        bool nonNeg = true;
        float minV = 1.0f;
        for (int ia = 0; ia < 36; ++ia)
            for (float e : { -60.0f, 0.0f, 60.0f })
            {
                const float a = -180.0f + 10.0f * (float) ia;
                osa::encodeSH16 (a * osa::kDegToRad, e * osa::kDegToRad, y);
                osa::StereoDecoder::decode (y, L, R);
                nonNeg = nonNeg && L >= 0.0f && R >= 0.0f;
                minV = std::min ({ minV, L, R });
            }
        check ("DSP-04 72 directions: L >= 0 && R >= 0 (in-phase everywhere)", nonNeg,
               fmt ("min %.6f", (double) minV));
    }
    {
        // Processor in Stereo mode, az +90°, 100 Hz tone: rms(L)/rms(R) ≥ 20 dB.
        auto p = makeProc (2, kDefaultBlock, [] (Proc& q)
        {
            setParam (q, ParamIDs::azimuth, 90.0f);
            setParam (q, ParamIDs::decoder, 1.0f);
        });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, [] (int, int n) { return tone100 (n); }, out);
        const double sep = dB (rms (out, 0, 2, 1024, kRenderSamples) / rms (out, 1, 2, 1024, kRenderSamples));
        check ("DSP-04 processor Stereo at +90 deg: rms(L)/rms(R) >= 20 dB", sep >= 20.0,
               fmt ("%.2f dB", sep));
    }
    {
        // Correlation of L and R with the delayed source ≥ 0 over a direction set,
        // normalised by the source energy. The cardioid has an ANALYTIC null at
        // az = ∓120° (e = 0): V = 0.5 + 0.5·cos(180°) = 0, where the float
        // pattern value is a ±1e-8 residual — so the floor is −1e-6 of the
        // source energy, not a slack on the in-phase claim (the first run read
        // −3e-9 there).
        double minCorr = 1.0e9;
        bool ok = true;
        for (int ia = 0; ia < 12; ++ia)
            for (float e : { -60.0f, 0.0f, 60.0f })
            {
                osa::Params prm;
                prm.azimuthDeg   = -180.0f + 30.0f * (float) ia;
                prm.elevationDeg = e;
                prm.decoder      = 1;
                const auto r = renderEngine (prm, kFs, 512, 2048, { 512 },
                                             [] (int n) { return tone100 (n); }, [] (int, osa::Params&) {});
                double cL = 0.0, cR = 0.0, ss = 0.0;
                for (int n = kLatency; n < 2048; ++n)
                {
                    const double s = (double) tone100 (n - kLatency);
                    cL += (double) r.lr[(size_t) n * 2] * s;
                    cR += (double) r.lr[(size_t) n * 2 + 1] * s;
                    ss += s * s;
                }
                minCorr = std::min ({ minCorr, cL / ss, cR / ss });
                ok = ok && cL / ss >= -1.0e-6 && cR / ss >= -1.0e-6;
            }
        check ("DSP-04 normalised correlation of L/R with delayed source >= -1e-6 over 36 directions (analytic null at -/+120)", ok,
               fmt ("min %.4e (analytic minimum 0)", minCorr));
    }

    //==========================================================================
    // DSP-05 — distance = level + air (R5 / A9).
    {
        constexpr int settle = 4800, win = 48000, total = settle + win;

        std::vector<float> ds;
        for (int i = 0; i < 25; ++i)
            ds.push_back ((float) (0.2 * std::pow (100.0, (double) i / 24.0)));
        ds.push_back (1.0f);
        std::sort (ds.begin(), ds.end());

        std::vector<double> levelDb, ratioDb;
        bool   nearBitExact = true;
        double maxNearRatio = 0.0;
        int    firstBitMismatch = -1;

        for (float d : ds)
        {
            osa::Params prm;
            prm.distanceM = d;
            prm.decoder   = 1;
            const auto r = renderEngine (prm, kFs, 512, total, { 512 }, twoTone, [] (int, osa::Params&) {});

            const double a200 = binMagnitude (r.lr, 0, 2, settle, win, 200);
            const double a12k = binMagnitude (r.lr, 0, 2, settle, win, 12000);
            levelDb.push_back (dB (a200 / (0.25 * 0.75)));
            ratioDb.push_back (dB (a12k / a200));

            if (d <= 1.0f)
            {
                // Bit-level claim: m == 0 → y == g·x exactly, so the output is
                // the front/order-1 anchor 0.75f·(g·x[n−64]) sample for sample.
                const float gd = osa::DistanceModel::gainFor (d);
                const int mm = firstMismatch (r.lr, [gd] (int n) { return 0.75f * (gd * twoTone (n)); }, total);
                if (mm >= 0 && firstBitMismatch < 0)
                    firstBitMismatch = mm;
                nearBitExact = nearBitExact && mm < 0;
                maxNearRatio = std::max (maxNearRatio, std::abs (ratioDb.back()));
            }
        }

        bool levelDec = true, ratioDec = true;
        size_t idxOne = 0;
        for (size_t i = 0; i < ds.size(); ++i)
            if (ds[i] == 1.0f)
                idxOne = i;
        for (size_t i = 1; i < ds.size(); ++i)
            levelDec = levelDec && levelDb[i] < levelDb[i - 1];
        for (size_t i = idxOne + 1; i < ds.size(); ++i)
            ratioDec = ratioDec && ratioDb[i] < ratioDb[i - 1];

        juce::String table;
        for (size_t i = 0; i < ds.size(); ++i)
            table << fmt ("%.3g:%.2f/%.2f ", (double) ds[i], levelDb[i], ratioDb[i]);
        std::printf ("  DSP-05 table (d m : level dB / HF-LF dB): %s\n", table.toRawUTF8());

        check ("DSP-05 level (200 Hz) strictly decreasing over 26 distances in [0.2, 20]", levelDec);
        check ("DSP-05 d <= 1 m: output bit-equal to 0.75f*(g(d)*x[n-64]) (wet mix exactly 0)", nearBitExact,
               fmt ("10 distances, first mismatch %d, measured |HF/LF| max %.2e dB", firstBitMismatch, maxNearRatio));
        check ("DSP-05 HF/LF strictly decreasing on (1, 20]", ratioDec);
        check ("DSP-05 anchor HF/LF at 20 m = -10.788 dB (0.1)", std::abs (ratioDb.back() + 10.788) < 0.1,
               fmt ("%.3f dB", ratioDb.back()));
        check ("DSP-05 anchor level at 0.2 m = +8.87 dB (0.1)", std::abs (levelDb.front() - 8.87) < 0.1,
               fmt ("%.3f dB", levelDb.front()));
        check ("DSP-05 anchor level at 1 m = 0.00 dB (0.1)", std::abs (levelDb[idxOne]) < 0.1,
               fmt ("%.4f dB", levelDb[idxOne]));
        check ("DSP-05 anchor level at 20 m = -24.19 dB (0.1)", std::abs (levelDb.back() + 24.19) < 0.1,
               fmt ("%.3f dB", levelDb.back()));
        check ("DSP-05 liveness: HF/LF at 20 m < -5 dB (a dead distance path reads 0)", ratioDb.back() < -5.0,
               fmt ("%.3f dB", ratioDb.back()));
    }
    {
        // Clamp TARGET (pattern_downstream_guard_masks_defect_gate_the_clamp_target).
        const float c441 = osa::DistanceModel::cutoffFor (1.0f, 44100.0);
        const float c48  = osa::DistanceModel::cutoffFor (1.0f, 48000.0);
        check ("DSP-05 clamp: cutoffFor(1 m, 44100) == 0.45*44100 == 19845 exactly", c441 == 19845.0f,
               fmt ("%.1f Hz", (double) c441));
        check ("DSP-05 clamp: cutoffFor(1 m, 48000) == 20000 exactly", c48 == 20000.0f,
               fmt ("%.1f Hz", (double) c48));

        osa::DistanceModel dm;
        dm.prepare (44100.0);
        dm.snapTo (1.0f);
        const float inst1 = dm.clampedCutoffHz();
        dm.snapTo (0.2f);
        const float inst02 = dm.clampedCutoffHz();
        check ("DSP-05 clamp: clampedCutoffHz() == 19845 at d = 1.0 and d = 0.2 / fs 44100", inst1 == 19845.0f && inst02 == 19845.0f,
               fmt ("%.1f / %.1f Hz", (double) inst1, (double) inst02));

        // Clamp engaged in the AUDIBLE path: fs 22050, d = 1.5 (m = 0.5, f_c request 16330 → 9922.5).
        constexpr double fsLow = 22050.0;
        constexpr int settle = 2205, win = 22050, total = settle + win;
        osa::Params prm;
        prm.distanceM = 1.5f;
        prm.decoder   = 1;
        const auto r = renderEngine (prm, fsLow, 512, total, { 512 },
                                     [] (int n) { return Sine { 200.0, 0.25, fsLow }(n) + Sine { 4000.0, 0.25, fsLow }(n); },
                                     [] (int, osa::Params&) {});
        const double a200 = binMagnitude (r.lr, 0, 2, settle, win, 200);
        const double a4k  = binMagnitude (r.lr, 0, 2, settle, win, 4000);
        check ("DSP-05 clamp engaged at fs 22050 / d 1.5: all finite, HF/LF (4 kHz - 200 Hz) < 0 dB",
               allFinite (r.lr) && dB (a4k / a200) < 0.0,
               fmt ("clamp target %.1f Hz, HF/LF %.3f dB", (double) osa::DistanceModel::cutoffFor (1.5f, fsLow), dB (a4k / a200)));
    }

    //==========================================================================
    // DSP-06a — shortest-path trajectory on the control grid.
    {
        const auto trajectoryOk = [] (const std::vector<float>& log, float target, const char* name)
        {
            bool near180 = true, awayFrom0 = true, converged = true;
            float worst = 0.0f;
            for (size_t i = 0; i < log.size(); ++i)
            {
                const float v  = log[i];
                const float dv = std::abs (osa::wrapTo180 (v - 180.0f));
                worst = std::max (worst, dv);
                near180   = near180 && dv <= 10.0f + 1.0e-3f;
                awayFrom0 = awayFrom0 && std::abs (v) >= 90.0f;
                if (i >= 150)   // 0.2 s at 64-sample steps
                    converged = converged && std::abs (osa::wrapTo180 (v - target)) <= 1.0e-3f;
            }
            check (name, near180 && awayFrom0 && converged,
                   fmt ("%d steps, max |wrap(az-180)| %.3f deg, last %.5f (target %.0f), residual at 0.2 s %.2e",
                        (int) log.size(), (double) worst, (double) log.back(), (double) target,
                        (double) std::abs (osa::wrapTo180 (log[150] - target))));
        };

        juce::ScopedNoDenormals noDenormals;
        std::vector<float> mono (64, 0.1f), L (64), R (64);

        // +170 → −170.
        {
            osa::SpatialEngine e;
            osa::Params prm;
            prm.azimuthDeg = 170.0f;
            e.setParams (prm);
            e.prepare (kFs, 64);
            prm.azimuthDeg = -170.0f;
            e.setParams (prm);
            std::vector<float> log;
            for (int b = 0; b < 188; ++b)
            {
                e.process (mono.data(), L.data(), R.data(), 64);
                log.push_back (e.frame().azEffDeg);
            }
            trajectoryOk (log, -170.0f, "DSP-06a +170 -> -170: every step within 10 deg of 180, never |az| < 90, converged by 0.2 s");
        }

        // Orbit Off → On resets to the puck.
        {
            osa::SpatialEngine e;
            osa::Params prm;
            prm.azimuthDeg  = 30.0f;
            prm.orbitRateHz = 2.0f;
            e.setParams (prm);
            e.prepare (kFs, 64);

            prm.orbit = true;                               // On for 0.15 s → phase 0.30, az_eff → ~138
            e.setParams (prm);
            for (int b = 0; b < 113; ++b)
                e.process (mono.data(), L.data(), R.data(), 64);
            const float farAz = e.frame().azEffDeg;

            prm.orbit = false;                              // Off for 0.5 s → glides back to 30 (snaps)
            e.setParams (prm);
            for (int b = 0; b < 375; ++b)
                e.process (mono.data(), L.data(), R.data(), 64);
            const float backAz = e.frame().azEffDeg;

            prm.orbit = true;                               // On again: phase resets → departs from 30
            e.setParams (prm);
            e.process (mono.data(), L.data(), R.data(), 64);
            const float firstAz = e.frame().azEffDeg;

            check ("DSP-06a orbit Off->On departs from the puck (first frame within 1 deg of azimuth)",
                   farAz > 100.0f && std::abs (backAz - 30.0f) <= 1.0e-3f && std::abs (firstAz - 30.0f) <= 1.0f,
                   fmt ("after 0.15 s On: %.2f; after 0.5 s Off: %.5f; first frame after re-On: %.4f",
                        (double) farAz, (double) backAz, (double) firstAz));
        }

        // On → Off from az_eff ≈ +170 with azimuth = −170 glides through 180.
        {
            osa::SpatialEngine e;
            osa::Params prm;
            prm.azimuthDeg  = -170.0f;
            prm.orbitRateHz = 2.0f;
            prm.orbit       = true;
            e.setParams (prm);
            e.prepare (kFs, 64);

            int   steps = 0;
            float az    = -170.0f;
            while (steps < 600 && ! (az >= 170.0f && az < 180.0f))
            {
                e.process (mono.data(), L.data(), R.data(), 64);
                az = e.frame().azEffDeg;
                ++steps;
            }

            prm.orbit = false;
            e.setParams (prm);
            std::vector<float> log;
            for (int b = 0; b < 188; ++b)
            {
                e.process (mono.data(), L.data(), R.data(), 64);
                log.push_back (e.frame().azEffDeg);
            }
            std::printf ("  DSP-06a On->Off: orbit reached %.3f deg after %d steps\n", (double) az, steps);
            trajectoryOk (log, -170.0f, "DSP-06a orbit On->Off from +170 to azimuth -170 glides through 180");
        }
    }

    //==========================================================================
    // DSP-06b — full-circle sweep click gate (R1 / A10).
    {
        constexpr int skip  = 64 + 960;          // delay fill + 20 ms
        constexpr int total = skip + 48000;      // one full circle at 1 rev/s after the skip
        const auto toneGen = [] (int n) { return tone100 (n); };

        // Matched controls: parked at +60° (L carries the largest legitimate
        // curvature) and −60° (R).
        const auto parked = [&] (float azDeg)
        {
            auto p = makeProc (2, 64, [azDeg] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth, azDeg);
                setParam (q, ParamIDs::order,   2.0f);
                setParam (q, ParamIDs::decoder, 1.0f);
            });
            return renderHooked (*p, total, toneGen, [] (int) {});
        };
        const auto ctrlL = parked (60.0f);
        const auto ctrlR = parked (-60.0f);
        const double controlL = worstCurvature (ctrlL, 0, 2, skip, total);
        const double controlR = worstCurvature (ctrlR, 1, 2, skip, total);

        const auto verdictPair = [&] (const char* name, const std::vector<float>& ev, const juce::String& extra)
        {
            ClickReading rL { worstCurvature (ev, 0, 2, skip, total), controlL };
            ClickReading rR { worstCurvature (ev, 1, 2, skip, total), controlR };
            clickVerdict ((juce::String (name) + " [L]").toRawUTF8(), rL, extra);
            clickVerdict ((juce::String (name) + " [R]").toRawUTF8(), rR);
        };

        const auto livenessDb = [&] (const std::vector<float>& ev)
        {
            const double e1 = energy (ev, 0, 2, skip, skip + 24000), e2 = energy (ev, 0, 2, skip + 24000, total);
            return 10.0 * std::log10 (e2 / e1);
        };

        // (i) hook-driven at 64-sample blocks: +0.48°/block through ±180.
        {
            auto p = makeProc (2, 64, [] (Proc& q)
            {
                setParam (q, ParamIDs::order,   2.0f);
                setParam (q, ParamIDs::decoder, 1.0f);
            });
            const auto ev = renderHooked (*p, total, toneGen, [&p] (int n)
            {
                setParam (*p, ParamIDs::azimuth, osa::wrapTo180 (-180.0f + 0.48f * (float) (n / 64)));
            });
            verdictPair ("DSP-06b hook-driven sweep, 64-sample blocks, +0.48 deg/block", ev, {});

            const double live = livenessDb (ev);
            check ("DSP-06b liveness: L energy second half-circle vs first differs >= 6 dB", std::abs (live) >= 6.0,
                   fmt ("%.2f dB", live));

            // Informational: energy outside 100 Hz ± 2 bins over the circle (Parseval).
            {
                const double tot = energy (ev, 0, 2, skip, total);
                double inBand = 0.0;
                for (int k = 98; k <= 102; ++k)
                {
                    const double a = binMagnitude (ev, 0, 2, skip, 48000, k);   // amplitude → energy = a²·N/2
                    inBand += a * a * 48000.0 / 2.0;
                }
                std::printf ("  DSP-06b info: energy outside 100 Hz +/- 2 bins = %.1f dB re in-band\n",
                             10.0 * std::log10 (std::max (tot - inBand, 1.0e-30) / inBand));
            }
        }

        // Informational: hook-driven at 1024-sample blocks (7.68°/block is automation
        // granularity under the τ = 20 ms contract, not a zipper) — printed, no verdict.
        {
            auto p = makeProc (2, 1024, [] (Proc& q)
            {
                setParam (q, ParamIDs::order,   2.0f);
                setParam (q, ParamIDs::decoder, 1.0f);
            });
            const auto ev = renderHooked (*p, total, toneGen, [&p] (int n)
            {
                setParam (*p, ParamIDs::azimuth, osa::wrapTo180 (-180.0f + 7.68f * (float) (n / 1024)));
            }, 1024);
            ClickReading rL { worstCurvature (ev, 0, 2, skip, total), controlL };
            ClickReading rR { worstCurvature (ev, 1, 2, skip, total), controlR };
            std::printf ("  DSP-06b info: hook-driven at 1024-sample blocks (7.68 deg/block): L %s; R %s\n",
                         clickDetail (rL).toRawUTF8(), clickDetail (rR).toRawUTF8());
        }

        // (ii) orbit-driven 1 Hz at 64 and 4096 host blocks — bit-equal, both ≤ 2.0.
        {
            const auto orbitSetup = [] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth,   -180.0f);
                setParam (q, ParamIDs::order,     2.0f);
                setParam (q, ParamIDs::decoder,   1.0f);
                setParam (q, ParamIDs::orbit,     1.0f);
                setParam (q, ParamIDs::orbitRate, 1.0f);
            };
            auto p64 = makeProc (2, 64, orbitSetup);
            std::vector<float> ev64;
            renderWith (*p64, 2, { 64 }, [] (int, int n) { return tone100 (n); }, ev64, total);

            auto p4096 = makeProc (2, 4096, orbitSetup);
            std::vector<float> ev4096;
            renderWith (*p4096, 2, { 4096 }, [] (int, int n) { return tone100 (n); }, ev4096, total);

            check ("DSP-06b orbit 1 Hz: 64-block render == 4096-block render (bit)", buffersBitEqual (ev64, ev4096));
            verdictPair ("DSP-06b orbit-driven 1 Hz, 64-sample blocks", ev64, {});
            verdictPair ("DSP-06b orbit-driven 1 Hz, 4096-sample blocks", ev4096, {});

            const double live = livenessDb (ev64);
            check ("DSP-06b orbit liveness: L energy second half vs first >= 6 dB", std::abs (live) >= 6.0,
                   fmt ("%.2f dB", live));

            // Negative control: the reference model with the per-sample lerp defeated
            // (coefficients snapped per grid) over the same orbit must FAIL the detector.
            RefParams rp = refParamsFrom (*p64);
            ReferenceModel lerped, snapped;
            snapped.snapCoefficientsPerGrid = true;
            const auto evLerped  = renderRef (lerped,  rp, kFs, total, 64, toneGen, [] (int, RefParams&) {});
            const auto evSnapped = renderRef (snapped, rp, kFs, total, 64, toneGen, [] (int, RefParams&) {});

            RefParams rpL = rp, rpR = rp;
            rpL.orbit = false; rpL.azimuthDeg = 60.0f;
            rpR.orbit = false; rpR.azimuthDeg = -60.0f;
            ReferenceModel mL, mR;
            const auto cL = renderRef (mL, rpL, kFs, total, 64, toneGen, [] (int, RefParams&) {});
            const auto cR = renderRef (mR, rpR, kFs, total, 64, toneGen, [] (int, RefParams&) {});
            const double mControlL = worstCurvature (cL.lr, 0, 2, skip, total);
            const double mControlR = worstCurvature (cR.lr, 1, 2, skip, total);

            ClickReading okL { worstCurvature (evLerped.lr, 0, 2, skip, total), mControlL };
            ClickReading okR { worstCurvature (evLerped.lr, 1, 2, skip, total), mControlR };
            std::printf ("  DSP-06b info: reference model (lerped) over the same orbit: L %s; R %s\n",
                         clickDetail (okL).toRawUTF8(), clickDetail (okR).toRawUTF8());

            ClickReading badL { worstCurvature (evSnapped.lr, 0, 2, skip, total), mControlL };
            ClickReading badR { worstCurvature (evSnapped.lr, 1, 2, skip, total), mControlR };
            checkFailsAsExpected (badL.ratio() <= kClickRatioCeiling && badR.ratio() <= kClickRatioCeiling,
                                  "DSP-06b negative control: snapped-per-grid model FAILS the click gate (expect >= 24)",
                                  fmt ("L ratio %.2f, R ratio %.2f", badL.ratio(), badR.ratio()));
        }
    }

    //==========================================================================
    // Order fade — 1 → 3 at 0.5 s → 1 at 1.0 s, gated on the 16-channel BUS TAP.
    {
        constexpr int blocks = 1125;                 // 1.5 s at 64
        constexpr int switchUp = 375, switchDown = 750;
        float y[16];
        osa::encodeSH16 (30.0f * osa::kDegToRad, 0.0f, y);
        const float Y4 = y[4], Y9 = y[9];

        osa::Params prm;
        prm.azimuthDeg = 30.0f;
        prm.decoder    = 1;
        const auto orderHook = [] (int n, auto& p)
        {
            const int b = n / 64;
            p.order = (b >= switchUp && b < switchDown) ? 3 : 1;
        };

        // Engine, with per-block frame reads (inline so frame() is reachable).
        std::vector<float> g4, g9, tap4, tap9;
        {
            juce::ScopedNoDenormals noDenormals;
            osa::SpatialEngine e;
            e.setParams (prm);
            e.prepare (kFs, 64);
            osa::Params p = prm;
            float mono, L, R;
            for (int b = 0; b < blocks; ++b)
            {
                orderHook (b * 64, p);
                e.setParams (p);
                for (int i = 0; i < 64; ++i)
                {
                    mono = tone100 (b * 64 + i);
                    e.process (&mono, &L, &R, 1);
                    tap4.push_back (e.busTap()[4]);
                    tap9.push_back (e.busTap()[9]);
                }
                g4.push_back (e.frame().shGain[4]);
                g9.push_back (e.frame().shGain[9]);
            }

            // (c) after the fade back to order 1, a fresh peak window has shPeak[4..15] == 0.
            e.resetPeaks();
            std::vector<float> m (64), l (64), r (64);
            for (int i = 0; i < 64; ++i)
                m[(size_t) i] = tone100 (blocks * 64 + i);
            e.process (m.data(), l.data(), r.data(), 64);
            bool zero = true;
            float worst = 0.0f;
            for (int acn = 4; acn < 16; ++acn)
            {
                zero = zero && e.frame().shPeak[acn] == 0.0f;
                worst = std::max (worst, e.frame().shPeak[acn]);
            }
            check ("order fade (c): shPeak[4..15] == 0 exactly after the fade back to order 1", zero,
                   fmt ("max %.3e, shPeak[0] %.4f", (double) worst, (double) e.frame().shPeak[0]));
        }

        // (a) exact arrival within 16 steps, monotonic in between.
        const auto arrival = [&] (const std::vector<float>& g, float Yv, const char* nm)
        {
            int up = -1, down = -1;
            for (int b = switchUp; b < switchDown && up < 0; ++b)
                if (bitEqual (g[(size_t) b], Yv))
                    up = b;
            for (int b = switchDown; b < blocks && down < 0; ++b)
                if (g[(size_t) b] == 0.0f)
                    down = b;
            bool monoUp = true, monoDown = true;
            if (up > 0)
                for (int b = switchUp; b < up; ++b)
                    monoUp = monoUp && g[(size_t) b + 1] >= g[(size_t) b] && g[(size_t) b] >= 0.0f;
            if (down > 0)
                for (int b = switchDown; b < down; ++b)
                    monoDown = monoDown && g[(size_t) b + 1] <= g[(size_t) b];
            const bool before = g[(size_t) switchUp - 1] == 0.0f;
            check (nm, before && up >= 0 && up - switchUp <= 16 && down >= 0 && down - switchDown <= 16 && monoUp && monoDown,
                   fmt ("Y %.6f; reached exactly Y %d steps after the 1->3 switch, exactly 0 %d steps after 3->1 (<= 16), monotonic %s/%s",
                        (double) Yv, up - switchUp, down - switchDown, monoUp ? "yes" : "NO", monoDown ? "yes" : "NO"));
        };
        arrival (g4, Y4, "order fade (a): shGain[4] arrives EXACTLY at Y_4 then EXACTLY at 0 within 16 steps, monotonic");
        arrival (g9, Y9, "order fade (a): shGain[9] arrives EXACTLY at Y_9 then EXACTLY at 0 within 16 steps, monotonic");

        // (b) click: engine tap vs the reference model's tap (same ramps).
        RefParams rp = refParamsFrom (prm);
        ReferenceModel ramped, snappedGains;
        snappedGains.snapOrderGains = true;
        const auto ctrl = renderRef (ramped, rp, kFs, blocks * 64, 64, [] (int n) { return tone100 (n); }, orderHook, true);
        const auto bad  = renderRef (snappedGains, rp, kFs, blocks * 64, 64, [] (int n) { return tone100 (n); }, orderHook, true);

        ClickReading r4 { worstCurvature (tap4, 0, 1, 2, blocks * 64), worstCurvature (ctrl.tap4, 0, 1, 2, blocks * 64) };
        ClickReading r9 { worstCurvature (tap9, 0, 1, 2, blocks * 64), worstCurvature (ctrl.tap9, 0, 1, 2, blocks * 64) };
        clickVerdict ("order fade (b): bus-tap ACN 4 click ratio vs model <= 2.0 across both switches", r4);
        clickVerdict ("order fade (b): bus-tap ACN 9 click ratio vs model <= 2.0 across both switches", r9);

        ClickReading b4 { worstCurvature (bad.tap4, 0, 1, 2, blocks * 64), r4.control };
        ClickReading b9 { worstCurvature (bad.tap9, 0, 1, 2, blocks * 64), r9.control };
        checkFailsAsExpected (b4.ratio() <= kClickRatioCeiling && b9.ratio() <= kClickRatioCeiling,
                              "order fade negative control: snapped order gains FAIL the bus-tap click gate",
                              fmt ("ACN 4 ratio %.1f, ACN 9 ratio %.1f", b4.ratio(), b9.ratio()));

        // Liveness of the tap itself: band 2/3 energy is present only while order 3 is active.
        const double eOn  = energy (tap4, 0, 1, switchUp * 64 + 2000, switchDown * 64);
        const double eOff = energy (tap4, 0, 1, 0, switchUp * 64) + energy (tap4, 0, 1, switchDown * 64 + 2000, blocks * 64);
        check ("order fade liveness: ACN 4 tap carries energy only while order 3 is active", eOn > 1.0 && eOff == 0.0,
               fmt ("on %.3f, off %.3e", eOn, eOff));
    }

    //==========================================================================
    // Meter frame — the Stage-3 reader contract.
    {
        // orderActive 4 / 9 / 16 with bands above EXACTLY zero after 20 ms (set
        // from order 3 after prepare so the unused bands must actually fade to 0).
        for (int idx = 0; idx < 3; ++idx)
        {
            auto p = makeProc (2, kDefaultBlock, [] (Proc& q) { setParam (q, ParamIDs::order, 2.0f); });
            setParam (*p, ParamIDs::order, (float) idx);
            std::vector<float> out;
            renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, 1536);   // 32 ms
            const auto& f = p->meterFrames.read();
            const int active = (idx + 2) * (idx + 2);
            bool zeros = true;
            float worst = 0.0f;
            for (int acn = active; acn < 16; ++acn)
            {
                zeros = zeros && f.shGain[acn] == 0.0f;
                worst = std::max (worst, std::abs (f.shGain[acn]));
            }
            check ((juce::String ("meter frame: order ") + juce::String (idx + 1) + " -> orderActive == "
                    + juce::String (active) + " and shGain[active..15] == 0").toRawUTF8(),
                   f.orderActive == active && zeros && f.latencySamples == 64,
                   fmt ("orderActive %d, max |shGain| above %.3e, shGain[0] %.3f, latency %d",
                        f.orderActive, (double) worst, (double) f.shGain[0], f.latencySamples));
        }

        // azEff follows the orbit: 10 reads over ~0.5 s advance monotonically mod 360, span ≥ 300°.
        {
            auto p = makeProc (2, 64, [] (Proc& q)
            {
                setParam (q, ParamIDs::orbit,     1.0f);
                setParam (q, ParamIDs::orbitRate, 2.0f);
            });
            std::vector<float> reads;
            juce::String detail;
            for (int k = 0; k < 10; ++k)
            {
                std::vector<float> out;
                renderWith (*p, 2, { 64 }, noiseAt, out, 2432);   // 38 blocks ≈ 50.7 ms
                const auto& f = p->meterFrames.read();
                reads.push_back (f.azEffDeg);
                detail << fmt ("%.1f ", (double) f.azEffDeg);
                if (k == 0)
                    check ("meter frame: azRotDeg == azEffDeg (identity rotation)", bitEqual (f.azRotDeg, f.azEffDeg));
            }
            bool mono = true;
            double span = 0.0;
            for (size_t i = 1; i < reads.size(); ++i)
            {
                double d = (double) reads[i] - (double) reads[i - 1];
                d -= 360.0 * std::floor (d / 360.0);   // [0, 360)
                mono = mono && d > 0.0 && d < 180.0;
                span += d;
            }
            check ("meter frame: azEff advances monotonically mod 360 over 10 reads and spans >= 300 deg", mono && span >= 300.0,
                   fmt ("span %.1f deg; reads: %s", span, detail.toRawUTF8()));
        }

        // Peaks: accumulate across 8 un-read blocks, then reset after a read.
        {
            auto p = makeProc (2, 64);
            juce::MidiBuffer midi;
            juce::AudioBuffer<float> buf (2, 64);
            // shPeak[0] is the bus (|s| at front / 1 m / order 1, pre-delay);
            // outPeakL is the OUTPUT (0.75·s delayed 64 = one block), so its
            // expectation is taken from the output samples themselves.
            float expectedMax = 0.0f, expectedOut = 0.0f;
            for (int b = 0; b < 8; ++b)
            {
                const float scale = (float) (b + 1) / 8.0f;
                for (int i = 0; i < 64; ++i)
                {
                    const float l = noiseAt (0, b * 64 + i) * scale, r = noiseAt (1, b * 64 + i) * scale;
                    buf.setSample (0, i, l);
                    buf.setSample (1, i, r);
                    expectedMax = std::max (expectedMax, std::abs (0.5f * (l + r)));
                }
                p->processBlock (buf, midi);
                for (int i = 0; i < 64; ++i)
                    expectedOut = std::max (expectedOut, std::abs (buf.getSample (0, i)));
            }
            const auto f8 = p->meterFrames.read();
            check ("meter frame: shPeak[0] == max |s| over 8 un-read blocks (accumulate), outPeakL == max |out| over them",
                   bitEqual (f8.shPeak[0], expectedMax) && bitEqual (f8.outPeakL, expectedOut),
                   fmt ("shPeak[0] %.6f expected %.6f; outPeakL %.6f expected %.6f",
                        (double) f8.shPeak[0], (double) expectedMax, (double) f8.outPeakL, (double) expectedOut));

            float blockMax = 0.0f;
            for (int i = 0; i < 64; ++i)
            {
                const float l = noiseAt (0, 512 + i) * 0.1f, r = noiseAt (1, 512 + i) * 0.1f;
                buf.setSample (0, i, l);
                buf.setSample (1, i, r);
                blockMax = std::max (blockMax, std::abs (0.5f * (l + r)));
            }
            p->processBlock (buf, midi);
            const auto f9 = p->meterFrames.read();
            check ("meter frame: after a read, the next frame's shPeak[0] == that block's peak only (reset)",
                   bitEqual (f9.shPeak[0], blockMax) && blockMax < expectedMax,
                   fmt ("shPeak[0] %.6f expected %.6f (previous max %.6f)", (double) f9.shPeak[0], (double) blockMax, (double) expectedMax));
        }
    }

    //==========================================================================
    // Extremes (QUAL-01 partial) — 16 corner settings at 48 k and 96 k.
    {
        for (double sr : { 48000.0, 96000.0 })
        {
            bool finite = true, live = true;
            int badCase = -1;
            for (int i = 0; i < 16; ++i)
            {
                const bool muted = (i % 4) == 3;
                auto p = makeProcAt (sr, 2, kDefaultBlock, [i, muted] (Proc& q)
                {
                    setParam (q, ParamIDs::azimuth,     (i & 1) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::elevation,   (i & 2) ? 90.0f : -90.0f);
                    setParam (q, ParamIDs::distance,    (i & 4) ? 20.0f : 0.2f);
                    setParam (q, ParamIDs::decoder,     (i & 8) ? 1.0f : 0.0f);
                    setParam (q, ParamIDs::order,       2.0f);
                    setParam (q, ParamIDs::orbit,       1.0f);
                    setParam (q, ParamIDs::orbitRate,   2.0f);
                    setParam (q, ParamIDs::outputLevel, muted ? -60.0f : 0.0f);
                    setParam (q, ParamIDs::inputMode,   (float) ((i / 4) % 2));
                });
                std::vector<float> out;
                renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, 4096);
                const bool f = allFinite (out);
                bool nonZero = false;
                for (float v : out)
                    if (v != 0.0f) { nonZero = true; break; }
                finite = finite && f;
                if (! muted)
                    live = live && nonZero;
                if ((! f || (! muted && ! nonZero)) && badCase < 0)
                    badCase = i;
            }
            check ((juce::String ("extremes: 16 corners x 4096 samples at fs ") + juce::String ((int) sr)
                    + " finite and not silenced").toRawUTF8(), finite && live,
                   badCase < 0 ? juce::String ("all 16 ok") : juce::String ("first bad case ") + juce::String (badCase));
        }
    }

    //==========================================================================
    // PERF-01 — allocation counter across processBlock (A11).
    {
        auto p = makeProc (2, 64);

        juce::MidiBuffer midi;
        midi.ensureSize (256);
        juce::AudioBuffer<float> small (2, 64);
        juce::AudioBuffer<float> big (2, 4096);

        const auto fill = [] (juce::AudioBuffer<float>& b, int n0)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < b.getNumSamples(); ++i)
                    b.setSample (ch, i, noiseAt (ch, n0 + i));
        };

        fill (small, 0);
        p->processBlock (small, midi);                 // warm-up, NOT counted

        rtcheck::arm();

        int n0 = 64;
        for (int k = 0; k < 200; ++k)
        {
            // Parameter changes between blocks, on the harness thread: every fade armed.
            switch (k)
            {
                case 10:  setParam (*p, ParamIDs::orbit, 1.0f); setParam (*p, ParamIDs::orbitRate, 2.0f); break;
                case 20:  setParam (*p, ParamIDs::order, 2.0f); break;
                case 30:  setParam (*p, ParamIDs::distance, 0.2f); break;
                case 40:  setParam (*p, ParamIDs::decoder, 1.0f); break;
                case 60:  setParam (*p, ParamIDs::order, 1.0f); break;
                case 80:  setParam (*p, ParamIDs::distance, 20.0f); break;
                case 100: setParam (*p, ParamIDs::decoder, 0.0f); break;
                default: break;
            }

            if (k == 150)
            {
                fill (big, n0);                        // the over-size block: slice loop
                p->processBlock (big, midi);
                n0 += 4096;
            }
            else
            {
                fill (small, n0);
                p->processBlock (small, midi);
                n0 += 64;
            }
        }

        rtcheck::disarm();
        const long long counted = rtcheck::allocations.load();

        check ("PERF-01 zero allocations across 200 processBlock calls (order 1->3->2, decoder B->S->B, orbit On, d 0.2->20, one 4096 block)",
               counted == 0, juce::String ((int) counted) + " allocation(s)" + rtcheck::foreignNote());

        // Liveness: the counter must see one deliberate new/delete pair.
        rtcheck::arm (false);
        {
            auto* leak = new int (1);
            livenessSink.store (leak);
            delete static_cast<int*> (livenessSink.exchange (nullptr));
        }
        rtcheck::disarm();
        const long long seen = rtcheck::allocations.load();
        check ("PERF-01 liveness: one deliberate new counts exactly 1", seen == 1,
               juce::String ((int) seen) + rtcheck::foreignNote());
    }

    //==========================================================================
    // Digest — Phase 2.1 baseline over the P1-null render (informational).
    std::printf ("  digest: fnv1a64=%016llx (P1-null render: %d samples x 2ch, flat %d, az 37 el -12 d 3.3 order 3 Stereo)\n",
                 (unsigned long long) fnv1a64 (nullRender), kRenderSamples, kDefaultBlock);

    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
