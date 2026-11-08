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

#include <juce_core/juce_core.h>   // juce::SpinLock
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include "CounterRng.h"            // grain-engine 1.1.0
#include "PitchCollections.h"      // pitch-collections 1.0.0 (unmodified — D1)
#include "EngineConstants.h"

// ============================================================================
// Scale quantiser (CONTEXT-2.2 D4 — GrainScatter 3.x parity; RESEARCH-2.2 R9;
// PLAN-2.2 T6).
//
// With sN_scale ≠ Off a grain's pitch offset is a UNIFORM pick among the
// active degrees inside ±pitch_rand, added to the UNSNAPPED base pitch:
//     st = pitch + pick/100, clamp ±24 (a safety no-op)
// Empty mask, no active degree in span, or pitch_rand 0 → offset 0, NO draw.
//
// Active set (per stream, fixed array, audio thread, no allocation):
//   - tile deg_d + k·period for every k landing inside [−span, +span]
//     (span = pitch_rand·100 ¢; degree 0 at k = 0 is the unison);
//   - test mask bit d with the SOURCE index d (R9: 7-Limit Just is unsorted at
//     source — sorting the table first would misalign the bits);
//   - then insertion-sort the ACTIVE array (the module stays untouched).
//   Rebuild key: (choice, mask, spanCents, customVersion). Base pitch is NOT a
//   key (it becomes a mod destination in 2.5): window (pitch) restricts the
//   pick range with two binary searches instead.
//
// Index traps (R9): sN_scale 0 = Off; the table is
// GranulatorParamIds::scaleChoiceToId (choice) (spec 1…5 → gs Id 0…4, 6…31 →
// 6…31); choice 31 = Custom reads the ENGINE's copy of the custom table. The
// mask is keyed by SPEC index and 1-BASED stream: (s + 1 − 1)·32 + choice.
// ============================================================================
namespace ogi
{
    namespace PitchCollections = ouaricon::tuning::PitchCollections;

    inline constexpr int kNumScaleChoices = 32;   // sN_scale: 0 Off … 31 Custom

    /** sN_scale choice → PitchCollections::Id (−1 = Off). Mirrors
        GranulatorParamIds::scaleChoiceToId (static_assert'd equal in the processor). */
    inline constexpr int scaleChoiceToId (int c) noexcept { return c <= 0 ? -1 : (c <= 5 ? c - 1 : c); }

    /** Degree-mask array index for engine stream s (0-based) × choice 1…31. */
    inline constexpr int scaleMaskIndex (int engineStream, int choice) noexcept
    {
        return engineStream * kNumScaleChoices + choice;
    }

    /** The processor's scale state, bound once (bind(), message thread). The
        audio thread loads masks relaxed and copies the custom table through a
        NON-BLOCKING try-lock when the version changes (P11). */
    struct ScaleSource
    {
        const std::atomic<std::uint64_t>* masks   = nullptr;   // [kMaxStreams × kNumScaleChoices]
        juce::SpinLock*                   lock    = nullptr;   // guards *customTable
        const PitchCollections::Table*    customTable   = nullptr;
        const std::atomic<std::uint32_t>* customVersion = nullptr;
    };

    class ActiveSet
    {
    public:
        /** Grid tick. Returns true when it rebuilt. Bounded: ≤ (2·span/period + 2)
            × 64 iterations, truncated safely at kMaxActive (outward from unison). */
        bool rebuildIfNeeded (int choice, std::uint64_t mask, float spanCents, std::uint32_t customVersion,
                              const PitchCollections::Table& table) noexcept
        {
            if (valid && choice == keyChoice && mask == keyMask
                && juce::exactlyEqual (spanCents, keySpan) && customVersion == keyVersion)
                return false;

            valid      = true;
            keyChoice  = choice;
            keyMask    = mask;
            keySpan    = spanCents;
            keyVersion = customVersion;

            count     = 0;
            truncated = false;
            const int nDeg = std::clamp (table.count, 0, PitchCollections::kMaxPitches);
            unisonActive = nDeg > 0 && (mask & 1u) != 0;

            const std::uint64_t used = nDeg >= 64 ? ~std::uint64_t (0) : ((std::uint64_t (1) << nDeg) - 1);
            const double span   = std::max (0.0, static_cast<double> (spanCents));
            const double period = static_cast<double> (table.period);
            if ((mask & used) != 0 && period > 0.0)
            {
                // Outward from k = 0 (0, +1, −1, +2, …) so a truncation keeps the
                // degrees nearest unison.
                const int kMax = static_cast<int> (std::ceil (span / period)) + 1;
                // ! truncated (2.6, the AbsSet form): with count < kMaxActive here a cap that
                // landed exactly on a period boundary (512 = 8 × 64 degrees) ended the loops
                // before the inner check could flag it — same members, wasTruncated() wrong.
                for (int a = 0; a <= kMax && ! truncated; ++a)
                    for (int sgn = 0; sgn < (a == 0 ? 1 : 2) && ! truncated; ++sgn)
                    {
                        const int k = sgn == 0 ? a : -a;
                        for (int d = 0; d < nDeg; ++d)
                        {
                            if ((mask >> d & 1u) == 0)
                                continue;
                            const double c = static_cast<double> (table.cents[static_cast<size_t> (d)])
                                           + static_cast<double> (k) * period;
                            if (c < -span || c > span)
                                continue;
                            if (count >= kMaxActive) { truncated = true; break; }
                            cents[count++] = static_cast<float> (c);
                        }
                    }
            }

            // Insertion sort of the ACTIVE array (R9) — rebuild only, ≤ 512 entries.
            for (int i = 1; i < count; ++i)
            {
                const float v = cents[i];
                int j = i;
                for (; j > 0 && cents[j - 1] > v; --j)
                    cents[j] = cents[j - 1];
                cents[j] = v;
            }
            return true;
        }

        /** Pick range [lo, hi) for base pitch p (st): members inside
            [max (−span, −2400 − 100p), min (span, 2400 − 100p)]. */
        void window (double pitchSt, double spanCents, int& lo, int& hi) const noexcept
        {
            const float a = static_cast<float> (std::max (-spanCents, -2400.0 - 100.0 * pitchSt));
            const float b = static_cast<float> (std::min (spanCents,   2400.0 - 100.0 * pitchSt));
            lo = static_cast<int> (std::lower_bound (cents, cents + count, a) - cents);
            hi = static_cast<int> (std::upper_bound (cents, cents + count, b) - cents);
            if (hi < lo) hi = lo;
        }

        /** Uniform pick (caller guarantees hi > lo; Slot::Pitch). */
        float pick (const ouaricon::grain::CounterRng::Draw& draw, int lo, int hi) const noexcept
        {
            return cents[lo + draw.nextInt (Slot::Pitch, hi - lo)];
        }

        const float* data() const noexcept      { return cents; }
        int  size() const noexcept              { return count; }
        bool hasUnison() const noexcept         { return unisonActive; }
        bool wasTruncated() const noexcept      { return truncated; }
        void invalidate() noexcept              { valid = false; }

    private:
        float cents[kMaxActive] {};
        int   count = 0;
        bool  unisonActive = false;
        bool  truncated    = false;

        bool          valid      = false;
        int           keyChoice  = -1;
        std::uint64_t keyMask    = 0;
        float         keySpan    = -1.0f;
        std::uint32_t keyVersion = 0;
    };

    //==========================================================================
    /** 2.5b absolute pitch follow (PLAN-2.5b P71; RESEARCH-2.5b R70–R72, R71 and
        D19 signed off / locked). One per stream, rebuilt at ticks only.

        Members (cents relative to R = 6000 + 100·root, i.e. 1/1 at C4 + root,
        R70) are c_d + k·P for the active degrees (mask tested by SOURCE index,
        then insertion-sorted — R9), over [−2400 − P, 2400 + 2P]: the maximum
        span, so the key is (table choice, mask, customVersion) only and a
        modulated pitch_rand never rebuilds it. Stored in DOUBLE. Truncated
        outward from k = 0 at kMaxActive (wasTruncated). D19: scale Off → the
        Chromatic built-in, every degree (the caller passes allDegrees).

        Per grain, in double, MIDI cents:
          T = 100·(f0 + pitch),  o = T − R,  k0 = floor (o/P),  q = o − k0·P
          clamp [100·f0 − 2400, 100·f0 + 2400] − R − k0·P   (|interval| <= 24 st)
          span > 0 and members in [q ± span] ∩ clamp → uniform pick
                       (draw.nextInt (Slot::Pitch, n), R72)
          otherwise → the NEAREST member to q inside the clamp, ties up (R71)
          none inside the clamp → NoMember (4); empty set → EmptySet (3)
          target t = R + k0·P + member (MIDI cents). */
    class AbsSet
    {
    public:
        enum Result : int { Hit = 0, EmptySet = 3, NoMember = 4 };

        bool rebuildIfNeeded (int choice, std::uint64_t mask, std::uint32_t customVersion,
                              const PitchCollections::Table& table, bool allDegrees) noexcept
        {
            const std::uint64_t m = allDegrees ? ~std::uint64_t (0) : mask;
            if (valid && choice == keyChoice && m == keyMask && customVersion == keyVersion)
                return false;
            valid      = true;
            keyChoice  = choice;
            keyMask    = m;
            keyVersion = customVersion;

            count     = 0;
            truncated = false;
            period    = static_cast<double> (table.period);
            const int nDeg = std::clamp (table.count, 0, PitchCollections::kMaxPitches);
            const std::uint64_t used = nDeg >= 64 ? ~std::uint64_t (0) : ((std::uint64_t (1) << nDeg) - 1);
            if ((m & used) != 0 && period > 0.0)
            {
                const double lo = -kAbsMaxSpanCents - period, hi = kAbsMaxSpanCents + 2.0 * period;
                const int kMax = static_cast<int> (std::ceil (hi / period)) + 1;
                for (int a = 0; a <= kMax && ! truncated; ++a)
                    for (int sgn = 0; sgn < (a == 0 ? 1 : 2) && ! truncated; ++sgn)
                    {
                        const int k = sgn == 0 ? a : -a;
                        for (int d = 0; d < nDeg; ++d)
                        {
                            if ((m >> d & 1u) == 0)
                                continue;
                            const double c = static_cast<double> (table.cents[static_cast<size_t> (d)])
                                           + static_cast<double> (k) * period;
                            if (c < lo || c > hi)
                                continue;
                            if (count >= kMaxActive) { truncated = true; break; }
                            cents[count++] = c;
                        }
                    }
            }
            for (int i = 1; i < count; ++i)
            {
                const double v = cents[i];
                int j = i;
                for (; j > 0 && cents[j - 1] > v; --j)
                    cents[j] = cents[j - 1];
                cents[j] = v;
            }
            return true;
        }

        /** The absolute target (MIDI cents) for a grain of a follow stream. */
        int target (double f0Midi, double pitchSt, double pitchRandSt, int root,
                    const ouaricon::grain::CounterRng::Draw& draw, double& targetCents) const noexcept
        {
            if (count <= 0 || ! (period > 0.0))
                return EmptySet;
            const double R  = kAbsRefCents + 100.0 * static_cast<double> (root);
            const double T  = 100.0 * (f0Midi + pitchSt);
            const double o  = T - R;
            const double k0 = std::floor (o / period);
            const double q  = o - k0 * period;
            const double cLo = 100.0 * f0Midi - kAbsMaxSpanCents - R - k0 * period;
            const double cHi = 100.0 * f0Midi + kAbsMaxSpanCents - R - k0 * period;
            const double span = 100.0 * pitchRandSt;

            if (span > 0.0)
            {
                const double a = std::max (q - span, cLo), b = std::min (q + span, cHi);
                const int lo = static_cast<int> (std::lower_bound (cents, cents + count, a) - cents);
                const int hi = static_cast<int> (std::upper_bound (cents, cents + count, b) - cents);
                if (hi > lo)
                {
                    targetCents = R + k0 * period + cents[lo + draw.nextInt (Slot::Pitch, hi - lo)];
                    return Hit;
                }
            }
            // Nearest member to q inside the clamp, ties up (R71).
            const int up = static_cast<int> (std::lower_bound (cents, cents + count, q) - cents);   // first >= q
            int best = -1;
            if (up < count && cents[up] <= cHi && cents[up] >= cLo)
                best = up;
            if (up - 1 >= 0 && cents[up - 1] >= cLo && cents[up - 1] <= cHi)
                if (best < 0 || q - cents[up - 1] < cents[best] - q)
                    best = up - 1;
            // q lies inside the clamp (|pitch| <= 24), so any member inside it is
            // either >= q (then cents[up] is inside) or < q (then cents[up − 1] is).
            if (best < 0)
                return NoMember;
            targetCents = R + k0 * period + cents[best];
            return Hit;
        }

        const double* data() const noexcept { return cents; }
        int    size() const noexcept         { return count; }
        double getPeriod() const noexcept    { return period; }
        bool   wasTruncated() const noexcept { return truncated; }
        void   invalidate() noexcept         { valid = false; }

    private:
        double cents[kMaxActive] {};
        int    count = 0;
        double period = 1200.0;
        bool   truncated = false;

        bool          valid      = false;
        int           keyChoice  = -1;
        std::uint64_t keyMask    = 0;
        std::uint32_t keyVersion = 0;
    };
} // namespace ogi
