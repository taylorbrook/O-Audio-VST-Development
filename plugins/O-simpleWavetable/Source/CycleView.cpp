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

    O-simpleWavetable - CycleRenderer (Stage 3.2, RESEARCH Q3)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase j / 2048 gives idx = j and t = 0, so the rendered cycle is the
    frame-lerped (Interp On) or latched (Off) table samples, then quantized.
    The quantizer is memoryless, so bins 1..32 are the harmonics of the
    continuous heard cycle. No window: the cycle is exactly periodic.

  ==============================================================================
*/

#include "CycleView.h"

#include <algorithm>
#include <cmath>

void CycleRenderer::renderCycle (const WavetableBank& b, int level, bool interp, float pos, int latched,
                                 const BitQuantizer& q, Cycle& out, Cycle* unquantized) noexcept
{
    for (int j = 0; j < kSize; ++j)
    {
        const float s = wt::readSample (b, level, (double) j / (double) kSize, interp, pos, latched);
        if (unquantized != nullptr)
            (*unquantized)[(size_t) j] = s;
        out[(size_t) j] = q.apply (s);                           // same order as the voice
    }
}

void CycleRenderer::transform (const Cycle& x) noexcept
{
    std::copy (x.begin(), x.end(), work.begin());
    std::fill (work.data() + kSize, work.data() + 2 * kSize, 0.0f);
    fft.performFrequencyOnlyForwardTransform (work.data(), true);   // work[k] = |X_k|, k = 0..1024
}

float CycleRenderer::referenceMagnitude() const noexcept
{
    float ref = 0.0f;
    for (int k = 1; k <= kRefMaxBin; ++k)
    {
        const float m = work[(size_t) k];
        if (std::isfinite (m) && m > ref)
            ref = m;
    }
    return ref;
}

void CycleRenderer::toDb (float ref, std::array<float, CycleView::kHarmonics>& out) const noexcept
{
    for (int k = 1; k <= CycleView::kHarmonics; ++k)
    {
        const float m = work[(size_t) k];
        float db = kFloorDb;
        if (ref > 1.0e-12f && std::isfinite (ref) && m > 0.0f && std::isfinite (m))
        {
            const double d = 20.0 * std::log10 ((double) m / (double) ref);
            db = std::isfinite (d) ? (float) d : kFloorDb;
        }
        out[(size_t) (k - 1)] = juce::jlimit (kFloorDb, 0.0f, db);
    }
}

void CycleRenderer::render (const WavetableBank* b, int level, bool interp, float pos, int latched,
                            int bitIdx, CycleView& v) noexcept
{
    BitQuantizer q;
    q.setChoiceIndex (bitIdx);
    v.quantized = q.bits > 0;

    if (b == nullptr || b->numFrames <= 0)
    {
        v.empty = true;
        v.cycle.fill (0.0f);
        v.preQ.fill (0.0f);
        v.harmonicsDb.fill (kFloorDb);
        v.harmonicsRawDb.fill (kFloorDb);
        return;
    }
    v.empty = false;

    const int lvl = juce::jlimit (0, WavetableBank::kLevels - 1, level);

    renderCycle (*b, lvl, interp, pos, latched, q, heard, &pre);

    constexpr int step = kSize / CycleView::kPoints;               // POINT sampling (keeps staircases)
    for (int i = 0; i < CycleView::kPoints; ++i)
    {
        v.cycle[(size_t) i] = heard[(size_t) (i * step)];
        v.preQ[(size_t) i]  = pre[(size_t) (i * step)];
    }

    transform (heard);
    const float ref = referenceMagnitude();
    toDb (ref, v.harmonicsDb);

    // Ghost bars (band-limit On, k > kmax): the level-0 cycle at the same
    // position through the same quantizer, on the HEARD scale, clamped <= 0.
    if (lvl > 0)
    {
        renderCycle (*b, 0, interp, pos, latched, q, raw, nullptr);
        transform (raw);
        toDb (ref, v.harmonicsRawDb);
    }
    else
    {
        v.harmonicsRawDb = v.harmonicsDb;
    }
}
