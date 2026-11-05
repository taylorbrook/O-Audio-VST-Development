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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "CounterRng.h"     // grain-engine 1.1.0
#include "GrainPool.h"      // grain-engine 1.1.0
#include "ControlGrid.h"
#include "EngineConstants.h"
#include "Grain.h"

// ============================================================================
// Grain spawner (ARCHITECTURE §5–6, RESEARCH-2.1 §2, §3, §8, PLAN-2.1 T6).
//
// 1. plan():    every draw and every latched value, BEFORE any slot is touched
//               (draw before obtain: a refused or stolen spawn consumes exactly
//               the draws a granted one does). Pure function of (params, seed,
//               stream, globalIndex).
// 2. acquire(): the slot, through the R3 grain_limit cap.
// 3. fill():    writes the latched grain into the slot.
//
// Position (delay D behind the head, level-0 samples):
//     D0 = margin(level) + reflect (pos·span + spray·span·(u − ½), [0, span])
// Reach floor (P1 — the floor wins over the span; it bounds VALIDITY, the span
// only bounds the distribution):
//     forward: D >= margin + max (0, (r − 1)·len) + 1
//              (delay at age k is D − (r−1)k − r·frac > D − (r−1)·len − 1 >= margin)
//     reverse: D >= margin + 1  and  D + (r + 1)·len <= kRingSamples (backstop;
//              never binds under D2: D <= margin + 30 s, (r+1)·len <= 10 s)
// The floor derives from PARAMETERS (r, len), never from the pass or host block.
// ============================================================================
namespace ogran
{
    using GrainPoolT = ouaricon::grain::GrainPool<Grain, kPoolLive, kPoolFade>;

    /** Single reflection into [0, span] (|spray·span/2| <= span/2 keeps x in
        [−span/2, 3span/2], so one fold suffices), then a safety clamp. */
    inline double reflectInto (double x, double span) noexcept
    {
        if (x < 0.0)  x = -x;
        if (x > span) x = 2.0 * span - x;
        return std::clamp (x, 0.0, span);
    }

    /** Everything a grain latches at spawn. */
    struct GrainPlan
    {
        double len   = 0.0;   // output samples
        double rate  = 1.0;
        int    dir   = 1;
        int    level = 0;
        int    set   = 0;
        double delay = 0.0;   // D, level-0 samples behind the head at the onset
    };

    class Spawner
    {
    public:
        using Pool = GrainPoolT;

        /** Message thread (Engine::prepare). */
        void prepare (double sampleRate) noexcept
        {
            killStep = static_cast<float> (1.0 / std::max (1.0, kKillFadeMs * 0.001 * sampleRate));
            std::fill (std::begin (pendingKill), std::end (pendingKill), std::uint8_t (0));
            anyPending = false;
        }

        //======================================================================
        /** Draws + formulas. Each randomisation at 0 draws nothing; reverse_prob is
            gated to (0, 1) before drawing, so 0 % / 100 % are exact. */
        static GrainPlan plan (const GridParams::Stream& sp, double spanSamples, double fs, int ringSamples,
                               std::uint64_t seed, int stream, std::uint64_t globalIndex, bool forceLevel0) noexcept
        {
            const ouaricon::grain::CounterRng::Draw draw (seed, kGrainStream + static_cast<std::uint64_t> (stream), globalIndex);
            auto uni = [&draw] (std::uint32_t slot) noexcept { return static_cast<double> (draw.nextFloat (slot)); };

            GrainPlan p;

            // Length: size · (1 + sizeRand · (2u − 1) · 0.5), clamped [5, 2000] ms.
            double sizeMs = static_cast<double> (sp.sizeMs);
            if (sp.sizeRand > 0.0f)
                sizeMs *= 1.0 + static_cast<double> (sp.sizeRand) * (2.0 * uni (Slot::Size) - 1.0) * 0.5;
            p.len = std::clamp (sizeMs, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;

            // Pitch: st = clamp (pitch + pitchRand · (2u − 1), ±24); rate = 2^(st/12) in double.
            double st = static_cast<double> (sp.pitch);
            if (sp.pitchRand > 0.0f)
                st += static_cast<double> (sp.pitchRand) * (2.0 * uni (Slot::Pitch) - 1.0);
            st = std::clamp (st, -kMaxSemis, kMaxSemis);
            p.rate = std::exp2 (st / 12.0);

            // Direction.
            const double rp = static_cast<double> (sp.reverseProb);
            if (rp >= 1.0)
                p.dir = -1;
            else if (rp > 0.0)
                p.dir = uni (Slot::Reverse) < rp ? -1 : 1;

            // Band-limited reader latch (rate is constant per grain).
            p.level = forceLevel0 ? 0 : ouaricon::grain::latchLevel (p.rate, kMaxLevel);
            p.set   = ouaricon::grain::SincBank::setFor (std::ldexp (p.rate, -p.level));

            // Position.
            const double margin = static_cast<double> (Ring::margin (p.level));
            double x = static_cast<double> (sp.position) * spanSamples;
            if (sp.spray > 0.0f)
                x += static_cast<double> (sp.spray) * spanSamples * (uni (Slot::PosSpray) - 0.5);
            double d = margin + reflectInto (x, spanSamples);

            // Reach floor (P1).
            if (p.dir > 0)
            {
                d = std::max (d, margin + std::max (0.0, (p.rate - 1.0) * p.len) + 1.0);
            }
            else
            {
                d = std::max (d, margin + 1.0);
                const double maxD = static_cast<double> (ringSamples) - (p.rate + 1.0) * p.len;
                jassert (d <= maxD);   // never binds under D2 (RESEARCH §1)
                d = std::min (d, maxD);
            }
            p.delay = d;
            return p;
        }

        //======================================================================
        /** Result of acquire(). aliveAfter = live, non-killing grains after this spawn. */
        struct Acquired
        {
            Grain* grain      = nullptr;
            int    slot       = 0;
            int    aliveAfter = 0;
            bool   killed     = false;   // R3 kill path taken (a victim got a 5 ms release)
            bool   stolen     = false;   // native fade-on-steal (pool.obtain stole a slot)
        };

        /** R3 cap (RESEARCH §3), exactly:
              nAlive = live slots with killStep == 0 (pending kills count as killing)
              nAlive >= limit:
                  free live slot → victim (argmax progress over non-killing live grains)
                                   gets killStep = 1/(5 ms·fs); g = tryObtain()
                  no free slot   → g = obtain (progress')   (native steal)
              else: g = tryObtain() ?: obtain (progress')
              progress'(g) = (killing ? 3 : 0) + age·invLen
            At grain_limit 128 this reduces to the pool's native behaviour.

            The pool exposes grains read-only (getGrains), so a kill is RECORDED here
            and applied by the next render pass through the pool's own Grain&
            (applyPendingKill) — before that grain renders another sample. */
        Acquired acquire (Pool& pool, int grainLimit) noexcept
        {
            const Grain* const base = pool.getGrains().data();

            int alive = 0, freeSlot = -1, victim = -1;
            bool anyKilling = false;
            double best = -1.0;
            for (int i = 0; i < kPoolLive; ++i)
            {
                if (! pool.isActive (i))
                {
                    if (freeSlot < 0) freeSlot = i;
                    continue;
                }
                if (isKilling (base[i], i))
                {
                    anyKilling = true;
                    continue;
                }
                ++alive;
                const double pr = progress (base[i]);
                if (pr > best) { best = pr; victim = i; }   // ties: lowest slot (as GrainPool)
            }

            auto progressPrime = [this, base] (const Grain& g) noexcept
            {
                return (isKilling (g, static_cast<int> (&g - base)) ? 3.0 : 0.0) + progress (g);
            };

            Acquired a;
            if (alive >= grainLimit && freeSlot >= 0 && victim >= 0)
            {
                pendingKill[victim] = 1;
                anyPending   = true;
                a.killed     = true;
                a.grain      = pool.tryObtain();          // non-null: freeSlot exists
                a.aliveAfter = alive;                     // one killed, one added
            }
            else
            {
                a.grain = (alive >= grainLimit) ? nullptr : pool.tryObtain();
                if (a.grain != nullptr)
                {
                    a.aliveAfter = alive + 1;
                }
                else
                {
                    a.grain      = &pool.obtain (progressPrime);
                    a.stolen     = true;
                    // progress' prefers a killing grain (>= 3) whenever one is live.
                    a.aliveAfter = anyKilling ? alive + 1 : alive;
                }
            }

            jassert (a.grain != nullptr);
            a.slot = static_cast<int> (a.grain - base);
            if (a.slot >= 0 && a.slot < kPoolLive)
                pendingKill[a.slot] = 0;                  // the slot now holds the NEW grain
            return a;
        }

        /** Writes the plan into the freshly value-initialised slot.
            writeHeadAtOnset = absolute index of the onset sample + 1 (the head a
            sample-by-sample engine would see). readAbs = (W_on − D) + dir·rate·frac. */
        static void fill (Grain& g, const GrainPlan& p, int stream, double frac, std::int64_t writeHeadAtOnset) noexcept
        {
            g.stream     = stream;
            g.level      = p.level;
            g.set        = p.set;
            g.dir        = p.dir;
            g.age        = 0;
            g.rate       = p.rate;
            g.step       = static_cast<double> (p.dir) * p.rate;
            g.frac       = frac;
            g.len        = p.len;
            g.invLen     = 1.0 / p.len;
            g.startDelay = p.delay;
            g.readAbs    = (static_cast<double> (writeHeadAtOnset) - p.delay) + g.step * frac;
            // gainL/R = 1, killGain = 1, killStep = 0 from value-initialisation.
        }

        //======================================================================
        bool hasPendingKills() const noexcept { return anyPending; }

        /** Render-pass hook: applies a recorded kill to its grain (live slots only). */
        void applyPendingKill (Grain& g, const Grain* base) noexcept
        {
            const auto idx = &g - base;
            if (idx >= 0 && idx < kPoolLive && pendingKill[idx] != 0)
            {
                g.killStep = killStep;
                pendingKill[idx] = 0;
            }
        }

        /** After a render pass has visited every active slot. */
        void pendingKillsApplied() noexcept { anyPending = false; }

    private:
        static double progress (const Grain& g) noexcept { return static_cast<double> (g.age) * g.invLen; }

        bool isKilling (const Grain& g, int idx) const noexcept
        {
            return g.killStep > 0.0f || (idx >= 0 && idx < kPoolLive && pendingKill[idx] != 0);
        }

        std::uint8_t pendingKill[kPoolLive] {};
        bool  anyPending = false;
        float killStep   = 0.0f;
    };
} // namespace ogran
