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

    Vibrato.h
    Modal Synthesis Bassoon - Per-Voice Sine LFO + Onset Envelope (Phase 2.3)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.3 deliverable: per-voice sine LFO with variable-duration onset
    SmoothedValue ramp. Output is multiplicative pitch-cents (depth × onset
    × sin(phase)) consumed by BassoonVoice::renderNextBlock and composed
    multiplicatively with pitch-bend at f_final = base × pow(2, c/1200) ×
    pow(2, pb/12) (locked OQ#7-rev-3).

    Random initial phase per startNote (locked OQ#9-rev-3, O-Wind
    FluteSynthVoice.cpp:114-116 precedent — overrides CONTEXT-rev-3 default
    instant-zero per D2-rev-3) ensures per-voice phase stagger across
    polyphony — confirmed audibly at Gate 3 item 7.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Vibrato
{
public:
    Vibrato() = default;
    ~Vibrato() = default;

    void  prepare        (double sr) noexcept;
    void  reset          () noexcept;
    void  setRateHz      (float r) noexcept;
    void  setDepthCents  (float d) noexcept;
    void  setOnsetMs     (float ms) noexcept;
    float getCurrentCents() noexcept;

    // Stage 3 / Phase 3.2 — UI feedback accessor.
    // Returns the onset envelope value (0..1) for the vibrato pulsing dot.
    // Header-inline; no .cpp touched.
    float getEnvelope() const noexcept { return onset.getCurrentValue(); }

private:
    void  recomputeIncrement() noexcept;

    double sampleRate            = 48000.0;
    float  rateHz                = 5.0f;
    float  depthCents            = 0.0f;
    float  phase                 = 0.0f;
    float  phaseIncrement        = 0.0f;
    double onsetDurationSeconds  = 0.0;   // cache; consumed at reset()

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> onset { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vibrato)
};
