/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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

    MeterFramePayload - the `meterFrame` event text (v3.0.0).

    Ported from O-simpleAmbisonics (Source/MeterFramePayload.h): scaled /
    Writer / fnvInt / LiveTracker / sampleFrame are unchanged; QuantisedFrame,
    quantise, hash and format carry this plugin's fields.

    The editor's 30 Hz Timer and render-check call the SAME functions:
      sampleFrame()  hasNewData() polled BEFORE read(), the live rule, quantise
      quantise()     MeterFrame -> integers (dB x 10 for the two levels,
                     linear x 1000 for the output peaks, whole ms for the span)
      hash()         FNV-1a 64 over exactly the emitted integers, so a frame
                     either changes the hash or prints the same bytes - a
                     steady scene emits nothing
      format()       one line of JSON into a caller-owned char buffer

    Why a pre-formatted string and not a juce::var tree: the bytes are
    deterministic and byte-comparable with the fixtures
    (tests/ui-fixtures/frames.json, written by render-check --write-frames).

    Why no printf("%f"): LC_NUMERIC is process-wide, and a host that sets a
    comma locale would print -12,5 and break JSON.parse on the page. Integers
    only, through std::to_chars, with the decimal point written by hand.

    Robustness: non-finite inputs map to 0 and magnitudes are clamped to +-1e6
    before lround; the two levels are then clamped to -120 .. +24 dB. Fuzzed
    input can never produce text JSON.parse rejects.

    Host-idle display: `live` is everPublished && staleTicks < kStaleTicksIdle
    (3 ticks, about 100 ms). When !live, quantise() floors the two levels and
    zeroes the peaks and the span. `live` is hashed, so the live -> idle edge
    emits exactly one floor frame, then nothing; the page freezes its plot.

    JUCE-free: render-check includes it.

  ==============================================================================
*/

#pragma once

#include "MeterFrame.h"
#include "TripleBuffer.h"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace osr::ui
{

inline constexpr int kStaleTicksIdle = 3;   // about 100 ms at 30 Hz
inline constexpr int kDbFloorX10 = -1200;   // -120.0 dB
inline constexpr int kDbCeilX10  =   240;   //  +24.0 dB

struct QuantisedFrame
{
    int inDb = kDbFloorX10, wetDb = kDbFloorX10;   // dB x 10
    int outL = 0, outR = 0;                        // linear x 1000
    int spanMs = 0;
    bool live = false;
};

namespace detail
{
    inline int scaled (float v, double k) noexcept
    {
        double d = (double) v;
        if (! std::isfinite (d))
            d = 0.0;
        if (d >  1.0e6) d =  1.0e6;
        if (d < -1.0e6) d = -1.0e6;
        return (int) std::lround (d * k);
    }

    inline int clampDb (int v) noexcept
    {
        return v > kDbCeilX10 ? kDbCeilX10 : (v < kDbFloorX10 ? kDbFloorX10 : v);
    }

    // A level: non-finite -> the floor (silence), never 0 dB.
    inline int levelDb (float v) noexcept
    {
        return std::isfinite (v) ? clampDb (scaled (v, 10.0)) : kDbFloorX10;
    }

    // Bounded writer: once out of room it stays failed and format() returns 0.
    struct Writer
    {
        char* p;
        char* end;
        bool ok = true;

        void put (char c) noexcept
        {
            if (! ok || p >= end) { ok = false; return; }
            *p++ = c;
        }

        void puts (const char* s) noexcept
        {
            while (*s != '\0')
                put (*s++);
        }

        void putInt (long long v) noexcept
        {
            if (! ok) return;
            const auto r = std::to_chars (p, end, v);
            if (r.ec != std::errc()) { ok = false; return; }
            p = r.ptr;
        }

        // scaled integer -> fixed-point text with `decimals` digits after '.'
        void putFixed (int v, int decimals) noexcept
        {
            long long a = v < 0 ? -(long long) v : (long long) v;
            long long div = 1;
            for (int i = 0; i < decimals; ++i)
                div *= 10;
            if (v < 0)
                put ('-');
            putInt (a / div);
            put ('.');
            long long frac = a % div;
            char digits[8] {};
            for (int i = decimals - 1; i >= 0; --i)
            {
                digits[(size_t) i] = (char) ('0' + (int) (frac % 10));
                frac /= 10;
            }
            for (int i = 0; i < decimals; ++i)
                put (digits[(size_t) i]);
        }
    };

    inline std::uint64_t fnvInt (std::uint64_t h, int v) noexcept
    {
        auto u = (std::uint32_t) v;
        for (int i = 0; i < 4; ++i)
        {
            h ^= (std::uint64_t) (u & 0xffu);
            h *= 1099511628211ULL;
            u >>= 8;
        }
        return h;
    }
}

inline QuantisedFrame quantise (const MeterFrame& f, bool live) noexcept
{
    QuantisedFrame q;
    q.inDb   = live ? detail::levelDb (f.inDb)  : kDbFloorX10;
    q.wetDb  = live ? detail::levelDb (f.wetDb) : kDbFloorX10;
    q.outL   = live ? detail::scaled (f.outPeakL, 1000.0) : 0;
    q.outR   = live ? detail::scaled (f.outPeakR, 1000.0) : 0;
    q.spanMs = live ? detail::scaled (f.spanMs, 1.0) : 0;
    q.live   = live;
    return q;
}

// FNV-1a 64, byte-wise, over exactly the emitted integers in emission order.
inline std::uint64_t hash (const QuantisedFrame& q) noexcept
{
    std::uint64_t h = 1469598103934665603ULL;
    for (int v : { q.inDb, q.wetDb, q.outL, q.outR, q.spanMs, q.live ? 1 : 0 })
        h = detail::fnvInt (h, v);
    return h;
}

// One line of JSON (field order fixed). Returns the length written (no
// terminator counted; one is written when room allows), or 0 if `cap` is too
// small - the caller then skips the emission.
inline int format (const QuantisedFrame& q, char* buf, int cap) noexcept
{
    if (buf == nullptr || cap <= 1)
        return 0;

    detail::Writer w { buf, buf + (cap - 1) };   // keep one byte for '\0'
    w.puts ("{\"inDb\":");       w.putFixed (q.inDb, 1);
    w.puts (",\"wetDb\":");      w.putFixed (q.wetDb, 1);
    w.puts (",\"outPeakL\":");   w.putFixed (q.outL, 3);
    w.puts (",\"outPeakR\":");   w.putFixed (q.outR, 3);
    w.puts (",\"spanMs\":");     w.putInt (q.spanMs);
    w.puts (",\"live\":");       w.put (q.live ? '1' : '0');
    w.put ('}');

    if (! w.ok)
        return 0;
    *w.p = '\0';
    return (int) (w.p - buf);
}

struct LiveTracker
{
    int  staleTicks    = 0;
    bool everPublished = false;

    bool update (bool fresh) noexcept
    {
        staleTicks = fresh ? 0 : (staleTicks + 1 < 1000 ? staleTicks + 1 : 1000);
        everPublished = everPublished || fresh;
        return everPublished && staleTicks < kStaleTicksIdle;
    }
};

// The one sampler: poll hasNewData() BEFORE read() (read() consumes it).
inline QuantisedFrame sampleFrame (TripleBuffer<MeterFrame>& buffer, LiveTracker& tracker) noexcept
{
    const bool fresh = buffer.hasNewData();
    const auto& f = buffer.read();
    const bool live = tracker.update (fresh);
    return quantise (f, live);
}

} // namespace osr::ui
