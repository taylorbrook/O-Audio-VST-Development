/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "BandlimitedReader.h"   // grain-engine 1.1.0
#include "ReverbUnit.h"          // reverb-unit 1.2.0 (atomic bind, PLAN-2.4 PD11)
#include "CounterRng.h"
#include "GrainPool.h"
#include "GrainScheduler.h"

#include "GiConstants.h"
#include "forked/ControlGrid.h"
#include "forked/FeedbackChain.h"
#include "forked/Grain.h"
#include "forked/OutputLaw.h"
#include "forked/WindowMorphBank.h"
#include "AmpEnv.h"
#include "GiFxLatch.h"
#include "GiRecordHead.h"
#include "GiHeadClamp.h"
#include "KaiserResampler.h"   // kaiser::ratesEqual
#include "GiRenderer.h"
#include "GiScale.h"
#include "GiSpawner.h"
#include "MidiCursor.h"
#include "ScanPlayhead.h"
#include "SlotHandoff.h"
#include "SlotTape.h"
#include "VoiceAllocator.h"
#include "VoiceGrain.h"
#include "VoiceQuotas.h"

// ============================================================================
// Engine — Phases 2.1 / 2.2 (PLAN-2.1 T8, PLAN-2.2 T7 / T8, ARCHITECTURE
// Processing Chain).
//
// Absolute 32-sample grid pass engine. The host block is cut at every
// absolute multiple of kCell; each piece [a, b) runs:
//   1. grid tick (at a multiple of 32): latch GiGridParams, enable edges
//      (epoch += onset count, P14), ramps, take the pending tapes (kill fade /
//      SR-rebuild crossfade), finish crossfades, retire tapes no grain holds
//      (288-slot scan), loop regions (one per stream), polyphony shrink
//      (engine events BEFORE the sample's host events), scale state (custom
//      table try-lock copy, ActiveSets, PD3), quotas (water-fill over the 80
//      (voice, stream) demands + jitter-aware P1 thinning, rescheduleNext on a
//      quota decrease), per-(k,s) law gains with coherence (PD2), scan
//      re-anchor;
//   2. host MIDI at its clamped offsets (iterated in place; P5), splitting the
//      piece into segments;
//   3. per segment: amp envs (a[k][n]), schedulers (Free, one per counted
//      voice × enabled stream, telescoping jitter), onsets merged by
//      (offset, serial, stream) together with voice-end points, rendered
//      grain-major in sub-segments split at those points;
//   4. mix (PD12): per stream acc_s = Σ_k a_k · g_out,kS · bus_kS, then
//      Σ_s acc_s · level_s · aud_s · voiceGain · pan_s (the sum starts from the
//      first audible stream's term; a stream whose audibility ramp sits at 0
//      is skipped) → wet safety = the parent D9 knee wetKnee (identity at or
//      below kKneeT; PLAN-2.4 PD1) → output_gain → out.
//      Reverb absent (unity), feedback return zeroed (P8).
//
// Stream s reads slot s. Every random draw keys on (seed, kGrainStream + s /
// kSchedStream + s, (serial << 24) | ((epoch_kS + onsetIndex) & 0xFFFFFF)) —
// never the voice slot (PD13).
//
// Threads: process() is the audio thread (no allocation; the only lock is the
// PD3 non-blocking try-lock of the custom scale table; the caller holds
// ScopedNoDenormals). prepare() / bind() / bindScale() / the constructor run
// on a host / message thread while process() is not running.
// ============================================================================
namespace ogi
{
   #if OGI_TEST_HOOKS
    /** One spawned grain (row V3 / N1 per-serial logs; 2.2 statistics arm). */
    struct GrainLogEntry
    {
        std::uint64_t serial      = 0;
        std::uint64_t onsetIndex  = 0;    // scheduler index since note-on / the last enable edge
        std::int64_t  onsetSample = 0;    // absolute engine sample of the onset
        double        frac        = 0.0;  // onset fraction [0, 1)
        double        len         = 0.0;  // grain length, output samples
        double        startPos    = 0.0;  // tape frames (region-fit p0, before the frac advance)
        double        rate        = 0.0;  // tape frames per output sample (forward magnitude)
        int           voice       = 0;    // voice slot (NOT part of the identity)
        int           level       = 0;
        int           poolSlot    = 0;
        // 2.2
        int           stream      = 0;    // 0..3 (= the tape slot read)
        int           dir         = 1;    // +1 forward, −1 reverse
        float         gainL       = 1.0f; // spread pan × gain_rand
        float         gainR       = 1.0f;
        int           windowStep  = 0;    // WindowMorphBank step 0..64
        float         tiltT       = 0.5f; // makeTilt (0.5 + 0.45·skew).t
        float         pitchCents  = 0.0f; // latched random / scale-pick offset (cents)
        double        centre      = 0.0;  // scan centre c (tape frames), before spray
        double        regionW     = 0.0;  // loop-region width W (tape frames)
        std::uint64_t counterIdx  = 0;    // epoch_kS + onsetIndex (the CounterRng index)
        // 2.3 (PLAN-2.3 T5) — the REAL latch (fxSharedLatch never rewrites these)
        std::uint32_t fxActive    = 0;
        int           svfMode     = 0;
        float         fcHz        = 0.0f;
        float         ringHz      = 0.0f;
        bool          crushed     = false;
        float         driveD      = 1.0f;
    };

    /** FX STATE of a grain (PLAN-2.3 PD12): the fields the stages advance. The
        latch is separate (assignFxLatch). Test-only helpers. */
    namespace fxstate
    {
        inline void copy (Grain& dst, const Grain& src) noexcept
        {
            dst.svfIc1[0] = src.svfIc1[0]; dst.svfIc1[1] = src.svfIc1[1];
            dst.svfIc2[0] = src.svfIc2[0]; dst.svfIc2[1] = src.svfIc2[1];
            dst.crushHoldPhase = src.crushHoldPhase;
            dst.crushHeld[0] = src.crushHeld[0]; dst.crushHeld[1] = src.crushHeld[1];
            dst.ringRe = src.ringRe; dst.ringIm = src.ringIm;
        }

        inline bool sameBits (float a, float b) noexcept
        {
            std::uint32_t x = 0, y = 0;
            std::memcpy (&x, &a, 4);
            std::memcpy (&y, &b, 4);
            return x == y;
        }

        /** Bitwise equality of the FX state. */
        inline bool stateEq (const Grain& a, const Grain& b) noexcept
        {
            return sameBits (a.svfIc1[0], b.svfIc1[0]) && sameBits (a.svfIc1[1], b.svfIc1[1])
                && sameBits (a.svfIc2[0], b.svfIc2[0]) && sameBits (a.svfIc2[1], b.svfIc2[1])
                && sameBits (a.crushHoldPhase, b.crushHoldPhase)
                && sameBits (a.crushHeld[0], b.crushHeld[0]) && sameBits (a.crushHeld[1], b.crushHeld[1])
                && sameBits (a.ringRe, b.ringRe) && sameBits (a.ringIm, b.ringIm);
        }

        /** Bitwise equality of the FX latch. */
        inline bool latchEq (const Grain& a, const Grain& b) noexcept
        {
            return a.fxActive == b.fxActive && a.svfMode == b.svfMode
                && sameBits (a.svfK, b.svfK) && sameBits (a.svfA1, b.svfA1) && sameBits (a.svfA2, b.svfA2)
                && sameBits (a.svfA3, b.svfA3) && sameBits (a.svfMake, b.svfMake)
                && sameBits (a.driveD, b.driveD) && sameBits (a.driveNorm, b.driveNorm)
                && sameBits (a.crushHoldInc, b.crushHoldInc) && sameBits (a.crushScale, b.crushScale)
                && sameBits (a.ringRotRe, b.ringRotRe) && sameBits (a.ringRotIm, b.ringRotIm)
                && sameBits (a.ringMix, b.ringMix);
        }

        /** PD12 poison: every FX state field away from its Grain {} default. */
        inline void poison (Grain& g) noexcept
        {
            g.svfIc1[0] = g.svfIc1[1] = 0.7f;
            g.svfIc2[0] = g.svfIc2[1] = 0.7f;
            g.crushHeld[0] = g.crushHeld[1] = 0.4f;
            g.crushHoldPhase = 0.3f;
            g.ringRe = 0.0f;
            g.ringIm = 1.0f;
        }

        /** FZ1: subnormal FX state fields of one grain. */
        inline int subnormals (const Grain& g) noexcept
        {
            const float f[9] { g.svfIc1[0], g.svfIc1[1], g.svfIc2[0], g.svfIc2[1], g.crushHoldPhase,
                               g.crushHeld[0], g.crushHeld[1], g.ringRe, g.ringIm };
            int n = 0;
            for (float x : f)
                n += std::fpclassify (x) == FP_SUBNORMAL ? 1 : 0;
            return n;
        }
    }

    /** Render-harness hooks. Plain fields written on the audio thread and read
        by the (single-threaded) harness between processBlock calls. Fixed-size
        storage only; the log is sized once in the Engine constructor. */
    struct EngineTestHooks
    {
        static constexpr int kLogCapacity    = 65536;
        static constexpr int kStolenCapacity = 256;

        // ── switches (set before rendering) ──────────────────────────────────
        bool thinningOff   = false;   // Q1b control: m = ∞ (no integer thinning)
        bool overQuotaOff  = false;   // Q2 control: acquire's 2·overQuota term zeroed
        bool periodThinOff = false;   // Q2 regime: spawn period ignores m (demand outruns the quota) but acquire keeps the finite m
        bool d25Off        = false;   // Q3 control: no over-cap convergence kill
        bool stealRampZero = false;   // V2 control: voice steal / poly shrink cut in 0 ms
        bool retireScan256 = false;   // H3 control: retire scan of the 256 live slots only
                                      //   (an early retire is COUNTED in earlyRetires and
                                      //    the tape is kept, so the control cannot UAF)
        bool seedOverrideOn = false;  // N1 / V3: replace the processor seed
        std::uint64_t seedOverride = 0;
        bool slotPermOn = false;      // V3: free-slot search order = slotPerm[0..19]
        int  slotPerm[kVoiceSlots] {};
        // 2.2 (PLAN-2.2 T8 / T9)
        bool   coherenceOff      = false;   // PD2 / RB1: c := 0 (2.1 law equivalence)
        bool   coherenceVerbatim = false;   // CO1 control: option (a), pitch arg = sN_pitch
        bool   lawOff            = false;   // LF1 control: g_out := 1
        bool   forceBankPath     = false;   // W0b control: no P14 closed-form Hann fast path
        bool   mixerRampZero     = false;   // MX1 control: stream level / pan / audibility step (0 ms)
        double voiceGainDbOverride = std::numeric_limits<double>::quiet_NaN();   // NaN = off (kVoiceGainDb)
        double jitterScale       = 1.0;     // PS3 engine-side control: J × jitterScale
        // 2.3 (PLAN-2.3 PD12; the parent Engine.h:218-242 shapes)
        bool fxSharedLatch    = false;   // FV2–FV4 control: latch re-planned with filter_rand = ring_rand = 0, crush_prob = 1 (log keeps the real draws)
        bool fxNoReset        = false;   // FB control: the previous occupant's FX state (fxShadow) is copied into the new grain
        bool poisonFreeSlots  = false;   // FB1 / FB4: every inactive slot's FX state := poison at each grid tick
        bool snapshotSteals   = false;   // FB3: snapshot the live slots before acquire, compare the fade copy on a steal
        bool stealSnapPerturb = false;   // FB3 control: 1-ulp flip of the snapshot's svfIc1[0] before comparing
        bool nanKillOff       = false;   // FZ control: skip the P12 finite check
        bool fxOrderSwap      = false;   // PFc5: crush before drive
        bool legacySvfMake    = false;   // PFc6: svfMake := 1 (the pre-D10 latch)
        bool wetSafetyOff     = false;   // PF rows: the wet knee skipped (identity)
        // 2.4 (PLAN-2.4 PD19)
        bool legacyWetTanh    = false;   // PD1 attribution: the 2.1–2.3 kWetCeil·tanh statements instead of wetKnee
        bool reverbHardCut    = false;   // RV2 control: reverb.reset() at the mix-0 edge (no 50 ms fade)
        struct InjectAt { std::int64_t atSample = -1; float value = 0.0f; bool done = false; };
        InjectAt injectPreKnee;          // CL2 control: wet L / R := value at atSample (before the knee)
        bool chainKnobParity  = false;   // PB rows: the chains read 0.95·feedback (the parent drive), not unity
        bool sumTap           = false;   // FL2 control: tap ÷ 1 instead of ÷ max (1, Σa)
        bool fbArchLaw        = false;   // FL8 control: g_fb = fbTapGainArch (1 / max (1, S1))
        struct Probe { float* buf = nullptr; std::int64_t len = 0; int slot = 0; };
        Probe tapProbe, retProbe;        // interleaved L / R per absolute sample of one slot (PB rows)
        struct InjectFb { int slot = -1; std::int64_t atSample = -1; bool done = false; };
        InjectFb injectFbNan;            // FL6: the leaf's testInjectAt in the first piece >= atSample
        bool lambdaRampZero   = false;   // CK2 control: the feedback (λ) ramp steps at the tick (0 ms)
        bool runRampZero      = false;   // CK3 control: the head run ramp steps (hard start / stop)
        bool freezeHardStop   = false;   // CK1 control: on a freeze edge the run ramp snaps to 0
        bool resetHardSwap    = false;   // CK4 control: a Reset take swaps tapes with no crossfade (x.len 1)
        bool writeScrubOff    = false;   // FL6 control: a non-finite write lands in the tape
        bool rClampOff        = false;   // FL1s control: no ±0.7 writer clamp
        bool settleOff        = false;   // MS1 control: no mip settle at wrap / park
        bool runIgnoreRate    = false;   // SR1 control: the run condition ignores tape rate != fs
        struct InjectRet { int slot = -1; std::int64_t atSample = -1; float value = 0.0f; bool done = false; };
        InjectRet injectReturn;          // FL1s / FL6 controls: the writer's return r := value at atSample
        bool headClampOff     = false;   // HC1 control: no spawner head clamp
        bool headReadScan     = false;   // HC1: per segment, live grain path vs the running head path
        bool fxSubnormalScan  = false;   // FZ1: per-tick scan of the active slots' FX state for subnormals
        struct Inject { int slot = -1; std::int64_t atSample = 0; int field = 0; float value = 0.0f; bool done = false; };
        Inject injectNan;                // write value into SVF field (0 ic1[0], 1 ic1[1], 2 ic2[0], 3 ic2[1]) of slot, first segment t0 >= atSample
        struct Retire { int slot = -1; std::int64_t atSample = 0; bool done = false; };
        Retire forceRetire;              // same trigger → the P12 verdict forced false

        // ── voice readouts (per voice SLOT; stream-0 views) ──────────────────
        double        voiceRate[kVoiceSlots]   {};   // rate the voice latches now (tick, note-on, s1 spawn)
        int           voiceLevel[kVoiceSlots]  {};   // latchLevel (rate, 4) of that rate
        std::uint64_t voiceSerial[kVoiceSlots] {};
        int           voiceNote[kVoiceSlots]   {};
        int           voiceState[kVoiceSlots]  {};   // 0 free, 1 counted, 2 stolen

        // ── grain log (ring) ──────────────────────────────────────────────────
        std::vector<GrainLogEntry> log;              // kLogCapacity entries
        std::uint64_t logCount = 0;                  // total written (ring index = n % capacity)

        // ── counters ──────────────────────────────────────────────────────────
        std::uint64_t spawns = 0, r3Kills = 0, nativeSteals = 0, capKills = 0, swapKills = 0;
        std::uint64_t voiceSteals = 0, polyForceReleases = 0, gridTicks = 0;
        std::uint64_t stolenSerials[kStolenCapacity] {};   // ring of stolen note serials
        std::uint64_t stolenCount = 0;
        std::uint64_t tapesTaken = 0, xfadesStarted = 0, earlyRetires = 0, retiredByEngine = 0;
        // 2.2 scale state (PD3)
        std::uint64_t customCopies = 0, customTryFails = 0, activeSetRebuilds = 0;
        // 2.3
        std::uint64_t nanKills = 0, nanKillNotRetired = 0, fxStateNonDefaultAtSpawn = 0;
        std::uint64_t stealStateChecked = 0, stealStateMismatches = 0, fxSubnormals = 0;
        // 2.4
        std::uint64_t wetScrubs = 0;                 // R96 wet guard: non-finite wet samples zeroed
        float preKneePeak = 0.0f, kneePeak = 0.0f;   // max |y| before / after the knee (before output_gain)
        RecordHeadCounters head[kTapeSlots] {};      // writes, rClamps, writeScrubs, settles, parks, maxWritten per slot
        std::uint64_t swapWaits = 0;                 // ticks a pending tape waited for its head to park
        std::uint64_t forcedCrossings = 0;           // PD9: no feasible p0 (or reach >= W) while the head runs
        std::uint64_t clampMoves = 0;                // PD9: p0 moved out of the forbidden set
        std::uint64_t headScanViolations = 0;        // headReadScan: a grain spawned with a running head read inside [−g_b, g_a]
        std::uint64_t parkedSpawnCrossings = 0;      // headReadScan INFO: the same for grains spawned while parked
        std::uint64_t headScanSegments = 0;          // headReadScan liveness: grain segments tested against a running head
        bool spawnedRunning[kPoolLive] {};           // headReadScan: the head was running (and the clamp ran) at spawn
        bool spawnedForced[kPoolLive] {};
        Grain fxShadow[kPoolLive + kPoolFade] {};            // FX state at the last retire (and poison)
        std::array<VoiceGrain, kPoolLive> stealSnap {};      // FB3 snapshot

        // ── per-tick pool / quota state (written at every grid tick) ──────────
        int    liveNow = 0;                 // non-killing grains in the 256 live slots
        int    liveMin = INT_MAX, liveMax = 0;
        int    activeNow = 0;               // every sounding slot (live + killing + fade), 288
        int    liveVoice[kVoiceSlots] {};   // non-killing live grains per voice slot (all streams)
        int    quotaM[kVoiceSlots]    {};   // stream-0 view of quotaMVS
        double quotaQ[kVoiceSlots]    {};   // stream-0 view of quotaQVS
        double periodT[kVoiceSlots]   {};   // stream-0 view of periodTVS
        double lambda = 0.0;
        bool   capped = false;
        // 2.2 per (voice slot, stream)
        int    quotaMVS[kVoiceSlots][kGiStreams]    {};   // P1 thinning integer (INT_MAX = uncapped)
        double quotaQVS[kVoiceSlots][kGiStreams]    {};
        double periodTVS[kVoiceSlots][kGiStreams]   {};   // thinned onset period, samples
        int    liveVS[kVoiceSlots][kGiStreams]      {};   // non-killing live grains
        float  lawTargetVS[kVoiceSlots][kGiStreams] {};   // g_out target pushed this tick
        double coherenceVS[kVoiceSlots][kGiStreams] {};   // c used by that target

        // ── slots ─────────────────────────────────────────────────────────────
        double regionLo = 0.0, regionHi = 0.0;      // s1 loop region, tape frames
        double regionLoS[kGiStreams] {}, regionHiS[kGiStreams] {};   // per stream (2.2)
        bool   xfadeActive[kTapeSlots] {};
        double tapeRate[kTapeSlots]    {};
        int    retiringNow = 0;

        /** Zeroes counters, min / max trackers and the log index (not switches). */
        void resetCounters() noexcept
        {
            logCount = 0;
            spawns = r3Kills = nativeSteals = capKills = swapKills = 0;
            voiceSteals = polyForceReleases = gridTicks = 0;
            stolenCount = 0;
            tapesTaken = xfadesStarted = earlyRetires = retiredByEngine = 0;
            customCopies = customTryFails = activeSetRebuilds = 0;
            nanKills = nanKillNotRetired = fxStateNonDefaultAtSpawn = 0;
            stealStateChecked = stealStateMismatches = fxSubnormals = 0;
            wetScrubs = 0;
            preKneePeak = kneePeak = 0.0f;
            for (auto& h : head)
                h = RecordHeadCounters {};
            swapWaits = 0;
            forcedCrossings = clampMoves = headScanViolations = parkedSpawnCrossings = headScanSegments = 0;
            liveMin = INT_MAX;
            liveMax = 0;
        }

        /** The log in chronological order (copy; harness thread). */
        std::vector<GrainLogEntry> logInOrder() const
        {
            std::vector<GrainLogEntry> out;
            if (log.empty())
                return out;
            const std::uint64_t cap = static_cast<std::uint64_t> (log.size());
            const std::uint64_t n = logCount < cap ? logCount : cap;
            out.reserve (static_cast<std::size_t> (n));
            for (std::uint64_t i = logCount - n; i < logCount; ++i)
                out.push_back (log[static_cast<std::size_t> (i % cap)]);
            return out;
        }

        /** Identity permutation for slotPerm, then offset (V3 helper). */
        void setSlotOffset (int offset) noexcept
        {
            for (int i = 0; i < kVoiceSlots; ++i)
                slotPerm[i] = ((i + offset) % kVoiceSlots + kVoiceSlots) % kVoiceSlots;
            slotPermOn = true;
        }
    };
   #endif

    class Engine
    {
    public:
        explicit Engine (SlotHandoff& handoff);
        ~Engine();

        /** Message thread (processor constructor). */
        void bind (juce::AudioProcessorValueTreeState& apvts);

        /** Message thread (processor constructor, after bind): the processor's
            scale state (PD3). Copies the custom table (blocking) once. */
        void bindScale (const ScaleSource& source) noexcept;

        /** Host thread (prepareToPlay); process() is not running. Allocates the
            feedback return; clears voices and pool; retires idle tapes. */
        void prepare (double sampleRate, int maxBlockSize);

        /** Audio thread. outL / outR must be CLEARED by the caller; numSamples
            may exceed the prepared block size (everything is 32-sample scratch). */
        void process (float* outL, float* outR, int numSamples, const juce::MidiBuffer& midi,
                      std::uint64_t seed) noexcept;

        double       getSampleRate() const noexcept     { return fs; }
        std::int64_t getSamplePosition() const noexcept { return absPos; }
        const SlotTape* getCurrentTape (int slot) const noexcept
        {
            return (slot >= 0 && slot < kTapeSlots) ? current[(size_t) slot] : nullptr;
        }

        /** Rate (tape frames per output sample) a voice on `note` latches on
            stream s with the current grid parameters and the current slot-s
            tape (0 without a tape). No random pitch term. */
        double rateForNote (int note, int s = 0) const noexcept;

       #if OGI_TEST_HOOKS
        EngineTestHooks hooks;
        const GiPool&    getPool() const noexcept           { return pool; }
        /** H3 stimulus: re-prepare the pool with a longer steal fade so a fade
            spare can outlive the 5 ms swap kills (clears the pool; call before
            any note). The shipping fade stays kPoolFadeMs. */
        void testSetPoolFadeMs (double ms) noexcept       { pool.prepare (fs, ms); }
        const GiSpawner& getSpawner() const noexcept        { return spawner; }
        const Voice&     getVoice (int k) const noexcept    { return voices[std::clamp (k, 0, kVoiceSlots - 1)]; }
        const GiGridParams& getGridParams() const noexcept  { return gp; }
        int              getRetiringCount() const noexcept  { return numRetiring; }
        const LoopRegion& getRegion (int s = 0) const noexcept { return region[std::clamp (s, 0, kGiStreams - 1)]; }
        const GiScaleState& getScaleState() const noexcept  { return scale; }
        const FeedbackChain& getChainForTest (int s) const noexcept { return chain[std::clamp (s, 0, kGiStreams - 1)]; }
        FeedbackChain&       chainForTest (int s) noexcept          { return chain[std::clamp (s, 0, kGiStreams - 1)]; }
        const float (&getFbRetForTest (int s) const noexcept)[2][kFbDelay] { return fbRet[std::clamp (s, 0, kGiStreams - 1)]; }
        const RecordHead& getHeadForTest (int s) const noexcept     { return head[std::clamp (s, 0, kTapeSlots - 1)]; }
        const WindowMorphBank& getMorphBank() const noexcept { return morphBank; }
       #endif

    private:
        struct PendingOnset
        {
            int           voice  = 0;
            int           stream = 0;
            int           offset = 0;    // segment-relative
            double        frac   = 0.0;
            std::uint64_t index  = 0;    // scheduler index since the last reset
            std::uint64_t idx    = 0;    // epoch_kS + index (the CounterRng index, PD13)
            std::uint64_t serial = 0;
        };

        struct PendingEnd
        {
            int           voice  = 0;
            int           offset = 0;    // segment-relative
            std::uint64_t serial = 0;
        };

        // grid
        void gridTick (std::int64_t t) noexcept;
        void takeTapes (std::int64_t t) noexcept;
        void finishXfades (std::int64_t t) noexcept;
        void retireScan() noexcept;
        void killGrainsOfTape (const SlotTape* tape) noexcept;
        void updateRegion() noexcept;
        void enforcePolyphony() noexcept;
        void updateScale() noexcept;
        void updateQuotas() noexcept;
        void updateScan (std::int64_t t) noexcept;

        // events
        void applyEvent (const MidiEvent& e, std::int64_t t) noexcept;
        void noteOn (int channel, int note, float velocity01, std::int64_t t) noexcept;
        void noteOff (int channel, int note) noexcept;
        void allNotesOff() noexcept;
        void allSoundOff() noexcept;
        void startVoice (int k, int channel, int note, float velocity01, std::int64_t t) noexcept;
        void stealVoice (int k) noexcept;
        void forceReleaseVoice (int k) noexcept;
        void endVoice (int k) noexcept;
        bool endQuietestStolen() noexcept;

        // audio
        void runSegment (int c0, int c1, std::int64_t t0) noexcept;
        void renderGrains (int c0, int c1, std::int64_t t0) noexcept;
        void spawn (const PendingOnset& o, std::int64_t tAbs) noexcept;
        void mixPiece (int c0, int c1, float* outL, float* outR, std::int64_t tAbs) noexcept;
       #if OGI_TEST_HOOKS
        void scanGrainVsHead (const VoiceGrain& vg, int slot, int c0, int c1) noexcept;
       #endif
        void writeReverbAtomics() noexcept;
        void updateChainCutoffs() noexcept;
        float pairFbGain (int k, int s, double nEff) const noexcept;
        void runChains (int c0, int c1, std::int64_t tAbs) noexcept;
        void updateHeads (std::int64_t t) noexcept;   // ramps, run condition, park (before takeTapes)
        void writeHeads (std::int64_t t) noexcept;    // the whole cell, at the tick (after updateRegion)
        RecordHeadCounters& headCounters (int s) noexcept;

        // helpers
        AmpParams ampParams() const noexcept;
        double voiceSemis (const Voice& v, int s) const noexcept;
        double voiceRate (const Voice& v, const SlotTape& tape, int s) const noexcept;
        double uncappedPeriod (int s) const noexcept;
        double lMaxSamples (int s) const noexcept;
        double lenBarSamples (int s) const noexcept;
        double jitterOf (int s) const noexcept;
        bool   audibleTarget (int s, bool anySolo) const noexcept;
        double jitterU (int s, std::uint64_t serial, std::uint64_t epoch, std::uint64_t k) const noexcept;
        double jitterGap (double T, double J, int s, std::uint64_t serial, std::uint64_t epoch, std::uint64_t k) const noexcept;
        double pairCoherence (int k, int s) const noexcept;
        float  pairLaw (int k, int s, double nEff) noexcept;
        void   setStreamRamps (bool snap) noexcept;
        void   writeVoiceHooks (int k) noexcept;

        static constexpr int kLawKeyPerStream = 16;
        static constexpr int kLawKeySize = kGiStreams * kLawKeyPerStream + 5;
        void fillLawKey (double (&key)[kLawKeySize]) const noexcept;

        SlotHandoff& handoff;
        GiParamPointers ptrs;
        GiGridParams gp;
        WindowMorphBank morphBank;   // built once here (message thread); renderer bank path + law
        GiScaleState scale;          // PD3: custom-table copy + per-stream ActiveSets

        double        fs = 44100.0;
        std::int64_t  absPos = 0;
        std::uint64_t seed = 0;
        float         bend = 0.0f;   // −1..1, channel pitch wheel (non-MPE; any channel)
        float         voiceGainLin = 1.0f;
        float         voiceGainNow = 1.0f;   // voiceGainLin, or the test override (per block)

        GiPool    pool;
        GiSpawner spawner;

        Voice voices[kVoiceSlots];
        std::uint64_t serialCounter = 0;

        std::array<SlotTape*, kTapeSlots> current {};
        SlotTape* retiring[kMaxRetiring] {};
        int numRetiring = 0;
        XfadeState xfade[kTapeSlots];

        float    envBuf[kVoiceSlots][kCell] {};
        VoiceBus bus;
        CellLerp lawGain[kVoiceSlots][kGiStreams];
        float    lawTarget[kVoiceSlots][kGiStreams] {};
        double   coher[kVoiceSlots][kGiStreams] {};
        float    accL[kGiStreams][kCell] {}, accR[kGiStreams][kCell] {};   // mixer scratch

        ExactRamp outGainRamp;
        ExactRamp levelRamp[kGiStreams], panLRamp[kGiStreams], panRRamp[kGiStreams], audibleRamp[kGiStreams];
        int rampCells20 = 1, rampCells10 = 1;

        QuotasN<kPairs> quotas;
        int    quotaM[kVoiceSlots][kGiStreams] {};
        double periodT[kVoiceSlots][kGiStreams] {};
        double prevDemand[kPairs] {};
        int    prevCap = -1;
        double prevLMax[kGiStreams] {}, prevDensity[kGiStreams] {}, prevJitter[kGiStreams] {};
        double prevLawKey[kLawKeySize] {};
        int    prevThinOff = 0;
        bool   quotasDirty = true;
        double jit[kGiStreams] {};          // effective jitter J_s latched at the tick
        bool   prevEnable[kGiStreams] {};   // P14 enable-edge detection

        ScanAnchor freeRun[kGiStreams];
        LoopRegion region[kGiStreams];

        // 2.4 feedback network (PLAN-2.4 PD7 / PD8): per stream a mean tap →
        // FeedbackChain (VERBATIM leaf, driven at a UNITY knob: the loop gain
        // lives in the writer's λ, P1) → fbRet, the z^-32 return of the
        // previous cell (kFbDelay == kCell). Fixed arrays, zeroed in prepare.
        FeedbackChain chain[kGiStreams];
        float fbRet[kGiStreams][2][kFbDelay] {};
        ExactRamp unityKnob, parityKnob, fbLowLogRamp, fbHighLogRamp, diffRamp, satDRamp;
        float chainLowHz = 0.0f, chainHighHz = 0.0f;
        CellLerp gFbLerp[kVoiceSlots][kGiStreams];
        float gFbTarget[kVoiceSlots][kGiStreams] {};
        float tapAcc[kGiStreams][2][kCell] {};
        float envSum[kGiStreams][kCell] {};
        float tapBuf[2][kCell] {};

        // 2.4 record heads (PLAN-2.4 PD2–PD5): one per slot, λ = 0.95·fbRamp·run.
        ExactRamp  fbRamp;
        RecordHead head[kTapeSlots];
        bool       headRelatch[kTapeSlots] {};
        bool       prevFreeze = false;
        float      pkRel = 0.0f;                 // returnPk release per cell (50 ms)
        RecordHeadCounters headCntShip[kTapeSlots] {};

        // 2.4 output (PLAN-2.4 PD11): ReverbUnit bound to engine-owned atomics
        // (size / damp / mix written at grid ticks only; fixed internals), the
        // wet bus of one piece (reverb → knee act here) and a non-owning view.
        ReverbUnit reverb;
        std::atomic<float> revEnabled { 1.0f }, revSize { 0.5f }, revDamp { 0.5f }, revPredelay { kRevPredelayMs },
                           revMix { 0.0f }, revMod { kRevMod }, revShimmer { kRevShimmer };
        float wet[2][kCell] {};
        juce::AudioBuffer<float> revView;
        bool  revMixWasOn = false;     // reverbHardCut edge detection (test)

        ouaricon::grain::Onset onsetBuf[kVoiceSlots][kGiStreams][kOnsetCap] {};
        PendingOnset merged[kVoiceSlots * kGiStreams * kOnsetCap] {};
        PendingEnd   ends[kVoiceSlots] {};

        JUCE_DECLARE_NON_COPYABLE (Engine)
    };
} // namespace ogi
