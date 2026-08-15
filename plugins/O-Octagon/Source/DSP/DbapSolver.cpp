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
#include "DbapSolver.h"

// NOTHING ELSE MAY BE INCLUDED HERE. DbapSolver.h pulls <cmath>, <atomic>, <cstdint> and Vec.h and
// that is the whole dependency set. The unit target's link line carries no juce_audio_processors and
// no juce_dsp; a new include that drags either in is a design error in this TU, not in the CMake
// (PLAN-2.2 P19 / Task 8).

namespace oo::dbap
{

//==============================================================================
float blurToRadius (float blur, float rigScale) noexcept
{
    const float r = blur * kBlurScale * rigScale;

    // Two independent ceilings, both deliberate: the exposed 0-1 range IS the musical cap
    // (blur = 1 → half the RMS rig radius), and kMaxBlurMetres is an absolute backstop so a
    // mistyped 1000 m coordinate cannot produce an r_s that swamps the array (§3.3.2).
    return r < kMaxBlurMetres ? r : kMaxBlurMetres;
}

float rolloffToAlpha (float rolloff) noexcept
{
    // eq 4. At the exposed extremes: R = 3 → a = 0.49829, R = 6 → a = 0.99658.
    return rolloff * kInvTwentyLog10Two;
}

//==============================================================================
void solve (const Vec3 spk[kNumSpeakers], const float w[kNumSpeakers], Vec3 src,
            float a, float rs, float outV[kNumSpeakers],
            float* outInvK) noexcept
{
    const float rsSquared = rs * rs;

    float t[kNumSpeakers];
    float denom = 0.0f;

    for (int i = 0; i < kNumSpeakers; ++i)
    {
        const float dx = spk[i].x - src.x;
        const float dy = spk[i].y - src.y;
        const float dz = spk[i].z - src.z;   // DSP-01/2: the 3D term. A dropped (z_i − z_s)² is
                                             // invisible on a flat rig, which is why §OQ4's default
                                             // heights are GRADED 4.50 → 5.40 m. Do not flatten them.

        // §3.3.3 — the floor is applied UNCONDITIONALLY, on every path, including blur = 0 with the
        // source sitting exactly on a speaker (QUAL-02 criterion 1). A conditional floor is the same
        // bug with a longer fuse.
        const float dRaw = std::sqrt (dx * dx + dy * dy + dz * dz + rsSquared);
        const float d    = dRaw < kMinDistance ? kMinDistance : dRaw;

        // ONE pow per speaker. `t` is d^(−a); `t²` is d^(−2a), which is what the denominator wants.
        // Calling pow twice would double the §3.3.5 budget for no gain — probe AE asserts the exact
        // count, because the ≤ 32 bound alone still passes if this reuse is dropped.
        t[i] = countedPow (d, -a);

        const float wt = w[i] * t[i];
        denom += wt * wt;
    }

    // §3.3.4 — all-zero weights. Without this, denom = 0 → k = inf → v_i = inf · 0 = NaN, which
    // propagates into the SmoothedValue targets and LATCHES PERMANENTLY. Silence is written
    // explicitly: not a NaN, not full scale (DSP-05/3).
    if (denom < kDenomEpsilon)
    {
        for (int i = 0; i < kNumSpeakers; ++i)
            outV[i] = 0.0f;

        // 0.0f, not a sentinel and not left uninitialised. A rig with no active speaker really does
        // produce a zero field, and UI-04's colour map must be able to say so (P69).
        if (outInvK != nullptr)
            *outInvK = 0.0f;

        return;
    }

    // sqrtDenom is 1/k, and it is what UI-04 draws (P69 / N10). Computed as the sqrt this line
    // already needed — the reciprocal below is the only extra arithmetic in the whole feature, and
    // NO NEW std::pow IS INTRODUCED, so probe AE's powCalls == 16 is untouched.
    //
    // It is written through a pointer rather than returned because the AUDIO PATH passes nullptr:
    // the field is a message-thread diagnostic and must cost the RT path nothing but a branch that
    // is perfectly predicted (P54's precedent, one phase on).
    const float sqrtDenom = std::sqrt (denom);
    const float k         = 1.0f / sqrtDenom;

    if (outInvK != nullptr)
        *outInvK = sqrtDenom;

    for (int i = 0; i < kNumSpeakers; ++i)
        outV[i] = k * w[i] * t[i];   // w_i = 0 → EXACTLY 0.0f at speaker i (DSP-05/1)
}

} // namespace oo::dbap
