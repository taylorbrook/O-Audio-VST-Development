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
#include "FieldSampler.h"

#include "../Data/VenueGeometry.h"
#include "ConvexHull2D.h"

#include <cmath>

// ── ON THE INCLUDE SET, AND A RECORDED DEVIATION FROM PLAN-3.3 TASK 2 ─────────────────────────
// The plan named DbapSolver.h / Vec.h / HullProcessor.h / VenueSnapshot.h. Two more are needed and
// both are inside the chain the plan itself specifies:
//
//   ConvexHull2D.h   — hull::isInside() and hull::project() ARE the "hull-project if outside" step.
//                      HullProcessor.h carries only the two outside-hull LAWS (the trim and the
//                      cutoff curve), never the geometry.
//   VenueGeometry.h  — plane::earHeight() puts each grid point on the audience plane. Header-only,
//                      no JUCE, and already reached by VenueModel.cpp in this same target.
//
// Both are already compiled into the unit target and NEITHER WIDENS THE LINK LINE — ConvexHull2D.cpp
// has been in it since Phase 2.1 and VenueGeometry.h is header-only. Gate 11 re-verifies that
// rather than trusting this comment.

namespace oo
{

//==============================================================================
FieldSampler::Field FieldSampler::sample (const VenueSnapshot& v, const float w[dbap::kNumSpeakers],
                                          float a, float rs, float hullAtten) noexcept
{
    ++recomputes;

    Field f {};

    const float spanX = v.bbMaxX - v.bbMinX;
    const float spanY = v.bbMaxY - v.bbMinY;

    // A degenerate rig has no area to sample. Returning a silent field is the honest answer and it
    // keeps the caller's normalisation from dividing by a zero span — the same decision
    // VenueModel makes when it pins a degenerate axis rather than dividing (QUAL-02).
    if (! (spanX > plane::kMinSpan) || ! (spanY > plane::kMinSpan))
        return f;

    float lo = 0.0f;
    float hi = 0.0f;
    bool  any = false;

    for (int row = 0; row < kRows; ++row)
    {
        // CELL CENTRES, not cell corners. Sampling the corners puts the first column exactly on
        // bbMinX, where a speaker may sit, and dbap::solve's kMinDistance floor would then flatten
        // the whole edge of the picture into one value.
        const float ty = (static_cast<float> (row) + 0.5f) / static_cast<float> (kRows);
        const float y  = v.bbMinY + ty * spanY;

        for (int col = 0; col < kCols; ++col)
        {
            const float tx = (static_cast<float> (col) + 0.5f) / static_cast<float> (kCols);
            const float x  = v.bbMinX + tx * spanX;

            // ── THE FULL CHAIN, THROUGH THE SHIPPING FUNCTIONS ────────────────────────────────
            //
            // Step 1: the point sits ON THE AUDIENCE PLANE — srcZ = 0 riding the rake, which is
            // DSP-04's own convention. plane::earHeight() is the SAME function VenueModel and
            // SourceShaper call, so the field's z and the solve's z cannot diverge.
            //
            // shaper::shape() is deliberately NOT called here: it maps a NORMALISED puck position
            // into metres and splits it by `width`, and this grid is already in metres and has no
            // source. Applying it would make the field depend on srcX/srcY/width, which are
            // precisely the four inputs N12 established it does NOT have — and would break
            // UI-04/2's puck assertion by construction.
            const float z = plane::earHeight (v.rakeFront, v.rakeRear, v.bbMinY, v.bbMaxY, y);

            const Vec3 p { x, y, z };

            // Step 2: hull-project if outside, and keep d_hull. INSIDE the hull d_hull is 0 and
            // hullTrimGain returns BIT-EXACT unity (RESEARCH-2.3 Q5), so the inside of the field is
            // the bare solve and the outside carries the trim — which is exactly the difference
            // that would be missing if the sampler called dbap::solve alone (P73).
            Vec3  solveAt = p;
            float dHull   = 0.0f;

            if (! hull::isInside (v.hullPts.data(), v.hullCount, { x, y }, v.hullEpsCross))
            {
                const auto proj = hull::project (v.hullPts.data(), v.hullCount, { x, y });
                solveAt = { proj.point.x, proj.point.y, z };
                dHull   = proj.distance;
            }

            // Step 3: the shipping solve, asking for the UN-NORMALISED field.
            float gains[dbap::kNumSpeakers] {};
            float invK = 0.0f;

            dbap::solve (v.spk.data(), w, solveAt, a, rs, gains, &invK);

            // Step 4: the shipping hull trim.
            const float value = invK * hullproc::hullTrimGain (hullAtten, dHull);

            f.cell[static_cast<std::size_t> (row * kCols + col)] = value;

            if (! any)
            {
                lo = hi = value;
                any = true;
            }
            else
            {
                lo = value < lo ? value : lo;
                hi = value > hi ? value : hi;
            }
        }
    }

    f.minValue = lo;
    f.maxValue = hi;

    // All-zero weights produce an identically zero field (dbap::solve's §3.3.4 early return writes
    // 0.0f through outInvK). That is DSP-05's silence, and the UI must draw nothing rather than
    // normalise a zero span into a full-scale wash.
    f.isSilent = ! (hi > 0.0f);

    // The span the legend prints. Against the WEAKEST cell rather than against full scale, because
    // the ramp is normalised per recompute and a viewer needs to know how much dynamic range the
    // picture is spending — 1.3 dB and 10.4 dB look identical without it.
    f.spanDb = f.isSilent || ! (lo > 0.0f)
                   ? 0.0f
                   : 20.0f * std::log10 (hi / lo);

    return f;
}

//==============================================================================
std::vector<std::uint8_t> FieldSampler::quantise (const Field& f)
{
    std::vector<std::uint8_t> out (static_cast<std::size_t> (kNumCells), 0);

    if (f.isSilent)
        return out;

    const float span = f.maxValue - f.minValue;

    // A uniform field — one active speaker in a tiny room, or a single-point rig — has no range to
    // stretch. Encoding it at mid scale says "uniform" rather than dividing by zero and painting
    // whatever NaN happens to quantise to.
    if (! (span > 0.0f))
    {
        for (auto& byte : out)
            byte = 128;

        return out;
    }

    for (int i = 0; i < kNumCells; ++i)
    {
        const float t = (f.cell[static_cast<std::size_t> (i)] - f.minValue) / span;
        const float c = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

        out[static_cast<std::size_t> (i)] =
            static_cast<std::uint8_t> (std::lround (c * 255.0f));
    }

    return out;
}

} // namespace oo
