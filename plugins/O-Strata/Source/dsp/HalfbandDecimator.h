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

    HalfbandDecimator.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Per-sample polyphase-IIR half-band decimators (ARCHITECTURE Core 5,
    Decision 5; RESEARCH §2.2). Coefficients designed ONCE per process by
    juce::dsp::FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod
    — (0.06, −70 dB) for the 2× stage (5 sections: 3 direct + 2 delayed) and
    (0.15, −60 dB) for the 4×→2× stage (3 sections: 2 + 1) — under std::call_once
    from TerrainOscillator::prepare (the design allocates; never on the audio
    thread). Each section is a one-state transposed allpass at the decimated
    rate, mirroring juce::dsp::Oversampling2TimesPolyphaseIIR::processSamplesDown;
    the delayed path's extra z⁻¹ is the `prevOdd` register.

    Not juce::dsp::Oversampling: that class is block-oriented and pairs up + down,
    which would break the per-sample FM cross-feed and mod destinations.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <mutex>

struct HalfbandCoeffs
{
    std::array<float, 3> direct2 {};
    std::array<float, 2> delayed2 {};
    std::array<float, 2> direct4 {};
    std::array<float, 1> delayed4 {};
    double latency2 = 0.0;   // base samples at 2× (≈ 1.259)
    double latency4 = 0.0;   // base samples at 4× (≈ 1.743, chained)

    /** Shared, rate-independent halfband coefficients; built on first call (prepare-time only). */
    static const HalfbandCoeffs& get()
    {
        static HalfbandCoeffs c;
        static std::once_flag once;
        std::call_once (once, [] { c.design(); });
        return c;
    }

private:
    // prepare-time only: FilterDesign allocates (juce::Array / ReferenceCountedArray).
    void design()
    {
        using FD = juce::dsp::FilterDesign<float>;
        const auto s2 = FD::designIIRLowpassHalfBandPolyphaseAllpassMethod (0.06f, -70.0f);
        const auto s4 = FD::designIIRLowpassHalfBandPolyphaseAllpassMethod (0.15f, -60.0f);
        jassert (s2.directPath.size() == 3 && s2.delayedPath.size() == 3);   // 5 sections = 3 + 2 (+ the delay element)
        jassert (s4.directPath.size() == 2 && s4.delayedPath.size() == 2);   // 3 sections = 2 + 1
        for (int i = 0; i < 3; ++i) direct2[(size_t) i]  = s2.directPath[i]->coefficients[0];
        for (int i = 0; i < 2; ++i) delayed2[(size_t) i] = s2.delayedPath[i + 1]->coefficients[0];   // i = 1 skips the z⁻¹ element
        for (int i = 0; i < 2; ++i) direct4[(size_t) i]  = s4.directPath[i]->coefficients[0];
        for (int i = 0; i < 1; ++i) delayed4[(size_t) i] = s4.delayedPath[i + 1]->coefficients[0];

        // DC group delay: per section 2 (1 − a) / (1 + a) at the oversampled rate;
        // D₀ = Σ direct, D₁ = 1 + Σ delayed, L_os = ½ (D₀ + D₁), L_out = L_os / OS.
        auto dc = [] (const float* a, size_t n) { double d = 0.0; for (size_t i = 0; i < n; ++i) d += 2.0 * (1.0 - a[i]) / (1.0 + a[i]); return d; };
        latency2 = 0.5 * (dc (direct2.data(), 3) + 1.0 + dc (delayed2.data(), 2)) / 2.0;
        latency4 = 0.5 * (dc (direct4.data(), 2) + 1.0 + dc (delayed4.data(), 1)) / 4.0 + latency2;
        jassert (latency2 <= 2.0 && latency4 <= 2.0);   // ARCH Decision 5: reported as a constant +1
    }
};

/** One allpass section at the decimated rate: out = a·in + v; v = in − a·out. */
namespace HalfbandDetail
{
    inline float allpass (float in, float a, float& v) noexcept
    {
        const float out = a * in + v;
        v = in - a * out;
        return out;
    }
}

/** 2× → 1× stage (3 direct + 2 delayed sections). */
struct HalfbandStage2
{
    float vDirect[3] = {}, vDelayed[2] = {}, prevOdd = 0.0f;

    void reset() noexcept { vDirect[0] = vDirect[1] = vDirect[2] = vDelayed[0] = vDelayed[1] = prevOdd = 0.0f; }

    float down (float even, float odd, const HalfbandCoeffs& c) noexcept
    {
        float x = even;
        for (int n = 0; n < 3; ++n) x = HalfbandDetail::allpass (x, c.direct2[(size_t) n], vDirect[n]);
        float d = odd;
        for (int n = 0; n < 2; ++n) d = HalfbandDetail::allpass (d, c.delayed2[(size_t) n], vDelayed[n]);
        const float y = 0.5f * (x + prevOdd);
        prevOdd = d;
        return y;
    }
};

/** 4× → 2× stage (2 direct + 1 delayed sections); chained into a HalfbandStage2 by the caller. */
struct HalfbandStage4
{
    float vDirect[2] = {}, vDelayed[1] = {}, prevOdd = 0.0f;

    void reset() noexcept { vDirect[0] = vDirect[1] = vDelayed[0] = prevOdd = 0.0f; }

    float down (float even, float odd, const HalfbandCoeffs& c) noexcept
    {
        float x = even;
        for (int n = 0; n < 2; ++n) x = HalfbandDetail::allpass (x, c.direct4[(size_t) n], vDirect[n]);
        float d = HalfbandDetail::allpass (odd, c.delayed4[0], vDelayed[0]);
        const float y = 0.5f * (x + prevOdd);
        prevOdd = d;
        return y;
    }
};
