/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include "../DSP/Vec.h"

namespace oo
{

//==============================================================================
/**
    Free functions over the four published venue scalars — the audience plane and the bounding-box
    denormalisation, expressed so the audio thread and the message thread provably run the SAME
    arithmetic (PLAN-2.2 P14).

    This is the `hull::` precedent applied a second time. `VenueModel` (message thread) holds the
    venue and delegates to these; `SourceShaper` (audio thread) holds a `VenueSnapshot` and calls
    them directly. There is no second implementation to drift.

    Header-only, no JUCE, no allocation, no branching beyond the two zero-span guards. It is NOT
    added to target_sources, exactly like Vec.h and VenueSnapshot.h.
*/
namespace plane
{
    /** Below this span an axis is degenerate.

        THE SINGLE DEFINITION. `VenueModel::kMinSpan` aliases it rather than declaring a second
        constant that happens to agree.
    */
    inline constexpr float kMinSpan = 1.0e-6f;

    /** Speed of sound in air, metres per second — 343 m/s, dry air at 20 °C.

        THE SINGLE DEFINITION, and it is here rather than in VenueModel for the reason every other
        constant in this header is here: two consumers derive from it and they must not be able to
        disagree. `VenueModel::suggestedDelaysMs()` turns metres of path difference into
        milliseconds; the Venue table's metres/ms toggle turns the operator's typed metres back into
        the stored milliseconds. A second literal in venue.js would be a mirrored fixture over this
        one (pattern_test_fixture_mirrors_drift_silently), which is why the page is handed the
        number rather than holding its own.

        DELIBERATELY NOT TEMPERATURE-DEPENDENT. c varies about 0.6 m/s per °C, so a 10 °C error is
        1.7 % — roughly 0.35 ms across a 20 m hall, an order of magnitude below what the operator's
        own distance measurement contributes. A temperature field would look like precision it
        cannot deliver.
    */
    inline constexpr float kSpeedOfSoundMps = 343.0f;

    /** The alignment-delay rail, in milliseconds — THE SINGLE DEFINITION.

        THREE consumers need this number and they sit in three layers that cannot include each
        other: `VenueModel::kMaxSuggestedDelayMs` clamps the derived suggestion,
        `GainStage::kMaxAlignDelayMs` sizes the eight delay lines, and
        `OOctagonProcessor::kVenueDelayClampMs` rails what reaches the audio thread. All three
        ALIAS this, exactly as `VenueModel::kMinSpan` aliases kMinSpan above.

        The rejected alternative was three 50.0f literals tied together by static_asserts. It
        works, and it produced a -Wfloat-equal warning on every build — because comparing two
        floats for equality IS the smell the warning names, even when the floats are constants.
        One definition needs no comparison. (A suggestion that proposed a value the funnel would
        then clamp to something different is the failure all of this exists to prevent: the
        operator sees a number in the table that the audio thread does not honour.)

        50 ms is ~17 m of path difference at 343 m/s — larger than any single-array skew a hall of
        the size this plugin addresses can produce, and small enough that a mis-typed coordinate
        rails visibly instead of introducing a quarter-second of delay nobody asked for.
    */
    inline constexpr float kMaxAlignDelayMs = 50.0f;

    //==========================================================================
    /** Audience-plane ear height at depth `y`.

        Linear from `rakeFront` at `bbMinY` to `rakeRear` at `bbMaxY`, EXTRAPOLATED linearly outside
        that range — deliberately unclamped, so a source placed behind the rear speakers keeps
        rising with the rake instead of flattening at a seam.

        Zero-span guard: all eight speakers at one depth is a legitimate venue-entry state, not a
        programming error. The plane collapses to the constant `rakeFront` (QUAL-02).
    */
    inline float earHeight (float rakeFront, float rakeRear,
                            float bbMinY, float bbMaxY, float y) noexcept
    {
        const float span = bbMaxY - bbMinY;

        if (span < kMinSpan)
            return rakeFront;

        return rakeFront + (rakeRear - rakeFront) * ((y - bbMinY) / span);
    }

    /** Normalised srcX/srcY (0..1, the host-facing parameter range) → metres, via the speaker
        bounding box.

        CARRIES ITS OWN ZERO-SPAN GUARD, PER AXIS, independently of earHeight's. The two
        degeneracies are not the same state: a rig with all eight speakers at one x has a perfectly
        good rake and a zero-width bbox, so collapsing the two guards into one would leave this path
        dividing by zero. A degenerate axis pins to its minimum.
    */
    inline Vec2 normToMetres (float bbMinX, float bbMaxX, float bbMinY, float bbMaxY,
                              float nx, float ny) noexcept
    {
        const float spanX = bbMaxX - bbMinX;
        const float spanY = bbMaxY - bbMinY;

        return { spanX < kMinSpan ? bbMinX : bbMinX + nx * spanX,
                 spanY < kMinSpan ? bbMinY : bbMinY + ny * spanY };
    }
}

} // namespace oo
