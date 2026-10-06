/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - C++ -> page payloads (Stage 3.2, D-P, D-T)
    Ouaricon Audio
    Developer: Taylor Brook

    Header-only so the editor AND the viz-check console driver use the very
    same quantize / hash / var code (the gate tests the real wire).

    D-P: every wire value is quantized ONCE, q = round (x * 10^d), non-finite
    -> 0, and both the hash and the var are built from the same quanta
    (var value = q / 10^d). So sub-LSB noise cannot defeat the change gate,
    and no NaN, inf or -0 ever reaches the JSON.
      4 dp  cycle, preQ, pos, f0, nyquistH
      3 dp  bank thumbnails, lfo, menv, amp
      2 dp  harmonics, harmonicsRaw (dB)
    The hashes (FNV-1a over the int64 quanta) allocate nothing; the vars are
    built only when an event is actually emitted.

    Event shapes (the page's contract, mockups/v1-ui.html):
      cycleUpdate  { cycle[256], harmonics[32], harmonicsRaw[32], preQ[256]
                     (only when quantized), pos, frame, level, kmax, nyquistH,
                     note, f0, lfo, menv, amp, sounding }
      bankUpdate   { bank, imported, numFrames, filename, frames[N][128] }
      importStatus { state: idle|busy|done|error, filename, frames, error }

    The processor never includes this file.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

#include <cmath>
#include <cstddef>
#include <utility>

#include "PluginProcessor.h"

namespace viz
{
    // D-P quanta (decimal places).
    constexpr int kDpCycle = 4;   // cycle, preQ, pos, f0, nyquistH
    constexpr int kDpThumb = 3;   // thumbnails, lfo, menv, amp
    constexpr int kDpDb    = 2;   // harmonics, harmonicsRaw

    inline double scaleFor (int dp) noexcept
    {
        static constexpr double kScales[] = { 1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0 };
        return kScales[juce::jlimit (0, 6, dp)];
    }

    // Non-finite (and absurd magnitudes) -> 0. The int64 has no sign of zero,
    // so a value that rounds to 0 is sent as +0.
    inline juce::int64 quantize (double x, int dp) noexcept
    {
        if (! std::isfinite (x))
            return 0;
        const double s = std::round (x * scaleFor (dp));
        if (! (std::abs (s) < 9.0e15))
            return 0;
        return (juce::int64) s;
    }

    inline double dequantize (juce::int64 q, int dp) noexcept
    {
        return (double) q / scaleFor (dp);
    }

    // FNV-1a 64. No allocation.
    struct Fnv1a
    {
        juce::uint64 h = 14695981039346656037ull;

        void bytes (const void* p, std::size_t n) noexcept
        {
            const auto* c = static_cast<const unsigned char*> (p);
            for (std::size_t i = 0; i < n; ++i)
            {
                h ^= (juce::uint64) c[i];
                h *= 1099511628211ull;
            }
        }

        void add (juce::int64 q) noexcept { bytes (&q, sizeof (q)); }
    };

    //==========================================================================
    // cycleUpdate change gate: every field the var carries, as quanta.
    inline juce::uint64 cycleHash (const ::CycleView& v) noexcept
    {
        Fnv1a f;
        for (const float x : v.cycle)          f.add (quantize (x, kDpCycle));
        for (const float x : v.harmonicsDb)    f.add (quantize (x, kDpDb));
        for (const float x : v.harmonicsRawDb) f.add (quantize (x, kDpDb));
        f.add (v.quantized ? 1 : 0);
        if (v.quantized)
            for (const float x : v.preQ)       f.add (quantize (x, kDpCycle));
        f.add (quantize (v.pos, kDpCycle));
        f.add (v.frame);
        f.add (v.level);
        f.add (v.kmax);
        f.add (quantize (v.nyquistH, kDpCycle));
        f.add (v.note);
        f.add (quantize (v.f0, kDpCycle));
        f.add (quantize (v.lfo, kDpThumb));
        f.add (quantize (v.menv, kDpThumb));
        f.add (quantize (v.amp, kDpThumb));
        f.add (v.sounding ? 1 : 0);
        return f.h;
    }

    inline juce::var quantizedNumber (double x, int dp)
    {
        return juce::var (dequantize (quantize (x, dp), dp));
    }

    inline juce::var quantizedArray (const float* data, std::size_t n, int dp)
    {
        juce::Array<juce::var> arr;
        arr.ensureStorageAllocated ((int) n);
        for (std::size_t i = 0; i < n; ++i)
            arr.add (quantizedNumber (data[i], dp));
        return juce::var (std::move (arr));
    }

    inline juce::var cycleToVar (const ::CycleView& v)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("cycle",        quantizedArray (v.cycle.data(), v.cycle.size(), kDpCycle));
        obj->setProperty ("harmonics",    quantizedArray (v.harmonicsDb.data(), v.harmonicsDb.size(), kDpDb));
        obj->setProperty ("harmonicsRaw", quantizedArray (v.harmonicsRawDb.data(), v.harmonicsRawDb.size(), kDpDb));
        if (v.quantized)   // the unquantized ghost only matters when Bit Depth != Full
            obj->setProperty ("preQ", quantizedArray (v.preQ.data(), v.preQ.size(), kDpCycle));
        obj->setProperty ("pos",      quantizedNumber (v.pos, kDpCycle));    // effective position 0..1
        obj->setProperty ("frame",    v.frame);                              // latched frame, -1 with Interp On
        obj->setProperty ("level",    v.level);                              // mip level read
        obj->setProperty ("kmax",     v.kmax);                               // highest harmonic in that level
        obj->setProperty ("nyquistH", quantizedNumber (v.nyquistH, kDpCycle)); // (fs / 2) / f0
        obj->setProperty ("note",     v.note);                               // lead MIDI note, -1 silent
        obj->setProperty ("f0",       quantizedNumber (v.f0, kDpCycle));     // Hz incl. bend
        obj->setProperty ("lfo",      quantizedNumber (v.lfo, kDpThumb));    // -1..1, 0 at depth 0
        obj->setProperty ("menv",     quantizedNumber (v.menv, kDpThumb));
        obj->setProperty ("amp",      quantizedNumber (v.amp, kDpThumb));
        obj->setProperty ("sounding", v.sounding);
        return juce::var (obj);
    }

    //==========================================================================
    // bankUpdate change gate (D-T): drops the identical resend that the audio
    // thread's follow-up generation bump causes.
    inline juce::uint64 bankHash (const ::BankThumbs& t) noexcept
    {
        Fnv1a f;
        f.add (t.bank);
        f.add (t.imported ? 1 : 0);
        f.add (t.numFrames);
        const auto nameBytes = t.filename.getNumBytesAsUTF8();
        f.add ((juce::int64) nameBytes);
        f.bytes (t.filename.toRawUTF8(), nameBytes);
        for (const float x : t.points)
            f.add (quantize (x, kDpThumb));
        return f.h;
    }

    inline juce::var bankToVar (const ::BankThumbs& t)
    {
        constexpr int kThumb = WavetableBank::kThumbSize;
        const int n = juce::jlimit (0, (int) (t.points.size() / (std::size_t) kThumb), t.numFrames);
        jassert (n == t.numFrames);

        juce::Array<juce::var> frames;
        frames.ensureStorageAllocated (n);
        for (int fr = 0; fr < n; ++fr)
            frames.add (quantizedArray (t.points.data() + (std::size_t) fr * (std::size_t) kThumb,
                                        (std::size_t) kThumb, kDpThumb));

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("bank",      t.bank);
        obj->setProperty ("imported",  t.imported);
        obj->setProperty ("numFrames", n);
        obj->setProperty ("filename",  t.filename);
        obj->setProperty ("frames",    juce::var (std::move (frames)));
        return juce::var (obj);
    }

    //==========================================================================
    inline const char* importStateName (OSimpleWavetableAudioProcessor::ImportStatus::State s) noexcept
    {
        using S = OSimpleWavetableAudioProcessor::ImportStatus::State;
        switch (s)
        {
            case S::busy:  return "busy";
            case S::done:  return "done";
            case S::error: return "error";
            case S::idle:  return "idle";
        }
        return "idle";
    }

    inline juce::var importToVar (const OSimpleWavetableAudioProcessor::ImportStatus& st)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("state",    juce::String (importStateName (st.state)));
        obj->setProperty ("filename", st.filename);
        obj->setProperty ("frames",   st.frames);
        obj->setProperty ("error",    st.error);
        return juce::var (obj);
    }
}
