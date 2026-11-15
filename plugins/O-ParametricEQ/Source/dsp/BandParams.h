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

    O-ParametricEQ - raw parameter values -> designer input (Stage 2, Phase 2.2)

    makeBandInput is the ONE function from the raw parameter floats of a band
    to what the designer and the router consume. The processor, both test
    harnesses and the display model (Phase 2.6) call it; there is no second
    raw -> settings path (PLAN-2.2 P1, P2).

    What it does, in order (PLAN-2.2 P2):
      1. a non-finite raw value becomes that parameter's default
         (type 0, freq 1000, gain 0, Q 1, slope index 1, placement 0,
         scale 100; toggles off);
      2. clamp: freq 10 - 30 000 Hz, gain +-30 dB, Q 0.025 - 40, scale 0 - 200;
         choices round to nearest (a half rounds up) and clamp to their index
         range; a toggle is on when raw >= 0.5;
      3. gEff = clamp (gain * scale / 100 + dynOffsetDb, -60, +60) in double,
         the sum and the clamp through offsetGainDb (below). At scale 100 and
         offset 0 this is the float gain, bit for bit;
      4. active = enabled && ! bypass;
      5. canonical form: slopeIndex = 0 for Bell, Notch, Band Pass (D6);
         q = 1.0 for a slope type at 6 dB/oct (D9); gainDbEff = 0 for Low Cut,
         High Cut, Notch, Band Pass (PLAN-2.3 P6: the offset is ignored there
         too). The designer reads none of these values there, so the design is
         the same; the canonical form is what makes those edits compare as
         "identical" and cost nothing: no ramp, no design.

    offsetGainDb is the ONE place a dynamic offset is summed into a gain
    (PLAN-2.3 P5): step 3 above uses it, and so does the band at each control
    tick, where the offset is added after the control ramp (Band.h). The
    processor passes makeBandInput (..., 0.0) targets to the band; the band
    reads its own dynOffsetDb.

    Nothing here tests for an exact frequency or Q: the skewed parameter
    defaults are inexact (999.99988 Hz, 0.99999976).

    The clamps run in double on the widened float, against double limits. The
    float nearest 0.025 (0.025f) lies just ABOVE it, at about 0.025 + 3.7e-10,
    so the lower Q clamp does not act on the lowest raw Q: it reads
    (double) 0.025f here, not 0.025.

    classifyBandChange is the second level of the processor's change detection
    (PLAN-2.2 P3): it runs only after a bit-pattern difference in the raw
    snapshot.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"
#include "Prototype.h"

#include <cmath>

namespace eq
{

//==============================================================================
// Raw values
//==============================================================================

/** The eight raw parameter values of one band, in registration order
    (bNN_enabled, bNN_bypass, bNN_type, bNN_freq, bNN_gain, bNN_q, bNN_slope,
    bNN_placement), exactly as the parameter atomics hold them: toggles 0 / 1,
    choices as an index, floats in engineering units. Eight floats and no
    padding, so two snapshots can be compared by bit pattern. */
struct RawBand
{
    float enabled;
    float bypass;
    float type;
    float freq;
    float gain;
    float q;
    float slope;
    float placement;
};

inline constexpr int numRawBandValues = 8;

static_assert (sizeof (RawBand) == static_cast<std::size_t> (numRawBandValues) * sizeof (float),
               "RawBand is compared by bit pattern: eight floats, no padding");

/** Where a band acts. The value is the bNN_placement choice index. */
enum class Placement : int { stereo, left, right, mid, side };

inline constexpr int numPlacements = 5;

/** One band as the engine sees it: what the designer reads, where the result
    goes, and whether the band runs at all. Always in canonical form when it
    comes from makeBandInput. */
struct BandInput
{
    BandSettings settings;
    Placement placement;
    bool active;
};

//==============================================================================
// Defaults and limits (parameter-spec; the layout is frozen at 0.1.0)
//==============================================================================

inline constexpr double rawFreqMin = 10.0;
inline constexpr double rawFreqMax = 30000.0;
inline constexpr double rawFreqDefault = 1000.0;
inline constexpr double rawGainMin = -30.0;
inline constexpr double rawGainMax = 30.0;
inline constexpr double rawGainDefault = 0.0;
inline constexpr double rawQMin = 0.025;
inline constexpr double rawQMax = 40.0;
inline constexpr double rawQDefault = 1.0;
inline constexpr double rawScaleMin = 0.0;
inline constexpr double rawScaleMax = 200.0;
inline constexpr double rawScaleDefault = 100.0;
inline constexpr double gainEffMin = -60.0;
inline constexpr double gainEffMax = 60.0;
inline constexpr int rawTypeDefault = 0;
inline constexpr int rawSlopeDefault = 1;
inline constexpr int rawPlacementDefault = 0;

namespace detail
{
    inline double clampTo (double v, double lo, double hi) noexcept
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    /** Non-finite -> the default; otherwise widened and clamped. */
    inline double sanitisedValue (float raw, double defaultValue, double lo, double hi) noexcept
    {
        if (! std::isfinite (raw))
            return defaultValue;

        return clampTo (static_cast<double> (raw), lo, hi);
    }

    /** Non-finite -> the default index; otherwise clamped to the index range
        and rounded to nearest, a half rounding up. The sum is formed in
        double, where it is exact. */
    inline int sanitisedChoice (float raw, int defaultIndex, int numChoices) noexcept
    {
        if (! std::isfinite (raw))
            return defaultIndex;

        const double top = static_cast<double> (numChoices - 1);
        const double v = clampTo (static_cast<double> (raw), 0.0, top);
        return static_cast<int> (v + 0.5);
    }

    /** Non-finite -> off; otherwise on from 0.5. */
    inline bool sanitisedToggle (float raw) noexcept
    {
        return std::isfinite (raw) && raw >= 0.5f;
    }
}

//==============================================================================
// Exact comparison; the offset sum
//==============================================================================

/** Equality of two finite values, written with the ordering operators so that
    it is exact and reads as intended; +0 and -0 compare equal. */
constexpr bool sameValue (double a, double b) noexcept
{
    return ! (a < b) && ! (a > b);
}

/** clamp (gainDb + offsetDb, -60, +60): the one place a dynamic gain offset
    is summed into a gain (PLAN-2.3 P5). A non-finite offset is read as 0,
    and an offset of exactly 0 is not added at all, so the result is then the
    clamped gain, bit for bit. Callers: makeBandInput step 3 and the band's
    control tick. Not meant for the four gainless types (isGainType false):
    their gain is canonically 0 and the offset is ignored. */
inline double offsetGainDb (double gainDb, double offsetDb) noexcept
{
    const double offset = std::isfinite (offsetDb) ? offsetDb : 0.0;
    const double summed = sameValue (offset, 0.0) ? gainDb : gainDb + offset;
    return detail::clampTo (summed, gainEffMin, gainEffMax);
}

//==============================================================================
// Raw -> canonical band input
//==============================================================================

/** raw: the band's eight values. gainScaleRaw: the raw gain_scale value in
    percent. dynOffsetDb: the band's dynamic gain offset in dB (0 in v1;
    a non-finite offset is read as 0).

    The product, the quotient and the sum sit in separate statements. At
    scale 100 the product of a float gain and 100 is exact in double and so
    is the quotient, and a zero offset is not added at all: gainDbEff is then
    the float gain, bit for bit. */
inline BandInput makeBandInput (const RawBand& raw, float gainScaleRaw, double dynOffsetDb) noexcept
{
    // 1, 2: sanitise and clamp
    const bool enabled = detail::sanitisedToggle (raw.enabled);
    const bool bypass = detail::sanitisedToggle (raw.bypass);
    const int typeIndex = detail::sanitisedChoice (raw.type, rawTypeDefault, numTypes);
    const double freqHz = detail::sanitisedValue (raw.freq, rawFreqDefault, rawFreqMin, rawFreqMax);
    const double gainDb = detail::sanitisedValue (raw.gain, rawGainDefault, rawGainMin, rawGainMax);
    const double q = detail::sanitisedValue (raw.q, rawQDefault, rawQMin, rawQMax);
    const int slopeIndex = detail::sanitisedChoice (raw.slope, rawSlopeDefault, numSlopes);
    const int placementIndex = detail::sanitisedChoice (raw.placement, rawPlacementDefault, numPlacements);
    const double scale = detail::sanitisedValue (gainScaleRaw, rawScaleDefault, rawScaleMin, rawScaleMax);

    // 3: effective gain
    const double product = gainDb * scale;
    const double scaled = product / 100.0;
    const double gainDbEff = offsetGainDb (scaled, dynOffsetDb);

    BandInput in;
    in.settings.type = static_cast<Type> (typeIndex);
    in.settings.freqHz = freqHz;
    in.settings.gainDbEff = gainDbEff;
    in.settings.q = q;
    in.settings.slopeIndex = slopeIndex;
    in.placement = static_cast<Placement> (placementIndex);

    // 4
    in.active = enabled && ! bypass;

    // 5: canonical form
    if (! usesSlope (in.settings.type))
        in.settings.slopeIndex = 0;
    else if (in.settings.slopeIndex == 0)
        in.settings.q = 1.0;

    if (! isGainType (in.settings.type))
        in.settings.gainDbEff = 0.0;

    return in;
}

//==============================================================================
// Placement on the negotiated layout
//==============================================================================

/** The placement that takes effect: Stereo on a mono layout, whatever the
    parameter says. The processor stores and compares this one, so a
    placement edit on mono compares as "identical". */
constexpr Placement effectivePlacement (Placement placement, bool stereoLayout) noexcept
{
    return stereoLayout ? placement : Placement::stereo;
}

//==============================================================================
// Change detection, second level (PLAN-2.2 P3)
//==============================================================================

/** Field-by-field equality. Every value is finite after makeBandInput, so
    the comparison is exact; +0 and -0 compare equal. */
inline bool sameBandInput (const BandInput& a, const BandInput& b) noexcept
{
    return a.active == b.active
        && a.placement == b.placement
        && a.settings.type == b.settings.type
        && a.settings.slopeIndex == b.settings.slopeIndex
        && sameValue (a.settings.freqHz, b.settings.freqHz)
        && sameValue (a.settings.gainDbEff, b.settings.gainDbEff)
        && sameValue (a.settings.q, b.settings.q);
}

/** identical    nothing to do: no design, no state change (D6, D9)
    structural   active, type, slopeIndex or placement differs: design, and
                 the band's state is cleared
    continuous   anything else (frequency, effective gain, Q): state kept; since
                 Phase 2.3 a 20 ms ramp to the fresh settings (Band::setTarget).
                 A gain or gain_scale edit on a gainless type never gets here:
                 it compares as identical (step 5 of makeBandInput) */
enum class BandChange : int { identical, structural, continuous };

/** Both inputs in canonical form, and both carrying the EFFECTIVE placement
    (see effectivePlacement): the caller maps it before calling. */
inline BandChange classifyBandChange (const BandInput& current, const BandInput& next) noexcept
{
    if (sameBandInput (current, next))
        return BandChange::identical;

    if (current.active != next.active
        || current.placement != next.placement
        || current.settings.type != next.settings.type
        || current.settings.slopeIndex != next.settings.slopeIndex)
        return BandChange::structural;

    return BandChange::continuous;
}

//==============================================================================
// Added in Phase 2.6 (PLAN-2.6 P1): the one raw -> effective input
//==============================================================================

/** makeBandInput (raw, gainScaleRaw, 0.0), then the placement mapped by
    effectivePlacement for the layout: the band input the processor's
    makeInput builds (the same two statements, in the same order) and the
    display model builds, from one definition. The offset argument is 0: each
    slot reads its own dynamic offset (Band.h). */
inline BandInput effectiveBandInput (const RawBand& raw, float gainScaleRaw, bool stereoLayout) noexcept
{
    BandInput input = makeBandInput (raw, gainScaleRaw, 0.0);
    input.placement = effectivePlacement (input.placement, stereoLayout);
    return input;
}

} // namespace eq
