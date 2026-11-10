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

#include <algorithm>
#include <cstdint>
#include "GiConstants.h"
#include "VoiceGrain.h"
#include "VoiceQuotas.h"

// ============================================================================
// GiSpawner — grain acquisition on the (voice, stream) axis (RESEARCH-2.1
// §1.4, PLAN-2.1 T8, PLAN-2.2 T5). A re-derivation of the parent
// Spawner::acquire (82406fce Spawner.h:428-565) keyed on the PAIR (k, s):
// k = vg.voice, s = vg.g.stream.
//
//   live_kS    = live, non-killing grains of pair (k, s) (one scan of the 256 live slots)
//   pri (g)    = 4·stolenVoice (k) + 2·(live_kS > m_kS) + ((k, s) == spawning)
//                (m_kS = the P1 thinning integer; kQuotaUncapped never over)
//   With one stream every grain has s = 0, so pri is the 2.1 voice-axis key.
//   victim     = LEXICOGRAPHIC argmax on (pri, age·invLen) over non-killing
//                live grains, ties → the lowest pool slot
//   alive >= limit, free slot, victim → R3: the victim gets the 5 ms killStep
//                (applied by applyPendingKill before its next sample);
//                g = tryObtain()
//   alive >= limit, no free slot      → native steal obtain (progress')
//   else                              → tryObtain() ?: obtain (progress')
//   progress'  = (killing ? 8 : 0) + (pri − minPri) + age·invLen
//   D25        while alive > limit, each acquire kill-fades ONE more victim
//                (same order) so a lowered cap converges.
//
// Deviation (reported): the parent's killing bonus is 3 because its pri range
// is 0..3; with the stolen-voice term the range is 0..7, so the bonus is 8 to
// keep the parent invariant "progress' prefers a killing grain whenever one
// is live".
//
// Kill flags cover all 288 pool slots (swap kills may mark any slot).
// Audio thread only; no allocation.
// ============================================================================
namespace ogi
{
    struct AcquirePolicy
    {
        bool overQuotaOff = false;   // Q2 control: the 2·overQuota term zeroed
        bool d25Off       = false;   // Q3 control: no over-cap convergence kill
    };

    class GiSpawner
    {
    public:
        static constexpr int kSlots = GiPool::kNumSlots;

        /** Message / host thread (Engine::prepare). */
        void prepare (double sampleRate, double killMs = kKillFadeMs) noexcept
        {
            killStep = static_cast<float> (1.0 / std::max (1.0, killMs * 0.001 * sampleRate));
            clearAll();
        }

        void clearAll() noexcept
        {
            std::fill (std::begin (pendingKill), std::end (pendingKill), std::uint8_t (0));
            anyPending = false;
        }

        struct Acquired
        {
            VoiceGrain* grain      = nullptr;
            int         slot       = 0;
            int         aliveAfter = 0;
            bool        killed     = false;   // R3 kill path (a victim got a 5 ms release)
            bool        stolen     = false;   // native fade-on-steal
            bool        capKill    = false;   // D25 extra kill
        };

        Acquired acquire (GiPool& pool, int grainLimit, const int (&m)[kVoiceSlots][kGiStreams],
                          const bool (&stolenVoice)[kVoiceSlots], int spawnK, int spawnS,
                          AcquirePolicy policy = {}) noexcept
        {
            const VoiceGrain* const base = pool.getGrains().data();

            // Pass 1: occupancy.
            int live[kVoiceSlots][kGiStreams] {};
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
                ++live[voiceOf (base[i])][streamOf (base[i])];
            }

            int pk[kVoiceSlots][kGiStreams] {};
            for (int k = 0; k < kVoiceSlots; ++k)
                for (int s = 0; s < kGiStreams; ++s)
                {
                    const bool over = ! policy.overQuotaOff && m[k][s] < kQuotaUncapped && live[k][s] > m[k][s];
                    pk[k][s] = 4 * (stolenVoice[k] ? 1 : 0) + 2 * (over ? 1 : 0) + ((k == spawnK && s == spawnS) ? 1 : 0);
                }

            // Pass 2: victim = lexicographic argmax (pri, age·invLen); ties: lowest slot.
            int victim = -1, bestPri = -1, minPri = 7;
            double best = -1.0;
            for (int i = 0; i < kPoolLive; ++i)
            {
                if (! pool.isActive (i))
                    continue;
                const int pr = priOf (pk, base[i]);
                minPri = std::min (minPri, pr);
                if (isKilling (base[i], i))
                    continue;
                const double x = progress (base[i]);
                if (pr > bestPri || (pr == bestPri && x > best)) { bestPri = pr; best = x; victim = i; }
            }

            auto progressPrime = [this, base, &pk, minPri] (const VoiceGrain& g) noexcept
            {
                const int idx = static_cast<int> (&g - base);
                const int k = (isKilling (g, idx) ? 8 : 0) + (priOf (pk, g) - minPri);
                return static_cast<double> (k) + progress (g);
            };

            Acquired a;
            if (alive >= grainLimit && freeSlot >= 0 && victim >= 0)
            {
                pendingKill[victim] = 1;
                anyPending   = true;
                a.killed     = true;
                a.grain      = pool.tryObtain();          // non-null: freeSlot exists
                a.aliveAfter = alive;
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
                    a.aliveAfter = anyKilling ? alive + 1 : alive;
                    // Every live slot busy and the steal took a grain that was
                    // already killing: the new grain is a net +1 alive. At or
                    // over the cap that must be paid for like the R3 path, or the
                    // D25 extra kill only breaks even and a lowered grain_limit
                    // never converges while the pool stays full (Q3, 2.1 execute).
                    if (anyKilling && alive >= grainLimit && victim >= 0)
                    {
                        pendingKill[victim] = 1;
                        anyPending   = true;
                        a.killed     = true;
                        a.aliveAfter = alive;
                    }
                }
            }

            a.slot = static_cast<int> (a.grain - base);
            if (a.slot >= 0 && a.slot < kSlots)
                pendingKill[a.slot] = 0;                  // the slot now holds the NEW grain

            if (alive > grainLimit && ! policy.d25Off)
            {
                int extra = -1, ePri = -1;
                double eBest = -1.0;
                for (int i = 0; i < kPoolLive; ++i)
                {
                    if (i == a.slot || ! pool.isActive (i) || isKilling (base[i], i))
                        continue;
                    const int pr = priOf (pk, base[i]);
                    const double x = progress (base[i]);
                    if (pr > ePri || (pr == ePri && x > eBest)) { ePri = pr; eBest = x; extra = i; }
                }
                if (extra >= 0)
                {
                    pendingKill[extra] = 1;
                    anyPending   = true;
                    a.capKill    = true;
                    a.aliveAfter -= 1;
                }
            }
            return a;
        }

        /** Records a 5 ms forced release for pool slot idx (swap kills). */
        void requestKill (int idx) noexcept
        {
            if (idx >= 0 && idx < kSlots)
            {
                pendingKill[idx] = 1;
                anyPending = true;
            }
        }

        bool isPendingKill (int idx) const noexcept { return idx >= 0 && idx < kSlots && pendingKill[idx] != 0; }
        void clearPendingKill (int idx) noexcept    { if (idx >= 0 && idx < kSlots) pendingKill[idx] = 0; }
        bool hasPendingKills() const noexcept       { return anyPending; }
        void pendingKillsApplied() noexcept         { anyPending = false; }

        /** Render-pass hook: applies a recorded kill before the grain's next sample. */
        void applyPendingKill (VoiceGrain& g, const VoiceGrain* base) noexcept
        {
            const auto idx = &g - base;
            if (idx >= 0 && idx < kSlots && pendingKill[idx] != 0)
            {
                if (! (g.g.killStep > 0.0f))
                    g.g.killStep = killStep;
               #if OGI_TEST_HOOKS
                if (testKillStepOverride > 0.0f)
                    g.g.killStep = testKillStepOverride;   // CK6n: swapKillHard (a 1-sample cut)
               #endif
                pendingKill[idx] = 0;
            }
        }

       #if OGI_TEST_HOOKS
        float testKillStepOverride = 0.0f;   // PLAN-2.6 PD23 swapKillHard (0 = the 5 ms law)
       #endif

        /** Non-killing live grains of the 256 live slots (hooks / gates). */
        int countAlive (const GiPool& pool) const noexcept
        {
            const VoiceGrain* const base = pool.getGrains().data();
            int alive = 0;
            for (int i = 0; i < kPoolLive; ++i)
                if (pool.isActive (i) && ! isKilling (base[i], i))
                    ++alive;
            return alive;
        }

        /** Per-voice non-killing live counts, summed over streams (hooks / gates). */
        void countAlivePerVoice (const GiPool& pool, int (&out)[kVoiceSlots]) const noexcept
        {
            for (auto& x : out) x = 0;
            const VoiceGrain* const base = pool.getGrains().data();
            for (int i = 0; i < kPoolLive; ++i)
                if (pool.isActive (i) && ! isKilling (base[i], i))
                    ++out[voiceOf (base[i])];
        }

        /** Per-(voice, stream) non-killing live counts (hooks / gates). */
        void countAlivePerPair (const GiPool& pool, int (&out)[kVoiceSlots][kGiStreams]) const noexcept
        {
            for (auto& row : out)
                for (auto& x : row)
                    x = 0;
            const VoiceGrain* const base = pool.getGrains().data();
            for (int i = 0; i < kPoolLive; ++i)
                if (pool.isActive (i) && ! isKilling (base[i], i))
                    ++out[voiceOf (base[i])][streamOf (base[i])];
        }

        float getKillStep() const noexcept { return killStep; }

        bool isKilling (const VoiceGrain& g, int idx) const noexcept
        {
            return g.g.killStep > 0.0f || (idx >= 0 && idx < kSlots && pendingKill[idx] != 0);
        }

    private:
        static int voiceOf (const VoiceGrain& g) noexcept  { return std::clamp (g.voice, 0, kVoiceSlots - 1); }
        static int streamOf (const VoiceGrain& g) noexcept { return std::clamp (g.g.stream, 0, kGiStreams - 1); }
        static int priOf (const int (&pk)[kVoiceSlots][kGiStreams], const VoiceGrain& g) noexcept
        {
            return pk[voiceOf (g)][streamOf (g)];
        }
        static double progress (const VoiceGrain& g) noexcept { return static_cast<double> (g.g.age) * g.g.invLen; }

        std::uint8_t pendingKill[kSlots] {};
        bool  anyPending = false;
        float killStep   = 0.0f;
    };
} // namespace ogi
