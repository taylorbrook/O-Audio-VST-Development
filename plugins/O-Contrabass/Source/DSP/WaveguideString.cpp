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

    WaveguideString.cpp
    O-Contrabass - Split-Rail Digital Waveguide for Bass-Range Bowed String
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1a-recovery rev-3 — applies F1 (split-rail), F2 (bridge LP
    recurrence fix; DC gain = g exactly), F3 (no in-loop DC blocker) as
    a single coupled change. See PLAN rev-3 + RESEARCH §11.

  ==============================================================================
*/

#include "WaveguideString.h"
#include "HyperbolicFriction.h"
#include <cmath>

void WaveguideString::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;

    // Internal-rate ProcessSpec (caller passes sr already at 2× oversampled rate
    // and a doubled maxBlockSize, per BowedContrabassVoice::prepareToPlay).
    juce::dsp::ProcessSpec spec {
        sr,
        static_cast<juce::uint32> (maxBlockSize),
        1
    };

    bridgeDelay.prepare (spec);
    bridgeDelay.setMaximumDelayInSamples (8192);
    neckDelay.prepare (spec);
    neckDelay.setMaximumDelayInSamples (8192);

    // Phase 2.1c — bridge-rail dispersion (M=4 hardcoded for E1; CONTEXT rev-3 Q2).
    // Identity coefficient (a=0) until voice pushes a non-zero `a` per block.
    bridgeDispersion.prepare (sr);
    bridgeDispersion.setActiveSections (4);
    bridgeDispersion.setCoefficient (0.0f);

    // 20 ms linear ramp on STRING_STIFFNESS (RESEARCH §5 pitfall #6).
    // WR-09: this smoother is advanced by HOST-rate numSamples (the voice calls
    // advanceStiffnessSmootherBy(numSamples) with the host block size), but `sr`
    // here is the 2× internal rate. Resetting at `sr` made the 20 ms ramp take
    // 40 ms of host time. Reset at the host rate (sr·0.5) so it completes in 20 ms.
    stiffnessSmoothed.reset (sr * 0.5, 0.020);
    stiffnessSmoothed.setCurrentAndTargetValue (cachedStringStiffness);

    filterDirty = true;
    reset();
}

void WaveguideString::trigger (float frequency)
{
    currentFrequency = frequency;

    // Clean start: clear delay lines + filter state to avoid artifacts from previous note.
    reset();

    updateDelayLengths();
    updateBridgeFilterCoeffs();
}

void WaveguideString::reset()
{
    bridgeDelay.reset();
    neckDelay.reset();
    bridgeDispersion.reset();
    bridgeY = 0.0f;
    energyEstimate = 0.0f;
    cancelRelease();
}

//==============================================================================
// v1.3 release damping (see WaveguideString.h for the measured rationale)
//==============================================================================

void WaveguideString::cancelRelease() noexcept
{
    // Exact 1.0f — processSample multiplies bridgeG by this every sample, so any
    // value other than exactly 1.0f would perturb the sustain path.
    releaseGain       = 1.0f;
    releaseGainTarget = 1.0f;
    releaseGainStep   = 0.0f;
}

void WaveguideString::startRelease (float t60Seconds) noexcept
{
    // Guard the whole domain: a non-finite or non-positive T60 would produce a
    // negative or NaN exponent below and poison the loop for the voice's life.
    if (! std::isfinite (t60Seconds) || t60Seconds <= 0.0f)
        t60Seconds = 0.05f;

    const float f0 = std::max (1.0f, currentFrequency);

    // Round trips per second == f0, so -60 dB over t60Seconds needs
    //   20·log10(g_eff) · f0 · t60 = -60  =>  g_eff = 10^(-3 / (f0 · t60)).
    // Deriving from f0 is what makes the release time pitch-INDEPENDENT: a fixed
    // per-round-trip gain would make an E1 release last 2.4x a G2 release.
    const float gEff = std::pow (10.0f, -3.0f / (f0 * t60Seconds));

    // bridgeG is the sustain-mode loop gain (>= 0.997), so the multiplier that
    // lands the effective gain on gEff is gEff/bridgeG. Never allowed above 1.0 —
    // a release must not ADD energy, which it otherwise would whenever the
    // requested T60 is longer than the free-decay time INFINITE_SUSTAIN implies.
    const float safeBridgeG = std::max (1.0e-6f, bridgeG);
    releaseGainTarget = std::min (1.0f, gEff / safeBridgeG);

    // Ramp rather than step, so the bow-lift reads as a gesture. Positive step;
    // processSample subtracts it. Guard the ramp length against a zero/absurd
    // sample rate so the step stays finite.
    constexpr float kReleaseRampMs = 30.0f;
    const float rampSamples = std::max (1.0f,
                                        static_cast<float> (sampleRate) * kReleaseRampMs * 0.001f);
    releaseGainStep = std::max (0.0f, (releaseGain - releaseGainTarget) / rampSamples);
}

void WaveguideString::updateDelayLengths()
{
    // Split-rail topology — total round-trip = sr / f0 minus bridge LP group delay,
    // partitioned at the bow contact point β.
    float totalDelay = static_cast<float> (sampleRate) / std::max (1.0f, currentFrequency);

    float filterGroupDelay = bridgeGroupDelaySamples();   // WR-07 (bounded to pole clamp)
    float compensated = totalDelay - filterGroupDelay;

    float bridgeSamples = compensated * bowPosition;
    float neckSamples   = compensated * (1.0f - bowPosition);

    // Phase 2.1c: dispersion lives on the bridge rail only → compensate bridge-rail
    // delay-line length only, NOT `compensated`. Identity at a=0 (groupDelay returns
    // M=4 samples; cascade adds M unit delays; net round-trip preserved → bit-exact
    // regression at stiffness=0 holds). RESEARCH §14.7 option (i).
    const float dispersionDelay = bridgeDispersion.getGroupDelaySamples (currentFrequency);
    bridgeSamples -= dispersionDelay;

    // Lagrange3rd needs ≥4 samples per rail (4-tap kernel).
    bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
    neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);

    lastBridgeSamples = bridgeSamples;
    lastNeckSamples   = neckSamples;

    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay   (neckSamples);
}

void WaveguideString::setDelaySamples (float totalSamples)
{
    // Per-sample total-delay setter (vibrato/detune ramps in 2.2/2.3). Splits
    // the target round-trip period across the two rails using the current
    // bowPosition, and applies the same LP + dispersion group-delay
    // compensation as updateDelayLengths() so that calling
    // setDelaySamples(sr/f) is bit-exactly equivalent to trigger(f) for the
    // slot-0 regression preset (HARD RULE §15.9.5).
    const float filterGroupDelay = bridgeGroupDelaySamples();   // WR-07 (bounded to pole clamp)
    const float dispersionDelay  = bridgeDispersion.getGroupDelaySamples (currentFrequency);
    const float compensated      = totalSamples - filterGroupDelay;

    float bridgeSamples = compensated * bowPosition - dispersionDelay;
    float neckSamples   = compensated * (1.0f - bowPosition);
    bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
    neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);
    lastBridgeSamples = bridgeSamples;
    lastNeckSamples   = neckSamples;
    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay   (neckSamples);
}

void WaveguideString::seedFundamental (float amplitude) noexcept
{
    if (! (amplitude > 0.0f) || ! std::isfinite (amplitude))
        return;

    const int nB = juce::jlimit (1, 8190, static_cast<int> (std::lround (lastBridgeSamples)));
    const int nN = juce::jlimit (1, 8190, static_cast<int> (std::lround (lastNeckSamples)));
    const int L  = nB + nN;                       // full round trip in samples

    // One period of the fundamental laid across the round trip, split across the
    // two rails at the bow contact point. Both rails carry velocity waves and the
    // junction sums their reflections, so a continuous phase ramp over L keeps the
    // seed coherent across the split. Endpoints are zero-valued, so no click.
    const float k = juce::MathConstants<float>::twoPi / static_cast<float> (L);

    // CRITICAL: push AND pop each sample, exactly as processSample() does.
    // juce::dsp::DelayLine keeps independent read/write pointers; pushSample()
    // advances only the write pointer. Filling a rail with N bare pushSample()
    // calls therefore shifts the write pointer N samples relative to the read
    // pointer, ADDING N samples to the effective delay — which detuned the string
    // by 2-3 semitones and broke every pitch-dependent gate (vibrato read 306
    // cents and 20.4 Hz; microtonal pitchAccuracy and note-expression tracking
    // all failed). Popping in lockstep keeps the configured delay intact.
    for (int i = 0; i < nB; ++i)
    {
        bridgeDelay.pushSample (0, amplitude * std::sin (k * static_cast<float> (i)));
        juce::ignoreUnused (bridgeDelay.popSample (0));
    }

    for (int i = 0; i < nN; ++i)
    {
        neckDelay.pushSample (0, amplitude * std::sin (k * static_cast<float> (nB + i)));
        juce::ignoreUnused (neckDelay.popSample (0));
    }
}

float WaveguideString::computeLoopGain (float infSustainParam01) noexcept
{
    // Quadratic skew per RESEARCH §Q5 — most perceptual range concentrated
    // in upper half of knob; hard-clamped to architectural ceiling.
    constexpr float kFloor   = 0.997f;
    constexpr float kSpan    = 0.00295f;        // 0.99995 − 0.997 at param=1
    constexpr float kCeiling = 0.9999999f;
    const float g = kFloor + kSpan * infSustainParam01 * infSustainParam01;
    return std::min (g, kCeiling);
}

float WaveguideString::bridgeGroupDelaySamples() const noexcept
{
    // WR-07: The bridge-LP pole p is clamped to ≤ 0.95 (updateBridgeFilterCoeffs),
    // so its true group delay saturates at ~19 samples once brightnessHz falls below
    // the pole-clamp frequency (~720 Hz @ 88.2 kHz). The analog approximation
    // sr/(2π·f) keeps growing below that → subtracts MORE delay than the filter adds
    // → loop too short → pitch drifts sharp (~1.3 semitones at BRIGHTNESS=80 Hz on E1).
    // Floor the brightness fed to the compensation at the pole-clamp frequency so the
    // compensation never exceeds what the (clamped) filter realizes. No-op for
    // brightnessHz ≥ ~720 Hz (incl. the 4500 Hz default) ⇒ golden-neutral.
    constexpr float kBridgePoleMax = 0.95f;   // matches the clamp in updateBridgeFilterCoeffs
    const float poleClampHz = -std::log (kBridgePoleMax) * static_cast<float> (sampleRate)
                            / juce::MathConstants<float>::twoPi;
    const float fForGroupDelay = std::max (poleClampHz, brightnessHz);
    return static_cast<float> (sampleRate)
         / (juce::MathConstants<float>::twoPi * fForGroupDelay);
}

void WaveguideString::updateBridgeFilterCoeffs()
{
    // g — loop gain, quadratic skew + ceiling clamp.
    bridgeG = computeLoopGain (infiniteSustain);

    // p — HF damping pole, clamped [0.05, 0.95] to ensure non-zero damping
    // in all states (ARCHITECTURE §"Bridge Filter").
    float pi = juce::MathConstants<float>::pi;
    float p = std::exp (-2.0f * pi * brightnessHz / static_cast<float> (sampleRate));
    bridgeP = juce::jlimit (0.05f, 0.95f, p);
    bridgeOneMinusP = 1.0f - bridgeP;

    // Drone-mode boundary uses the user-knob value (RESEARCH §Q5):
    // outside drone (INFINITE_SUSTAIN < 0.95), inject a constant −1e-20 into y
    // to keep the recursion above the subnormal range. In drone mode the leak
    // would slowly bleed energy away from a true sustain, so we disable it.
    denormalLeak = (infiniteSustain >= 0.95f) ? 0.0f : -1.0e-20f;

    filterDirty = false;
}

float WaveguideString::processSample (float v_bow, float F_bow,
                                       const HyperbolicFriction& friction)
{
    // Apply pending coefficient update (audio-thread only path).
    if (filterDirty)
        updateBridgeFilterCoeffs();

    // Step 1: Read both rails.
    float bridgeRaw = bridgeDelay.popSample (0);
    float neckRaw   = neckDelay.popSample   (0);

    // Step 1.5 [Phase 2.1c]: bridge-rail dispersion (cascaded first-order allpass).
    // Identity at a=0; per ARCHITECTURE.md §"Cascaded Allpass Dispersion".
    const float bridgeDispersed = bridgeDispersion.processSample (bridgeRaw);

    // Step 2: Bridge LP — F2 fixed form: y = g·(1−p)·x + p·y_prev + leak.
    // (DROP the `g` from the feedback term so DC gain == g exactly. See
    //  RESEARCH §11.1 B2; mirrors O-Bowed WaveguideString.cpp:94-95
    //  coefficient form (b0, b1, a0, a1) = (g·(1−p), 0, 1, −p).)
    if (! std::isfinite (bridgeY))
        bridgeY = 0.0f;

    // v1.3 release ramp. While not releasing releaseGain is EXACTLY 1.0f and
    // releaseGainStep is 0.0f, so the multiply below is an exact identity and the
    // sustain path stays bit-identical to v1.2 (verified against the 19 goldens'
    // sustain phases). Advanced here rather than per block so the ramp is
    // block-size invariant.
    if (releaseGainStep > 0.0f)
    {
        releaseGain -= releaseGainStep;
        if (releaseGain <= releaseGainTarget)
        {
            releaseGain     = releaseGainTarget;
            releaseGainStep = 0.0f;
        }
    }

    float bridgeFiltered = (bridgeG * releaseGain) * bridgeOneMinusP * bridgeDispersed
                         + bridgeP * bridgeY
                         + denormalLeak;
    bridgeY = bridgeFiltered;

    // Step 3: Boundary reflections.
    //   Bridge: −1 boundary AFTER LP (rigid bridge; matches O-Bowed line 108).
    //   Nut:    −1 boundary, no LP (rigid nut).
    float bridgeReflection = -bridgeFiltered;
    float nutReflection    = -neckRaw;

    // Step 4: Sum at bow point (split-rail v_string_incoming).
    float v_string_incoming = bridgeReflection + nutReflection;
    float v_delta = v_bow - v_string_incoming;

    // Step 5: Friction (unchanged from rev-1 single-rail).
    float rho = friction.computeReflectionCoefficient (v_delta, F_bow);
    float clampedRho = std::min (rho, 0.85f);
    float frictionVelocity = 2.0f * clampedRho / (1.0f - clampedRho);
    float absVd = std::abs (v_delta);
    float injection = std::min (frictionVelocity, absVd);
    float newVelocity = (v_delta >= 0.0f) ? injection : -injection;

    // Step 6: Symmetric injection into both rails (canonical Smith two-port).
    //  (Dispersion already ran at Step 1.5, between bridgeRaw popSample and bridge LP —
    //   bridge rail only, per ARCHITECTURE.md §"Cascaded Allpass Dispersion" and
    //   §"Processing Order"; mirrors O-Bowed bridge-rail-only loop chain.)
    float toBridge = nutReflection    + newVelocity;
    float toNeck   = bridgeReflection + newVelocity;

    // Step 7: In-loop tanh saturator on each rail (Phase 2.4c-bis R36-bis port from
    //   O-Bowed WaveguideString.cpp:218-219 writeJunction; closes Phase 2.4c §19.7.6
    //   escalation flag locked at 5.92 dB envelope divergence; see RESEARCH §20.4).
    constexpr float sat = 4.0f;
    toBridge = sat * std::tanh (toBridge / sat);
    toNeck   = sat * std::tanh (toNeck   / sat);

    // Step 8: F3 — NO in-loop DC blocker. (If long-form drone surfaces DC
    //  drift in Phase 2.4/2.5, an output-path DCB at
    //  BowedContrabassVoice::renderNextBlock post-processSamplesDown is
    //  the correct placement; it does not interfere with bootstrapping.)

    bridgeDelay.pushSample (0, toBridge);
    neckDelay.pushSample   (0, toNeck);

    // Step 9: Output from bridge end (matches O-Bowed `output = toBridge`).
    float output = toBridge;
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);
    return output;
}

void WaveguideString::setBowPosition (float beta)
{
    // Split-rail β: clamp [0.02, 0.98] (no-bow at ends would zero a delay rail).
    float clamped = juce::jlimit (0.02f, 0.98f, beta);
    if (std::abs (bowPosition - clamped) < 1e-6f)
        return;
    bowPosition = clamped;
    updateDelayLengths();
}

void WaveguideString::setBrightness (float cutoffHz)
{
    if (std::abs (brightnessHz - cutoffHz) < 0.5f)
        return;

    brightnessHz = cutoffHz;
    filterDirty = true;
    // Brightness affects bridge filter group delay → recompensate base delay.
    updateDelayLengths();
}

void WaveguideString::setInfiniteSustain (float amount)
{
    if (std::abs (infiniteSustain - amount) < 1e-6f)
        return;

    infiniteSustain = amount;
    filterDirty = true;
}

void WaveguideString::setStringStiffness (float amount)
{
    // Drives the 20 ms smoother. Phase 2.1c voice path: the per-block update
    // in BowedContrabassVoice::renderNextBlock advances the smoother via
    // advanceStiffnessSmootherBy(numSamples), reads the current smoothed
    // value via getCurrentSmoothedStiffness(), computes `a` via
    // DispersionFilter::computeAllpassCoefficient(f0, B, M), then pushes via
    // setDispersionCoefficient(a).
    cachedStringStiffness = juce::jlimit (0.0f, 1.0f, amount);
    stiffnessSmoothed.setTargetValue (cachedStringStiffness);
}

void WaveguideString::setDispersionCoefficient (float a) noexcept
{
    // Defensive: voice should already pass a finite, clamped value via
    // DispersionFilter::computeAllpassCoefficient; belt-and-braces guard
    // mirrors RESEARCH §14.9 against pathological inputs (NaN/Inf).
    const float safe = std::isfinite (a) ? a : 0.0f;
    bridgeDispersion.setCoefficient (safe);
}

void WaveguideString::advanceStiffnessSmootherBy (int numSamples) noexcept
{
    stiffnessSmoothed.skip (juce::jmax (0, numSamples));
}

void WaveguideString::setDispersionActiveSections (int M) noexcept
{
    bridgeDispersion.setActiveSections (M);
}

float WaveguideString::getCurrentSmoothedStiffness() const noexcept
{
    return stiffnessSmoothed.getCurrentValue();
}

bool WaveguideString::isActive() const noexcept
{
    return energyEstimate > 1e-7f;
}
