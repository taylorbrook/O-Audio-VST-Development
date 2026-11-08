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

#include <juce_audio_basics/juce_audio_basics.h>   // juce::exactlyEqual (OutputLaw.h)
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "BandlimitedReader.h"
#include "GiConstants.h"
#include "SlotTape.h"
#include "VoiceGrain.h"
#include "GiSpawner.h"
#include "forked/OutputLaw.h"   // hannWindow, streamPower, outputGainFromPower
#include "forked/GrainFx.h"     // per-grain FX stages (2.3)

// ============================================================================
// GiRenderer — grain render + the per-voice output law
// (ARCHITECTURE §12 / §15, PLAN-2.1 T8, P1; PLAN-2.2 T8, PD8).
//
// Per grain per sample (grain-major over a segment of one cell):
//   phase = (age + frac) · invLen      w = window (phase)       (window LAST)
//   (l, r) = tape.readStereo (level, readAbs, set)   positional, double pos
//   bus[voice][stream] += (l, r) · w · killGain · fade · (gainL, gainR)
//   readAbs += step (step = dir · rate; wrapped inside the loop region when
//   wrapW > 0: forward past the top, reverse below the bottom — P20)
// Window (PD8, the parent P14 rule): windowStep 0 && tiltT == 0.5 → the
// closed-form hannWindow (phase), the 2.1 statements verbatim (bitwise);
// every other latched window reads the forked WindowMorphBank (table + lerp +
// tilt warp). forceBank (test hook forceBankPath) disables the fast path.
// During an SR-rebuild crossfade the grain also reads its OLD tape at its old
// coordinates: (1 − x)·old + x·new, x = (t − start) / len over 20 ms.
// Every quantity advances one sample at a time; nothing is keyed to the
// segment, so the render is block-size invariant.
//
// 2.3 (PLAN-2.3 T6 / PD8): ONE kernel for every grain — gather → FX → scatter.
//   gather : the 2.2 per-sample loop, unchanged, into L / R / GK
//            (GK[n] = w · killGain · fade, the 2.2 gk statement verbatim);
//   FX     : per ACTIVE stage (fxActive hoisted per segment), ARCHITECTURE §13
//            order SVF → drive → crush → ring, every operation per sample; the
//            SVF NaN kill (P12) retires the grain BEFORE the scatter;
//   scatter: bus += L · GK · gain — the 2.2 single statements, same operand
//            order. With fxActive == 0 every float expression and its order is
//            that of 2.2, so FX-off renders are bitwise 2.2 (FR1 / RB1). Do not
//            split the GK or scatter statements: FP contraction is per statement.
//
// Output law per voice (P1): N_eff = len̄ / T_k (thinning keeps full
// windows), g_out = outputGainFromPower (streamPower (N_eff, N_eff, Hann, c 0,
// spread 0)) = 1 / sqrt (max (1, N_eff)). Never (N, q) with N > q (the
// parent's truncation model is wrong under thinning).
// ============================================================================
namespace ogi
{
    /** One slot's SR-rebuild crossfade (audio thread). */
    struct XfadeState
    {
        bool            active  = false;
        const SlotTape* oldTape = nullptr;
        const SlotTape* newTape = nullptr;
        double          ratio   = 1.0;     // newRate / oldRate (tape frames per tape frame)
        std::int64_t    start   = 0;       // absolute sample of the take
        int             len     = 1;       // 20 ms in samples

        float weightAt (std::int64_t t) const noexcept
        {
            const double x = static_cast<double> (t - start) / static_cast<double> (std::max (1, len));
            return static_cast<float> (std::clamp (x, 0.0, 1.0));
        }
    };

    /** Per-(voice, stream) bus: [voice][stream][channel][cell offset] (20 KB). */
    struct VoiceBus
    {
        float s[kVoiceSlots][kGiStreams][2][kCell] {};
    };

    /** Test-hook plumbing for the FX block (PLAN-2.3 T6). Empty in the shipping
        build, so the shipping kernel carries no extra branches. */
    struct GiFxRenderFlags
    {
       #if OGI_TEST_HOOKS
        bool forceKill   = false;   // FZ2 forceRetire: the P12 verdict forced false
        bool nanKillOff  = false;   // FZ control: no finite check
        bool fxOrderSwap = false;   // PFc5: crush before drive
        bool nanKilled   = false;   // out: the grain was NaN-killed this segment
       #endif
    };

    struct GiRenderer
    {
        /** The Hann window selection of the law (step 0, identity tilt). */
        static WindowSel hannSel() noexcept
        {
            WindowSel sel;
            sel.step   = 0;
            sel.tilt.t = 0.5f;
            sel.tilt.a = 1.0f;
            sel.tilt.b = 1.0f;
            return sel;
        }

        /** g_out for one voice from its effective count (P1 full-window branch). */
        static float voiceLawGain (double nEff, const WindowMorphBank& bank) noexcept
        {
            const double p = streamPower (nEff, nEff, hannSel(), 0.0, 0.0, bank);
            return outputGainFromPower (p);
        }

        /** Rebases a grain onto the new tape of a starting crossfade (lazy, at its
            first render after the take): the old coordinates move to x*. */
        static void rebaseForXfade (VoiceGrain& vg, const XfadeState& xf) noexcept
        {
            Grain& g = vg.g;
            vg.xTape    = xf.oldTape;
            vg.xReadAbs = g.readAbs;
            vg.xStep    = g.step;
            vg.xWrapLo  = vg.wrapLo;
            vg.xWrapW   = vg.wrapW;
            vg.xLevel   = g.level;
            vg.xSet     = g.set;

            g.readAbs *= xf.ratio;
            g.step    *= xf.ratio;
            g.rate    *= xf.ratio;
            vg.wrapLo *= xf.ratio;
            vg.wrapW  *= xf.ratio;
            g.level = ouaricon::grain::latchLevel (g.rate, kTapeLevels);
            g.set   = ouaricon::grain::SincBank::setFor (std::ldexp (g.rate, -g.level));
            vg.tape = xf.newTape;
        }

        /** Renders one grain over cell offsets [c0, c1); t0 = absolute sample of
            offset c0. Returns false when the grain is finished (retire). */
        static bool renderGrain (VoiceGrain& vg, GiPool::Fader& fader, VoiceBus& bus,
                                 int c0, int c1, std::int64_t t0, const XfadeState (&xf)[kTapeSlots],
                                 const WindowMorphBank& bank, GiFxRenderFlags& ff, bool forceBank = false) noexcept
        {
            juce::ignoreUnused (ff);
            const int s = std::clamp (vg.slot, 0, kTapeSlots - 1);
            const XfadeState& x = xf[s];

            if (vg.xTape == nullptr)
            {
                if (x.active && vg.tape == x.oldTape && x.oldTape != nullptr)
                    rebaseForXfade (vg, x);
            }
            else if (! x.active || vg.xTape != x.oldTape)
            {
                vg.xTape = nullptr;   // crossfade over: new tape only
            }

            const SlotTape* tp = vg.tape;
            if (tp == nullptr)
                return false;

            Grain& g = vg.g;
            const int v  = std::clamp (vg.voice, 0, kVoiceSlots - 1);
            const int st = std::clamp (g.stream, 0, kGiStreams - 1);
            float* bl = bus.s[v][st][0];
            float* br = bus.s[v][st][1];

            // PD8 / P14: latched per grain, so the branch is constant for its life.
            const bool hann = ! forceBank && g.windowStep == 0 && juce::exactlyEqual (g.tiltT, 0.5f);
            const float* const tab = bank.table (g.windowStep);
            const WindowMorphBank::Tilt tilt { g.tiltT, g.tiltA, g.tiltB };

            float L[kCell], R[kCell], GK[kCell];
            int n = 0;

            bool done = false;
            for (int c = c0; c < c1; ++c)
            {
                const double phase = (static_cast<double> (g.age) + g.frac) * g.invLen;
                if (phase >= 1.0)
                {
                    done = true;
                    break;
                }
                const float w = hann ? hannWindow (phase) : WindowMorphBank::read (tab, tilt, phase);

                float l = 0.0f, r = 0.0f;
                tp->tape.readStereo (g.level, g.readAbs, g.set, l, r);

                if (vg.xTape != nullptr)
                {
                    float xl = 0.0f, xr = 0.0f;
                    vg.xTape->tape.readStereo (vg.xLevel, vg.xReadAbs, vg.xSet, xl, xr);
                    const float wt = x.weightAt (t0 + (c - c0));
                    l = xl + wt * (l - xl);
                    r = xr + wt * (r - xr);
                    vg.xReadAbs += vg.xStep;
                    if (vg.xWrapW > 0.0 && vg.xReadAbs >= vg.xWrapLo + vg.xWrapW)
                        vg.xReadAbs -= vg.xWrapW;
                    else if (vg.xWrapW > 0.0 && vg.xReadAbs < vg.xWrapLo)
                        vg.xReadAbs += vg.xWrapW;   // reverse (P20)
                }

                GK[n] = w * g.killGain * fader.next();
                L[n] = l;
                R[n] = r;
                ++n;

                if (g.killStep > 0.0f)
                {
                    g.killGain -= g.killStep;
                    if (! (g.killGain > 0.0f))
                    {
                        g.killGain = 0.0f;
                        done = true;
                    }
                }

                g.readAbs += g.step;
                if (vg.wrapW > 0.0 && g.readAbs >= vg.wrapLo + vg.wrapW)
                    g.readAbs -= vg.wrapW;
                else if (vg.wrapW > 0.0 && g.readAbs < vg.wrapLo)
                    g.readAbs += vg.wrapW;   // reverse (P20)

                ++g.age;
                if ((static_cast<double> (g.age) + g.frac) * g.invLen >= 1.0)
                    done = true;
                if (done)
                    break;
            }

            // FX, in ARCHITECTURE §13 order. The flags are latched per grain.
            const std::uint32_t fxa = g.fxActive;
            if (fxa != 0)
            {
                if ((fxa & kFxSvf) != 0)
                {
                    fx::svf (g, L, R, n);
                    // P12 NaN kill: a non-finite integrator retires the grain and
                    // its segment contributes nothing (FTZ does not catch NaN).
                    bool finite = fx::svfStateFinite (g);
                   #if OGI_TEST_HOOKS
                    finite = (finite || ff.nanKillOff) && ! ff.forceKill;
                   #endif
                    if (! finite)
                    {
                       #if OGI_TEST_HOOKS
                        ff.nanKilled = true;
                       #endif
                        return false;
                    }
                }
               #if OGI_TEST_HOOKS
                if (ff.fxOrderSwap)
                {
                    if ((fxa & kFxCrush) != 0)
                        fx::crush (g, L, R, n);
                    if ((fxa & kFxDrive) != 0)
                        fx::drive (g, L, R, n);
                }
                else
               #endif
                {
                    if ((fxa & kFxDrive) != 0)
                        fx::drive (g, L, R, n);
                    if ((fxa & kFxCrush) != 0)
                        fx::crush (g, L, R, n);
                }
                if ((fxa & kFxRing) != 0)
                    fx::ring (g, L, R, n);
            }

            for (int j = 0; j < n; ++j)
            {
                bl[c0 + j] += L[j] * GK[j] * g.gainL;
                br[c0 + j] += R[j] * GK[j] * g.gainR;
            }
            return ! done;
        }
    };
} // namespace ogi
