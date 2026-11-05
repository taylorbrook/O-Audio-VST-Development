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
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "CaptureWriter.h"
#include "ControlGrid.h"
#include "EngineConstants.h"
#include "Grain.h"
#include "OutputLaw.h"
#include "Spawner.h"
#include "StreamScheduler.h"

// ============================================================================
// O-Granulator pass engine (Stage 2, Phase 2.1).
// ARCHITECTURE Processing Chain (per-pass order, normative), §1–7, §10, §14, §16;
// RESEARCH-2.1 R1–R4; PLAN-2.1 T7.
//
// The host block is cut at every ABSOLUTE multiple of kCell (absCount, int64).
// Each piece [a, b) (<= 32 samples, inside one cell) runs:
//   1. grid tick if a % 32 == 0: latch GridParams, retarget ramps, g_out
//   2. (analysis — 2.5)
//   3. write the piece into the ring (input·inGain + fb[n − 32])
//   4. schedule enabled streams → onsets, merged by (offset, stream)
//   5. render grain-major, split at onset offsets, spawn at each split
//   6. stream mixer: bus × g_out × level × pan → wet   (feedback tap — 2.4)
//   7. (feedback chain — 2.4; fbDelay stays all zero in 2.1)
//   8. (reverb, wet safety — 2.6) dry/wet + output gain → out
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

        Engine() = default;

        /** Message thread, once (processor constructor). */
        void bind (const ParamPointers& p) noexcept { ptrs = p; }

        /** Message thread. Allocates the ring, resets every clock and ramp
            (ramps snapped to the current parameter values). */
        void prepare (double sampleRate, int maxBlockSize);

        bool isPrepared() const noexcept { return prepared; }

        /** Audio thread. in/out are the BUS channels (numIn, numOut in {1, 2});
            in and out may alias (JUCE in-place buffers). `seed` is adopted at the
            next grid point. */
        void process (const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, std::uint64_t seed) noexcept;

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
        };

        /** One spawn. The grain's readAbs at age k is anchor + dir·rate·(k + frac)
            (exact reference; the engine accumulates readAbs += dir·rate per sample). */
        struct SpawnRecord
        {
            std::int64_t  onsetSample = 0;   // absolute sample index of the onset
            std::int64_t  writeHead   = 0;   // W_on = onsetSample + 1
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
        };

        static constexpr int kSpawnLogSize = 4096;

        /** Plain counters, written on the audio thread, read by the harness
            BETWEEN blocks. Never feeds a gain law. */
        struct Diag
        {
            std::int64_t reachViolations = 0;   // reads with delay > kRingSamples or < margin(level)
            std::int64_t kills           = 0;   // R3 kill path taken
            std::int64_t maxAlive        = 0;   // max live non-killing grains right after any spawn
            std::int64_t spawns          = 0;
            std::int64_t steals          = 0;   // native pool steals
            std::int64_t r1Hazards       = 0;   // see TestHooks::countR1Hazards
            std::uint64_t logCount       = 0;   // total spawns logged; entry k at log[k % kSpawnLogSize]
            std::array<SpawnRecord, kSpawnLogSize> log {};

            void reset() noexcept
            {
                reachViolations = kills = maxAlive = spawns = steals = r1Hazards = 0;
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
        const GridParams& getGridParams() const noexcept      { return gp; }
       #endif

    private:
        void gridTick() noexcept;
        int  scheduleStreams (int len) noexcept;
        void spawn (const StreamOnset& o, std::int64_t pieceStart) noexcept;
       #if OGRAN_TEST_HOOKS
        Spawner::Acquired nativeAcquireForTest() noexcept;
       #endif
        void renderSegment (int from, int to) noexcept;
        void mixOut (const float* dryL, const float* dryR, float* outL, float* outR,
                     int numOut, int len, int cellPos) noexcept;
        void snapRampsToParams() noexcept;

        /** THE grain read site (R1). Every grain read goes through here, now and
            in later phases. The delay is formed from the integer part of readAbs
            and its fraction quantised to 2^-28, so W − ri is an exact integer
            (< 2^24) and the delay is exact: locateStart recovers the same
            (t, phi) for every W — i.e. for every host block size. Error <= 1.9e-9
            samples; the accumulator stays full double (no drift). */
        inline void readGrain (const Grain& g, std::int64_t writeHead, float& l, float& r) noexcept
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
            if (delay > static_cast<double> (ringSamples)
                || delay < static_cast<double> (Ring::margin (g.level)) - 1.0e-6)
                ++diag.reachViolations;
           #endif
            ring.readStereo (g.level, delay, g.set, l, r);
        }

        // ── parameters ──────────────────────────────────────────────────────
        ParamPointers ptrs;
        GridParams    gp;

        // ── core ────────────────────────────────────────────────────────────
        Ring    ring;
        Pool    pool;
        Spawner spawner;
        StreamScheduler scheds[kMaxStreams];

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
        StreamOnset streamOnsets[kMaxStreams][kCell] {};
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard3[8] {};
       #endif
        StreamOnset onsets[kMaxStreams * kCell] {};
       #if OGRAN_TEST_HOOKS
        std::uint64_t guard4[8] {};

        void armGuards() noexcept
        {
            for (auto* g : { guard0, guard1, guard2, guard3, guard4 })
                std::fill (g, g + 8, kCanary);
        }
       public:
        /** True when every canary around the per-pass arrays is intact. */
        bool guardsIntact() const noexcept
        {
            for (const auto* g : { guard0, guard1, guard2, guard3, guard4 })
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
        CellLerp  dryGain, wetGain;                    // mixGains (mixRamp) per grid point
        CellLerp  streamGainL[kMaxStreams], streamGainR[kMaxStreams];   // g_out · level · pan

        // ── state ───────────────────────────────────────────────────────────
        double        fs           = 48000.0;
        double        spanSamples  = 8.0 * 48000.0;
        int           rampCells    = 30;
        int           ringSamples  = 0;
        std::int64_t  absCount     = 0;
        std::uint64_t activeSeed   = 0;
        std::uint64_t pendingSeed  = 0;
        bool          prepared     = false;
    };
} // namespace ogran
