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

    AspirationNoise.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Per-voice pitch-synchronous aspiration noise with:
    - Asymmetric triangular glottal envelope (dual-peak: open-phase ramp + closure burst)
    - Breathiness-dependent spectral tilt (one-pole LP: 2kHz breathy, 6kHz pressed)
    - Stochastic drift on amplitude and tilt cutoff (~75ms random walk)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

class AspirationNoise
{
public:
    explicit AspirationNoise (int seed = 0)
        : random (seed)
    {
    }

    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        breathSmoothed.reset (sr, 0.020);  // 20ms ramp
        driftAmpSmoothed.reset (sr, 0.050); // 50ms ramp for drift interpolation
        driftTiltSmoothed.reset (sr, 0.050);
        driftAmpSmoothed.setCurrentAndTargetValue (0.0f);
        driftTiltSmoothed.setCurrentAndTargetValue (0.0f);
        driftUpdateInterval = static_cast<int> (sr * 0.075); // ~75ms
        if (driftUpdateInterval < 1) driftUpdateInterval = 1;
        glottalPhase = 0.0f;
    }

    void setBreathiness (float breath) noexcept
    {
        breathSmoothed.setTargetValue (breath);
    }

    void setGlottalPhase (float phase01) noexcept
    {
        glottalPhase = phase01;
    }

    inline float process (float glottalSample) noexcept
    {
        // Generate white noise [-1, 1]
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // --- (1) Asymmetric triangular envelope with closure burst ---
        // Dual-peak pattern: ramp during open phase, burst at glottal closure
        static constexpr float openPhaseEnd = 0.6f;
        static constexpr float burstWidth   = 0.05f;
        static constexpr float noiseFloor   = 0.3f;

        float noiseGain;
        if (glottalPhase < openPhaseEnd)
        {
            // Open phase: linear rise from floor to peak
            noiseGain = noiseFloor + (1.0f - noiseFloor) * (glottalPhase / openPhaseEnd);
        }
        else if (glottalPhase < openPhaseEnd + burstWidth)
        {
            // Closure burst: brief spike decaying to floor
            float burstProgress = (glottalPhase - openPhaseEnd) / burstWidth;
            noiseGain = 1.0f - (1.0f - noiseFloor) * burstProgress;
        }
        else
        {
            // Closed phase: floor
            noiseGain = noiseFloor;
        }

        // --- (3) Stochastic drift — non-stationary breath turbulence ---
        updateDrift();
        float ampDriftLinear = juce::Decibels::decibelsToGain (driftAmpSmoothed.getNextValue());

        float modulatedNoise = noise * noiseGain * ampDriftLinear;

        // --- (2) Spectral tilt one-pole lowpass ---
        // High breathiness -> 2kHz (warm/airy), low breathiness -> 6kHz (hissy/pressed)
        float breath = breathSmoothed.getNextValue();
        float baseCutoff = 6000.0f - breath * 4000.0f;
        float cutoff = juce::jlimit (500.0f, 10000.0f,
                                     baseCutoff + driftTiltSmoothed.getNextValue());
        float alpha = std::exp (-juce::MathConstants<float>::twoPi * cutoff
                                / static_cast<float> (sampleRate));
        float filtered = (1.0f - alpha) * modulatedNoise + alpha * tiltPrev;
        // WR-06: cheap denormal flush on the one-pole state (defence-in-depth
        // alongside processBlock's ScopedNoDenormals) — the tilt feedback can
        // decay into subnormals once the breath source goes quiet on release.
        filtered += 1.0e-20f; filtered -= 1.0e-20f;
        tiltPrev = filtered;

        // Mix: voice/noise crossfade based on breathiness
        return (1.0f - breath) * glottalSample + breath * filtered;
    }

    void reset() noexcept
    {
        glottalPhase = 0.0f;
        // IN-09 (known onset behaviour): this seeds breath to 0.1 rather than the
        // caller's target. FormantVoice::noteStarted calls reset() then
        // setBreathiness(), so breath ramps 0.1 -> target over the smoothing time
        // at every note-on — a small attack-time breathiness sweep. Resetting to
        // getTargetValue() (or leaving breath untouched) would remove it, but that
        // changes the note-onset timbre, so the current default is preserved here.
        breathSmoothed.setCurrentAndTargetValue (0.1f);
        tiltPrev = 0.0f;
        driftAmpDb = 0.0f;
        driftTiltHz = 0.0f;
        driftAmpSmoothed.setCurrentAndTargetValue (0.0f);
        driftTiltSmoothed.setCurrentAndTargetValue (0.0f);
        driftCounter = 0;
    }

private:
    void updateDrift() noexcept
    {
        if (++driftCounter >= driftUpdateInterval)
        {
            driftCounter = 0;

            // Independent random walks for amplitude and tilt cutoff
            float ampStep = (random.nextFloat() * 2.0f - 1.0f) * 0.5f;
            driftAmpDb = juce::jlimit (-2.0f, 2.0f, driftAmpDb + ampStep);
            driftAmpSmoothed.setTargetValue (driftAmpDb);

            float tiltStep = (random.nextFloat() * 2.0f - 1.0f) * 50.0f;
            driftTiltHz = juce::jlimit (-200.0f, 200.0f, driftTiltHz + tiltStep);
            driftTiltSmoothed.setTargetValue (driftTiltHz);
        }
    }

    juce::Random random;
    double sampleRate = 44100.0;

    float glottalPhase = 0.0f;
    juce::SmoothedValue<float> breathSmoothed { 0.1f };

    // Spectral tilt one-pole state
    float tiltPrev = 0.0f;

    // Stochastic drift state
    float driftAmpDb  = 0.0f;
    float driftTiltHz = 0.0f;
    juce::SmoothedValue<float> driftAmpSmoothed { 0.0f };
    juce::SmoothedValue<float> driftTiltSmoothed { 0.0f };
    int driftCounter = 0;
    int driftUpdateInterval = 3307; // ~75ms at 44.1kHz, recalculated in prepare()
};
