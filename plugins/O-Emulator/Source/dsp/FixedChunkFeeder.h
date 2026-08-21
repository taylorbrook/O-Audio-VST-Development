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

    O-Emulator — FixedChunkFeeder (Stage 2, Task 1)

    The block-size-invariance spine: the host feeds arbitrary block sizes,
    the console engine only ever sees fixed 32-host-sample chunks, keyed to
    an ABSOLUTE sample counter reset in prepare() and nowhere else
    (pattern_block_rate_envelope_breaks_blocksize_invariance — O-Octagon
    GainStage model).

    Ordering inside the walk is write-then-consume: the chunk that a sample
    completes is processed BEFORE that call reads its own output sample
    (pattern_grain_read_before_capture_write_blocksize). The output read at
    absolute index n is position n - kChunk, which the previous chunk wrote
    at time <= n - 1, so no read ever precedes its write at ANY host block
    size, including blockSize >= the chunk length.

    The feeder owns its latency contribution: exactly kChunk samples
    (the first kChunk outputs are the cleared ring's zeros).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>
#include <cstring>

namespace oemu
{

class FixedChunkFeeder
{
public:
    /** 32 host samples — LCM-friendly against every console rate ratio and
        comfortably under 1 ms at all supported host rates (ARCHITECTURE
        "Fixed-Chunk Processing"). */
    static constexpr int kChunk = 32;

    static_assert ((kChunk & (kChunk - 1)) == 0,
                   "kChunk must be a power of two: the walk uses & (kChunk - 1) in "
                   "place of %, and kChunk dividing 2^64 makes the absolute "
                   "counter's eventual wrap preserve chunk phase.");

    /** The feeder's OWN latency figure, composed into the engine total. */
    static constexpr int getLatencySamples() noexcept { return kChunk; }

    void prepare() noexcept
    {
        counter = 0;                       // reset HERE and nowhere else
        std::memset (inChunk, 0, sizeof (inChunk));
        std::memset (outRing, 0, sizeof (outRing));
    }

    /** Walks `n` stereo samples through the fixed-chunk grid, in place.

        `chunkFn (inL, inR, outL, outR)` is called once per completed chunk and
        must fill exactly kChunk output samples per channel. The input pointers
        are the feeder's own chunk buffers and MAY be mutated in place (the
        engine runs its AA filters directly on them); the output pointers land
        directly inside the output ring (contiguous by construction: chunk
        bases are always kChunk-aligned and kChunk divides kOutRing).

        Templated on the callable so no std::function ever sits on the audio
        thread; call with a [this]-capturing lambda. */
    template <typename ChunkFn>
    void process (float* l, float* r, int n, ChunkFn&& chunkFn) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const int phase = (int) (counter & (std::uint64_t) (kChunk - 1));

            inChunk[0][phase] = l[i];
            inChunk[1][phase] = r[i];

            if (phase == kChunk - 1)
            {
                // Chunk complete: render it into the ring BEFORE this call's
                // output read below (write-then-consume).
                const int base = (int) ((counter - (std::uint64_t) (kChunk - 1))
                                        & (std::uint64_t) (kOutRing - 1));
                chunkFn (inChunk[0], inChunk[1], &outRing[0][base], &outRing[1][base]);
            }

            if (counter < (std::uint64_t) kChunk)
            {
                // The first kChunk outputs are the feeder's latency: zeros.
                l[i] = 0.0f;
                r[i] = 0.0f;
            }
            else
            {
                const int readIdx = (int) ((counter - (std::uint64_t) kChunk)
                                           & (std::uint64_t) (kOutRing - 1));
                l[i] = outRing[0][readIdx];
                r[i] = outRing[1][readIdx];
            }

            ++counter;
        }
    }

#if OUARICON_RENDER_HARNESS
    int getChunkPhaseForTest() const noexcept
    {
        return (int) (counter & (std::uint64_t) (kChunk - 1));
    }

    std::uint64_t getAbsoluteSamplesForTest() const noexcept { return counter; }
#endif

private:
    /** Two chunks — a position is written at latest at its own chunk's end and
        read exactly kChunk samples later, so double-buffering suffices. */
    static constexpr int kOutRing = 2 * kChunk;

    static_assert ((kOutRing & (kOutRing - 1)) == 0, "ring walk uses & (kOutRing - 1)");

    float inChunk[2][kChunk] {};
    float outRing[2][kOutRing] {};

    /** Absolute samples fed since prepare(). Deliberately uint64_t and NOT
        derived from the playhead (a host locate would jump the grid). */
    std::uint64_t counter = 0;
};

} // namespace oemu
