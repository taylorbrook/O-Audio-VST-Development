/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

    JetNonlinearity.h
    O-Wind - Jet-Labium Nonlinearity (tanh saturation)
    Ouaricon Audio
    Developer: Taylor Brook

    Models amplitude-limiting interaction where jet hits labium.
    Core nonlinear element that sustains oscillation and shapes waveform.
    Supports reversed jet for creative sound design.

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

class JetNonlinearity
{
public:
    void setJetGain (float gain) { jetGain = gain; }
    void setReversedJet (float amount) { reversedJetParam = amount; }

    // Per-sample: Verge (1995) jet-labium nonlinearity
    // jetVelocity sets saturation amplitude; small-signal gain = jetGain (constant)
    float processSample (float jetOutput, float jetVelocity) const
    {
        // No excitation without breath — prevents residual waveguide drive after release
        if (jetVelocity < 0.001f)
            return 0.0f;

        // Safety clamp input (wide range — tanh does the real soft-limiting)
        float clamped = juce::jlimit (-10.0f, 10.0f, jetOutput);

        // Reversed jet: phase-invert jet signal before nonlinearity
        // Changes phase relationship between jet and bore -> different resonance modes
        float effectiveJet = clamped;
        if (reversedJetParam > 0.0f)
            effectiveJet = juce::jmap (reversedJetParam, clamped, -clamped);

        // Verge (1995): vel * tanh(gain * input / vel)
        // Small-signal gain = jetGain (independent of breath pressure)
        // Saturation amplitude = jetVelocity (proportional to breath pressure)
        return jetVelocity * std::tanh (jetGain * effectiveJet / jetVelocity);
    }

    void reset() {}

private:
    float jetGain = 2.0f;
    float reversedJetParam = 0.0f;
};
