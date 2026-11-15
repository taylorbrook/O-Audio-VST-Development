/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - auto gain estimator
    (Stage 2, Phase 2.5; PLAN-2.5 P4, P5; RESEARCH-2.5 Q3; ARCHITECTURE
    "Auto gain" as amended by call C1)

    The formula:

      grid    f_i = exp (ln 40 + (ln fTop - ln 40)(i + 0.5) / M),  i = 0 ... M-1,
              M = 512 cell midpoints, fTop = min (16 kHz, 0.45 fs),
              t2_i = tan^2 (pi f_i / fs)                          (prepare only)
      band b  P_b[i] = product over its sections of sectionMag2 (t2_i, s),
              of the TARGET design of its latest request (desired input),
              never of the ramping design
      lanes   PL[i] = product of P_b[i] over Stereo, Mid and Left bands,
              PR[i] = product of P_b[i] over Stereo, Mid and Right bands;
              Side bands in neither; on a mono layout every band in both
      mean    sum over i of 0.5 (PL[i] + PR[i]), divided by M, in index order
      AG_dB   -10 log10 (mean), clamped to [-18, +12]; the target is 10^(AG_dB/20)

    Why 512 midpoints (C1). With 64 points a narrow boost swept in frequency
    passes between grid points and the estimate wobbles: 0.88 dB p-p for a
    +12 dB Q 40 bell, 8.7 dB for +30 dB Q 10. At 512 midpoints: 0.028 dB and
    0.12 dB. The midpoint rule also makes the G12 shelf exact (-6.167 dB):
    an inclusive grid weights its two ends by half a cell too many.

    Linear power. P_b holds |H|^2, and the lanes multiply them: no log or
    exp per point, the same number as summing log powers. A product only
    underflows where the true power is below about 1e-300, where it adds
    nothing to the mean.

    Guards. mean NaN: 1.0 (0 dB). mean +inf: the -18 dB clamp. mean at or
    below 1e-30 (everything cut): the +12 dB clamp.

    Contributing bands. A band contributes when its desired input is
    active, is not a gain type at exactly 0 dB effective gain (the sleep
    target of SlotBand: SlotBand::gainIsZero is private, so the test is
    restated here, the same expression), and, on a stereo layout, is not a
    Side band (it enters neither lane). A band that does not contribute is
    never designed. With no contributing band the target is exactly 1.0.

    The pure-function invariant (binding). The value estimate returns is a
    pure function of the 24 desired inputs, the layout flag and fs. The per
    band cache is only an optimisation: a band is designed again when its
    desired input differs from the one last evaluated (compared field by
    field, sameBandInput), and the lane products and the mean are computed
    afresh from the cached vectors at every call, never kept as a running
    sum. This is what lets the processor skip refreshes while it is idle
    without changing any later output.

    A change that keeps a band non-contributing (an edit to a disabled
    band) is not a dirty band: the estimate cannot move. Its last evaluated
    input is updated at the next estimate without a design.

    Storage is fixed: the t2 table, 24 band vectors and two lane vectors of
    512 doubles, about 220 KB. std-only, no allocation.

  ==============================================================================
*/

#pragma once

#include "BandParams.h"
#include "Designer.h"
#include "EqTypes.h"
#include "ResponseEval.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace eq
{

//==============================================================================
// Constants
//==============================================================================

inline constexpr int autoGainPoints = 512;
inline constexpr int autoGainBands = 24;
inline constexpr double autoGainLowHz = 40.0;
inline constexpr double autoGainHighHz = 16000.0;
inline constexpr double autoGainTopFraction = 0.45;
inline constexpr double autoGainMinDb = -18.0;
inline constexpr double autoGainMaxDb = 12.0;
inline constexpr double autoGainRefreshSamples48k = 1024.0;

/** Refresh period in control ticks: max (1, round (1024 / 48000 fs / K)).
    59 at 44.1 / 88.2 / 176.4 kHz, 64 at 48 / 96 / 192 kHz, 29 at 22.05 kHz,
    43 at 32 kHz. A refresh tick is a tick whose index (clock / K) is a
    multiple of it. */
inline int autoGainRefreshTicks (double sampleRateHz, int controlGrid) noexcept
{
    const double k = controlGrid < 1 ? 1.0 : static_cast<double> (controlGrid);
    const double r = std::round (autoGainRefreshSamples48k / 48000.0 * sampleRateHz / k);
    return r < 1.0 ? 1 : static_cast<int> (r);
}

//==============================================================================
class AutoGain
{
public:
    /** The grid for this rate; every band unevaluated. */
    void prepare (double sampleRateHz) noexcept
    {
        constexpr double twoPi = 6.283185307179586476925286766559;

        const double top = autoGainTopFraction * sampleRateHz;
        const double fTop = top < autoGainHighHz ? top : autoGainHighHz;
        const double lnLo = std::log (autoGainLowHz);
        const double lnHi = std::log (fTop);

        for (int i = 0; i < autoGainPoints; ++i)
        {
            const double u = (static_cast<double> (i) + 0.5) / static_cast<double> (autoGainPoints);
            const double f = std::exp (lnLo + (lnHi - lnLo) * u);
            t2[static_cast<std::size_t> (i)] = tanSquaredHalf (twoPi * f / sampleRateHz);
        }

        for (auto& band : cache)
        {
            band.evaluated = false;
            band.contributes = false;
        }

        cacheValid = false;
        evaluations = 0;
        refreshes = 0;
    }

    /** Whether a desired input contributes to the estimate. */
    static bool contributes (const BandInput& input, bool stereo) noexcept
    {
        if (! input.active)
            return false;

        if (isGainType (input.settings.type) && sameValue (input.settings.gainDbEff, 0.0))
            return false;

        return ! (stereo && input.placement == Placement::side);
    }

    /** True when an estimate now could differ from the last one: some band's
        desired input differs from the one last evaluated and contributes
        now or did then. No design; cheap. */
    bool isDirty (const BandInput* const* desired, bool stereo) const noexcept
    {
        if (! cacheValid || stereo != lastStereo)
            return true;

        for (int b = 0; b < autoGainBands; ++b)
        {
            const auto& band = cache[static_cast<std::size_t> (b)];
            const BandInput& in = *desired[b];

            if (! band.evaluated)
            {
                if (contributes (in, stereo))
                    return true;

                continue;
            }

            if (! sameBandInput (in, band.lastEval)
                && (band.contributes || contributes (in, stereo)))
                return true;
        }

        return false;
    }

    /** The linear auto-gain target for the 24 desired inputs. Designs only
        the bands whose desired input changed since they were last
        evaluated. */
    template <typename DesignerType>
    double estimate (const BandInput* const* desired, bool stereo, const DesignerType& designer) noexcept
    {
        return run (desired, stereo, designer, false);
    }

    /** The same with every band designed afresh (seeding). Returns the same
        value as estimate on the same inputs, bit for bit. */
    template <typename DesignerType>
    double evaluateAll (const BandInput* const* desired, bool stereo, const DesignerType& designer) noexcept
    {
        return run (desired, stereo, designer, true);
    }

    /** Since the last prepare: band designs evaluated over the grid, and
        calls of estimate / evaluateAll. */
    unsigned long getEvaluationCount() const noexcept   { return evaluations; }
    unsigned long getRefreshCount() const noexcept      { return refreshes; }

    /** The grid's t2 values (harness rows). */
    const std::array<double, static_cast<std::size_t> (autoGainPoints)>& getGrid() const noexcept { return t2; }

private:
    using Vector = std::array<double, static_cast<std::size_t> (autoGainPoints)>;

    struct BandCache
    {
        BandInput lastEval {};
        bool contributes = false;
        bool evaluated = false;
    };

    template <typename DesignerType>
    double run (const BandInput* const* desired, bool stereo, const DesignerType& designer, bool force) noexcept
    {
        ++refreshes;

        // A different layout flag changes which bands contribute and where:
        // every band is evaluated again.
        if (! cacheValid || stereo != lastStereo)
            force = true;

        lastStereo = stereo;
        cacheValid = true;

        // 1. Per band: design and multiply over the grid where needed.
        for (int b = 0; b < autoGainBands; ++b)
        {
            auto& band = cache[static_cast<std::size_t> (b)];
            const BandInput& in = *desired[b];

            if (! force && band.evaluated && sameBandInput (in, band.lastEval))
                continue;

            const bool c = contributes (in, stereo);

            if (c)
            {
                BandDesign design;
                designer.design (in.settings, design);

                auto& p = power[static_cast<std::size_t> (b)];

                for (int i = 0; i < autoGainPoints; ++i)
                {
                    const auto ui = static_cast<std::size_t> (i);
                    double mag2 = 1.0;

                    for (int s = 0; s < design.numSections; ++s)
                        mag2 *= sectionMag2 (t2[ui], design.params[static_cast<std::size_t> (s)]);

                    p[ui] = mag2;
                }

                ++evaluations;
            }

            band.lastEval = in;
            band.contributes = c;
            band.evaluated = true;
        }

        // 2. No contributing band: exactly 1.0.
        bool any = false;

        for (const auto& band : cache)
            any = any || band.contributes;

        if (! any)
            return 1.0;

        // 3. The lane products, afresh from the cached vectors.
        for (int i = 0; i < autoGainPoints; ++i)
        {
            const auto ui = static_cast<std::size_t> (i);
            laneL[ui] = 1.0;
            laneR[ui] = 1.0;
        }

        for (int b = 0; b < autoGainBands; ++b)
        {
            const auto ub = static_cast<std::size_t> (b);

            if (! cache[ub].contributes)
                continue;

            const Placement placement = desired[b]->placement;
            const bool intoL = ! stereo || placement == Placement::stereo || placement == Placement::mid || placement == Placement::left;
            const bool intoR = ! stereo || placement == Placement::stereo || placement == Placement::mid || placement == Placement::right;
            const auto& p = power[ub];

            if (intoL)
                for (int i = 0; i < autoGainPoints; ++i)
                    laneL[static_cast<std::size_t> (i)] *= p[static_cast<std::size_t> (i)];

            if (intoR)
                for (int i = 0; i < autoGainPoints; ++i)
                    laneR[static_cast<std::size_t> (i)] *= p[static_cast<std::size_t> (i)];
        }

        // 4. The mean, in index order.
        double sum = 0.0;

        for (int i = 0; i < autoGainPoints; ++i)
        {
            const auto ui = static_cast<std::size_t> (i);
            sum += 0.5 * (laneL[ui] + laneR[ui]);
        }

        const double mean = sum / static_cast<double> (autoGainPoints);

        // 5. Guards, the clamp, the linear target.
        if (std::isnan (mean))
            return 1.0;

        double db = 0.0;

        if (std::isinf (mean))
            db = autoGainMinDb;
        else if (mean <= 1.0e-30)
            db = autoGainMaxDb;
        else
        {
            db = -10.0 * std::log10 (mean);
            db = db < autoGainMinDb ? autoGainMinDb : (db > autoGainMaxDb ? autoGainMaxDb : db);
        }

        return std::pow (10.0, db / 20.0);
    }

    Vector t2 {};
    std::array<Vector, static_cast<std::size_t> (autoGainBands)> power {};
    Vector laneL {};
    Vector laneR {};
    std::array<BandCache, static_cast<std::size_t> (autoGainBands)> cache {};
    bool lastStereo = false;
    bool cacheValid = false;
    unsigned long evaluations = 0;
    unsigned long refreshes = 0;
};

} // namespace eq
