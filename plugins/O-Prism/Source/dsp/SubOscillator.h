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

    SubOscillator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <cmath>

class SubOscillator
{
public:
    SubOscillator() = default;

    void prepare (double sampleRate);
    void reset();
    void setFrequency (double freq);
    void setShape (int shape);
    void setOctave (int octave);
    double getNextSample();

private:
    static inline double polyBLEP (double t, double dt)
    {
        if (t < dt)       { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    double currentSampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double baseFrequency = 220.0;
    int currentShape = 0;   // 0=Sine, 1=Triangle, 2=Saw, 3=Square
    int octaveOffset = -1;  // -2, -1, or 0

    // Triangle state (leaky integrator of square)
    double triState = 0.0;
};
