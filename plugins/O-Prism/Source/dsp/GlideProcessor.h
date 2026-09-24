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
#include <algorithm>
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
        currentLog = targetLog;
    }

    /** Force the glide's starting frequency. Used to seed a fresh voice from
        the processor-level last-played note so "Always" mode actually glides
        polyphonically (WR-06). */
    void startFrom (double freq)
    {
        currentFreq = freq;
        currentLog = toLog (freq);
    }

    /** Set the target. glideIn=false (or mode Off) snaps immediately; the
        caller decides the legato/always gating (WR-06). */
    void setTarget (double freq, bool glideIn)
    {
        targetFreq = freq;
        targetLog = toLog (freq);

        if (mode == 0 || ! glideIn)
        {
            currentFreq = targetFreq;
            currentLog = targetLog;
        }
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
            currentLog = targetLog;
            return currentFreq;
        }

        // WR-08: the one-pole runs on log2(frequency), not on frequency.
        //
        // In the linear-Hz domain a glide covers equal numbers of HERTZ per
        // sample, but perceived pitch is logarithmic — so a C2 to C5 glide
        // crawled through the bottom octave and crossed the top one in a
        // fraction of the time. For a plugin whose premise is microtonal pitch
        // accuracy that is the wrong domain. Smoothing the pitch makes the
        // glide cover equal numbers of CENTS per sample, so its perceived rate
        // is constant and independent of register.
        //
        // The one-pole SHAPE is unchanged (same coefficient, same asymptotic
        // approach) — only the domain moves. Cost is one exp2 per sample, and
        // only while a glide is actually running: the early-out above returns
        // before it once the note has arrived.
        currentLog = currentLog * glideCoeff + targetLog * (1.0 - glideCoeff);
        currentFreq = std::exp2 (currentLog);
        return currentFreq;
    }

    double getCurrentFrequency() const { return currentFreq; }

private:
    /** log2 of a frequency, floored off zero. targetFreq can legitimately be 0
        before the first note (currentFreq/targetFreq both default to 440, but a
        caller could hand us 0), and log2(0) is -inf, which would poison
        currentLog permanently — exp2(-inf) is 0 and the one-pole never
        recovers. 1e-6 Hz is ~20 octaves below hearing and finite. */
    static double toLog (double freq)
    {
        return std::log2 (std::max (freq, 1.0e-6));
    }

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

    // WR-08: the smoothed state. Kept in step with currentFreq/targetFreq at
    // every mutation so getCurrentFrequency() and the convergence test above
    // stay valid.
    double currentLog = 8.7813597135246596; // log2(440)
    double targetLog  = 8.7813597135246596;
    double glideTime = 0.1;
    double glideCoeff = 0.0;
    int mode = 0;     // 0=Off, 1=Legato, 2=Always
};
