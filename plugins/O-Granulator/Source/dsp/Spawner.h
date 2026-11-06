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
#include "GrainFx.h"
#include "Quotas.h"
#include "ScaleQuantiser.h"
#include "WindowMorphBank.h"

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
        // 2.2
        float  gainL = 1.0f;  // spread pan × gain_rand; exactly (1, 1) with both at 0
        float  gainR = 1.0f;
        int    windowStep = 0;
        float  windowSkew = 0.0f;
        WindowMorphBank::Tilt tilt {};   // {0.5, 1, 1} at skew 0
        float  pitchCents = 0.0f;        // the latched scale pick / continuous random offset
        // 2.3 per-grain FX latch (Grain {} defaults when a stage is inactive)
        std::uint32_t fxActive = 0;
        int    svfMode = 0;
        float  svfK = 0.0f, svfA1 = 0.0f, svfA2 = 0.0f, svfA3 = 0.0f;
        float  svfMake = 1.0f;           // D10 (2.4)
        float  driveD = 1.0f, driveNorm = 1.0f;
        float  crushHoldInc = 0.0f, crushScale = 0.0f;
        float  ringRotRe = 1.0f, ringRotIm = 0.0f, ringMix = 0.0f;
        // telemetry (spawn log / census)
        float  fcHz = 0.0f;              // post-clamp cutoff (0 when the SVF is inactive)
        float  ringHz = 0.0f;            // post-clamp carrier (0 when the ring is inactive)
        bool   crushed = false;          // the Bernoulli outcome (false when crush == 0)
    };

    /** The scale-on pitch source for one stream at this grid (D4): set != nullptr
        ⇔ sN_scale ≠ Off; [lo, hi) = ActiveSet::window (pitch). */
    struct PitchSel
    {
        const ActiveSet* set = nullptr;
        int lo = 0, hi = 0;
    };

    /** Frozen spawner geometry (2.4, RESEARCH-2.4 R24 / R27, PLAN P39). on ⇔ the
        engine is frozen at this grid point; spanFrz = min (span, heldCaptured). */
    struct FrozenGeom
    {
        bool   on           = false;
        double spanFrz      = 0.0;
        double heldCaptured = 0.0;
        double headLead     = 0.0;     // inside the deferred stop: W0 − W_on(live) (> 0; the head still moves)
        bool   noMinSpray   = false;   // test hook freezeNoMinSpray (FZ3n): live formula + plain clamp
    };

    /** 2.5b absolute pitch follow (PLAN-2.5b P71 / P72): the pass-2 pitch of a
        follow stream whose f0 lookup hit. st = the interval (target − f0) in
        semitones, pitchCents = target − (f0 + pitch)·100 (the spawn log field). */
    struct FollowPitch
    {
        double st         = 0.0;
        float  pitchCents = 0.0f;
    };

    /** Victim-choice variants. Plugin builds always use the default (R8). */
    struct AcquirePolicy
    {
        bool nativeProgress = false;   // test hook: progress = age·invLen only (FUNC-03 negative control)
        bool noQuotaCeil    = false;   // test hook: overQuota = live > q (real), not > ceil (q) (R8 control)
    };

    class Spawner
    {
    public:
        using Pool = GrainPoolT;

        /** Message thread (Engine::prepare). */
        void prepare (double sampleRate, double killMs = kKillFadeMs) noexcept
        {
            killStep = static_cast<float> (1.0 / std::max (1.0, killMs * 0.001 * sampleRate));   // 2.6 P82: killMs 0 → a hard cut (test builds)
            std::fill (std::begin (pendingKill), std::end (pendingKill), std::uint8_t (0));
            anyPending = false;
        }

        //======================================================================
        /** Draws + formulas. Each randomisation at 0 draws nothing; reverse_prob is
            gated to (0, 1) before drawing, so 0 % / 100 % are exact.

            Fixed draw order (R10, append-only): Size, Pitch, Reverse, PosSpray,
            Pan, Gain — all here, BEFORE acquire(), so a refused or stolen spawn
            consumes exactly the draws a granted one does. (CounterRng is keyed by
            slot, so the order is documentation; the before-acquire rule is not.)
            2.5b: follow != nullptr (pass 2 of a follow stream whose lookup hit,
            P70 / P72) replaces the pitch step; nullptr → the 2.5a code verbatim. */
        static GrainPlan plan (const GridParams::Stream& sp, double spanSamples, double fs, int ringSamples,
                               std::uint64_t seed, int stream, std::uint64_t globalIndex, bool forceLevel0,
                               const PitchSel& pitchSel = {}, const FrozenGeom& fz = {},
                               const FollowPitch* follow = nullptr) noexcept
        {
            const ouaricon::grain::CounterRng::Draw draw (seed, kGrainStream + static_cast<std::uint64_t> (stream), globalIndex);
            auto uni = [&draw] (std::uint32_t slot) noexcept { return static_cast<double> (draw.nextFloat (slot)); };

            GrainPlan p;

            // Length: size · (1 + sizeRand · (2u − 1) · 0.5), clamped [5, 2000] ms.
            double sizeMs = static_cast<double> (sp.sizeMs);
            if (sp.sizeRand > 0.0f)
                sizeMs *= 1.0 + static_cast<double> (sp.sizeRand) * (2.0 * uni (Slot::Size) - 1.0) * 0.5;
            p.len = std::clamp (sizeMs, kMinSizeMs, kMaxSizeMs) * 0.001 * fs;

            // Pitch.
            //   scale Off: st = clamp (pitch + pitchRand · (2u − 1), ±24) — the 2.1
            //              expression verbatim (bitwise).
            //   scale on (D4): st = pitch + pick/100, pick uniform over the active
            //              degrees inside the window; no pick (pitch_rand 0, empty
            //              mask, no degree in span) → st = pitch exactly, NO draw.
            // rate = 2^(st/12) in double.
            double st = static_cast<double> (sp.pitch);
            if (follow != nullptr)
            {
                // 2.5b pass 2 (P72): the absolute-follow interval (target − f0) and
                // its log field; then the ±24 clamp and the rest of plan, unchanged.
                st = follow->st;
                p.pitchCents = follow->pitchCents;
            }
            else if (pitchSel.set == nullptr)
            {
                if (sp.pitchRand > 0.0f)
                {
                    st += static_cast<double> (sp.pitchRand) * (2.0 * uni (Slot::Pitch) - 1.0);
                    p.pitchCents = static_cast<float> (static_cast<double> (sp.pitchRand) * (2.0 * uni (Slot::Pitch) - 1.0) * 100.0);
                }
            }
            else if (sp.pitchRand > 0.0f && pitchSel.hi > pitchSel.lo)
            {
                p.pitchCents = pitchSel.set->pick (draw, pitchSel.lo, pitchSel.hi);
                st += static_cast<double> (p.pitchCents) / 100.0;
            }
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
            if (! fz.on)
            {
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
            }
            else
            {
                p.delay = frozenDelay (sp, p, margin, fz, uni);
            }

            // Stereo spread (R10): x = spread · (2u − 1), constant-power balance about
            // centre (the stream pan stays on the bus). Spread 0 → no draw, (1, 1).
            if (sp.spread > 0.0f)
            {
                const float px = static_cast<float> (static_cast<double> (sp.spread) * (2.0 * uni (Slot::Pan) - 1.0));
                const auto  g = panGains (px);
                p.gainL = g.a;
                p.gainR = g.b;
            }

            // Gain rand (D5): cut only, g = 10^(−gain_rand · u · 24 / 20); never
            // seen by the output law. gain_rand 0 → no draw.
            if (sp.gainRand > 0.0f)
            {
                const float g = static_cast<float> (std::pow (10.0, -static_cast<double> (sp.gainRand) * uni (Slot::Gain)
                                                                    * kGainRandDb / 20.0));
                p.gainL *= g;
                p.gainR *= g;
            }

            // Window morph step + skew tilt, latched (P13).
            p.windowStep = WindowMorphBank::stepFor (sp.window);
            p.windowSkew = sp.windowSkew;
            p.tilt       = ouaricon::grain::WindowLut::makeTilt (0.5f + 0.45f * sp.windowSkew);

            planFx (p, sp, fs, uni);
            return p;
        }

        /** Frozen position (R24 / R27). The head is stopped, so a forward grain's
            delay falls by r per sample: floor = margin + r·len + 1 (forward), and
            a reverse grain must not read past the captured span. Position is
            measured FROM the floor over A = max (span_frz − floorOff, r·len/2),
            with a minimum spray of ±½ grain of source (w >= r·len): PosSpray is
            drawn even at user spray 0 (CounterRng is keyed by slot — no other
            draw moves; live renders never come here).
                x = position·A + w·(u − ½),  D = margin + floorOff + reflect (x, A) */
        template <typename Uni>
        static double frozenDelay (const GridParams::Stream& sp, const GrainPlan& p, double margin,
                                   const FrozenGeom& fz, Uni&& uni) noexcept
        {
            const double rl   = p.rate * p.len;
            const double span = fz.spanFrz;
            const double pos  = static_cast<double> (sp.position), spray = static_cast<double> (sp.spray);
            double d = 0.0;
            if (fz.noMinSpray)
            {
                // FZ3n control: the live formula against the frozen span + the plain clamp.
                double x = pos * span;
                if (sp.spray > 0.0f)
                    x += spray * span * (uni (Slot::PosSpray) - 0.5);
                d = margin + reflectInto (x, span);
                d = p.dir > 0 ? std::max (d, margin + rl + 1.0) : std::max (d, margin + 1.0);
            }
            else
            {
                const double floorOff = p.dir > 0 ? rl + 1.0 : 0.0;
                const double A = std::max (span - floorOff, 0.5 * rl);
                const double w = std::min (std::max (spray * span, rl), A);
                const double x = pos * A + w * (uni (Slot::PosSpray) - 0.5);
                d = margin + floorOff + reflectInto (x, A);
            }
            if (p.dir < 0)
            {
                d = std::max (d, margin + 1.0);
                d = std::min (d, std::max (margin + 1.0, fz.heldCaptured - rl));   // D + r·len <= heldCaptured
            }
            // Deferred stop: anchored at the future W0 while the head is still
            // headLead samples short of it — never read ahead of the moving head.
            // Forward: head − read >= margin for every age up to the stop needs
            // D >= margin + r·(headLead + 1); reverse only needs it at age 0.
            if (fz.headLead > 0.0)
                d = std::max (d, p.dir > 0 ? margin + p.rate * (fz.headLead + 1.0) + 1.0 : margin + fz.headLead + 1.0);
            return d;
        }

        /** 2.3 per-grain FX latch (PLAN-2.3 P22–P24, ARCHITECTURE §9 order).
            Reserved draw slots FxCutoff / FxCrushProb / FxRing, each drawn only
            behind its gate, all BEFORE acquire() (called from plan()). A stage
            at its neutral value latches nothing: its fields keep the Grain {}
            defaults and its fxActive bit stays clear. */
        template <typename Uni>
        static void planFx (GrainPlan& p, const GridParams::Stream& sp, double fs, Uni&& uni) noexcept
        {
            // SVF: active for every type ≠ Off (also at the 20 kHz default, which
            // clamps to 0.45·fs — CONTEXT).
            if (sp.filterType != kSvfOff)
            {
                double fc = static_cast<double> (sp.filterCutoffHz);
                if (sp.filterRand > 0.0f)
                    fc = fx::randOct (fc, static_cast<double> (sp.filterRand), uni (Slot::FxCutoff));
                const auto s = fx::svfLatch (fc, static_cast<double> (sp.filterRes), fs);
                p.fxActive |= kFxSvf;
                p.svfMode = sp.filterType;
                p.svfK  = s.k;
                p.svfA1 = s.a1;
                p.svfA2 = s.a2;
                p.svfA3 = s.a3;
                p.svfMake = s.make;
                p.fcHz  = s.fcHz;
            }

            // Drive (D6 / P23): active iff d = 16·drive² >= 1e-4; drive 0 is the
            // bitwise bypass. No draw.
            const auto dl = fx::driveLatch (sp.drive);
            if (dl.active)
            {
                p.fxActive |= kFxDrive;
                p.driveD    = dl.d;
                p.driveNorm = dl.norm;
            }

            // Crush (D7 / P24): drawn only when crush > 0 and 0 < prob < 1, so
            // prob 0 % / 100 % are exact and draw nothing.
            if (sp.crush > 0.0f)
            {
                const double cp = static_cast<double> (sp.crushProb);
                p.crushed = cp >= 1.0 ? true : cp <= 0.0 ? false : uni (Slot::FxCrushProb) < cp;
                if (p.crushed)
                {
                    const auto cl = fx::crushLatch (sp.crush, fs);
                    p.fxActive    |= kFxCrush;
                    p.crushHoldInc = cl.inc;
                    p.crushScale   = cl.scale;
                }
            }

            // Ring (R14 / P21): carrier drawn only when ring_rand > 0; the carrier
            // starts at the Grain default (1, 0).
            if (sp.ringMix > 0.0f)
            {
                double f = static_cast<double> (sp.ringFreqHz);
                if (sp.ringRand > 0.0f)
                    f = fx::randOct (f, static_cast<double> (sp.ringRand), uni (Slot::FxRing));
                const auto rl = fx::ringLatch (f, fs);
                p.fxActive |= kFxRing;
                p.ringRotRe = rl.rotRe;
                p.ringRotIm = rl.rotIm;
                p.ringMix   = sp.ringMix;
                p.ringHz    = rl.hz;
            }
        }

        /** Writes the FX LATCH (never FX state) into a filled grain. fill() calls
            it; the fxSharedLatch test hook calls it again with a re-planned latch.
            No-bleed comes from GrainPool::activate()'s value-initialisation. */
        static void assignFxLatch (Grain& g, const GrainPlan& p) noexcept
        {
            g.fxActive     = p.fxActive;
            g.svfMode      = p.svfMode;
            g.svfK         = p.svfK;
            g.svfA1        = p.svfA1;
            g.svfA2        = p.svfA2;
            g.svfA3        = p.svfA3;
            g.svfMake      = p.svfMake;
            g.driveD       = p.driveD;
            g.driveNorm    = p.driveNorm;
            g.crushHoldInc = p.crushHoldInc;
            g.crushScale   = p.crushScale;
            g.ringRotRe    = p.ringRotRe;
            g.ringRotIm    = p.ringRotIm;
            g.ringMix      = p.ringMix;
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

        /** R3 cap (RESEARCH-2.1 §3) with stream-fair progress (RESEARCH-2.2 §3, R8):
              live_s     = live, non-killing grains of stream s (same scan)
              overQuota  = live_s > ceil (q_s)
              pri (g)    = 2·overQuota[g.stream] + (g.stream == spawning)       (integer)
              progress   = pri + age·invLen
              nAlive >= limit:
                  free live slot → victim (argmax progress over non-killing live grains)
                                   gets killStep = 1/(5 ms·fs); g = tryObtain()
                  no free slot   → g = obtain (progress')   (native steal)
              else: g = tryObtain() ?: obtain (progress')
              progress'(g) = (killing ? 3 : 0) + progress (g)

            Exactness: the argmax is LEXICOGRAPHIC on (pri, age·invLen) — the same
            order as the real-valued sum (age·invLen < 1), without the rounding
            of pri + x that could merge two candidates into a tie. progress' (a
            scalar for GrainPool::obtain) subtracts the minimum pri over the live
            grains (a common shift, order-preserving). With ONE stream every grain
            has the same pri, so both reduce exactly to the 2.1 victim (S0 / F7).

            The pool exposes grains read-only (getGrains), so a kill is RECORDED here
            and applied by the next render pass through the pool's own Grain&
            (applyPendingKill) — before that grain renders another sample. */
        Acquired acquire (Pool& pool, int grainLimit, const Quotas& quotas, int spawning,
                          AcquirePolicy policy = {}) noexcept
        {
            const Grain* const base = pool.getGrains().data();

            // Pass 1: occupancy (live[4], alive, free slot, killing).
            int live[kMaxStreams] {};
            int alive = 0, freeSlot = -1;
            bool anyKilling = false;
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
                ++live[std::clamp (base[i].stream, 0, kMaxStreams - 1)];
            }

            int pk[kMaxStreams] {};   // pri per stream
            if (! policy.nativeProgress)
                for (int s = 0; s < kMaxStreams; ++s)
                {
                    const bool over = policy.noQuotaCeil ? static_cast<double> (live[s]) > quotas.q[s]
                                                         : live[s] > quotas.qCeil[s];
                    pk[s] = 2 * (over ? 1 : 0) + (s == spawning ? 1 : 0);
                }
            auto priOf = [&pk] (const Grain& g) noexcept { return pk[std::clamp (g.stream, 0, kMaxStreams - 1)]; };

            // Pass 2: victim = lexicographic argmax (pri, age·invLen); ties: lowest slot.
            int victim = -1, bestPri = -1, minPri = 3;
            double best = -1.0;
            for (int i = 0; i < kPoolLive; ++i)
            {
                if (! pool.isActive (i))
                    continue;
                const int pr = priOf (base[i]);
                minPri = std::min (minPri, pr);
                if (isKilling (base[i], i))
                    continue;
                const double x = progress (base[i]);
                if (pr > bestPri || (pr == bestPri && x > best)) { bestPri = pr; best = x; victim = i; }
            }

            auto progressPrime = [this, base, &priOf, minPri] (const Grain& g) noexcept
            {
                const int k = (isKilling (g, static_cast<int> (&g - base)) ? 3 : 0) + (priOf (g) - minPri);
                return static_cast<double> (k) + progress (g);
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
            W_on is RING-domain (R23): the level-0 write count a sample-by-sample
            engine would see at the onset — W_piece + offset + 1 while live (==
            the absolute onset + 1 until the first freeze), the stopped head while
            frozen. readAbs = (W_on − D) + dir·rate·frac. */
        static void fill (Grain& g, const GrainPlan& p, int stream, double frac, std::int64_t W_on) noexcept
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
            g.readAbs    = (static_cast<double> (W_on) - p.delay) + g.step * frac;
            g.gainL      = p.gainL;
            g.gainR      = p.gainR;
            g.windowStep = p.windowStep;
            g.windowSkew = p.windowSkew;
            g.tiltT      = p.tilt.t;
            g.tiltA      = p.tilt.a;
            g.tiltB      = p.tilt.b;
            g.pitchCents = p.pitchCents;
            assignFxLatch (g, p);
            // killGain = 1, killStep = 0 and every FX STATE field from value-initialisation.
        }

        //======================================================================
        bool hasPendingKills() const noexcept { return anyPending; }

        /** 2.4 freeze engage (R25): record a 5 ms forced release for live slot idx,
            applied by the next render pass (the R3 kill path). */
        void requestKill (int idx) noexcept
        {
            if (idx >= 0 && idx < kPoolLive)
            {
                pendingKill[idx] = 1;
                anyPending = true;
            }
        }
        bool isPendingKill (int idx) const noexcept { return idx >= 0 && idx < kPoolLive && pendingKill[idx] != 0; }

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
