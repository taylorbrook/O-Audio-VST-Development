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

    TerrainImage.cpp
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    // background job only — decode, blur, projection, view. Nothing here runs on
    // the audio thread (the read path is inline in the header).

  ==============================================================================
*/

#include "TerrainImage.h"
#include "ChebyshevProjector.h"

// ═══════════════════════════════════════════════════════════════════
// Decode
// ═══════════════════════════════════════════════════════════════════

std::shared_ptr<const DecodedImage> DecodedImage::decode (const void* bytes, size_t size)
{
    juce::Image img = juce::ImageFileFormat::loadFrom (bytes, size);
    if (! img.isValid() || img.getWidth() <= 0 || img.getHeight() <= 0)
        return nullptr;

    const int W = img.getWidth(), H = img.getHeight();
    const int k = juce::jmax (1, (juce::jmax (W, H) + kMaxSide - 1) / kMaxSide);   // ⌈max (W, H) / 1024⌉
    auto out = std::make_shared<DecodedImage>();
    out->w = juce::jmax (1, W / k);
    out->h = juce::jmax (1, H / k);
    out->lum.assign (static_cast<size_t> (out->w) * static_cast<size_t> (out->h), 0.0f);

    juce::Image::BitmapData bd (img, juce::Image::BitmapData::readOnly);
    const bool argb = img.getFormat() == juce::Image::ARGB, rgb = img.getFormat() == juce::Image::RGB;
    auto lumAt = [&] (int x, int y) -> float
    {
        const juce::uint8* p = bd.getPixelPointer (x, y);
        float r, g, b;
        if (argb)      { const auto* px = reinterpret_cast<const juce::PixelARGB*> (p); r = px->getRed(); g = px->getGreen(); b = px->getBlue(); }
        else if (rgb)  { const auto* px = reinterpret_cast<const juce::PixelRGB*> (p);  r = px->getRed(); g = px->getGreen(); b = px->getBlue(); }
        else           { const auto c = bd.getPixelColour (x, y); r = c.getRed(); g = c.getGreen(); b = c.getBlue(); }
        return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
    };

    const float inv = 1.0f / static_cast<float> (k * k);
    for (int y = 0; y < out->h; ++y)
        for (int x = 0; x < out->w; ++x)
        {
            float acc = 0.0f;
            for (int dy = 0; dy < k; ++dy)
                for (int dx = 0; dx < k; ++dx)
                    acc += lumAt (x * k + dx, y * k + dy);
            out->lum[static_cast<size_t> (y * out->w + x)] = 2.0f * acc * inv - 1.0f;
        }
    return out;
}

// ═══════════════════════════════════════════════════════════════════
// Blur (3-pass box, mirror-padded, separable running sums — plan Decision 36)
// ═══════════════════════════════════════════════════════════════════

int TerrainImage::boxWidthFor (float blur, int w) noexcept
{
    if (blur <= 0.0f) return 1;
    const double sigma = static_cast<double> (blur) * static_cast<double> (w) / 32.0;
    const double ideal = std::sqrt (12.0 * sigma * sigma / 3.0 + 1.0);
    int width = static_cast<int> (std::lround (ideal));
    if ((width & 1) == 0) width += 1;
    return juce::jmax (1, width);
}

namespace
{
    inline int reflect (int i, int n) noexcept
    {
        // whole-sample even reflection at the pixel-edge boundary; loops for r >= n
        while (i < 0 || i >= n)
        {
            if (i < 0) i = -i - 1;
            if (i >= n) i = 2 * n - i - 1;
        }
        return i;
    }

    void boxPass (const std::vector<float>& src, std::vector<float>& dst, int w, int h, int r, bool horizontal)
    {
        const int len = horizontal ? w : h, lines = horizontal ? h : w;
        const size_t stepAlong = horizontal ? 1 : static_cast<size_t> (w);
        const size_t stepLine  = horizontal ? static_cast<size_t> (w) : 1;
        const float inv = 1.0f / static_cast<float> (2 * r + 1);
        for (int line = 0; line < lines; ++line)
        {
            const float* s = src.data() + static_cast<size_t> (line) * stepLine;
            float* d = dst.data() + static_cast<size_t> (line) * stepLine;
            double sum = 0.0;
            for (int k = -r; k <= r; ++k) sum += s[static_cast<size_t> (reflect (k, len)) * stepAlong];
            for (int i = 0; i < len; ++i)
            {
                d[static_cast<size_t> (i) * stepAlong] = static_cast<float> (sum * inv);
                sum += s[static_cast<size_t> (reflect (i + r + 1, len)) * stepAlong];
                sum -= s[static_cast<size_t> (reflect (i - r, len)) * stepAlong];
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Build
// ═══════════════════════════════════════════════════════════════════

std::unique_ptr<TerrainImage> TerrainImage::build (std::shared_ptr<const DecodedImage> source, float blur, EdgeMode edge,
                                                   int revision, const juce::String& name, const juce::String& sha256,
                                                   const std::atomic<bool>* cancel)
{
    if (source == nullptr || source->w <= 0 || source->h <= 0)
        return nullptr;

    auto img = std::make_unique<TerrainImage>();
    img->source = std::move (source);
    img->w = img->source->w; img->h = img->source->h;
    img->blur = blur; img->edge = edge;
    img->name = name; img->sha256 = sha256;

    // blur
    const int width = boxWidthFor (blur, img->w);
    const int r = (width - 1) / 2;
    if (r <= 0)
    {
        img->blurred = img->source->lum;   // Blur 0 = copy
    }
    else
    {
        std::vector<float> a = img->source->lum, b (a.size());
        for (int pass = 0; pass < 3; ++pass)
        {
            boxPass (a, b, img->w, img->h, r, true);
            boxPass (b, a, img->w, img->h, r, false);
            if (cancel != nullptr && cancel->load()) return nullptr;
        }
        img->blurred = std::move (a);
    }

    // embedded Chebyshev set at F = 1, M = 64: node-matched box average (4 × 4 sub-samples
    // over ± 1/64, i.e. a W/64-px box) through the edge function
    {
        const int M = 64;
        std::vector<double> x, T;
        ChebyshevProjector::buildNodes (M, x, T);
        std::vector<double> f (static_cast<size_t> (M) * static_cast<size_t> (M));
        constexpr int q = 4;
        const float half = 1.0f / 64.0f;
        for (int i = 0; i < M; ++i)
            for (int j = 0; j < M; ++j)
            {
                double acc = 0.0;
                for (int a = 0; a < q; ++a)
                    for (int b = 0; b < q; ++b)
                    {
                        const float du = -half + (2.0f * half) * (static_cast<float> (a) + 0.5f) / static_cast<float> (q);
                        const float dv = -half + (2.0f * half) * (static_cast<float> (b) + 0.5f) / static_cast<float> (q);
                        acc += img->sample (static_cast<float> (x[static_cast<size_t> (i)]) + du,
                                            static_cast<float> (x[static_cast<size_t> (j)]) + dv, 1.0f, edge);
                    }
                f[static_cast<size_t> (i * M + j)] = acc / static_cast<double> (q * q);
            }
        if (cancel != nullptr && cancel->load()) return nullptr;
        ChebyshevProjector::projectGrid (f.data(), M, T.data(), img->cheb, cancel);
        if (cancel != nullptr && cancel->load()) return nullptr;
        img->cheb.key.terrain = static_cast<int> (TerrainKind::Imported);
        img->cheb.key.F = 1.0f; img->cheb.key.modX = 0.0f; img->cheb.key.modY = 0.0f;
        img->cheb.key.imageRevision = revision;
        img->fit = img->cheb.fit;
    }

    // 64 × 64 view heightmap (box average of the blurred copy; data only for Stage 3)
    {
        for (int vy = 0; vy < 64; vy++)
            for (int vx = 0; vx < 64; ++vx)
            {
                const int x0 = vx * img->w / 64, x1 = juce::jmax (x0 + 1, (vx + 1) * img->w / 64);
                const int y0 = vy * img->h / 64, y1 = juce::jmax (y0 + 1, (vy + 1) * img->h / 64);
                double acc = 0.0; int n = 0;
                for (int y = y0; y < y1 && y < img->h; ++y)
                    for (int xx = x0; xx < x1 && xx < img->w; ++xx) { acc += img->blurred[static_cast<size_t> (y * img->w + xx)]; ++n; }
                img->view[static_cast<size_t> (vy * 64 + vx)] = n > 0 ? static_cast<float> (acc / n) : 0.0f;
            }
    }
    return img;
}
