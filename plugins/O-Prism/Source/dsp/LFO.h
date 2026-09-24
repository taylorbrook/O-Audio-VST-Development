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

    LFO.h
    O-Prism - Microtonal Wavetable Synthesizer
    Generic, reusable LFO class for per-voice modulation

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class LFO
{
public:
    enum class Shape { Sine, Triangle, Saw, Square, SampleAndHold };

    LFO() = default;

    void prepare (double sampleRate);
    void reset();

    void setRate (float hz);
    void setShape (Shape shape);

    /** Returns bipolar output in range [-1, 1] */
    float getNextSample();

    /** Set phase directly in [0, 1). Used for free-running sync from global phase. */
    void setPhase (double newPhase) { phase = newPhase; }

    /** Get current phase in [0, 1). */
    double getPhase() const { return phase; }

    /** WR-06: seed the Sample & Hold random stream deterministically.
        A default-constructed juce::Random calls setSeedRandomly() — ticks,
        wall clock and the object address — so every instantiation drew a
        different stream and two bounces of the same project with S&H on an
        LFO differed sample-for-sample. The seed is stored and re-applied by
        prepare(), which the host calls before each render, so a render always
        starts the stream from the same place. */
    void setSeed (juce::uint32 seed)
    {
        rngSeed = seed;
        random.setSeed (static_cast<juce::int64> (seed));
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    Shape currentShape = Shape::Sine;
    float shHeldValue = 0.0f;

    // WR-06: explicitly seeded at construction too, so an LFO whose owner
    // never calls setSeed() is still reproducible rather than clock-seeded.
    static constexpr juce::uint32 kDefaultSeed = 0x9E3779B9u;
    juce::uint32 rngSeed = kDefaultSeed;
    juce::Random random { static_cast<juce::int64> (kDefaultSeed) };
};
