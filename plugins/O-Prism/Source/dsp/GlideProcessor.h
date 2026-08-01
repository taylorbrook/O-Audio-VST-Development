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

    GlideProcessor.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio (Header-only)

  ==============================================================================
*/

#pragma once
#include <cmath>

class GlideProcessor
{
public:
    GlideProcessor() = default;

    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
        updateCoefficient();
    }

    void reset()
    {
        currentFreq = targetFreq;
    }

    /** Force the glide's starting frequency. Used to seed a fresh voice from
        the processor-level last-played note so "Always" mode actually glides
        polyphonically (WR-06). */
    void startFrom (double freq)
    {
        currentFreq = freq;
    }

    /** Set the target. glideIn=false (or mode Off) snaps immediately; the
        caller decides the legato/always gating (WR-06). */
    void setTarget (double freq, bool glideIn)
    {
        targetFreq = freq;
        if (mode == 0 || ! glideIn)
            currentFreq = targetFreq;
    }

    void setMode (int m) { mode = m; }

    void setTime (double seconds)
    {
        glideTime = seconds;
        updateCoefficient();
    }

    double getNextFrequency()
    {
        if (mode == 0 || std::abs (currentFreq - targetFreq) < targetFreq * 0.00001)
        {
            currentFreq = targetFreq;
            return currentFreq;
        }

        currentFreq = currentFreq * glideCoeff + targetFreq * (1.0 - glideCoeff);
        return currentFreq;
    }

    double getCurrentFrequency() const { return currentFreq; }

private:
    void updateCoefficient()
    {
        if (glideTime > 0.0 && currentSampleRate > 0.0)
            glideCoeff = std::exp (-1.0 / (glideTime * currentSampleRate));
        else
            glideCoeff = 0.0;
    }

    double currentSampleRate = 44100.0;
    double currentFreq = 440.0;
    double targetFreq = 440.0;
    double glideTime = 0.1;
    double glideCoeff = 0.0;
    int mode = 0;     // 0=Off, 1=Legato, 2=Always
};
