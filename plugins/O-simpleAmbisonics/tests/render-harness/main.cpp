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

    O-simpleAmbisonics render-harness — Stage 2 / Phase 2.1 + 2.2 + 2.3 DSP gate.

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
                          (incl. yaw / pitch / roll moving from 2.2) and one
                          4096-sample block; liveness new == 1.
      digest-2.1        — FNV-1a 64 of the P1-null render. From Phase 2.2 this
                          is a VERDICT (D3): zero rotation is bit-transparent,
                          so 9189d911a1fe98d5 must reproduce.

    Phase 2.2 probes (scene rotation; CONTEXT-2.2 / RESEARCH-2.2 / PLAN-2.2 T6):

      digest-2.1 verdict — fnv1a64 (nullRender) == 9189d911a1fe98d5 asserted.
      DSP-02            — 200 tuples × orders 1 / 2 / 3: ShRotator (float) applied
                          to the gated SN3D table vs re-encoding at dir (R3·d)
                          (double): max |err| < 1e-4; W exact; block
                          orthogonality < 1e-5; transpose trap FAILS as
                          expected; yaw-only closed form (psi 73) within 1e-6;
                          identity exact (0 / 1 to the bit).
      cardinal ×3       — through the processor frame: psi +90 → azRot +90;
                          theta +90 → elRot +90; phi +90 on a left source →
                          elRot +90 (elRot only at the pole, R4).
      yaw = −azimuth    — az +90 / psi −90: shGain == refSh16 (0, 0), azEff
                          unchanged, Stereo |L − R| < 1e-5 (FUNC-01 third box).
      P1 null rotated   — the P1 tuple + psi 25 / theta −40 / phi 70 vs the
                          ReferenceModel: < 1e-5. Input of digest-2.2.
      meter frame 2.2   — shGain / azRot / elRot vs the reference's rotated
                          direction; azEff / elEff pinned to the parameters
                          under a yaw hook; azRot moves.
      round trip        — psi 0 → 90 → 0: identity fast path reachable again,
                          tail memcmp-equal to a fresh static render.
      yaw sweep click   — 16-channel bus tap vs 24 static parks (15°), ratio
                          ≤ 2.0, sweep started AT the prepared 0° (R6);
                          snapped-reference negative control on m ≠ 0;
                          zonal channels 0 / 2 / 6 / 12 bit-identical (F3).
      cross-gate        — front source: yaw sweep ≡ azimuth sweep within 1e-5
                          on all 16 channels; Stereo liveness ≥ 6 dB.
      yaw step click    — 0 → 180: vs the same-motion reference ≤ 2.0 (expect
                          1.000); snapped negative control; vs-static ratios
                          PRINTED only (R2: grid-boundary kinks, same as the
                          2.1 azimuth path).
      P2c / P3c / P2d   — slicing bit-equality with a static non-identity
                          rotation + orbit, and with yaw / pitch / roll stepped
                          at 4096-sample boundaries (64 vs 4096 blocks).
      extremes 2.2      — ±180° on all three angles × fs 48 k / 96 k finite and
                          live; gimbal theta = +90 under a yaw sweep finite.
      digest-2.2        — FNV-1a 64 of the rotated P1-null render. From Phase
                          2.3 a VERDICT (D13): the Stereo path is untouched by
                          the binaural leg, so 7f35bc9f91e321b9 must reproduce.

    Phase 2.3 — the C15 MIGRATION (entry-check correction). Six 2.1 probes
    anchored on the default (Binaural) output while it was the A14 placeholder
    (P1 anchor default / out[64] / Stereo == Binaural / engine-direct, P1b ×2,
    P4). Each now renders in STEREO mode ("(Stereo)" in its verdict string) so
    the exact 0.75f·s[n−64] anchor stays a verdict, and gains a BINAURAL TWIN
    gated against the ReferenceModel's direct-form convolution (< 1e-5).
    `flat` (defaults, Binaural) is still the P2 / P3 reference — those gates
    now exercise the convolver under slicing for free. The "Stereo ==
    Binaural" equality probe is INVERTED ("A14 placeholder gone"). Every other
    2.1 / 2.2 probe body and verdict string is untouched; digest-2.1 and
    digest-2.2 are verdicts from 2.3 (D13).

    Phase 2.3 probes (binaural leg; CONTEXT-2.3 / RESEARCH-2.3 / PLAN-2.3 T7):

      header sanity     — constants, every float finite, level match ±3 dB,
                          tap 0 exactly 0.
      UPOLS null (D7)   — processor at the rotated null tuple, Binaural, orders
                          1 / 2 / 3 × host blocks 32 / 64 / 100 / 512 / 1024 /
                          4096 vs the direct-form double reference: < 1e-5.
      null at 96 k      — engine-direct, K = 8, reference seeded with the
                          engine's resampled table: < 1e-5.
      resampler (C3)    — engine tables at 44.1 k / 96 k vs the scipy
                          resample_poly goldens: ≤ 1.5e-2 / 6e-3 of peak, |H|
                          within 0.1 dB below 16 kHz; un-calibrated negative
                          control FAILS; 48 k table bit-exact to the header.
      DSP-03 onset      — 10 %-of-peak onset == in + 64 + kBinauralOnsetFront
                          Samples[n] (± 2 / ± 3) at 44.1 / 48 / 96 k;
                          getLatencySamples() == 64 both modes every rate.
      DSP-03 ILD / ITD  — ±90°: ILD ≥ 6 dB with the AmbiX sign; ITD by low-
                          passed cross-correlation 25–45 samples (C10).
      FUNC-05 level     — front / order 3: A-weighted Binaural vs Stereo
                          < 1 dB (orders 1 / 2 printed).
      DSP-07 clicks     — order 1→3 / 3→1 and Binaural→Stereo→Binaural on a
                          100 Hz tone at az +90° at 64 and 1024 blocks: ratio
                          ≤ 2.0 vs the same-motion reference; snapped-bank /
                          snapped-mix negative controls FAIL; spectral
                          liveness 3 kHz R ear ≥ 3 dB between orders 1 / 3.
      bank-fade queue   — queued latest-wins requests null to the reference;
                          float ramp arrives EXACTLY at step 15.
      P2e / P3e / P2f   — slicing bit-equality in Binaural mode, with order /
                          decoder / yaw stepped at 4096 boundaries.
      FDL decay         — exact 0 within K grids of silence at 48 k / 96 k.
      placeholder gone  — Binaural ≠ Stereo; kStereoLevelMatch == 1.0f (D2).
      extremes 2.3      — the 2.2 corners forced Binaural, orders 1 / 3,
                          48 k / 96 k; gimbal in Binaural; headroom printed.
      PERF-01           — 0 allocations with the convolver live.
      digest-2.3        — FNV-1a 64 of the Binaural null render (informational,
                          D6: vDSP vs Fallback FFT).
      PERF-02 (C9)      — ≥ 20× real time, Release only, best of three, LAST.

    Stage 3 (GUI; PLAN-3 T4) adds ST1–ST6 / PL1–PL4 after PERF-01 and before
    the digest prints, on fresh processors only. The 241 Stage-2 checks and
    both digest verdicts are untouched; the denominator becomes 241 + 10 = 251.

      ST1–ST5           — uiLanguage / editorScale state round-trips: code
                          string, absent / unknown → en, scale reject list
                          removed from the re-saved copy, 12 values bit-equal.
      ST6 (UI-08)       — applyFactoryPreset: recipe-or-default from an
                          off-default state, outputLevel untouched, begins ==
                          ends == changed, idempotent, unknown ids inert.
      PL1–PL3           — MeterFramePayload.h (the editor's own code): bytes,
                          hash, locale-free zero, fuzz-safe; UI-01 / UI-02
                          facts from real processors; the U1 live rule.
      PL4               — the UI fixture set built twice is byte-identical;
                          OSAMB_UI_FIXTURES=<dir> also writes <dir>/frames.json.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp/SpatialEngine.h"
#include "dsp/ShEncoder.h"
#include "dsp/ShRotator.h"
#include "dsp/DistanceModel.h"
#include "dsp/StereoDecoder.h"
#include "dsp/BinauralConvolver.h"
#include "ReferenceModel.h"
#include "BinauralFilterGoldens.h"
#include "MeterFramePayload.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <limits>
#include <string>
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
        r.yawDeg       = rawParam (p, ParamIDs::yaw);
        r.pitchDeg     = rawParam (p, ParamIDs::pitch);
        r.rollDeg      = rawParam (p, ParamIDs::roll);
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
        r.yawDeg       = p.yawDeg;
        r.pitchDeg     = p.pitchDeg;
        r.rollDeg      = p.rollDeg;
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
    // a time (the grid persists across calls) and busTap[4] / [9] are logged,
    // plus (Phase 2.2, R7) all 16 channels per sample in tap16 (stride 16).
    struct EngineRender
    {
        std::vector<float> lr, tap4, tap9, tap16;
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
                    for (int c = 0; c < 16; ++c)
                        r.tap16.push_back (e.busTap()[c]);
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
        std::vector<double> lr, tap4, tap9, tap16;
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
                    for (int c = 0; c < 16; ++c)
                        r.tap16.push_back (model.busTap()[c]);
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

    //==========================================================================
    // ── Phase 2.2 helpers (PLAN-2.2 T5) ────────────────────────────────────────

    struct Tuple { double az, el, yaw, pitch, roll; };

    // Deterministic tuple set — fixed seed, printed by the DSP-02 probe.
    constexpr juce::int64 kTupleSeed = 0x2D5A2D5A;

    std::vector<Tuple> makeTuples (int count)
    {
        juce::Random rng (kTupleSeed);
        std::vector<Tuple> t;
        t.reserve ((size_t) count);
        for (int i = 0; i < count; ++i)
            t.push_back ({ -180.0 + 360.0 * rng.nextDouble(),
                           -90.0  + 180.0 * rng.nextDouble(),
                           -180.0 + 360.0 * rng.nextDouble(),
                           -180.0 + 360.0 * rng.nextDouble(),
                           -180.0 + 360.0 * rng.nextDouble() });
        return t;
    }

    // The generic SN3D table with the bands above `order` zeroed (order gating).
    void gatedRefSh16 (double azDeg, double elDeg, int order, double out[16])
    {
        ReferenceModel::refSh16 (azDeg, elDeg, out);
        for (int i = 0; i < 16; ++i)
            if (ReferenceModel::band (i) > order)
                out[i] = 0.0;
    }

    // The DSP-02 oracle: re-encode at the rotated direction (double, D6).
    void oracleSh16 (const Tuple& q, int order, double out[16])
    {
        double azR, elR;
        ReferenceModel::rotatedDirection (q.az, q.el, q.yaw, q.pitch, q.roll, azR, elR);
        gatedRefSh16 (azR, elR, order, out);
    }

    double maxAbsDiff16 (const float a[16], const double b[16])
    {
        double m = 0.0;
        for (int i = 0; i < 16; ++i)
            m = std::max (m, std::abs ((double) a[i] - b[i]));
        return m;
    }

    // Each block transposed — the R1 convention trap, for the negative control.
    void transposeBlocks (const float R[osa::kNumRotElems], float Rt[osa::kNumRotElems])
    {
        for (int l = 1; l <= 3; ++l)
            for (int m = -l; m <= l; ++m)
                for (int n = -l; n <= l; ++n)
                    Rt[osa::rotIndex (l, m, n)] = R[osa::rotIndex (l, n, m)];
    }

    // max |R_l^T R_l − I| over the three blocks (double accumulation of float elements).
    double blockOrthoError (const float R[osa::kNumRotElems])
    {
        double worst = 0.0;
        for (int l = 1; l <= 3; ++l)
            for (int m = -l; m <= l; ++m)
                for (int n = -l; n <= l; ++n)
                {
                    double s = 0.0;
                    for (int k = -l; k <= l; ++k)
                        s += (double) R[osa::rotIndex (l, k, m)] * (double) R[osa::rotIndex (l, k, n)];
                    worst = std::max (worst, std::abs (s - (m == n ? 1.0 : 0.0)));
                }
        return worst;
    }

    double orthoError3 (const float R3[3][3])
    {
        double worst = 0.0;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
            {
                double s = 0.0;
                for (int k = 0; k < 3; ++k)
                    s += (double) R3[k][i] * (double) R3[k][j];
                worst = std::max (worst, std::abs (s - (i == j ? 1.0 : 0.0)));
            }
        return worst;
    }

    // Per-channel click ratio over a 16-stride tap against a per-channel control.
    double tapRatio (const std::vector<float>& tap16, int ch, const double control[16], int from, int to)
    {
        return control[ch] > 1.0e-300 ? worstCurvature (tap16, ch, 16, from, to) / control[ch] : 0.0;
    }

    double tapRatio (const std::vector<double>& tap16, int ch, const double control[16], int from, int to)
    {
        return control[ch] > 1.0e-300 ? worstCurvature (tap16, ch, 16, from, to) / control[ch] : 0.0;
    }

    //==========================================================================
    // ── Phase 2.3 helpers (PLAN-2.3 T5) ────────────────────────────────────────

    constexpr int kFft4096 = 4096;

    // A 4096-point real FFT with the R6 contract (bins 0 … 2048 interleaved).
    struct Fft4096
    {
        juce::dsp::FFT fft { 12 };
        std::vector<float> buf = std::vector<float> ((size_t) 2 * kFft4096, 0.0f);

        // x[0 … n) zero-padded (or truncated) to 4096; bins in buf[0 … 4098).
        void forward (const float* x, int n)
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            std::copy (x, x + std::min (n, kFft4096), buf.begin());
            fft.performRealOnlyForwardTransform (buf.data(), true);
        }

        double magnitude (int bin) const
        {
            const double re = buf[(size_t) 2 * (size_t) bin], im = buf[(size_t) 2 * (size_t) bin + 1];
            return std::sqrt (re * re + im * im);
        }

        // Zero every bin above cutoffHz, inverse → buf[0 … 4096) is the low-passed signal.
        void brickWallInverse (double fs, double cutoffHz)
        {
            for (int k = 0; k <= kFft4096 / 2; ++k)
                if ((double) k * fs / kFft4096 > cutoffHz)
                    buf[(size_t) 2 * (size_t) k] = buf[(size_t) 2 * (size_t) k + 1] = 0.0f;
            fft.performRealOnlyInverseTransform (buf.data());
        }
    };

    // IEC 61672 A-weighting magnitude, normalised to 1 at 1 kHz (R4).
    double aWeight (double f)
    {
        const auto ra = [] (double x)
        {
            const double x2 = x * x;
            return 12194.0 * 12194.0 * x2 * x2
                 / ((x2 + 20.6 * 20.6) * std::sqrt ((x2 + 107.7 * 107.7) * (x2 + 737.9 * 737.9)) * (x2 + 12194.0 * 12194.0));
        };
        return ra (f) / ra (1000.0);
    }

    // A-weighted RMS gain-like level of one channel of an interleaved stereo render over
    // [from, from + len): Hann-windowed 4096-point frames averaged (hop 4096), power
    // weighted by A²(f), DC excluded: sqrt (Σ A²·P / Σ A²). Absolute scale is arbitrary
    // but identical for every caller, so RATIOS between renders are exact (FUNC-05).
    double aWeightedLevel (const std::vector<float>& lr, int ch, int from, int len, double fs = kFs)
    {
        Fft4096 f;
        std::vector<double> power ((size_t) kFft4096 / 2 + 1, 0.0);
        std::vector<float> frame ((size_t) kFft4096);
        int frames = 0;
        for (int start = from; start + kFft4096 <= from + len; start += kFft4096, ++frames)
        {
            for (int i = 0; i < kFft4096; ++i)
            {
                const double w = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) kFft4096);
                frame[(size_t) i] = (float) (w * (double) lr[(size_t) ((start + i) * 2 + ch)]);
            }
            f.forward (frame.data(), kFft4096);
            for (int k = 0; k <= kFft4096 / 2; ++k)
            {
                const double m = f.magnitude (k);
                power[(size_t) k] += m * m;
            }
        }
        double num = 0.0, den = 0.0;
        for (int k = 1; k <= kFft4096 / 2; ++k)
        {
            const double a2 = std::pow (aWeight ((double) k * fs / kFft4096), 2.0);
            num += a2 * power[(size_t) k] / (double) std::max (frames, 1);
            den += a2;
        }
        return std::sqrt (num / den);
    }

    // C10: both ears brick-wall low-passed at cutoffHz on a 4096-point FFT, then the lag d
    // maximising Σ_n L[n]·R[n + d] over |d| ≤ 200. POSITIVE = R lags = L LEADS. Same
    // definition as the generator's itd_lp_xcorr (sign anchored there on a synthetic pair).
    int itdLowPassXcorr (const std::vector<float>& lr, double fs, double cutoffHz, int from, int len)
    {
        Fft4096 fL, fR;
        std::vector<float> L ((size_t) len), R ((size_t) len);
        for (int i = 0; i < len; ++i)
        {
            L[(size_t) i] = lr[(size_t) ((from + i) * 2)];
            R[(size_t) i] = lr[(size_t) ((from + i) * 2 + 1)];
        }
        fL.forward (L.data(), len);
        fR.forward (R.data(), len);
        fL.brickWallInverse (fs, cutoffHz);
        fR.brickWallInverse (fs, cutoffHz);

        int    best = 0;
        double bestC = -1.0e300;
        for (int d = -200; d <= 200; ++d)
        {
            double c = 0.0;
            for (int n = 0; n < kFft4096; ++n)
            {
                const int m = n + d;
                if (m >= 0 && m < kFft4096)
                    c += (double) fL.buf[(size_t) n] * (double) fR.buf[(size_t) m];
            }
            if (c > bestC)
            {
                bestC = c;
                best = d;
            }
        }
        return best;
    }

    // Broadband rms ratio L / R in dB over [from, from + len).
    double ildDb (const std::vector<float>& lr, int from, int len)
    {
        return dB (rms (lr, 0, 2, from, from + len) / rms (lr, 1, 2, from, from + len));
    }

    // Band-limited power ratio L / R in dB (4096-point FFT of [from, from + len)).
    double ildBandDb (const std::vector<float>& lr, int from, int len, double fs, double lo, double hi)
    {
        Fft4096 fL, fR;
        std::vector<float> L ((size_t) len), R ((size_t) len);
        for (int i = 0; i < len; ++i)
        {
            L[(size_t) i] = lr[(size_t) ((from + i) * 2)];
            R[(size_t) i] = lr[(size_t) ((from + i) * 2 + 1)];
        }
        fL.forward (L.data(), len);
        fR.forward (R.data(), len);
        double pl = 0.0, pr = 0.0;
        for (int k = 0; k <= kFft4096 / 2; ++k)
        {
            const double f = (double) k * fs / kFft4096;
            if (f >= lo && f < hi)
            {
                pl += std::pow (fL.magnitude (k), 2.0);
                pr += std::pow (fR.magnitude (k), 2.0);
            }
        }
        return 10.0 * std::log10 (pl / pr);
    }

    // First index at which |x| ≥ 10 % of that channel's peak (F6 — never "first nonzero").
    int onset10 (const std::vector<float>& lr, int ch)
    {
        float peak = 0.0f;
        const int n = (int) lr.size() / 2;
        for (int i = 0; i < n; ++i)
            peak = std::max (peak, std::abs (lr[(size_t) (i * 2 + ch)]));
        for (int i = 0; i < n; ++i)
            if (std::abs (lr[(size_t) (i * 2 + ch)]) >= 0.1f * peak)
                return i;
        return -1;
    }

    int firstNonZero (const std::vector<float>& lr)
    {
        const int n = (int) lr.size() / 2;
        for (int i = 0; i < n; ++i)
            if (lr[(size_t) (i * 2)] != 0.0f || lr[(size_t) (i * 2 + 1)] != 0.0f)
                return i;
        return -1;
    }

    // The reference reads the engine's resampled table at fs ≠ 48 k (D7).
    void loadExternalFilters (ReferenceModel& model, const osa::SpatialEngine& engine)
    {
        const auto& c = engine.convolver();
        for (int order = 1; order <= 3; ++order)
            for (int ch = 0; ch < osa::BinauralConvolver::channelsFor (order); ++ch)
                for (int ear = 0; ear < 2; ++ear)
                    model.setExternalFilters (order, ch, ear, c.filterTaps (order, ch, ear), c.tapsAtRate());
    }

    // max |engine − reference| over an interleaved float render vs an interleaved double one.
    double maxErrLr (const std::vector<float>& a, const std::vector<double>& b, double& peakL)
    {
        double worst = 0.0;
        peakL = 0.0;
        const size_t n = std::min (a.size(), b.size());
        for (size_t i = 0; i < n; ++i)
        {
            worst = std::max (worst, std::abs ((double) a[i] - b[i]));
            if ((i & 1) == 0)
                peakL = std::max (peakL, std::abs (b[i]));
        }
        return worst;
    }

    // ReferenceModel render on the mono equivalent of a processor render: gen (n) is the
    // mono input; params from the processor's raw atomics.
    template <typename Gen>
    std::vector<double> refRenderStatic (const RefParams& rp, double fs, int total, Gen gen, ReferenceModel* seeded = nullptr)
    {
        ReferenceModel local;
        ReferenceModel& model = seeded != nullptr ? *seeded : local;
        model.setParams (rp);
        model.prepare (fs);
        std::vector<float>  mono ((size_t) total);
        std::vector<double> L ((size_t) total), R ((size_t) total), lr ((size_t) total * 2);
        for (int n = 0; n < total; ++n)
            mono[(size_t) n] = gen (n);
        model.process (mono.data(), L.data(), R.data(), total);
        for (int n = 0; n < total; ++n)
        {
            lr[(size_t) n * 2]     = L[(size_t) n];
            lr[(size_t) n * 2 + 1] = R[(size_t) n];
        }
        return lr;
    }

    const float* headerTaps (int order, int ch, int ear)
    {
        switch (order)
        {
            case 1:  return osa::kMagLsFilters1[ch][ear];
            case 2:  return osa::kMagLsFilters2[ch][ear];
            default: return osa::kMagLsFilters3[ch][ear];
        }
    }

    const float* goldenTaps (int fs, int order, int ch, int ear)
    {
        using namespace OSimpleAmbisonics::test;
        if (fs == 44100)
            return order == 1 ? kBinauralGolden44100_1[ch][ear] : order == 2 ? kBinauralGolden44100_2[ch][ear] : kBinauralGolden44100_3[ch][ear];
        return order == 1 ? kBinauralGolden96000_1[ch][ear] : order == 2 ? kBinauralGolden96000_2[ch][ear] : kBinauralGolden96000_3[ch][ear];
    }
} // namespace

//==============================================================================
// ── Stage 3 helpers (PLAN-3 T4): state round-trips, lesson gestures, the
//    meterFrame payload (MeterFramePayload.h — the editor's own code) and the
//    UI fixture set (tests/ui-fixtures/frames.json). Fresh processors only;
//    no Stage-2 render is touched.
namespace
{
namespace st3
{
    namespace ui  = OSimpleAmbisonics::ui;
    namespace ids = OSimpleAmbisonics::ParamIDs;

    constexpr const char* kAllIds[12] = {
        ids::azimuth, ids::elevation, ids::distance, ids::order, ids::yaw, ids::pitch,
        ids::roll, ids::decoder, ids::orbit, ids::orbitRate, ids::inputMode, ids::outputLevel };

    std::unique_ptr<juce::XmlElement> stateXml (Proc& p)
    {
        juce::MemoryBlock mb;
        p.getStateInformation (mb);
        return std::unique_ptr<juce::XmlElement> (Proc::getXmlFromBinary (mb.getData(), (int) mb.getSize()));
    }

    void restoreXml (Proc& p, const juce::XmlElement& xml)
    {
        juce::MemoryBlock mb;
        Proc::copyXmlToBinary (xml, mb);
        p.setStateInformation (mb.getData(), (int) mb.getSize());
    }

    void roundTrip (Proc& from, Proc& to)
    {
        juce::MemoryBlock mb;
        from.getStateInformation (mb);
        to.setStateInformation (mb.getData(), (int) mb.getSize());
    }

    // Counts begin / end gestures on every parameter.
    struct GestureCounter : juce::AudioProcessorParameter::Listener
    {
        int begins = 0, ends = 0;
        void parameterValueChanged (int, float) override {}
        void parameterGestureChanged (int, bool starting) override { ++(starting ? begins : ends); }
    };

    // ── fixtures ──────────────────────────────────────────────────────────────
    struct ParamSet { const char* id; float plain; };

    constexpr int kFixtureBlock   = 64;
    constexpr int kFixtureSamples = 24000;   // 0.5 s at 48 k

    void play (Proc& p, int samples, bool silent)
    {
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> buf (2, kFixtureBlock);
        for (int n0 = 0; n0 < samples; n0 += kFixtureBlock)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < kFixtureBlock; ++i)
                    buf.setSample (ch, i, silent ? 0.0f : noiseAt (ch, n0 + i));
            p.processBlock (buf, midi);
        }
    }

    std::string payloadOf (const ui::QuantisedFrame& q)
    {
        char buf[1024];
        const int n = ui::format (q, buf, (int) sizeof (buf));
        return std::string (buf, (size_t) juce::jmax (0, n));
    }

    std::unique_ptr<Proc> fixtureProc (std::initializer_list<ParamSet> params)
    {
        std::vector<ParamSet> ps (params);
        return makeProc (2, kFixtureBlock, [ps] (Proc& q)
        {
            for (const auto& s : ps)
                setParam (q, s.id, s.plain);
        });
    }

    struct Fixture
    {
        std::string name;
        std::string paramsJson;
        ui::QuantisedFrame frame;
        std::string payload;
    };

    std::string paramsJsonOf (Proc& p)
    {
        std::string s = "{";
        for (int i = 0; i < 12; ++i)
        {
            if (i != 0) s += ",";
            s += "\"";
            s += kAllIds[i];
            s += "\":";
            s += fmt ("%.6g", (double) rawParam (p, kAllIds[i])).toStdString();
        }
        return s + "}";
    }

    // The standard recipe: prepared with the params set BEFORE prepare, 0.5 s
    // of noise in 64-sample blocks, one tick.
    Fixture standard (const char* name, std::initializer_list<ParamSet> params)
    {
        auto p = fixtureProc (params);
        play (*p, kFixtureSamples, false);
        ui::LiveTracker t;
        Fixture f { name, paramsJsonOf (*p), ui::sampleFrame (p->meterFrames, t), {} };
        f.payload = payloadOf (f.frame);
        return f;
    }

    std::vector<Fixture> buildFixtures()
    {
        std::vector<Fixture> v;
        v.push_back (standard ("front_o1",    { { ids::azimuth, 0.0f } }));
        v.push_back (standard ("left_o1",     { { ids::azimuth, 90.0f } }));
        v.push_back (standard ("behind_o1",   { { ids::azimuth, 180.0f } }));
        v.push_back (standard ("right_o1",    { { ids::azimuth, -90.0f } }));
        v.push_back (standard ("overhead_o2", { { ids::elevation, 90.0f }, { ids::order, 1.0f } }));
        v.push_back (standard ("o1_az45",     { { ids::azimuth, 45.0f }, { ids::order, 0.0f } }));
        v.push_back (standard ("o2_az45",     { { ids::azimuth, 45.0f }, { ids::order, 1.0f } }));
        v.push_back (standard ("o3_az45",     { { ids::azimuth, 45.0f }, { ids::order, 2.0f } }));
        v.push_back (standard ("yaw90_front", { { ids::azimuth, 0.0f }, { ids::yaw, 90.0f } }));
        v.push_back (standard ("pole_a",      { { ids::elevation, 89.999f }, { ids::yaw, 30.0f } }));
        v.push_back (standard ("pole_b",      { { ids::elevation, 89.999f }, { ids::yaw, 150.0f } }));
        v.push_back (standard ("stereo_o3",   { { ids::decoder, 1.0f }, { ids::order, 2.0f }, { ids::azimuth, 45.0f } }));

        {   // live but silent: noise, tick, zeros, tick, zeros, tick. The FIRST
            // silent tick still holds the output tail (64-sample latency + the
            // binaural FIR) as peak-since-last-read — correct semantics, so the
            // fixture is the SECOND silent tick, what a 30 Hz reader sees next.
            auto p = fixtureProc ({ { ids::azimuth, 45.0f }, { ids::order, 2.0f } });
            ui::LiveTracker t;
            play (*p, kFixtureSamples, false);
            (void) ui::sampleFrame (p->meterFrames, t);
            play (*p, kFixtureSamples, true);
            (void) ui::sampleFrame (p->meterFrames, t);
            play (*p, kFixtureSamples, true);
            Fixture f { "silent_after_play", paramsJsonOf (*p), ui::sampleFrame (p->meterFrames, t), {} };
            f.payload = payloadOf (f.frame);
            v.push_back (f);
        }
        {   // prepared, no processBlock, one tick
            auto p = fixtureProc ({ { ids::azimuth, 90.0f } });
            ui::LiveTracker t;
            Fixture f { "idle_before_first_block", paramsJsonOf (*p), ui::sampleFrame (p->meterFrames, t), {} };
            f.payload = payloadOf (f.frame);
            v.push_back (f);
        }
        {   // play, tick 1 (fresh), ticks 2..4 without processBlock: the tick-4 payload
            auto p = fixtureProc ({ { ids::azimuth, 90.0f } });
            ui::LiveTracker t;
            play (*p, kFixtureSamples, false);
            ui::QuantisedFrame q = ui::sampleFrame (p->meterFrames, t);
            for (int k = 2; k <= 4; ++k)
                q = ui::sampleFrame (p->meterFrames, t);
            Fixture f { "idle_stale_after_publish", paramsJsonOf (*p), q, {} };
            f.payload = payloadOf (f.frame);
            v.push_back (f);
        }
        return v;
    }

    std::string escapeJsonString (const std::string& s)
    {
        std::string o;
        o.reserve (s.size() + 64);
        for (char c : s)
        {
            if (c == '"' || c == '\\') o += '\\';
            o += c;
        }
        return o;
    }

    std::string fixturesJson (const std::vector<Fixture>& v)
    {
        std::string s = "[\n";
        for (size_t i = 0; i < v.size(); ++i)
        {
            s += "  {\"name\":\"" + v[i].name + "\",\"params\":" + v[i].paramsJson
               + ",\"payload\":\"" + escapeJsonString (v[i].payload) + "\"}";
            s += (i + 1 < v.size()) ? ",\n" : "\n";
        }
        return s + "]\n";
    }

    const Fixture* find (const std::vector<Fixture>& v, const char* name)
    {
        for (const auto& f : v)
            if (f.name == name)
                return &f;
        return nullptr;
    }
} // namespace st3
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    using namespace OSimpleAmbisonics;

    std::printf ("O-simpleAmbisonics render-harness (Stage 2 / Phase 2.1 + 2.2 + 2.3) — fs=%.0f, latency=%d\n",
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
    // P1 anchor — front / 1 m / order 1: out == 0.75f·s[n−64] BIT-EQUAL in
    // STEREO mode (F2 / F3; C15 — the A14 placeholder is gone, so the exact
    // anchor is a Stereo-leg property) and engine-direct. `flat` (defaults,
    // Binaural) is STILL the P2 / P3 reference and the P1 Binaural twin.
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

        bool headZero = true;
        for (int n = 0; n < kLatency && headZero; ++n)
            headZero = flat[(size_t) n * 2] == 0.0f && flat[(size_t) n * 2 + 1] == 0.0f;
        check ("P1 anchor out[0..63] == 0", headZero);

        // Binaural twin (C15): the default render vs the direct-form reference.
        double peak = 0.0;
        const double err = maxErrLr (flat, refRenderStatic (refParamsFrom (*p), kFs, kRenderSamples, monoNoiseAt), peak);
        check ("P1 Binaural (defaults) vs ReferenceModel direct-form: max|err| < 1e-5, peak |L| > 0.01", err < 1.0e-5 && peak > 0.01,
               fmt ("max|err| %.3e, peak |L| %.3f", err, peak));
    }
    std::vector<float> stereoAnchor;
    stereoAnchor.reserve ((size_t) kRenderSamples * 2);
    {
        auto p = makeProc (2, kDefaultBlock, [] (Proc& q) { setParam (q, ParamIDs::decoder, 1.0f); });
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, stereoAnchor);
        const int mm = firstMismatch (stereoAnchor, anchorExpected);
        check ("P1 anchor Stereo (default tuple): out == 0.75f*s[n-64] bit-equal both ch", mm < 0, juce::String (mm));

        check ("P1 anchor out[64] exact (no fade-in) (Stereo)",
               bitEqual (stereoAnchor[(size_t) kLatency * 2], anchorExpected (0))
                   && bitEqual (stereoAnchor[(size_t) kLatency * 2 + 1], anchorExpected (0)),
               juce::String (stereoAnchor[(size_t) kLatency * 2], 7) + " vs " + juce::String (anchorExpected (0), 7));

        // D13: the equality probe INVERTED — the binaural leg is real now.
        double maxDiff = 0.0;
        for (int n = 0; n < kRenderSamples; ++n)
            maxDiff = std::max (maxDiff, std::abs ((double) flat[(size_t) n * 2] - (double) stereoAnchor[(size_t) n * 2]));
        check ("A14 placeholder gone: Binaural render != Stereo render, max|L_bin - L_st| > 0.05", maxDiff > 0.05,
               fmt ("max|L_bin - L_st| %.4f", maxDiff));
    }
    {
        osa::Params defaults;
        defaults.decoder = 1;
        const auto r = renderEngine (defaults, kFs, kDefaultBlock, kRenderSamples, { kDefaultBlock },
                                     monoNoiseAt, [] (int, osa::Params&) {});
        const int mm = firstMismatch (r.lr, anchorExpected);
        check ("P1 anchor engine-direct (Stereo): out == 0.75f*s[n-64] bit-equal", mm < 0, juce::String (mm));

        osa::Params bin;
        const auto rb = renderEngine (bin, kFs, kDefaultBlock, kRenderSamples, { kDefaultBlock },
                                      monoNoiseAt, [] (int, osa::Params&) {});
        double peak = 0.0;
        const double err = maxErrLr (rb.lr, refRenderStatic (refParamsFrom (bin), kFs, kRenderSamples, monoNoiseAt), peak);
        check ("P1 engine-direct Binaural twin vs ReferenceModel: max|err| < 1e-5", err < 1.0e-5 && peak > 0.01,
               fmt ("max|err| %.3e, peak |L| %.3f", err, peak));
    }

    //==========================================================================
    // P1b — Left only (stereo in), then a mono input bus — both with the 0.75
    // factor in Stereo mode (C15), each with a Binaural twin vs the reference.
    {
        auto p = makeProc (2, kDefaultBlock, [] (Proc& q)
        {
            setParam (q, ParamIDs::inputMode, 1.0f);
            setParam (q, ParamIDs::decoder,   1.0f);
        });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, [] (int n) { return 0.75f * noiseAt (0, n); });
        check ("P1b Left only == 0.75f*L[n-64] on both ch (Stereo)", mm < 0, juce::String (mm));

        auto pb = makeProc (2, kDefaultBlock, [] (Proc& q) { setParam (q, ParamIDs::inputMode, 1.0f); });
        std::vector<float> outB;
        outB.reserve ((size_t) kRenderSamples * 2);
        renderWith (*pb, 2, { kDefaultBlock }, noiseAt, outB);
        double peak = 0.0;
        const double err = maxErrLr (outB, refRenderStatic (refParamsFrom (*pb), kFs, kRenderSamples, [] (int n) { return noiseAt (0, n); }), peak);
        check ("P1b Left only Binaural twin vs ReferenceModel (mono = L): max|err| < 1e-5", err < 1.0e-5 && peak > 0.01,
               fmt ("max|err| %.3e, peak |L| %.3f", err, peak));
    }
    {
        auto p = makeProc (1, kDefaultBlock, [] (Proc& q) { setParam (q, ParamIDs::decoder, 1.0f); });   // mono input bus, stereo out
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 1, { kDefaultBlock }, noiseAt, out);
        const int mm = firstMismatch (out, [] (int n) { return 0.75f * noiseAt (0, n); });
        check ("P1b mono-in == 0.75f*in0[n-64] on both ch (Stereo)", mm < 0, juce::String (mm));

        auto pb = makeProc (1, kDefaultBlock);
        std::vector<float> outB;
        outB.reserve ((size_t) kRenderSamples * 2);
        renderWith (*pb, 1, { kDefaultBlock }, noiseAt, outB);
        double peak = 0.0;
        const double err = maxErrLr (outB, refRenderStatic (refParamsFrom (*pb), kFs, kRenderSamples, [] (int n) { return noiseAt (0, n); }), peak);
        check ("P1b mono-in Binaural twin vs ReferenceModel (mono = in0): max|err| < 1e-5", err < 1.0e-5 && peak > 0.01,
               fmt ("max|err| %.3e, peak |L| %.3f", err, peak));
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
    // amplitude exactly 0.75f (front / 1 m / order 1 anchor) — Stereo (C15);
    // the Binaural impulse is DSP-03 (2.3).
    {
        auto p = makeProc (2, kDefaultBlock, [] (Proc& q) { setParam (q, ParamIDs::decoder, 1.0f); });
        std::vector<float> out;
        out.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock },
                    [] (int, int n) { return n == 100 ? 1.0f : 0.0f; }, out);

        int firstNonZero = -1;
        for (int n = 0; n < kRenderSamples && firstNonZero < 0; ++n)
            if (out[(size_t) n * 2] != 0.0f || out[(size_t) n * 2 + 1] != 0.0f)
                firstNonZero = n;

        const int reported = p->getLatencySamples();
        check ("P4 measured latency == reported (Stereo)", firstNonZero == 100 + reported,
               juce::String ("first nonzero ") + juce::String (firstNonZero)
                   + ", reported " + juce::String (reported));
        check ("P4 impulse amplitude exactly 0.75f (Stereo)", firstNonZero >= 0
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
    //==========================================================================
    // ── Phase 2.2 — scene rotation in the SH domain (PLAN-2.2 T6) ────────────
    //==========================================================================

    // digest-2.1 VERDICT (D3 / B12): zero rotation is bit-transparent, so the
    // Phase 2.1 P1-null digest must reproduce exactly through the rotator. If
    // this ever fails, suspect a signed zero (R3) FIRST — memcmp the render
    // against a 2.1 build before anything else. From 2.3 (D13): the Stereo
    // path must not have moved either — suspect StereoDecoder / DecoderMixer
    // edits or the renderSample signature change before anything else.
    check ("digest-2.1 verdict: fnv1a64 (P1-null render) == 9189d911a1fe98d5 (zero rotation bit-transparent, D3)",
           fnv1a64 (nullRender) == 0x9189d911a1fe98d5ULL,
           fmt ("%016llx", (unsigned long long) fnv1a64 (nullRender)));

    //==========================================================================
    // DSP-02 — R_SH (float, ShRotator) applied to the gated SN3D table vs
    // re-encoding at dir (R3·d) (double, ReferenceModel's own builder), 200
    // tuples × orders 1 / 2 / 3; orthogonality; transpose trap; yaw-only closed
    // form; identity exact.
    {
        const auto tuples = makeTuples (200);
        std::printf ("  DSP-02 tuple set: %d tuples from juce::Random seed 0x%llx (az/el/psi/theta/phi uniform)\n",
                     (int) tuples.size(), (unsigned long long) kTupleSeed);

        osa::ShRotator rot;
        rot.prepare();

        double maxErr[4] = { 0.0, 0.0, 0.0, 0.0 }, maxOrtho = 0.0, maxOrthoR3 = 0.0, maxTransposed = 0.0;
        Tuple  worst[4] = {};
        bool   wExact = true;
        float  R[osa::kNumRotElems], Rt[osa::kNumRotElems], R3[3][3];

        for (const auto& q : tuples)
        {
            rot.build ((float) q.yaw, (float) q.pitch, (float) q.roll, R, R3);

            for (int order = 1; order <= 3; ++order)
            {
                double c[16], ref[16];
                gatedRefSh16 (q.az, q.el, order, c);
                oracleSh16 (q, order, ref);

                float in[16], out[16];
                for (int i = 0; i < 16; ++i)
                    in[i] = (float) c[i];
                osa::ShRotator::apply (R, in, out);

                const double e = maxAbsDiff16 (out, ref);
                if (e > maxErr[order])
                {
                    maxErr[order] = e;
                    worst[order]  = q;
                }
                wExact = wExact && out[0] == 1.0f;

                if (order == 3)
                {
                    transposeBlocks (R, Rt);
                    float outT[16];
                    osa::ShRotator::apply (Rt, in, outT);
                    maxTransposed = std::max (maxTransposed, maxAbsDiff16 (outT, ref));
                }
            }

            maxOrtho   = std::max (maxOrtho, blockOrthoError (R));
            maxOrthoR3 = std::max (maxOrthoR3, orthoError3 (R3));
        }

        for (int order = 1; order <= 3; ++order)
            check ((juce::String ("DSP-02 order ") + juce::String (order)
                    + ": R_SH * refSh16 (az, el) vs refSh16 (dir (R3 * d)) over 200 tuples, max|err| < 1e-4").toRawUTF8(),
                   maxErr[order] < 1.0e-4,
                   fmt ("max|err| %.3e (expect ~2e-7); worst tuple az %.2f el %.2f psi %.2f theta %.2f phi %.2f",
                        maxErr[order], worst[order].az, worst[order].el, worst[order].yaw, worst[order].pitch, worst[order].roll));
        check ("DSP-02 W passes exactly (out[0] == 1.0f) on every tuple and order (W implicit)", wExact);
        check ("DSP-02 orthogonality: max |R_l^T R_l - I| < 1e-5 over blocks 1-3 x 200 tuples", maxOrtho < 1.0e-5,
               fmt ("%.3e (expect ~1e-7)", maxOrtho));
        check ("DSP-02 orthogonality: engine buildCartesian max |R3^T R3 - I| < 1e-5", maxOrthoR3 < 1.0e-5,
               fmt ("%.3e", maxOrthoR3));
        checkFailsAsExpected (maxTransposed < 1.0e-4,
                              "DSP-02 transpose trap: R_SH^T (each block transposed) vs the oracle FAILS the 1e-4 gate (expect >= 1)",
                              fmt ("max|err| %.3f", maxTransposed));

        // Yaw-only closed form (D10 fallback written down): per order, the (−m, +m)
        // pair rotates by [[cos m*psi, sin m*psi], [−sin m*psi, cos m*psi]], m = 0 unchanged.
        {
            constexpr double psi = 73.0;
            rot.build ((float) psi, 0.0f, 0.0f, R, R3);
            double maxD = 0.0;
            for (int l = 1; l <= 3; ++l)
                for (int m = -l; m <= l; ++m)
                    for (int n = -l; n <= l; ++n)
                    {
                        const int    am  = m < 0 ? -m : m;
                        const double ang = (double) am * psi * kPi / 180.0;
                        double expected = 0.0;
                        if (m == 0 && n == 0)          expected = 1.0;
                        else if (m != 0 && n == m)     expected = std::cos (ang);
                        else if (m < 0 && n == -m)     expected = std::sin (ang);     // [−m][+m] slot: +sin
                        else if (m > 0 && n == -m)     expected = -std::sin (ang);    // [+m][−m] slot: −sin
                        maxD = std::max (maxD, std::abs ((double) R[osa::rotIndex (l, m, n)] - expected));
                    }
            check ("DSP-02 yaw-only closed form (psi 73): blocks 1-3 vs [[cos, sin], [-sin, cos]] on (-m, +m), max|err| < 1e-6",
                   maxD < 1.0e-6, fmt ("%.3e (expect ~1e-7 float)", maxD));
        }

        // Identity exact (R3 made mechanical): build (0, 0, 0) is 0 / 1 to the bit.
        {
            rot.build (0.0f, 0.0f, 0.0f, R, R3);
            bool zeros = true, ones = true;
            int  negZeros = 0;
            for (int l = 1; l <= 3; ++l)
                for (int m = -l; m <= l; ++m)
                    for (int n = -l; n <= l; ++n)
                    {
                        const float v = R[osa::rotIndex (l, m, n)];
                        if (m == n)
                            ones = ones && bitEqual (v, 1.0f);
                        else
                        {
                            zeros = zeros && v == 0.0f;
                            if (v == 0.0f && std::signbit (v))
                                ++negZeros;
                        }
                    }
            bool r3Identity = true;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    r3Identity = r3Identity && bitEqual (R3[i][j], i == j ? 1.0f : 0.0f);
            check ("identity exact: build (0, 0, 0) — all 83 elements exactly 0 or 1, diagonal bit-equal 1.0f, R3 bit-equal I (R3)",
                   zeros && ones && r3Identity,
                   fmt ("%d negative zero(s) among the off-diagonals (harmless: the lerp maps -0 to +0)", negZeros));
        }
    }

    //==========================================================================
    // Cardinal signs ×3 (D7) — through the processor FRAME, against literals.
    // Each a fresh instance with the tuple set BEFORE prepare (snapped: the
    // first frame is at steady state). A transposed Rz reads −90 at (a).
    {
        const auto frameAfterOneBlock = [] (float azDeg, float yawDeg, float pitchDeg, float rollDeg)
        {
            auto p = makeProc (2, 64, [azDeg, yawDeg, pitchDeg, rollDeg] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth, azDeg);
                setParam (q, ParamIDs::order,   2.0f);
                setParam (q, ParamIDs::yaw,     yawDeg);
                setParam (q, ParamIDs::pitch,   pitchDeg);
                setParam (q, ParamIDs::roll,    rollDeg);
            });
            std::vector<float> out;
            renderWith (*p, 2, { 64 }, noiseAt, out, 64);
            const osa::MeterFrame f = p->meterFrames.read();
            return f;
        };

        const auto a = frameAfterOneBlock (0.0f, 90.0f, 0.0f, 0.0f);
        check ("cardinal (a): psi = +90 on a front source -> azRot = +90 and elRot = 0 (1e-3) [a transposed Rz reads -90]",
               std::abs (a.azRotDeg - 90.0f) < 1.0e-3f && std::abs (a.elRotDeg) < 1.0e-3f,
               fmt ("azRot %.5f, elRot %.5f", (double) a.azRotDeg, (double) a.elRotDeg));

        const auto b = frameAfterOneBlock (0.0f, 0.0f, 90.0f, 0.0f);
        check ("cardinal (b): theta = +90 on a front source -> elRot = +90 (1e-3); azRot undefined at the pole, printed (R4)",
               std::abs (b.elRotDeg - 90.0f) < 1.0e-3f,
               fmt ("elRot %.5f, azRot %.3f (pole)", (double) b.elRotDeg, (double) b.azRotDeg));

        const auto c = frameAfterOneBlock (90.0f, 0.0f, 0.0f, 90.0f);
        check ("cardinal (c): phi = +90 on a LEFT source (az +90) -> elRot = +90 (1e-3); azRot printed (R4)",
               std::abs (c.elRotDeg - 90.0f) < 1.0e-3f,
               fmt ("elRot %.5f, azRot %.3f (pole)", (double) c.elRotDeg, (double) c.azRotDeg));
    }

    //==========================================================================
    // FUNC-01 third box — yaw = −azimuth brings the source back to the front:
    // processor az +90 / psi −90 / order 3 / Stereo, 100 Hz tone, 1 s.
    {
        constexpr int total = 48000;
        auto p = makeProc (2, 64, [] (Proc& q)
        {
            setParam (q, ParamIDs::azimuth,  90.0f);
            setParam (q, ParamIDs::yaw,     -90.0f);
            setParam (q, ParamIDs::order,     2.0f);
            setParam (q, ParamIDs::decoder,   1.0f);
        });
        const auto out = renderHooked (*p, total, [] (int n) { return tone100 (n); }, [] (int) {});
        const osa::MeterFrame f = p->meterFrames.read();

        double front[16];
        ReferenceModel::refSh16 (0.0, 0.0, front);
        const double maxG = maxAbsDiff16 (f.shGain, front);
        check ("FUNC-01 yaw = -azimuth (az +90, psi -90, order 3): frame shGain == refSh16 (0, 0) within 1e-5 on all 16",
               maxG < 1.0e-5, fmt ("max|diff| %.3e", maxG));
        check ("FUNC-01 yaw = -azimuth: azEffDeg == 90.0f and elEffDeg == 0 bit-equal (the puck does not move)",
               bitEqual (f.azEffDeg, 90.0f) && f.elEffDeg == 0.0f,
               fmt ("azEff %.6f elEff %.6f, azRot %.5f elRot %.5f", (double) f.azEffDeg, (double) f.elEffDeg,
                    (double) f.azRotDeg, (double) f.elRotDeg));

        double maxLR = 0.0, maxL = 0.0;
        for (int n = 0; n < total; ++n)
        {
            const double L = out[(size_t) n * 2], R = out[(size_t) n * 2 + 1];
            maxLR = std::max (maxLR, std::abs (L - R));
            maxL  = std::max (maxL, std::abs (L));
        }
        check ("FUNC-01 yaw = -azimuth: Stereo output max |L - R| < 1e-5 over 48000 samples (front symmetry), max |L| > 0.1 (live)",
               maxLR < 1.0e-5 && maxL > 0.1, fmt ("max|L - R| %.3e, max|L| %.3f", maxLR, maxL));
    }

    //==========================================================================
    // P1 null ROTATED — the P1 tuple + psi 25 / theta −40 / phi 70 vs the
    // ReferenceModel (which rotates by re-encoding, D6). digest-2.2's input.
    const auto rotatedNullSetup = [] (Proc& q)
    {
        setParam (q, ParamIDs::azimuth,   37.0f);
        setParam (q, ParamIDs::elevation, -12.0f);
        setParam (q, ParamIDs::distance,  3.3f);
        setParam (q, ParamIDs::order,     2.0f);
        setParam (q, ParamIDs::decoder,   1.0f);
        setParam (q, ParamIDs::yaw,       25.0f);
        setParam (q, ParamIDs::pitch,    -40.0f);
        setParam (q, ParamIDs::roll,      70.0f);
    };
    std::vector<float> rotRender;
    rotRender.reserve ((size_t) kRenderSamples * 2);
    {
        auto p = makeProc (2, kDefaultBlock, rotatedNullSetup);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, rotRender);

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
            maxErr = std::max (maxErr, std::abs ((double) rotRender[(size_t) n * 2]     - L[(size_t) n]));
            maxErr = std::max (maxErr, std::abs ((double) rotRender[(size_t) n * 2 + 1] - R[(size_t) n]));
            maxAbs = std::max (maxAbs, std::abs (L[(size_t) n]));
        }
        check ("P1 null rotated: processor vs ReferenceModel max|err| < 1e-5 (az 37, el -12, d 3.3, order 3, Stereo, psi 25, theta -40, phi 70)",
               maxErr < 1.0e-5 && maxAbs > 0.01,
               fmt ("max|err| %.3e over %d samples x 2 ch, peak |L| %.3f", maxErr, kRenderSamples, maxAbs));
        check ("P1 null rotated liveness: the rotated render differs from the 2.1 null render", ! buffersBitEqual (rotRender, nullRender));
    }

    // digest-2.2 VERDICT (2.3, D13 / C16): the rotated P1-null render is a
    // Stereo render; the binaural leg must not move it by a bit.
    check ("digest-2.2 verdict: fnv1a64 (rotated P1-null render) == 7f35bc9f91e321b9 (Stereo path untouched by 2.3, D2 / D13)",
           fnv1a64 (rotRender) == 0x7f35bc9f91e321b9ULL,
           fmt ("%016llx", (unsigned long long) fnv1a64 (rotRender)));

    //==========================================================================
    // Meter frame (2.2) — shGain / azRot / elRot vs the reference's rotated
    // direction; azEff / elEff pinned to the parameters; then under a yaw hook.
    {
        auto p = makeProc (2, kDefaultBlock, rotatedNullSetup);
        std::vector<float> out;
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, 1536);
        const osa::MeterFrame f = p->meterFrames.read();

        double azR, elR;
        ReferenceModel::rotatedDirection (37.0, -12.0, 25.0, -40.0, 70.0, azR, elR);
        double ref[16];
        gatedRefSh16 (azR, elR, 3, ref);
        const double maxG = maxAbsDiff16 (f.shGain, ref);
        check ("meter frame (2.2): shGain == refSh16 (rotatedDirection (37, -12 | 25, -40, 70)) within 1e-5 (order 3, all 16)",
               maxG < 1.0e-5, fmt ("max|diff| %.3e", maxG));
        check ("meter frame (2.2): azRot / elRot within 1e-3 of the reference's rotated direction",
               std::abs ((double) f.azRotDeg - azR) < 1.0e-3 && std::abs ((double) f.elRotDeg - elR) < 1.0e-3,
               fmt ("azRot %.4f (ref %.4f), elRot %.4f (ref %.4f)", (double) f.azRotDeg, azR, (double) f.elRotDeg, elR));
        // Against the host-visible RAW values, not the literals: the APVTS float
        // round-trip of −12 is −11.999999 (−90 + 180·(78/180) in float) — the
        // same reason refParamsFrom reads the raw atomics. The engine must
        // publish exactly what the host holds.
        const float rawAz = rawParam (*p, ParamIDs::azimuth), rawEl = rawParam (*p, ParamIDs::elevation);
        check ("meter frame (2.2): azEffDeg / elEffDeg bit-equal to the raw azimuth / elevation parameters (pre-rotation puck, UI-02)",
               bitEqual (f.azEffDeg, rawAz) && bitEqual (f.elEffDeg, rawEl),
               fmt ("azEff %.7f (raw %.7f), elEff %.7f (raw %.7f)", (double) f.azEffDeg, (double) rawAz, (double) f.elEffDeg, (double) rawEl));
    }
    {
        // Yaw hook 0 → 90° at 0.25 s across 10 frame reads: azEff pinned, azRot moves ≥ 60°.
        auto p = makeProc (2, 64, [] (Proc& q)
        {
            setParam (q, ParamIDs::azimuth,   37.0f);
            setParam (q, ParamIDs::elevation, -12.0f);
            setParam (q, ParamIDs::order,     2.0f);
        });
        bool  azPinned = true;
        float firstRot = 0.0f, lastRot = 0.0f;
        juce::String detail;
        int pos = 0;
        for (int k = 0; k < 10; ++k)
        {
            const int start = pos;
            renderHooked (*p, 2432, [start] (int n) { return monoNoiseAt (start + n); },
                          [&p, start] (int n) { setParam (*p, ParamIDs::yaw, start + n >= 12000 ? 90.0f : 0.0f); });
            pos += 2432;
            const osa::MeterFrame f = p->meterFrames.read();
            azPinned = azPinned && bitEqual (f.azEffDeg, 37.0f);
            if (k == 0)
                firstRot = f.azRotDeg;
            lastRot = f.azRotDeg;
            detail << fmt ("%.1f ", (double) f.azRotDeg);
        }
        check ("meter frame (2.2): under a yaw hook 0 -> 90 at 0.25 s, azEffDeg == 37.0f bit-equal at all 10 reads and azRot moves >= 60 deg",
               azPinned && std::abs (osa::wrapTo180 (lastRot - firstRot)) >= 60.0f,
               fmt ("azRot reads: %s", detail.toRawUTF8()));
    }

    //==========================================================================
    // Round trip (D2 / F2) — psi 0 → 90 → 0 on the 2.1 null tuple: the identity
    // fast path is reachable again and a static render after a move is
    // bit-identical to a fresh render, through the recursion.
    const auto nullTupleSetup = [] (Proc& q)
    {
        setParam (q, ParamIDs::azimuth,   37.0f);
        setParam (q, ParamIDs::elevation, -12.0f);
        setParam (q, ParamIDs::distance,  3.3f);
        setParam (q, ParamIDs::order,     2.0f);
        setParam (q, ParamIDs::decoder,   1.0f);
    };
    {
        constexpr int total = 72000, win = 8192;
        const auto gen = [] (int n) { return monoNoiseAt (n); };

        auto ps = makeProc (2, 64, nullTupleSetup);
        const auto stat = renderHooked (*ps, total, gen, [] (int) {});

        auto pm = makeProc (2, 64, nullTupleSetup);
        const auto moved = renderHooked (*pm, total, gen, [&pm] (int n)
        {
            if (n == 0)     setParam (*pm, ParamIDs::yaw, 90.0f);
            if (n == 24000) setParam (*pm, ParamIDs::yaw, 0.0f);
        });
        const osa::MeterFrame f = pm->meterFrames.read();
        check ("round trip: after psi 0 -> 90 -> 0 the frame's azRotDeg == azEffDeg bit-equal (identity fast path reachable again, F2)",
               bitEqual (f.azRotDeg, f.azEffDeg) && bitEqual (f.elRotDeg, f.elEffDeg),
               fmt ("azRot %.6f azEff %.6f", (double) f.azRotDeg, (double) f.azEffDeg));

        const size_t tail = (size_t) (total - win) * 2, mid = (size_t) (24000 - win) * 2, winBytes = (size_t) win * 2 * sizeof (float);
        int firstDiff = -1;
        for (size_t i = tail; i < (size_t) total * 2 && firstDiff < 0; ++i)
            if (! bitEqual (stat[i], moved[i]))
                firstDiff = (int) (i / 2);
        check ("round trip: last 8192 samples memcmp-equal to a fresh static render (static after a move == fresh render)",
               std::memcmp (stat.data() + tail, moved.data() + tail, winBytes) == 0,
               firstDiff < 0 ? juce::String ("identical") : juce::String ("first differing sample ") + juce::String (firstDiff));
        check ("round trip liveness: the window before the return differs from the static render",
               std::memcmp (stat.data() + mid, moved.data() + mid, winBytes) != 0);
    }

    //==========================================================================
    // Yaw sweep click gate (B8 / B9) — engine-direct 16-channel bus tap, az 37 /
    // el −12 / d 1 / order 3 / Stereo (every channel live, R6), sweep started AT
    // the prepared 0° (the 2.1 −180 idiom hides a 180° step in the render head),
    // vs 24 static parks at 15°. Negative control: snapped reference on the
    // m ≠ 0 channels. Zonal invariance (F3) on 0 / 2 / 6 / 12.
    constexpr int kSkip22  = 64 + 960;
    constexpr int kTotal22 = kSkip22 + 48000;
    const auto toneGen22 = [] (int n) { return tone100 (n); };
    const auto yawSweepHook = [] (int pos, auto& p) { p.yawDeg = osa::wrapTo180 (0.48f * (float) (pos / 64)); };

    osa::Params sweepPrm;
    sweepPrm.azimuthDeg   = 37.0f;
    sweepPrm.elevationDeg = -12.0f;
    sweepPrm.distanceM    = 1.0f;
    sweepPrm.order        = 3;
    sweepPrm.decoder      = 1;

    std::vector<EngineRender> parks;
    double parkControl[16] = {};
    for (int k = 0; k < 24; ++k)
    {
        osa::Params pk = sweepPrm;
        pk.yawDeg = 15.0f * (float) k;
        parks.push_back (renderEngine (pk, kFs, 64, kTotal22, { 64 }, toneGen22, [] (int, osa::Params&) {}, true));
        for (int ch = 0; ch < 16; ++ch)
            parkControl[ch] = std::max (parkControl[ch], worstCurvature (parks.back().tap16, ch, 16, kSkip22, kTotal22));
    }
    {
        const auto ev = renderEngine (sweepPrm, kFs, 64, kTotal22, { 64 }, toneGen22, yawSweepHook, true);

        double worstRatio = 0.0;
        int    worstCh    = -1;
        juce::String perCh;
        for (int ch = 0; ch < 16; ++ch)
        {
            const double r = tapRatio (ev.tap16, ch, parkControl, kSkip22, kTotal22);
            perCh << fmt ("%d:%.3f ", ch, r);
            if (r > worstRatio)
            {
                worstRatio = r;
                worstCh    = ch;
            }
        }
        check ("yaw sweep click (B8/B9): engine bus tap vs 24 static parks (15 deg), ratio <= 2.0 on all 16 channels (az 37 / el -12 / order 3, +0.48 deg/block from the prepared 0)",
               worstRatio <= kClickRatioCeiling,
               fmt ("worst ratio %.4f at ACN %d (expect ~1.13 at ACN 9); per channel: %s", worstRatio, worstCh, perCh.toRawUTF8()));

        // Negative control on the m ≠ 0 channels (0 / 2 / 6 / 12 are yaw-invariant and read exactly 1 — F3).
        ReferenceModel snapped;
        snapped.snapCoefficientsPerGrid = true;
        const auto bad = renderRef (snapped, refParamsFrom (sweepPrm), kFs, kTotal22, 64, toneGen22, yawSweepHook, true);
        const int mNonZero[12] = { 1, 3, 4, 5, 7, 8, 9, 10, 11, 13, 14, 15 };
        double badMax = 0.0, badMin = 1.0e300;
        for (int ch : mNonZero)
        {
            const double r = tapRatio (bad.tap16, ch, parkControl, kSkip22, kTotal22);
            badMax = std::max (badMax, r);
            badMin = std::min (badMin, r);
        }
        checkFailsAsExpected (badMax <= kClickRatioCeiling,
                              "yaw sweep negative control: snapped-per-grid reference FAILS the bus-tap gate on the 12 m != 0 channels (expect >= 49)",
                              fmt ("ratio min %.1f, max %.1f", badMin, badMax));

        // Zonal invariance (F3): rotation about z preserves the m = 0 harmonics.
        bool zonal = true;
        int  firstBad = -1;
        for (int n = 0; n < kTotal22 && zonal; ++n)
            for (int ch : { 0, 2, 6, 12 })
                if (! bitEqual (ev.tap16[(size_t) (n * 16 + ch)], parks[0].tap16[(size_t) (n * 16 + ch)]))
                {
                    zonal    = false;
                    firstBad = n;
                    break;
                }
        check ("zonal invariance (F3): ACN 0 / 2 / 6 / 12 of the yaw-sweep tap bit-identical to the yaw = 0 park (rotation about z preserves the zonal harmonics)",
               zonal, firstBad < 0 ? juce::String ("identical over the render") : juce::String ("first differing sample ") + juce::String (firstBad));

        //──────────────────────────────────────────────────────────────────────
        // Yaw step click (B8): 0 → 180 at 0.5 s. Verdict vs the SAME-MOTION
        // reference (lerped); negative control = snapped reference; the
        // vs-static numbers are printed as information only (R2).
        {
            const auto stepHook = [] (int pos, auto& p) { p.yawDeg = pos >= 24000 ? 180.0f : 0.0f; };
            const auto evStep = renderEngine (sweepPrm, kFs, 64, kTotal22, { 64 }, toneGen22, stepHook, true);

            ReferenceModel lerped, snappedStep;
            snappedStep.snapCoefficientsPerGrid = true;
            const RefParams rp = refParamsFrom (sweepPrm);
            const auto ctrl = renderRef (lerped, rp, kFs, kTotal22, 64, toneGen22, stepHook, true);
            const auto badS = renderRef (snappedStep, rp, kFs, kTotal22, 64, toneGen22, stepHook, true);

            double ctrlC[16];
            for (int ch = 0; ch < 16; ++ch)
                ctrlC[ch] = worstCurvature (ctrl.tap16, ch, 16, kSkip22, kTotal22);

            double worstStep = 0.0;
            int    worstStepCh = -1;
            juce::String perChS;
            for (int ch = 0; ch < 16; ++ch)
            {
                const double r = tapRatio (evStep.tap16, ch, ctrlC, kSkip22, kTotal22);
                perChS << fmt ("%d:%.4f ", ch, r);
                if (r > worstStep)
                {
                    worstStep   = r;
                    worstStepCh = ch;
                }
            }
            check ("yaw step click (B8): 0 -> 180 at 0.5 s, engine bus tap vs the same-motion ReferenceModel, ratio <= 2.0 on all 16 channels",
                   worstStep <= kClickRatioCeiling,
                   fmt ("worst %.4f at ACN %d (expect 1.000); per channel: %s", worstStep, worstStepCh, perChS.toRawUTF8()));

            // On the m ≠ 0 channels: a pure yaw leaves the zonal 2 / 6 / 12 untouched,
            // so snapped == lerped there (ratio exactly 1, F3) — not a zipper to detect.
            double badSMax = 0.0, badSMin = 1.0e300;
            for (int ch : mNonZero)
            {
                const double r = tapRatio (badS.tap16, ch, ctrlC, kSkip22, kTotal22);
                badSMax = std::max (badSMax, r);
                badSMin = std::min (badSMin, r);
            }
            checkFailsAsExpected (badSMax <= kClickRatioCeiling,
                                  "yaw step negative control: snapped reference vs the lerped reference FAILS on the 12 m != 0 channels (expect >> 2)",
                                  fmt ("ratio min %.1f, max %.1f", badSMin, badSMax));

            // Informational: vs the max of the yaw 0 / yaw 180 parks, per band.
            juce::String bands;
            for (int band = 1; band <= 3; ++band)
            {
                double worstBand = 0.0;
                for (int ch = band * band; ch < (band + 1) * (band + 1); ++ch)
                {
                    const double parkC = std::max (worstCurvature (parks[0].tap16, ch, 16, kSkip22, kTotal22),
                                                   worstCurvature (parks[12].tap16, ch, 16, kSkip22, kTotal22));
                    worstBand = std::max (worstBand, parkC > 1.0e-300 ? worstCurvature (evStep.tap16, ch, 16, kSkip22, kTotal22) / parkC : 0.0);
                }
                bands << fmt ("band %d: %.1fx  ", band, worstBand);
            }
            std::printf ("  yaw step info (R2, NOT a verdict): engine vs the max of the yaw 0 / yaw 180 static parks — %s"
                         "(expect ~12 / 25 / 44): grid-boundary kinks of the one-pole + per-sample lerp, identical in kind to the 2.1 azimuth "
                         "path's response to a 180 deg azimuth step; audibility is a hands-on item at verify (v1.1: longer tau for jumps, "
                         "quadratic element interpolation)\n", bands.toRawUTF8());
        }
    }

    //==========================================================================
    // Yaw ≡ azimuth cross-gate + liveness (R6) — a FRONT source: the yaw sweep
    // must equal the identical azimuth sweep (binds 2.2 to 2.1's proven path,
    // independent of any rotation matrix in the reference).
    {
        osa::Params front = sweepPrm;
        front.azimuthDeg   = 0.0f;
        front.elevationDeg = 0.0f;

        const auto A = renderEngine (front, kFs, 64, kTotal22, { 64 }, toneGen22, yawSweepHook, true);
        const auto B = renderEngine (front, kFs, 64, kTotal22, { 64 }, toneGen22,
                                     [] (int pos, osa::Params& p) { p.azimuthDeg = osa::wrapTo180 (0.48f * (float) (pos / 64)); }, true);

        double maxTap = 0.0, maxLr = 0.0;
        for (int n = kSkip22; n < kTotal22; ++n)
        {
            for (int ch = 0; ch < 16; ++ch)
                maxTap = std::max (maxTap, std::abs ((double) A.tap16[(size_t) (n * 16 + ch)] - (double) B.tap16[(size_t) (n * 16 + ch)]));
            for (int ch = 0; ch < 2; ++ch)
                maxLr = std::max (maxLr, std::abs ((double) A.lr[(size_t) (n * 2 + ch)] - (double) B.lr[(size_t) (n * 2 + ch)]));
        }
        check ("yaw == azimuth cross-gate (R6): front source, yaw sweep vs the identical azimuth sweep — max |tap16 diff| < 1e-5 on all 16 channels after the skip",
               maxTap < 1.0e-5, fmt ("%.3e (expect ~1.5e-7)", maxTap));
        check ("yaw == azimuth cross-gate: max |L / R diff| < 1e-5", maxLr < 1.0e-5, fmt ("%.3e", maxLr));

        const double e1 = energy (A.lr, 0, 2, kSkip22, kSkip22 + 24000), e2 = energy (A.lr, 0, 2, kSkip22 + 24000, kTotal22);
        const double live = 10.0 * std::log10 (e2 / e1);
        check ("yaw sweep liveness: Stereo L energy second half-circle vs first differs >= 6 dB (front source)", std::abs (live) >= 6.0,
               fmt ("%.2f dB (expect ~8.2)", live));
    }

    //==========================================================================
    // P2c / P3c (B10) — the P2b / P3b shape with a STATIC non-identity rotation
    // + orbit On: ragged / oversized ≡ flat. P2b / P3b themselves are untouched.
    const auto rotatedMovingSetup = [&movingSetup] (Proc& q)
    {
        movingSetup (q);
        setParam (q, ParamIDs::yaw,    25.0f);
        setParam (q, ParamIDs::pitch, -40.0f);
        setParam (q, ParamIDs::roll,   70.0f);
    };
    {
        auto p = makeProc (2, kDefaultBlock, rotatedMovingSetup);
        std::vector<float> flatRot;
        flatRot.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, flatRot);
        check ("P2c liveness: a static non-identity rotation changes the orbit render (flat != 2.1 flatMoving)",
               ! buffersBitEqual (flatRot, flatMoving));

        auto q = makeProc (2, 4096, rotatedMovingSetup);
        std::vector<float> ragged;
        ragged.reserve ((size_t) kRenderSamples * 2);
        renderWith (*q, 2, { 1, 7, 64, 333, 4096, 13, 512, 2 }, noiseAt, ragged);
        check ("P2c ragged == flat with orbit On 2 Hz / order 3 / psi 25 theta -40 phi 70", buffersBitEqual (ragged, flatRot));

        auto small = makeProc (2, 64, rotatedMovingSetup);
        std::vector<float> outSmall;
        outSmall.reserve ((size_t) kRenderSamples * 2);
        renderWith (*small, 2, { 4096 }, noiseAt, outSmall);

        auto big = makeProc (2, 4096, rotatedMovingSetup);
        std::vector<float> outBig;
        outBig.reserve ((size_t) kRenderSamples * 2);
        renderWith (*big, 2, { 4096 }, noiseAt, outBig);

        check ("P3c oversized (prep 64, feed 4096) == prep 4096 with rotation + orbit On", buffersBitEqual (outSmall, outBig));
        check ("P3c oversized == flat with rotation + orbit On", buffersBitEqual (outBig, flatRot));
    }

    //==========================================================================
    // P2d (B10) — yaw / pitch / roll stepped ONLY at 4096-sample boundaries:
    // 64-block render == 4096-block render (host-side changes land on a block
    // boundary of both renders).
    {
        constexpr int total = 32768;
        const auto gen = [] (int n) { return monoNoiseAt (n); };
        const auto hookFor = [] (Proc& p)
        {
            return [&p] (int n)
            {
                if (n % 4096 == 0)
                {
                    const float k = (float) (n / 4096);
                    setParam (p, ParamIDs::yaw,   osa::wrapTo180 (30.0f * k));
                    setParam (p, ParamIDs::pitch, osa::wrapTo180 (-20.0f * k));
                    setParam (p, ParamIDs::roll,  osa::wrapTo180 (45.0f * k));
                }
            };
        };

        auto p64 = makeProc (2, 64, nullTupleSetup);
        const auto r64 = renderHooked (*p64, total, gen, hookFor (*p64), 64);

        auto p4096 = makeProc (2, 4096, nullTupleSetup);
        const auto r4096 = renderHooked (*p4096, total, gen, hookFor (*p4096), 4096);

        auto p0 = makeProc (2, 64, nullTupleSetup);
        const auto r0 = renderHooked (*p0, total, gen, [] (int) {}, 64);

        check ("P2d: yaw / pitch / roll stepped at every 4096-sample boundary — 64-block render == 4096-block render (bit)",
               buffersBitEqual (r64, r4096));
        check ("P2d liveness: the hooked render differs from the hook-less render", ! buffersBitEqual (r64, r0));
    }

    //==========================================================================
    // Extremes (2.2) — 8 corners over the signs of (psi, theta, phi) = ±180°,
    // az ±180 / el ±90 / d 0.2 | 20 / decoder alternating, order 3, orbit On
    // 2 Hz, at 48 k and 96 k; plus gimbal theta = +90 under a yaw sweep.
    {
        for (double sr : { 48000.0, 96000.0 })
        {
            bool finite = true, live = true;
            int  badCase = -1;
            for (int i = 0; i < 8; ++i)
            {
                auto p = makeProcAt (sr, 2, kDefaultBlock, [i] (Proc& q)
                {
                    setParam (q, ParamIDs::yaw,       (i & 1) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::pitch,     (i & 2) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::roll,      (i & 4) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::azimuth,   (i & 1) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::elevation, (i & 2) ? 90.0f : -90.0f);
                    setParam (q, ParamIDs::distance,  (i & 4) ? 20.0f : 0.2f);
                    setParam (q, ParamIDs::decoder,   (i & 1) ? 1.0f : 0.0f);
                    setParam (q, ParamIDs::order,     2.0f);
                    setParam (q, ParamIDs::orbit,     1.0f);
                    setParam (q, ParamIDs::orbitRate, 2.0f);
                });
                std::vector<float> out;
                renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, 4096);
                const bool f = allFinite (out);
                bool nonZero = false;
                for (float v : out)
                    if (v != 0.0f) { nonZero = true; break; }
                finite = finite && f;
                live   = live && nonZero;
                if ((! f || ! nonZero) && badCase < 0)
                    badCase = i;
            }
            check ((juce::String ("extremes (2.2): 8 corners of (psi, theta, phi) = +/-180 x 4096 samples at fs ") + juce::String ((int) sr)
                    + " finite and not silenced").toRawUTF8(), finite && live,
                   badCase < 0 ? juce::String ("all 8 ok") : juce::String ("first bad case ") + juce::String (badCase));
        }

        // Gimbal lock (F5): theta = +90 exactly, yaw sweeping — finite, live, frame finite.
        {
            auto p = makeProc (2, 64, [] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth,   37.0f);
                setParam (q, ParamIDs::elevation, -12.0f);
                setParam (q, ParamIDs::pitch,     90.0f);
                setParam (q, ParamIDs::order,     2.0f);
            });
            const auto out = renderHooked (*p, 8192, [] (int n) { return monoNoiseAt (n); },
                                           [&p] (int n) { setParam (*p, ParamIDs::yaw, osa::wrapTo180 (0.48f * (float) (n / 64))); });
            bool nonZero = false;
            for (float v : out)
                if (v != 0.0f) { nonZero = true; break; }
            const osa::MeterFrame f = p->meterFrames.read();
            check ("extremes (2.2): gimbal theta = +90 under a yaw sweep — every sample finite and live, frame angles finite",
                   allFinite (out) && nonZero && std::isfinite (f.azRotDeg) && std::isfinite (f.elRotDeg),
                   fmt ("azRot %.3f elRot %.3f", (double) f.azRotDeg, (double) f.elRotDeg));
        }
    }

    //==========================================================================
    //==========================================================================
    // ── Phase 2.3 — the binaural leg (PLAN-2.3 T7) ───────────────────────────
    //==========================================================================

    const auto noEngineHook = [] (int, osa::Params&) {};

    //==========================================================================
    // header sanity — the generated artifact's contract.
    {
        bool finite = true, tap0 = true;
        for (int order = 1; order <= 3; ++order)
            for (int ch = 0; ch < osa::BinauralConvolver::channelsFor (order); ++ch)
                for (int ear = 0; ear < 2; ++ear)
                {
                    const float* h = headerTaps (order, ch, ear);
                    tap0 = tap0 && h[0] == 0.0f;
                    for (int t = 0; t < osa::kBinauralFilterTaps; ++t)
                        finite = finite && std::isfinite (h[t]);
                }
        check ("header sanity: fs 48000, 256 taps, onsets[1..3] > 0, every float finite (3 sets), level match within +/-3 dB, tap 0 of every FIR exactly 0",
               osa::kBinauralFilterSampleRate == 48000 && osa::kBinauralFilterTaps == 256
                   && osa::kBinauralOnsetFrontSamples[1] > 0 && osa::kBinauralOnsetFrontSamples[2] > 0 && osa::kBinauralOnsetFrontSamples[3] > 0
                   && finite && std::abs (osa::kBinauralLevelMatchDb) <= 3.0f && tap0,
               fmt ("onsets {%d, %d, %d, %d}, level match %+.3f dB, trim %d, blend %s, f_a {%.1f, %.1f, %.1f} Hz, DFE %s",
                    osa::kBinauralOnsetFrontSamples[0], osa::kBinauralOnsetFrontSamples[1], osa::kBinauralOnsetFrontSamples[2], osa::kBinauralOnsetFrontSamples[3],
                    (double) osa::kBinauralLevelMatchDb, osa::kBinauralFilterTrimSamples, osa::kBinauralFilterBlend,
                    (double) osa::kBinauralAliasingHz[1], (double) osa::kBinauralAliasingHz[2], (double) osa::kBinauralAliasingHz[3],
                    osa::kBinauralFilterDfe ? "as shipped" : "none"));
    }

    //==========================================================================
    // UPOLS-vs-direct null (D7) — the rotated null tuple in BINAURAL mode,
    // orders 1 / 2 / 3 × six host blocks vs the direct-form double reference.
    constexpr int kNullTotal = kRenderSamples + 64;
    const auto binauralNullSetup = [] (Proc& q, int orderIdx)
    {
        setParam (q, ParamIDs::azimuth,   37.0f);
        setParam (q, ParamIDs::elevation, -12.0f);
        setParam (q, ParamIDs::distance,  3.3f);
        setParam (q, ParamIDs::order,     (float) orderIdx);
        setParam (q, ParamIDs::decoder,   0.0f);
        setParam (q, ParamIDs::yaw,       25.0f);
        setParam (q, ParamIDs::pitch,    -40.0f);
        setParam (q, ParamIDs::roll,      70.0f);
    };
    std::vector<float> binauralNullRender;   // order 3 / block 512 — digest-2.3's input
    {
        for (int order = 1; order <= 3; ++order)
        {
            auto p0 = makeProc (2, kDefaultBlock, [&] (Proc& q) { binauralNullSetup (q, order - 1); });
            const auto ref = refRenderStatic (refParamsFrom (*p0), kFs, kNullTotal, monoNoiseAt);

            for (int block : { 32, 64, 100, 512, 1024, 4096 })
            {
                auto p = makeProc (2, block, [&] (Proc& q) { binauralNullSetup (q, order - 1); });
                std::vector<float> out;
                out.reserve ((size_t) kNullTotal * 2);
                renderWith (*p, 2, { block }, noiseAt, out, kNullTotal);

                double peak = 0.0;
                const double err = maxErrLr (out, ref, peak);
                check (fmt ("UPOLS null (D7): processor order %d / host block %d vs ReferenceModel direct-form: max|err| < 1e-5 (az 37 el -12 d 3.3 psi 25 theta -40 phi 70, Binaural)", order, block).toRawUTF8(),
                       err < 1.0e-5 && peak > 0.01, fmt ("max|err| %.3e (expect ~1e-6), peak |L| %.3f", err, peak));

                if (order == 3 && block == kDefaultBlock)
                    binauralNullRender = out;
            }
        }
    }

    //==========================================================================
    // null at 96 k (K = 8) — engine-direct, the reference seeded with the
    // engine's resampled table (D7; the resampler itself is gated below).
    {
        osa::Params prm;
        prm.azimuthDeg = 37.0f; prm.elevationDeg = -12.0f; prm.distanceM = 3.3f; prm.order = 3;
        prm.yawDeg = 25.0f; prm.pitchDeg = -40.0f; prm.rollDeg = 70.0f; prm.decoder = 0;

        osa::SpatialEngine seed;
        seed.setParams (prm);
        seed.prepare (96000.0, kDefaultBlock);
        ReferenceModel model;
        loadExternalFilters (model, seed);
        const auto ref = refRenderStatic (refParamsFrom (prm), 96000.0, kNullTotal, monoNoiseAt, &model);
        const auto r   = renderEngine (prm, 96000.0, kDefaultBlock, kNullTotal, { kDefaultBlock }, monoNoiseAt, noEngineHook);

        double peak = 0.0;
        const double err = maxErrLr (r.lr, ref, peak);
        check ("null at 96 k (K = 8): engine-direct Binaural vs ReferenceModel seeded with the engine's resampled table: max|err| < 1e-5",
               err < 1.0e-5 && peak > 0.01,
               fmt ("max|err| %.3e, peak |L| %.3f, K %d, L %d", err, peak, seed.convolver().partitions(), seed.convolver().tapsAtRate()));
    }

    //==========================================================================
    // resampler vs golden (C3) — the engine's tables at 44.1 k / 96 k vs the
    // scipy resample_poly goldens; un-calibrated negative control; 48 k bit-exact.
    {
        for (int fs : { 44100, 96000 })
        {
            osa::SpatialEngine e;
            osa::Params prm;
            e.setParams (prm);
            e.prepare ((double) fs, kDefaultBlock);
            const auto& c = e.convolver();
            const int L = c.tapsAtRate();
            const double tol = fs == 44100 ? 1.5e-2 : 6.0e-3;

            // Dev aid: OSA_DUMP_TABLES=<dir> writes every engine table and golden as text.
            if (const char* dir = std::getenv ("OSA_DUMP_TABLES"))
            {
                for (int order = 1; order <= 3; ++order)
                    for (int ch = 0; ch < osa::BinauralConvolver::channelsFor (order); ++ch)
                        for (int ear = 0; ear < 2; ++ear)
                        {
                            const juce::String path = juce::String (dir) + fmt ("/t%d_o%d_c%d_e%d.txt", fs, order, ch, ear);
                            if (FILE* f = std::fopen (path.toRawUTF8(), "w"))
                            {
                                for (int i = 0; i < L; ++i)
                                    std::fprintf (f, "%.9g %.9g\n", (double) c.filterTaps (order, ch, ear)[i], (double) goldenTaps (fs, order, ch, ear)[i]);
                                std::fclose (f);
                            }
                        }
            }

            // EXECUTE FINDING (a C3 gate correction): the two resamplers agree within ±0.02 dB
            // below 18 kHz, and differ only in their anti-alias TRANSITION BANDS above it
            // (WindowedSinc passes up to +6 dB more at 20–22 kHz at 44.1 k than scipy's
            // Kaiser β = 5) — inaudible HRTF content, but the HF-rich order-3 channels put
            // it straight into a raw time-domain max|err| (0.22 of peak), which research's
            // LF-heavy test FIR never showed. So the gate is taken where BOTH designs are
            // transparent: both tables brick-wall low-passed at 16 kHz (4096-point FFT), the
            // research tolerances on that; |H| within 0.1 dB below 16 kHz on bins ≥ −20 dB re
            // the FIR's spectral peak; the 16–22 kHz design difference printed.
            // Band: 16 kHz, or 0.68 × the output Nyquist when that is lower — WindowedSinc's
            // 44.1 k downsampling roll-off starts at ≈ 15.7 kHz (−0.14 dB there), so 15.0 kHz.
            const double kBandHz = std::min (16000.0, 0.68 * (double) fs / 2.0);
            double worstRel = 0.0, worstRelRaw = 0.0, worstDb = 0.0, worstRawTd = 0.0;
            int worstOrder = 0, worstCh = 0;
            double hiBandMaxDb = 0.0;
            Fft4096 fe, fg, feRaw;
            std::vector<float> tabLp ((size_t) kFft4096), gLp ((size_t) kFft4096), rawLp ((size_t) kFft4096);
            std::vector<float> scaled ((size_t) L);
            for (int order = 1; order <= 3; ++order)
                for (int ch = 0; ch < osa::BinauralConvolver::channelsFor (order); ++ch)
                    for (int ear = 0; ear < 2; ++ear)
                    {
                        const float* tab = c.filterTaps (order, ch, ear);
                        const float* g   = goldenTaps (fs, order, ch, ear);
                        for (int i = 0; i < L; ++i)
                        {
                            worstRawTd = std::max (worstRawTd, std::abs ((double) tab[i] - (double) g[i]));
                            scaled[(size_t) i] = tab[i] * c.resamplerKernelGain();   // un-calibrated: the kernel's −0.087 dB left in
                        }

                        fe.forward (tab, L);
                        fg.forward (g, L);
                        feRaw.forward (scaled.data(), L);

                        // |H| on the raw spectra below 16 kHz, −20 dB mask; the design difference above.
                        double gmax = 0.0;
                        for (int k = 0; k <= kFft4096 / 2; ++k)
                            gmax = std::max (gmax, fg.magnitude (k));
                        for (int k = 1; k <= kFft4096 / 2; ++k)
                        {
                            const double f = (double) k * fs / kFft4096;
                            if (fg.magnitude (k) < 0.1 * gmax)
                                continue;
                            const double d = std::abs (dB (fe.magnitude (k) / fg.magnitude (k)));
                            if (f < kBandHz)
                                worstDb = std::max (worstDb, d);
                            else if (f < 20000.0)
                                hiBandMaxDb = std::max (hiBandMaxDb, d);
                        }

                        // Both low-passed at 16 kHz → time domain.
                        fe.brickWallInverse (fs, kBandHz);
                        fg.brickWallInverse (fs, kBandHz);
                        feRaw.brickWallInverse (fs, kBandHz);
                        double peak = 0.0, err = 0.0, errRaw = 0.0;
                        for (int i = 0; i < kFft4096; ++i)
                        {
                            peak   = std::max (peak, std::abs ((double) fg.buf[(size_t) i]));
                            err    = std::max (err, std::abs ((double) fe.buf[(size_t) i] - (double) fg.buf[(size_t) i]));
                            errRaw = std::max (errRaw, std::abs ((double) feRaw.buf[(size_t) i] - (double) fg.buf[(size_t) i]));
                        }
                        if (err / peak > worstRel)
                        {
                            worstRel = err / peak;
                            worstOrder = order;
                            worstCh = ch;
                        }
                        worstRelRaw = std::max (worstRelRaw, errRaw / peak);
                    }

            const int expectL = fs == 44100 ? 256 : 512, expectK = expectL / 64;
            check (fmt ("resampler vs scipy golden at %d: both low-passed at %.0f Hz, max|err| <= %.1e of peak on all 29 x 2 FIRs; |H| within 0.1 dB below %.0f Hz (bins >= -20 dB); L %d / K %d",
                        fs, kBandHz, tol, kBandHz, expectL, expectK).toRawUTF8(),
                   worstRel <= tol && worstDb <= 0.1 && L == expectL && c.partitions() == expectK,
                   fmt ("worst rel err %.3e (order %d ch %d; expect ~%s), worst |H| diff %.4f dB in band; design difference band-20 kHz up to %.2f dB (raw unfiltered max|err| %.3e, info); resamplerKernelGain %.6f (expect ~0.99), divisor %.6f (= kernel / ratio), prefix %d, delay %d",
                        worstRel, worstOrder, worstCh, fs == 44100 ? "4e-3" : "2e-3", worstDb, hiBandMaxDb, worstRawTd,
                        (double) c.resamplerKernelGain(), (double) c.resamplerDcGain(), c.resamplerPrefix(), c.resamplerDelay()));

            if (fs == 96000)
                checkFailsAsExpected (worstRelRaw <= tol,
                                      "resampler negative control at 96 k: the UN-calibrated table (x resamplerKernelGain, the kernel's -0.087 dB left in) FAILS the 6e-3 gate (expect ~1.0e-2)",
                                      fmt ("raw rel err %.3e", worstRelRaw));
        }

        // Cross-rate level (execute finding): a 1 kHz front tone at order 3 / Binaural reads the
        // same amplitude at 44.1 / 96 k as at 48 k — an amplitude-preserving resample would be
        // +6 dB at 96 k, which no relative gate above can see. 1 s windows → exact bins.
        {
            double amp[3] = {};
            int idx = 0;
            for (double sr : { 48000.0, 44100.0, 96000.0 })
            {
                osa::Params prm;
                prm.order = 3;
                const int settle = 4096, win = (int) sr;
                const auto r = renderEngine (prm, sr, kDefaultBlock, settle + win, { kDefaultBlock },
                                             [sr] (int n) { return Sine { 1000.0, 0.5, sr }(n); }, noEngineHook);
                amp[idx++] = binMagnitude (r.lr, 0, 2, settle, win, 1000);
            }
            check ("resampler level: 1 kHz front tone, order 3 Binaural — 44.1 k and 96 k within 0.1 dB of 48 k (the resampled bank keeps its frequency response, not its waveform amplitude)",
                   std::abs (dB (amp[1] / amp[0])) <= 0.1 && std::abs (dB (amp[2] / amp[0])) <= 0.1,
                   fmt ("L amplitude 48 k %.5f, 44.1 k %.5f (%+.3f dB), 96 k %.5f (%+.3f dB)", amp[0], amp[1], dB (amp[1] / amp[0]), amp[2], dB (amp[2] / amp[0])));
        }

        osa::SpatialEngine e48;
        osa::Params prm;
        e48.setParams (prm);
        e48.prepare (48000.0, kDefaultBlock);
        bool exact = true;
        for (int order = 1; order <= 3 && exact; ++order)
            for (int ch = 0; ch < osa::BinauralConvolver::channelsFor (order) && exact; ++ch)
                for (int ear = 0; ear < 2 && exact; ++ear)
                    exact = std::memcmp (e48.convolver().filterTaps (order, ch, ear), headerTaps (order, ch, ear), 256 * sizeof (float)) == 0;
        check ("48 k table bit-exact to the header arrays (all 29 x 2 FIRs), resamplerDcGain == resamplerKernelGain == 1.0f, L 256 / K 4",
               exact && e48.convolver().resamplerDcGain() == 1.0f && e48.convolver().resamplerKernelGain() == 1.0f
                   && e48.convolver().tapsAtRate() == 256 && e48.convolver().partitions() == 4);
    }

    //==========================================================================
    // DSP-03 onset (C4) — front / d 1 / Binaural, impulse at 100, orders 1 / 3
    // at 44.1 / 48 / 96 k; latency 64 in both modes at every rate.
    {
        const auto impulse = [] (int, int n) { return n == 100 ? 1.0f : 0.0f; };

        // The expected onset comes from the ENGINE TABLE's own front response (Σ_ch Y_front[ch] ·
        // taps[ch]) at this rate — at 48 k that is the generator's definition on the same
        // floats, asserted EXACTLY equal to kBinauralOnsetFrontSamples[n]; at 44.1 / 96 k the
        // resampled waveform moves a 10 %-threshold crossing by a few samples (the L ear read
        // 237 vs a scaled 241 at 44.1 k while R read 241), so the scaled constant is printed,
        // not asserted. Tolerance ±2 at every rate (the engine's float path).
        const auto tableFrontOnset = [] (const osa::SpatialEngine& e, int order, int ear)
        {
            float y[16];
            osa::encodeSH16 (0.0f, 0.0f, y);
            const auto& c = e.convolver();
            std::vector<float> resp ((size_t) c.tapsAtRate(), 0.0f);
            for (int ch = 0; ch < osa::BinauralConvolver::channelsFor (order); ++ch)
                for (int t = 0; t < c.tapsAtRate(); ++t)
                    resp[(size_t) t] += y[ch] * c.filterTaps (order, ch, ear)[t];
            float peak = 0.0f;
            for (float v : resp)
                peak = std::max (peak, std::abs (v));
            for (int t = 0; t < c.tapsAtRate(); ++t)
                if (std::abs (resp[(size_t) t]) >= 0.1f * peak)
                    return t;
            return -1;
        };

        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (int order : { 1, 3 })
            {
                auto p = makeProcAt (sr, 2, kDefaultBlock, [order] (Proc& q) { setParam (q, ParamIDs::order, (float) (order - 1)); });
                std::vector<float> out;
                renderWith (*p, 2, { kDefaultBlock }, impulse, out, 4096);

                osa::SpatialEngine e;
                osa::Params prm;
                e.setParams (prm);
                e.prepare (sr, kDefaultBlock);
                const int tableOnset = tableFrontOnset (e, order, 0);
                const int scaledConst = (int) std::lround ((double) osa::kBinauralOnsetFrontSamples[order] * sr / 48000.0);
                const int expected = 100 + 64 + tableOnset;
                const int tol = 2;
                const int on = onset10 (out, 0), onR = onset10 (out, 1), fnz = firstNonZero (out);
                if (sr == 48000.0)
                    check (fmt ("DSP-03 onset constant at 48 k order %d: the engine table's front onset10 == kBinauralOnsetFrontSamples[%d] == %d exactly (generator definition)",
                                order, order, osa::kBinauralOnsetFrontSamples[order]).toRawUTF8(),
                           tableOnset == osa::kBinauralOnsetFrontSamples[order], fmt ("table onset %d", tableOnset));
                // Before the impulse's grid is transformed the FDL is all-zero → out[0 … 127] is
                // EXACTLY 0. From 128 (the grid after the impulse's) the FFT circular path leaves
                // roundoff (~1e-8) where the exact convolution is 0 — "first nonzero" is not a
                // property of an FFT convolver (F6); the pre-onset floor is gated at 1e-6 instead.
                bool headExact = true;
                double preOnset = 0.0;
                for (int n = 0; n < 128; ++n)
                    headExact = headExact && out[(size_t) n * 2] == 0.0f && out[(size_t) n * 2 + 1] == 0.0f;
                for (int n = 128; n < 164; ++n)
                    preOnset = std::max (preOnset, std::max (std::abs ((double) out[(size_t) n * 2]), std::abs ((double) out[(size_t) n * 2 + 1])));
                check (fmt ("DSP-03 onset at fs %.0f order %d: onset10 (L) == 100 + 64 + table front onset (%d) = %d +/- %d; out[0..127] exactly 0; |out[128..163]| < 1e-6 (FFT roundoff floor)",
                            sr, order, tableOnset, expected, tol).toRawUTF8(),
                       std::abs (on - expected) <= tol && headExact && preOnset < 1.0e-6 && allFinite (out),
                       fmt ("onset10 L %d (R %d; table R %d), scaled constant round (%d * fs / 48000) = %d (info), first nonzero %d (roundoff), pre-onset floor %.2e, reported latency %d",
                            on, onR, tableFrontOnset (e, order, 1), osa::kBinauralOnsetFrontSamples[order], scaledConst, fnz, preOnset, p->getLatencySamples()));
            }

        bool lat = true;
        juce::String detail;
        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (int dec : { 0, 1 })
            {
                auto p = makeProcAt (sr, 2, kDefaultBlock, [dec] (Proc& q) { setParam (q, ParamIDs::decoder, (float) dec); });
                lat = lat && p->getLatencySamples() == 64;
                detail << fmt ("%.0f/%s:%d ", sr, dec == 0 ? "bin" : "st", p->getLatencySamples());
            }
        check ("DSP-03 getLatencySamples() == 64 in both decoder modes at 44.1 / 48 / 96 k", lat, detail);
    }

    //==========================================================================
    // DSP-03 ILD / ITD (C10) — impulse at 100, el 0, d 1, orders 1 / 3, az ±90.
    {
        const auto impulse = [] (int, int n) { return n == 100 ? 1.0f : 0.0f; };
        for (int order : { 1, 3 })
            for (float azd : { 90.0f, -90.0f })
            {
                auto p = makeProc (2, kDefaultBlock, [order, azd] (Proc& q)
                {
                    setParam (q, ParamIDs::azimuth, azd);
                    setParam (q, ParamIDs::order,   (float) (order - 1));
                });
                std::vector<float> out;
                renderWith (*p, 2, { kDefaultBlock }, impulse, out, 4096);
                const int from = 164, len = 512;
                const double ild = ildDb (out, from, len);
                const double fc  = std::min (1500.0, 0.8 * (double) osa::kBinauralAliasingHz[order]);
                const int itd = itdLowPassXcorr (out, kFs, fc, from, len);
                const bool left = azd > 0.0f;
                check (fmt ("DSP-03 order %d az %+.0f: ILD %s 6 dB (%s louder, AmbiX sign) and ITD by LP < %.0f Hz xcorr in [25, 45] with %s leading",
                            order, (double) azd, left ? ">=" : "<=", left ? "L" : "R", fc, left ? "L" : "R").toRawUTF8(),
                       left ? (ild >= 6.0 && itd >= 25 && itd <= 45) : (ild <= -6.0 && itd <= -25 && itd >= -45),
                       fmt ("ILD %+.2f dB (expect %s), ITD %+d samples (expect %s); per band 200-1k/1-3k/3-8k/8-16k: %+.1f / %+.1f / %+.1f / %+.1f dB",
                            ild, order == 1 ? "+12.3 / -11.9" : "+13.9 / -14.6", itd, order == 1 ? "+/-36" : "+/-37",
                            ildBandDb (out, from, len, kFs, 200.0, 1000.0), ildBandDb (out, from, len, kFs, 1000.0, 3000.0),
                            ildBandDb (out, from, len, kFs, 3000.0, 8000.0), ildBandDb (out, from, len, kFs, 8000.0, 16000.0)));
            }
    }

    //==========================================================================
    // FUNC-05 level delta (C11) — front / d 1, static white noise, A-weighted
    // mean-of-ears power, Binaural vs Stereo at order 3 (orders 1 / 2 printed).
    {
        constexpr int total = 48000;
        double lvl[2][4] = {};
        for (int dec : { 0, 1 })
            for (int order = 1; order <= 3; ++order)
            {
                auto p = makeProc (2, kDefaultBlock, [dec, order] (Proc& q)
                {
                    setParam (q, ParamIDs::order,   (float) (order - 1));
                    setParam (q, ParamIDs::decoder, (float) dec);
                });
                std::vector<float> out;
                out.reserve ((size_t) total * 2);
                renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, total);
                const double l = aWeightedLevel (out, 0, 1024, total - 1024), r = aWeightedLevel (out, 1, 1024, total - 1024);
                lvl[dec][order] = std::sqrt (0.5 * (l * l + r * r));
            }
        const double d1 = dB (lvl[0][1] / lvl[1][1]), d2 = dB (lvl[0][2] / lvl[1][2]), d3 = dB (lvl[0][3] / lvl[1][3]);
        check ("FUNC-05 level delta (front, order 3, A-weighted white, mean-of-ears power): |Binaural - Stereo| < 1 dB",
               std::abs (d3) < 1.0,
               fmt ("order 3 %+.3f dB (expect ~0.0); order 1 %+.3f (expect -0.37), order 2 %+.3f (expect -0.03)", d3, d1, d2));
    }

    //==========================================================================
    // DSP-07 order 1→3 / 3→1 click (C11) — 100 Hz tone at az +90°, Binaural,
    // at 64 and 1024 host blocks vs the same-motion reference; snapped-bank
    // negative control; spectral liveness on a 3 kHz tone.
    constexpr int kSkip23  = 64 + 960 + 256;
    constexpr int kTotal23 = kSkip23 + 48000;
    const auto toneGen23 = [] (int n) { return tone100 (n); };
    {
        for (int block : { 64, 1024 })
            for (int dir = 0; dir < 2; ++dir)
            {
                const int from = dir == 0 ? 1 : 3, to = dir == 0 ? 3 : 1;
                auto p = makeProc (2, block, [from] (Proc& q)
                {
                    setParam (q, ParamIDs::azimuth, 90.0f);
                    setParam (q, ParamIDs::order,   (float) (from - 1));
                });
                const RefParams rp = refParamsFrom (*p);
                const auto ev = renderHooked (*p, kTotal23, toneGen23, [&p, to] (int n)
                {
                    if (n >= 24000)
                        setParam (*p, ParamIDs::order, (float) (to - 1));
                }, block);
                const auto refHook = [to] (int n, RefParams& r) { if (n >= 24000) r.order = to; };

                ReferenceModel lerped, snapped;
                snapped.snapBankFade   = true;
                snapped.snapOrderGains = true;
                const auto ctrl = renderRef (lerped,  rp, kFs, kTotal23, block, toneGen23, refHook);
                const auto bad  = renderRef (snapped, rp, kFs, kTotal23, block, toneGen23, refHook);

                ClickReading rL { worstCurvature (ev, 0, 2, kSkip23, kTotal23), worstCurvature (ctrl.lr, 0, 2, kSkip23, kTotal23) };
                ClickReading rR { worstCurvature (ev, 1, 2, kSkip23, kTotal23), worstCurvature (ctrl.lr, 1, 2, kSkip23, kTotal23) };
                const juce::String nm = fmt ("DSP-07 order %d->%d click at %d-sample blocks (100 Hz, az +90, Binaural) vs the same-motion reference", from, to, block);
                clickVerdict ((nm + " [L]").toRawUTF8(), rL,
                              fmt ("; static endpoints L %.3e / %.3e", worstCurvature (ev, 0, 2, kSkip23, 24000), worstCurvature (ev, 0, 2, 24000 + kSkip23, kTotal23)));
                clickVerdict ((nm + " [R]").toRawUTF8(), rR);

                ClickReading bL { worstCurvature (bad.lr, 0, 2, kSkip23, kTotal23), rL.control };
                ClickReading bR { worstCurvature (bad.lr, 1, 2, kSkip23, kTotal23), rR.control };
                checkFailsAsExpected (bL.ratio() <= kClickRatioCeiling && bR.ratio() <= kClickRatioCeiling,
                                      (fmt ("DSP-07 negative control order %d->%d at %d blocks: snapped bank + snapped order gains FAIL the gate (expect >= 100, predicted 239 / 345)", from, to, block)).toRawUTF8(),
                                      fmt ("L ratio %.1f, R ratio %.1f", bL.ratio(), bR.ratio()));
            }
    }
    {
        // Spectral liveness: 3 kHz tone (16 samples per cycle), az +90, orders 1 vs 3, R ear.
        constexpr int settle = 1024, win = 4800;
        double mag[4][2] = {};
        for (int order : { 1, 3 })
        {
            osa::Params prm;
            prm.azimuthDeg = 90.0f;
            prm.order = order;
            const auto r = renderEngine (prm, kFs, kDefaultBlock, settle + win, { kDefaultBlock },
                                         [] (int n) { return Sine { 3000.0, 0.5, kFs }(n); }, noEngineHook);
            mag[order][0] = binMagnitude (r.lr, 0, 2, settle, win, 300);
            mag[order][1] = binMagnitude (r.lr, 1, 2, settle, win, 300);
        }
        const double dR = dB (mag[3][1] / mag[1][1]), dL = dB (mag[3][0] / mag[1][0]);
        check ("DSP-07 spectral liveness: 3 kHz at az +90, R ear |dB (order 3 / order 1)| >= 3 (the banks differ where the click gate must be live)",
               std::abs (dR) >= 3.0, fmt ("R %+.2f dB (expect +6.2), L %+.2f dB (expect -2.2); R mags %.4f / %.4f", dR, dL, mag[1][1], mag[3][1]));
    }

    //==========================================================================
    // DSP-07 decoder Binaural → Stereo → Binaural click — same stimulus, hook
    // decoder = 1 at 24000 and 0 at 48000; snapped-mix negative control.
    {
        constexpr int total = kSkip23 + 72000;
        for (int block : { 64, 1024 })
        {
            auto p = makeProc (2, block, [] (Proc& q) { setParam (q, ParamIDs::azimuth, 90.0f); });
            const RefParams rp = refParamsFrom (*p);
            const auto decoderAt = [] (int n) { return n >= 48000 ? 0 : n >= 24000 ? 1 : 0; };
            const auto ev = renderHooked (*p, total, toneGen23, [&p, decoderAt] (int n)
            {
                setParam (*p, ParamIDs::decoder, (float) decoderAt (n));
            }, block);
            const auto refHook = [decoderAt] (int n, RefParams& r) { r.decoder = decoderAt (n); };

            ReferenceModel lerped, snapped;
            snapped.snapDecoderMix = true;
            const auto ctrl = renderRef (lerped,  rp, kFs, total, block, toneGen23, refHook);
            const auto bad  = renderRef (snapped, rp, kFs, total, block, toneGen23, refHook);

            ClickReading rL { worstCurvature (ev, 0, 2, kSkip23, total), worstCurvature (ctrl.lr, 0, 2, kSkip23, total) };
            ClickReading rR { worstCurvature (ev, 1, 2, kSkip23, total), worstCurvature (ctrl.lr, 1, 2, kSkip23, total) };
            const juce::String nm = fmt ("DSP-07 decoder B->S->B click at %d-sample blocks (100 Hz, az +90, order 1) vs the same-motion reference", block);
            clickVerdict ((nm + " [L]").toRawUTF8(), rL);
            clickVerdict ((nm + " [R]").toRawUTF8(), rR);

            ClickReading bL { worstCurvature (bad.lr, 0, 2, kSkip23, total), rL.control };
            ClickReading bR { worstCurvature (bad.lr, 1, 2, kSkip23, total), rR.control };
            checkFailsAsExpected (bL.ratio() <= kClickRatioCeiling && bR.ratio() <= kClickRatioCeiling,
                                  fmt ("DSP-07 decoder negative control at %d blocks: snapped decoder mix FAILS the gate", block).toRawUTF8(),
                                  fmt ("L ratio %.1f, R ratio %.1f", bL.ratio(), bR.ratio()));

            if (block == 64)
            {
                const double rBin = rms (ev, 1, 2, kSkip23, 24000), rSt = rms (ev, 1, 2, 24000 + kSkip23, 48000);
                check ("DSP-07 decoder endpoints differ: R ear rms |dB (Binaural / Stereo)| >= 6 at az +90 (the switch is a live event)",
                       std::abs (dB (rBin / rSt)) >= 6.0, fmt ("%+.2f dB (expect ~19)", dB (rBin / rSt)));
            }
        }
    }

    //==========================================================================
    // bank-fade queue — order 1 → 3 at 1024, 2 at +7 grids (mid-fade → queued),
    // 1 at +9 (latest wins), 2 at 4096 (idle → immediate); null to the reference
    // at 64 and 1024 blocks; liveness; float-ramp arrival EXACTLY at step 15.
    {
        constexpr int total = 8192;
        const auto queueHook = [] (int n, auto& p)
        {
            p.order = n >= 4096 ? 2 : n >= 1600 ? 1 : n >= 1472 ? 2 : n >= 1024 ? 3 : 1;
        };
        osa::Params prm;
        prm.azimuthDeg = 37.0f;
        prm.elevationDeg = -12.0f;
        prm.order = 1;

        std::vector<float> queue64;
        for (int block : { 64, 1024 })
        {
            juce::ScopedNoDenormals noDenormals;
            osa::SpatialEngine e;
            osa::Params p = prm;
            e.setParams (p);
            e.prepare (kFs, block);
            std::vector<float> mono ((size_t) block), L ((size_t) block), R ((size_t) block), lr;
            lr.reserve ((size_t) total * 2);
            for (int pos = 0; pos < total; pos += block)
            {
                queueHook (pos, p);
                e.setParams (p);
                for (int i = 0; i < block; ++i)
                    mono[(size_t) i] = monoNoiseAt (pos + i);
                e.process (mono.data(), L.data(), R.data(), block);
                for (int i = 0; i < block; ++i)
                {
                    lr.push_back (L[(size_t) i]);
                    lr.push_back (R[(size_t) i]);
                }
            }

            ReferenceModel m;
            const auto ref = renderRef (m, refParamsFrom (prm), kFs, total, block, monoNoiseAt, queueHook);
            double peak = 0.0;
            const double err = maxErrLr (lr, ref.lr, peak);
            check (fmt ("bank-fade queue at %d blocks: engine vs reference under the queued schedule max|err| < 1e-5; final activeSet == 2, not fading", block).toRawUTF8(),
                   err < 1.0e-5 && peak > 0.01 && e.convolver().activeSet() == 2 && ! e.convolver().isFading(),
                   fmt ("max|err| %.3e, peak |L| %.3f, final set %d fading %d (reference set %d fading %d)", err, peak,
                        e.convolver().activeSet(), (int) e.convolver().isFading(), m.activeSet(), (int) m.isFading()));
            if (block == 64)
                queue64 = lr;
        }

        // Liveness: the mid-fade window differs from the static order-1 and order-2 renders.
        {
            osa::Params p1 = prm, p2 = prm;
            p2.order = 2;
            const auto s1 = renderEngine (p1, kFs, 64, total, { 64 }, monoNoiseAt, noEngineHook);
            const auto s2 = renderEngine (p2, kFs, 64, total, { 64 }, monoNoiseAt, noEngineHook);
            double d1 = 0.0, d2 = 0.0;
            for (int n = 1100; n < 1900; ++n)
            {
                d1 = std::max (d1, std::abs ((double) queue64[(size_t) n * 2] - (double) s1.lr[(size_t) n * 2]));
                d2 = std::max (d2, std::abs ((double) queue64[(size_t) n * 2] - (double) s2.lr[(size_t) n * 2]));
            }
            check ("bank-fade queue liveness: the mid-fade window [1100, 1900) differs from both the order-1 and order-2 static renders by > 0.1",
                   d1 > 0.1 && d2 > 0.1, fmt ("|q - o1| %.3f, |q - o2| %.3f", d1, d2));
        }

        // Arrival exactness: order 1 → 3 at n = 1024 only; the float ramp reads exactly
        // 1.0f on the block starting at 1920 (step 15) and the arrival step at 1984
        // leaves activeSet 3, not fading, bankMix exactly 0.
        {
            juce::ScopedNoDenormals noDenormals;
            osa::SpatialEngine e;
            osa::Params p = prm;
            e.setParams (p);
            e.prepare (kFs, 64);
            std::vector<float> mono (64), L (64), R (64);
            float mixAt1920 = -1.0f, mixAt1984 = -1.0f, mixAt1856 = -1.0f;
            bool fadingAt1920 = false, fadingAt1984 = true;
            int setAt1920 = 0, setAt1984 = 0;
            for (int pos = 0; pos < 4096; pos += 64)
            {
                p.order = pos >= 1024 ? 3 : 1;
                e.setParams (p);
                for (int i = 0; i < 64; ++i)
                    mono[(size_t) i] = monoNoiseAt (pos + i);
                e.process (mono.data(), L.data(), R.data(), 64);
                if (pos == 1856) mixAt1856 = e.convolver().bankMixCur();
                if (pos == 1920) { mixAt1920 = e.convolver().bankMixCur(); fadingAt1920 = e.convolver().isFading(); setAt1920 = e.convolver().activeSet(); }
                if (pos == 1984) { mixAt1984 = e.convolver().bankMixCur(); fadingAt1984 = e.convolver().isFading(); setAt1984 = e.convolver().activeSet(); }
            }
            check ("bank-fade arrival exactness: request at 1024 -> bankMix == 1.0f exactly at step 15 (block 1920, still fading, set 1); block 1984: activeSet 3, not fading, bankMix == 0 (float ramp, F1)",
                   mixAt1920 == 1.0f && fadingAt1920 && setAt1920 == 1 && mixAt1984 == 0.0f && ! fadingAt1984 && setAt1984 == 3 && mixAt1856 < 1.0f,
                   fmt ("step 14 %.6f, step 15 %.6f (fading %d, set %d), after arrival %.6f (fading %d, set %d)",
                        (double) mixAt1856, (double) mixAt1920, (int) fadingAt1920, setAt1920, (double) mixAt1984, (int) fadingAt1984, setAt1984));
        }
    }

    //==========================================================================
    // P2e / P3e / P2f — slicing bit-equality in BINAURAL mode.
    const auto binauralMovingSetup = [&rotatedMovingSetup] (Proc& q)
    {
        rotatedMovingSetup (q);
        setParam (q, ParamIDs::decoder, 0.0f);
    };
    {
        auto p = makeProc (2, kDefaultBlock, binauralMovingSetup);
        std::vector<float> flatBin;
        flatBin.reserve ((size_t) kRenderSamples * 2);
        renderWith (*p, 2, { kDefaultBlock }, noiseAt, flatBin);

        auto pr = makeProc (2, kDefaultBlock, rotatedMovingSetup);
        std::vector<float> flatRot;
        flatRot.reserve ((size_t) kRenderSamples * 2);
        renderWith (*pr, 2, { kDefaultBlock }, noiseAt, flatRot);
        check ("P2e liveness: the Binaural moving render differs from the 2.2 Stereo moving render (flatRot)", ! buffersBitEqual (flatBin, flatRot));

        auto q = makeProc (2, 4096, binauralMovingSetup);
        std::vector<float> ragged;
        ragged.reserve ((size_t) kRenderSamples * 2);
        renderWith (*q, 2, { 1, 7, 64, 333, 4096, 13, 512, 2 }, noiseAt, ragged);
        check ("P2e ragged == flat in Binaural with orbit On 2 Hz / order 3 / psi 25 theta -40 phi 70", buffersBitEqual (ragged, flatBin));

        auto small = makeProc (2, 64, binauralMovingSetup);
        std::vector<float> outSmall;
        outSmall.reserve ((size_t) kRenderSamples * 2);
        renderWith (*small, 2, { 4096 }, noiseAt, outSmall);
        auto big = makeProc (2, 4096, binauralMovingSetup);
        std::vector<float> outBig;
        outBig.reserve ((size_t) kRenderSamples * 2);
        renderWith (*big, 2, { 4096 }, noiseAt, outBig);
        check ("P3e oversized (prep 64, feed 4096) == prep 4096 in Binaural with rotation + orbit On", buffersBitEqual (outSmall, outBig));
        check ("P3e oversized == flat in Binaural", buffersBitEqual (outBig, flatBin));
    }
    {
        constexpr int total = 32768;
        const auto gen = [] (int n) { return monoNoiseAt (n); };
        const auto binauralTupleSetup = [&nullTupleSetup] (Proc& q)
        {
            nullTupleSetup (q);
            setParam (q, ParamIDs::decoder, 0.0f);
        };
        const auto hookFor = [] (Proc& p)
        {
            return [&p] (int n)
            {
                if (n % 4096 == 0)
                {
                    const int k = n / 4096;
                    setParam (p, ParamIDs::order,   (float) (k % 3));
                    setParam (p, ParamIDs::decoder, (float) (k % 2));
                    setParam (p, ParamIDs::yaw,     osa::wrapTo180 (30.0f * (float) k));
                }
            };
        };
        auto p64 = makeProc (2, 64, binauralTupleSetup);
        const auto r64 = renderHooked (*p64, total, gen, hookFor (*p64), 64);
        auto p4096 = makeProc (2, 4096, binauralTupleSetup);
        const auto r4096 = renderHooked (*p4096, total, gen, hookFor (*p4096), 4096);
        auto p0 = makeProc (2, 64, binauralTupleSetup);
        const auto r0 = renderHooked (*p0, total, gen, [] (int) {}, 64);
        check ("P2f: order / decoder / yaw stepped at every 4096-sample boundary (bank fades + mixer fades under slicing) — 64-block render == 4096-block render (bit)",
               buffersBitEqual (r64, r4096));
        check ("P2f liveness: the hooked render differs from the hook-less render", ! buffersBitEqual (r64, r0));
    }

    //==========================================================================
    // FDL decays to exact 0 — 2048 samples of noise then silence, 48 k / 96 k.
    {
        for (double sr : { 48000.0, 96000.0 })
        {
            osa::Params prm;
            prm.order = 3;
            const int K = sr == 48000.0 ? 4 : 8;
            const int total = 2048 + 4096;
            const auto r = renderEngine (prm, sr, kDefaultBlock, total, { kDefaultBlock },
                                         [] (int n) { return n < 2048 ? monoNoiseAt (n) : 0.0f; }, noEngineHook);
            int lastNonZero = -1;
            for (int n = 0; n < total; ++n)
                if (r.lr[(size_t) n * 2] != 0.0f || r.lr[(size_t) n * 2 + 1] != 0.0f)
                    lastNonZero = n;
            const int bound = 2048 + 64 + K * 64;
            check (fmt ("FDL decays to exact 0 at fs %.0f (K %d): every sample from %d on is exactly 0.0f", sr, K, bound).toRawUTF8(),
                   lastNonZero < bound && lastNonZero > 2048, fmt ("last nonzero sample %d (bound %d)", lastNonZero, bound));
        }
    }

    //==========================================================================
    // "placeholder gone" (D13 / D2) — the T6 inversion above, plus the constant.
    check ("placeholder gone (D2): kStereoLevelMatch == 1.0f by design — the level match lives in the filter scale (kBinauralLevelMatchDb)",
           osa::kStereoLevelMatch == 1.0f,
           fmt ("kStereoLevelMatch %.7f, kBinauralLevelMatchDb %+.3f dB", (double) osa::kStereoLevelMatch, (double) osa::kBinauralLevelMatchDb));

    //==========================================================================
    // extremes (2.3) — the 2.2 corners forced BINAURAL, orders 1 / 3 alternating,
    // 48 k / 96 k; gimbal in Binaural at order 3; headroom printed.
    {
        for (double sr : { 48000.0, 96000.0 })
        {
            bool finite = true, live = true;
            int  badCase = -1;
            for (int i = 0; i < 8; ++i)
            {
                auto p = makeProcAt (sr, 2, kDefaultBlock, [i] (Proc& q)
                {
                    setParam (q, ParamIDs::yaw,       (i & 1) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::pitch,     (i & 2) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::roll,      (i & 4) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::azimuth,   (i & 1) ? 180.0f : -180.0f);
                    setParam (q, ParamIDs::elevation, (i & 2) ? 90.0f : -90.0f);
                    setParam (q, ParamIDs::distance,  (i & 4) ? 20.0f : 0.2f);
                    setParam (q, ParamIDs::decoder,   0.0f);
                    setParam (q, ParamIDs::order,     (i & 1) ? 2.0f : 0.0f);
                    setParam (q, ParamIDs::orbit,     1.0f);
                    setParam (q, ParamIDs::orbitRate, 2.0f);
                });
                std::vector<float> out;
                renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, 4096);
                const bool f = allFinite (out);
                bool nonZero = false;
                for (float v : out)
                    if (v != 0.0f) { nonZero = true; break; }
                finite = finite && f;
                live   = live && nonZero;
                if ((! f || ! nonZero) && badCase < 0)
                    badCase = i;
            }
            check ((juce::String ("extremes (2.3): 8 corners forced Binaural, orders 1 / 3 alternating, x 4096 samples at fs ") + juce::String ((int) sr)
                    + " finite and not silenced").toRawUTF8(), finite && live,
                   badCase < 0 ? juce::String ("all 8 ok") : juce::String ("first bad case ") + juce::String (badCase));
        }
        {
            auto p = makeProc (2, 64, [] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth,   37.0f);
                setParam (q, ParamIDs::elevation, -12.0f);
                setParam (q, ParamIDs::pitch,     90.0f);
                setParam (q, ParamIDs::order,     2.0f);
                setParam (q, ParamIDs::decoder,   0.0f);
            });
            const auto out = renderHooked (*p, 8192, [] (int n) { return monoNoiseAt (n); },
                                           [&p] (int n) { setParam (*p, ParamIDs::yaw, osa::wrapTo180 (0.48f * (float) (n / 64))); });
            bool nonZero = false;
            for (float v : out)
                if (v != 0.0f) { nonZero = true; break; }
            check ("extremes (2.3): gimbal theta = +90 under a yaw sweep in Binaural at order 3 — finite and live", allFinite (out) && nonZero);
        }
        {
            // Headroom (F7): the hottest legal setting — d 0.2 (g 2.78), az +90, order 3, Binaural, outputLevel 0.
            auto p = makeProc (2, kDefaultBlock, [] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth,  90.0f);
                setParam (q, ParamIDs::distance, 0.2f);
                setParam (q, ParamIDs::order,    2.0f);
                setParam (q, ParamIDs::decoder,  0.0f);
            });
            std::vector<float> out;
            renderWith (*p, 2, { kDefaultBlock }, noiseAt, out, 8192);
            float peak = 0.0f;
            for (float v : out)
                peak = std::max (peak, std::abs (v));
            check ("extremes (2.3): headroom corner (d 0.2, az +90, order 3, Binaural, full-scale noise) finite", allFinite (out),
                   fmt ("peak |out| %.3f (%+.1f dBFS) — printed, not asserted < 1 (F7: the bank peaks at %+.1f dB; outputLevel is the user's tool)",
                        (double) peak, dB ((double) peak), 6.5));
        }
    }

    //==========================================================================
    // PERF-01 — allocation counter across processBlock (A11); 2.2 adds the
    // three rotation angles moving inside the window; 2.3 runs it with the
    // convolver live (the order / decoder fades inside the window exercise the
    // dual-bank path).
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
                case 15:  setParam (*p, ParamIDs::yaw, 90.0f); break;
                case 20:  setParam (*p, ParamIDs::order, 2.0f); break;
                case 25:  setParam (*p, ParamIDs::pitch, -45.0f); break;
                case 30:  setParam (*p, ParamIDs::distance, 0.2f); break;
                case 35:  setParam (*p, ParamIDs::roll, 30.0f); break;
                case 40:  setParam (*p, ParamIDs::decoder, 1.0f); break;
                case 60:  setParam (*p, ParamIDs::order, 1.0f); break;
                case 70:  setParam (*p, ParamIDs::yaw, -180.0f); break;
                case 80:  setParam (*p, ParamIDs::distance, 20.0f); break;
                case 90:  setParam (*p, ParamIDs::pitch, 0.0f); setParam (*p, ParamIDs::roll, 0.0f); break;
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

        check ("PERF-01 zero allocations across 200 processBlock calls (order 1->3->2, decoder B->S->B, orbit On, d 0.2->20, yaw / pitch / roll moving, one 4096 block; convolver live, order fades 1->3->2 with the bank fades inside the window)",
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
    // Stage 3 (PLAN-3 T4) — ST1–ST6 state + lessons, PL1–PL4 meterFrame payload.
    // Fresh processors only, rtcheck disarmed, after PERF-01 and before the
    // digest prints: no Stage-2 render or verdict is touched (241 + 10 = 251).
    {
        rtcheck::disarm();
        namespace ui = OSimpleAmbisonics::ui;
        using st3::kAllIds;

        // ── ST1 uiLanguage round-trip ─────────────────────────────────────────
        {
            Proc a, b;
            a.uiLanguage.store (2);
            auto xml = st3::stateXml (a);
            st3::roundTrip (a, b);
            const auto attr = xml != nullptr ? xml->getStringAttribute ("uiLanguage") : juce::String();
            check ("ST1 uiLanguage 2 round-trips through get/setStateInformation; the XML attribute is \"zh-Hans\"",
                   b.uiLanguage.load() == 2 && attr == "zh-Hans",
                   fmt ("restored %d, attribute \"%s\"", b.uiLanguage.load(), attr.toRawUTF8()));
        }

        // ── ST2 absent / unknown → English ────────────────────────────────────
        {
            Proc src;
            auto xml = st3::stateXml (src);
            bool ok = xml != nullptr;
            int absentGot = -1, unknownGot = -1;
            if (ok)
            {
                xml->removeAttribute ("uiLanguage");
                Proc c;
                st3::restoreXml (c, *xml);
                absentGot = c.uiLanguage.load();

                xml->setAttribute ("uiLanguage", "de");
                Proc d;
                st3::restoreXml (d, *xml);
                unknownGot = d.uiLanguage.load();
                ok = absentGot == 0 && unknownGot == 0;
            }
            check ("ST2 uiLanguage absent -> 0 (English), \"de\" -> 0", ok,
                   fmt ("absent %d, \"de\" %d", absentGot, unknownGot));
        }

        // ── ST3 editorScale round-trip; a fresh instance writes none ──────────
        {
            Proc a, b, fresh;
            a.editorScale.store (0.85f);
            st3::roundTrip (a, b);
            auto freshXml = st3::stateXml (fresh);
            const bool noAttr = freshXml != nullptr && ! freshXml->hasAttribute ("editorScale");
            const auto lang   = freshXml != nullptr ? freshXml->getStringAttribute ("uiLanguage") : juce::String();
            check ("ST3 editorScale 0.85 round-trips (within 1e-3); a fresh instance writes no editorScale and uiLanguage=\"en\"",
                   std::abs (b.editorScale.load() - 0.85f) < 1.0e-3f && noAttr && lang == "en",
                   fmt ("restored %.4f, fresh has editorScale: %s, uiLanguage \"%s\"",
                        (double) b.editorScale.load(), noAttr ? "no" : "YES", lang.toRawUTF8()));
        }

        // ── ST4 editorScale rejected and removed from the re-saved copy ───────
        {
            Proc src;
            auto xml = st3::stateXml (src);
            bool ok = xml != nullptr;
            juce::String detail;
            for (const char* bad : { "abc", "3.0", "0.1", "1.6", "nan", "inf" })
            {
                if (xml == nullptr) break;
                xml->setAttribute ("editorScale", bad);
                Proc b;
                st3::restoreXml (b, *xml);
                auto resaved = st3::stateXml (b);
                const bool rejected = juce::exactlyEqual (b.editorScale.load(), 0.0f);
                const bool removed  = resaved != nullptr && ! resaved->hasAttribute ("editorScale");
                ok = ok && rejected && removed;
                detail << "\"" << bad << "\"->" << juce::String (b.editorScale.load(), 3)
                       << (removed ? " (absent) " : " (RE-SAVED) ");
            }
            check ("ST4 editorScale \"abc\" / \"3.0\" / \"0.1\" / \"1.6\" / \"nan\" / \"inf\" rejected (stays 0) and absent from the re-saved XML",
                   ok, detail.trim());
        }

        // ── ST5 12 values bit-equal through the round-trip, two passes ────────
        {
            bool ok = true;
            int mismatches = 0;
            juce::String detail;
            for (int pass = 0; pass < 2; ++pass)
            {
                Proc a, b;
                if (pass == 1)
                {
                    setParam (a, ParamIDs::azimuth, -70.0f);   setParam (a, ParamIDs::elevation, 20.0f);
                    setParam (a, ParamIDs::distance, 5.0f);    setParam (a, ParamIDs::order, 2.0f);
                    setParam (a, ParamIDs::yaw, 30.0f);        setParam (a, ParamIDs::pitch, 10.0f);
                    setParam (a, ParamIDs::roll, -15.0f);      setParam (a, ParamIDs::decoder, 1.0f);
                    setParam (a, ParamIDs::orbit, 1.0f);       setParam (a, ParamIDs::orbitRate, 1.0f);
                    setParam (a, ParamIDs::inputMode, 1.0f);   setParam (a, ParamIDs::outputLevel, -12.0f);
                    a.uiLanguage.store (1);
                    a.editorScale.store (1.25f);
                }
                st3::roundTrip (a, b);
                for (const char* id : kAllIds)
                {
                    const float va = a.apvts.getParameter (id)->getValue(), vb = b.apvts.getParameter (id)->getValue();
                    const float ra = rawParam (a, id), rb = rawParam (b, id);
                    if (std::memcmp (&va, &vb, sizeof (float)) != 0 || std::memcmp (&ra, &rb, sizeof (float)) != 0)
                    {
                        ++mismatches;
                        detail << id << " " << juce::String (ra, 9) << "!=" << juce::String (rb, 9) << " ";
                    }
                }
                const bool props = pass == 0 ? (b.uiLanguage.load() == 0 && juce::exactlyEqual (b.editorScale.load(), 0.0f))
                                             : (b.uiLanguage.load() == 1 && std::abs (b.editorScale.load() - 1.25f) < 1.0e-3f);
                ok = ok && props;
                if (! props) detail << "pass " << pass << " properties not restored ";
            }
            ok = ok && mismatches == 0;
            check ("ST5 all 12 getValue() and raw atomics bit-equal through the round-trip (defaults incl. distance raw, and an off-default set with uiLanguage 1 / editorScale 1.25); both properties restored",
                   ok, detail.isEmpty() ? fmt ("0 mismatches over 2 x 12; distance default raw %.15g", (double) rawParam (*std::make_unique<Proc>(), ParamIDs::distance)) : detail.trim());
        }

        // ── ST6 lessons (UI-08) ───────────────────────────────────────────────
        {
            struct Recipe { const char* id; std::vector<std::pair<const char*, float>> sets; };
            const std::vector<Recipe> recipes = {
                { "pointSource",        { { ParamIDs::order, 0.0f }, { ParamIDs::azimuth, 30.0f }, { ParamIDs::elevation, 0.0f }, { ParamIDs::distance, 1.0f } } },
                { "orderStep",          { { ParamIDs::order, 2.0f }, { ParamIDs::azimuth, 45.0f } } },
                { "behindYou",          { { ParamIDs::azimuth, 180.0f } } },
                { "overhead",           { { ParamIDs::elevation, 90.0f }, { ParamIDs::order, 1.0f } } },
                { "rotateRoom",         { { ParamIDs::azimuth, 60.0f }, { ParamIDs::yaw, 120.0f }, { ParamIDs::order, 1.0f } } },
                { "farNear",            { { ParamIDs::distance, 12.0f } } },
                { "slowOrbit",          { { ParamIDs::orbit, 1.0f }, { ParamIDs::orbitRate, 0.05f }, { ParamIDs::order, 1.0f } } },
                { "speakersHeadphones", { { ParamIDs::decoder, 1.0f }, { ParamIDs::order, 2.0f }, { ParamIDs::azimuth, 45.0f } } },
            };

            const auto offDefault = [] (Proc& q)
            {
                setParam (q, ParamIDs::azimuth, -70.0f);   setParam (q, ParamIDs::elevation, 20.0f);
                setParam (q, ParamIDs::distance, 5.0f);    setParam (q, ParamIDs::order, 2.0f);
                setParam (q, ParamIDs::yaw, 30.0f);        setParam (q, ParamIDs::pitch, 10.0f);
                setParam (q, ParamIDs::roll, -15.0f);      setParam (q, ParamIDs::decoder, 1.0f);
                setParam (q, ParamIDs::orbit, 1.0f);       setParam (q, ParamIDs::orbitRate, 1.0f);
                setParam (q, ParamIDs::inputMode, 1.0f);   setParam (q, ParamIDs::outputLevel, -12.0f);
            };

            bool ok = true;
            juce::String detail;
            int totalChanged = 0;
            float outRaw = 0.0f;
            for (const auto& r : recipes)
            {
                Proc q;
                offDefault (q);
                const float outBefore = rawParam (q, ParamIDs::outputLevel);
                std::vector<float> before;
                for (const char* id : kAllIds) before.push_back (q.apvts.getParameter (id)->getValue());

                st3::GestureCounter gc;
                for (auto* prm : q.getParameters()) prm->addListener (&gc);

                const bool applied = q.applyFactoryPreset (r.id);

                int changed = 0, wrong = 0;
                for (size_t i = 0; i < 12; ++i)
                {
                    auto* prm = q.apvts.getParameter (kAllIds[i]);
                    const float now = prm->getValue();
                    if (std::memcmp (&now, &before[i], sizeof (float)) != 0) ++changed;
                    if (std::strcmp (kAllIds[i], ParamIDs::outputLevel) == 0) continue;
                    float target = prm->getDefaultValue();
                    for (const auto& s : r.sets)
                        if (std::strcmp (s.first, kAllIds[i]) == 0)
                            target = prm->convertTo0to1 (s.second);
                    target = prm->convertTo0to1 (prm->convertFrom0to1 (target));
                    if (std::abs (now - target) > 1.0e-6f)
                    {
                        ++wrong;
                        detail << r.id << ":" << kAllIds[i] << " ";
                    }
                }
                outRaw = rawParam (q, ParamIDs::outputLevel);
                // The host path snaps -12 dB to raw -11.999999 (range -60..0 step 0.1:
                // -60 + 0.1f * 480 in float), so "untouched" is BIT-EQUAL to the value
                // the host set, not the literal -12.0f (PLAN-3 ST6 deviation, SUMMARY).
                const bool outUntouched = std::memcmp (&outRaw, &outBefore, sizeof (float)) == 0
                                       && std::abs (outRaw + 12.0f) < 1.0e-5f;
                const bool gesturesOk   = gc.begins == changed && gc.ends == changed;

                gc.begins = gc.ends = 0;
                const bool again   = q.applyFactoryPreset (r.id);
                const bool idemOk  = again && gc.begins == 0 && gc.ends == 0;

                for (auto* prm : q.getParameters()) prm->removeListener (&gc);

                totalChanged += changed;
                if (! (applied && wrong == 0 && outUntouched && gesturesOk && idemOk))
                {
                    ok = false;
                    detail << r.id << (applied ? "" : " not-applied") << (outUntouched ? "" : fmt (" OUTPUT-MOVED(%.9g->%.9g)", (double) outBefore, (double) outRaw))
                           << (gesturesOk ? "" : " gestures") << (idemOk ? "" : " not-idempotent") << " ";
                }
            }

            {   // unknown ids: false, nothing changed, no gesture
                Proc q;
                offDefault (q);
                std::vector<float> before;
                for (const char* id : kAllIds) before.push_back (q.apvts.getParameter (id)->getValue());
                st3::GestureCounter gc;
                for (auto* prm : q.getParameters()) prm->addListener (&gc);
                const bool a = q.applyFactoryPreset ("nope");
                const bool b = q.applyFactoryPreset ("");
                int changed = 0;
                for (size_t i = 0; i < 12; ++i)
                {
                    const float now = q.apvts.getParameter (kAllIds[i])->getValue();
                    if (std::memcmp (&now, &before[i], sizeof (float)) != 0) ++changed;
                }
                for (auto* prm : q.getParameters()) prm->removeListener (&gc);
                if (a || b || changed != 0 || gc.begins != 0 || gc.ends != 0)
                {
                    ok = false;
                    detail << "unknown-id: " << (int) a << (int) b << " changed " << changed << " gestures " << gc.begins << " ";
                }
            }

            check ("ST6 lessons: 8 ids apply exactly recipe-or-default from an off-default state, outputLevel raw bit-unchanged at the host-set -12 dB, begins == ends == changed, re-apply = 0 gestures; \"nope\" / \"\" -> false, no change, 0 gestures",
                   ok, detail.isEmpty() ? fmt ("%d parameter changes over 8 lessons, outputLevel raw %.9g (bit-unchanged)", totalChanged, (double) outRaw) : detail.trim());
        }

        // ── PL1 payload format ────────────────────────────────────────────────
        {
            osa::MeterFrame f;
            for (int i = 0; i < 16; ++i) { f.shGain[i] = 0.1f * (float) (i - 7); f.shPeak[i] = 0.05f * (float) i; }
            f.shGain[0] = -4.4e-8f;
            f.azEffDeg = -179.996f; f.elEffDeg = 12.345f; f.azRotDeg = 33.3f; f.elRotDeg = -89.999f;
            f.decoderMix = 0.25f; f.outPeakL = 0.301f; f.outPeakR = 1.25f;

            const auto q1 = ui::quantise (f, true), q2 = ui::quantise (f, true);
            const auto t1 = st3::payloadOf (q1), t2 = st3::payloadOf (q2);
            auto f3 = f; f3.shPeak[5] += 0.002f;
            const auto q3 = ui::quantise (f3, true);
            const bool sameBytes = t1 == t2 && ui::hash (q1) == ui::hash (q2);
            const bool peakMoves = ui::hash (q3) != ui::hash (q1);
            const bool zeroOk    = t1.rfind ("{\"shGain\":[0.000,", 0) == 0 && t1.find ("-0.000") == std::string::npos;

            const auto parsed = juce::JSON::parse (juce::String (t1));
            bool shapeOk = false;
            if (auto* obj = parsed.getDynamicObject())
            {
                const auto& props = obj->getProperties();
                const auto* g = obj->getProperty ("shGain").getArray();
                const auto* pk = obj->getProperty ("shPeak").getArray();
                shapeOk = props.size() == 12 && g != nullptr && pk != nullptr && g->size() == 16 && pk->size() == 16;
            }

            osa::MeterFrame bad;
            const float inf = std::numeric_limits<float>::infinity();
            for (int i = 0; i < 16; ++i) { bad.shGain[i] = (i % 3 == 0) ? std::nanf ("") : (i % 3 == 1 ? inf : -1.0e9f); bad.shPeak[i] = (i % 2) ? -inf : 1.0e9f; }
            bad.azEffDeg = std::nanf (""); bad.elEffDeg = inf; bad.azRotDeg = -inf; bad.elRotDeg = 1.0e9f;
            bad.decoderMix = -1.0e9f; bad.outPeakL = inf; bad.outPeakR = std::nanf ("");
            bad.orderActive = 2000000000; bad.latencySamples = -2000000000;
            char big[1024];
            const int n = ui::format (ui::quantise (bad, true), big, (int) sizeof (big));
            const auto badParsed = juce::JSON::parse (juce::String (big, (size_t) juce::jmax (0, n)));
            bool badOk = n > 0 && n < 1024 && badParsed.getDynamicObject() != nullptr;
            if (badOk)
                for (auto& nv : badParsed.getDynamicObject()->getProperties())
                {
                    if (auto* arr = nv.value.getArray())
                        { for (auto& e : *arr) badOk = badOk && std::isfinite ((double) e); }
                    else
                        badOk = badOk && std::isfinite ((double) nv.value);
                }

            check ("PL1 payload: same frame -> identical bytes + hash; a peak change -> new hash; q3(-4.4e-8) prints 0.000 (no -0.000); JSON.parse has 12 keys + two 16-arrays; NaN / +-inf / 1e9 fields format finite within 1024 bytes",
                   sameBytes && peakMoves && zeroOk && shapeOk && badOk,
                   fmt ("%d bytes typical, %d bytes worst case; same %d, peak-moves %d, zero %d, shape %d, fuzz %d",
                        (int) t1.size(), n, (int) sameBytes, (int) peakMoves, (int) zeroOk, (int) shapeOk, (int) badOk));
        }

        // ── fixtures (built twice for PL4) ────────────────────────────────────
        const auto fixtures  = st3::buildFixtures();
        const auto fixtures2 = st3::buildFixtures();

        // ── PL2 UI-01 / UI-02 facts from the real processor ───────────────────
        {
            const auto get = [&] (const char* n) -> const ui::QuantisedFrame&
            {
                static const ui::QuantisedFrame none {};
                const auto* f = st3::find (fixtures, n);
                return f != nullptr ? f->frame : none;
            };
            const auto& fr = get ("front_o1");
            const auto& le = get ("left_o1");
            const auto& be = get ("behind_o1");
            const auto& ov = get ("overhead_o2");
            const auto& o1 = get ("o1_az45");
            const auto& o2 = get ("o2_az45");
            const auto& o3 = get ("o3_az45");
            const auto& yw = get ("yaw90_front");
            const auto& pa = get ("pole_a");
            const auto& pb = get ("pole_b");
            const auto& so = get ("stereo_o3");

            juce::String bad;
            const auto need = [&] (bool c, const char* what) { if (! c) bad << what << " "; };
            need (fr.gain[0] == 1000 && fr.gain[3] == 1000 && fr.gain[1] == 0, "front");
            need (le.gain[0] == 1000 && le.gain[1] == 1000 && le.gain[3] == 0, "left");
            need (be.gain[3] == -1000, "behind");
            need (ov.gain[2] == 1000 && ov.gain[1] == 0 && ov.gain[3] == 0, "overhead");
            need (o1.orderActive == 4 && o2.orderActive == 9 && o3.orderActive == 16, "orderActive");
            bool o1High = true;
            for (int i = 4; i < 16; ++i) o1High = o1High && o1.gain[i] == 0;
            need (o1High, "o1-bands");
            need (yw.azEff == 0 && std::abs (yw.azRot - 9000) <= 1, "yaw90");
            int dAz = std::abs (pa.azRot - pb.azRot);
            if (dAz > 18000) dAz = 36000 - dAz;
            need (std::abs (pa.elRot) >= 8999 && std::abs (pb.elRot) >= 8999 && dAz >= 6000, "pole");
            need (so.mix == 0, "stereo-mix");

            check ("PL2 UI-01 / UI-02 facts: front W=X=1 Y=0; left W=Y=1 X=0; behind X=-1; overhead Z=1; orderActive 4/9/16 with o1 bands 2-3 zero; yaw90 azEff 0 azRot 90; pole pair |elRot|>=89.99 with |dAzRot|>=60; stereo mix 0",
                   bad.isEmpty(),
                   bad.isEmpty() ? fmt ("left Y %d X %d; yaw90 azRot %d; pole elRot %d / %d azRot %d / %d; stereo mix %d",
                                        le.gain[1], le.gain[3], yw.azRot, pa.elRot, pb.elRot, pa.azRot, pb.azRot, so.mix)
                                 : "FAILED: " + bad.trim());
        }

        // ── PL3 U1 live rule (both idle states produced by C++) ───────────────
        {
            juce::String bad;
            const auto need = [&] (bool c, const char* what) { if (! c) bad << what << " "; };

            if (const auto* f = st3::find (fixtures, "idle_before_first_block"))
            {
                bool allZero = true;
                for (int i = 0; i < 16; ++i) allZero = allZero && f->frame.peak[i] == 0 && f->frame.gain[i] == 0;
                need (! f->frame.live && allZero && f->frame.orderActive == 4, "before-first-block");
            }
            else need (false, "before-first-block-missing");

            {
                auto p = st3::fixtureProc ({ { ParamIDs::azimuth, 90.0f } });
                ui::LiveTracker t;
                st3::play (*p, st3::kFixtureSamples, false);
                ui::QuantisedFrame tk[6];
                tk[1] = ui::sampleFrame (p->meterFrames, t);
                tk[2] = ui::sampleFrame (p->meterFrames, t);
                tk[3] = ui::sampleFrame (p->meterFrames, t);
                tk[4] = ui::sampleFrame (p->meterFrames, t);
                tk[5] = ui::sampleFrame (p->meterFrames, t);
                bool peaksZero = tk[4].outL == 0 && tk[4].outR == 0, gainsSame = true, t1Peaks = false;
                for (int i = 0; i < 16; ++i)
                {
                    peaksZero = peaksZero && tk[4].peak[i] == 0;
                    gainsSame = gainsSame && tk[4].gain[i] == tk[1].gain[i];
                    t1Peaks   = t1Peaks || tk[1].peak[i] != 0;
                }
                need (tk[1].live && tk[2].live && tk[3].live && ! tk[4].live, "live-sequence");
                need (t1Peaks, "tick1-has-peaks");
                need (peaksZero && gainsSame, "tick4-zero-peaks-same-gains");
                need (ui::hash (tk[4]) != ui::hash (tk[3]), "one-emission");
                need (ui::hash (tk[5]) == ui::hash (tk[4]), "then-silence");

                juce::MidiBuffer midi;
                juce::AudioBuffer<float> buf (2, st3::kFixtureBlock);
                buf.clear();
                p->processBlock (buf, midi);
                need (ui::sampleFrame (p->meterFrames, t).live, "publish-revives");
            }

            if (const auto* f = st3::find (fixtures, "silent_after_play"))
            {
                bool z = f->frame.outL == 0 && f->frame.outR == 0;
                for (int i = 0; i < 16; ++i) z = z && f->frame.peak[i] == 0;
                need (f->frame.live && z, "silent-after-play");
            }
            else need (false, "silent-missing");

            check ("PL3 U1 live rule: before the first block live 0 / peaks 0 / shGain 0 / orderActive 4; stale ticks 1-3 live, tick 4 live 0 with zero peaks and tick-1 gains, one emission then silence, a publish revives; silent_after_play live 1 with peaks 0",
                   bad.isEmpty(), bad.isEmpty() ? juce::String ("idle edge = 1 emission, kStaleTicksIdle ") + juce::String (ui::kStaleTicksIdle)
                                                : "FAILED: " + bad.trim());
        }

        // ── PL4 fixture determinism (+ opt-in write) ──────────────────────────
        {
            const auto j1 = st3::fixturesJson (fixtures);
            const auto j2 = st3::fixturesJson (fixtures2);
            check ("PL4 frames.json built twice from independent processor sets is byte-identical",
                   j1 == j2 && fixtures.size() == 15,
                   fmt ("%d fixtures, %d bytes", (int) fixtures.size(), (int) j1.size()));

            if (const char* dir = std::getenv ("OSAMB_UI_FIXTURES"); dir != nullptr && *dir != '\0')
            {
                const juce::File d = juce::File::getCurrentWorkingDirectory().getChildFile (dir);
                d.createDirectory();
                const auto out = d.getChildFile ("frames.json");
                const bool wrote = out.replaceWithText (juce::String::fromUTF8 (j1.data(), (int) j1.size()), false, false, "\n");
                std::printf ("  info: OSAMB_UI_FIXTURES -> %s (%s)\n", out.getFullPathName().toRawUTF8(), wrote ? "written" : "WRITE FAILED");
            }
        }
    }

    //==========================================================================
    // Digests — 2.1 baseline over the P1-null render (a VERDICT from 2.2, D3)
    // and the 2.2 rotated P1-null render (a VERDICT from 2.3, D13) — both
    // asserted above; 2.3's Binaural null render (informational, D6).
    std::printf ("  digest: fnv1a64=%016llx (P1-null render: %d samples x 2ch, flat %d, az 37 el -12 d 3.3 order 3 Stereo)\n",
                 (unsigned long long) fnv1a64 (nullRender), kRenderSamples, kDefaultBlock);
    std::printf ("  digest-2.2: fnv1a64=%016llx (rotated P1-null render: the same + psi 25 theta -40 phi 70; a verdict from 2.3)\n",
                 (unsigned long long) fnv1a64 (rotRender));
    std::printf ("  digest-2.3: fnv1a64=%016llx (Binaural null render: the rotated tuple, order 3, block %d, %d samples; informational — vDSP vs Fallback FFT, D6)\n",
                 (unsigned long long) fnv1a64 (binauralNullRender), kDefaultBlock, kNullTotal);

    //==========================================================================
    // PERF-02 (C9 / R9) — throughput, LAST: steady_clock around exactly 480 000
    // samples at 64-sample blocks through the processor (order 3 / Binaural /
    // the digest tuple + yaw 25, white noise), 1 s warm-up excluded, best of
    // three; asserted ≥ 20× only under NDEBUG; 1024-block and 96 k printed;
    // never ANDed with anything; rtcheck disarmed.
    {
        rtcheck::disarm();
        const auto perfSetup = [] (Proc& q)
        {
            setParam (q, ParamIDs::azimuth,   37.0f);
            setParam (q, ParamIDs::elevation, -12.0f);
            setParam (q, ParamIDs::distance,  3.3f);
            setParam (q, ParamIDs::order,     2.0f);
            setParam (q, ParamIDs::yaw,       25.0f);
            setParam (q, ParamIDs::decoder,   0.0f);
        };
        constexpr int kPerfSamples = 480000;
        std::vector<float> tmpl ((size_t) 4096 * 2);
        for (int n = 0; n < 4096; ++n)
            for (int ch = 0; ch < 2; ++ch)
                tmpl[(size_t) (n * 2 + ch)] = noiseAt (ch, n);

        const auto measure = [&] (double sr, int block)
        {
            auto p = makeProcAt (sr, 2, block, perfSetup);
            juce::MidiBuffer midi;
            juce::AudioBuffer<float> buf (2, block);
            int n0 = 0;
            const auto feed = [&]
            {
                for (int i = 0; i < block; ++i)
                {
                    const int m = (n0 + i) % 4096;
                    buf.setSample (0, i, tmpl[(size_t) (m * 2)]);
                    buf.setSample (1, i, tmpl[(size_t) (m * 2 + 1)]);
                }
                p->processBlock (buf, midi);
                n0 += block;
            };
            for (int n = 0; n < (int) sr; n += block)      // 1 s warm-up, not timed
                feed();
            double best = 1.0e300;
            for (int rep = 0; rep < 3; ++rep)
            {
                const auto t0 = std::chrono::steady_clock::now();
                for (int n = 0; n < kPerfSamples; n += block)
                    feed();
                best = std::min (best, std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
            }
            livenessSink.store (p.get());                  // the renders are not elided
            livenessSink.store (nullptr);
            return ((double) kPerfSamples / sr) / best;    // audio seconds per wall second
        };

        const double r64   = measure (kFs, 64);
        const double r1024 = measure (kFs, 1024);
        const double r96   = measure (96000.0, 64);
        const juce::String detail = fmt ("%.0fx real time at 48 k / 64 blocks (best of 3 over %d samples; expect >= 150); 1024 blocks %.0fx; 96 k / 64 blocks (K 8) %.0fx",
                                         r64, kPerfSamples, r1024, r96);
#ifdef NDEBUG
        check ("PERF-02 >= 20x real time (Release, dev machine, order 3 Binaural, 64-sample blocks, best of three)", r64 >= 20.0, detail);
#else
        std::printf ("  info: PERF-02 debug build — printed, not asserted: %s\n", detail.toRawUTF8());
#endif
    }

    std::printf ("%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
