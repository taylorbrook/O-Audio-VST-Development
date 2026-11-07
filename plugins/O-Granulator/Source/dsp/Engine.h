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

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "Analysis.h"         // 2.5b onset detector, MPM, D20 guard, f0 ring (P61)
#include "CaptureWriter.h"
#include "ControlGrid.h"
#include "EngineConstants.h"
#include "FeedbackChain.h"
#include "Grain.h"
#include "GrainFx.h"
#include "ModMatrix.h"
#include "ModSources.h"
#include "OutputLaw.h"
#include "Quotas.h"
#include "ReverbUnit.h"     // reverb-unit 1.2.0 (atomic bind, D11)
#include "ScaleQuantiser.h"
#include "Spawner.h"
#include "StreamScheduler.h"
#include "TempoTracker.h"
#include "UiTelemetry.h"      // 3.2 buffer strip: triple buffer, peak ring (P114)
#include "WindowMorphBank.h"

// ============================================================================
// O-Granulator pass engine (Stage 2, Phases 2.1–2.6).
// ARCHITECTURE Processing Chain (per-pass order, normative), §1–7, §10, §14, §16;
// RESEARCH-2.1 R1–R4; PLAN-2.1 T7.
//
// The host block is cut at every ABSOLUTE multiple of kCell (absCount, int64).
// Each piece [a, b) (<= 32 samples, inside one cell) runs:
//   1. grid tick if a % 32 == 0: latch GridParams, custom-table try-lock copy
//      (P11), active-set rebuilds, water-filled quotas, R5 coherence → g_out,
//      mute/solo audibility, mixer ramps (10 ms, P12)
//      2.5a: then the tempo slot state, the mod sources (LFO ×4, env, macros —
//      sampled once per cell) and the sparse matrix (routed destinations only;
//      unrouted = the raw 2.4 value). The transport snapshot is taken once per
//      HOST block (TempoTracker), before the first piece.
//      2.5b: after the freeze edges, the analysis tick (Analysis.h): cell log
//      {absCount, W, live}, thrLin, scrub checks, the R59 commit of the pending
//      MPM hop (D20 guard → f0 ring → live note), then the Pitch / Onset
//      sources with the others.
//   2. 2.5b analysis of the piece on the conditioned input (after the env
//      follower): onset detector → at most one event; decimator; an MPM hop
//      into the pending slot. It never touches an audio statement (N3).
//   3. write the piece into the ring (input·inGain + fb[n − 32]) through the
//      freeze Write Arbiter (2.4: no push while frozen; 10 ms mirror content
//      crossfade on release). Grains anchor in the RING domain (R23).
//   4. schedule enabled streams → onsets, merged by (offset, stream); 2.5b: an
//      event re-phases a trigger-on Free stream (D14) or adds one grain to a
//      trigger-on Tempo stream (D18)
//   5. render grain-major, split at onset offsets, spawn at each split
//   6. stream mixer: bus × g_out × level × pan → wet[2][kCell]; feedback tap
//      bus × g_fb × level × audible / max (1, Σ level·audible) → tap[2][kCell]
//      (pre-g_out, no pan — R29, R31)
//   7. feedback chain (FeedbackChain.h) over the tap → fbDelay (z^-32), run
//      every pass, frozen included (the arbiter discards it while frozen)
//   8. reverb (reverb-unit 1.2.0, plugin atomics) → D9 wet knee → dry/wet +
//      output gain → out. Reverb and knee run BEFORE the mix-0 fast path.
// Every per-pass array is kCell long; nothing scales with the host block, so
// an oversized block cannot overrun anything, and output is bit-identical at
// every host block size.
//
// Threading: bind() and prepare() on the message thread (prepare allocates the
// ring: 27.6 MB @ 48 k, 110 MB @ 192 k — D2). process() on the audio thread:
// no allocation, locks or logging; never writes a parameter.
// ============================================================================
namespace ogran
{
    class Engine
    {
    public:
        using Pool = Spawner::Pool;

        /** Message thread: builds the 1.6 MB WindowMorphBank (never rebuilt). */
        Engine() = default;

        /** Message thread, once (processor constructor): the matrix range table
            (the parameter objects' own NormalisableRanges — R46 parity) and the raw
            APVTS atomic of every destination. */
        void setModDestination (int dest, const juce::NormalisableRange<float>& range, std::atomic<float>* raw) noexcept
        {
            matrix.setRange (dest, range);
            matrix.setRawPointer (dest, raw);
        }
        const ModMatrix& getMatrix() const noexcept { return matrix; }

        /** Message thread, once (processor constructor). */
        void bind (const ParamPointers& p, const ScaleSource& scale = {}) noexcept
        {
            ptrs     = p;
            scaleSrc = scale;
            // D11: the reverb reads plugin-owned atomics, written at grid ticks only
            // (2.5's matrix will write modulated values into the same atomics).
            reverb.bind (&revEnabled, &revSize, &revDamp, &revPredelay, &revMix, &revMod, &revShimmer);
        }

        /** Message thread. Allocates the ring, resets every clock and ramp
            (ramps snapped to the current parameter values). */
        void prepare (double sampleRate, int maxBlockSize);

        bool isPrepared() const noexcept { return prepared; }

        /** Audio thread. in/out are the BUS channels (numIn, numOut in {1, 2});
            in and out may alias (JUCE in-place buffers). `seed` is adopted at the
            next grid point. */
        void process (const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, std::uint64_t seed,
                      const TransportSnapshot& transport = TransportSnapshot {}) noexcept;

        //======================================================================
        // Telemetry (P55, R58): relaxed atomics written per cell on the audio
        // thread, read by the UI (Stage 3). dest k: routed → v01, unrouted →
        // base01 (written only when the raw value changes). src 1…11: LFO 1–4
        // in [−1, 1], Env / Macros / Pitch / Onset in [0, 1].
        float getDestTelemetry (int dest) const noexcept
        {
            return dest >= 0 && dest < kModDests ? destTelemetry[static_cast<size_t> (dest)].load (std::memory_order_relaxed) : 0.0f;
        }
        float getSrcTelemetry (int src) const noexcept
        {
            return src >= 1 && src <= kSrcTelemetry ? srcTelemetry[static_cast<size_t> (src - 1)].load (std::memory_order_relaxed) : 0.0f;
        }
        /** 2.5b (P68): the live pitch-follower note (MIDI) and confidence. */
        float getPitchNote() const noexcept      { return pitchNoteTel.load (std::memory_order_relaxed); }
        bool  getPitchConfident() const noexcept { return pitchConfTel.load (std::memory_order_relaxed) > 0.5f; }

        /** Stage 3 (PLAN-3.1 P101, RESEARCH-3.1 R107): the window figure. `n`
            samples (phase i / (n − 1)) of the bank window a grain latches at
            `window` (0…1) and raw `skewRaw` (−100…100) — the same step / tilt
            mapping the grid uses (ModMatrix / spawn). Read-only on the bank,
            which is built once in the constructor and never rebuilt, so a
            message-thread call is race-free. Never called on the audio thread. */
        void windowCurve (float window, float skewRaw, float* out, int n) const noexcept
        {
            if (out == nullptr || n <= 0)
                return;
            const int  step = WindowMorphBank::stepFor (std::clamp (window, 0.0f, 1.0f));
            const auto tilt = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * (std::clamp (skewRaw, -100.0f, 100.0f) / 100.0f));
            const float* tab = bank.table (step);
            for (int i = 0; i < n; ++i)
                out[i] = WindowMorphBank::read (tab, tilt, n > 1 ? static_cast<double> (i) / static_cast<double> (n - 1) : 0.0);
        }

        //======================================================================
        // Stage 3 / 3.2 buffer strip (PLAN-3.2 P114–P116). The audio thread
        // only COPIES into these (peak per ring cell after the capture write;
        // a <= 60 Hz POD snapshot at the end of process()). Nothing here is
        // read by the DSP. Readers: one at a time (the processor's SpinLock).
        /** Reader: the newest complete snapshot (or the previous one again). */
        const UiSnapshot& readUiSnapshot() const noexcept     { return uiSnap.read(); }
        const PeakRing&   getPeakRing() const noexcept        { return peaks; }
        /** Message thread: the grain's window value at `phase` — the exact
            renderSegment branch (closed-form Hann at step 0 / tilt 0.5, else the
            immutable bank). */
        float grainWindow (int step, float tiltT, float tiltA, float tiltB, double phase) const noexcept
        {
            if (step == 0 && juce::exactlyEqual (tiltT, 0.5f))
                return hannWindow (phase);
            const WindowMorphBank::Tilt tilt { tiltT, tiltA, tiltB };
            return WindowMorphBank::read (bank.table (std::clamp (step, 0, WindowMorphBank::kSteps - 1)), tilt, phase);
        }

       #if OGRAN_TEST_HOOKS
        //======================================================================
        // Test hooks — compiled into the render-harness target only.
        struct TestHooks
        {
            bool forceLevel0      = false;   // DSP-02 negative control: every grain reads level 0
            bool disableFracQuant = false;   // R1 negative control: delay = (double) W − readAbs
            bool countR1Hazards   = false;   // count reads whose pre-R1 (t, phi) depends on the head
            // S0 reference (PLAN-2.2 P9/T1): bypass Spawner::acquire entirely and take
            // the bare pool policy, tryObtain() ?: obtain (age·invLen). No cap, no
            // pending-kill bookkeeping, no Spawner state read or written.
            bool nativeAcquire    = false;
            // F4n (PLAN-2.2 T8): progress = age·invLen only (native steal-oldest
            // ranking through the R3 cap) — the FUNC-03 starvation negative control.
            bool nativeProgress   = false;
            // F5 R8 control: overQuota = live_s > q_s (real-valued), not > ceil (q_s).
            bool noQuotaCeil      = false;

            // ── 2.3 (PLAN-2.3 P28) ───────────────────────────────────────────
            // V2n/V3n/V4n: the grain's FX latch is re-planned with filterRand =
            // ringRand = 0 and crushProb = 1 (the "latch right, renderer ignores
            // it" failure); the spawn log keeps the REAL draws.
            bool fxSharedLatch    = false;
            // H1n: the previous occupant's FX state (retire shadow) is copied into
            // the new occupant after fill.
            bool fxNoReset        = false;
            // K2n / K2i: SVF coefficients re-derived from the LIVE stream cutoff /
            // res at every segment (steps the coefficients mid-grain).
            bool fxLiveCoeffs     = false;
            // H2: snapshot the live grains before every acquire; on a native steal
            // compare the fade-slot copy's FX latch + state with the snapshot.
            bool snapshotSteals   = false;
            // Z2: write `value` into SVF integrator `field` (0 ic1[0], 1 ic1[1],
            // 2 ic2[0], 3 ic2[1]) of `slot` in the first segment starting at or
            // after `atSample` (slot < 0 = off). forceRetire: the same trigger
            // forces the P25 verdict to false instead.
            struct Inject { int slot = -1; std::int64_t atSample = 0; int field = 0; float value = 0.0f; bool done = false; };
            Inject injectNan;
            struct Retire { int slot = -1; std::int64_t atSample = 0; bool done = false; };
            Retire forceRetire;

            // ── 2.4 (PLAN-2.4 P33, P35, P38, P39) ───────────────────────────
            bool legacyKnee       = false;   // P33: bypass the D9 wet knee (2.3 output law)
            bool legacySvfMake    = false;   // P33: force svfMake = 1 (2.3 LP/HP, D8)
            bool fbArchLaw        = false;   // F3n / U8n: g_fb = 1/max (1, S1) (the ARCH §10 law)
            bool freezeNoMinSpray = false;   // FZ3n: frozen spawns use the live formula + plain clamp
            bool releaseSpliceOld = false;   // FZ2bn: release continuation ring[W − heldSpan] (R26 control)
            bool freezeNoReflect  = false;   // INFO: no R25 head reflect (the ring clamps instead — stuck reads)
            bool freezeNoDefer    = false;   // FZ2an / FZ6: stop the head AT the edge (the PLAN P38 form: crossers reflect)
            struct FbNan { std::int64_t atSample = -1; bool reset = false; bool done = false; };
            FbNan injectFbNan;               // Z4 / M4: NaN into the chain input at the first piece >= atSample
                                             // (reset: std::fill the chain instead — the Z4 reference)
            bool snapshotChain    = false;   // FZ7: chain state at every tick (ring of 64; the ring hash is harness-side)
            // FB5: per-sample tap capture into a harness buffer (L, R interleaved by absolute sample).
            float*       tapProbe    = nullptr;
            std::int64_t tapProbeLen = 0;

            // ── 2.5a (PLAN-2.5a P56, T4–T9) ─────────────────────────────────
            // P56 trajectory trace: dest = kModDests index (−1 off); stream for
            // the latched record. Ramped dests: the per-sample engine value;
            // latched dests: one entry per spawn of a grain whose field is read.
            struct TraceDest { int dest = -1; int stream = 0; };
            TraceDest traceDest;
            int  forceLfoMode   = 0;       // PH controls: 0 off, 1 always Direct, 2 always Accum
            bool noResidual     = false;   // U9en / MF7n: a route change drops the slot's contribution (a step)
            bool fullRoundTrip  = false;   // U9g: every destination through convertFrom0to1 (convertTo0to1 (raw)) (R46 control)
            bool perBlockOrigin = false;   // TT1n / N8n: re-anchor on the host ppq every block (GrainScatter form, R39)
            bool ceilPerTick    = false;   // TS1n: the pending slot re-derived from ppq at every tick (R40 control)
            std::int64_t injectEnvNanAt = -1;   // EV3: NaN into the env follower state at the first piece >= this sample

            // ── 2.5b (PLAN-2.5b P73) ────────────────────────────────────────
            bool analysisOff          = false;   // OD0: skip the analysis (detector, MPM, sources stay 0)
            bool levelTrigger         = false;   // OD1n: the ARCH §8 level-triggered detector (no armed state)
            bool commitHopImmediately = false;   // N9n: commit a hop at its sample inside the piece (R59 class)
            bool guardOff             = false;   // OG2n: raw hops straight through the D20 guard
            bool ringHold             = false;   // FR7n: write the HELD live value into the f0 ring (R65 control)
            bool strictNewest         = false;   // FR8n: a read point newer than the newest entry misses (strict D17)
            bool lookupAtStart        = false;   // FR9n: look f0 up at the grain's read start, not the content midpoint
            bool liveF0Lookup         = false;   // FR5n / FR6n: follow the LIVE tracker note (the ARCH §5 form)
            struct InjectTap { std::int64_t at = -1; float value = 0.0f; bool done = false; };
            InjectTap injectAnalysis;            // OD6 / Z6b: replace the tap sample at `at`

            // ── 2.6 (PLAN-2.6 P81, P82) ─────────────────────────────────────
            bool   noDepthRamp  = false;   // MX3nD / MF8n: a route change / depth edit snaps the slot depth (no ramp)
            double stealFadeMs  = -1.0;    // K3bn / QB3bn: pool steal fade (ms) at prepare; 0 = hard cut; < 0 = kPoolFadeMs
            double killFadeMs   = -1.0;    // K3an / QB3an: R3 kill fade (ms) at prepare; 0 = hard cut; < 0 = kKillFadeMs
            bool   hostGuardOff = false;   // Z7hn / Z7hr: no R96 ring-write guard (the host sample reaches the ring as is)
            bool   wetGuardOff  = false;   // Z7hn: no R96 wet-bus guard before the reverb
            bool   capNoConverge = false;  // GL3n: no over-cap convergence kill (the pre-2.6 Spawner::acquire)
            bool   deferFloorR   = false;  // Z7m9n: the pre-2.6 deferred-stop floor r·(headLead + 1) (no max (r, 1))
        };

        /** P56 trace ring (test builds): {absolute sample, value}. */
        struct TracePoint { std::int64_t at = 0; double value = 0.0; double aux = 0.0; };   // aux: the spawn's offset (latched)
        std::array<TracePoint, kTraceCapacity> trace {};
        std::uint64_t traceCount = 0;

        /** FZ7 chain snapshot (one per grid tick while snapshotChain). */
        struct ChainSnap { std::int64_t tick = 0; float hpIc1 = 0.0f, hpIc2 = 0.0f, lpIc1 = 0.0f, lpIc2 = 0.0f, env = 0.0f; };
        static constexpr int kChainSnaps = 64;
        std::array<ChainSnap, kChainSnaps> chainSnaps {};
        std::uint64_t chainSnapCount = 0;
        const FeedbackChain& getChainForTest() const noexcept { return chain; }
        const FreezeState&   getFreezeForTest() const noexcept { return fz; }
        /** FZ6: the pool as it was at the last freeze engage (before the R25 scan). */
        std::array<Grain, kPoolLive + kPoolFade> engageSnap {};
        std::array<bool,  kPoolLive + kPoolFade> engageActive {};
        std::int64_t engageW0 = -1;
        const Ring& getRingForTest() const noexcept           { return ring; }
        const Spawner& getSpawnerForTest() const noexcept     { return spawner; }   // 3.2 CN1 census (pending kills)
        /** 2.6 census (P82): the z^-32 feedback return and the reverb (its privates via the harness idiom). */
        const float (&getFbDelayForTest() const noexcept)[2][kFbDelay] { return fbDelay; }
        const ReverbUnit& getReverbForTest() const noexcept    { return reverb; }
        float getFbTapTarget (int s) const noexcept           { return gFbRamp[s].getTarget(); }

        /** One spawn. The grain's readAbs at age k is anchor + dir·rate·(k + frac)
            (exact reference; the engine accumulates readAbs += dir·rate per sample). */
        struct SpawnRecord
        {
            std::int64_t  onsetSample = 0;   // absolute sample index of the onset
            std::int64_t  writeHead   = 0;   // W_on, ring domain (R23): W_piece + offset + 1 live, the stopped head frozen
            std::uint64_t globalIndex = 0;   // CounterRng counter (epoch + index)
            int           stream      = 0;
            int           slot        = 0;   // pool slot at spawn (0..127)
            int           dir         = 1;
            int           level       = 0;
            int           set         = 0;
            double        delay       = 0.0; // D (level-0 samples behind W_on)
            double        anchor      = 0.0; // W_on − D
            double        frac        = 0.0;
            double        rate        = 1.0;
            double        len         = 0.0; // output samples
            float         spanSamples = 0.0f;
            int           grainLimit  = 128;
            bool          killed      = false;
            bool          stolen      = false;
            // 2.2 (PLAN-2.2 T2)
            float         gainL       = 1.0f; // latched spread × gain_rand
            float         gainR       = 1.0f;
            int           windowStep  = 0;
            float         tiltT       = 0.5f;
            float         pitchCents  = 0.0f; // scale pick / continuous offset (cents)
            double        quota       = 0.0;  // the stream's water-filled q_s at spawn
            int           quotaCeil   = 0;
            // 2.3 (PLAN-2.3 T2) — the REAL latch (fxSharedLatch never rewrites these)
            std::uint32_t fxActive    = 0;
            int           svfMode     = 0;
            float         fcHz        = 0.0f;
            float         ringHz      = 0.0f;
            bool          crushed     = false;
            float         driveD      = 1.0f;
            // 2.5b (PLAN-2.5b P71)
            float         f0          = std::numeric_limits<float>::quiet_NaN();    // the looked-up f0 (MIDI), follow streams
            double        target      = std::numeric_limits<double>::quiet_NaN();   // the absolute target (MIDI) on a hit
            bool          followHit   = false;
            int           followMiss  = 0;     // 0 none, 1 unconfident, 2 missing / overwritten, 3 empty set, 4 no member
            double        lookupW     = 0.0;   // the ring position looked up (R64 content midpoint)
            bool          burst       = false; // an onset-trigger burst (Free re-phase / Tempo extra grain)
        };

        static constexpr int kSpawnLogSize = 4096;

        /** Plain counters, written on the audio thread, read by the harness
            BETWEEN blocks. Never feeds a gain law. */
        struct Diag
        {
            std::int64_t reachViolations = 0;   // reads with delay > kRingSamples, or < margin(level) by a NON-releasing grain (pre-reflect, 2.4)
            std::int64_t kills           = 0;   // R3 kill path taken
            std::int64_t maxAlive        = 0;   // max live non-killing grains right after any spawn
            std::int64_t spawns          = 0;
            std::int64_t steals          = 0;   // native pool steals
            std::int64_t r1Hazards       = 0;   // see TestHooks::countR1Hazards
            // FUNC-03 zero-draw proof: Spawner::plan / StreamScheduler::process calls per stream.
            std::int64_t planCalls[kMaxStreams]  {};
            std::int64_t schedCalls[kMaxStreams] {};
            std::int64_t activeSetRebuilds = 0;   // ActiveSet rebuilds (any stream)
            std::int64_t customCopies      = 0;   // P11 audio-side custom-table copies
            std::int64_t customTryFails    = 0;   // P11 try-lock misses (retried next grid)
            // 2.3
            std::int64_t nanKills                 = 0;   // P25 NaN-kill retirements
            std::int64_t fxStateNonDefaultAtSpawn = 0;   // H3: FX state ≠ Grain {} right after fill
            std::int64_t stealStateChecked        = 0;   // H2: steals whose fade copy was compared
            std::int64_t stealStateMismatches     = 0;   // H2: … and differed from the snapshot
            std::int64_t nanKillNotRetired        = 0;   // Z2: killed slot still active right after its segment
            // 2.4
            std::int64_t freezeEngages              = 0;
            std::int64_t freezeReleases             = 0;
            std::int64_t forcedReleases             = 0;   // R25 engage scan: live crossers given the 5 ms kill
            std::int64_t headReflections            = 0;   // R25 reflect taken (any grain)
            std::int64_t headReflectionsFrozenSpawn = 0;   // … by a grain spawned while frozen (expected 0)
            std::int64_t fbScrubs                   = 0;   // feedback chain NaN / Inf resets
            std::int64_t reverbCells                = 0;   // pieces whose wet the reverb changed (RV4 / M4 liveness)
            // 2.5a
            std::int64_t tempoReanchors = 0;   // TempoTracker anchors adopted (excl. prepare)
            std::int64_t tempoRelocks   = 0;   // … of which re-locks (transport start, loop, locate)
            std::int64_t tempoOnsets    = 0;   // onsets emitted by the Tempo clock
            std::int64_t routeChanges   = 0;   // slot source / dest changes seen at ticks
            std::int64_t routedDests    = 0;   // max routed destinations in one cell
            std::int64_t envScrubs      = 0;   // env follower NaN / Inf resets
            float        maxWetPeak     = 0.0f; // Z5: max |wet| after the D9 knee (the knee's own bound, pre mix / output gain)
            // 2.5b (PLAN-2.5b P73)
            std::int64_t onsetEvents       = 0;   // detector events
            std::int64_t maxEventsPerPiece = 0;
            std::int64_t freeBursts        = 0;   // Free re-phases (per stream per event)
            std::int64_t tempoBursts       = 0;   // Tempo extra grains
            std::int64_t mpmHops           = 0;
            std::int64_t mpmConfident      = 0;
            std::int64_t guardHeld         = 0;   // D20: harmonic jumps held at acc
            std::int64_t f0Writes          = 0;
            std::int64_t f0Skips           = 0;   // decided hops centred in a frozen cell
            std::int64_t followLookups     = 0;
            std::int64_t followHits        = 0;
            std::int64_t followMiss[5]     {};    // by kind (index 0 unused)
            std::int64_t absSetRebuilds    = 0;
            std::int64_t analysisScrubs    = 0;
            // 2.6 (PLAN-2.6 P81)
            std::int64_t hostScrubs        = 0;   // R96: host samples the ring-write guard replaced by 0
            std::int64_t wetScrubs         = 0;   // R96: non-finite wet samples zeroed before the reverb
            std::int64_t capKills          = 0;   // D25: over-cap convergence kills (alive > grain_limit)
            std::uint64_t logCount       = 0;   // total spawns logged; entry k at log[k % kSpawnLogSize]
            std::array<SpawnRecord, kSpawnLogSize> log {};

            void reset() noexcept
            {
                reachViolations = kills = maxAlive = spawns = steals = r1Hazards = 0;
                for (int s = 0; s < kMaxStreams; ++s)
                    planCalls[s] = schedCalls[s] = 0;
                activeSetRebuilds = customCopies = customTryFails = 0;
                nanKills = fxStateNonDefaultAtSpawn = stealStateChecked = stealStateMismatches = nanKillNotRetired = 0;
                freezeEngages = freezeReleases = forcedReleases = headReflections = headReflectionsFrozenSpawn = fbScrubs = reverbCells = 0;
                tempoReanchors = tempoRelocks = tempoOnsets = routeChanges = routedDests = envScrubs = 0;
                maxWetPeak = 0.0f;
                onsetEvents = maxEventsPerPiece = freeBursts = tempoBursts = mpmHops = mpmConfident = guardHeld = 0;
                f0Writes = f0Skips = followLookups = followHits = absSetRebuilds = analysisScrubs = 0;
                hostScrubs = wetScrubs = capKills = 0;
                for (auto& m : followMiss) m = 0;
                logCount = 0;
            }
            const SpawnRecord& record (std::uint64_t k) const noexcept
            {
                return log[static_cast<std::size_t> (k % static_cast<std::uint64_t> (kSpawnLogSize))];
            }
        };

        TestHooks hooks;
        Diag      diag;

        const Pool&   getPool() const noexcept                { return pool; }
        std::int64_t  getAbsCount() const noexcept            { return absCount; }
        std::int64_t  getWriteCount() const noexcept          { return ring.getWriteCount(); }
        int           getRingSamples() const noexcept         { return ringSamples; }
        double        getSampleRate() const noexcept          { return fs; }
        std::uint64_t getEpoch (int stream) const noexcept    { return scheds[stream].getEpoch(); }
        const TempoTracker& getTracker() const noexcept       { return tracker; }
        const Lfo& getLfo (int n) const noexcept              { return lfos[n]; }
        const EnvFollower& getEnv() const noexcept            { return env; }
        const SourceValues& getSources() const noexcept       { return srcVals; }
        const GridParams& getGridParams() const noexcept      { return gp; }
        const WindowMorphBank& getBank() const noexcept       { return bank; }
        const Quotas& getQuotas() const noexcept              { return quotas; }
        const ActiveSet& getActiveSet (int s) const noexcept  { return activeSets[s]; }
        const AbsSet& getAbsSet (int s) const noexcept        { return absSets[s]; }
        const Analysis& getAnalysis() const noexcept          { return analysis; }
        std::uint64_t getBurstCount (int s) const noexcept    { return burstCount[s]; }
        std::int64_t  getSlotNext (int s) const noexcept      { return scheds[s].getSlotNext(); }
        const PitchSel& getPitchSel (int s) const noexcept    { return pitchSel[s]; }
        double getCoherence (int s) const noexcept            { return coherence[s]; }
        double getTempoDiv (int s) const noexcept             { return tempoDiv[s]; }
        float  getGOutTarget (int s) const noexcept           { return gOutRamp[s].getTarget(); }
        float  getAudibleTarget (int s) const noexcept        { return audRamp[s].getTarget(); }
        int    getMixerRampCells() const noexcept             { return mixerRampCells; }
        int    getRampCells() const noexcept                  { return rampCells; }
        std::uint32_t getCustomCopyVersion() const noexcept   { return customCopyVersion; }
        const PitchCollections::Table& getCustomCopy() const noexcept { return customCopy; }

        /** 2.3 retire shadow: a live slot's FX STATE as its last occupant left it
            (recorded at every retire in test builds; H1, fxNoReset). */
        struct FxShadow
        {
            float svfIc1[2] {}, svfIc2[2] {};
            float crushHeld[2] {};
            float crushHoldPhase = 1.0f;
            float ringRe = 1.0f, ringIm = 0.0f;
            bool  valid = false;
        };
        const FxShadow& getRetireShadow (int slot) const noexcept { return retireShadow[slot]; }
       #endif

    private:
        void gridTick() noexcept;
        int  scheduleStreams (int len, int eventOffset) noexcept;   // 2.5b: eventOffset = the piece's onset event (−1 none)
        bool analysisOn() const noexcept
        {
           #if OGRAN_TEST_HOOKS
            return ! hooks.analysisOff;
           #else
            return true;
           #endif
        }
        void spawn (const StreamOnset& o, std::int64_t pieceStart, std::int64_t wPiece) noexcept;
        void handleFreezeEdges() noexcept;                 // 2.4: tick-sampled freeze edges
        void applyTempoEffective() noexcept;               // 2.5a P50: effective Tempo density / size (R45)
        void tickSources() noexcept;                       // 2.5a P51: LFO ×4, env, macros (once per cell)
        void matrixTick() noexcept;                        // 2.5a P52–P55: slots, residuals, sparse offsets, latched + tick dests, telemetry
        void traceAt (int dest, std::int64_t at, double v, double aux = 0.0) noexcept
        {
           #if OGRAN_TEST_HOOKS
            if (hooks.traceDest.dest == dest)
                trace[static_cast<size_t> (traceCount++ % static_cast<std::uint64_t> (kTraceCapacity))] = { at, v, aux };
           #else
            juce::ignoreUnused (dest, at, v, aux);
           #endif
        }
        void forcedReleaseScan() noexcept;                 // 2.4 R25 engage scan
       #if OGRAN_TEST_HOOKS
        Spawner::Acquired nativeAcquireForTest() noexcept;
        void checkStealState (const Spawner::Acquired& acq) noexcept;
        void recordRetire (const Grain& g, int slot) noexcept
        {
            if (slot < 0 || slot >= kPoolLive)
                return;
            auto& sh = retireShadow[slot];
            for (int c = 0; c < 2; ++c)
            {
                sh.svfIc1[c] = g.svfIc1[c];
                sh.svfIc2[c] = g.svfIc2[c];
                sh.crushHeld[c] = g.crushHeld[c];
            }
            sh.crushHoldPhase = g.crushHoldPhase;
            sh.ringRe = g.ringRe;
            sh.ringIm = g.ringIm;
            sh.valid  = true;
        }
       #endif
        void renderSegment (int from, int to) noexcept;
        void mixOut (const float* dryL, const float* dryR, float* outL, float* outR,
                     int numOut, int len, int cellPos) noexcept;
        void snapRampsToParams() noexcept;
        void publishUi() noexcept;                          // 3.2 P116: the <= 60 Hz snapshot (copy only)

        // 2.2 grid helpers (audio thread except where noted).
        void copyCustomTable (bool blocking) noexcept;     // P11 (blocking only in prepare)
        void updateScaleState() noexcept;                   // active sets + pitch windows
        void computeStreamTargets (float (&gOut)[kMaxStreams], float (&audible)[kMaxStreams], float (&gFb)[kMaxStreams]) noexcept;
        void pushFbTapGains (bool snap) noexcept;          // 2.4 tap gains from the current ramps
        void updateChainCutoffs() noexcept;                // 2.4 fb HP / LP coefficients (tan) at ticks
        void writeReverbAtomics() noexcept;                // 2.4 reverb base values at ticks
        const PitchCollections::Table& tableForChoice (int choice) const noexcept;

        /** THE grain read site (R1). Every grain read goes through here, now and
            in later phases. The delay is formed from the integer part of readAbs
            and its fraction quantised to 2^-28, so W − ri is an exact integer
            (< 2^24) and the delay is exact: locateStart recovers the same
            (t, phi) for every W — i.e. for every host block size. Error <= 1.9e-9
            samples; the accumulator stays full double (no drift). */
        inline void readGrain (const Grain& g, std::int64_t writeHead, bool releasing, float& l, float& r) noexcept
        {
            const double ri = std::floor (g.readAbs);
            const double fq = std::round ((g.readAbs - ri) * 0x1p28) * 0x1p-28;   // may round to 1.0 — still exact
            double delay = static_cast<double> (writeHead - static_cast<std::int64_t> (ri)) - fq;
           #if OGRAN_TEST_HOOKS
            if (hooks.disableFracQuant)
                delay = static_cast<double> (writeHead) - g.readAbs;               // pre-R1 form (negative control)
            if (hooks.countR1Hazards && (writeHead % kCell) != 0 && g.readAbs > 64.0)   // real audio only
            {
                // Would the pre-R1 delay reconstruct a different (t, phi) here than at
                // the grid-aligned head a 32-sample host block would have used?
                const std::int64_t wg = ((writeHead + kCell - 1) / kCell) * kCell;
                // MipRing::locateStart + SincBank::kernel, level-0 form: (t, row, u).
                auto key = [&g] (std::int64_t w, std::int64_t& t, int& ip, float& u) noexcept
                {
                    const double d  = static_cast<double> (w) - g.readAbs;
                    const double dI = std::floor (d), df = d - dI;
                    t = w - static_cast<std::int64_t> (dI);
                    double phi = 0.0;
                    if (df > 0.0) { t -= 1; phi = 1.0 - df; }
                    const double x = phi * ouaricon::grain::blr::kPhases;
                    ip = std::clamp (static_cast<int> (x), 0, ouaricon::grain::blr::kPhases - 1);
                    u  = static_cast<float> (x - static_cast<double> (ip));
                };
                std::int64_t t1 = 0, t2 = 0; int i1 = 0, i2 = 0; float u1 = 0.0f, u2 = 0.0f;
                key (writeHead, t1, i1, u1);
                key (wg, t2, i2, u2);
                if (t1 != t2 || i1 != i2 || ! juce::exactlyEqual (u1, u2))
                    ++diag.r1Hazards;
            }
            // 2.4: a crossing by a RELEASING grain (forced release / fade slot)
            // is allowed and reflects; any other crossing is a spawner bug.
            if (delay > static_cast<double> (ringSamples)
                || (delay < static_cast<double> (Ring::margin (g.level)) - 1.0e-6 && ! releasing))
                ++diag.reachViolations;
           #else
            juce::ignoreUnused (releasing);
           #endif
            // R25 reflect at the head: a grain that reaches the (stopped) head
            // plays back out of it instead of sticking on the ring's margin clamp.
            // Value-continuous; exact under R1 (integer margin). Unreachable live
            // (the reach floor keeps delay >= margin), so live renders are bitwise
            // 2.3 — the same 1e-6 tolerance as the reach diag.
            const double mg = static_cast<double> (Ring::margin (g.level));
            if (delay < mg - 1.0e-6)
            {
                bool reflect = true;
               #if OGRAN_TEST_HOOKS
                ++diag.headReflections;
                if (g.frozenSpawn) ++diag.headReflectionsFrozenSpawn;
                reflect = ! hooks.freezeNoReflect;
               #endif
                if (reflect)
                    delay = 2.0 * mg - delay;
            }
            ring.readStereo (g.level, delay, g.set, l, r);
        }

        // ── parameters ──────────────────────────────────────────────────────
        ParamPointers ptrs;
        ScaleSource   scaleSrc;
        GridParams    gp;

        // ── 2.2: window bank, scale state, quotas ───────────────────────────
        WindowMorphBank bank;                              // ctor-built (message thread)
        const std::array<PitchCollections::Table, PitchCollections::kCount>& builtIns = PitchCollections::builtIns();
        PitchCollections::Table customCopy;                // the AUDIO thread's custom table (P11)
        std::uint32_t customCopyVersion = 0;
        ActiveSet  activeSets[kMaxStreams];
        PitchSel   pitchSel[kMaxStreams] {};
        AbsSet     absSets[kMaxStreams];               // 2.5b absolute follow sets (P71), follow streams only
        Quotas     quotas;
        double     coherence[kMaxStreams] {};

        // ── core ────────────────────────────────────────────────────────────
        Ring    ring;
        Pool    pool;
        Spawner spawner;
        StreamScheduler scheds[kMaxStreams];
        TempoTracker    tracker;                       // 2.5a held anchor (R39, R42, R44)
        double          tempoDiv[kMaxStreams] {};      // 2.5a div_eff per stream (quarters), set at ticks in Tempo mode
        // 2.5a sources (P51)
        Lfo          lfos[kLfos];
        EnvFollower  env;
        ExactRamp    macroRamp[kMacros];
        SourceValues srcVals;
        double       lfoRateHz[kLfos] {};              // the (modulated) rate of the PREVIOUS cell (R50.5)
        float        lfoRateOff[kLfos] {};             // the matrix offset on lfoN_rate of the previous cell
        // 2.5a matrix (P52–P54)
        ModMatrix    matrix;
        float        rawBase[kModDests] {};            // raw base of each routed latched / tick dest, read at the tick
        bool         streamRouted[kMaxStreams] {};     // any spawn-latched field of the stream routed this cell
        double       tempoSizeBaseMs[kMaxStreams] {};  // Tempo: the synced size before modulation (P50)
        // 2.5b analysis (P61): detector, decimator + MPM, D20 guard, f0 ring, sources.
        Analysis      analysis;
        std::uint64_t burstCount[kMaxStreams] {};      // Tempo bursts per stream (counter 2^63 | n), zeroed only in prepare()
        // Ramped-destination substitutes (R47): a settled cell prev → cur, used
        // while the dest is routed this cell or was routed in the previous one.
        ExactRamp    inGainMod, outGainMod, fbKnobMod, fbDiffMod, fbSatDMod;
        float        inGainNow = 1.0f, outGainNow = 1.0f, fbKnobNow = 0.0f, fbDiffNow = 0.0f, fbSatDNow = 1.0f;
        bool         useInMod = false, useOutMod = false, useFbKnobMod = false, useFbDiffMod = false, useFbSatMod = false;
        float        levelNow[kMaxStreams] {};         // the (modulated) level gain of this cell (mixer + tap)

        // ── fixed per-pass storage (kCell — never the host block) ───────────
        // Test builds bracket every per-pass array with canaries (PERF-03
        // oversize row: no ASan on macOS 26 — poison-guard pattern).
       #if OGRAN_TEST_HOOKS
        static constexpr std::uint64_t kCanary = 0xDEADBEEFCAFEF00Dull;
        std::uint64_t guard0[8] {};
       #endif
        float fbDelay[2][kFbDelay] {};                 // z^-32 feedback return (all zero in 2.1)
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard1[8] {};
       #endif
        float bus[kMaxStreams][2][kCell] {};           // stream buses
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard2[8] {};
       #endif
        StreamOnset streamOnsets[kMaxStreams][kOnsetCap] {};   // 2.5b: kCell + 1 (one Tempo burst)
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard3[8] {};
       #endif
        StreamOnset onsets[kMaxStreams * kOnsetCap] {};
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard4[8] {};
       #endif
        float wet[2][kCell] {};                        // 2.4: the wet bus of one piece (reverb → knee act here)
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard5[8] {};
       #endif
        float tap[2][kCell] {};                        // 2.4: the feedback tap of one piece (→ chain → fbDelay)
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard6[8] {};

        void armGuards() noexcept
        {
            for (auto* g : { guard0, guard1, guard2, guard3, guard4, guard5, guard6 })
                std::fill (g, g + 8, kCanary);
        }
       public:
        /** True when every canary around the per-pass arrays is intact. */
        bool guardsIntact() const noexcept
        {
            for (const auto* g : { guard0, guard1, guard2, guard3, guard4, guard5, guard6 })
                for (int i = 0; i < 8; ++i)
                    if (g[i] != kCanary)
                        return false;
            return true;
        }
       private:
       #endif

        // ── control grid ────────────────────────────────────────────────────
        ExactRamp inGainRamp, outGainRamp, mixRamp;
        ExactRamp levelRamp[kMaxStreams], panRamp[kMaxStreams], gOutRamp[kMaxStreams];
        ExactRamp audRamp[kMaxStreams];                // mute / solo audibility 0 ↔ 1 (P12, 10 ms)
        CellLerp  dryGain, wetGain;                    // mixGains (mixRamp) per grid point
        CellLerp  streamGainL[kMaxStreams], streamGainR[kMaxStreams];   // g_out · level · audible · pan
        // 2.4 feedback network (P34, P37)
        FeedbackChain chain;
        ExactRamp fbKnobRamp, fbDiffRamp, fbSatDRamp;   // 0.95·feedback, diffusion mix, d = 1 + 7·sat (20 ms)
        ExactRamp fbLowLogRamp, fbHighLogRamp;          // log2 of the fb cutoffs (exponential, 20 ms)
        ExactRamp gFbRamp[kMaxStreams];                 // g_fb = 1/(S1 + 1) (20 ms, like g_out)
        ExactRamp fbEnRamp[kMaxStreams];                // stream enable → tap (10 ms: no step from tails)
        CellLerp  fbTapCell[kMaxStreams];               // g_fb · level · audible · enable / max (1, Σ level·audible·enable)
        float     chainLowHz = -1.0f, chainHighHz = -1.0f;   // last cutoffs given to the chain
        // 2.4 output reverb (P40): plugin-owned atomics, fixed internals.
        ReverbUnit reverb;
        std::atomic<float> revEnabled { 1.0f }, revSize { 0.5f }, revDamp { 0.5f }, revPredelay { kRevPredelayMs },
                           revMix { 0.0f }, revMod { kRevMod }, revShimmer { kRevShimmer };
        juce::AudioBuffer<float> revView;              // refers to wet[] (setDataToReferTo: no allocation for 2 ch)

        // 2.5a telemetry (P55)
        std::array<std::atomic<float>, kModDests>     destTelemetry {};
        std::array<std::atomic<float>, kSrcTelemetry> srcTelemetry {};
        std::atomic<float> pitchNoteTel { 0.0f }, pitchConfTel { 0.0f };   // 2.5b (P68)
        // 3.2 buffer strip (P114–P116): written on the audio thread, never read by DSP.
        mutable TripleBuffer<UiSnapshot> uiSnap;       // 3 × ≈ 8.2 KB, members (no heap)
        PeakRing      peaks;                           // 2^18 entries, constructor-allocated (R120)
        float         panNow[kMaxStreams] {};          // the cell's (modulated) bus pan — display only
        std::uint64_t uiSeq = 0;
        std::int64_t  lastPublish = 0;
        std::int64_t  publishInterval = 800;           // round (fs / 60)
        bool          publishFirst = true;

       #if OGRAN_TEST_HOOKS
        // ── 2.3 test-only state (P28) ───────────────────────────────────────
        FxShadow retireShadow[kPoolLive] {};
        int nanKillSlots[kPoolLive + kPoolFade] {};   // slots NaN-killed in the current segment
        int numNanKills = 0;
        std::array<Grain, kPoolLive> stealSnap {};
       #endif

        // ── state ───────────────────────────────────────────────────────────
        double        fs           = 48000.0;
        double        spanSamples  = 8.0 * 48000.0;
        int           rampCells    = 30;   // 20 ms base (in/out gain, mix, g_out)
        int           mixerRampCells = 15; // 10 ms (level, pan, audibility — P12)
        int           ringSamples  = 0;
        std::int64_t  absCount     = 0;
        std::uint64_t activeSeed   = 0;
        std::uint64_t pendingSeed  = 0;
        bool          frozen       = false;   // the head is stopped (edges at ticks only)
        bool          freezePending = false;  // rising edge seen, head stops at freezeStopTick
        std::int64_t  freezeStopTick = 0;
        int           freezeDeferCells = 9;
        FreezeState   fz;                     // Write Arbiter (CaptureWriter.h)
        bool          prepared     = false;
    };
} // namespace ogran
