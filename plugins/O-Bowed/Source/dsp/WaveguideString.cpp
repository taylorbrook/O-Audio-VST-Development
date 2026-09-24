/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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
    O-Bowed - Bidirectional Digital Waveguide for Bowed String
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "WaveguideString.h"
#include "HyperbolicFriction.h"
#include <array>

void WaveguideString::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;

    // Lowest note ~20Hz + headroom
    int maxDelay = static_cast<int> (sampleRate / 20.0) + 100;

    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (maxBlockSize),
        1
    };

    bridgeDelay.prepare (spec);
    bridgeDelay.setMaximumDelayInSamples (maxDelay);

    neckDelay.prepare (spec);
    neckDelay.setMaximumDelayInSamples (maxDelay);

    bridgeLossFilter.prepare (spec);

    // Seed the coefficient storage once, off the audio thread. This allocates the
    // internal coefficient array (capacity >= 8) so every subsequent in-place update
    // on the audio thread reuses it without reallocating. CR-01.
    *bridgeLossFilter.coefficients = std::array<float, 4> { 1.0f, 0.0f, 1.0f, 0.0f };
    bridgeLossFilter.reset();  // size state to the order-1 layout off-thread

    filterDirty = true;
}

void WaveguideString::trigger (float frequency)
{
    currentFrequency = frequency;

    // Clean start: clear delay lines to avoid artifacts from previous note
    reset();

    updateDelayLengths();
    updateBridgeFilterCoeffs();
}

void WaveguideString::reset()
{
    bridgeDelay.reset();
    neckDelay.reset();
    bridgeLossFilter.reset();
    energyEstimate = 0.0f;
}

void WaveguideString::setFrequency (float frequency)
{
    // CR-02: retune without reset(). trigger() clears both rails and the energy
    // estimate, so calling it on every pitch-bend message restarted the string
    // from silence. A held note keeps its travelling waves and only moves the
    // rail lengths.
    if (std::abs (currentFrequency - frequency) < 1e-4f)
        return;

    currentFrequency = frequency;
    updateDelayLengths();
}

void WaveguideString::updateDelayLengths()
{
    float totalDelay = static_cast<float> (sampleRate) / currentFrequency;

    // WR-05: compensate the bridge loss filter with its PHASE delay at f0, not the
    // DC-ish sr/(2*pi*fc) estimate, which over-compensated (pitch sharp) whenever
    // the corner sat near or below f0 (A4 at Brightness 300 Hz was +161 c).
    // One-pole H = b0 / (1 - p z^-1): phase delay = atan2(p sin w, 1 - p cos w) / w.
    float pi = juce::MathConstants<float>::pi;
    float p = std::exp (-2.0f * pi * brightnessHz / static_cast<float> (sampleRate));
    float w = 2.0f * pi * currentFrequency / static_cast<float> (sampleRate);
    float filterPhaseDelay = std::atan2 (p * std::sin (w), 1.0f - p * std::cos (w)) / w;
    float compensatedDelay = totalDelay - filterPhaseDelay;

    // Clamp to ensure positive total delay
    compensatedDelay = std::max (4.0f, compensatedDelay);

    // Split at bow position
    float bridgeSamples = compensatedDelay * bowPosition;
    float neckSamples = compensatedDelay * (1.0f - bowPosition);

    // No per-rail sample correction for readJunction popping before
    // writeJunction pushes: JUCE's DelayLine read/write pointers advance in
    // lockstep, so pop-then-push is exactly setDelay samples (it's a BARE push
    // without a pop that shifts the delay). Measured: subtracting 1 per rail
    // put A4 +17.6 c sharp. The review's CR-01/WR-05 "+1 per rail" is wrong.

    // Clamp minimum delay per rail (Thiran needs >= 2 samples)
    bridgeSamples = std::max (2.0f, bridgeSamples);
    neckSamples = std::max (2.0f, neckSamples);

    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay (neckSamples);
}

void WaveguideString::updateBridgeFilterCoeffs()
{
    // Compute loop gain from INFINITE_SUSTAIN parameter
    // Cap at 0.9995 to prevent true infinite oscillation (still ~15s decay at 440Hz)
    float g = 0.990f + 0.0095f * infiniteSustain;

    // Compute pole from brightness
    float pi = juce::MathConstants<float>::pi;
    float p = std::exp (-2.0f * pi * brightnessHz / static_cast<float> (sampleRate));

    // Custom one-pole: H(z) = g*(1-p) / (1 - p*z^-1)
    // IIR coefficients: b0 = g*(1-p), b1 = 0, a0 = 1, a1 = -p
    *bridgeLossFilter.coefficients = juce::dsp::IIR::Coefficients<float> (
        g * (1.0f - p), 0.0f, 1.0f, -p);

    filterDirty = false;
}

float WaveguideString::processSample (float v_bow, float F_bow,
                                       const HyperbolicFriction& friction)
{
    // Apply pending filter coefficient update (thread-safe: runs on audio thread only)
    if (filterDirty)
        updateBridgeFilterCoeffs();

    // Step 2: Read incoming waves from delay line ends
    float bridgeReflection = -bridgeLossFilter.processSample (bridgeDelay.popSample (0));
    float nutReflection = -neckDelay.popSample (0);   // sign inversion = hard boundary

    // Step 3: Combine traveling waves at bow point
    float v_string_incoming = bridgeReflection + nutReflection;

    // Step 4: Compute differential velocity
    float v_delta = v_bow - v_string_incoming;

    // Step 5: Evaluate friction -> reflection coefficient
    float rho = friction.computeReflectionCoefficient (v_delta, F_bow);

    // Step 6: Compute velocity injection (stick-slip model)
    // Reconstruct friction-limited velocity from rho: rho = r/(1+r) → r = rho/(1-rho)
    // frictionVelocity = 2r = mu*F_bow/(2*R_s), the max velocity the bow can impose.
    // Sticking (|v_delta| < frictionVel): injection = v_delta (string follows bow)
    // Slipping (|v_delta| ≥ frictionVel): injection = frictionVel (capped — negative slope)
    float clampedRho = std::min (rho, 0.99f);
    float frictionVelocity = 2.0f * clampedRho / (1.0f - clampedRho);
    float absVd = std::abs (v_delta);
    float injection = std::min (frictionVelocity, absVd);
    float newVelocity = (v_delta >= 0.0f) ? injection : -injection;

    // Step 7: Write outgoing waves into delay lines (symmetric injection)
    float toBridge = nutReflection + newVelocity;
    float toNeck = bridgeReflection + newVelocity;

    // Soft saturation prevents numerical blowup without generating DC
    // (tanh is odd-symmetric, unlike hard clipping which creates DC offset)
    constexpr float sat = 4.0f;
    toBridge = sat * std::tanh (toBridge / sat);
    toNeck = sat * std::tanh (toNeck / sat);

    bridgeDelay.pushSample (0, toBridge);
    neckDelay.pushSample (0, toNeck);

    // Step 8: Output from bridge end
    float output = toBridge;

    // Energy tracking for voice cleanup
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);

    // Denormal flush
    if (std::abs (output) < 1e-15f)
        output = 0.0f;

    return output;
}

void WaveguideString::setBowPosition (float beta)
{
    if (std::abs (bowPosition - beta) < 1e-6f)
        return;

    bowPosition = beta;
    updateDelayLengths();
}

void WaveguideString::setBrightness (float cutoffHz)
{
    if (std::abs (brightnessHz - cutoffHz) < 0.5f)
        return;

    brightnessHz = cutoffHz;
    filterDirty = true;
    // Brightness affects filter group delay, so update delay lengths too
    updateDelayLengths();
}

void WaveguideString::setInfiniteSustain (float amount)
{
    if (std::abs (infiniteSustain - amount) < 1e-6f)
        return;

    infiniteSustain = amount;
    filterDirty = true;
}

WaveguideString::JunctionState WaveguideString::readJunction (float /*v_bow*/)
{
    // Apply pending filter coefficient update (thread-safe: runs on audio thread only)
    if (filterDirty)
        updateBridgeFilterCoeffs();

    JunctionState state;

    // Step 2: Read incoming waves from delay line ends
    state.bridgeReflection = -bridgeLossFilter.processSample (bridgeDelay.popSample (0));
    state.nutReflection = -neckDelay.popSample (0);   // sign inversion = hard boundary

    // Step 3: Combine traveling waves at bow point
    state.v_string_incoming = state.bridgeReflection + state.nutReflection;

    return state;
}

float WaveguideString::writeJunction (float rho, float v_delta, const JunctionState& state)
{
    // Step 6: Compute velocity injection (stick-slip model)
    float clampedRho = std::min (rho, 0.99f);
    float frictionVelocity = 2.0f * clampedRho / (1.0f - clampedRho);
    float absVd = std::abs (v_delta);
    float injection = std::min (frictionVelocity, absVd);
    float newVelocity = (v_delta >= 0.0f) ? injection : -injection;

    // Step 7: Write outgoing waves into delay lines (symmetric injection)
    float toBridge = state.nutReflection + newVelocity;
    float toNeck = state.bridgeReflection + newVelocity;

    constexpr float sat = 4.0f;
    toBridge = sat * std::tanh (toBridge / sat);
    toNeck = sat * std::tanh (toNeck / sat);

    bridgeDelay.pushSample (0, toBridge);
    neckDelay.pushSample (0, toNeck);

    // Step 8: Output from bridge end
    float output = toBridge;

    // Energy tracking for voice cleanup
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);

    // Denormal flush
    if (std::abs (output) < 1e-15f)
        output = 0.0f;

    return output;
}

bool WaveguideString::isActive() const noexcept
{
    return energyEstimate > 1e-7f;
}
