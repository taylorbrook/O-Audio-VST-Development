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

    O-ParametricEQ - section engine, settled and ramping paths
    (Stage 2, Phases 2.2 and 2.3)

    The two recurrences that realise a designed section, exactly as
    ARCHITECTURE "SVF section and one-pole (realisation)" writes them, and the
    runtime coefficients they run on. Double state throughout.

      SVF (state ic1, ic2)
        a1 = 1 / (1 + g (g + k));  a2 = g a1;  a3 = g a2
        v3 = x - ic2
        v1 = a1 ic1 + a2 v3
        v2 = ic2 + a2 ic1 + a3 v3
        ic1 = 2 v1 - ic1;  ic2 = 2 v2 - ic2
        y  = mL v2 + mB v1 + mH (x - k v1 - v2)

      One-pole TPT (state s)
        Gc = G / (1 + G)
        v = (x - s) Gc;  lp = v + s;  s = lp + v
        y = cL lp + cH (x - lp)

    Settled path (2.2). The coefficients are converted once per design
    (toRuntime); the settled path never divides. A section runs only the
    halves its flags name, the SVF half first.

    Ramping path (2.3). Between two control ticks the eight section
    parameters g, k, mL, mB, mH, G, cL, cH move linearly, one increment per
    sample (sectionIncrements). The ramping tick is, per sample and per
    section:

        add the increments to g, k, mL, mB, mH, G, cL, cH       (rampSection)
        a1 = 1 / (1 + g (g + k));  a2 = g a1;  a3 = g a2
        Gc = G / (1 + G)
        then the same two recurrences as above                  (svfTick,
                                                                 onePoleTick)

    The parameters are advanced BEFORE use, so the last sample of an interval
    runs on (the accumulated sum for) the tick's design. rampSection is the
    parameter half: it writes the section's runtime coefficients, and every
    lane of the band then runs the settled recurrences on them. The
    coefficients therefore advance once per sample, never once per lane
    (PLAN-2.3 P4).

    a1, a2, a3 are always derived from ONE interpolated (g, k) pair, by
    division, every sample. That keeps every per-sample state matrix in the
    family whose Euclidean norm is exactly 1 for g, k > 0, which is what the
    time-varying stability argument rests on (RESEARCH-2.3 Q2). Linear
    interpolation between two designs with g, k, G > 0 stays positive. Do not
    interpolate a1, a2, a3 or Gc themselves.

    Both halves have a direct path, so the first output sample of a section at
    rest is its input times a constant: the first-sample gains below (gate G8).

    Callers: Band.h. The state lives with the caller; these functions are
    pure in their coefficients.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"

namespace eq
{

/** Runtime coefficients of the SVF half. */
struct SvfCoeffs
{
    double a1 = 1.0;
    double a2 = 0.0;
    double a3 = 0.0;
    double k  = 1.0;
    double mL = 1.0;
    double mB = 1.0;
    double mH = 1.0;
};

/** Runtime coefficients of the one-pole half. */
struct OnePoleCoeffs
{
    double Gc = 0.0;
    double cL = 1.0;
    double cH = 1.0;
};

/** One section as the engine runs it. A half whose flag is false is skipped
    and its coefficients are not read. */
struct SectionRuntime
{
    SvfCoeffs svf;
    OnePoleCoeffs onePole;
    bool hasSvf = false;
    bool hasOnePole = false;
};

/** Designer output -> runtime coefficients. The one place the divisions
    happen. Needs g, k, G > 0 and finite, which the designer guarantees. */
inline SectionRuntime toRuntime (const SectionParams& p) noexcept
{
    SectionRuntime r;
    r.hasSvf = p.hasSvf;
    r.hasOnePole = p.hasOnePole;

    r.svf.a1 = 1.0 / (1.0 + p.g * (p.g + p.k));
    r.svf.a2 = p.g * r.svf.a1;
    r.svf.a3 = p.g * r.svf.a2;
    r.svf.k = p.k;
    r.svf.mL = p.mL;
    r.svf.mB = p.mB;
    r.svf.mH = p.mH;

    r.onePole.Gc = p.G / (1.0 + p.G);
    r.onePole.cL = p.cL;
    r.onePole.cH = p.cH;
    return r;
}

//==============================================================================
// Recurrences, one sample
//==============================================================================

/** One sample of the SVF half. Updates ic1 and ic2. */
inline double svfTick (const SvfCoeffs& c, double& ic1, double& ic2, double x) noexcept
{
    const double v3 = x - ic2;
    const double v1 = c.a1 * ic1 + c.a2 * v3;
    const double v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
    ic1 = 2.0 * v1 - ic1;
    ic2 = 2.0 * v2 - ic2;
    return c.mL * v2 + c.mB * v1 + c.mH * (x - c.k * v1 - v2);
}

/** One sample of the one-pole half. Updates s. */
inline double onePoleTick (const OnePoleCoeffs& c, double& s, double x) noexcept
{
    const double v = (x - s) * c.Gc;
    const double lp = v + s;
    s = lp + v;
    return c.cL * lp + c.cH * (x - lp);
}

//==============================================================================
// Ramping path (Phase 2.3): the parameter half of the ramping tick
//==============================================================================

/** Per-sample increments of the eight section parameters. */
struct SectionIncrements
{
    double g  = 0.0;
    double k  = 0.0;
    double mL = 0.0;
    double mB = 0.0;
    double mH = 0.0;
    double G  = 0.0;
    double cL = 0.0;
    double cH = 0.0;
};

/** (to - from) * perSample for each of the eight parameters; perSample is
    1 / K for an interval of K samples. A half the section does not have holds
    the same identity values on both sides, so its increments are 0. */
inline SectionIncrements sectionIncrements (const SectionParams& from, const SectionParams& to,
                                            double perSample) noexcept
{
    SectionIncrements d;
    d.g  = (to.g  - from.g)  * perSample;
    d.k  = (to.k  - from.k)  * perSample;
    d.mL = (to.mL - from.mL) * perSample;
    d.mB = (to.mB - from.mB) * perSample;
    d.mH = (to.mH - from.mH) * perSample;
    d.G  = (to.G  - from.G)  * perSample;
    d.cL = (to.cL - from.cL) * perSample;
    d.cH = (to.cH - from.cH) * perSample;
    return d;
}

/** One sample of parameter motion for one section: adds the increments to
    the parameters of the halves the section has, then derives that half's
    runtime coefficients from them, in the statements of toRuntime. The flags
    of `rt` are not written; the caller set them when it took the design.
    Call once per sample, before the recurrences of every lane. */
inline void rampSection (SectionParams& p, const SectionIncrements& d, SectionRuntime& rt) noexcept
{
    if (p.hasSvf)
    {
        p.g  += d.g;
        p.k  += d.k;
        p.mL += d.mL;
        p.mB += d.mB;
        p.mH += d.mH;

        rt.svf.a1 = 1.0 / (1.0 + p.g * (p.g + p.k));
        rt.svf.a2 = p.g * rt.svf.a1;
        rt.svf.a3 = p.g * rt.svf.a2;
        rt.svf.k = p.k;
        rt.svf.mL = p.mL;
        rt.svf.mB = p.mB;
        rt.svf.mH = p.mH;
    }

    if (p.hasOnePole)
    {
        p.G  += d.G;
        p.cL += d.cL;
        p.cH += d.cH;

        rt.onePole.Gc = p.G / (1.0 + p.G);
        rt.onePole.cL = p.cL;
        rt.onePole.cH = p.cH;
    }
}

//==============================================================================
// First-sample gain (gate G8)
//==============================================================================

/** Output of the SVF half at rest for a unit input sample:
    mL a3 + mB a2 + mH (1 - k a2 - a3). Written in the shape of svfTick with
    x = 1 and zero state. */
inline double svfFirstSampleGain (const SvfCoeffs& c) noexcept
{
    return c.mL * c.a3 + c.mB * c.a2 + c.mH * (1.0 - c.k * c.a2 - c.a3);
}

/** Output of the one-pole half at rest for a unit input sample:
    cL Gc + cH (1 - Gc). */
inline double onePoleFirstSampleGain (const OnePoleCoeffs& c) noexcept
{
    return c.cL * c.Gc + c.cH * (1.0 - c.Gc);
}

/** First-sample gain of one section: the product over the halves it has. */
inline double sectionFirstSampleGain (const SectionRuntime& s) noexcept
{
    double gain = 1.0;

    if (s.hasSvf)
        gain *= svfFirstSampleGain (s.svf);

    if (s.hasOnePole)
        gain *= onePoleFirstSampleGain (s.onePole);

    return gain;
}

} // namespace eq
