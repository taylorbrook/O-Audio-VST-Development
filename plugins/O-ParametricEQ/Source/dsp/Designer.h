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

    O-ParametricEQ - section designer (Stage 2, Phases 2.1 and 2.1b)

    Band settings in, up to eight section parameter sets out
    (ARCHITECTURE "Algorithm Details").

    Designer::design is the closed form with every second-order gain section
    (Bell; the MLR sections of Low Shelf, High Shelf, Tilt) refined by the
    accuracy polish of Polish.h. Designer::designSeed is the closed form
    alone: the seed, and the polish's fallback.

    Since the Phase 2.1b rework (RESEARCH-2.1b-continuity):
      - the closed form designs a shelf / tilt second-order section from its
        re-centred pairs (Prototype.h, D10), so designSeed differs from the
        first Phase 2.1 result for those sections and for no others;
      - every polish step is length-limited (Polish.h);
      - below polish::minSampleRateHz nothing is polished: design equals
        designSeed bit for bit, every weight 0. prepare() decides this once
        per sample rate, so it must run before the first design at every
        rate change.

      MatchedPair          the one primitive: analog pair -> matched digital
                           pair, expm1 forms, pole angle clamped at pi
      mlr                  matched low-pass ratio: bells, shelf / tilt
                           second-order sections, notch (Qz = +infinity)
      matchedLowPass2      3-point matched low-pass (High Cut)
      matchedHighPass2     matched high-pass, double zero at DC (Low Cut)
      cutLowPass1 / cutHighPass1   first-order cut sections
      firstOrderShelf      Vicanek 2019 first-order shelf section
      bandPassExactPeak    exact-peak matched band-pass
      invert               a cut is the algebraic inverse of its boost

    One Designer per thread. design() is allocation-free, bounded, branch-free
    on exact input values other than gainDbEff < 0, and deterministic: the
    audio thread and the message thread get the same bits from the same input.

    The arithmetic follows tests/model/eqmodel.py operation for operation, and
    every product that feeds a sum sits in its own statement, so a compiler
    that contracts a * b + c inside one expression produces the same bits as
    the reference model. Several of these sums cancel heavily (the overdamped
    exponents, the matched low-pass mB); keep the statements split when
    editing.

    Standard C++17 only; noexcept throughout.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"
#include "Polish.h"
#include "Prototype.h"

#include <cmath>
#include <cstddef>

namespace eq
{

//==============================================================================
// MatchedPair
//==============================================================================

/** Digital image of the analog pair s^2 + (w/Q) s + w^2.
      P0 = 1 + a1 + a2,  P1 = 1 - a1 + a2,  Om = 1 - a2  (a1, a2 are never formed)
      g, k   the trapezoidal SVF parameters of the pair
      hN     magnitude of the analog 2-pole low-pass at Nyquist */
struct MatchedPair
{
    double P0, P1, Om, g, k, hN;
};

namespace detail
{
    inline constexpr double p1Floor      = 1.0e-12;
    inline constexpr double lowPassMatch = 0.75;     // x pi, 3-point matched low-pass
    inline constexpr double highPassMatch = 0.9;     // x pi, matched high-pass
    inline constexpr double shelfMatch   = 0.9;      // Vicanek 2019 match frequency, units of Nyquist

    /** X (w, g) = tan^2 (w/2) / g^2. */
    inline double xOf (double w, double g) noexcept
    {
        const double t = std::tan (w / 2.0);
        return t * t / (g * g);
    }

    inline SectionParams neutralParams() noexcept
    {
        SectionParams p;
        p.g = 1.0;  p.k = 1.0;  p.mL = 1.0;  p.mB = 1.0;  p.mH = 1.0;
        p.G = 1.0;  p.cL = 1.0;  p.cH = 1.0;
        p.hasSvf = false;
        p.hasOnePole = false;
        return p;
    }
}

/** w in rad/sample (may exceed pi). Q may be +infinity (q = 0). */
inline MatchedPair matchedPair (double w, double Q) noexcept
{
    MatchedPair m;
    const double q = 1.0 / (2.0 * Q);
    const double qw = q * w;
    const double qq = q * q;
    double P0 = 0.0;
    double P1 = 0.0;

    if (q < 1.0)
    {
        const double r = std::exp (-qw);
        const double root = std::sqrt (1.0 - qq);
        const double th = detail::minOf (w * root, pi);
        const double s = std::sin (th / 2.0);
        const double s2 = s * s;
        const double em = std::expm1 (-qw);
        const double em2 = em * em;
        const double rs = 4.0 * r * s2;
        P0 = em2 + rs;                                  // = 1 + a1 + a2
        const double onePlusR = 1.0 + r;
        const double onePlusR2 = onePlusR * onePlusR;
        P1 = onePlusR2 - rs;                            // = 1 - a1 + a2
    }
    else
    {
        const double root = std::sqrt (qq - 1.0);
        const double d = w * root;
        const double e1 = -std::expm1 (-qw + d);        // 1 - p1
        const double e2 = -std::expm1 (-qw - d);        // 1 - p2
        P0 = e1 * e2;
        P1 = (2.0 - e1) * (2.0 - e2);
    }

    P1 = detail::maxOf (P1, detail::p1Floor);

    const double twoQw = 2.0 * q * w;
    m.P0 = P0;
    m.P1 = P1;
    m.Om = -std::expm1 (-twoQw);                        // = 1 - a2
    m.g = std::sqrt (P0 / P1);
    m.k = 2.0 * m.Om / std::sqrt (P0 * P1);

    const double f = w / pi;
    const double f2 = f * f;
    const double oneMinusF2 = 1.0 - f2;
    const double a = oneMinusF2 * oneMinusF2;
    const double b = f2 / (Q * Q);
    m.hN = f2 / std::sqrt (a + b);
    return m;
}

//==============================================================================
// Section designs
//==============================================================================

/** Matched low-pass ratio: SVF + corrector one-pole. Qz may be +infinity. */
inline SectionParams mlr (double wz, double Qz, double wp, double Qp, double c) noexcept
{
    const MatchedPair N = matchedPair (wz, Qz);
    const MatchedPair D = matchedPair (wp, Qp);

    SectionParams p;
    p.g = D.g;
    p.k = D.k;
    p.mL = N.P0 / D.P0;
    p.mB = 2.0 * N.Om / std::sqrt (D.P0 * D.P1);
    p.mH = N.P1 / D.P1;
    p.G = N.g * N.g / N.hN;
    p.cL = c * D.P0 / N.P0;
    p.cH = c * (D.hN * D.P1) / (N.hN * N.P1);
    p.hasSvf = true;
    p.hasOnePole = true;
    return p;
}

/** 3-point matched low-pass (High Cut second-order section): matched at DC,
    at min (wc, 0.75 pi) and at Nyquist. SVF only. */
inline SectionParams matchedLowPass2 (double wc, double Q) noexcept
{
    const MatchedPair D = matchedPair (wc, Q);
    const double wm = detail::minOf (wc, detail::lowPassMatch * pi);
    const double Xm = detail::xOf (wm, D.g);
    const double ratio = wm / wc;
    const double y = ratio * ratio;
    const double oneMinusY = 1.0 - y;
    const double oneMinusY2 = oneMinusY * oneMinusY;
    const double yOverQ2 = y / (Q * Q);
    const double Ta = 1.0 / (oneMinusY2 + yOverQ2);     // analog |H|^2 at wm
    const double mH = D.hN;
    const double oneMinusX = 1.0 - Xm;
    const double oneMinusX2 = oneMinusX * oneMinusX;
    const double kk = D.k * D.k;
    const double kkX = kk * Xm;
    const double denAtWm = oneMinusX2 + kkX;
    const double target = Ta * denAtWm;
    const double hX = mH * Xm;
    const double v = 1.0 - hX;
    const double v2 = v * v;
    const double diff = target - v2;
    const double mB2 = diff / Xm;

    SectionParams p = detail::neutralParams();
    p.g = D.g;
    p.k = D.k;
    p.mL = 1.0;
    p.mB = std::sqrt (detail::maxOf (mB2, 0.0));
    p.mH = mH;
    p.hasSvf = true;
    return p;
}

/** Matched high-pass (Low Cut second-order section): double zero at DC,
    matched at min (wc, 0.9 pi). SVF only. */
inline SectionParams matchedHighPass2 (double wc, double Q) noexcept
{
    const MatchedPair D = matchedPair (wc, Q);
    const double wm = detail::minOf (wc, detail::highPassMatch * pi);
    const double Xm = detail::xOf (wm, D.g);
    const double ratio = wm / wc;
    const double y = ratio * ratio;
    const double oneMinusY = 1.0 - y;
    const double oneMinusY2 = oneMinusY * oneMinusY;
    const double yOverQ2 = y / (Q * Q);
    const double Ta = y * y / (oneMinusY2 + yOverQ2);
    const double oneMinusX = 1.0 - Xm;
    const double oneMinusX2 = oneMinusX * oneMinusX;
    const double kk = D.k * D.k;
    const double kkX = kk * Xm;
    const double denAtWm = oneMinusX2 + kkX;

    SectionParams p = detail::neutralParams();
    p.g = D.g;
    p.k = D.k;
    p.mL = 0.0;
    p.mB = 0.0;
    p.mH = std::sqrt (Ta * denAtWm) / Xm;
    p.hasSvf = true;
    return p;
}

/** First-order low-pass wc / (s + wc): matched pole e^(-wc), analog DC and
    Nyquist gain. One-pole only. */
inline SectionParams cutLowPass1 (double wc) noexcept
{
    const double pw = pi / wc;
    const double pw2 = pw * pw;

    SectionParams p = detail::neutralParams();
    p.G = std::tanh (wc / 2.0);
    p.cL = 1.0;
    p.cH = 1.0 / std::sqrt (1.0 + pw2);
    p.hasOnePole = true;
    return p;
}

/** First-order high-pass s / (s + wc). One-pole only. */
inline SectionParams cutHighPass1 (double wc) noexcept
{
    const double pw = pi / wc;
    const double pw2 = pw * pw;

    SectionParams p = detail::neutralParams();
    p.G = std::tanh (wc / 2.0);
    p.cL = 0.0;
    p.cH = pw / std::sqrt (1.0 + pw2);
    p.hasOnePole = true;
    return p;
}

/** Vicanek 2019 first-order shelf section (eq. 10-12, match at 0.9 Nyquist)
    of linear gain `gain` at corner wc. A low shelf of gain G is the high
    shelf of gain 1/G scaled by G. One-pole only. */
inline SectionParams firstOrderShelf (double wc, double gain, bool high) noexcept
{
    const double fc = wc / pi;
    const double Gh = high ? gain : 1.0 / gain;
    const double fm = detail::shelfMatch;
    const double phm = 1.0 - std::cos (pi * fm);
    const double scale = 2.0 / (pi * pi);
    const double invFm2 = 1.0 / (fm * fm);
    const double invPhm = 1.0 / phm;

    const double termA = 1.0 / (Gh * fc * fc);
    const double sumA = invFm2 + termA;
    const double prodA = scale * sumA;
    const double al = prodA - invPhm;

    const double termB = Gh / (fc * fc);
    const double sumB = invFm2 + termB;
    const double prodB = scale * sumB;
    const double be = prodB - invPhm;

    const double twoAl = 2.0 * al;
    const double a1 = -al / (1.0 + al + std::sqrt (1.0 + twoAl));
    const double twoBe = 2.0 * be;
    const double b = -be / (1.0 + be + std::sqrt (1.0 + twoBe));
    const double b0 = (1.0 + a1) / (1.0 + b);
    const double b1 = b * b0;

    double cL = (b0 + b1) / (1.0 + a1);
    double cH = (b0 - b1) / (1.0 - a1);

    if (! high)
    {
        cL = cL * gain;
        cH = cH * gain;
    }

    SectionParams p = detail::neutralParams();
    p.G = (1.0 + a1) / (1.0 - a1);
    p.cL = cL;
    p.cH = cH;
    p.hasOnePole = true;
    return p;
}

/** Exact-peak matched band-pass: single zero at DC, unity and maximum at wc.
    SVF only. */
inline SectionParams bandPassExactPeak (double wc, double Q) noexcept
{
    const MatchedPair D = matchedPair (wc, Q);
    const double X0 = detail::xOf (wc, D.g);
    const double x02 = X0 * X0;
    const double inv = 1.0 / x02;
    const double kk = D.k * D.k;
    const double term = 2.0 * (1.0 - X0) / X0;

    SectionParams p = detail::neutralParams();
    p.g = D.g;
    p.k = D.k;
    p.mL = 0.0;
    p.mB = std::sqrt (detail::maxOf (kk + term, 0.0));
    p.mH = std::sqrt (detail::maxOf (1.0 - inv, 0.0));
    p.hasSvf = true;
    return p;
}

/** The exact algebraic inverse of a section (RESEARCH-2.1 Q5). Needs
    mL, mH, cL, cH > 0, which holds for gain2 and gain1 on the boost side.
    Never applied to a cut, a notch or a band pass. */
inline void invert (SectionParams& p) noexcept
{
    if (p.hasSvf)
    {
        const double rt = std::sqrt (p.mL * p.mH);
        const double g = p.g * std::sqrt (p.mL / p.mH);
        const double k = p.mB / rt;
        const double mL = 1.0 / p.mL;
        const double mB = p.k / rt;
        const double mH = 1.0 / p.mH;
        p.g = g;
        p.k = k;
        p.mL = mL;
        p.mB = mB;
        p.mH = mH;
    }

    if (p.hasOnePole)
    {
        const double G = p.G * p.cL / p.cH;
        const double cL = 1.0 / p.cL;
        const double cH = 1.0 / p.cH;
        p.G = G;
        p.cL = cL;
        p.cH = cH;
    }
}

//==============================================================================
// Designer
//==============================================================================

class Designer
{
public:
    /** Stores the sample rate, builds the polish match grid for it and
        decides, once for this rate, whether design() polishes at all. */
    void prepare (double sampleRateHz) noexcept
    {
        sampleRate = sampleRateHz;
        grid.prepare (sampleRateHz);
        polishEnabled = sampleRateHz >= polish::minSampleRateHz;
    }

    /** False below polish::minSampleRateHz: design() is then designSeed(). */
    bool isPolishEnabled() const noexcept                   { return polishEnabled; }

    double getSampleRate() const noexcept                   { return sampleRate; }

    const PolishGrid& getPolishGrid() const noexcept        { return grid; }

    /** The closed-form (seed) design of one second-order gain section:
        analog section in, eight parameters out. The polish starts from this. */
    static SectionParams designSeedSection (const AnalogSection& a) noexcept
    {
        return mlr (a.wz, a.Qz, a.wp, a.Qp, a.c);
    }

    /** Band settings -> section parameter sets, polished (Phase 2.1b).
        Writes every field of `out`. */
    void design (const BandSettings& settings, BandDesign& out) const noexcept
    {
        designWith<detail::ShippedPolicy> (settings, out);
    }

    /** The closed form alone: the polish's seed and fallback. Writes every field of `out`; polishWeight is all 0. */
    void designSeed (const BandSettings& settings, BandDesign& out) const noexcept
    {
        AnalogBand proto;
        seedSections (settings, proto, out);
        finish (settings, proto, out);
    }

    /** design() with the polish policy as a compile-time parameter. The
        plugin uses design(); the designer harness uses this to build the
        variants its rows must reject. */
    template <typename Policy>
    void designWith (const BandSettings& settings, BandDesign& out) const noexcept
    {
        AnalogBand proto;
        seedSections (settings, proto, out);

        // The polish sees boost-side sections and the plain shelf section:
        // the tilt scalar and the inversion come after it.
        const int numToPolish = polishEnabled ? proto.numSections : 0;

        for (int i = 0; i < numToPolish; ++i)
        {
            const auto u = static_cast<std::size_t> (i);

            if (proto.cls[u] == SectionClass::gain2)
            {
                PolishInfo info;
                out.params[u] = detail::polishSectionWith<Policy> (grid, proto.trueSide[u], out.params[u], &info);
                out.polishWeight[u] = static_cast<float> (info.weight);
            }
        }

        finish (settings, proto, out);
    }

private:
    /** The closed form of every section, on the boost side. */
    void seedSections (const BandSettings& settings, AnalogBand& proto, BandDesign& out) const noexcept
    {
        buildPrototype (settings, sampleRate, proto);

        out.numSections = proto.numSections;
        out.inverted = isGainType (settings.type) && settings.gainDbEff < 0.0;

        for (int i = 0; i < maxSections; ++i)
        {
            const auto u = static_cast<std::size_t> (i);
            out.cls[u] = proto.cls[u];
            out.trueProto[u] = proto.trueSide[u];
            out.polishWeight[u] = 0.0f;

            if (i >= proto.numSections)
            {
                out.params[u] = detail::neutralParams();
                continue;
            }

            const AnalogSection& a = proto.designSide[u];

            switch (proto.cls[u])
            {
                case SectionClass::gain2:
                case SectionClass::notch:
                    out.params[u] = mlr (a.wz, a.Qz, a.wp, a.Qp, a.c);
                    break;

                case SectionClass::gain1:
                    out.params[u] = firstOrderShelf (proto.wc, proto.shelfGain[u], proto.shelfHigh[u]);
                    break;

                case SectionClass::cutLp2:
                    out.params[u] = matchedLowPass2 (a.wp, a.Qp);
                    break;

                case SectionClass::cutHp2:
                    out.params[u] = matchedHighPass2 (a.wp, a.Qp);
                    break;

                case SectionClass::cutLp1:
                    out.params[u] = cutLowPass1 (a.wp);
                    break;

                case SectionClass::cutHp1:
                    out.params[u] = cutHighPass1 (a.wp);
                    break;

                case SectionClass::bandPass:
                    out.params[u] = bandPassExactPeak (a.wp, a.Qp);
                    break;
            }
        }
    }

    /** The tilt scalar, then the inversion for a cut. */
    static void finish (const BandSettings& settings, const AnalogBand& proto, BandDesign& out) noexcept
    {
        // D8: the tilt scalar goes into the last section (it always has a one-pole half).
        if (settings.type == Type::tilt && proto.numSections > 0)
        {
            const auto last = static_cast<std::size_t> (proto.numSections - 1);
            out.params[last].cL = out.params[last].cL * proto.tiltScalar;
            out.params[last].cH = out.params[last].cH * proto.tiltScalar;
            out.trueProto[last].c = out.trueProto[last].c * proto.tiltScalar;
        }

        // P3: a cut is the inverse of the boost, section by section.
        if (out.inverted)
            for (int i = 0; i < proto.numSections; ++i)
                invert (out.params[static_cast<std::size_t> (i)]);
    }

    double sampleRate = 48000.0;
    PolishGrid grid;
    bool polishEnabled = true;
};

} // namespace eq
