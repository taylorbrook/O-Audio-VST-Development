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

#include <cstdint>
#include "forked/ControlGrid.h"
#include "forked/EngineConstants.h"
#include "forked/Grain.h"
#include "forked/GrainFx.h"

// ============================================================================
// GiFxLatch — the per-grain FX latch at spawn (PLAN-2.3 T5, ARCHITECTURE §14).
//
// planFx / assignFxLatch are the parent's statements (O-Granulator @ 82406fce
// Spawner.h:334-392 and :398-414) re-derived with names only changed: the
// parent writes into its GrainPlan, here into an FxPlan. Draw slots
// FxCutoff / FxCrushProb / FxRing, each behind its gate, all BEFORE acquire
// (a refused or stolen spawn consumes the same draws). A stage at its neutral
// value latches nothing: its fields keep the Grain {} defaults and its
// fxActive bit stays clear.
//
// assignFxLatch writes the LATCH only — never FX state. No-bleed comes from
// GrainPool::activate()'s value-initialisation (Grain {}). It writes svfMake
// (RESEARCH-2.3 P1: an LP/HP null with the make-down left at 1 still passes
// after gain alignment; the PF gain band + PFc6 catch it).
// ============================================================================
namespace ogi
{
    struct FxPlan
    {
        std::uint32_t fxActive = 0;
        int   svfMode = 0;
        float svfK = 0.0f, svfA1 = 0.0f, svfA2 = 0.0f, svfA3 = 0.0f;
        float svfMake = 1.0f;
        float fcHz = 0.0f;                       // telemetry (post-clamp)
        float driveD = 1.0f, driveNorm = 1.0f;
        bool  crushed = false;
        float crushHoldInc = 0.0f, crushScale = 0.0f;
        float ringRotRe = 1.0f, ringRotIm = 0.0f, ringMix = 0.0f;
        float ringHz = 0.0f;                     // telemetry (post-clamp)
    };

    template <typename Uni>
    inline void planFx (FxPlan& p, const GiGridParams::Stream& sp, double fs, Uni&& uni) noexcept
    {
        // SVF: active for every type ≠ Off (also at the 20 kHz default, which
        // clamps to 0.45·fs).
        if (sp.filterType != kSvfOff)
        {
            double fc = static_cast<double> (sp.filterCutoffHz);
            if (sp.filterRand > 0.0f)
                fc = fx::randOct (fc, static_cast<double> (sp.filterRand), uni (Slot::FxCutoff));
            const auto s = fx::svfLatch (fc, static_cast<double> (sp.filterRes), fs);
            p.fxActive |= kFxSvf;
            p.svfMode = sp.filterType;
            p.svfK  = s.k;
            p.svfA1 = s.a1;
            p.svfA2 = s.a2;
            p.svfA3 = s.a3;
            p.svfMake = s.make;
            p.fcHz  = s.fcHz;
        }

        // Drive (D6): active iff d = 16·drive² >= 1e-4; drive 0 is the bitwise
        // bypass. No draw.
        const auto dl = fx::driveLatch (sp.drive);
        if (dl.active)
        {
            p.fxActive |= kFxDrive;
            p.driveD    = dl.d;
            p.driveNorm = dl.norm;
        }

        // Crush (D7): drawn only when crush > 0 and 0 < prob < 1, so prob 0 % /
        // 100 % are exact and draw nothing.
        if (sp.crush > 0.0f)
        {
            const double cp = static_cast<double> (sp.crushProb);
            p.crushed = cp >= 1.0 ? true : cp <= 0.0 ? false : uni (Slot::FxCrushProb) < cp;
            if (p.crushed)
            {
                const auto cl = fx::crushLatch (sp.crush, fs);
                p.fxActive    |= kFxCrush;
                p.crushHoldInc = cl.inc;
                p.crushScale   = cl.scale;
            }
        }

        // Ring: carrier drawn only when ring_rand > 0; the carrier starts at the
        // Grain default (1, 0).
        if (sp.ringMix > 0.0f)
        {
            double f = static_cast<double> (sp.ringFreqHz);
            if (sp.ringRand > 0.0f)
                f = fx::randOct (f, static_cast<double> (sp.ringRand), uni (Slot::FxRing));
            const auto rl = fx::ringLatch (f, fs);
            p.fxActive |= kFxRing;
            p.ringRotRe = rl.rotRe;
            p.ringRotIm = rl.rotIm;
            p.ringMix   = sp.ringMix;
            p.ringHz    = rl.hz;
        }
    }

    /** Writes the FX LATCH (never FX state) into a filled grain. */
    inline void assignFxLatch (Grain& g, const FxPlan& p) noexcept
    {
        g.fxActive     = p.fxActive;
        g.svfMode      = p.svfMode;
        g.svfK         = p.svfK;
        g.svfA1        = p.svfA1;
        g.svfA2        = p.svfA2;
        g.svfA3        = p.svfA3;
        g.svfMake      = p.svfMake;
        g.driveD       = p.driveD;
        g.driveNorm    = p.driveNorm;
        g.crushHoldInc = p.crushHoldInc;
        g.crushScale   = p.crushScale;
        g.ringRotRe    = p.ringRotRe;
        g.ringRotIm    = p.ringRotIm;
        g.ringMix      = p.ringMix;
    }
} // namespace ogi
