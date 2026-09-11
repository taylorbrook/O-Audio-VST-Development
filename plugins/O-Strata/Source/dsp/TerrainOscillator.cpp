/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    TerrainOscillator.cpp
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "TerrainOscillator.h"
#include "MathConstants.h"

// ═══════════════════════════════════════════════════════════════════
// Inherited interface (the wavetable oscillator at efae0bfc, read call swapped;
// tests/render-harness/reference/theta_reference.h keeps the verbatim copy)
// ═══════════════════════════════════════════════════════════════════

void TerrainOscillator::setWarpType (WarpType type)
{
    if (warpType != type)
    {
        warpType = type;
        // Reset master phases when switching warp modes
        for (int i = 0; i < kMaxUnison; ++i)
            masterPhases[i] = phaseAccumulators[i];
    }
}

void TerrainOscillator::setWarpAmount (float amount)
{
    warpAmount = amount;
}

void TerrainOscillator::setFMInput (double value)
{
    fmInput = value;
}

void TerrainOscillator::prepare (double sampleRate)
{
    // prepare-time only (not RT): the shared Superellipse LUT is built here under call_once.
    currentSampleRate = sampleRate;
    superLUT = SuperellipseLUT::get();
    dcR = 1.0 - kTwoPi * 5.0 / sampleRate;
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
    lastNormalisedOrbitMod = -10.0f;
    lastRotation = 1.0e9f;
    setFrequency (frequency);
}

void TerrainOscillator::setFrequency (double freq)
{
    // Clamp at Nyquist: phaseIncrement must stay well below 1.0 or the
    // accumulators outrun the per-sample wrap.
    frequency = juce::jlimit (0.0, 0.5 * currentSampleRate, freq);
    phaseIncrement = frequency / currentSampleRate;
}

void TerrainOscillator::setPosition (float pos)
{
    position = pos;
}

void TerrainOscillator::setQuality (Quality q)
{
    quality = q;   // Round A Phase 2.1: every path runs at 1×; Phase 2.3 adds the OS loop + crossfade
}

void TerrainOscillator::reset()
{
    for (int i = 0; i < kMaxUnison; ++i)
    {
        phaseAccumulators[i] = 0.0;
        masterPhases[i] = 0.0;
        fbD[i] = fbY1[i] = fbY2[i] = 0.0f;
    }
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
}

void TerrainOscillator::resetWithPhase (double phase)
{
    for (int i = 0; i < kMaxUnison; ++i)
    {
        phaseAccumulators[i] = phase;
        masterPhases[i] = phase;
        fbD[i] = fbY1[i] = fbY2[i] = 0.0f;
    }
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
}

void TerrainOscillator::resetWithRandomPhases (uint32_t seed)
{
    // seed == 0: the inherited deterministic-per-instance LCG seeded from the
    // oscillator address (production behaviour, unchanged). Otherwise the LCG
    // runs from the caller's seed (harness determinism, plan Decision 8).
    if (seed == 0)
        seed = static_cast<uint32_t> (reinterpret_cast<uintptr_t> (this) ^ 0x12345678u);
    for (int i = 0; i < kMaxUnison; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        phaseAccumulators[i] = static_cast<double> (seed) / 4294967296.0;
        masterPhases[i] = phaseAccumulators[i];
        fbD[i] = fbY1[i] = fbY2[i] = 0.0f;
    }
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
}

void TerrainOscillator::setUnison (int count, float detune, float width)
{
    const int newCount = juce::jlimit (1, kMaxUnison, count);

    // Cached on (count, detune, width) change — setUnison is called every block
    // from StrataVoice::renderNextBlock (plan Decision 10).
    if (newCount == unisonCount && detune == cachedDetune && width == cachedWidth)
        return;

    unisonCount = newCount;
    cachedDetune = detune;
    cachedWidth = width;

    if (unisonCount == 1)
    {
        unisonDetuneFactors[0] = 1.0;
        unisonPanL[0] = 1.0;
        unisonPanR[0] = 1.0;
        unisonGain = 1.0;
        return;
    }

    unisonGain = 1.0 / std::sqrt (static_cast<double> (unisonCount));
    double centerIndex = (unisonCount - 1) / 2.0;
    double normFactor = centerIndex;

    for (int i = 0; i < unisonCount; ++i)
    {
        double normalizedPos = (i - centerIndex) / normFactor;

        // Detune: max 50 cents spread — block-rate (DSP-05 exemption: setUnison cache, plan Decision 10)
        unisonDetuneFactors[i] = std::pow (2.0, normalizedPos * detune * 50.0 / 1200.0);

        // Pan: equal-power pan law
        double panNorm = (normalizedPos * width + 1.0) * 0.5; // Map to [0,1]
        panNorm = juce::jlimit (0.0, 1.0, panNorm);
        unisonPanL[i] = std::cos (panNorm * kHalfPi);
        unisonPanR[i] = std::sin (panNorm * kHalfPi);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Block-rate feed
// ═══════════════════════════════════════════════════════════════════

void TerrainOscillator::updateBlockRate (double /*noteHz*/)
{
    // Squarcle 1 / tanh k (block-rate; std::tanh allowed here only)
    if (orbitKind == OrbitKind::Squarcle)
    {
        const float k = 0.3f + 6.0f * orbitMod;
        scratch.invTanhK = 1.0f / std::tanh (k);   // block-rate (DSP-05 exemption: updateBlockRate)
    }
    else
    {
        scratch.invTanhK = 1.0f;
    }

    // Orbit re-normalisation to max radius 1 (plan Decision 15): 64-point scan
    // when the orbit kind or Orbit Mod moved by > 1e-3 since the last scan.
    if (orbitKind != lastNormalisedOrbit || std::abs (orbitMod - lastNormalisedOrbitMod) > 1.0e-3f)
    {
        scratch.normFactor = 1.0f;
        scratch.normFactor = 1.0f / OrbitDetail::maxRadius (orbitKind, orbitMod, scratch, superLUT);
        lastNormalisedOrbit = orbitKind;
        lastNormalisedOrbitMod = orbitMod;
    }

    // Phase 2.2 adds rTrack (pitch tracking) and aEff (feedback damp) here.
}

// ═══════════════════════════════════════════════════════════════════
// Per-sample
// ═══════════════════════════════════════════════════════════════════

double TerrainOscillator::applyWarp (double phase) const noexcept
{
    switch (warpType)
    {
        case WarpType::Bend:
        {
            // Phase distortion: pow(phase, exponent) where exponent 1..4 —
            // inherited per-sample pow from the O-Prism applyWarp (Bend warp only;
            // not in the terrain budget, unchanged since O-Prism).
            double exponent = 1.0 + static_cast<double> (warpAmount) * 3.0;
            return std::pow (phase, exponent);
        }
        case WarpType::FM:
        {
            // Phase modulation: add other osc output to phase
            double warped = phase + fmInput * static_cast<double> (warpAmount);
            warped = warped - std::floor (warped); // wrap to [0, 1)
            return warped;
        }
        default:
            return phase;
    }
}

float TerrainOscillator::scan (double phase, int partial) noexcept
{
    if (kernelBypass)
        return 0.0f;   // H7 baseline: everything but the terrain kernel runs

    // Defensive wrap: warp/sync paths can hand us phase outside [0, 1).
    if (! std::isfinite (phase))
        return 0.0f;
    phase -= std::floor (phase);

    const float theta = static_cast<float> (kTwoPi * phase);

    // Orbit → affine (aspect on y, rotation, size, centre)
    const OrbitPoint b = baseOrbit (orbitKind, theta, orbitMod, scratch, superLUT);
    const float bx = b.x;
    const float by = b.y * aspect;
    const float r = 0.05f + 0.95f * position;
    float px = (bx * cosRot - by * sinRot) * r + centreX;
    float py = (bx * sinRot + by * cosRot) * r + centreY;

    // Trajectory feedback displacement lands in Phase 2.2 (fbD / fbY1 / fbY2).
    juce::ignoreUnused (partial);

    px = juce::jlimit (-1.0f, 1.0f, px);
    py = juce::jlimit (-1.0f, 1.0f, py);

    float y = terrain (terrainKind, px, py, terrainFreq, terrainModX, terrainModY);   // clamped inside

    // Core 10: the display voice's partial 0 writes (θ, p.x, p.y, y) per base sample
    if (capture != nullptr && partial == 0)
    {
        const uint32_t w = capture->writeIndex.load (std::memory_order_relaxed);
        float* slot = capture->ring.data() + (w % CycleCapture::kPoints) * 4;
        slot[0] = theta; slot[1] = px; slot[2] = py; slot[3] = y;
        capture->writeIndex.store (w + 1, std::memory_order_release);
    }

    // Saturation (DSP-07): skipped exactly at 0 so identity is bit-exact
    // (plan Decision 13). y' = tanh (g·y) / tanh (g), g = 1 + 4·sat; Padé tanh on
    // a ±3-clamped argument (RESEARCH §2.3).
    if (saturation > 0.0f && ! saturationBypass)
    {
        const float g = 1.0f + 4.0f * saturation;
        const float num = juce::dsp::FastMathApproximations::tanh (juce::jlimit (-3.0f, 3.0f, g * y));
        const float den = juce::dsp::FastMathApproximations::tanh (juce::jmin (3.0f, g));
        y = juce::jlimit (-1.0f, 1.0f, num / den);
    }

    return y;
}

void TerrainOscillator::getNextSampleStereo (double& outL, double& outR)
{
    outL = 0.0;
    outR = 0.0;

    // One sincos per oscillator per sample at most (only when the rotation moved)
    if (rotation != lastRotation)
    {
        sinRot = std::sin (rotation);
        cosRot = std::cos (rotation);
        lastRotation = rotation;
    }

    bool isSyncMode = (warpType == WarpType::Sync || warpType == WarpType::Window)
                      && warpAmount > 0.001f;

    for (int i = 0; i < unisonCount; ++i)
    {
        double readPhase = phaseAccumulators[i];

        if (isSyncMode)
        {
            double sample = scan (readPhase, i) * unisonGain;

            if (warpType == WarpType::Window)
                sample *= std::sin (kPi * masterPhases[i]);

            outL += sample * unisonPanL[i];
            outR += sample * unisonPanR[i];

            double masterInc = phaseIncrement * unisonDetuneFactors[i];
            masterPhases[i] += masterInc;

            double syncRatio = 1.0 + static_cast<double> (warpAmount) * 3.0;
            phaseAccumulators[i] += masterInc * syncRatio;

            if (phaseAccumulators[i] >= 1.0)
                phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);

            if (masterPhases[i] >= 1.0)
            {
                masterPhases[i] -= std::floor (masterPhases[i]);
                // Re-seed can land ≥ 1.0 (syncRatio up to 4) — wrap it too
                phaseAccumulators[i] = masterPhases[i] * syncRatio;
                phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
            }
        }
        else
        {
            double warped = applyWarp (readPhase);
            double sample = scan (warped, i) * unisonGain;
            outL += sample * unisonPanL[i];
            outR += sample * unisonPanR[i];

            phaseAccumulators[i] += phaseIncrement * unisonDetuneFactors[i];
            if (phaseAccumulators[i] >= 1.0)
                phaseAccumulators[i] -= std::floor (phaseAccumulators[i]);
        }
    }

    // Output conditioning (Core 1): 5 Hz one-pole DC blocker per channel
    const double bL = outL - dcX1L + dcR * dcY1L;
    dcX1L = outL; dcY1L = bL; outL = bL;
    const double bR = outR - dcX1R + dcR * dcY1R;
    dcX1R = outR; dcY1R = bR; outR = bR;
}
