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

    TerrainImage.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    The PNG terrain (ARCHITECTURE Core 8, Decision 4; plan Decisions 35–37).
    DecodedImage = the luminance of an imported PNG (0.299 R + 0.587 G + 0.114 B
    mapped to 2·lum − 1 ∈ [−1, 1]), box-downsampled by the integer factor
    ⌈max (W, H) / 1024⌉ so no side exceeds 1024; non-square images stretch to
    [−1, 1]² (documented). Cached per import revision so Blur / Edge re-runs skip
    the decode.

    TerrainImage = one published, immutable object: the pre-blurred copy (3-pass
    box blur, σ = Blur · W / 32 px, mirror-padded regardless of Edge — Edge applies
    at read time only, a zero pad would rim-darken Window mode before the window
    does), the Edge mode it was built for, its own degree-16 Chebyshev set
    projected at F = 1 (Bandlimited + Imported takes it as the set source), a 64 × 64
    view heightmap for Stage 3, name + SHA-256, and a live-instance counter (the
    Round B leak verdict).

    Read path (audio thread): sample (px, py, F, edgeOverride) — (u, v) = F·(px, py);
    Mirror = even reflection with period 4; Window = clamp × w (u)·w (v),
    w (t) = ½ (1 + cos πt) for |t| <= 1, else 0; HarnessWrap (100) = periodic tiling,
    harness-only (the H11 negative control); bilinear over the pre-blurred copy;
    clamped to [−1, 1], isfinite-guarded. py = +1 reads the TOP row of the image.
    No pow, no allocation, no std::function (DSP-05).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Retirable.h"
#include "ChebyshevSet.h"
#include "TerrainOscillator.h"   // EdgeMode
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

struct DecodedImage
{
    int w = 0, h = 0;
    std::vector<float> lum;   // row-major, row 0 = top, values in [−1, 1]

    static constexpr int kMaxSide = 1024;

    /** // background job only (or the message thread at import — a user gesture).
        nullptr when the bytes do not decode. */
    static std::shared_ptr<const DecodedImage> decode (const void* bytes, size_t size);
};

struct TerrainImage final : Retirable
{
    std::shared_ptr<const DecodedImage> source;
    int w = 0, h = 0;
    std::vector<float> blurred;
    float blur = 0.0f;
    EdgeMode edge = EdgeMode::Mirror;
    ChebyshevSet cheb;            // key.terrain = Imported (6), F = 1; counts in ChebyshevSet::liveCount too
    float fit = 0.0f;             // the embedded set's fit (%), mirrored into chebFit / imageFit
    std::array<float, 64 * 64> view {};   // Stage 3 heightmap (data only)
    juce::String name, sha256;

    TerrainImage()  { liveCount.fetch_add (1, std::memory_order_relaxed); }
    ~TerrainImage() override { liveCount.fetch_sub (1, std::memory_order_relaxed); }
    TerrainImage (const TerrainImage&) = delete;
    TerrainImage& operator= (const TerrainImage&) = delete;

    inline static std::atomic<int> liveCount { 0 };

    /** // background job only. Blur, project (F = 1, M = 64), build the view.
        `cancel` (nullable) is polled between the stages; nullptr when cancelled. */
    static std::unique_ptr<TerrainImage> build (std::shared_ptr<const DecodedImage> source, float blur, EdgeMode edge,
                                                int revision, const juce::String& name, const juce::String& sha256,
                                                const std::atomic<bool>* cancel = nullptr);

    /** Box-blur width (odd, px) per pass for a given Blur at width w — exposed for the harness. */
    static int boxWidthFor (float blur, int w) noexcept;

    // ─── audio-thread read path ───
    /** `mode` = HarnessWrap forces the tiling control; anything else reads this image's `edge`. */
    inline float sample (float px, float py, float F, EdgeMode mode) const noexcept
    {
        float u = F * px, v = F * py, gain = 1.0f;
        const EdgeMode e = mode == EdgeMode::HarnessWrap ? mode : edge;
        if (e == EdgeMode::Mirror)
        {
            u = mirrorFold (u); v = mirrorFold (v);
        }
        else if (e == EdgeMode::Window)
        {
            gain = window (u) * window (v);
            u = juce::jlimit (-1.0f, 1.0f, u); v = juce::jlimit (-1.0f, 1.0f, v);
        }
        else
        {
            u = wrapFold (u); v = wrapFold (v);
        }
        const float y = gain * bilinear (u, v);
        return std::isfinite (y) ? juce::jlimit (-1.0f, 1.0f, y) : 0.0f;
    }

private:
    static inline float mirrorFold (float t) noexcept   // even reflection, period 4, into [−1, 1]
    {
        float s = t + 1.0f;
        s -= 4.0f * std::floor (s * 0.25f);   // [0, 4)
        if (s > 2.0f) s = 4.0f - s;
        return s - 1.0f;
    }
    static inline float wrapFold (float t) noexcept     // periodic tiling into [−1, 1)
    {
        float s = t + 1.0f;
        s -= 2.0f * std::floor (s * 0.5f);
        return s - 1.0f;
    }
    static inline float window (float t) noexcept
    {
        return std::abs (t) <= 1.0f ? 0.5f * (1.0f + std::cos (3.14159265358979f * t)) : 0.0f;
    }
    inline float bilinear (float u, float v) const noexcept
    {
        // pixel centres at (i + ½) / w; py = +1 → the top row
        const float cx = (u + 1.0f) * 0.5f * static_cast<float> (w) - 0.5f;
        const float cy = (1.0f - v) * 0.5f * static_cast<float> (h) - 0.5f;
        const float fx0 = std::floor (cx), fy0 = std::floor (cy);
        const float tx = cx - fx0, ty = cy - fy0;
        const int x0 = juce::jlimit (0, w - 1, static_cast<int> (fx0)), x1 = juce::jlimit (0, w - 1, static_cast<int> (fx0) + 1);
        const int y0 = juce::jlimit (0, h - 1, static_cast<int> (fy0)), y1 = juce::jlimit (0, h - 1, static_cast<int> (fy0) + 1);
        const float* r0 = blurred.data() + static_cast<size_t> (y0) * static_cast<size_t> (w);
        const float* r1 = blurred.data() + static_cast<size_t> (y1) * static_cast<size_t> (w);
        const float a = r0[x0] + tx * (r0[x1] - r0[x0]);
        const float b = r1[x0] + tx * (r1[x1] - r1[x0]);
        return a + ty * (b - a);
    }
};
