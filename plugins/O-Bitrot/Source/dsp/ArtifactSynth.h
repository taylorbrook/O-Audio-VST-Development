/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - ArtifactSynth (Stage 2, Phase 2.2)

    Synthesized mechanical artifacts, all ~-18 dBFS pre-mix (levels tuned by
    harness renders):

      * POP   (vinyl jumps): +/- impulse pair -> 1-3 kHz FirstOrderTPT LPF,
              level scaled by VINYL_POP with +/-3 dB per-pop variation and a
              per-pop cutoff roll (artifactSynth RNG stream, consumed at the
              jump instant only).
      * TICK  (CD mute residual): single impulse -> ~4 kHz SVF bandpass,
              +/-3 dB variation.
      * CHIRP (CD loop restart): direct-synthesized decaying sine sweep,
              ~3 kHz -> 8 kHz over ~4 ms, tau ~= 1.5 ms.

    The post-filters run EVERY sample — including silence — so the IIR state
    stays continuous across events (O-Polystutter RepeatLane lesson). With no
    trigger ever fired the filters compute exact 0.0f from zero state/input,
    so adding the artifact bus preserves the FUNC-02 bit-exact null.

    Artifacts are MONO and added to both channels: the failure is the player,
    not the channels.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ArtifactSynth
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;

        const juce::dsp::ProcessSpec spec { fs, 512u, 1u };

        popFilter.prepare (spec);
        popFilter.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        popFilter.setCutoffFrequency (2000.0f);

        tickFilter.prepare (spec);
        tickFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        tickFilter.setCutoffFrequency (4000.0f);
        tickFilter.setResonance (0.707f);

        chirpFreqSlope   = (8000.0 - 3000.0) / (0.004 * fs);       // Hz per sample
        chirpDecayCoef   = std::exp (-1.0 / (0.0015 * fs));        // tau ~= 1.5 ms
        chirpLenSamples  = juce::jmax (1, static_cast<int> (0.008 * fs));

        reset();
    }

    void reset() noexcept
    {
        popFilter.reset();
        tickFilter.reset();
        popAge  = -1;  popAmp  = 0.0f;
        tickAge = -1;  tickAmp = 0.0f;
        chirpAge = -1; chirpPhase = 0.0; chirpFreq = 3000.0; chirpEnv = 0.0;
    }

    // Vinyl pop. Consumes 2 draws from `rng` (level variation + cutoff) on
    // EVERY call — including level 0 — so probes that silence pops keep an
    // identical draw sequence.
    void triggerPop (float level01, juce::Random& rng) noexcept
    {
        const float dbVar  = 3.0f * (rng.nextFloat() * 2.0f - 1.0f);        // +/-3 dB
        const float cutoff = 1000.0f + rng.nextFloat() * 2000.0f;           // 1-3 kHz

        popFilter.setCutoffFrequency (cutoff);

        // First-order TPT LP unit-impulse peak = G = g/(1+g); compensate so
        // the pop lands near the base level.
        const double g    = std::tan (juce::MathConstants<double>::pi * cutoff / fs);
        const double comp = juce::jmin (20.0, (1.0 + g) / g);

        popAmp = kBaseLevel * juce::jlimit (0.0f, 1.0f, level01)
                 * std::pow (10.0f, dbVar / 20.0f)
                 * static_cast<float> (comp);
        popAge = 0;
    }

    // UI telemetry only: a pop impulse is being injected this-or-next sample.
    bool popActive() const noexcept { return popAge >= 0; }

    // CD mute residual tick. Consumes 1 draw.
    void triggerTick (juce::Random& rng) noexcept
    {
        const float dbVar = 3.0f * (rng.nextFloat() * 2.0f - 1.0f);
        tickAmp = kBaseLevel * std::pow (10.0f, dbVar / 20.0f) * 4.0f;   // rough BP makeup
        tickAge = 0;
    }

    // CD loop restart chirp. No draws.
    void triggerChirp() noexcept
    {
        chirpAge   = 0;
        chirpPhase = 0.0;
        chirpFreq  = 3000.0;
        chirpEnv   = static_cast<double> (kBaseLevel);
    }

    // Mono artifact bus for this sample. MUST be called every sample.
    float renderSample() noexcept
    {
        // Pop: +/- impulse pair through the LPF (filter always runs).
        float popIn = 0.0f;
        if (popAge == 0)      popIn =  popAmp;
        else if (popAge == 1) popIn = -popAmp;
        if (popAge >= 0 && ++popAge > 2)
            popAge = -1;                                   // input dead; filter rings
        const float popOut = popFilter.processSample (0, popIn);

        // Tick: single impulse through the BPF (filter always runs).
        const float tickIn = (tickAge == 0) ? tickAmp : 0.0f;
        if (tickAge >= 0 && ++tickAge > 1)
            tickAge = -1;
        const float tickOut = tickFilter.processSample (0, tickIn);

        // Chirp: direct synthesis, bounded lifetime.
        float chirpOut = 0.0f;
        if (chirpAge >= 0)
        {
            chirpPhase += juce::MathConstants<double>::twoPi * chirpFreq / fs;
            chirpFreq   = juce::jmin (8000.0, chirpFreq + chirpFreqSlope);
            chirpEnv   *= chirpDecayCoef;
            chirpOut    = static_cast<float> (std::sin (chirpPhase) * chirpEnv);

            if (++chirpAge > chirpLenSamples)
                chirpAge = -1;
        }

        return popOut + tickOut + chirpOut;
    }

private:
    static constexpr float kBaseLevel = 0.126f;   // ~-18 dBFS

    double fs = 48000.0;

    juce::dsp::FirstOrderTPTFilter<float>    popFilter;
    juce::dsp::StateVariableTPTFilter<float> tickFilter;

    int   popAge  = -1;
    float popAmp  = 0.0f;
    int   tickAge = -1;
    float tickAmp = 0.0f;

    int    chirpAge        = -1;
    int    chirpLenSamples = 1;
    double chirpPhase      = 0.0;
    double chirpFreq       = 3000.0;
    double chirpEnv        = 0.0;
    double chirpFreqSlope  = 0.0;
    double chirpDecayCoef  = 0.0;
};
