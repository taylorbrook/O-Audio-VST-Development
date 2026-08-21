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
/*
  ==============================================================================

    O-Emulator — AdpcmEncoder (Stage 2, Task 4)

    Shared closed-loop block-search ADPCM skeleton, templated on block length
    and predictor-coefficient table (ARCHITECTURE "One shared engine
    skeleton"): SNES BRR instantiates it as <16, 4>, PS1 SPU-ADPCM (Phase 2.2)
    as <28, 5>. Coefficients are /64 fixed point — the S-DSP ratios (15/16,
    61/32, 13/16, 115/64) and the SPU pairs both reduce to that form exactly.

    CLOSED-LOOP: every candidate (shift × filter) is decoded against the
    DECODED predictor history carried across blocks, never the clean input —
    open-loop encoding accumulates artifacts wrongly (ARCHITECTURE
    "Implementation notes"). Brute force is cheap: 13 shifts × NumFilters
    decodes of BlockLen samples per block at console rates.

    Streaming contract: processSample() presents a CONSTANT BlockLen-sample
    delay in every state (O-Bitrot CodecStage constant-latency model) — the
    decoded block is emitted while the next block accumulates, so
    out[n] = roundtrip(in[n - BlockLen]) always, independent of host block
    size by construction.

    All integer math, int32 intermediates, saturating int16 (the hardware's
    clamp/wrap simplified to saturation per ARCHITECTURE). GPL hygiene: written
    from the published S-DSP / SPU specs; no blargg/Nuked code consulted.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace oemu
{

template <int BlockLen, int NumFilters>
class AdpcmEncoder
{
public:
    struct Coeff
    {
        int f0, f1;   // predictor numerators, /64 fixed point
    };

    using CoeffTable = std::array<Coeff, (size_t) NumFilters>;

    /** Bind the (static-storage) coefficient table and reset all state.
        Prepare-time only. */
    void prepare (const CoeffTable& t) noexcept
    {
        table = &t;
        resetState();
    }

    void resetState() noexcept
    {
        pending.fill (0);
        emit.fill (0.0f);
        slot = 0;
        p1 = 0;
        p2 = 0;
    }

    /** Crush's shift floor: restricts the search to [floor, 12] (coarser
        quantisation as crush rises). Clamped so the encoder always keeps at
        least three shift candidates. */
    void setShiftFloor (int f) noexcept
    {
        shiftFloor = juce::jlimit (0, kMaxShift - 2, f);
    }

    /** One sample in, the round-trip of the sample BlockLen samples ago out.
        Input in [-1, 1] float; converted to the int16 codec domain here. */
    float processSample (float x) noexcept
    {
        jassert (table != nullptr);
        if (table == nullptr)
            return x;   // defensive: unprepared encoder passes through

        const float out = emit[(size_t) slot];

        pending[(size_t) slot] = juce::jlimit (-32768, 32767,
                                               (int) std::lrint (x * 32768.0f));

        if (++slot == BlockLen)
        {
            encodeBlock();
            slot = 0;
        }

        return out;
    }

private:
    static constexpr int kMaxShift = 12;

    /** floor(v / 64) — the hardware's arithmetic right shift, written without
        relying on implementation-defined signed >>. */
    static int shr6 (int v) noexcept
    {
        return v >= 0 ? (v >> 6) : -((-v + 63) >> 6);
    }

    /** Round-half-away-from-zero quantisation of a residual at 2^shift,
        clamped to the 4-bit signed nibble range. */
    static int quantize (int r, int shift) noexcept
    {
        const int half = shift > 0 ? (1 << (shift - 1)) : 0;
        const int q = (r >= 0 ? r + half : r - half) / (1 << shift);
        return juce::jlimit (-8, 7, q);
    }

    void encodeBlock() noexcept
    {
        const CoeffTable& coeffs = *table;

        long long bestErr = std::numeric_limits<long long>::max();
        int bestShift = shiftFloor;
        int bestFilter = 0;

        for (int f = 0; f < NumFilters; ++f)
        {
            for (int shift = shiftFloor; shift <= kMaxShift; ++shift)
            {
                int cp1 = p1, cp2 = p2;        // candidate starts from DECODED history
                long long err = 0;

                for (int i = 0; i < BlockLen; ++i)
                {
                    const int pred = shr6 (cp1 * coeffs[(size_t) f].f0
                                           + cp2 * coeffs[(size_t) f].f1);
                    const int q = quantize (pending[(size_t) i] - pred, shift);
                    const int d = juce::jlimit (-32768, 32767, pred + q * (1 << shift));

                    const long long e = (long long) (pending[(size_t) i] - d);
                    err += e * e;

                    cp2 = cp1;
                    cp1 = d;
                }

                if (err < bestErr)
                {
                    bestErr = err;
                    bestShift = shift;
                    bestFilter = f;
                }
            }
        }

        // Commit: re-decode the winner, advancing the carried predictor state
        // and filling the emit block (float, /32768).
        for (int i = 0; i < BlockLen; ++i)
        {
            const int pred = shr6 (p1 * coeffs[(size_t) bestFilter].f0
                                   + p2 * coeffs[(size_t) bestFilter].f1);
            const int q = quantize (pending[(size_t) i] - pred, bestShift);
            const int d = juce::jlimit (-32768, 32767, pred + q * (1 << bestShift));

            emit[(size_t) i] = (float) d * (1.0f / 32768.0f);

            p2 = p1;
            p1 = d;
        }
    }

    std::array<int, (size_t) BlockLen> pending {};
    std::array<float, (size_t) BlockLen> emit {};

    const CoeffTable* table = nullptr;

    int slot = 0;
    int p1 = 0, p2 = 0;       // decoded predictor history, carried across blocks
    int shiftFloor = 0;
};

} // namespace oemu
