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

    LFO.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Generic, reusable LFO class for per-voice modulation

  ==============================================================================
*/

#include "LFO.h"

void LFO::prepare (double sr)
{
    sampleRate = sr;
    phase = 0.0;
    phaseIncrement = 0.0;
    shHeldValue = 0.0f;
}

void LFO::reset()
{
    phase = 0.0;
    shHeldValue = random.nextFloat() * 2.0f - 1.0f;
}

void LFO::setRate (float hz)
{
    phaseIncrement = static_cast<double> (hz) / sampleRate;
}

void LFO::setShape (Shape shape)
{
    currentShape = shape;
}

float LFO::getNextSample()
{
    float output = 0.0f;

    switch (currentShape)
    {
        case Shape::Sine:
            output = static_cast<float> (std::sin (2.0 * juce::MathConstants<double>::pi * phase));
            break;

        case Shape::Triangle:
            output = static_cast<float> (4.0 * std::abs (phase - 0.5) - 1.0);
            break;

        case Shape::Saw:
            output = static_cast<float> (2.0 * phase - 1.0);
            break;

        case Shape::Square:
            output = phase < 0.5 ? 1.0f : -1.0f;
            break;

        case Shape::SampleAndHold:
            output = shHeldValue;
            break;
    }

    // Advance phase
    phase += phaseIncrement;

    // Wrap and trigger S&H on wrap
    if (phase >= 1.0)
    {
        phase -= 1.0;
        if (currentShape == Shape::SampleAndHold)
            shHeldValue = random.nextFloat() * 2.0f - 1.0f;
    }

    return output;
}
