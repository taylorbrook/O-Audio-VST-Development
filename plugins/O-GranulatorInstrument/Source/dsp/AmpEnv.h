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

// ============================================================================
// Per-voice amp envelope (ARCHITECTURE §4, CONTEXT-2.1 defaults, PLAN-2.1 T8).
// Not juce::ADSR. Per-sample analog-style exponential segments, double state.
//
//   attack  : RC toward an overshoot target 1.2; ends EXACTLY at 1.0 after
//             T_a (from 0): c = (1/6)^(1/N), level = 1.2 + (level − 1.2)·c.
//   decay   : level = S + (level − S)·c, c = 10^(−3/N): the distance to S
//             falls 60 dB in T_d; snaps to S when within 1e-3 of the span.
//   sustain : S (latched at decay entry).
//   release : level ·= c, c = 10^(−3/N): −60 dB in T_r from the release
//             start level; the voice ENDS below −96 dB (level → exactly 0).
//   forced  : steal / polyphony shrink — linear to 0 over 5 ms from the
//             current level (0 ms test hook → 0 at once); the voice ends at 0.
//
// N = max (T, 2 ms floor) · fs samples. Times are latched at stage ENTRY from
// the grid parameters current at that sample, so a knob move never steps a
// running segment. Output a[n] = level · g_v,  g_v = (1 − d) + d·v².
// ============================================================================
namespace ogi
{
    struct AmpParams
    {
        float attackMs  = 20.0f;
        float decayMs   = 300.0f;
        float sustain   = 1.0f;     // 0..1
        float releaseMs = 1500.0f;
    };

    class AmpEnv
    {
    public:
        enum class Stage { idle, attack, decay, sustain, release, forced };

        static constexpr double kAttackTarget = 1.2;
        static constexpr double kLog60 = -6.907755278982137;   // ln (1e-3)

        /** g_v = (1 − d) + d·v², v and d in [0, 1]. */
        static float velocityGain (float velocity01, float velToAmp01) noexcept
        {
            const float v = std::clamp (velocity01, 0.0f, 1.0f);
            const float d = std::clamp (velToAmp01, 0.0f, 1.0f);
            return (1.0f - d) + d * v * v;
        }

        /** Note-on: from 0 (Poly), times latched now. */
        void noteOn (const AmpParams& p, double fs, float velGain) noexcept
        {
            level = 0.0;
            gain = velGain;
            gainN = 0;
            enterAttack (p, fs);
        }

        /** PLAN-2.5b PD1: Mono retrigger — the attack restarts from the CURRENT
            level (the ModEnv::retrigger precedent) and the velocity gain glides
            linearly to newGain over rampMs (a sample-counted ramp latched here:
            gain_i = from + step·i, exactly newGain at i = N; 0 ms → a step). */
        void retrigger (const AmpParams& p, double fs, float newGain, double rampMs) noexcept
        {
            const double n = std::floor (rampMs * 0.001 * fs + 0.5);
            gainFrom = gain;
            gainTo   = newGain;
            if (n >= 1.0)
            {
                gainN    = static_cast<std::int64_t> (n);
                gainI    = 0;
                gainStep = (static_cast<double> (newGain) - static_cast<double> (gain)) / n;
            }
            else
            {
                gainN = 0;
                gain  = newGain;
            }
            enterAttack (p, fs);
        }

        /** Key up: release from the current level (ignored once released / forced / idle). */
        void noteOff (const AmpParams& p, double fs) noexcept
        {
            if (stage == Stage::attack || stage == Stage::decay || stage == Stage::sustain)
                enterRelease (p, fs);
        }

        /** Steal / polyphony shrink: linear ramp to 0 over ms (0 → immediate). */
        void forceRelease (double fs, double ms) noexcept
        {
            const double n = ms * 0.001 * fs;
            stage = Stage::forced;   // an idle env ends on its next sample
            if (! (n >= 1.0) || ! (level > 0.0))
            {
                level = 0.0;
                forcedStep = 1.0;
                return;
            }
            forcedStep = level / n;
        }

        /** Fills out[0, n) with a[i] = level · g_v. Returns the index of the sample
            at which the envelope ENDED (level became exactly 0 and the stage
            idle), or −1. After the end the remaining samples are 0. */
        int process (float* out, int n, const AmpParams& p, double fs) noexcept
        {
            int endedAt = -1;
            for (int i = 0; i < n; ++i)
            {
                switch (stage)
                {
                    case Stage::attack:
                        level = kAttackTarget + (level - kAttackTarget) * coef;
                        if (level >= 1.0)
                        {
                            level = 1.0;
                            enterDecay (p, fs);
                        }
                        break;

                    case Stage::decay:
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
                        level *= coef;
                        if (level < kEnvEndLevel)
                        {
                            level = 0.0;
                            stage = Stage::idle;
                            if (endedAt < 0) endedAt = i;
                        }
                        break;

                    case Stage::forced:
                        level -= forcedStep;
                        if (! (level > 0.0))
                        {
                            level = 0.0;
                            stage = Stage::idle;
                            if (endedAt < 0) endedAt = i;
                        }
                        break;

                    case Stage::idle:
                        level = 0.0;
                        break;
                }
                if (gainN > 0)   // PD1 gain glide (never armed outside a Mono retrigger)
                {
                    if (++gainI >= gainN)
                    {
                        gain  = gainTo;
                        gainN = 0;
                    }
                    else
                        gain = static_cast<float> (static_cast<double> (gainFrom) + gainStep * static_cast<double> (gainI));
                }
                out[i] = static_cast<float> (level) * gain;
            }
            return endedAt;
        }

        Stage  getStage() const noexcept  { return stage; }
        double getLevel() const noexcept  { return level; }
        float  getGain() const noexcept   { return gain; }
        bool   isActive() const noexcept  { return stage != Stage::idle; }

    private:
        static double samplesFor (double ms, double fs) noexcept
        {
            return std::max (ms, kEnvFloorMs) * 0.001 * fs;
        }

        void enterAttack (const AmpParams& p, double fs) noexcept
        {
            stage = Stage::attack;
            // 1.2 (1 − c^N) = 1  →  c^N = 1/6
            coef = std::exp (std::log (1.0 / 6.0) / samplesFor (p.attackMs, fs));
        }

        void enterDecay (const AmpParams& p, double fs) noexcept
        {
            sustainLevel = std::clamp (static_cast<double> (p.sustain), 0.0, 1.0);
            decayEps = 1.0e-3 * (1.0 - sustainLevel);
            if (! (decayEps > 0.0))
            {
                stage = Stage::sustain;   // sustain 100 %: no decay segment
                level = sustainLevel;
                return;
            }
            stage = Stage::decay;
            coef = std::exp (kLog60 / samplesFor (p.decayMs, fs));
        }

        void enterRelease (const AmpParams& p, double fs) noexcept
        {
            stage = Stage::release;
            coef = std::exp (kLog60 / samplesFor (p.releaseMs, fs));
        }

        Stage  stage = Stage::idle;
        double level = 0.0;
        double coef = 0.0;
        double sustainLevel = 1.0;
        double decayEps = 0.0;
        double forcedStep = 0.0;
        float  gain = 1.0f;
        // PD1 gain glide
        float        gainFrom = 1.0f, gainTo = 1.0f;
        double       gainStep = 0.0;
        std::int64_t gainN = 0, gainI = 0;
    };
} // namespace ogi
