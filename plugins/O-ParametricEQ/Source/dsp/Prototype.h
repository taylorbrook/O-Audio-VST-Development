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

    O-ParametricEQ - analog prototypes (Stage 2, Phase 2.1)

    Band settings -> the analog sections the designer realises
    (ARCHITECTURE "Per-type prototypes" and "Frequency policy near and above
    Nyquist").

    Conventions
      P3   Every gain-bearing band is built on its boost side, from |gainDbEff|.
           The caller inverts each section for a cut.
      P4   Two analog sections per section: the DESIGN side (after the bell
           re-centre and the notch / band-pass clamps) and the TRUE side
           (before them). The true side is the scoring and fitting target.
      D6   Slope is read only by Low Shelf, High Shelf, Low Cut, High Cut, Tilt.
      D8   Tilt = high shelf of the gain times 10^(-gain/40). The scalar is
           returned in tiltScalar; the analog sections here do not carry it.
      D9   Q0 <- Q0 * Qu on cuts, shelves and tilt; Q is not read at 6 dB/oct.
      D10  Bell re-centred above 0.95 pi; notch clamped at 0.95 pi; band pass
           at 0.9 pi; everything else at the true frequency.

    The arithmetic follows tests/model/eqmodel.py operation for operation, and
    every product that feeds a sum sits in its own statement, so a compiler
    that contracts a * b + c inside one expression produces the same bits as
    the reference model. Keep it that way when editing.

    Standard C++17 only; allocation-free; noexcept.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"

#include <cmath>
#include <cstddef>
#include <limits>

namespace eq
{

inline constexpr double bellRecentre  = 0.95;     // x pi
inline constexpr double notchClamp    = 0.95;     // x pi
inline constexpr double bandPassClamp = 0.9;      // x pi
inline constexpr double recentreQMin  = 0.025;
inline constexpr double recentreQMax  = 160.0;

constexpr bool isGainType (Type t) noexcept
{
    return t == Type::bell || t == Type::lowShelf || t == Type::highShelf || t == Type::tilt;
}

constexpr bool usesSlope (Type t) noexcept
{
    return t == Type::lowShelf || t == Type::highShelf || t == Type::lowCut
        || t == Type::highCut || t == Type::tilt;
}

constexpr int clampedSlopeIndex (int slopeIndex) noexcept
{
    return slopeIndex < 0 ? 0 : (slopeIndex >= numSlopes ? numSlopes - 1 : slopeIndex);
}

/** The analog side of one band. Entries at and beyond numSections hold
    defined neutral values. */
struct AnalogBand
{
    int numSections;
    std::array<SectionClass,  maxSections> cls;
    std::array<AnalogSection, maxSections> designSide;
    std::array<AnalogSection, maxSections> trueSide;
    std::array<double,        maxSections> shelfGain;   // gain1: the section's own linear gain (>= 1)
    std::array<bool,          maxSections> shelfHigh;   // gain1: high-shelf side
    double wc;                                          // true corner / centre, rad/sample
    double tiltScalar;                                  // 10^(-|gain|/40) for Tilt, else 1
};

namespace detail
{
    /** The reference model's min (a, b): b when b < a, else a. */
    inline double minOf (double a, double b) noexcept   { return (b < a) ? b : a; }

    /** The reference model's max (a, b): b when b > a, else a. */
    inline double maxOf (double a, double b) noexcept   { return (b > a) ? b : a; }

    /** Butterworth section Q number i of order N (Q0 is the highest). */
    inline double butterworthQ (int i, int order) noexcept
    {
        const double angle = (static_cast<double> (2 * i + 1) * pi) / (2.0 * static_cast<double> (order));
        return 1.0 / (2.0 * std::sin (angle));
    }

    /** |Ha|^2 of a second-order gain section at the analog frequency wa. */
    inline double analogSecondOrderMag2 (double wa, const AnalogSection& a) noexcept
    {
        const double wz2 = a.wz * a.wz;
        const double wp2 = a.wp * a.wp;
        const double wa2 = wa * wa;
        const double zn = wz2 - wa2;
        const double zd = a.wz * wa / a.Qz;
        const double znSq = std::pow (zn, 2.0);
        const double zdSq = std::pow (zd, 2.0);
        const double num = znSq + zdSq;
        const double pn = wp2 - wa2;
        const double pd = a.wp * wa / a.Qp;
        const double pnSq = std::pow (pn, 2.0);
        const double pdSq = std::pow (pd, 2.0);
        const double den = pnSq + pdSq;
        const double cc = a.c * a.c;
        const double ratio4 = std::pow (a.wp / a.wz, 4.0);
        return cc * ratio4 * num / den;
    }

    inline AnalogSection neutralAnalog() noexcept
    {
        return AnalogSection { 1.0, 1.0, 1.0, 1.0, 1.0 };
    }

    inline void addSection (AnalogBand& out, SectionClass cls, const AnalogSection& designSide,
                            const AnalogSection& trueSide, double shelfGain, bool shelfHigh) noexcept
    {
        if (out.numSections >= maxSections)
            return;

        const auto u = static_cast<std::size_t> (out.numSections);
        out.cls[u] = cls;
        out.designSide[u] = designSide;
        out.trueSide[u] = trueSide;
        out.shelfGain[u] = shelfGain;
        out.shelfHigh[u] = shelfHigh;
        ++out.numSections;
    }

    /** Boost-side shelf of the given order (gain >= 1). */
    inline void shelfSections (AnalogBand& out, double wc, double Qu, double gainDbAbs, int order, bool high) noexcept
    {
        const double orderD = static_cast<double> (order);
        const double Gl = std::pow (10.0, gainDbAbs / 20.0);
        const double r = std::pow (Gl, 1.0 / (2.0 * orderD));

        for (int i = 0; i < order / 2; ++i)
        {
            const double Qi = butterworthQ (i, order);
            const double Q = (i == 0) ? Qi * Qu : Qi;      // D9: resonance on Q0 only

            const AnalogSection a = high ? AnalogSection { wc / r, Q, wc * r, Q, 1.0 }
                                         : AnalogSection { wc * r, Q, wc / r, Q, std::pow (Gl, 2.0 / orderD) };
            addSection (out, SectionClass::gain2, a, a, 1.0, high);
        }

        if (order % 2 != 0)
        {
            const double Gs = std::pow (Gl, 1.0 / orderD);  // the section's own gain

            const AnalogSection a = high ? AnalogSection { wc / r, 1.0, wc * r, 1.0, 1.0 }
                                         : AnalogSection { wc * r, 1.0, wc / r, 1.0, Gs };
            addSection (out, SectionClass::gain1, a, a, Gs, high);
        }
    }

    /** Butterworth cut of the given order; lowPass = High Cut. */
    inline void cutSections (AnalogBand& out, double wc, double Qu, int order, bool lowPass) noexcept
    {
        for (int i = 0; i < order / 2; ++i)
        {
            const double Qi = butterworthQ (i, order);
            const double Q = (i == 0) ? Qi * Qu : Qi;

            const AnalogSection a { 0.0, 1.0, wc, Q, 1.0 };
            addSection (out, lowPass ? SectionClass::cutLp2 : SectionClass::cutHp2, a, a, 1.0, false);
        }

        if (order % 2 != 0)
        {
            const AnalogSection a { 0.0, 1.0, wc, 1.0, 1.0 };
            addSection (out, lowPass ? SectionClass::cutLp1 : SectionClass::cutHp1, a, a, 1.0, false);
        }
    }

    /** Bell: the true prototype, and the design-side one (re-centred at
        0.95 pi when the centre is above it).

          Ge = |Ha (wL)|,  T = |Ha (wL / 2)|^2,
          Qe = sqrt (0.25 (Ge - T / Ge) / (0.5625 (T - 1)))

        Numerator or denominator <= 0 -> Qu; then clamp to [0.025, 160]. */
    inline void bellSections (AnalogBand& out, double wc, double Qu, double gainDbAbs) noexcept
    {
        const double A = std::pow (10.0, gainDbAbs / 40.0);
        const AnalogSection trueSide { wc, Qu / A, wc, Qu * A, 1.0 };
        const double wL = bellRecentre * pi;

        if (! (wc > wL))
        {
            addSection (out, SectionClass::gain2, trueSide, trueSide, 1.0, false);
            return;
        }

        const double Ge = std::sqrt (analogSecondOrderMag2 (wL, trueSide));
        const double T = analogSecondOrderMag2 (wL / 2.0, trueSide);
        const double tOverGe = T / Ge;
        const double num = 0.25 * (Ge - tOverGe);
        const double den = 0.5625 * (T - 1.0);
        double Qe = (num > 0.0 && den > 0.0) ? std::sqrt (num / den) : Qu;
        Qe = minOf (maxOf (Qe, recentreQMin), recentreQMax);
        const double Ae = std::sqrt (Ge);                   // = 10^(Ge dB / 40)

        const AnalogSection designSide { wL, Qe / Ae, wL, Qe * Ae, 1.0 };
        addSection (out, SectionClass::gain2, designSide, trueSide, 1.0, false);
    }
}

/** Number of sections a band of this type and slope index has. */
constexpr int sectionCount (Type t, int slopeIndex) noexcept
{
    return usesSlope (t) ? (slopeOrder[clampedSlopeIndex (slopeIndex)] / 2 + slopeOrder[clampedSlopeIndex (slopeIndex)] % 2)
                         : 1;
}

/** Band settings -> analog sections, on the boost side. fs in Hz. */
inline void buildPrototype (const BandSettings& s, double fs, AnalogBand& out) noexcept
{
    const double wc = 2.0 * pi * s.freqHz / fs;
    const double gainDbAbs = std::fabs (s.gainDbEff);

    out.numSections = 0;
    out.wc = wc;
    out.tiltScalar = 1.0;

    for (int i = 0; i < maxSections; ++i)
    {
        const auto u = static_cast<std::size_t> (i);
        out.cls[u] = SectionClass::gain2;
        out.designSide[u] = detail::neutralAnalog();
        out.trueSide[u] = detail::neutralAnalog();
        out.shelfGain[u] = 1.0;
        out.shelfHigh[u] = false;
    }

    const int order = slopeOrder[clampedSlopeIndex (s.slopeIndex)];

    switch (s.type)
    {
        case Type::bell:
            detail::bellSections (out, wc, s.q, gainDbAbs);
            break;

        case Type::lowShelf:
            detail::shelfSections (out, wc, s.q, gainDbAbs, order, false);
            break;

        case Type::highShelf:
            detail::shelfSections (out, wc, s.q, gainDbAbs, order, true);
            break;

        case Type::lowCut:
            detail::cutSections (out, wc, s.q, order, false);
            break;

        case Type::highCut:
            detail::cutSections (out, wc, s.q, order, true);
            break;

        case Type::notch:
        {
            const double inf = std::numeric_limits<double>::infinity();
            const double wd = detail::minOf (wc, notchClamp * pi);
            detail::addSection (out, SectionClass::notch,
                                AnalogSection { wd, inf, wd, s.q, 1.0 },
                                AnalogSection { wc, inf, wc, s.q, 1.0 }, 1.0, false);
            break;
        }

        case Type::bandPass:
        {
            const double wd = detail::minOf (wc, bandPassClamp * pi);
            detail::addSection (out, SectionClass::bandPass,
                                AnalogSection { 0.0, 1.0, wd, s.q, 1.0 },
                                AnalogSection { 0.0, 1.0, wc, s.q, 1.0 }, 1.0, false);
            break;
        }

        case Type::tilt:
            detail::shelfSections (out, wc, s.q, gainDbAbs, order, true);
            out.tiltScalar = std::pow (10.0, -gainDbAbs / 40.0);
            break;
    }
}

} // namespace eq
