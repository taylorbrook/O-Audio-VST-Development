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

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// ============================================================================
// GiHash — XXH64, the 24-bit quantiser and the slot content hash
// (PLAN-2.6 PD1–PD3, RESEARCH-2.6 §5).
//
//   * xxh64: written from the xxHash specification (doc/xxhash_spec.md,
//     BSD-2-Clause library / permissive spec). Lanes are assembled byte by
//     byte (little-endian on every host). Known-answer gate HS1.
//     Xxh64Stream is the same hash fed in pieces (PLAN-3.2 PD16, gate UIM3).
//   * quantise / dequantise: Q(x) = clamp (nearbyint (x · 2²³), −2²³, 2²³−1),
//     deq(q) = q · 2⁻²³ — exactly what JUCE's FLAC reader returns for a
//     24-bit sample, so Q(deq(q)) == q for every code (gate HS2). JUCE's
//     float writer path is NOT this (floor of round32/256, RS2), so FLAC is
//     always written from Q codes through the int path.
//   * contentHash: XXH64 (seed 0) of the canonical stream
//       uint32 LE C | uint32 LE K | K·C × int32 LE Q(x) (interleaved L, R)
//     with C = channels (1 or 2), K = min (frames, 131072). Persisted as 16
//     lowercase hex digits; parsed only as exactly 16 hex (never
//     getLargeIntValue, which clamps above 2⁶³).
// ============================================================================
namespace ogi
{
namespace gihash
{
    inline constexpr std::uint64_t kP1 = 0x9E3779B185EBCA87ull;
    inline constexpr std::uint64_t kP2 = 0xC2B2AE3D27D4EB4Full;
    inline constexpr std::uint64_t kP3 = 0x165667B19E3779F9ull;
    inline constexpr std::uint64_t kP4 = 0x85EBCA77C2B2AE63ull;
    inline constexpr std::uint64_t kP5 = 0x27D4EB2F165667C5ull;

    inline constexpr int kHashFrames = 131072;   // K cap: stereo = exactly 1 MiB of int32 samples

    inline std::uint64_t rotl (std::uint64_t x, int r) noexcept { return (x << r) | (x >> (64 - r)); }

    inline std::uint64_t read64 (const std::uint8_t* p) noexcept
    {
        std::uint64_t v = 0;
        for (int i = 7; i >= 0; --i)
            v = (v << 8) | p[i];
        return v;
    }

    inline std::uint32_t read32 (const std::uint8_t* p) noexcept
    {
        return static_cast<std::uint32_t> (p[0]) | (static_cast<std::uint32_t> (p[1]) << 8)
             | (static_cast<std::uint32_t> (p[2]) << 16) | (static_cast<std::uint32_t> (p[3]) << 24);
    }

    inline std::uint64_t round (std::uint64_t acc, std::uint64_t lane) noexcept
    {
        acc += lane * kP2;
        acc = rotl (acc, 31);
        return acc * kP1;
    }

    inline std::uint64_t merge (std::uint64_t acc, std::uint64_t v) noexcept
    {
        acc ^= round (0, v);
        return acc * kP1 + kP4;
    }

    /** XXH64 of `len` bytes (seed). */
    inline std::uint64_t xxh64 (const void* data, std::size_t len, std::uint64_t seed = 0) noexcept
    {
        const auto* p = static_cast<const std::uint8_t*> (data);
        const std::uint8_t* const end = p + len;
        std::uint64_t h;

        if (len >= 32)
        {
            std::uint64_t v1 = seed + kP1 + kP2, v2 = seed + kP2, v3 = seed, v4 = seed - kP1;
            const std::uint8_t* const limit = end - 32;
            do
            {
                v1 = round (v1, read64 (p));      p += 8;
                v2 = round (v2, read64 (p));      p += 8;
                v3 = round (v3, read64 (p));      p += 8;
                v4 = round (v4, read64 (p));      p += 8;
            }
            while (p <= limit);
            h = rotl (v1, 1) + rotl (v2, 7) + rotl (v3, 12) + rotl (v4, 18);
            h = merge (h, v1);
            h = merge (h, v2);
            h = merge (h, v3);
            h = merge (h, v4);
        }
        else
        {
            h = seed + kP5;
        }

        h += static_cast<std::uint64_t> (len);

        while (p + 8 <= end)
        {
            h ^= round (0, read64 (p));
            h = rotl (h, 27) * kP1 + kP4;
            p += 8;
        }
        if (p + 4 <= end)
        {
            h ^= static_cast<std::uint64_t> (read32 (p)) * kP1;
            h = rotl (h, 23) * kP2 + kP3;
            p += 4;
        }
        while (p < end)
        {
            h ^= static_cast<std::uint64_t> (*p) * kP5;
            h = rotl (h, 11) * kP1;
            ++p;
        }

        h ^= h >> 33;
        h *= kP2;
        h ^= h >> 29;
        h *= kP3;
        h ^= h >> 32;
        return h;
    }

    /** Streaming XXH64 (PLAN-3.2 PD16): update() in any chunking, digest()
        at any point; the result is bit-identical to xxh64 () over the
        concatenated bytes for every length (harness row UIM3). Used to name a
        dropped file by its WHOLE content without reading it in one piece.
        No allocation; any one thread at a time. */
    class Xxh64Stream
    {
    public:
        explicit Xxh64Stream (std::uint64_t seed = 0) noexcept { reset (seed); }

        void reset (std::uint64_t seed = 0) noexcept
        {
            seedValue = seed;
            acc[0] = seed + kP1 + kP2;
            acc[1] = seed + kP2;
            acc[2] = seed;
            acc[3] = seed - kP1;
            totalLen = 0;
            buffered = 0;
        }

        void update (const void* data, std::size_t len) noexcept
        {
            if (data == nullptr || len == 0)
                return;
            const auto* p = static_cast<const std::uint8_t*> (data);
            const std::uint8_t* const end = p + len;
            totalLen += static_cast<std::uint64_t> (len);

            if (buffered + len < 32)
            {
                std::memcpy (stripe + buffered, p, len);
                buffered += len;
                return;
            }
            if (buffered > 0)
            {
                const std::size_t fill = 32 - buffered;
                std::memcpy (stripe + buffered, p, fill);
                consume (stripe);
                p += fill;
                buffered = 0;
            }
            while (static_cast<std::size_t> (end - p) >= 32)
            {
                consume (p);
                p += 32;
            }
            if (p < end)
            {
                buffered = static_cast<std::size_t> (end - p);
                std::memcpy (stripe, p, buffered);
            }
        }

        std::uint64_t digest() const noexcept
        {
            std::uint64_t h;
            if (totalLen >= 32)
            {
                h = rotl (acc[0], 1) + rotl (acc[1], 7) + rotl (acc[2], 12) + rotl (acc[3], 18);
                h = merge (h, acc[0]);
                h = merge (h, acc[1]);
                h = merge (h, acc[2]);
                h = merge (h, acc[3]);
            }
            else
            {
                h = seedValue + kP5;
            }

            h += totalLen;

            const std::uint8_t* p = stripe;
            const std::uint8_t* const end = stripe + buffered;
            while (p + 8 <= end)
            {
                h ^= round (0, read64 (p));
                h = rotl (h, 27) * kP1 + kP4;
                p += 8;
            }
            if (p + 4 <= end)
            {
                h ^= static_cast<std::uint64_t> (read32 (p)) * kP1;
                h = rotl (h, 23) * kP2 + kP3;
                p += 4;
            }
            while (p < end)
            {
                h ^= static_cast<std::uint64_t> (*p) * kP5;
                h = rotl (h, 11) * kP1;
                ++p;
            }

            h ^= h >> 33;
            h *= kP2;
            h ^= h >> 29;
            h *= kP3;
            h ^= h >> 32;
            return h;
        }

        std::uint64_t totalBytes() const noexcept { return totalLen; }

    private:
        void consume (const std::uint8_t* p) noexcept
        {
            acc[0] = round (acc[0], read64 (p));
            acc[1] = round (acc[1], read64 (p + 8));
            acc[2] = round (acc[2], read64 (p + 16));
            acc[3] = round (acc[3], read64 (p + 24));
        }

        std::uint64_t seedValue = 0;
        std::uint64_t acc[4] {};
        std::uint64_t totalLen = 0;
        std::uint8_t  stripe[32] {};
        std::size_t   buffered = 0;   // 0…31 bytes waiting in `stripe`
    };

    /** PD3: the 24-bit quantiser (round half to even; clips at ±1). */
    inline std::int32_t quantise (float x) noexcept
    {
        const double v = std::nearbyint (static_cast<double> (x) * 8388608.0);
        return static_cast<std::int32_t> (std::clamp (v, -8388608.0, 8388607.0));
    }

    /** Exactly the FLAC reader's float for a 24-bit code. */
    inline float dequantise (std::int32_t q) noexcept
    {
        return static_cast<float> (static_cast<double> (q) * (1.0 / 8388608.0));
    }

    /** PD2: the slot content hash over the first min (frames, 131072) frames
        of up to 2 channels. Worker / message threads only (allocates). */
    inline std::uint64_t contentHash (const float* const* ch, int numChannels, int frames)
    {
        const int C = std::clamp (numChannels, 1, 2);
        const int K = std::clamp (frames, 0, kHashFrames);
        std::vector<std::uint8_t> buf (static_cast<std::size_t> (8 + 4 * K * C));
        auto put32 = [] (std::uint8_t* d, std::uint32_t v)
        {
            d[0] = static_cast<std::uint8_t> (v);
            d[1] = static_cast<std::uint8_t> (v >> 8);
            d[2] = static_cast<std::uint8_t> (v >> 16);
            d[3] = static_cast<std::uint8_t> (v >> 24);
        };
        put32 (buf.data(), static_cast<std::uint32_t> (C));
        put32 (buf.data() + 4, static_cast<std::uint32_t> (K));
        std::uint8_t* d = buf.data() + 8;
        for (int i = 0; i < K; ++i)
            for (int c = 0; c < C; ++c)
            {
                put32 (d, static_cast<std::uint32_t> (quantise (ch[c][i])));   // sign-extended int32, LE
                d += 4;
            }
        return xxh64 (buf.data(), buf.size(), 0);
    }

    inline std::uint64_t contentHash (const juce::AudioBuffer<float>& pcm)
    {
        const float* ch[2] = { pcm.getReadPointer (0), pcm.getReadPointer (pcm.getNumChannels() > 1 ? 1 : 0) };
        return contentHash (ch, std::min (2, pcm.getNumChannels()), pcm.getNumSamples());
    }

    /** 16 lowercase hex digits. */
    inline juce::String toHex (std::uint64_t h)
    {
        return juce::String::toHexString (static_cast<juce::int64> (h)).paddedLeft ('0', 16);
    }

    /** Exactly 16 hex digits → true + value; anything else → false. */
    inline bool fromHex (const juce::String& s, std::uint64_t& out)
    {
        if (s.length() != 16 || ! s.containsOnly ("0123456789abcdefABCDEF"))
            return false;
        out = static_cast<std::uint64_t> (s.getHexValue64());
        return true;
    }
} // namespace gihash
} // namespace ogi
