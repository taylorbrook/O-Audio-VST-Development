/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    PitchGlide.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice exponential one-pole smoother for portamento.
    Adapted from O-Prism GlideProcessor pattern.

  ==============================================================================
*/

#pragma once
#include <cmath>

class PitchGlide
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        coeff = 0.0f;
    }

    void setTarget (float freqHz) noexcept
    {
        targetFreq = freqHz;
    }

    void snapTo (float freqHz) noexcept
    {
        currentFreq = freqHz;
        targetFreq = freqHz;
    }

    void setTime (float timeMs) noexcept
    {
        if (timeMs > 0.0f && sampleRate > 0.0)
            coeff = static_cast<float> (std::exp (-1.0 / (static_cast<double> (timeMs) * 0.001 * sampleRate)));
        else
            coeff = 0.0f;
    }

    inline float getNextFrequency() noexcept
    {
        if (coeff <= 0.0f || std::abs (currentFreq - targetFreq) < targetFreq * 0.00001f)
        {
            currentFreq = targetFreq;
            return currentFreq;
        }

        currentFreq = currentFreq * coeff + targetFreq * (1.0f - coeff);
        return currentFreq;
    }

    void reset() noexcept
    {
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        coeff = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float coeff = 0.0f;
};
