/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive - Oscillator Bank (band-limited source, per voice)

    Generates the harmonically-rich source the filter shapes (DSP-01):

      src = mainOsc(oscWave, f0) + subLevel * subSquare(f0/2) + noiseLevel * white

    Anti-aliasing (ARCHITECTURE.md §"Anti-Aliasing — PolyBLEP / polyBLAMP"):
      - Saw    : naive 2φ-1 minus a two-sided PolyBLEP residual at the φ=0 wrap.
      - Square : naive ±1 pulse plus PolyBLEP at the rising edge (φ=0) minus
                 PolyBLEP at the falling edge (φ=0.5).
      - Triangle: naive /\ wave plus polyBLAMP slope-corrections at both corners
                 (already 1/n² dark — minimal correction; amplitude is pitch-
                 independent because the naive triangle dominates).
      - Sine    : 1024-pt LookupTableTransform, exact (single partial, no AA).

    PolyBLEP composes here because the oscillator runs at a STEADY phase
    increment (no phase-modulation) — so NO oversampling is needed and latency
    stays zero. (This is the key divergence from O-simpleFM, which oversamples.)

    Phase discipline: never reset phase mid-note (click/zipper). reset() is
    called only on voice (re)allocation.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath>

class OscillatorBank
{
public:
    // Matches the APVTS `oscWave` choice order: Saw / Square / Triangle / Sine.
    enum Wave { Saw = 0, Square = 1, Triangle = 2, Sine = 3 };

    void setSampleRate (double sr) noexcept { fs = sr; }

    // Voice (re)allocation only — never mid-note.
    void reset() noexcept
    {
        mainPhase = 0.0f;
        subPhase  = 0.0f;
        rngState  = 0x9E3779B9u;   // deterministic seed (xorshift)
    }

    //==========================================================================
    // One sample of the mixed source. f0 is the (possibly glided) note frequency.
    float next (int wave, float f0, float subLevel, float noiseLevel) noexcept
    {
        const float dtMain = (float) (f0 / fs);

        float main = 0.0f;
        switch (wave)
        {
            case Saw:      main = sawSample      (mainPhase, dtMain); break;
            case Square:   main = squareSample   (mainPhase, dtMain); break;
            case Triangle: main = triangleSample (mainPhase, dtMain); break;
            case Sine:     main = sineSample     (mainPhase);         break;
            default:       main = sawSample      (mainPhase, dtMain); break;
        }
        advance (mainPhase, dtMain);

        // Sub: square one octave below. Always advanced (so toggling subLevel
        // never jumps phase); cost is negligible.
        const float f0sub = 0.5f * f0;
        const float dtSub  = (float) (f0sub / fs);
        const float sub    = squareSample (subPhase, dtSub);
        advance (subPhase, dtSub);

        const float noise = whiteNoise();

        return main + subLevel * sub + noiseLevel * noise;
    }

private:
    //==========================================================================
    static void advance (float& phase, float dt) noexcept
    {
        phase += dt;
        while (phase >= 1.0f) phase -= 1.0f;
        while (phase <  0.0f) phase += 1.0f;
    }

    // PolyBLEP step-discontinuity residual (Välimäki). t,dt in phase units.
    static float polyBlep (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt)            { const float x = t / dt;            return x + x - x * x - 1.0f; }
        if (t > 1.0f - dt)     { const float x = (t - 1.0f) / dt;   return x * x + x + x + 1.0f; }
        return 0.0f;
    }

    // polyBLAMP slope-discontinuity residual (integrated BLEP).
    static float polyBlamp (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt)            { const float x = t / dt - 1.0f;        return -1.0f / 3.0f * x * x * x; }
        if (t > 1.0f - dt)     { const float x = (t - 1.0f) / dt + 1.0f; return  1.0f / 3.0f * x * x * x; }
        return 0.0f;
    }

    static float sawSample (float phase, float dt) noexcept
    {
        float s = 2.0f * phase - 1.0f;          // naive ramp [-1, +1)
        s -= polyBlep (phase, dt);              // correct the φ=0 wrap discontinuity
        return s;
    }

    static float squareSample (float phase, float dt) noexcept
    {
        float s = (phase < 0.5f) ? 1.0f : -1.0f;            // naive 50% pulse
        s += polyBlep (phase, dt);                          // rising edge at φ=0
        float ph2 = phase + 0.5f; if (ph2 >= 1.0f) ph2 -= 1.0f;
        s -= polyBlep (ph2, dt);                            // falling edge at φ=0.5
        return s;
    }

    static float triangleSample (float phase, float dt) noexcept
    {
        // Naive triangle: -1 at φ=0, +1 at φ=0.5, back to -1 at φ=1. Slope ±4.
        const float naive = (phase < 0.5f) ? (4.0f * phase - 1.0f)
                                           : (3.0f - 4.0f * phase);
        // Slope jumps +8 at φ=0 and -8 at φ=0.5 → polyBLAMP corrections (× dt).
        float ph2 = phase + 0.5f; if (ph2 >= 1.0f) ph2 -= 1.0f;
        float s = naive;
        s += 8.0f * dt * polyBlamp (phase, dt);
        s -= 8.0f * dt * polyBlamp (ph2,  dt);
        return s;
    }

    static float sineSample (float phase) noexcept
    {
        struct SineTable
        {
            juce::dsp::LookupTableTransform<float> t;
            SineTable()
            {
                t.initialise ([] (float x) { return std::sin (x); },
                              0.0f, juce::MathConstants<float>::twoPi, 1024);
            }
        };
        static const SineTable table;
        // Phase is already wrapped to [0,1); LUT clamps out-of-range, so map to radians safely.
        const float rad = juce::jlimit (0.0f, juce::MathConstants<float>::twoPi,
                                        phase * juce::MathConstants<float>::twoPi);
        return table.t (rad);
    }

    // Fast xorshift32 white noise in [-1, +1].
    float whiteNoise() noexcept
    {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return (float) ((int32_t) rngState) * (1.0f / 2147483648.0f);
    }

    double   fs        = 44100.0;
    float    mainPhase = 0.0f;
    float    subPhase  = 0.0f;
    uint32_t rngState  = 0x9E3779B9u;
};
