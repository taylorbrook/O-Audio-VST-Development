/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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

    NoiseExciter.cpp
    Modal Synthesis Bassoon - Per-Voice Continuous Filtered-Noise Excitation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "NoiseExciter.h"

void NoiseExciter::prepare (double sr, int voiceIndex) noexcept
{
    sampleRate = sr;

    // Deterministic per-voice seed — O-Bowed BowNoiseGenerator.h:23 precedent
    // (locked OQ#3-rev-3, overrides CONTEXT-rev-3 default
    // Time::currentTimeMillis() ^ voiceIndex per D3-rev-3).
    rng.setSeed (static_cast<juce::int64> (voiceIndex) * 31337);

    // 1-pole low-pass coefficient: alpha = 1 - exp(-2π × fc / fs)
    lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                               * CUTOFF_HZ
                               / static_cast<float> (sampleRate));
    lpState = 0.0f;
}

void NoiseExciter::reset () noexcept
{
    // rng state preserved across notes (deterministic per-voice stream);
    // mode-bank reset already handles transient cleanup.
    lpState = 0.0f;
}

float NoiseExciter::getNextSample (float breathScaled) noexcept
{
    const float white = rng.nextFloat() * 2.0f - 1.0f;
    lpState += lpCoeff * (white - lpState);
    return lpState * BASE_NOISE_GAIN * breathScaled;
}
