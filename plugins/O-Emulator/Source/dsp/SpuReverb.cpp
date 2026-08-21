/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

#include "SpuReverb.h"

#include <cmath>
#include <cstring>

namespace oemu
{

namespace
{
    /*  "Hall" register set (named per the psx-spx register mnemonics).
        Offsets are in 22050 Hz samples into the shared circular work buffer.

        Writers descend in DISJOINT regions — each writer owns the addresses
        from the next-lower writer (exclusive) up to itself (inclusive), so
        every read below resolves to its intended writer:

            mLSAME 32000 │ mRSAME 24000 │ mLDIFF 16800 │ mRDIFF 10400
            mLAPF1  5600 │ mRAPF1  4400 │ mLAPF2  3200 │ mRAPF2  2400

        Delay of a read d against writer m = (m − d) ticks.               */

    // Reflection writers
    constexpr int kMLSAME = 32000;
    constexpr int kMRSAME = 24000;
    constexpr int kMLDIFF = 16800;
    constexpr int kMRDIFF = 10400;

    // Wall-feedback reads (recirculation loops ~65–77 ms)
    constexpr int kDLSAME = 30313;   // mLSAME − 1687  (76.5 ms)
    constexpr int kDRSAME = 22399;   // mRSAME − 1601  (72.6 ms)
    constexpr int kDLDIFF = 15301;   // mLDIFF − 1499  (68.0 ms) — feeds mRDIFF (cross)
    constexpr int kDRDIFF =  8967;   // mRDIFF − 1433  (65.0 ms) — feeds mLDIFF (cross)

    // Early-echo comb reads (16–54 ms reflections; L combs 1/2 read the
    // same-side region, 3/4 the different-side region; R mirrored)
    constexpr int kMLCOMB1 = 31647;  // mLSAME −  353  (16.0 ms)
    constexpr int kMLCOMB2 = 31063;  // mLSAME −  937  (42.5 ms)
    constexpr int kMLCOMB3 = 16127;  // mLDIFF −  673  (30.5 ms)
    constexpr int kMLCOMB4 = 15613;  // mLDIFF − 1187  (53.8 ms)
    constexpr int kMRCOMB1 = 23599;  // mRSAME −  401  (18.2 ms)
    constexpr int kMRCOMB2 = 23119;  // mRSAME −  881  (40.0 ms)
    constexpr int kMRCOMB3 =  9783;  // mRDIFF −  617  (28.0 ms)
    constexpr int kMRCOMB4 =  9271;  // mRDIFF − 1129  (51.2 ms)

    // All-pass writers + delays (dAPF < the APF regions' 1200/800 spans)
    constexpr int kMLAPF1 = 5600;
    constexpr int kMRAPF1 = 4400;
    constexpr int kMLAPF2 = 3200;
    constexpr int kMRAPF2 = 2400;
    constexpr int kDAPF1  = 1051;    // 47.7 ms
    constexpr int kDAPF2  =  587;    // 26.6 ms

    // Gains (the SPU's signed /0x8000 volume range, as floats)
    constexpr float kVIIR   = 0.55f;  // input one-pole smoothing
    constexpr float kVWALL  = 0.55f;  // recirculation — RT60 ≈ 76.5 ms · ln(0.001)/ln(0.55) ≈ 0.9 s
    constexpr float kVCOMB1 = 0.30f;
    constexpr float kVCOMB2 = 0.25f;
    constexpr float kVCOMB3 = 0.22f;
    constexpr float kVCOMB4 = 0.18f;
    constexpr float kVAPF1  = 0.60f;
    constexpr float kVAPF2  = 0.50f;
    constexpr float kVLIN   = 0.75f;
    constexpr float kVRIN   = 0.75f;
} // namespace

void SpuReverb::reset() noexcept
{
    std::memset (buf, 0, sizeof (buf));
    pos = 0;
}

void SpuReverb::processTick (float inL, float inR, float& outL, float& outR) noexcept
{
    const float Lin = kVLIN * inL;
    const float Rin = kVRIN * inR;

    // ── Same-side reflections ([mX−1] read BEFORE this tick's write) ────────
    {
        const float prev = at (kMLSAME - 1);
        at (kMLSAME) = (Lin + at (kDLSAME) * kVWALL - prev) * kVIIR + prev;
    }
    {
        const float prev = at (kMRSAME - 1);
        at (kMRSAME) = (Rin + at (kDRSAME) * kVWALL - prev) * kVIIR + prev;
    }

    // ── Different-side reflections (stereo cross: L writer reads dRDIFF) ────
    {
        const float prev = at (kMLDIFF - 1);
        at (kMLDIFF) = (Lin + at (kDRDIFF) * kVWALL - prev) * kVIIR + prev;
    }
    {
        const float prev = at (kMRDIFF - 1);
        at (kMRDIFF) = (Rin + at (kDLDIFF) * kVWALL - prev) * kVIIR + prev;
    }

    // ── Early echo: 4 comb taps per side ────────────────────────────────────
    float Lout = kVCOMB1 * at (kMLCOMB1) + kVCOMB2 * at (kMLCOMB2)
               + kVCOMB3 * at (kMLCOMB3) + kVCOMB4 * at (kMLCOMB4);
    float Rout = kVCOMB1 * at (kMRCOMB1) + kVCOMB2 * at (kMRCOMB2)
               + kVCOMB3 * at (kMRCOMB3) + kVCOMB4 * at (kMRCOMB4);

    // ── All-pass 1 (the delayed value is read ONCE — the spec's second
    //    [mX−dAPF] reference is the same pre-write sample) ────────────────────
    {
        const float d = at (kMLAPF1 - kDAPF1);
        Lout -= kVAPF1 * d;
        at (kMLAPF1) = Lout;
        Lout = Lout * kVAPF1 + d;
    }
    {
        const float d = at (kMRAPF1 - kDAPF1);
        Rout -= kVAPF1 * d;
        at (kMRAPF1) = Rout;
        Rout = Rout * kVAPF1 + d;
    }

    // ── All-pass 2 ──────────────────────────────────────────────────────────
    {
        const float d = at (kMLAPF2 - kDAPF2);
        Lout -= kVAPF2 * d;
        at (kMLAPF2) = Lout;
        Lout = Lout * kVAPF2 + d;
    }
    {
        const float d = at (kMRAPF2 - kDAPF2);
        Rout -= kVAPF2 * d;
        at (kMRAPF2) = Rout;
        Rout = Rout * kVAPF2 + d;
    }

    // ── Non-sticky guard: reset STATE only, keep coefficients (they are
    //    constexpr — there is nothing stale to keep, which is the point:
    //    recovery is immediate and silent, never a latched wrong-filter) ─────
    if (! (std::isfinite (Lout) && std::isfinite (Rout)))
    {
        std::memset (buf, 0, sizeof (buf));
        Lout = 0.0f;
        Rout = 0.0f;
    }

    // ── Advance the work-buffer pointer ─────────────────────────────────────
    pos = (pos + 1) & (kBufSize - 1);

    outL = Lout;
    outR = Rout;
}

} // namespace oemu
