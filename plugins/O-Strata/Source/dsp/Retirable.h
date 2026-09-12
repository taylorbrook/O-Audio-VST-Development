/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    Retirable.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

    Base of every object the audio thread may still be reading when the message
    thread unpublishes it (ARCHITECTURE Decision 6: type-erased reaper). Such an
    object is never freed in place: OStrataAudioProcessor::retire() parks it
    stamped with the current block generation, and the processor's timer frees it
    only after the generation has advanced >= 2 — guaranteeing a full processBlock
    has started and finished since the pointer was unpublished, so no voice still
    references it. Same class of fix as O-MicrotonalSampler v1.23.2.

    Round B users: ChebyshevSet (dsp/ChebyshevSet.h) and TerrainImage
    (dsp/TerrainImage.h). Moved out of PluginProcessor.h in Round B (plan
    Decision 33) so the DSP headers can derive from it without the processor.

  ==============================================================================
*/

#pragma once

struct Retirable
{
    virtual ~Retirable() = default;
};
