/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    SubOscillator.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "SubOscillator.h"
#include "MathConstants.h"

void SubOscillator::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    phaseIncrement = baseFrequency * std::pow (2.0, octaveOffset) / currentSampleRate;
}

void SubOscillator::reset()
{
    phase = 0.0;
    triState = 0.0;
}

void SubOscillator::setFrequency (double freq)
{
    baseFrequency = freq;
    phaseIncrement = baseFrequency * std::pow (2.0, octaveOffset) / currentSampleRate;
}

void SubOscillator::setShape (int shape)
{
    currentShape = shape;
}

void SubOscillator::setOctave (int octave)
{
    octaveOffset = octave;
    phaseIncrement = baseFrequency * std::pow (2.0, octaveOffset) / currentSampleRate;
}

double SubOscillator::getNextSample()
{
    double output = 0.0;
    double dt = phaseIncrement;

    switch (currentShape)
    {
        case 0: // Sine
            output = std::sin (phase * kTwoPi);
            break;

        case 1: // Triangle (leaky-integrated polyBLEP square)
        {
            double sq = (phase < 0.5) ? 1.0 : -1.0;
            sq += polyBLEP (phase, dt);
            sq -= polyBLEP (std::fmod (phase + 0.5, 1.0), dt);
            // Leaky integrate
            triState = dt * sq + (1.0 - dt) * triState;
            output = triState * 4.0; // Scale
            break;
        }

        case 2: // Saw
            output = 2.0 * phase - 1.0;
            output -= polyBLEP (phase, dt);
            break;

        case 3: // Square
            output = (phase < 0.5) ? 1.0 : -1.0;
            output += polyBLEP (phase, dt);
            output -= polyBLEP (std::fmod (phase + 0.5, 1.0), dt);
            break;
    }

    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return output;
}
