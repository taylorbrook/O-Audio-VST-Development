/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    Source/DSP/SubHarmonicBias.h
    O-Contrabass Phase 2.4b — Sub-Harmonic Bias DSP-07 (ARCHITECTURE §457).

    Friction-junction parameter biasing toward the Schelleng F_max regime to
    induce period-doubling f0/2 spectral content as a musical bass-extension
    feature on the SUB_HARMONICS APVTS knob (Float [0, 1] default 0.0).

    Per-plugin (NOT extracted to shared module per CONTEXT rev-7 Q24 — friction
    module v1.0.0 untouched per HR-10).

    HR-9 (Phase 2.4b hard rule): SUB_HARMONICS=0 IEEE 754 identity arithmetic
    + active-string-only gate. The caller (BowedContrabassVoice.cpp Step 2.5)
    MUST short-circuit BEFORE invoking applyBias() at subAmount==0.0f and for
    non-active-string slots. This function does NOT defensively short-circuit;
    HR-9 is enforced at the caller for clarity of the bit-exact regression bar.

    HR-10 (Phase 2.4b hard rule): friction module ABI preservation. The bias
    function mutates F_bow / v_0 / mu_s by reference; the caller pushes the
    mutated values back to frictionModel via the EXISTING HyperbolicFriction.h
    setter surface — NO module v1.0.0 ABI change.

    Coefficients verbatim from architecture §457 (lines 465–470). Architecture
    §661 fallback 1 (`kForceBoost` 0.8 → 0.4) is mechanically delivered via the
    SchellengCalibration safeDepth-scaled `effectiveBoost` (RESEARCH §18.4):
    fallback cells (safeDepth=0.5) automatically halve the F_bow uplift.

  ==============================================================================
*/

#pragma once
#include <algorithm>

namespace ouaricon::contrabass::sub_harmonics {

// Architecture §457 / §661 default coefficients. Phase 2.4b ships verbatim;
// Phase 2.4-bis or 2.5/2.6 may retune to fallback 0.4 if Gate 6b spectral
// pre-flight (RESEARCH §18.6) returns hard-FAIL.
inline constexpr float kForceBoost  = 0.8f;     // F_bow uplift coefficient
                                                // (architecture §457 line 465 multiplier;
                                                // ×subAmount factored at call-site for
                                                // safeDepth interaction); architecture
                                                // §661 fallback 1 → 0.4f.
inline constexpr float kV0Reduction = 0.5f;     // v_0 contraction coefficient (line 466).
inline constexpr float kGapWiden    = 0.25f;    // mu_s − mu_d gap widening (line 468).
inline constexpr float kFmaxScalar  = 0.95f;    // Schelleng ceiling fraction (line 470).
inline constexpr float kV0Floor     = 0.005f;   // architecture-spec'd lower clamp (line 466).

// Schelleng F_max formula (architecture §490 — slow-LFO wedge, with Z=0.5 and
// the dimensionless collapse used by Phase 2.3/2.4a closed-form math):
//   fMax = (2·Z · v_b) / (beta · (mu_s − mu_d))
// Re-used here for the final F_bow ceiling clamp; SchellengCalibration table
// provides the stable-cell vs fallback-cell scaling on the uplift, NOT the
// hard ceiling.
inline float schellengFmax (float beta, float v_b, float mu_gap) noexcept
{
    constexpr float Z2 = 1.0f;                  // 2·Z = 2·0.5 = 1.0
    return (Z2 * v_b)
         / std::max (1.0e-6f, beta * std::max (1.0e-6f, mu_gap));
}

// applyBias mutates F_bow / v_0 / mu_s in-place per architecture §457.
// See header banner for HR-9 caller-side short-circuit contract.
inline void applyBias (float       subAmount,
                       int         stringIdx,
                       float       v_b,
                       float       beta,
                       float       safeDepth,
                       float&      F_bow,
                       float&      v_0,
                       float&      mu_s,
                       float       mu_d) noexcept
{
    // F_bow uplift, scaled by Phase 2.4a empirical safeDepth (RESEARCH §18.4
    // mapping). At stable cells (safeDepth=1.0) full kForceBoost=0.8 → ×1.8.
    // At fallback cells (safeDepth=0.5) → effectiveBoost=0.4 → ×1.4 = auto
    // architecture §661 fallback 1.
    const float effectiveBoost = subAmount * kForceBoost * safeDepth;
    F_bow *= 1.0f + effectiveBoost;

    // v_0 contraction (architecture §457 line 466 verbatim, NOT scaled by
    // safeDepth per §18.4 rationale — v_0 sharpens stick-slip nonlinearity,
    // NOT the F_bow ceiling).
    v_0 = std::max (kV0Floor, v_0 * (1.0f - kV0Reduction * subAmount));

    // mu_s gap widen (architecture §457 line 468 verbatim).
    const float gap = mu_s - mu_d;
    mu_s = mu_d + gap * (1.0f + kGapWiden * subAmount);

    // Schelleng F_max ceiling (architecture line 470). Mu_gap from POST-bias
    // mu_s for accurate post-bias wedge computation.
    const float muGapPost = mu_s - mu_d;
    F_bow = std::min (F_bow, kFmaxScalar * schellengFmax (beta, v_b, muGapPost));

    // stringIdx ignored at v1.0 (CONTEXT rev-7 Q11 per-string variation deferred).
    (void) stringIdx;
}

} // namespace ouaricon::contrabass::sub_harmonics
