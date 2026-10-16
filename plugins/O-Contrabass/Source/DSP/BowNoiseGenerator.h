/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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

    BowNoiseGenerator.h
    O-Contrabass - Bow Noise Generator (Phase 2.5, rebuilt v1.12.0)
    Ouaricon Audio
    Developer: Taylor Brook

    Header-only per O-Bowed convention (no CMake update required).
    Deterministic per-voice seed: voiceIndex * 31337 (RESEARCH §21.6).

    Public API:
      - prepare(double sampleRate, int voiceIndex)
      - setNoiseLevel(float)    noexcept   [0, 1]
      - setFundamentalHz(float) noexcept
      - triggerOnset(float velocity) noexcept
      - processSample(float relVel, float force) noexcept
      - reset()                 noexcept

    v1.12.0 — why it was rebuilt. Through v1.11 the noise followed a per-block
    bowEnergy = |v_bow|·F_bow, which is CONSTANT through a sustained stroke, plus
    a timer-guessed "slip burst" whose 21 ms decay outlasted a bass period and
    so smeared into more hiss. Measured on E1/A1/E2 (1–4 s of sustain): the
    noise owned the whole 2–8 kHz band (−0.4 dB of it), its 25 ms envelope moved
    0.6 dB, and its AM at f0 was 0.08 — a flat, static hiss sitting on the tone.

    Now the drive is the string's OWN friction, per sample: the voice passes
    |v_bow − v_string| at the bow point and F_bow. The slip excursion above the
    stick-phase creep floor gates the noise, so it pulses pitch-synchronously
    with the real Helmholtz motion and follows speed, bow position and the
    irregular attack without a heuristic; F_bow^¼ sets its level, and a
    velocity-scaled onset boost makes the first bite scratchier.
    On top of it, two slow random walks move level (±kWanderDb) and colour
    (body band ↔ rosin band) the way hair/rosin contact varies along a stroke.
    One RNG stream per role, a fixed number of draws per sample → the output is
    block-size invariant.

    Output = coloured noise · slip gate · F^¼ · (1 + onset) · wander · level

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class BowNoiseGenerator
{
public:
    // Not noexcept: allocates the comb delay buffer (prepare-time only).
    inline void prepare (double sampleRate, int voiceIndex)
    {
        sr = sampleRate;
        noiseRandom.setSeed  (static_cast<juce::int64> (voiceIndex * 31337));
        wanderRandom.setSeed (static_cast<juce::int64> (voiceIndex * 31337 + 7919));

        combBuffer.assign (static_cast<size_t> (std::ceil (sampleRate / kCombMinF0)) + 1, 0.0f);
        combWrite     = 0;
        combPeriod    = 0;
        combDampState = 0.0f;
        combDampCoeff = onePole (4000.0);

        juce::dsp::ProcessSpec spec { sampleRate, 1u, 1u };
        bodyBand.prepare (spec);
        rosinBand.prepare (spec);
        *bodyBand.coefficients  = *juce::dsp::IIR::Coefficients<float>::makeBandPass (
                                      sampleRate, kBodyFc, kBodyQ);
        *rosinBand.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass (
                                      sampleRate, juce::jmin (kRosinFc, static_cast<float> (0.4 * sampleRate)), kRosinQ);

        // Drive envelope: fast attack so each slip reads as a scrape, short
        // release so the stick phase actually goes quiet.
        envAttack  = onePole (1.0 / (2.0 * juce::MathConstants<double>::pi * kEnvAttackSec));
        envRelease = onePole (1.0 / (2.0 * juce::MathConstants<double>::pi * kEnvReleaseSec));

        // Wander: a new random target every kWanderStepSec, glided to by a
        // one-pole with the same time constant (smooth, aperiodic).
        wanderStepSamples = juce::jmax (1, juce::roundToInt (kWanderStepSec * sampleRate));
        wanderCoeff       = onePole (1.0 / (2.0 * juce::MathConstants<double>::pi * kWanderStepSec));
        grainStepSamples  = juce::jmax (1, juce::roundToInt (kGrainStepSec * sampleRate));
        grainCoeff        = onePole (1.0 / (2.0 * juce::MathConstants<double>::pi * kGrainStepSec));
        floorRise         = onePole (1.0 / (2.0 * juce::MathConstants<double>::pi * kFloorRiseSec));
        onsetDecay        = static_cast<float> (std::exp (-1.0 / (kOnsetSec * sampleRate)));

        reset();
    }

    inline void setNoiseLevel (float v) noexcept
    {
        noiseLevel = juce::jlimit (0.0f, 1.0f, v);
    }

    // Note-on: the bow's first bite is scratchier than the settled stroke
    // (irregular slipping before Helmholtz locks). Decays over kOnsetSec.
    inline void triggerOnset (float velocity) noexcept
    {
        onsetGain = kOnsetBoost * juce::jlimit (0.0f, 1.0f, velocity);
    }

    inline void setFundamentalHz (float f0) noexcept
    {
        if (f0 < 1.0f || combBuffer.empty())
        {
            combPeriod = 0;
            return;
        }
        combPeriod = juce::jlimit (2, static_cast<int> (combBuffer.size()) - 1,
                                   juce::roundToInt (sr / f0));
    }

    // relVel: |v_bow − v_string| at the bow point; force: F_bow (N). Host rate.
    inline float processSample (float relVel, float force) noexcept
    {
        // RNG draws happen EVERY sample, unconditionally and in a fixed order,
        // so a block boundary (or the level gate below) can never shift the
        // streams — block-size invariance.
        const float white = noiseRandom.nextFloat() * 2.0f - 1.0f;
        advanceWander();

        // Slip detector. The hyperbolic friction model never fully sticks: in
        // the stick phase |v_rel| sits on a creep floor that tracks force
        // (measured ≈ 0.26·F_bow), while the slip excursion ABOVE it stays
        // ≈ 0.4 m/s at any pressure. So: follow the floor (instant down, slow
        // up — slips are short against kFloorRiseSec) and gate on the excess.
        const float v = juce::jmax (0.0f, relVel);
        floor = (v < floor) ? v : floor + floorRise * (v - floor);
        const float slip = (v - floor) * kInvSlipRef;

        // Expander: d²/(d²+k²) leaves the stick phase near-silent and lets each
        // slip read as a scrape; saturates so a hard stroke doesn't scream.
        // A small kStickFloor keeps a breath of hair noise between slips.
        const float s2     = slip * slip;
        const float gate   = kStickFloor + (1.0f - kStickFloor) * s2 / (s2 + kDriveKnee * kDriveKnee);
        // Pressure sets level gently (F^¼): flautando still breathes, a dug-in
        // stroke gets grittier, but the noise no longer swings 26 dB with it
        // (F_bow·|v_rel|² under the v1.12 draft: −48 → −22 dB noise/tone over
        // BOW_PRESSURE 0.4 → 4; F^¼ on the floor-gated slip: −37 → −25).
        const float fNorm  = std::sqrt (std::sqrt (juce::jlimit (0.0f, 16.0f, force * kInvForceRef)));
        const float target = gate * fNorm;
        env += (target > env ? envAttack : envRelease) * (target - env);
        onsetGain *= onsetDecay;

        const float body  = bodyBand.processSample (white);
        const float rosin = rosinBand.processSample (white);
        const float tilt  = juce::jlimit (0.0f, 1.0f, 0.5f + colourWander);
        const float band  = (1.0f - tilt) * body + tilt * rosin;

        float pitched = band;
        if (combPeriod > 0)
        {
            const size_t n = combBuffer.size();
            const float delayed = combBuffer[(combWrite + n - static_cast<size_t> (combPeriod)) % n];
            combDampState += combDampCoeff * (delayed - combDampState);
            const float combed = band + kCombFeedback * combDampState;
            combBuffer[combWrite] = combed;
            combWrite = (combWrite + 1) % n;
            pitched = kCombMix * (combed * kCombNorm) + (1.0f - kCombMix) * band;
        }

        if (noiseLevel < 0.001f)
            return 0.0f;

        const float wanderGain = std::exp (kDbToLn * (kWanderDb * levelWander
                                                    + kGrainDb  * grainWander));
        return pitched * env * (1.0f + onsetGain) * wanderGain * noiseLevel;
    }

    inline void reset() noexcept
    {
        bodyBand.reset();
        rosinBand.reset();
        std::fill (combBuffer.begin(), combBuffer.end(), 0.0f);
        combWrite     = 0;
        combDampState = 0.0f;
        env           = 0.0f;
        floor         = 0.0f;
        onsetGain     = 0.0f;
        levelWander = levelTarget = 0.0f;
        colourWander = colourTarget = 0.0f;
        grainWander = grainTarget = 0.0f;
        wanderCounter = 0;
        grainCounter  = 0;
    }

private:
    inline float onePole (double cornerHz) const noexcept
    {
        return 1.0f - static_cast<float> (std::exp (-juce::MathConstants<double>::twoPi
                                                    * cornerHz / sr));
    }

    inline void advanceWander() noexcept
    {
        if (--wanderCounter <= 0)
        {
            wanderCounter = wanderStepSamples;
            levelTarget   = 2.0f * wanderRandom.nextFloat() - 1.0f;
            colourTarget  = kColourDepth * (2.0f * wanderRandom.nextFloat() - 1.0f);
        }
        if (--grainCounter <= 0)
        {
            grainCounter = grainStepSamples;
            grainTarget  = 2.0f * wanderRandom.nextFloat() - 1.0f;
        }
        levelWander  += wanderCoeff * (levelTarget  - levelWander);
        colourWander += wanderCoeff * (colourTarget - colourWander);
        grainWander  += grainCoeff  * (grainTarget  - grainWander);
    }

    // Spectrum: a body band (the hair's dull "shh", which the body resonator
    // also colours) and a rosin band (the bright grit), crossfaded by wander.
    static constexpr float kBodyFc  = 1100.0f, kBodyQ  = 0.8f;
    static constexpr float kRosinFc = 4200.0f, kRosinQ = 0.9f;

    // Slip detector, expander and envelope (per slip).
    static constexpr double kFloorRiseSec  = 0.030;
    static constexpr float  kInvSlipRef    = 1.0f / 0.4f;   // measured slip excursion
    static constexpr float  kInvForceRef   = 1.0f / 0.5f;   // F_bow at default pressure
    static constexpr float  kDriveKnee     = 0.50f;
    static constexpr float  kStickFloor    = 0.06f;
    // Onset bite: +kOnsetBoost (linear, ≈ +6 dB at full velocity), τ kOnsetSec.
    static constexpr float  kOnsetBoost    = 1.0f;
    static constexpr double kOnsetSec      = 0.15;
    static constexpr double kEnvAttackSec  = 0.0003;
    static constexpr double kEnvReleaseSec = 0.0025;

    // Slow stroke-scale wander (level + colour) and faster hair "grain".
    static constexpr double kWanderStepSec = 0.35;
    static constexpr double kGrainStepSec  = 0.045;
    static constexpr float  kWanderDb      = 5.0f;   // ± level wander
    static constexpr float  kGrainDb       = 1.5f;   // ± grain wander
    static constexpr float  kColourDepth   = 0.35f;  // ± tilt around 0.5
    static constexpr float  kDbToLn        = 0.11512925f;   // ln(10)/20

    // Pitch-synchronous comb, lighter than v1.5–v1.11 (0.85 fb / 0.70 mix rang
    // as a buzz); the slip-gated drive now carries most of the periodicity.
    static constexpr float kCombMinF0    = 20.0f;
    static constexpr float kCombFeedback = 0.60f;
    static constexpr float kCombNorm     = 0.80f;    // ≈ sqrt(1 − 0.60²)
    static constexpr float kCombMix      = 0.40f;

    juce::Random noiseRandom;
    juce::Random wanderRandom;   // wander targets only — separate stream
    juce::dsp::IIR::Filter<float> bodyBand, rosinBand;

    std::vector<float> combBuffer;
    size_t combWrite     = 0;
    int    combPeriod    = 0;
    float  combDampState = 0.0f;
    float  combDampCoeff = 0.4f;

    float env = 0.0f, envAttack = 0.5f, envRelease = 0.05f;
    float floor = 0.0f, floorRise = 0.001f;
    float onsetGain = 0.0f, onsetDecay = 0.9999f;

    int   wanderStepSamples = 1, wanderCounter = 0;
    int   grainStepSamples  = 1, grainCounter  = 0;
    float wanderCoeff = 0.0f, grainCoeff = 0.0f;
    float levelWander = 0.0f, levelTarget = 0.0f;
    float colourWander = 0.0f, colourTarget = 0.0f;
    float grainWander = 0.0f, grainTarget = 0.0f;

    float  noiseLevel = 0.0f;
    double sr         = 44100.0;
};
