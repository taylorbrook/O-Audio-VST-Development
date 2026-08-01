/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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

    O-simplePhysicalModelSynth - BowExciter

    Sustained excitation via a band-limited friction-noise drive (BowNoiseGenerator,
    copied header), the energy continuously fed into whichever resonator is selected
    so it sustains instead of decaying. The memoryless STK Stribeck friction curve
    (shared module HyperbolicFriction) sets the bow→resonator COUPLING strength
    (more force / lower slip velocity → higher reflection coefficient → more drive).

    Why noise, not a friction junction (CONTEXT D2): memoryless friction self-
    oscillates only in a DUAL-RAIL waveguide (the boundary sign-inversions create
    the Helmholtz corner — see O-Bowed). In v1.0's SINGLE KS loop it settles into a
    stick state (vΔ→0) and decays, so the documented friction-noise fallback drives
    the sustain. (The friction-junction path returns with the Waveguide string, v1.1.)

  ==============================================================================
*/

#pragma once
#include "HyperbolicFriction.h"
#include "BowNoiseGenerator.h"

class BowExciter
{
public:
    void prepare (double sr, int voiceIndex) noexcept
    {
        sampleRate = sr;
        noise.prepare (sr, voiceIndex);
        reset();
    }

    void reset() noexcept { noise.reset(); }

    // bowForce01 0–1 (Bow Force / 100); vel 0–1; color01 0–1.
    void setParams (float bowForce01, float vel, float /*color01*/) noexcept
    {
        const float force = juce::jlimit (0.0f, 1.0f, bowForce01);
        vBow  = 0.05f + 0.20f * juce::jlimit (0.0f, 1.0f, vel);   // slip velocity grows with velocity
        F_bow = juce::jmap (force, 0.2f, 3.0f);                   // pressure → friction depth

        noisePressure = force;
        noiseSpeed    = 0.3f + 0.7f * juce::jlimit (0.0f, 1.0f, vel);

        // Stribeck coupling: reflection coeff at the current bow gesture scales how
        // much friction-noise energy couples into the resonator (1 + 2ρ, ρ∈[0,~0.5]).
        coupling = 1.0f + 2.0f * frictionModel.computeReflectionCoefficient (vBow, F_bow);
    }

    // Continuous friction-noise drive — String (KS loop filters it to pitch) and
    // Modal (bandpass bank sustains its modes). Same source for both paths.
    float drive() noexcept { return noise.processSample (noisePressure, noiseSpeed, 1.0f) * coupling; }

private:
    HyperbolicFriction frictionModel;
    BowNoiseGenerator  noise;

    float  vBow          = 0.1f;
    float  F_bow         = 1.0f;
    float  noisePressure = 0.5f;
    float  noiseSpeed    = 0.5f;
    float  coupling      = 1.0f;
    double sampleRate    = 44100.0;
};
