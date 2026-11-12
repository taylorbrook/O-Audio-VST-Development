/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "GiConstants.h"
#include "GiModMatrix.h"
#include "forked/ControlGrid.h"
#include "forked/ModSources.h"   // VERBATIM: the Lfo class (PLAN-2.5a PD2)

// ============================================================================
// Per-voice modulation state — Phase 2.5a (PLAN-2.5a PD4, PD11).
//
// GiModEnv (Mod Env 1–2): the AmpEnv math (RC 1.2-overshoot attack ending at
// exactly 1, −60 dB decay toward S, −60 dB release, 2 ms floors, times
// LATCHED AT STAGE ENTRY) with three differences: it never ends a voice (the
// release reaches exactly 0 below kEnvEndLevel and idles), it has no
// velocity gain, and retrigger() restarts the attack from the CURRENT level
// (used by 2.5b mono / legato; unit-gated in 2.5a, ME4).
//
// GiVoiceMod: one per voice slot, reset at note-on. Dense-stored,
// sparsely-touched fixed arrays (no allocation): the source values of the
// cell, the per-voice LFOs (the VERBATIM parent class), the Mod Envs, the
// per-voice matrix offsets (prev / cur) with per-voice route-change
// residuals, and the 2.5b seam fields (pressure, slide, bendSrc).
// ============================================================================
namespace ogi
{
    struct ModEnvParams
    {
        float attackMs  = 10.0f;
        float decayMs   = 500.0f;
        float sustain   = 0.0f;     // 0..1
        float releaseMs = 500.0f;
    };

    class GiModEnv
    {
    public:
        enum class Stage { idle, attack, decay, sustain, release };

        static constexpr double kAttackTarget = 1.2;
        static constexpr double kLog60 = -6.907755278982137;   // ln (1e-3)

        /** Note-on (Poly): from 0, attack time latched now. */
        void noteOn (const ModEnvParams& p, double fs) noexcept
        {
            level = 0.0;
            enterAttack (p, fs);
        }

        /** 2.5b mono / legato: the attack restarts from the CURRENT level. */
        void retrigger (const ModEnvParams& p, double fs) noexcept
        {
            enterAttack (p, fs);
        }

        void noteOff (const ModEnvParams& p, double fs) noexcept
        {
            if (stage == Stage::attack || stage == Stage::decay || stage == Stage::sustain)
                enterRelease (p, fs);
        }

        /** Advances n samples. liveTimes (test hook envLiveTimes, ME3 control):
            the running segment's coefficient is re-derived from p every sample. */
        void advance (int n, const ModEnvParams& p, double fs, bool liveTimes = false) noexcept
        {
            if (stage == Stage::sustain) { level = sustainLevel; return; }   // flat: no per-sample work
            if (stage == Stage::idle)    { level = 0.0; return; }
            for (int i = 0; i < n; ++i)
            {
                switch (stage)
                {
                    case Stage::attack:
                        if (liveTimes) coef = std::exp (std::log (1.0 / 6.0) / samplesFor (p.attackMs, fs));
                        level = kAttackTarget + (level - kAttackTarget) * coef;
                        if (level >= 1.0)
                        {
                            level = 1.0;
                            enterDecay (p, fs);
                        }
                        break;

                    case Stage::decay:
                        if (liveTimes) coef = std::exp (kLog60 / samplesFor (p.decayMs, fs));
                        level = sustainLevel + (level - sustainLevel) * coef;
                        if (level - sustainLevel <= decayEps)
                        {
                            level = sustainLevel;
                            stage = Stage::sustain;
                        }
                        break;

                    case Stage::sustain:
                        level = sustainLevel;
                        break;

                    case Stage::release:
                        if (liveTimes) coef = std::exp (kLog60 / samplesFor (p.releaseMs, fs));
                        level *= coef;
                        if (level < kEnvEndLevel)
                        {
                            level = 0.0;
                            stage = Stage::idle;
                        }
                        break;

                    case Stage::idle:
                        level = 0.0;
                        break;
                }
            }
        }

        double getLevel() const noexcept  { return level; }
        Stage  getStage() const noexcept  { return stage; }
        double getCoef() const noexcept   { return coef; }
        void   reset() noexcept           { stage = Stage::idle; level = 0.0; coef = 0.0; sustainLevel = 0.0; decayEps = 0.0; }
       #if OGI_TEST_HOOKS
        void   setLevelForTest (double v) noexcept { level = v; }
       #endif

        static double samplesFor (double ms, double fs) noexcept
        {
            return std::max (ms, kEnvFloorMs) * 0.001 * fs;
        }

    private:
        void enterAttack (const ModEnvParams& p, double fs) noexcept
        {
            stage = Stage::attack;
            // 1.2 (1 − c^N) = 1  →  c^N = 1/6 (from 0)
            coef = std::exp (std::log (1.0 / 6.0) / samplesFor (p.attackMs, fs));
        }

        void enterDecay (const ModEnvParams& p, double fs) noexcept
        {
            sustainLevel = std::clamp (static_cast<double> (p.sustain), 0.0, 1.0);
            decayEps = 1.0e-3 * (1.0 - sustainLevel);
            if (! (decayEps > 0.0))
            {
                stage = Stage::sustain;
                level = sustainLevel;
                return;
            }
            stage = Stage::decay;
            coef = std::exp (kLog60 / samplesFor (p.decayMs, fs));
        }

        void enterRelease (const ModEnvParams& p, double fs) noexcept
        {
            stage = Stage::release;
            coef = std::exp (kLog60 / samplesFor (p.releaseMs, fs));
        }

        Stage  stage = Stage::idle;
        double level = 0.0;
        double coef = 0.0;
        double sustainLevel = 0.0;
        double decayEps = 0.0;
    };

    //==========================================================================
    /** Per-voice modulation state (one per voice slot). */
    struct GiVoiceMod
    {
        static constexpr int kDests = kGiModDests;

        float  src[kGiModSources] {};     // source values of this cell (per-voice + global copies)
        Lfo    lfo[kGiLfos];              // the voice's own LFOs (Per-voice mode, or while xf ≠ 0)
        double lfoRateHz[kGiLfos] {};     // rate for the NEXT cell (R50.5)
        float  lfoRateOff[kGiLfos] {};
        bool   lfoOn[kGiLfos] {};         // ticked (prepared) since note-on / the Global → Per-voice switch
        GiModEnv env[kGiModEnvs];

        // Per-voice offsets: off[cur] = this cell, off[cur ^ 1] = the previous cell.
        float  off[2][kDests] {};
        int    cur = 0;
        int    touched[2][kDests] {};
        int    numTouched[2] {};
        bool   mark[kDests] {};
        float  lastContrib[kGiModSlots] {};
        ExactRamp res[kDests];            // per-voice route-change residuals
        bool   resLive[kDests] {};
        int    resList[kDests] {};
        int    numRes = 0;
        int    numRoutedLast = 0;                 // routed per-voice destinations of the last cell
        bool   streamRouted[kGiStreams] {};       // any per-voice offset on the stream (this cell)
        bool   streamRoutedPrev[kGiStreams] {};

        // 2.5b seam (non-MPE in 2.5a: written by channel-wide events)
        float pressure = 0.0f;            // 0..1
        float slide    = 0.0f;            // −1..1 ((cc74 − 64) / 64)
        float bendSrc  = 0.0f;            // −1..1

        const float* offCur() const noexcept  { return off[cur]; }
        const float* offPrev() const noexcept { return off[cur ^ 1]; }
        float offCurAt (int d) const noexcept { return off[cur][d]; }

        /** Offset at piece offset o of the cell (the parent offsetAt lerp). */
        float offsetAt (int d, double o) const noexcept
        {
            const float p = off[cur ^ 1][d], c = off[cur][d];
            return p + (c - p) * static_cast<float> (o / static_cast<double> (kCell));
        }

        void clear() noexcept
        {
            for (int b = 0; b < 2; ++b)
            {
                for (int i = 0; i < numTouched[b]; ++i)
                    off[b][touched[b][i]] = 0.0f;
                numTouched[b] = 0;
            }
            for (int i = 0; i < numRes; ++i)
            {
                res[resList[i]].reset (0.0f);
                resLive[resList[i]] = false;
            }
            numRes = 0;
            std::fill (std::begin (mark), std::end (mark), false);
            std::fill (std::begin (lastContrib), std::end (lastContrib), 0.0f);
            std::fill (std::begin (streamRouted), std::end (streamRouted), false);
            std::fill (std::begin (streamRoutedPrev), std::end (streamRoutedPrev), false);
            std::fill (std::begin (lfoOn), std::end (lfoOn), false);
            cur = 0;
        }

        /** Start a new cell: the old previous buffer becomes current, zeroed over its touched set. */
        void beginCell() noexcept
        {
            cur ^= 1;
            for (int i = 0; i < numTouched[cur]; ++i)
            {
                off[cur][touched[cur][i]] = 0.0f;
                mark[touched[cur][i]] = false;
            }
            numTouched[cur] = 0;
            // marks of the previous cell's set are cleared too (they index off[cur ^ 1])
            for (int i = 0; i < numTouched[cur ^ 1]; ++i)
                mark[touched[cur ^ 1][i]] = false;
            for (int s = 0; s < kGiStreams; ++s)
            {
                streamRoutedPrev[s] = streamRouted[s];
                streamRouted[s] = false;
            }
        }

        void add (int d, float v) noexcept
        {
            if (! mark[d])
            {
                mark[d] = true;
                touched[cur][numTouched[cur]++] = d;
            }
            off[cur][d] += v;
        }

        /** A per-voice route-change residual: reset (current + contrib) → 0. */
        void pushResidual (int d, float contrib, int rampCells) noexcept
        {
            auto& r = res[d];
            r.reset (r.current() + contrib);
            r.setTarget (0.0f, rampCells);
            if (! resLive[d])
            {
                resLive[d] = true;
                resList[numRes++] = d;
            }
        }

        /** Rolls the live residuals into this cell's offsets. */
        void tickResiduals() noexcept
        {
            int keep = 0;
            for (int i = 0; i < numRes; ++i)
            {
                const int d = resList[i];
                auto& r = res[d];
                r.tick();
                add (d, r.current());
                if (r.settledAt (0.0f))
                    resLive[d] = false;
                else
                    resList[keep++] = d;
            }
            numRes = keep;
        }

        /** Canonical +0 and the per-stream routed flags of this cell. Returns
            the number of routed destinations. */
        int finishCell() noexcept
        {
            int n = 0;
            for (int i = 0; i < numTouched[cur]; ++i)
            {
                const int d = touched[cur][i];
                float& o = off[cur][d];
                if (! std::isfinite (o)) o = 0.0f;
                if (o > 0.0f || o < 0.0f)
                {
                    ++n;
                    if (GiDest::isVoiceClass (d))   // R1 PD14: the 28 voice fields only
                        streamRouted[GiDest::streamOf (d)] = true;
                }
                else
                    o = 0.0f;
            }
            return n;
        }

        /** Note-on seed: the previous cell := this cell (no lerp into the first grain). */
        void seedPrevFromCur() noexcept
        {
            const int p = cur ^ 1;
            for (int i = 0; i < numTouched[p]; ++i)
                off[p][touched[p][i]] = 0.0f;
            numTouched[p] = numTouched[cur];
            for (int i = 0; i < numTouched[cur]; ++i)
            {
                const int d = touched[cur][i];
                touched[p][i] = d;
                off[p][d] = off[cur][d];
            }
            for (int s = 0; s < kGiStreams; ++s)
                streamRoutedPrev[s] = streamRouted[s];
        }

        /** The pair (this voice, stream s) carries a per-voice offset now or in the previous cell. */
        bool pairRouted (int s) const noexcept { return streamRouted[s] || streamRoutedPrev[s]; }
    };
} // namespace ogi
