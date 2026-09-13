/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    TerrainViewFeed.cpp
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "TerrainViewFeed.h"
#include "dsp/MathConstants.h"
#include "dsp/Terrains.h"
#include "dsp/ChebyshevSet.h"
#include "dsp/TerrainImage.h"
#include <cmath>
#include <cstring>

namespace TerrainViewFeed
{
    namespace
    {
        constexpr uint32_t kMask = static_cast<uint32_t> (CycleCapture::kPoints - 1);
        constexpr float kPiF = 3.14159265358979323846f;

        inline float slotTheta (const CycleScratch& s, uint32_t idx) noexcept { return s.ring[(idx & kMask) * 4 + 0]; }

        /** Slot `idx` (absolute write index) → {px, py, y}. */
        inline void slotPoint (const CycleScratch& s, uint32_t idx, float& px, float& py, float& y) noexcept
        {
            const float* slot = s.ring.data() + (idx & kMask) * 4;
            px = slot[1]; py = slot[2]; y = slot[3];
        }

        inline float scrub (float v) noexcept { return std::isfinite (v) ? v : 0.0f; }

        /** The oscillator's saturation law (TerrainOscillator::saturate), applied on
            extraction: y' = tanh (g · y) / tanh (g), g = 1 + 4 · sat; identity at sat = 0.
            Shared by copyCycle's third column and getPlayhead's `h` (plan Decision 32). */
        inline float saturateLikeOscillator (float y, float sat) noexcept
        {
            if (sat <= 0.0f) return y;
            const float g = 1.0f + 4.0f * sat;
            return juce::jlimit (-1.0f, 1.0f, std::tanh (g * y) / std::tanh (g));
        }

        inline float round4 (float v) noexcept { return std::round (scrub (v) * 10000.0f) / 10000.0f; }

        inline float rawParam (OStrataAudioProcessor& processor, int o, const char* suffix) noexcept
        {
            return processor.getAPVTS().getRawParameterValue ((o == 0 ? "oscA" : "oscB") + juce::String (suffix))->load();
        }
    }

    int copyCycle (OStrataAudioProcessor& processor, int osc, CycleScratch& scratch, float* out)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        const int o = juce::jlimit (0, 1, osc);
        const CycleCapture& cap = processor.getCycleCapture (o);

        // ── 1. Twice-fenced copy (RESEARCH §2.3 step 1) ──
        const uint32_t w1 = cap.writeIndex.load (std::memory_order_acquire);
        if (w1 == scratch.lastWriteIndex || w1 == 0)
            return 0;   // idle: no new sub-sample since the last cycle we extracted
        std::memcpy (scratch.ring.data(), cap.ring.data(), sizeof (scratch.ring));
        const uint32_t w2 = cap.writeIndex.load (std::memory_order_acquire);
        const uint32_t writtenDuringCopy = w2 - w1;
        if (writtenDuringCopy >= static_cast<uint32_t> (CycleCapture::kPoints))
            return 0;   // the whole ring turned over under us — nothing in the copy is trustworthy

        // Slots [w1, w2] (inclusive — w2 may be mid-write) were touched during the copy; every
        // index below w1 is complete (release / acquire on writeIndex). The ring slot for
        // index w1 − kPoints + j has been overwritten by index w1 + j for j ≤ writtenDuringCopy,
        // so the oldest trustworthy absolute index is:
        const uint32_t oldest = w1 - static_cast<uint32_t> (CycleCapture::kPoints) + writtenDuringCopy + 1;
        uint32_t valid = (w1 - 1) - oldest + 1;   // number of trustworthy slots ending at w1 − 1
        if (w1 < valid) valid = w1;               // fewer than kPoints sub-samples ever written
        if (valid < 4)
            return 0;
        const uint32_t newest = w1 - 1;
        const uint32_t floor = newest - valid + 1;   // oldest usable absolute index

        // ── 2. Walk back over the θ wraps (a drop > π = the cycle boundary) ──
        // wrap1 = the newest boundary (start of the current partial cycle); wrap2 = the one
        // before it. The last COMPLETE cycle is [wrap2, wrap1 − 1].
        uint32_t wrap1 = 0, wrap2 = 0;
        bool haveWrap1 = false, haveWrap2 = false;
        {
            uint32_t k = newest;
            while (k > floor)
            {
                const float th = slotTheta (scratch, k), prev = slotTheta (scratch, k - 1);
                if (prev > th + kPiF)   // θ[k − 1] → θ[k] dropped by more than π: k starts a cycle
                {
                    if (! haveWrap1) { wrap1 = k; haveWrap1 = true; }
                    else             { wrap2 = k; haveWrap2 = true; break; }
                }
                --k;
            }
        }

        uint32_t spanStart = 0, spanLen = 0;
        bool monotone = false;
        if (haveWrap1 && haveWrap2)
        {
            spanStart = wrap2;
            spanLen = wrap1 - wrap2;
            // θ must not go backwards inside a clean cycle (Sync / Bend / FM warps do that)
            monotone = true;
            for (uint32_t k = spanStart + 1; k < spanStart + spanLen; ++k)
                if (slotTheta (scratch, k) < slotTheta (scratch, k - 1)) { monotone = false; break; }
        }

        if (! monotone)
        {
            // FM-warp fallback (RESEARCH §2.3 step 3): the newest fs · OS / f_display slots
            const juce::String pre = o == 0 ? "oscA" : "oscB";
            auto& apvts = processor.getAPVTS();
            const int quality = juce::jlimit (0, 2, static_cast<int> (apvts.getRawParameterValue (pre + "Quality")->load()));
            const int os = quality == static_cast<int> (Quality::X4) ? 4 : quality == static_cast<int> (Quality::X2) ? 2 : 1;
            const int coarse = static_cast<int> (apvts.getRawParameterValue (pre + "Coarse")->load());
            const double fine = apvts.getRawParameterValue (pre + "Fine")->load();
            double fs = processor.getSampleRate();
            if (fs <= 0.0) fs = 48000.0;
            const double fDisplay = processor.getLastPlayedFrequency() * pitchRatio (coarse, fine);
            if (! (fDisplay > 0.0))
                return 0;
            const double L = std::round (fs * os / fDisplay);
            if (L < 2.0 || L > static_cast<double> (valid))
                return 0;   // no complete cycle in the ring yet (or an idle voice)
            spanLen = static_cast<uint32_t> (L);
            spanStart = newest - spanLen + 1;
        }

        if (spanLen < 2)
            return 0;

        // ── 3. Resample to 512 by linear interpolation on slot index (closed loop: the
        //       512th point stops short of the next cycle's first slot) ──
        const float sat = juce::jlimit (0.0f, 1.0f, rawParam (processor, o, "TerSat"));
        const double step = static_cast<double> (spanLen) / static_cast<double> (kCyclePoints);
        for (int i = 0; i < kCyclePoints; ++i)
        {
            const double pos = static_cast<double> (i) * step;
            const uint32_t a = static_cast<uint32_t> (pos);
            const float t = static_cast<float> (pos - static_cast<double> (a));
            const uint32_t ia = spanStart + juce::jmin (a, spanLen - 1);
            const uint32_t ib = spanStart + juce::jmin (a + 1, spanLen - 1);
            float ax, ay, av, bx, by, bv;
            slotPoint (scratch, ia, ax, ay, av);
            slotPoint (scratch, ib, bx, by, bv);
            float px = ax + t * (bx - ax), py = ay + t * (by - ay), y = av + t * (bv - av);
            y = saturateLikeOscillator (y, sat);
            out[i * 3 + 0] = scrub (px);
            out[i * 3 + 1] = scrub (py);
            out[i * 3 + 2] = scrub (y);
        }

        scratch.lastWriteIndex = w1;
        return kCyclePoints;
    }

    juce::String encodeFloat32Base64 (const float* data, size_t count)
    {
        static_assert (! juce::ByteOrder::isBigEndian(), "terrainCycle / terrainHeightmap are little-endian Float32 on the wire");
        static_assert (sizeof (float) == 4, "Float32 wire format");
        juce::HeapBlock<float> scrubbed (count);
        for (size_t i = 0; i < count; ++i)
            scrubbed[i] = scrub (data[i]);
        return juce::Base64::toBase64 (scrubbed.getData(), count * sizeof (float));
    }

    juce::var makeCycleEvent (int osc, const float* out512x3)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("osc", osc == 0 ? "A" : "B");
        obj->setProperty ("n", kCyclePoints);
        obj->setProperty ("data", encodeFloat32Base64 (out512x3, static_cast<size_t> (kCyclePoints) * 3));
        return juce::var (obj);
    }

    juce::var makeStatusEvent (int osc, const OStrataAudioProcessor::TerrainStatus& s)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("osc", osc == 0 ? "A" : "B");
        obj->setProperty ("quality", s.quality);
        obj->setProperty ("terrain", s.terrain);
        obj->setProperty ("partials", s.partialsAtC4);
        obj->setProperty ("fit", s.fitPercent);
        obj->setProperty ("approx", s.approximate);
        obj->setProperty ("topNote", s.topNote);
        obj->setProperty ("sourceMissing", s.sourceMissing);
        return juce::var (obj);
    }

    // ═══════════════════════════════════════════════════════════════════
    // Round B — playhead (plan Decision 32)
    // ═══════════════════════════════════════════════════════════════════

    bool getPlayhead (OStrataAudioProcessor& processor, int osc, uint32_t& lastWriteIndex, Playhead& out)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        const int o = juce::jlimit (0, 1, osc);
        const CycleCapture& cap = processor.getCycleCapture (o);
        const uint32_t w = cap.writeIndex.load (std::memory_order_acquire);
        if (w == lastWriteIndex || w == 0)
            return false;   // idle: nothing new since the last playhead we sent
        lastWriteIndex = w;

        // Slot (w − 1) was complete before writeIndex was released. It is overwritten
        // again only after kPoints more sub-samples (≥ 42 ms at 4× / 48 k); the second
        // load catches the turnover on a stalled message thread.
        const float* slot = cap.ring.data() + ((w - 1) & kMask) * 4;
        const float theta = slot[0], px = slot[1], py = slot[2], y = slot[3];
        const uint32_t w2 = cap.writeIndex.load (std::memory_order_acquire);
        if (w2 - w >= static_cast<uint32_t> (CycleCapture::kPoints))
            return false;

        const float sat = juce::jlimit (0.0f, 1.0f, rawParam (processor, o, "TerSat"));
        out.theta = round4 (theta);
        out.x = round4 (px);
        out.y = round4 (py);
        out.h = round4 (saturateLikeOscillator (scrub (y), sat));
        return true;
    }

    juce::var makeStateEvent (int osc, const Playhead& ph)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("osc", osc == 0 ? "A" : "B");
        obj->setProperty ("theta", static_cast<double> (ph.theta));
        obj->setProperty ("x", static_cast<double> (ph.x));
        obj->setProperty ("y", static_cast<double> (ph.y));
        obj->setProperty ("h", static_cast<double> (ph.h));
        return juce::var (obj);
    }

    // ═══════════════════════════════════════════════════════════════════
    // Round B — heightmap (plan Decision 31)
    // ═══════════════════════════════════════════════════════════════════

    namespace
    {
        struct HeightmapInputs
        {
            int quality = 1, terrain = 0, edge = 0;
            float F = 1.0f, mx = 0.5f, my = 0.5f;
        };

        HeightmapInputs readHeightmapInputs (OStrataAudioProcessor& processor, int o)
        {
            HeightmapInputs in;
            in.quality = juce::jlimit (0, 2, static_cast<int> (rawParam (processor, o, "Quality")));
            in.terrain = juce::jlimit (0, 6, static_cast<int> (rawParam (processor, o, "Terrain")));
            in.edge    = juce::jlimit (0, 1, static_cast<int> (rawParam (processor, o, "TerEdge")));
            in.F  = rawParam (processor, o, "TerFreq");
            in.mx = juce::jlimit (0.0f, 1.0f, rawParam (processor, o, "TerModX"));
            in.my = juce::jlimit (0.0f, 1.0f, rawParam (processor, o, "TerModY"));
            if (! (in.F > 0.0f)) in.F = 1.0f;
            return in;
        }

        inline float gridCoord (int i) noexcept { return -1.0f + 2.0f * static_cast<float> (i) / static_cast<float> (kHeightmapN - 1); }
    }

    uint64_t heightmapKey (OStrataAudioProcessor& processor, int osc)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        const int o = juce::jlimit (0, 1, osc);
        const auto in = readHeightmapInputs (processor, o);
        auto q = [] (float v) { return static_cast<uint64_t> (static_cast<int64_t> (std::lround (v * 1024.0f))); };   // 1/1024 quantisation (the scheduler's rule)
        uint64_t h = 0xcbf29ce484222325ull;
        auto mix = [&h] (uint64_t v) { h ^= v; h *= 0x100000001b3ull; };
        mix (static_cast<uint64_t> (in.quality));
        mix (static_cast<uint64_t> (in.terrain));
        mix (q (in.F));
        mix (q (in.mx));
        mix (q (in.my));
        mix (static_cast<uint64_t> (in.edge));
        mix (static_cast<uint64_t> (static_cast<uint32_t> (processor.chebGeneration[o].load (std::memory_order_acquire))));
        mix (static_cast<uint64_t> (static_cast<uint32_t> (processor.imageGeneration[o].load (std::memory_order_acquire))));
        mix (static_cast<uint64_t> (static_cast<uint32_t> (processor.importRevision[o].load (std::memory_order_acquire))));
        mix (static_cast<uint64_t> (processor.getStateGeneration()));
        return h;
    }

    void copyHeightmap (OStrataAudioProcessor& processor, int osc, float* out)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        const int o = juce::jlimit (0, 1, osc);
        const auto in = readHeightmapInputs (processor, o);
        const bool imported = in.terrain == static_cast<int> (TerrainKind::Imported);
        const TerrainImage* image = processor.imagePtr[o].load (std::memory_order_acquire);

        // The oscillator's set rule (TerrainOscillator::updateBlockRate): Bandlimited takes
        // the candidate set only when its key names this terrain; otherwise the 1× fallback.
        const ChebyshevSet* set = nullptr;
        if (in.quality == static_cast<int> (Quality::Bandlimited))
        {
            const ChebyshevSet* candidate = imported ? (image != nullptr ? &image->cheb : nullptr)
                                                     : processor.chebPtr[o].load (std::memory_order_acquire);
            if (candidate != nullptr && candidate->key.terrain == in.terrain)
                set = candidate;
        }

        for (int j = 0; j < kHeightmapN; ++j)
        {
            const float y = gridCoord (j);   // j = 0 ⇒ y = −1
            for (int i = 0; i < kHeightmapN; ++i)
            {
                const float x = gridCoord (i);
                float v;
                if (set != nullptr)
                    v = clenshaw2D (set->c.data(), x, y);   // untapered — the published set itself
                else if (imported)
                    v = image != nullptr ? image->sample (x, y, in.F, static_cast<EdgeMode> (in.edge)) : 0.0f;
                else
                    v = terrain (static_cast<TerrainKind> (in.terrain), x, y, in.F, in.mx, in.my);
                out[j * kHeightmapN + i] = scrub (v);
            }
        }
    }

    juce::var makeHeightmapEvent (int osc, const float* out4096)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("osc", osc == 0 ? "A" : "B");
        obj->setProperty ("n", kHeightmapN);
        obj->setProperty ("data", encodeFloat32Base64 (out4096, static_cast<size_t> (kHeightmapSize)));
        return juce::var (obj);
    }
}
