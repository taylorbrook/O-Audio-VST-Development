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

    O-ParametricEQ - closed-form responses (Stage 2, Phase 2.1)

    Digital: the exact response of the realised SVF section and one-pole, from
    the section parameters alone (no coefficients, no rendering).

        SVF       |H|^2 = ((mL - mH X)^2 + mB^2 X) / ((1 - X)^2 + k^2 X),  X = tan^2 (w/2) / g^2
        one-pole  |H|^2 = (cL^2 + cH^2 Y) / (1 + Y),                       Y = tan^2 (w/2) / G^2

    w is in rad/sample and must satisfy 0 <= w < pi. w >= pi is NOT a valid
    evaluation point: tan (w/2) has its pole at w = pi and folds beyond it.

    Analog: the magnitude of an AnalogSection at the analog frequency w (same
    rad/sample scale, f = w fs / 2 pi; any w >= 0 is valid here).

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"

#include <cmath>
#include <complex>
#include <cstddef>

namespace eq
{

/** Floor applied inside log10 so an exact zero reads -3000 dB, not -infinity. */
inline constexpr double magnitudeFloor = 1.0e-300;

/** tan^2 (w/2). Callers that evaluate many sections at one frequency compute
    it once. */
inline double tanSquaredHalf (double w) noexcept
{
    const double t = std::tan (w / 2.0);
    return t * t;
}

/** Power to dB with the floor. A NaN passes through. */
inline double powerToDb (double mag2) noexcept
{
    return 10.0 * std::log10 (mag2 < magnitudeFloor ? magnitudeFloor : mag2);
}

//==============================================================================
// Digital magnitude
//==============================================================================

/** |H|^2 of the SVF half. t2 = tan^2 (w/2). */
inline double svfMag2 (double t2, const SectionParams& p) noexcept
{
    const double X = t2 / (p.g * p.g);
    const double u = p.mL - p.mH * X;
    const double v = 1.0 - X;
    const double num = u * u + p.mB * p.mB * X;
    const double den = v * v + p.k * p.k * X;
    return num / den;
}

/** |H|^2 of the one-pole half. t2 = tan^2 (w/2). */
inline double onePoleMag2 (double t2, const SectionParams& p) noexcept
{
    const double Y = t2 / (p.G * p.G);
    const double num = p.cL * p.cL + p.cH * p.cH * Y;
    const double den = 1.0 + Y;
    return num / den;
}

/** |H|^2 of one section, using only the halves it has. */
inline double sectionMag2 (double t2, const SectionParams& p) noexcept
{
    double mag2 = 1.0;

    if (p.hasSvf)
        mag2 *= svfMag2 (t2, p);

    if (p.hasOnePole)
        mag2 *= onePoleMag2 (t2, p);

    return mag2;
}

/** Magnitude of a designed band in dB at w (rad/sample, 0 <= w < pi). */
inline double bandMagDb (const BandDesign& design, double w) noexcept
{
    const double t2 = tanSquaredHalf (w);
    double db = 0.0;

    for (int i = 0; i < design.numSections; ++i)
        db += powerToDb (sectionMag2 (t2, design.params[static_cast<std::size_t> (i)]));

    return db;
}

//==============================================================================
// Digital complex response
//==============================================================================

namespace detail
{
    /** (a + j b) / (c + j d). */
    inline std::complex<double> complexRatio (double a, double b, double c, double d) noexcept
    {
        const double m = c * c + d * d;
        return { (a * c + b * d) / m, (b * c - a * d) / m };
    }
}

/** H of the SVF half. tanHalfW = tan (w/2). */
inline std::complex<double> svfResponse (double tanHalfW, const SectionParams& p) noexcept
{
    const double x = tanHalfW / p.g;      // s = j x
    const double X = x * x;
    return detail::complexRatio (p.mL - p.mH * X, p.mB * x, 1.0 - X, p.k * x);
}

/** H of the one-pole half. tanHalfW = tan (w/2). */
inline std::complex<double> onePoleResponse (double tanHalfW, const SectionParams& p) noexcept
{
    const double x = tanHalfW / p.G;      // s = j x
    return detail::complexRatio (p.cL, p.cH * x, 1.0, x);
}

/** H of one section, using only the halves it has. */
inline std::complex<double> sectionResponse (double tanHalfW, const SectionParams& p) noexcept
{
    std::complex<double> h (1.0, 0.0);

    if (p.hasSvf)
        h *= svfResponse (tanHalfW, p);

    if (p.hasOnePole)
        h *= onePoleResponse (tanHalfW, p);

    return h;
}

/** H of a designed band at w (rad/sample, 0 <= w < pi), z = e^(j w). */
inline std::complex<double> bandResponse (const BandDesign& design, double w) noexcept
{
    const double tanHalfW = std::tan (w / 2.0);
    std::complex<double> h (1.0, 0.0);

    for (int i = 0; i < design.numSections; ++i)
        h *= sectionResponse (tanHalfW, design.params[static_cast<std::size_t> (i)]);

    return h;
}

//==============================================================================
// Analog prototypes
//==============================================================================

/** |Ha|^2 of one analog section at the analog frequency w (rad/sample scale).
    The cut, notch and band-pass prototypes read only wp and Qp. */
inline double analogMag2 (const AnalogSection& a, SectionClass cls, double w) noexcept
{
    const double w2 = w * w;

    switch (cls)
    {
        case SectionClass::gain2:
        {
            const double wz2 = a.wz * a.wz;
            const double wp2 = a.wp * a.wp;
            const double zn = wz2 - w2;
            const double zd = a.wz * w / a.Qz;
            const double pn = wp2 - w2;
            const double pd = a.wp * w / a.Qp;
            const double ratio = a.wp / a.wz;
            const double ratio2 = ratio * ratio;
            const double num = zn * zn + zd * zd;
            const double den = pn * pn + pd * pd;
            return a.c * a.c * ratio2 * ratio2 * num / den;
        }

        case SectionClass::gain1:
        {
            const double ratio = a.wp / a.wz;
            const double num = w2 + a.wz * a.wz;
            const double den = w2 + a.wp * a.wp;
            return a.c * a.c * ratio * ratio * num / den;
        }

        case SectionClass::cutLp2:
        {
            const double y = w2 / (a.wp * a.wp);
            const double u = 1.0 - y;
            return 1.0 / (u * u + y / (a.Qp * a.Qp));
        }

        case SectionClass::cutHp2:
        {
            const double y = w2 / (a.wp * a.wp);
            const double u = 1.0 - y;
            return y * y / (u * u + y / (a.Qp * a.Qp));
        }

        case SectionClass::cutLp1:
        {
            const double y = w2 / (a.wp * a.wp);
            return 1.0 / (1.0 + y);
        }

        case SectionClass::cutHp1:
        {
            const double y = w2 / (a.wp * a.wp);
            return y / (1.0 + y);
        }

        case SectionClass::notch:
        {
            const double d = a.wp * a.wp - w2;
            const double n = d * d;
            const double e = a.wp * w / a.Qp;
            return n / (n + e * e);
        }

        case SectionClass::bandPass:
        {
            const double d = a.wp * a.wp - w2;
            const double e = a.wp * w / a.Qp;
            const double n = e * e;
            return n / (d * d + n);
        }
    }

    return 1.0;
}

/** Magnitude in dB of the TRUE analog prototype of a designed band at the
    analog frequency w: the product of its trueProto sections, negated when
    the band is the inverse of its boost. */
inline double analogBandMagDb (const BandDesign& design, double w) noexcept
{
    double db = 0.0;

    for (int i = 0; i < design.numSections; ++i)
    {
        const auto u = static_cast<std::size_t> (i);
        db += powerToDb (analogMag2 (design.trueProto[u], design.cls[u], w));
    }

    return design.inverted ? -db : db;
}

//==============================================================================
// Added in Phase 2.6 (PLAN-2.6 P1; G9 (a) on the damped circle)
//==============================================================================

/** H of one section at any complex z (z = -1 excluded): the same rational
    forms as svfResponse and onePoleResponse, at s = (z - 1) / (z + 1), the
    bilinear map those are the images of.

      SVF       (mL + mB x + mH x^2) / (1 + k x + x^2),   x = s / g
      one-pole  (cL + cH x) / (1 + x),                    x = s / G

    Only the halves the section has. On the unit circle, z = e^(j w), s is
    j tan (w/2) and this is sectionResponse (to rounding). */
inline std::complex<double> sectionResponseAtZ (std::complex<double> z, const SectionParams& p) noexcept
{
    const std::complex<double> one (1.0, 0.0);
    const std::complex<double> s = (z - one) / (z + one);
    std::complex<double> h (1.0, 0.0);

    if (p.hasSvf)
    {
        const std::complex<double> x = s / p.g;
        h *= (p.mL + p.mB * x + p.mH * x * x) / (one + p.k * x + x * x);
    }

    if (p.hasOnePole)
    {
        const std::complex<double> x = s / p.G;
        h *= (p.cL + p.cH * x) / (one + x);
    }

    return h;
}

/** H of a designed band at any complex z (z = -1 excluded): the product of
    its sections in order (sectionResponseAtZ). With |z| > 1 inside the
    region of convergence this is the z-transform of the band's impulse
    response, which is what a damped measurement (weight a^n, z = e^(j w) / a)
    reads. */
inline std::complex<double> bandResponseAtZ (const BandDesign& design, std::complex<double> z) noexcept
{
    std::complex<double> h (1.0, 0.0);

    for (int i = 0; i < design.numSections; ++i)
        h *= sectionResponseAtZ (z, design.params[static_cast<std::size_t> (i)]);

    return h;
}

} // namespace eq
