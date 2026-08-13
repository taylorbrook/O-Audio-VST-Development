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

#include <array>
#include <cstdint>
#include <vector>

#include "../Data/VenueSnapshot.h"
#include "DbapSolver.h"
#include "HullProcessor.h"
#include "Vec.h"

namespace oo
{

//==============================================================================
/**
    UI-04's DBAP level field, sampled over the room on the MESSAGE THREAD.

    ── dbap::solve IS MESSAGE-THREAD SAFE, AND NO SECOND INSTANCE EXISTS TO NEED (Q1) ────────────
    It is a free function: no instance, no state, `noexcept`, no allocation, no JUCE. Calling it
    from the editor is therefore not a threading question at all. The only shared mutable state in
    that TU is the `oo::instr` counter block, which exists ONLY under OOCTAGON_INSTRUMENT — see the
    qualification below.

    ── THE FIELD FOLLOWS THE FULL CHAIN, NOT dbap::solve ALONE (P73) ─────────────────────────────
    `shaper::shape` → hull-project if outside → `dbap::solve` → `hullTrimGain`. Sampling the solve
    alone would match the plugin INSIDE the hull and diverge from it OUTSIDE, where `hullAtten` is
    audible — and the risk UI-04 exists to avoid is a picture the solver does not produce. Either
    way the comparison is against shipping functions; this way it is against the shipping CHAIN.

    THE SUB-POINT SPLIT IS DELIBERATELY NOT APPLIED. `width` moves the two sub-points apart, and
    the field is a property of the ROOM AND THE RIG rather than of where the source currently is —
    which is exactly why srcX/srcY/srcZ/width are not inputs to it (N12) and why UI-04 criterion 2's
    assertion is the puck one. Each grid point IS the solve position.

    ── 32 x 40, AND THE CONSTRAINT IS THE BRIDGE (Q2, measured) ──────────────────────────────────
    `pow` is NOT the bottleneck: 112 x 140 is 125,440 pow calls in 660 us on the message thread.
    THE PAYLOAD IS. A float per cell is 61 kB of JSON per recompute through a transport that
    serialises every value; 32 x 40 quantised to 8 bits and base64'd is 1.7 kB, at 183 us.

    QUANTISATION HAPPENS DOWNSTREAM OF THIS CLASS'S FLOAT OUTPUT, in a separate function, so
    UI-04 criterion 1's twenty-point comparison against a direct solve to 1e-3 (probe CB) asserts
    against the FLOAT field and is strictly upstream of transport. Asserting anywhere downstream
    would be asserting against a re-derivation, which the criterion forbids.

    ── THE RECOMPUTE COUNTER IS HERE, IN C++ (P74) ───────────────────────────────────────────────
    UI-04 criterion 2 requires a COUNTER rather than an eye, and D19's rule — assert against what
    C++ returned — applies to this as much as to scene membership. Probe CD drives all five inputs
    and the four non-inputs against it.

    ── ONE QUALIFICATION, AND IT IS REAL ─────────────────────────────────────────────────────────
    Under OOCTAGON_INSTRUMENT a field sample POLLUTES `powCalls`, and probe AE asserts
    `powCalls == 16` EXACTLY per control block. Any probe that samples a field in the same process
    as AE MUST call `instr::resetCounters()` between them — the render harness already does this at
    eleven sites, so it is a convention to follow rather than one to invent. PROBE CE ASSERTS THE
    DISCIPLINE HOLDS, so a probe inserted between them later fails loudly instead of silently
    inflating AE's figure.

    `solveRuns` is NOT at risk: `countSolveRun()` lives in `GainStage::updateControl`, not inside
    `solve`.
*/
class FieldSampler
{
public:
    //==========================================================================
    /** Grid resolution. 32 x 40 is portrait, matching the default envelope's 0.800 aspect closely
        enough that the blit's bilinear smoothing does not stretch the field on one axis. */
    static constexpr int kCols = 32;
    static constexpr int kRows = 40;

    static constexpr int kNumCells = kCols * kRows;

    static_assert (kCols > 1 && kRows > 1,
                   "a single-cell grid divides by zero in the cell-centre mapping below, and a "
                   "1-wide field would blit as a flat wash that looks exactly like a working one");

    //==========================================================================
    /** One sampled field. Floats, in the solver's own units — `1/k = sqrt(denom)`, times the hull
        trim. NOT dB and NOT normalised: probe CB compares these against a direct solve. */
    struct Field
    {
        std::array<float, kNumCells> cell {};

        /** The observed extremes of THIS recompute, in dB relative to the weakest cell. They are
            what the UI's colour ramp normalises against and what the legend prints.

            AN ABSOLUTE 0..1 MAP WOULD BE A LIE HERE. The field over a raked audience plane is
            GENUINELY FLAT — every grid point is at z = 0 while the speakers are 4.50-5.40 m up, so
            the minimum 3-D distance is >= 4.5 m in a 12 x 15 m hall. A fixed map renders a uniform
            wash WHILE LOOKING AS THOUGH IT CARRIES INFORMATION, which is CONTEXT-3.3's
            "beautiful and wrong" risk arriving from an unexpected direction.
        */
        float minValue { 0.0f };
        float maxValue { 0.0f };
        float spanDb   { 0.0f };

        /** True when every cell is 0 — a rig with no active speaker. The UI draws nothing rather
            than normalising a span of zero into a full-scale wash. */
        bool isSilent { true };
    };

    //==========================================================================
    FieldSampler() = default;

    /** Samples the field over the SPEAKER BOUNDING BOX at the audience plane.

        The bbox and not the envelope: the envelope's margin is a DRAWING convention (bbox + 15 %,
        ARCHITECTURE §6.2) and sampling into it would spend a quarter of the grid on floor the rig
        does not cover. The UI blits this onto the plan box, which spans the envelope, so the
        caller maps the two — one projection, in js/roomplan.js, as ever.

        @param v          the published room. Speaker positions, the hull, the bbox.
        @param w          the eight weights, as the parameters currently hold them
        @param a          rolloff exponent, from dbap::rolloffToAlpha()
        @param rs         blur radius in metres, from dbap::blurToRadius()
        @param hullAtten  dB per metre outside the hull

        INCREMENTS THE RECOMPUTE COUNTER. That is the whole reason it is not `const`.
    */
    Field sample (const VenueSnapshot& v, const float w[dbap::kNumSpeakers],
                  float a, float rs, float hullAtten) noexcept;

    /** UI-04 criterion 2's counter, read by probe CD and by the page. */
    std::uint64_t recomputeCount() const noexcept { return recomputes; }

    //==========================================================================
    /** 8-bit quantisation against the field's OWN observed range, then base64.

        DOWNSTREAM OF THE FLOAT OUTPUT, deliberately and separately (P73): UI-04/1's 1e-3
        comparison is made on `Field::cell` and never on these bytes, so the quantisation cannot
        weaken the criterion. A silent field encodes as all zeros.
    */
    static std::vector<std::uint8_t> quantise (const Field& f);

private:
    std::uint64_t recomputes { 0 };
};

} // namespace oo
