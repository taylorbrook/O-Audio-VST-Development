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

    O-ParametricEQ - accuracy polish of a second-order gain section
    (Stage 2, Phase 2.1b; ARCHITECTURE "Accuracy polish (v1, Phase 2.1b)")

    The closed-form MLR section (the seed) is refined toward its TRUE analog
    section by a fixed number of anchored Gauss-Newton steps on six
    log-parameters, ln (g, k, mB, mH, G, cH). mL and cL are copied from the
    seed, so the DC gain stays exact.

      grid     16 fixed points per sample rate + 10 points tied to the zero
               pair and the pole pair of the analog section
      step     A = JtJ + (lambda diag (JtJ) + mu) I,  b = Jt r + mu (phi - phi0)
               delta = A^-1 b                   (6x6 Cholesky, closed-form J)
               delta <- delta R / sqrt (R^2 + |delta|^2)    (smooth step limit)
               phi <- phi - delta
      skip     the seed is kept, exactly, where its match-grid error is
               already at or below the keep threshold
      extend   a weight wx fades two further steps in for hard cases
      accept   out = phi0 + a (phi - phi0); a = 0 where the fit does not
               reduce the objective, and for any value that is not finite

    Every decision is a weight, never a switch, so the output is a continuous
    function of the inputs. The only tests are the three weights and the
    guards against values that are not finite. Nothing here reads the band
    type, Q, gain, frequency or sample rate.

    Three things keep the map continuous and the fit near its seed
    (RESEARCH-2.1b-continuity): the step limit above; a seed that is already
    close for resonant shelf sections near and above Nyquist (the shelf
    re-centre of Prototype.h, on the seed side); and no polish at all below
    polish::minSampleRateHz, which Designer::prepare decides once per rate.

    Boost side only: the caller polishes the boost and inverts for a cut.
    Allocation-free, bounded (2 or 4 steps), deterministic. Every constant is
    in PolishConstants.h. Standard C++17 only; noexcept throughout.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"
#include "PolishConstants.h"

#include <cmath>

namespace eq
{

/** The fixed part of the match grid for one sample rate. */
struct PolishGrid
{
    PolishGrid() noexcept                                   { prepare (48000.0); }

    void prepare (double sampleRateHz) noexcept
    {
        const double fLo = polish::bandLowHz;
        const double top = polish::bandTopFraction * sampleRateHz;
        const double fHi = (top < polish::bandHighHz) ? top : polish::bandHighHz;

        wLo = 2.0 * pi * fLo / sampleRateHz;
        wHi = 2.0 * pi * fHi / sampleRateHz;

        for (int i = 0; i < polish::fixedPoints; ++i)
        {
            const double f = fLo * std::pow (fHi / fLo, i / (polish::fixedPoints - 1.0));
            w[i] = 2.0 * pi * f / sampleRateHz;
            const double tn = std::tan (w[i] / 2.0);
            t[i] = tn * tn;
        }
    }

    double w[polish::fixedPoints];      // rad/sample
    double t[polish::fixedPoints];      // tan^2 (w/2)
    double wLo, wHi;                    // band edges, rad/sample
};

/** What the polish did to one section. weight is the accept weight a in
    [0, 1] (0: the seed, untouched). steps is 0, 2 or 4. */
struct PolishInfo
{
    double weight;
    int steps;
};

namespace detail
{
    inline constexpr double polishDbPerNeper = 4.342944819032518;   // 10 / ln 10

    /** The shipped polish policy: the smooth extension weight and the step
        limit. (The designer harness substitutes its own policies through this
        parameter, a binary extension gate and a policy with the limit off, to
        prove that its continuity and never-worse rows can fail.)

        A policy has
          static double weight (double x)       the extension weight
          static constexpr bool limitSteps      whether the step limit applies
          static constexpr double stepLimit     its R (read only if limitSteps) */
    struct ShippedPolicy
    {
        static double weight (double x) noexcept            { return smoothStep (x); }
        static constexpr bool limitSteps = true;
        static constexpr double stepLimit = polish::stepLimit;
    };

    struct PolishWork
    {
        double t[polish::numPoints];        // tan^2 (w/2) at the match points
        double target[polish::numPoints];   // ln |Ha|^2 at the match points
    };

    /** L = ln |H|^2 of the section e = (g, k, mL, mB, mH, G, cL, cH) at
        t = tan^2 (w/2), and, when J is given, dL / d ln (g, k, mB, mH, G, cH). */
    inline double polishEvalRow (const double* e, double t, double* J) noexcept
    {
        const double g = e[0], k = e[1], mL = e[2], mB = e[3], mH = e[4], G = e[5], cL = e[6], cH = e[7];
        const double X = t / (g * g);
        const double Y = t / (G * G);
        const double u = mL - mH * X;
        const double Nn = u * u + mB * mB * X;
        const double oneMinusX = 1.0 - X;
        const double Dn = oneMinusX * oneMinusX + k * k * X;
        const double N1 = cL * cL + cH * cH * Y;
        const double D1 = 1.0 + Y;

        if (J != nullptr)
        {
            const double iN = 1.0 / Nn;
            const double iD = 1.0 / Dn;
            const double i1 = 1.0 / N1;
            J[0] = -2.0 * X * ((-2.0 * mH * u + mB * mB) * iN - (-2.0 * oneMinusX + k * k) * iD);
            J[1] = -2.0 * k * k * X * iD;
            J[2] = 2.0 * mB * mB * X * iN;
            J[3] = -2.0 * mH * X * u * iN;
            J[4] = -2.0 * Y * (cH * cH * i1 - 1.0 / D1);
            J[5] = 2.0 * cH * cH * Y * i1;
        }

        return std::log ((Nn * N1) / (Dn * D1));
    }

    inline constexpr int polishFree[polish::numFree] = { 0, 1, 3, 4, 5, 7 };

    /** Sum of squared residuals (dB^2) of the log-parameters phi. */
    inline double polishObjective (const PolishWork& wk, const double* phi) noexcept
    {
        double e[8];

        for (int i = 0; i < 8; ++i)
            e[i] = std::exp (phi[i]);

        double S = 0.0;

        for (int j = 0; j < polish::numPoints; ++j)
        {
            const double r = polishDbPerNeper * (polishEvalRow (e, wk.t[j], nullptr) - wk.target[j]);
            S += r * r;
        }

        return S;
    }

    /** One anchored Gauss-Newton step, in place, with the policy's step
        limit. Returns false, leaving phi unspecified, when the system is not
        positive definite or the result is not finite. */
    template <typename Policy>
    inline bool polishStep (const PolishWork& wk, const double* phi0, double* phi) noexcept
    {
        constexpr int nf = polish::numFree;
        double e[8];

        for (int i = 0; i < 8; ++i)
            e[i] = std::exp (phi[i]);

        double A[nf][nf] = {};
        double b[nf] = {};

        for (int j = 0; j < polish::numPoints; ++j)
        {
            double J[nf];
            const double r = polishDbPerNeper * (polishEvalRow (e, wk.t[j], J) - wk.target[j]);

            for (int a = 0; a < nf; ++a)
            {
                const double ja = polishDbPerNeper * J[a];
                b[a] += ja * r;

                for (int c = 0; c <= a; ++c)
                    A[a][c] += ja * (polishDbPerNeper * J[c]);
            }
        }

        for (int a = 0; a < nf; ++a)
        {
            A[a][a] += polish::lambda * A[a][a] + polish::mu;
            b[a] += polish::mu * (phi[polishFree[a]] - phi0[polishFree[a]]);
        }

        // Cholesky, lower triangle in place.
        for (int a = 0; a < nf; ++a)
        {
            for (int c = 0; c <= a; ++c)
            {
                double s = A[a][c];

                for (int q = 0; q < c; ++q)
                    s -= A[a][q] * A[c][q];

                if (a == c)
                {
                    if (! (s > 0.0))
                        return false;

                    A[a][c] = std::sqrt (s);
                }
                else
                {
                    A[a][c] = s / A[c][c];
                }
            }
        }

        for (int a = 0; a < nf; ++a)
        {
            double s = b[a];

            for (int q = 0; q < a; ++q)
                s -= A[a][q] * b[q];

            b[a] = s / A[a][a];
        }

        for (int a = nf - 1; a >= 0; --a)
        {
            double s = b[a];

            for (int q = a + 1; q < nf; ++q)
                s -= A[q][a] * b[q];

            b[a] = s / A[a][a];
        }

        if constexpr (Policy::limitSteps)
        {
            double n2 = 0.0;

            for (int a = 0; a < nf; ++a)
                n2 += b[a] * b[a];

            const double sc = Policy::stepLimit / std::sqrt (Policy::stepLimit * Policy::stepLimit + n2);

            for (int a = 0; a < nf; ++a)
                b[a] *= sc;
        }

        bool finite = true;

        for (int a = 0; a < nf; ++a)
        {
            phi[polishFree[a]] -= b[a];
            finite = finite && std::isfinite (phi[polishFree[a]]);
        }

        return finite;
    }

    /** polishSection with the policy as a compile-time parameter. */
    template <typename Policy>
    inline SectionParams polishSectionWith (const PolishGrid& grid, const AnalogSection& trueProto,
                                            const SectionParams& seed, PolishInfo* info) noexcept
    {
        constexpr int np = polish::numPoints;
        int stepsRun = 0;

        if (info != nullptr)
        {
            info->weight = 0.0;
            info->steps = 0;
        }

        const double s[8] = { seed.g, seed.k, seed.mL, seed.mB, seed.mH, seed.G, seed.cL, seed.cH };
        double phi0[8];
        bool finite = true;

        for (int i = 0; i < 8; ++i)
        {
            phi0[i] = std::log (s[i]);
            finite = finite && std::isfinite (phi0[i]);
        }

        // 1. Match points and targets.
        PolishWork wk;
        double w[np];

        for (int i = 0; i < polish::fixedPoints; ++i)
        {
            w[i] = grid.w[i];
            wk.t[i] = grid.t[i];
        }

        {
            const double centre[2] = { trueProto.wz, trueProto.wp };
            const double quality[2] = { trueProto.Qz, trueProto.Qp };
            int n = polish::fixedPoints;

            for (int pair = 0; pair < 2; ++pair)
            {
                w[n++] = centre[pair];

                for (int i = 0; i < polish::numTiedScales; ++i)
                {
                    const double h = polish::tiedScales[i] / (2.0 * quality[pair]);
                    const double rt = std::sqrt (1.0 + h * h);
                    w[n++] = centre[pair] * (rt + h);
                    w[n++] = centre[pair] * (rt - h);
                }
            }
        }

        for (int j = polish::fixedPoints; j < np; ++j)
        {
            double x = w[j];
            x = (x < grid.wLo) ? grid.wLo : x;
            x = (x > grid.wHi) ? grid.wHi : x;
            w[j] = x;
            const double tn = std::tan (x / 2.0);
            wk.t[j] = tn * tn;
        }

        {
            const double wz2 = trueProto.wz * trueProto.wz;
            const double wp2 = trueProto.wp * trueProto.wp;
            const double kz = wz2 / (trueProto.Qz * trueProto.Qz);
            const double kp = wp2 / (trueProto.Qp * trueProto.Qp);
            const double sc = trueProto.c * trueProto.c * (wp2 * wp2) / (wz2 * wz2);

            for (int j = 0; j < np; ++j)
            {
                const double x = w[j] * w[j];
                const double dz = wz2 - x;
                const double dp = wp2 - x;
                wk.target[j] = std::log (sc * (dz * dz + kz * x) / (dp * dp + kp * x));
                finite = finite && std::isfinite (wk.target[j]);
            }
        }

        if (! finite)
            return seed;

        // 2. The seed's objective; the exact skip.
        const double S0 = polishObjective (wk, phi0);

        if (! std::isfinite (S0) || S0 / np <= polish::seedKeepRms * polish::seedKeepRms)
            return seed;

        // 3. The anchored steps.
        double phi[8];

        for (int i = 0; i < 8; ++i)
            phi[i] = phi0[i];

        for (int it = 0; it < polish::steps; ++it)
        {
            ++stepsRun;

            if (! polishStep<Policy> (wk, phi0, phi))
                return seed;
        }

        // 4. The blended extension.
        const double S2 = polishObjective (wk, phi);

        if (! std::isfinite (S2))
            return seed;

        const double wx = Policy::weight ((std::sqrt (S2 / np) - polish::extensionRms) / polish::extensionRms);

        if (wx > 0.0)
        {
            double phi4[8];
            bool ok = true;

            for (int i = 0; i < 8; ++i)
                phi4[i] = phi[i];

            for (int it = 0; it < polish::extraSteps; ++it)
            {
                ++stepsRun;
                ok = ok && polishStep<Policy> (wk, phi0, phi4);
            }

            if (ok)
                for (int i = 0; i < 8; ++i)
                    phi[i] = phi[i] + wx * (phi4[i] - phi[i]);
        }

        // 5. The accept weight.
        const double S1 = polishObjective (wk, phi);
        finite = std::isfinite (S1);

        for (int i = 0; i < 8; ++i)
            finite = finite && std::isfinite (phi[i]);

        if (! finite)
            return seed;

        const double keep2 = polish::seedKeepRms * polish::seedKeepRms;
        const double full2 = polish::seedFullRms * polish::seedFullRms;
        const double a = smoothStep ((S0 / np - keep2) / (full2 - keep2))
                           * smoothStep ((1.0 - S1 / S0) / polish::acceptRamp);

        if (! (a > 0.0))
            return seed;

        SectionParams out = seed;       // mL and cL: the seed's own values
        out.g  = std::exp (phi0[0] + a * (phi[0] - phi0[0]));
        out.k  = std::exp (phi0[1] + a * (phi[1] - phi0[1]));
        out.mB = std::exp (phi0[3] + a * (phi[3] - phi0[3]));
        out.mH = std::exp (phi0[4] + a * (phi[4] - phi0[4]));
        out.G  = std::exp (phi0[5] + a * (phi[5] - phi0[5]));
        out.cH = std::exp (phi0[7] + a * (phi[7] - phi0[7]));

        const double free6[polish::numFree] = { out.g, out.k, out.mB, out.mH, out.G, out.cH };

        for (int i = 0; i < polish::numFree; ++i)
            if (! (std::isfinite (free6[i]) && free6[i] > 0.0))
                return seed;

        if (info != nullptr)
        {
            info->weight = a;
            info->steps = stepsRun;
        }

        return out;
    }
}

/** Polishes one boost-side gain2 section toward its true analog section.
    Where the accept weight is 0 the result is the seed object itself. */
inline SectionParams polishSection (const PolishGrid& grid, const AnalogSection& trueProto,
                                    const SectionParams& seed, PolishInfo* info = nullptr) noexcept
{
    return detail::polishSectionWith<detail::ShippedPolicy> (grid, trueProto, seed, info);
}

} // namespace eq
