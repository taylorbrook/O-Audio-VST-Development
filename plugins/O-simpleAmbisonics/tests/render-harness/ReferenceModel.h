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

    Four switches for the negative controls (default off):
      snapCoefficientsPerGrid — the 16-vector is NOT lerped per sample (c = cur)
      snapOrderGains          — order gains step instead of ramping
      snapBankFade            — (2.3) the binaural bank switches instead of fading
      snapDecoderMix          — (2.3) the decoder mix steps instead of ramping

    Uses: P1 null, DSP-06b negative control, order-fade control + negative
    control, DSP-01 table (refSh16).

    Phase 2.2: rotation by RE-ENCODING AT THE ROTATED DIRECTION — this file
    never contains R_SH (CONTEXT-2.2 D6). For one source,
    R_SH · encode (d) == encode (R₃ · d) is an identity, so the model is the
    oracle by construction: it builds R₃ (double, from the ARCHITECTURE
    matrices written below — NOT by reading Source/dsp/ShRotator.h), rotates
    the smoothed source direction, and evaluates the generic SN3D table there.
    The three rotation angles run through the same Wrap smoother (same k,
    same snap) at the same place in the control step as the engine's. The
    negative-control switches are untouched — snapCoefficientsPerGrid is the
    yaw zipper by construction. At zero rotation buildR3 is exactly I and
    dir (direction (az, el)) round-trips to 9e-13°, so every Phase 2.1
    reference render is unchanged.

    Phase 2.3: DIRECT-FORM DOUBLE CONVOLUTION of the model's own rotated bus
    with the header filters — the definition UPOLS must reproduce; never an
    FFT or a partition (CONTEXT-2.3 D7). bin[ear][n] = Σ_ch<(order+1)²
    Σ_t<hLen h[order][ch][ear][t] · tap[ch][n − 64 − t], the 64 being the
    structural latency the convolver has by construction. The filters are
    the header floats widened at 48 k (the ARTIFACT — never a re-declared
    copy); at fs ≠ 48 k the harness seeds the engine's resampled table via
    setExternalFilters (the resampler has its own golden gate). The bank
    state machine (cur / pending / queued / fading) is written from the
    PLAN-2.3 shared spec, op-for-op with the engine's: arrive → requestSet →
    stepBank at the control step, after the 2.2 angle steps. THE BANK RAMP IS
    FLOAT (RESEARCH-2.3 F1: a float LinearRamp arrives at step 15, a double
    one at 16 — a null spanning the fade needs the float), formed as
    (double) (prevBank + (float) t · (curBank − prevBank)); og[3] / mix stay
    double so every 2.1 / 2.2 reference literal is untouched. The A14
    placeholder is gone: binauralOnlyRef (both decoder-mix endpoints exactly
    1) copies the convolution, else the A3 blend with the delayed stereo leg.

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "dsp/BinauralFilterData.h"

struct RefParams
{
    float azimuthDeg   = 0.0f;
    float elevationDeg = 0.0f;
    float distanceM    = 1.0f;
    int   order        = 1;
    float yawDeg       = 0.0f;   // Phase 2.2
    float pitchDeg     = 0.0f;
    float rollDeg      = 0.0f;
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
    bool snapBankFade            = false;   // 2.3 negative control
    bool snapDecoderMix          = false;   // 2.3 negative control

    // Ring length ≥ 64 + the longest resampled table (1024 at 192 k).
    static constexpr int kRefRing = 64 + 1024;

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

    // ── Phase 2.2: the Cartesian rotation and the direction round trip ───────
    //
    // ARCHITECTURE §SH rotation (x front, y left, z up), composed Rz·Ry′·Rx:
    //   Rz(ψ)  = [[ c, −s, 0], [ s, c, 0], [ 0, 0, 1]]   +ψ: front → left
    //   Ry′(θ) = [[ c, 0, −s], [ 0, 1, 0], [ s, 0, c]]   +θ: front rises
    //   Rx(φ)  = [[ 1, 0, 0], [ 0, c, −s], [ 0, s, c]]   +φ: left rises
    static void buildR3 (double yawDeg, double pitchDeg, double rollDeg, double R[3][3])
    {
        const double psi = yawDeg * kPi / 180.0, theta = pitchDeg * kPi / 180.0, phi = rollDeg * kPi / 180.0;
        const double cz = std::cos (psi),   sz = std::sin (psi);
        const double cy = std::cos (theta), sy = std::sin (theta);
        const double cx = std::cos (phi),   sx = std::sin (phi);

        const double Rz[3][3] = { {  cz, -sz, 0.0 }, {  sz,  cz, 0.0 }, { 0.0, 0.0, 1.0 } };
        const double Ry[3][3] = { {  cy, 0.0, -sy }, { 0.0, 1.0, 0.0 }, {  sy, 0.0,  cy } };
        const double Rx[3][3] = { { 1.0, 0.0, 0.0 }, { 0.0,  cx, -sx }, { 0.0,  sx,  cx } };

        double YX[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                YX[i][j] = Ry[i][0] * Rx[0][j] + Ry[i][1] * Rx[1][j] + Ry[i][2] * Rx[2][j];

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                R[i][j] = Rz[i][0] * YX[0][j] + Rz[i][1] * YX[1][j] + Rz[i][2] * YX[2][j];
    }

    // d (az, el) = (cos e·cos a, cos e·sin a, sin e).
    static void direction (double azDeg, double elDeg, double d[3])
    {
        const double a = azDeg * kPi / 180.0, e = elDeg * kPi / 180.0;
        d[0] = std::cos (e) * std::cos (a);
        d[1] = std::cos (e) * std::sin (a);
        d[2] = std::sin (e);
    }

    // Back to degrees: az = atan2 (y, x) wrapped to [−180, 180), el = asin (clamp z).
    // Azimuth is undefined at the pole (|z| → 1) — callers compare el only there.
    static void dir (const double d[3], double& azDeg, double& elDeg)
    {
        azDeg = wrap180 (std::atan2 (d[1], d[0]) * 180.0 / kPi);
        elDeg = std::asin (std::clamp (d[2], -1.0, 1.0)) * 180.0 / kPi;
    }

    // The oracle for the frame's azRot / elRot: dir (R₃ · d (az, el)).
    static void rotatedDirection (double azDeg, double elDeg, double yawDeg, double pitchDeg, double rollDeg,
                                  double& azR, double& elR)
    {
        double R[3][3], d[3], dp[3];
        buildR3 (yawDeg, pitchDeg, rollDeg, R);
        direction (azDeg, elDeg, d);
        for (int i = 0; i < 3; ++i)
            dp[i] = R[i][0] * d[0] + R[i][1] * d[1] + R[i][2] * d[2];
        dir (dp, azR, elR);
    }

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

    // The engine's LinearRamp text in FLOAT — for the bank fade only (F1 / C5).
    struct FloatRamp
    {
        float cur = 0.0f;
        void  snapTo (float v) { cur = v; }
        float step (float target, float stepSize)
        {
            const float d = target - cur;
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

    // Phase 2.3: at fs ≠ 48 000 the harness copies the engine's resampled table
    // here (one call per (order, ch, ear), all the same len) BEFORE prepare().
    void setExternalFilters (int order, int ch, int ear, const float* tapsIn, int len)
    {
        auto& e = ext[(size_t) (order - 1)];
        const int nch = (order + 1) * (order + 1);
        if ((int) e.size() != nch * 2 * len)
            e.assign ((size_t) nch * 2 * (size_t) len, 0.0);
        extLen = len;
        for (int t = 0; t < len; ++t)
            e[(size_t) ((ch * 2 + ear) * len + t)] = (double) tapsIn[t];
    }

    // setParams() first, then prepare(): snaps everything (A7).
    void prepare (double sampleRate)
    {
        fs        = sampleRate;
        kOnePole  = 1.0 - std::exp (-(double) kGrid / (kTau * fs));
        kRampStep = (double) kGrid / (kTau * fs);
        kRampStepF = (float) ((double) kGrid / (kTau * fs));   // the engine's expression, so the float is identical

        // Filters: the header floats widened (48 k) or the external table.
        if (extLen > 0)
        {
            hLen = extLen;
            for (int o = 0; o < 3; ++o)
                h[o] = ext[(size_t) o];
        }
        else
        {
            if (fs != 48000.0)
            {
                std::printf ("ReferenceModel: fs %.0f != 48000 and no external filters set — call setExternalFilters first\n", fs);
                std::abort();
            }
            hLen = OSimpleAmbisonics::dsp::kBinauralFilterTaps;
            for (int order = 1; order <= 3; ++order)
            {
                const int nch = (order + 1) * (order + 1);
                auto& table = h[order - 1];
                table.assign ((size_t) nch * 2 * (size_t) hLen, 0.0);
                for (int ch = 0; ch < nch; ++ch)
                    for (int ear = 0; ear < 2; ++ear)
                    {
                        const float* src = order == 1 ? OSimpleAmbisonics::dsp::kMagLsFilters1[ch][ear]
                                         : order == 2 ? OSimpleAmbisonics::dsp::kMagLsFilters2[ch][ear]
                                                      : OSimpleAmbisonics::dsp::kMagLsFilters3[ch][ear];
                        for (int t = 0; t < hLen; ++t)
                            table[(size_t) ((ch * 2 + ear) * hLen + t)] = (double) src[t];
                    }
            }
        }
        if (hLen + kGrid > kRefRing)
        {
            std::printf ("ReferenceModel: table length %d exceeds the ring (%d)\n", hLen, kRefRing);
            std::abort();
        }
        hist.assign ((size_t) 16 * kRefRing, 0.0);
        hIdx = 0;

        // Bank state machine snapped to the order (no fade pending).
        convCur = params.order;
        convPending = convQueued = -1;
        convFading = false;
        bankMix.snapTo (0.0f);
        prevBank = curBank = 0.0f;

        az.snapTo (params.azimuthDeg);
        el.snapTo (params.elevationDeg);
        yaw.snapTo (params.yawDeg);
        pitch.snapTo (params.pitchDeg);
        roll.snapTo (params.rollDeg);

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
        binauralOnlyRef = (prevMixD == 1.0 && curMixD == 1.0);

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

    const double* currentC() const { return curC; }       // ROTATED from Phase 2.2
    double        currentAzEff() const { return az.cur; }
    const double* busTap() const { return tap; }

    // 2.3 bank state, for the queue probe.
    int   activeSet() const { return convCur; }
    bool  isFading() const { return convFading; }
    float bankMixCur() const { return curBank; }

private:
    // ── Phase 2.3: bank state machine (PLAN-2.3 shared spec, op-for-op) ──────
    void convArrive()
    {
        if (convFading && curBank == 1.0f)
        {
            convCur = convPending;
            convPending = -1;
            convFading = false;
            curBank = prevBank = 0.0f;
            if (convQueued >= 0)
            {
                const int q = convQueued;
                convQueued = -1;
                convRequestSet (q);
            }
        }
    }

    void convRequestSet (int n)
    {
        if (convFading)
        {
            convQueued = n;
            return;
        }
        if (n != convCur)
        {
            convPending = n;
            convFading = true;
            bankMix.snapTo (0.0f);
            prevBank = curBank = 0.0f;
        }
    }

    void convStepBank()
    {
        prevBank = curBank;
        if (convFading)
            curBank = bankMix.step (1.0f, kRampStepF);
    }

    // Direct form, double: Σ_ch<(order+1)² Σ_t<hLen h[ch][ear][t] · tap[ch][n − 64 − t],
    // read AFTER this sample's tap was written at hIdx (so hIdx ↔ n).
    double direct (int order, int ear) const
    {
        const int nch = (order + 1) * (order + 1);
        const std::vector<double>& table = h[order - 1];
        double s = 0.0;
        for (int ch = 0; ch < nch; ++ch)
        {
            const double* hh = table.data() + (size_t) ((ch * 2 + ear) * hLen);
            const double* ring = hist.data() + (size_t) ch * kRefRing;
            int idx = hIdx - kGrid;                        // n − 64
            if (idx < 0) idx += kRefRing;
            for (int t = 0; t < hLen; ++t)
            {
                s += hh[t] * ring[idx];
                if (--idx < 0) idx += kRefRing;            // n − 64 − t
            }
        }
        return s;
    }

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
        if (snapDecoderMix)
            mix.snapTo (params.decoder == 0 ? 1.0 : 0.0);
        curMixD  = snapDecoderMix ? mix.cur : mix.step (params.decoder == 0 ? 1.0 : 0.0, kRampStep);

        // Phase 2.2: the three rotation angles, after the mixer, before the copy.
        yaw.step (params.yawDeg, kOnePole);
        pitch.step (params.pitchDeg, kOnePole);
        roll.step (params.rollDeg, kOnePole);

        for (int i = 0; i < 16; ++i)
            prevC[i] = curC[i];
        evaluateC();

        // Phase 2.3: arrive → requestSet → stepBank (ramp only — no FFT step here).
        convArrive();
        convRequestSet (params.order);
        convStepBank();
        binauralOnlyRef = (prevMixD == 1.0 && curMixD == 1.0);
    }

    // c = encode (dir (R₃ · d (az, el))) × band gains — the rotated vector, by
    // re-encoding (D6). At zero rotation this is refSh16 (az, el) to 9e-13°.
    void evaluateC()
    {
        double azR, elR;
        rotatedDirection (az.cur, el.cur, yaw.cur, pitch.cur, roll.cur, azR, elR);

        double y[16];
        refSh16 (azR, elR, y);
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

        // Phase 2.3: write this sample's rotated bus into the ring, then convolve.
        for (int i = 0; i < 16; ++i)
            hist[(size_t) i * kRefRing + (size_t) hIdx] = tap[i];

        double binL, binR;
        if (! convFading)
        {
            binL = direct (convCur, 0);
            binR = direct (convCur, 1);
        }
        else if (snapBankFade)
        {
            binL = direct (convPending, 0);                  // the pending set from the request step
            binR = direct (convPending, 1);
        }
        else
        {
            const double mb = (double) (prevBank + (float) t * (curBank - prevBank));   // C5: float, widened
            const double aL = direct (convCur, 0), aR = direct (convCur, 1);
            const double bL = direct (convPending, 0), bR = direct (convPending, 1);
            binL = aL + mb * (bL - aL);
            binR = aR + mb * (bR - aR);
        }

        if (++hIdx == kRefRing)
            hIdx = 0;

        if (binauralOnlyRef)
        {
            outL = binL;
            outR = binR;
        }
        else
        {
            const double mixS = prevMixD + t * (curMixD - prevMixD);
            outL = dL + mixS * (binL - dL);
            outR = dR + mixS * (binR - dR);
        }
    }

    RefParams params;
    double fs = 48000.0, kOnePole = 0.0, kRampStep = 1.0;
    int    gridPos = 0;

    Wrap    az;
    Wrap    yaw, pitch, roll;
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

    // ── Phase 2.3 ────────────────────────────────────────────────────────────
    std::vector<double> h[3];            // [order-1] → [ch][ear][hLen]
    std::vector<double> ext[3];          // external (resampled) tables, if set
    int    hLen = 0, extLen = 0;
    std::vector<double> hist;            // [ch][kRefRing]
    int    hIdx = 0;

    int       convCur = 1, convPending = -1, convQueued = -1;
    bool      convFading = false;
    FloatRamp bankMix;
    float     prevBank = 0.0f, curBank = 0.0f, kRampStepF = 1.0f;
    bool      binauralOnlyRef = true;
};
