/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
#pragma once

#include <cmath>
#include <cstdint>
#include "BandlimitedReader.h"   // grain-engine 1.1.0 (MipRing, SincBank, latchLevel)

// Test hooks are compiled ONLY into the render-harness target (CMake defines
// OGI_TEST_HOOKS=1 there). The plugin builds with every hook compiled out.
#ifndef OGI_TEST_HOOKS
 #define OGI_TEST_HOOKS 0
#endif

// ============================================================================
// O-Granulator engine constants (Stage 2, Phases 2.1–2.6).
// ARCHITECTURE §2, §6, §16; RESEARCH-2.1 §1; PLAN-2.1 T1.
// ============================================================================
namespace ogi
{
    // ── Pass engine ─────────────────────────────────────────────────────────
    // The host block is cut at every ABSOLUTE multiple of kCell. Each piece
    // writes its input into the ring before any grain reads (write-before-
    // read). The feedback return is delayed kFbDelay samples, so every
    // feedback sample a piece writes was produced by an EARLIER piece.
    inline constexpr int kCell    = 32;
    inline constexpr int kFbDelay = 32;
    static_assert (kCell <= kFbDelay, "a pass may not be longer than the feedback return delay "
                                      "(write-before-read with feedback, ARCHITECTURE Algorithm §1)");

    inline constexpr int kMaxStreams = 4;     // 2.2: all four streams live
    // O-GranulatorInstrument (FORK-MANIFEST: ADAPTED): grain_limit max 256, 32 spares.
    inline constexpr int kPoolLive   = 256;   // GrainPool live slots (grain_limit max)
    inline constexpr int kPoolFade   = 32;    // fade-on-steal spares (bursts can steal > 8 in 5 ms)

    // ── Pitch / size limits ─────────────────────────────────────────────────
    // O-GranulatorInstrument (ADAPTED): final grain pitch clamp ±48 st (S6).
    inline constexpr double kMaxSemis  = 48.0;
    inline constexpr double kMaxRate   = 16.0;    // 2^(kMaxSemis / 12)
    inline constexpr double kMinSizeMs = 5.0;
    inline constexpr double kMaxSizeMs = 2000.0;

    // Rate <= 16 → latchLevel <= 4 → residual r' in [1, 2] → sinc sets S0..S6.
    inline constexpr int kMaxLevel = 4;
    using Ring = ouaricon::grain::MipRing<kMaxLevel>;

    // ── Ring capacity derivation ────────────────────────────────────────────
    //   kMaxSpanS  : buffer_length max (30 s) — how far back a grain may START.
    //   kMaxReachS : the most source a grain consumes beyond its start while the
    //                head keeps advancing: a reverse grain at rate 4 for 2 s
    //                reads (4 + 1) × 2 s = 10 s.
    //   kGuardS    : slack (margins, the ≤ 31-sample render-head lead, reflect).
    // D2: the full 41 s at every sample rate (27.6 MB @ 48 k, 110 MB @ 192 k).
    // O-GranulatorInstrument (ADAPTED): kMaxRate 16 needs (16 + 1) × 2 s = 34 s of
    // reach for the static_assert below to hold. The instrument reads positional
    // MipTapes and never allocates this ring; the values only keep the parent's
    // invariant true at the adapted rate.
    inline constexpr double kMaxSpanS    = 30.0;
    inline constexpr double kMaxReachS   = 34.0;
    inline constexpr double kGuardS      = 1.0;
    inline constexpr double kRingSeconds = 65.0;
    static_assert ((kMaxRate + 1.0) * (kMaxSizeMs / 1000.0) <= kMaxReachS,
                   "max grain reach must cover a reverse grain at max rate and max size");
    static_assert (kMaxSpanS + kMaxReachS + kGuardS <= kRingSeconds,
                   "ring must hold max span + max reach + guard (memory: ring invariant needs a static_assert)");

    /** Level-0 ring length passed to ring.prepare(), and the bound of the reverse
        reach clamp (RESEARCH §1: reads stay valid to at least this + 61). */
    inline int kRingSamples (double fs) noexcept
    {
        return static_cast<int> (std::ceil (kRingSeconds * fs));
    }

    // ── Ramps / fades ───────────────────────────────────────────────────────
    inline constexpr double kBaseRampMs = 20.0;   // control-grid base ramp (rounded UP to whole cells)
    inline constexpr double kKillFadeMs = 5.0;    // R3 / D1 forced release (Grain::killStep)
    inline constexpr double kPoolFadeMs = 5.0;    // GrainPool fade-on-steal
    inline constexpr double kMixerRampMs = 10.0;  // P12: stream level / pan / mute-solo audibility

    // ── Phase 2.2 (PLAN-2.2 layout table; RESEARCH-2.2 R5, R6, R9, D5) ─────
    // R5 partial-coherence break-points. NOT retuned against a single source
    // (RESEARCH-2.2 §1.4): the transition band is documented, not fitted.
    inline constexpr double kCoherW0Ms = 5.0;     // spray width W₀ (ms)
    inline constexpr double kCoherP0St = 0.5;     // |base pitch| p₀ (st)
    inline constexpr double kCoherR0St = 0.5;     // pitch_rand r₀ (st, scale Off)
    inline constexpr double kGainRandDb = 24.0;   // D5: gain_rand 100 % = −24 dB, cut only

    inline constexpr int kWindowSteps  = 65;      // WindowMorphBank: step = round (window · 64)
    inline constexpr int kWindowPoints = 2048;    // points per table (read at phase · 2047)
    inline constexpr int kMaxActive    = 512;     // ScaleQuantiser active-degree bound (GS parity)

    // ── Phase 2.3 per-grain FX (PLAN-2.3 layout table; RESEARCH-2.3 R14–R18,
    //    CONTEXT-2.3 D6–D8). Latched at spawn; see GrainFx.h. ──────────────
    inline constexpr double kFxPi      = 3.14159265358979323846;
    inline constexpr double kFxNyquist = 0.45;    // cutoff AND ring carrier ceiling (× fs), never a fixed 20 kHz
    inline constexpr double kFxMinHz   = 20.0;    // SVF cutoff floor
    inline constexpr double kFxRandOct = 4.0;     // 2^(rand · 4 · (u − ½)) = ±2 oct at rand 1
    inline constexpr double kSvfKMax   = 2.0;     // k = 2 − 1.9 · res
    inline constexpr double kSvfKSpan  = 1.9;
    inline constexpr float  kDriveRef   = 0.25f;  // D6 r: unity at −12 dBFS
    inline constexpr float  kDriveGain  = 16.0f;  // D6 d = 16 · drive²
    inline constexpr float  kDriveGuard = 1.0e-4f; // R16: drive inactive below d = 1e-4
    inline constexpr double kCrushHoldHz    = 1000.0;  // hold rate fs · (1000/fs)^crush
    inline constexpr double kCrushScaleTop  = 15.0;    // scale = 2^(15 − 14 · crush) = 2^(b − 1)
    inline constexpr double kCrushScaleSpan = 14.0;

    /** Grain::fxActive bits (one per stage, latched at spawn). */
    inline constexpr std::uint32_t kFxSvf   = 1u;
    inline constexpr std::uint32_t kFxDrive = 2u;
    inline constexpr std::uint32_t kFxCrush = 4u;
    inline constexpr std::uint32_t kFxRing  = 8u;

    /** Grain::svfMode == sN_filter_type choice index. */
    inline constexpr int kSvfOff = 0, kSvfLP = 1, kSvfBP = 2, kSvfHP = 3;

    // ── Phase 2.4 (PLAN-2.4 layout table; RESEARCH-2.4 R29–R36, CONTEXT-2.4
    //    D9–D12). Feedback chain (FeedbackChain.h), wet knee, freeze, reverb. ─
    inline constexpr float  kFbKnob    = 0.95f;          // g_knob = 0.95 · feedback (ARCH §12)
    inline constexpr float  kFbK       = 1.41421366f;    // R31: the next float ABOVE √2 (Butterworth, no peak)
    static_assert (static_cast<double> (kFbK) * static_cast<double> (kFbK) > 2.0,
                   "kFbK must be >= sqrt 2 (float (sqrt 2) sits 1.7e-8 below it)");
    inline constexpr double kFbNyquist = 0.45;           // fb LP ceiling (× fs)
    inline constexpr double kDiffuserMs[4] { 4.7, 8.3, 13.9, 21.7 };   // THE source of the lengths (R32)
    inline constexpr float  kDiffuserG = 0.7f;           // fixed allpass coefficient (diffusion is a MIX)
    inline constexpr int    kDiffuserMaxLen = 4167;      // 21.7 ms × 192 kHz, rounded up
    inline constexpr float  kSatSpan   = 7.0f;           // D12 d = 1 + 7·sat (listening fallback: 3)
    inline constexpr double kAgcAttackS  = 0.001;        // loop AGC peak follower
    inline constexpr double kAgcReleaseS = 0.4;
    inline constexpr float  kAgcCeil   = 0.7f;
    inline constexpr float  kKneeT     = 0.501187234f;   // D9 knee: identity at or below −6 dBFS
    inline constexpr float  kKneeA     = 1.99526231f;    // D9 ceiling +6 dBFS (output reaches float (A))
    inline constexpr double kFreezeXfadeS = 0.010;       // release content crossfade (raised cosine)
    inline constexpr double kFreezeSlopeS = 0.002;       // held continuation: odd → even mirror blend (2.4 deviation)
    inline constexpr float  kRevPredelayMs = 20.0f;      // reverb fixed internals (ARCH §13)
    inline constexpr float  kRevMod        = 0.2f;
    inline constexpr float  kRevShimmer    = 0.0f;

    // ── Phase 2.5a (PLAN-2.5a layout table; RESEARCH-2.5a R39–R58, CONTEXT-2.5a
    //    D13–D16). Tempo tracker / clock, LFO / env / macro sources, matrix. ──
    inline constexpr int    kLfos       = 4;
    inline constexpr int    kMacros     = 4;
    inline constexpr int    kModSlots   = 16;
    inline constexpr int    kModSources = 12;     // incl. None (0): LFO 1–4, Env, Pitch, Onset, Macro 1–4
    inline constexpr int    kModDests   = 113;    // incl. None (0) — GranulatorParamIds::kModDestIds
    inline constexpr int    kSrcTelemetry = 11;   // sources 1…11
    inline constexpr double kModRampMs  = 20.0;   // slot depth, residual, macro and lfo_phase ramps
    inline constexpr double kTempoFallbackBpm = 120.0;   // no playhead / no ppq / bpm <= 0 (WR-09)
    inline constexpr double kRetimeSamples    = 0.5;     // R42: re-anchor when the host is off by more
    inline constexpr double kRelockSeconds    = 0.001;   // R42: … and re-lock beyond 1 ms (loop, locate)
    inline constexpr double kDensityMin = 0.5;    // R45: Tempo density multiplier clamp (g/s)
    inline constexpr double kDensityMax = 200.0;
    inline constexpr int    kTraceCapacity = 65536;   // test builds: traceDest ring

    /** The 22 tempo divisions in QUARTER notes (parameter-spec order: 1/64 T …
        8 bars). Bars hold their 4/4 value; kTempoDivBars ≠ 0 marks them, and the
        engine uses bars × the host bar length (R44). T = ×2/3, D = ×3/2. */
    inline constexpr int    kTempoDivs = 22;
    inline constexpr double kTempoDivBeats[kTempoDivs] {
        0.0625 * 2.0 / 3.0, 0.0625, 0.0625 * 1.5,     // 1/64 T, 1/64, 1/64 D
        0.125  * 2.0 / 3.0, 0.125,  0.125  * 1.5,     // 1/32
        0.25   * 2.0 / 3.0, 0.25,   0.25   * 1.5,     // 1/16
        0.5    * 2.0 / 3.0, 0.5,    0.5    * 1.5,     // 1/8
        1.0    * 2.0 / 3.0, 1.0,    1.0    * 1.5,     // 1/4
        2.0    * 2.0 / 3.0, 2.0,    2.0    * 1.5,     // 1/2
        4.0, 8.0, 16.0, 32.0                          // 1, 2, 4, 8 bars (4/4 values)
    };
    inline constexpr int kTempoDivBars[kTempoDivs] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 8 };

    /** Division length in quarters under a bar of barLen quarters (4·num/den). */
    inline double tempoDivBeats (int index, double barLen) noexcept
    {
        const int i = index < 0 ? 0 : (index >= kTempoDivs ? kTempoDivs - 1 : index);
        return kTempoDivBars[i] != 0 ? static_cast<double> (kTempoDivBars[i]) * barLen : kTempoDivBeats[i];
    }

    // ── Phase 2.5b (PLAN-2.5b layout table; RESEARCH-2.5b R59–R78, CONTEXT-2.5b
    //    D17–D20). Onset detector, decimator + MPM, D20 guard, Pitch source,
    //    f0 history ring, absolute follow. See Analysis.h. ───────────────────
    // Onset detector (R60 signed off: edge-triggered, +3 dB re-arm; R75 linear compare)
    inline constexpr double kOnsetHpHz       = 150.0;
    inline constexpr double kOnsetFastAttMs  = 0.5;
    inline constexpr double kOnsetFastRelMs  = 10.0;
    inline constexpr double kOnsetSlowMs     = 80.0;                 // slow attack == release
    inline constexpr double kOnsetRatio      = 1.9952623149688795;   // 10^(6/20)
    inline constexpr double kOnsetRearm      = 1.4125375446227544;   // 10^(3/20)
    inline constexpr double kOnsetRefractS   = 0.05;
    inline constexpr double kOnsetTauS       = 0.15;                 // Onset source time constant (ARCH "150 ms")
    inline constexpr double kOnsetZeroTaus   = 16.0;                 // P64: exactly 0 beyond 16 τ (no subnormal tail)
    inline constexpr float  kAnalysisTapMax  = 1.0e4f;               // host-input sanitiser: analysis tap (P61) and ring write (R96): |x·g| beyond +80 dBFS (or non-finite) → 0
    // Decimator + MPM (ARCH §15 / Algorithm §7 contract; R68)
    inline constexpr double kDecimTargetHz = 12000.0;
    inline constexpr int    kDecimTaps     = 64;
    inline constexpr double kDecimCutoff   = 0.4;                    // × fs/D
    inline constexpr int    kMpmWindow     = 1024;
    inline constexpr int    kMpmFftOrder   = 11;                     // 2048-point FFT (zero-pad ×2)
    inline constexpr int    kMpmHop        = 128;                    // decimated samples
    inline constexpr double kMpmKeyMax     = 0.9;
    inline constexpr double kMpmClarity    = 0.8;
    inline constexpr double kMpmGateDb     = -50.0;                  // window RMS gate (dBFS)
    inline constexpr double kMpmMinHz      = 40.0;
    inline constexpr double kMpmMaxHz      = 1500.0;
    // D20 guard (R66 signed off: median-of-3 + asymmetric harmonic-class lookahead, N = 3)
    inline constexpr double kGuardHarm[4] { 7.02, 12.0, 19.02, 24.0 };
    inline constexpr double kGuardTolSt = 0.5;
    inline constexpr int    kGuardN     = 3;
    // Pitch source (P68)
    inline constexpr double kPitchHoldS   = 0.25;
    inline constexpr double kPitchSlewS   = 0.02;
    inline constexpr double kPitchSrcLo   = 24.0;                    // MIDI → 0
    inline constexpr double kPitchSrcSpan = 84.0;                    // MIDI 108 → 1
    // Fixed capacities (no allocation after construction)
    inline constexpr int    kF0RingCap  = 8192;
    inline constexpr int    kCellLogCap = 2048;
    // Tempo bursts (D18 / R73): counter 2^63 | n on kGrainStream + s, disjoint from every grid counter
    inline constexpr std::uint64_t kBurstBit = 1ull << 63;
    // Absolute follow (R70: 1/1 at C4 + scale_root; ±24 st interval clamp)
    inline constexpr double kAbsRefCents     = 6000.0;
    inline constexpr double kAbsMaxSpanCents = 2400.0;

    // ── CounterRng streams and slots ────────────────────────────────────────
    // Draw (seed, base + streamIndex, counter). Scheduler and grain draws use
    // different stream bases, so they never share values for the same counter.
    inline constexpr std::uint64_t kSchedStream = 0x0A5C'0000ull;
    inline constexpr std::uint64_t kGrainStream = 0x0A6E'0000ull;
    inline constexpr std::uint64_t kLfoStream   = 0x0A1F'0000ull;   // 2.5a R51: S&H / smooth random, counter = cycle

    /** Fixed slot per randomised quantity. APPEND-ONLY: 2.2/2.3 use the reserved
        entries; never renumber (it would change every seeded render). */
    namespace Slot
    {
        enum : std::uint32_t
        {
            Jitter = 0,     // scheduler (kSchedStream)
            PosSpray,       // grain (kGrainStream) from here on
            Size,
            Pitch,
            Reverse,
            // reserved — 2.2 / 2.3
            Pan,
            Gain,
            FxCutoff,
            FxReso,
            FxDrive,
            FxCrush,
            FxCrushProb,
            FxRing,
            Window,
            Skew
        };
    }
} // namespace ogi
