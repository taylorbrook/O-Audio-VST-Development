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

      * POP   (vinyl jumps): a THREE-CLASS taxonomy since v1.7.0 — see below.
              Level scaled by VINYL_POP, +/-3 dB per-pop variation, all rolls
              on the artifactSynth RNG stream at the jump instant.
      * TICK  (CD mute residual): single impulse -> ~4 kHz SVF bandpass,
              +/-3 dB variation.
      * CHIRP (CD loop restart): direct-synthesized decaying sine sweep,
              ~3 kHz -> 8 kHz over ~4 ms, tau ~= 1.5 ms.

    THE POP TAXONOMY (v1.7.0, improvement brief item 17)
    ----------------------------------------------------
    Every vinyl pop used to be the same shape: a +/- doublet through a
    FirstOrderTPT lowpass with only +/-3 dB and 1-3 kHz of variation. A
    one-pole CANNOT ring, so no pop had the damped stylus resonance that reads
    as "the stylus hit something" — the whole family sounded like one sample
    with a random gain. triggerPop now draws a class:

      tick    (~70%) — the historical doublet -> FirstOrderTPT LP, cutoff
                       biased HIGH (2-4 kHz) so it reads as the dry
                       surface-debris click it always was.
      pop     (~25%) — the same doublet exciting a damped 2nd-order resonator
                       (StateVariableTPT bandpass, 900-1800 Hz, Q 4-8). The
                       click peak is normalised to the same base level as the
                       tick; the RING that follows sits 7-13 dB under it and
                       decays to -30 dB in 2.4-9.8 ms across the (f0, Q) box
                       (measured, not asserted from the brief's "1-3 ms",
                       which is inconsistent with Q 4-8 at those frequencies).
      scratch  (~5%) — a 2-8 ms band-limited noise burst under a raised-cosine
                       window, plus a 40-80 Hz decaying sine: the tonearm
                       thump under the rasp.

    FIXED DRAW COUNT. triggerPop consumes EXACTLY five artifactSynth draws on
    every call, before any branch and regardless of class or of level 0 — the
    same discipline the two-draw version had, for the same reason: a seeded
    render must not change its event sequence because a probe silenced the
    pops. Draws 4 and 5 are unused by the tick class and are consumed anyway.

    The scratch burst is the one thing here that needs a per-SAMPLE draw, and
    it takes them from its own dedicated RngBank::scratch stream. A private
    stream on a sample-count schedule is block-size invariant; sharing the
    tick-scheduled artifactSynth stream with a per-sample consumer would not be
    (pattern_rng_stream_interleave_blocksize).

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

        ringFilter.prepare (spec);
        ringFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        ringFilter.setCutoffFrequency (1350.0f);
        ringFilter.setResonance (6.0f);

        chirpFreqSlope   = (8000.0 - 3000.0) / (0.004 * fs);       // Hz per sample
        chirpDecayCoef   = std::exp (-1.0 / (0.0015 * fs));        // tau ~= 1.5 ms
        chirpLenSamples  = juce::jmax (1, static_cast<int> (0.008 * fs));

        thumpDecayCoef   = std::exp (-1.0 / (kThumpTauSeconds * fs));

        reset();
    }

    void reset() noexcept
    {
        popFilter.reset();
        tickFilter.reset();
        ringFilter.reset();
        popAge  = -1;  popAmp  = 0.0f;  popToRing = false;
        tickAge = -1;  tickAmp = 0.0f;
        chirpAge = -1; chirpPhase = 0.0; chirpFreq = 3000.0; chirpEnv = 0.0;
        scratchAge = -1; scratchLen = 1; scratchAmp = 0.0f;
        thumpPhase = 0.0; thumpInc = 0.0; thumpEnv = 0.0;
    }

    // Vinyl pop. Consumes EXACTLY 5 draws from `rng` on EVERY call — including
    // level 0, and including the two that the tick class does not read — so a
    // probe that silences pops keeps an identical draw sequence. All five are
    // taken up front, before the class branch, for the same reason.
    void triggerPop (float level01, juce::Random& rng) noexcept
    {
        const float rClass = rng.nextFloat();                               // class
        const float rLevel = rng.nextFloat();                               // +/-3 dB
        const float rFreq  = rng.nextFloat();                               // cutoff / f0
        const float rShape = rng.nextFloat();                               // Q / burst len
        const float rSub   = rng.nextFloat();                               // thump f0

        const float gain = kBaseLevel * juce::jlimit (0.0f, 1.0f, level01)
                           * std::pow (10.0f, (3.0f * (rLevel * 2.0f - 1.0f)) / 20.0f);

        if (rClass < kTickWeight)
        {
            // ── tick: the historical doublet, cutoff biased HIGH ──────────
            popToRing = false;

            const float cutoff = 2000.0f + rFreq * 2000.0f;                 // 2-4 kHz
            popFilter.setCutoffFrequency (cutoff);

            // First-order TPT LP unit-impulse peak = G = g/(1+g); compensate
            // so the click lands near the base level.
            const double g = std::tan (juce::MathConstants<double>::pi * cutoff / fs);
            popAmp = gain * static_cast<float> (juce::jmin (20.0, (1.0 + g) / g));
            popAge = 0;
        }
        else if (rClass < kTickWeight + kPopWeight)
        {
            // ── pop: the same doublet into a damped 2nd-order resonator ───
            popToRing = true;

            const float f0 = 900.0f + rFreq * 900.0f;                       // 900-1800 Hz
            const float q  = 4.0f + rShape * 4.0f;                          // Q 4-8

            ringFilter.setCutoffFrequency (f0);
            ringFilter.setResonance (q);

            // The SVF bandpass's doublet response PEAKS on its first sample at
            // h*g, and h -> 1 for f0 << fs, so 1/g normalises the click to the
            // same base level the tick gets. Verified numerically across the
            // whole (f0, Q) box: 1/g is within 3% of the true 1/peak there.
            // The ring that follows is 7-13 dB under the click, which is the
            // shape a stylus impact actually has.
            const double g = std::tan (juce::MathConstants<double>::pi * f0 / fs);
            popAmp = gain * static_cast<float> (juce::jmin (20.0, 1.0 / g));
            popAge = 0;
        }
        else
        {
            // ── scratch: noise burst + tonearm thump ──────────────────────
            // Deliberately does NOT touch popToRing: it fires no doublet, and
            // a scratch landing on the same sample as an in-flight tick/pop
            // must not re-route that doublet's remaining half to the other
            // filter (reachable — Arbitration's vinyl branch can trigger twice
            // in one tick).
            const float cutoff = 3000.0f + rFreq * 3000.0f;                 // 3-6 kHz
            popFilter.setCutoffFrequency (cutoff);

            scratchLen = juce::jmax (1, static_cast<int> ((0.002 + 0.006 * rShape) * fs));
            scratchAge = 0;

            // A white sequence of unit variance leaves the TPT lowpass with
            // RMS sqrt(G), G = g/(1+g) — closed form, so the burst's level is
            // sample-rate invariant rather than drifting with fs
            // (pattern_noise_bed_level_is_rate_dependent). Drive it so the
            // FILTERED burst lands at kBaseLevel/3 RMS, i.e. a ~3-sigma peak
            // at the same base level the other two classes get. The uniform
            // draws below have RMS 1/sqrt(3), hence the sqrt(3.0) here.
            const double g = std::tan (juce::MathConstants<double>::pi * cutoff / fs);
            const double G = g / (1.0 + g);
            scratchAmp = gain / 3.0f
                         * static_cast<float> (std::sqrt (3.0) / std::sqrt (G));

            // Tonearm thump: 40-80 Hz decaying sine, direct-synthesized. Phase
            // starts at 0, so sin() starts at 0 — no step into the bus.
            thumpInc   = juce::MathConstants<double>::twoPi
                             * (40.0 + 40.0 * static_cast<double> (rSub)) / fs;
            thumpPhase = 0.0;
            thumpEnv   = static_cast<double> (gain) * kThumpLevel;
        }
    }

    // UI telemetry only: a pop artifact is being injected right now. Covers all
    // three classes — a scratch is a pop as far as the LED is concerned.
    bool popActive() const noexcept { return popAge >= 0 || scratchAge >= 0; }

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
    //
    // `scratchRng` is the DEDICATED RngBank::scratch stream and is drawn from
    // once per sample only while a scratch burst is in flight — a private
    // stream on a sample-count schedule, which is block-size invariant. It is
    // never touched by the other two classes, so a render with no scratch in it
    // leaves the stream exactly where it started.
    float renderSample (juce::Random& scratchRng) noexcept
    {
        // Pop, tick + scratch classes: +/- doublet or noise burst. Both filters
        // run every sample regardless, so their state stays continuous.
        float lpIn  = 0.0f;
        float resIn = 0.0f;

        if (popAge >= 0)
        {
            const float sign = (popAge == 0) ? 1.0f : (popAge == 1 ? -1.0f : 0.0f);

            if (popToRing) resIn = sign * popAmp;
            else           lpIn  = sign * popAmp;

            if (++popAge > 2)
                popAge = -1;                               // input dead; filter rings
        }

        // Scratch burst: raised-cosine window over the burst so neither end is
        // a step, and the same LPF the tick class uses gives it its top.
        if (scratchAge >= 0)
        {
            const float w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                                     * static_cast<float> (scratchAge)
                                                     / static_cast<float> (scratchLen)));
            lpIn += scratchAmp * w * (scratchRng.nextFloat() * 2.0f - 1.0f);

            if (++scratchAge >= scratchLen)
                scratchAge = -1;
        }

        const float popOut = popFilter.processSample (0, lpIn)
                             + ringFilter.processSample (0, resIn);

        // Tonearm thump under a scratch. Bounded by its own envelope floor, so
        // it stops contributing (and stops costing a sin()) once it is inaudible.
        float thumpOut = 0.0f;
        if (thumpEnv > 0.0)
        {
            thumpPhase += thumpInc;
            if (thumpPhase >= juce::MathConstants<double>::twoPi)
                thumpPhase -= juce::MathConstants<double>::twoPi;

            thumpEnv *= thumpDecayCoef;
            thumpOut  = static_cast<float> (std::sin (thumpPhase) * thumpEnv);

            if (thumpEnv < 1.0e-7)
                thumpEnv = 0.0;
        }

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

        return popOut + thumpOut + tickOut + chirpOut;
    }

private:
    static constexpr float kBaseLevel = 0.126f;   // ~-18 dBFS

    // Class weights (brief item 17's ~70/25/5). Scratch is whatever is left, so
    // the three always sum to exactly 1 by construction.
    static constexpr float kTickWeight = 0.70f;
    static constexpr float kPopWeight  = 0.25f;

    // Tonearm thump under a scratch: level relative to the class's base gain,
    // and its decay. Deliberately under the rasp rather than beside it.
    static constexpr double kThumpLevel      = 0.5;
    static constexpr double kThumpTauSeconds = 0.025;

    double fs = 48000.0;

    juce::dsp::FirstOrderTPTFilter<float>    popFilter;
    juce::dsp::StateVariableTPTFilter<float> tickFilter;
    juce::dsp::StateVariableTPTFilter<float> ringFilter;   // pop-class resonator

    int   popAge    = -1;
    float popAmp    = 0.0f;
    bool  popToRing = false;   // the live doublet's destination filter
    int   tickAge   = -1;
    float tickAmp   = 0.0f;

    int    scratchAge = -1;
    int    scratchLen = 1;
    float  scratchAmp = 0.0f;

    double thumpPhase     = 0.0;
    double thumpInc       = 0.0;
    double thumpEnv       = 0.0;
    double thumpDecayCoef = 0.0;

    int    chirpAge        = -1;
    int    chirpLenSamples = 1;
    double chirpPhase      = 0.0;
    double chirpFreq       = 3000.0;
    double chirpEnv        = 0.0;
    double chirpFreqSlope  = 0.0;
    double chirpDecayCoef  = 0.0;
};
