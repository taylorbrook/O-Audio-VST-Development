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

    O-ParametricEQ - output stage: output gain and auto gain
    (Stage 2, Phase 2.5; PLAN-2.5 P2, P3; RESEARCH-2.5 Q1, Q2)

    The smoother (GainSmoother). Four one-pole stages in cascade, in linear
    amplitude, per sample:

        y1 += a (T - y1);  y2 += a (y1 - y2);  y3 += a (y2 - y3);  y4 += a (y3 - y4)
        gain = y4,   a = 1 - exp (-X / N),   X = smootherSettleX = 13.0622

    X is the value at which a four-pole step response is within 1e-3 of the
    step at N samples. N = round (0.020 fs) for output gain, round (0.100 fs)
    for auto gain. So a step is within 1e-3 of its size at 20 / 100 ms and is
    exact at about 2.5 N (45 - 52 ms, 222 - 232 ms at every rate): when all
    four stages are within smootherSnapTolerance (1e-9) of T they are set to
    T exactly and the smoother is settled. From then on the gain is T, and a
    settled stage at exactly 1.0 does nothing at all.

    Why four poles. G3 at 50 Hz decides the shape (RESEARCH-2.5 Q1): the
    output is g x, so its second difference carries g'' x. A linear ramp has
    an unbounded g'' at its corners and reads 25x the louder held render on
    a +36 -> -36 dB step; two poles read 3.1x, four 1.56x (1 kHz: 1.00x, local
    kappa 2.22x). A second change inside the ramp: 1.58x.

    Why no clamp. Each y_k is a convex combination of its previous value and
    its input, so the output is a weighted average of past targets: it stays
    inside the range of the targets and is never at or below zero for
    positive targets, whatever the automation does. A clamp would only hide
    a defect, so there is none.

    No latch, no restart. A retarget only changes T; the state carries the
    rest.

    The stage (OutputStage). Two smoothers, applied as one multiply per lane
    sample: g = ag.next() * og.next(), in this order, then L *= g, R *= g.
    The output target is read at the block start (setOutputPending) and
    handed to its smoother at the next control tick (applyOutputPending); the
    auto-gain target is set at a refresh tick (setAutoTarget). Both settled
    with a product of exactly 1.0: process returns without touching a sample.

    std-only: no framework type, no allocation, nothing that can fail.

  ==============================================================================
*/

#pragma once

#include <cmath>

namespace eq
{

//==============================================================================
// Constants
//==============================================================================

inline constexpr double outputGainSeconds = 0.020;
inline constexpr double autoGainSeconds = 0.100;

/** The four-pole step response is within 1e-3 of the step at N samples. */
inline constexpr double smootherSettleX = 13.0622;

/** All four stages within this of the target: snapped to it, settled. */
inline constexpr double smootherSnapTolerance = 1.0e-9;

/** Exact equality without the float-equal warning: neither is below or
    above the other. Both arguments are finite here (targets are sanitised). */
constexpr bool sameGain (double a, double b) noexcept
{
    return ! (a < b) && ! (a > b);
}

inline constexpr double outputGainMinDb = -36.0;
inline constexpr double outputGainMaxDb = 36.0;

/** output_gain raw value (dB) to the linear target: a non-finite value reads
    0 dB, the value is clamped to [-36, +36] dB, and 0 dB is exactly 1.0. */
inline double outputGainTarget (double rawDb) noexcept
{
    double db = std::isfinite (rawDb) ? rawDb : 0.0;
    db = db < outputGainMinDb ? outputGainMinDb : (db > outputGainMaxDb ? outputGainMaxDb : db);
    return std::pow (10.0, db / 20.0);
}

/** Smoother length in samples for a time in seconds: round (t fs), at least 1. */
inline int smootherLengthSamples (double seconds, double sampleRateHz) noexcept
{
    const double n = std::round (seconds * sampleRateHz);
    return n < 1.0 ? 1 : static_cast<int> (n);
}

//==============================================================================
/** The four-pole one-pole cascade of the file comment. */
class GainSmoother
{
public:
    /** a = 1 - exp (-X / N), N at least 1. The state is not changed. */
    void prepare (int lengthSamples) noexcept
    {
        const double n = lengthSamples < 1 ? 1.0 : static_cast<double> (lengthSamples);
        a = 1.0 - std::exp (-smootherSettleX / n);
    }

    /** All four stages at T, settled. */
    void snap (double target) noexcept
    {
        T = target;
        y1 = y2 = y3 = y4 = target;
        settled = true;
    }

    /** Stores T. Settled stays true only if all four stages already equal T
        exactly; nothing else changes. */
    void setTarget (double target) noexcept
    {
        T = target;
        settled = sameGain (y1, target) && sameGain (y2, target) && sameGain (y3, target) && sameGain (y4, target);
    }

    /** One sample. Settled: T. Otherwise one step of the cascade, and the
        snap when every stage is within the tolerance. */
    double next() noexcept
    {
        if (settled)
            return T;

        y1 += a * (T - y1);
        y2 += a * (y1 - y2);
        y3 += a * (y2 - y3);
        y4 += a * (y3 - y4);

        if (std::abs (y1 - T) <= smootherSnapTolerance
            && std::abs (y2 - T) <= smootherSnapTolerance
            && std::abs (y3 - T) <= smootherSnapTolerance
            && std::abs (y4 - T) <= smootherSnapTolerance)
        {
            y1 = y2 = y3 = y4 = T;
            settled = true;
        }

        return y4;
    }

    /** Settled on exactly v. */
    bool isSettledAt (double v) const noexcept     { return settled && sameGain (T, v); }
    bool isSettled() const noexcept                { return settled; }
    double getTarget() const noexcept              { return T; }
    double getCurrent() const noexcept             { return y4; }
    double getCoefficient() const noexcept         { return a; }

private:
    double y1 = 1.0, y2 = 1.0, y3 = 1.0, y4 = 1.0;
    double T = 1.0;
    double a = 1.0;
    bool settled = true;
};

//==============================================================================
/** Auto gain x output gain, between the bands and the bypass mix. */
class OutputStage
{
public:
    /** Both smoothers at their lengths for this rate, snapped to 1.0; the
        pending output target 1.0. */
    void prepare (double sampleRateHz) noexcept
    {
        og.prepare (smootherLengthSamples (outputGainSeconds, sampleRateHz));
        ag.prepare (smootherLengthSamples (autoGainSeconds, sampleRateHz));
        og.snap (1.0);
        ag.snap (1.0);
        pendingOg = 1.0;
    }

    /** Seeding: both smoothers settled on their targets; the pending output
        target becomes the output target. */
    void snap (double outputTarget, double autoTarget) noexcept
    {
        pendingOg = outputTarget;
        og.snap (outputTarget);
        ag.snap (autoTarget);
    }

    /** Block start: the sanitised output target, applied at the next tick. */
    void setOutputPending (double target) noexcept      { pendingOg = target; }

    /** On a control tick. */
    void applyOutputPending() noexcept                  { og.setTarget (pendingOg); }

    /** On an auto-gain refresh tick. */
    void setAutoTarget (double target) noexcept         { ag.setTarget (target); }

    /** Nothing to do now and nothing pending: both settled at exactly 1.0
        and the pending output target exactly 1.0. */
    bool isUnity() const noexcept
    {
        return og.isSettledAt (1.0) && sameGain (pendingOg, 1.0) && ag.isSettledAt (1.0);
    }

    /** Applies the gain to n samples of L and R. Either pointer may be null
        (a lane that is not converted). Both settled: one constant gain, and a
        constant of exactly 1.0 touches nothing. Otherwise per sample
        g = ag.next() * og.next(). */
    void process (double* L, double* R, int n) noexcept
    {
        if (n <= 0)
            return;

        if (ag.isSettled() && og.isSettled())
        {
            const double g = ag.getTarget() * og.getTarget();

            if (sameGain (g, 1.0))
                return;

            if (L != nullptr)
                for (int i = 0; i < n; ++i)
                    L[i] *= g;

            if (R != nullptr)
                for (int i = 0; i < n; ++i)
                    R[i] *= g;

            return;
        }

        for (int i = 0; i < n; ++i)
        {
            const double gA = ag.next();
            const double g = gA * og.next();

            if (L != nullptr)
                L[i] *= g;

            if (R != nullptr)
                R[i] *= g;
        }
    }

    const GainSmoother& getOutputSmoother() const noexcept  { return og; }
    const GainSmoother& getAutoSmoother() const noexcept    { return ag; }
    double getPendingOutput() const noexcept                { return pendingOg; }

    //==========================================================================
    // Added in Phase 2.6 (PLAN-2.6 P9, call C3): the two gains apart
    //==========================================================================

    /** The gains process (L, R, n) would apply to the next n samples, as two
        factors, and the smoothers advanced exactly as process advances them:
        the same test at the call's start, then per sample ag.next() before
        og.next(). Both settled: ag[i] = auto target, og[i] = output target
        (process uses their product as its constant g). Otherwise
        ag[i] = ag.next(), og[i] = og.next(). Hence, for any history,
        ag[i] * og[i] is bit for bit the g process would use at that sample,
        and both smoothers end in the state process leaves them in. For the
        listen path: lanes x ag[i], listen, lanes x og[i]. Two multiplies
        round differently from the one multiply by g (and process skips a g
        of exactly 1.0, which two factors need not reproduce): the caller
        chooses one form per segment (PLAN-2.6 R1). */
    void nextGains (double* agOut, double* ogOut, int n) noexcept
    {
        if (n <= 0)
            return;

        if (ag.isSettled() && og.isSettled())
        {
            const double a = ag.getTarget();
            const double o = og.getTarget();

            for (int i = 0; i < n; ++i)
            {
                agOut[i] = a;
                ogOut[i] = o;
            }

            return;
        }

        for (int i = 0; i < n; ++i)
        {
            const double gA = ag.next();
            agOut[i] = gA;
            ogOut[i] = og.next();
        }
    }

private:
    GainSmoother og, ag;
    double pendingOg = 1.0;
};

} // namespace eq
