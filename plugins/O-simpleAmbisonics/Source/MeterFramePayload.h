/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    MeterFramePayload — the `meterFrame` event text (Stage 3).

    The editor's 30 Hz Timer and the render harness call the SAME functions:
      sampleFrame()  hasNewData() polled BEFORE read(), the live rule, quantise
      quantise()     MeterFrame → integers (1e-3 for gains / peaks / mix /
                     output peaks, 0.01° for angles)
      hash()         FNV-1a 64 over exactly the emitted integers, so a frame
                     either changes the hash or prints the same bytes — a
                     static scene emits nothing
      format()       one line of JSON into a caller-owned char buffer

    Why a pre-formatted string and not a juce::var tree: the bytes are
    deterministic and byte-comparable with the harness fixtures
    (tests/ui-fixtures/frames.json); juce::var serialises doubles at up to
    15 decimals, so the same frame would not print the same text.

    Why no printf("%f"): LC_NUMERIC is process-wide, and a host that sets a
    comma locale would print 0,707 and break JSON.parse on the page. Integers
    only, through std::to_chars (locale-independent), with the decimal point
    written by hand. Float to_chars is not used. The sign comes from the
    scaled integer, so "-0.000" can never appear.

    Robustness: non-finite inputs map to 0 and magnitudes are clamped to ±1e6
    before lround, so pluginval's fuzz can never produce text JSON.parse
    rejects, and the worst case fits the editor's 1024-byte buffer (harness PL1).

    U1 (user decision 2026-10-13) — host-idle display: `live` is
    everPublished && staleTicks < kStaleTicksIdle (3 ticks ≈ 100 ms at 30 Hz).
    When !live, quantise() zeroes shPeak[] and outPeakL/R. `live` is hashed and
    emitted, so the live → idle edge emits exactly one zero-peak frame, then
    silence; the page then draws the puck / bars / beam from the parameters.

    JUCE-free: the render harness (JUCE_WEB_BROWSER=0, no editor sources)
    includes this header. Includes only the two dsp headers and the standard
    library.

  ==============================================================================
*/

#pragma once

#include "dsp/MeterFrame.h"
#include "dsp/TripleBuffer.h"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace OSimpleAmbisonics::ui
{

inline constexpr int kStaleTicksIdle = 3;   // U1: ≈ 100 ms at 30 Hz

struct QuantisedFrame
{
    int gain[16] {}, peak[16] {};
    int azEff = 0, elEff = 0, azRot = 0, elRot = 0, orderActive = 4,
        mix = 1000, outL = 0, outR = 0, latency = 64;
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

    inline int clampInt (int v) noexcept
    {
        return v > 1000000 ? 1000000 : (v < -1000000 ? -1000000 : v);
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

        // scaled integer → fixed-point text with `decimals` digits after '.'
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

inline QuantisedFrame quantise (const dsp::MeterFrame& f, bool live) noexcept
{
    QuantisedFrame q;
    for (size_t i = 0; i < 16; ++i)
    {
        q.gain[i] = detail::scaled (f.shGain[i], 1000.0);
        q.peak[i] = live ? detail::scaled (f.shPeak[i], 1000.0) : 0;
    }
    q.azEff       = detail::scaled (f.azEffDeg, 100.0);
    q.elEff       = detail::scaled (f.elEffDeg, 100.0);
    q.azRot       = detail::scaled (f.azRotDeg, 100.0);
    q.elRot       = detail::scaled (f.elRotDeg, 100.0);
    q.orderActive = detail::clampInt (f.orderActive);
    q.mix         = detail::scaled (f.decoderMix, 1000.0);
    q.outL        = live ? detail::scaled (f.outPeakL, 1000.0) : 0;
    q.outR        = live ? detail::scaled (f.outPeakR, 1000.0) : 0;
    q.latency     = detail::clampInt (f.latencySamples);
    q.live        = live;
    return q;
}

// FNV-1a 64, byte-wise, over exactly the emitted integers in emission order.
inline std::uint64_t hash (const QuantisedFrame& q) noexcept
{
    std::uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < 16; ++i) h = detail::fnvInt (h, q.gain[i]);
    for (size_t i = 0; i < 16; ++i) h = detail::fnvInt (h, q.peak[i]);
    for (int v : { q.azEff, q.elEff, q.azRot, q.elRot, q.orderActive, q.mix,
                   q.outL, q.outR, q.latency, q.live ? 1 : 0 })
        h = detail::fnvInt (h, v);
    return h;
}

// One line of JSON (field order fixed). Returns the length written (no
// terminator counted; one is written when room allows), or 0 if `cap` is too
// small — the caller then skips the emission.
inline int format (const QuantisedFrame& q, char* buf, int cap) noexcept
{
    if (buf == nullptr || cap <= 1)
        return 0;

    detail::Writer w { buf, buf + (cap - 1) };   // keep one byte for '\0'
    w.puts ("{\"shGain\":[");
    for (size_t i = 0; i < 16; ++i) { if (i != 0) w.put (','); w.putFixed (q.gain[i], 3); }
    w.puts ("],\"shPeak\":[");
    for (size_t i = 0; i < 16; ++i) { if (i != 0) w.put (','); w.putFixed (q.peak[i], 3); }
    w.puts ("],\"azEffDeg\":");       w.putFixed (q.azEff, 2);
    w.puts (",\"elEffDeg\":");        w.putFixed (q.elEff, 2);
    w.puts (",\"azRotDeg\":");        w.putFixed (q.azRot, 2);
    w.puts (",\"elRotDeg\":");        w.putFixed (q.elRot, 2);
    w.puts (",\"orderActive\":");     w.putInt (q.orderActive);
    w.puts (",\"decoderMix\":");      w.putFixed (q.mix, 3);
    w.puts (",\"outPeakL\":");        w.putFixed (q.outL, 3);
    w.puts (",\"outPeakR\":");        w.putFixed (q.outR, 3);
    w.puts (",\"latencySamples\":");  w.putInt (q.latency);
    w.puts (",\"live\":");            w.put (q.live ? '1' : '0');
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
inline QuantisedFrame sampleFrame (dsp::TripleBuffer<dsp::MeterFrame>& buffer, LiveTracker& tracker) noexcept
{
    const bool fresh = buffer.hasNewData();
    const auto& f = buffer.read();
    const bool live = tracker.update (fresh);
    return quantise (f, live);
}

} // namespace OSimpleAmbisonics::ui
