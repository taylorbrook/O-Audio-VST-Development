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

    O-simpleWavetable - MipmapBuilder implementation (Stage 2.1)

  ==============================================================================
*/

#include "MipmapBuilder.h"

#include <algorithm>
#include <cmath>

MipmapBuilder::MipmapBuilder()
    : fft (kOrder),
      spec ((size_t) kPacked, 0.0f),
      work ((size_t) kPacked, 0.0f)
{
}

void MipmapBuilder::buildFromSpectrum (WavetableBank& bank, int frameIndex, const float* spectrum, Normalise norm)
{
    jassert (spectrum != nullptr);
    jassert (frameIndex >= 0 && frameIndex < bank.numFrames);
    if (spectrum == nullptr || frameIndex < 0 || frameIndex >= bank.numFrames)
        return;

    // Copy first: spectrum may alias work (buildFromTime passes work.data()).
    std::copy (spectrum, spectrum + kPacked, spec.begin());

    spec[0] = 0.0f;                                     // DC
    spec[1] = 0.0f;

    bool zeroNyquist = true;
   #if OSIW_TEST_HOOKS
    zeroNyquist = ! testKeepNyquist;
   #endif
    if (zeroNyquist)
    {
        spec[(size_t) (2 * kNyquist)]     = 0.0f;       // Nyquist (Prism kept it at L0)
        spec[(size_t) (2 * kNyquist + 1)] = 0.0f;
    }

    // Bins above Nyquist are never read by JUCE's real inverse; clear them so
    // the scratch is deterministic.
    std::fill (spec.begin() + 2 * kNyquist + 2, spec.end(), 0.0f);

    if (norm == Normalise::peakToUnity)
    {
        inverseLevel (0);
        float peak = 0.0f;
        for (int i = 0; i < kN; ++i)
            peak = std::max (peak, std::abs (work[(size_t) i]));

        if (peak > 0.0f && std::isfinite (peak))
        {
            const float g = 1.0f / peak;
            for (auto& v : spec)
                v *= g;                                 // scale the spectrum; levels never re-normalised
        }
    }

    for (int level = 0; level < WavetableBank::kLevels; ++level)
    {
        inverseLevel (level);
        float* dst = bank.frame (level, frameIndex);
        std::copy (work.begin(), work.begin() + kN, dst);
        dst[kN] = dst[0];                               // guard sample

        if (level == 0 && ! bank.thumbs.empty())
        {
            float* th = bank.thumb (frameIndex);
            for (int t = 0; t < WavetableBank::kThumbSize; ++t)
                th[t] = dst[t * WavetableBank::kThumbStep];
        }
    }
}

void MipmapBuilder::buildFromTime (WavetableBank& bank, int frameIndex, const float* x2048, Normalise norm)
{
    jassert (x2048 != nullptr);
    if (x2048 == nullptr)
        return;

    std::fill (work.begin(), work.end(), 0.0f);
    std::copy (x2048, x2048 + kN, work.begin());
    fft.performRealOnlyForwardTransform (work.data());  // standard (unnormalised) DFT
    buildFromSpectrum (bank, frameIndex, work.data(), norm);
}

void MipmapBuilder::inverseLevel (int level)
{
    const int k = WavetableBank::kmax (level);
    std::copy (spec.begin(), spec.end(), work.begin());

    // Zero kmax+1 .. 1023 here; the Nyquist bin (1024) was already handled in
    // buildFromSpectrum (zeroed, unless the G-NEG test hook keeps it, in which
    // case it must survive at every level so the gate can see it).
    for (int bin = k + 1; bin < kNyquist; ++bin)
    {
        work[(size_t) (2 * bin)]     = 0.0f;
        work[(size_t) (2 * bin + 1)] = 0.0f;
    }

    fft.performRealOnlyInverseTransform (work.data()); // scaled 1/N by JUCE
}

//==============================================================================
std::unique_ptr<WavetableBank> buildFromLevel0 (const float* level0, int numFrames, const juce::String& name)
{
    if (level0 == nullptr || numFrames < 1 || numFrames > WavetableBank::kMaxFrames)
        return nullptr;

    auto bank = std::make_unique<WavetableBank>();
    bank->name = name;
    bank->allocate (numFrames);

    MipmapBuilder builder;
    for (int f = 0; f < numFrames; ++f)
        builder.buildFromTime (*bank, f, level0 + (size_t) f * (size_t) WavetableBank::kTableSize,
                               MipmapBuilder::Normalise::none);

    return bank;
}
