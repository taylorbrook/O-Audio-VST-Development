/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    ReferenceModel — the harness's DOUBLE-PRECISION transcription of the
    Phase 2.1 chain (CONTEXT-2.1 D8, RESEARCH-2.1 F9, PLAN-2.1 T9).

    Written from PLAN-2.1 §The control step and the encoding deep-dive — NOT
    by reading Source/dsp/. It shares nothing with ShEncoder.h but <cmath>:
    the SN3D table below is the GENERIC triple product
    Y_n^m = N_n^|m| · P_n^|m|(sin e) · T_m(a) with factorials and the explicit
    §2.5 Legendre polynomials (no Condon–Shortley phase), ACN = n(n+1) + m,
    T_m = sin(|m|a) for m < 0 (AmbiX). Independence is the point of the null
    (pattern_test_fixture_mirrors_drift_silently).

    Same 64-sample control grid, same k / ramp step (in double), same snap
    rule, same linear ramps, same b + t·(a − b) forms, same t = (j+1)/64,
    same TPT one-pole, same D7 mix, same ±60° literals (0.5, 0.25, 0.4330127
    as doubles), same 64-sample delay, same orbit, same A14 placeholder.

    Two switches for the negative controls (default off):
      snapCoefficientsPerGrid — the 16-vector is NOT lerped per sample (c = cur)
      snapOrderGains          — order gains step instead of ramping

    Uses: P1 null, DSP-06b negative control, order-fade control + negative
    control, DSP-01 table (refSh16). Phase 2.2 extends it with a rotation.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>

struct RefParams
{
    float azimuthDeg   = 0.0f;
    float elevationDeg = 0.0f;
    float distanceM    = 1.0f;
    int   order        = 1;
    int   decoder      = 0;      // 0 Binaural, 1 Stereo
    bool  orbit        = false;
    float orbitRateHz  = 0.1f;
};

class ReferenceModel
{
public:
    static constexpr int    kGrid = 64;
    static constexpr double kTau  = 0.02;
    static constexpr double kPi   = 3.14159265358979323846;

    bool snapCoefficientsPerGrid = false;
    bool snapOrderGains          = false;

    // ── generic SN3D table (RESEARCH-2.1 R4) ─────────────────────────────────
    static double factorial (int n)
    {
        double f = 1.0;
        for (int i = 2; i <= n; ++i)
            f *= (double) i;
        return f;
    }

    // Associated Legendre P_n^m(x) WITHOUT the Condon–Shortley phase, x = sin e,
    // sqrt(1 − x²) = cos e — the explicit polynomials of the deep-dive §2.5.
    static double legendre (int n, int m, double s, double c)
    {
        switch (n)
        {
            case 0: return 1.0;
            case 1: return m == 0 ? s : c;
            case 2: return m == 0 ? 0.5 * (3.0 * s * s - 1.0)
                         : m == 1 ? 3.0 * s * c
                                  : 3.0 * c * c;
            case 3: return m == 0 ? 0.5 * s * (5.0 * s * s - 3.0)
                         : m == 1 ? 1.5 * c * (5.0 * s * s - 1.0)
                         : m == 2 ? 15.0 * s * c * c
                                  : 15.0 * c * c * c;
            default: return 0.0;
        }
    }

    static double azTerm (int m, double a)
    {
        if (m < 0)  return std::sin ((double) (-m) * a);
        if (m == 0) return 1.0;
        return std::cos ((double) m * a);
    }

    // All 16 SN3D/ACN coefficients for (azDeg, elDeg) in double.
    static void refSh16 (double azDeg, double elDeg, double out[16])
    {
        const double a = azDeg * kPi / 180.0;
        const double e = elDeg * kPi / 180.0;
        const double s = std::sin (e), c = std::cos (e);

        for (int n = 0; n <= 3; ++n)
            for (int m = -n; m <= n; ++m)
            {
                const int    am  = m < 0 ? -m : m;
                const double N   = std::sqrt ((m == 0 ? 1.0 : 2.0) * factorial (n - am) / factorial (n + am));
                out[n * (n + 1) + m] = N * legendre (n, am, s, c) * azTerm (m, a);
            }
    }

    static int band (int acn) { return acn == 0 ? 0 : acn < 4 ? 1 : acn < 9 ? 2 : 3; }

    // ── control-rate primitives (double twins of Smoothers.h) ─────────────────
    static double wrap180 (double x) { return x - 360.0 * std::floor ((x + 180.0) / 360.0); }

    struct OnePole
    {
        double cur = 0.0;
        void   snapTo (double v) { cur = v; }
        double step (double t, double k)
        {
            cur += k * (t - cur);
            if (std::abs (t - cur) <= 1.0e-6 * std::max (1.0, std::abs (t)))
                cur = t;
            return cur;
        }
    };

    struct Wrap
    {
        double cur = 0.0;
        void   snapTo (double v) { cur = wrap180 (v); }
        double step (double t, double k)
        {
            const double d = wrap180 (t - cur);
            cur = wrap180 (cur + k * d);
            if (std::abs (wrap180 (t - cur)) <= 1.0e-6 * std::max (1.0, std::abs (t)))
                cur = wrap180 (t);
            return cur;
        }
    };

    struct Ramp
    {
        double cur = 0.0;
        void   snapTo (double v) { cur = v; }
        double step (double target, double stepSize)
        {
            const double d = target - cur;
            cur = std::abs (d) <= stepSize ? target : cur + std::copysign (stepSize, d);
            return cur;
        }
    };

    // ── distance law (double twins of DistanceModel.h) ────────────────────────
    static double gainFor (double d) { return 1.25 / (d + 0.25); }
    static double cutoffFor (double d, double fs)
    {
        return std::min (20000.0 * std::min (1.0, 1.0 / std::sqrt (d)), 0.45 * fs);
    }
    static double tptCoefficient (double fc, double fs)
    {
        const double t = std::tan (kPi * fc / fs);
        return t / (1.0 + t);
    }
    static double mixFor (double d) { return std::clamp (d - 1.0, 0.0, 1.0); }

    // ── API ───────────────────────────────────────────────────────────────────
    void setParams (const RefParams& p) { params = p; }

    // setParams() first, then prepare(): snaps everything (A7).
    void prepare (double sampleRate)
    {
        fs        = sampleRate;
        kOnePole  = 1.0 - std::exp (-(double) kGrid / (kTau * fs));
        kRampStep = (double) kGrid / (kTau * fs);

        az.snapTo (params.azimuthDeg);
        el.snapTo (params.elevationDeg);

        const double d = params.distanceM;
        g.snapTo (gainFor (d));
        G.snapTo (tptCoefficient (cutoffFor (d, fs), fs));
        m.snapTo (mixFor (d));
        prevG = curG = g.cur;  prevCoef = curCoef = G.cur;  prevMix = curMix = m.cur;
        z = 0.0;

        for (int b = 0; b < 3; ++b)
            og[b].snapTo (b + 1 <= params.order ? 1.0 : 0.0);

        mix.snapTo (params.decoder == 0 ? 1.0 : 0.0);
        prevMixD = curMixD = mix.cur;

        orbitPhase = 0.0;
        orbitWasOn = false;

        evaluateC();
        for (int i = 0; i < 16; ++i) { prevC[i] = curC[i]; tap[i] = 0.0; }

        for (int i = 0; i < kGrid; ++i) { dlL[i] = 0.0; dlR[i] = 0.0; }
        dlIdx   = 0;
        gridPos = 0;
    }

    void process (const float* mono, double* L, double* R, int n)
    {
        int i = 0;
        while (i < n)
        {
            if (gridPos == 0)
                controlStep();

            const int run = std::min (n - i, kGrid - gridPos);

            for (int j = 0; j < run; ++j)
            {
                const double t = (double) (gridPos + j + 1) / (double) kGrid;
                renderSample ((double) mono[i + j], t, L[i + j], R[i + j]);
            }

            i += run;
            gridPos += run;
            if (gridPos == kGrid)
                gridPos = 0;
        }
    }

    const double* currentC() const { return curC; }
    double        currentAzEff() const { return az.cur; }
    const double* busTap() const { return tap; }

private:
    void controlStep()
    {
        // orbit
        const bool on = params.orbit;
        if (on && ! orbitWasOn)
            orbitPhase = 0.0;
        orbitWasOn = on;

        double azTarget = params.azimuthDeg;
        if (on)
        {
            orbitPhase += (double) params.orbitRateHz * (double) kGrid / fs;
            orbitPhase -= std::floor (orbitPhase);
            azTarget = wrap180 ((double) params.azimuthDeg + 360.0 * orbitPhase);
        }

        az.step (azTarget, kOnePole);
        el.step (params.elevationDeg, kOnePole);

        const double d = params.distanceM;
        prevG = curG;  prevCoef = curCoef;  prevMix = curMix;
        curG    = g.step (gainFor (d), kOnePole);
        curCoef = G.step (tptCoefficient (cutoffFor (d, fs), fs), kOnePole);
        curMix  = m.step (mixFor (d), kOnePole);

        for (int b = 0; b < 3; ++b)
        {
            const double target = b + 1 <= params.order ? 1.0 : 0.0;
            if (snapOrderGains)
                og[b].snapTo (target);
            else
                og[b].step (target, kRampStep);
        }

        prevMixD = curMixD;
        curMixD  = mix.step (params.decoder == 0 ? 1.0 : 0.0, kRampStep);

        for (int i = 0; i < 16; ++i)
            prevC[i] = curC[i];
        evaluateC();
    }

    void evaluateC()
    {
        double y[16];
        refSh16 (az.cur, el.cur, y);
        const double gains[4] = { 1.0, og[0].cur, og[1].cur, og[2].cur };
        for (int i = 0; i < 16; ++i)
            curC[i] = y[i] * gains[band (i)];
    }

    void renderSample (double x, double t, double& outL, double& outR)
    {
        const double gS = prevG + t * (curG - prevG);
        const double GS = prevCoef + t * (curCoef - prevCoef);
        const double mS = prevMix + t * (curMix - prevMix);

        const double u  = x * gS;
        const double v  = (u - z) * GS;
        const double lp = v + z;
        z = lp + v;
        const double y = u + mS * (lp - u);

        for (int i = 0; i < 16; ++i)
        {
            const double c = snapCoefficientsPerGrid ? curC[i] : prevC[i] + t * (curC[i] - prevC[i]);
            tap[i] = y * c;
        }

        const double stL = 0.5 * tap[0] + 0.25 * tap[3] + 0.4330127 * tap[1];
        const double stR = 0.5 * tap[0] + 0.25 * tap[3] - 0.4330127 * tap[1];

        const double dL = dlL[dlIdx];
        const double dR = dlR[dlIdx];
        dlL[dlIdx] = stL;
        dlR[dlIdx] = stR;
        if (++dlIdx == kGrid)
            dlIdx = 0;

        const double binL = dL, binR = dR;                   // A14 placeholder
        const double mixS = prevMixD + t * (curMixD - prevMixD);
        outL = dL + mixS * (binL - dL);
        outR = dR + mixS * (binR - dR);
    }

    RefParams params;
    double fs = 48000.0, kOnePole = 0.0, kRampStep = 1.0;
    int    gridPos = 0;

    Wrap    az;
    OnePole el, g, G, m;
    Ramp    og[3], mix;
    double  prevG = 1.0, curG = 1.0, prevCoef = 0.0, curCoef = 0.0, prevMix = 0.0, curMix = 0.0;
    double  prevMixD = 1.0, curMixD = 1.0;
    double  z = 0.0;
    double  orbitPhase = 0.0;
    bool    orbitWasOn = false;

    double prevC[16] {}, curC[16] {}, tap[16] {};
    double dlL[kGrid] {}, dlR[kGrid] {};
    int    dlIdx = 0;
};
