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
