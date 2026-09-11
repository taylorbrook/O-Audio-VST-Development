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
    hbCoeffs = &HalfbandCoeffs::get();   // designed once per process (allocates; prepare only)
    for (int s = 0; s < 2; ++s)
        for (int i = 0; i < kMaxUnison; ++i) { hb2[s][i].reset(); hb4[s][i].reset(); }
    xfadeRemaining = 0;
    fresh = true;
    oversampling = osFor (quality);
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
    if (q == quality)
        return;

    if (fresh || xfadeRemaining > 0)
    {
        // Inactive (or already fading): switch instantly, no crossfade (plan Decision 18)
        quality = q;
        oversampling = osFor (q);
        xfadeRemaining = 0;
        for (int i = 0; i < kMaxUnison; ++i) { hb2[currentSet][i].reset(); hb4[currentSet][i].reset(); }
        return;
    }

    // Mid-note: the old path keeps its decimator set as the shadow, the new path
    // starts from reset states and both run for 64 base samples (equal-gain linear).
    oldQuality = quality;
    quality = q;
    oversampling = osFor (q);
    currentSet ^= 1;
    for (int i = 0; i < kMaxUnison; ++i) { hb2[currentSet][i].reset(); hb4[currentSet][i].reset(); }
    xfadeRemaining = 64;
}

void TerrainOscillator::reset()
{
    for (int i = 0; i < kMaxUnison; ++i)
    {
        phaseAccumulators[i] = 0.0;
        masterPhases[i] = 0.0;
        fbD[i] = fbY1[i] = fbY2[i] = 0.0f;
        hb2[currentSet][i].reset(); hb4[currentSet][i].reset();
    }
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
    xfadeRemaining = 0;
    fresh = true;
}

void TerrainOscillator::resetWithPhase (double phase)
{
    for (int i = 0; i < kMaxUnison; ++i)
    {
        phaseAccumulators[i] = phase;
        masterPhases[i] = phase;
        fbD[i] = fbY1[i] = fbY2[i] = 0.0f;
        hb2[currentSet][i].reset(); hb4[currentSet][i].reset();
    }
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
    xfadeRemaining = 0;
    fresh = true;
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
        hb2[currentSet][i].reset(); hb4[currentSet][i].reset();
    }
    dcX1L = dcY1L = dcX1R = dcY1R = 0.0;
    xfadeRemaining = 0;
    fresh = true;
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

        // Detune: max 50 cents spread
        unisonDetuneFactors[i] = std::pow (2.0, normalizedPos * detune * 50.0 / 1200.0);   // block-rate (DSP-05 exemption: setUnison cache, plan Decision 10)

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

void TerrainOscillator::updateBlockRate (double noteHz)
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

    // Pitch tracking (DSP-01): F_eff = F_mod · r_track, r_track = min (1, C4 / f_note)^track,
    // from the glide TARGET the voice passes in. Block-rate pow (DSP-05 exemption: updateBlockRate).
    // Round A applies it in every Quality (Bandlimited = analytic fallback); Round B makes it inert there.
    const double ratio = noteHz > 0.0 ? juce::jmin (1.0, 261.6256 / noteHz) : 1.0;
    rTrack = static_cast<float> (std::pow (ratio, static_cast<double> (pitchTrack)));   // block-rate (DSP-05 exemption: updateBlockRate)

    // Feedback damp law (ARCH Core 4, Decision 1): a = 1 − 2^(−1 − 9·damp) at 48 kHz / 1×,
    // rate- and OS-corrected so the displacement time constant is invariant.
    const double a = 1.0 - std::exp2 (-1.0 - 9.0 * static_cast<double> (feedbackDamp));   // block-rate (DSP-05 exemption: updateBlockRate)
    aEff = static_cast<float> (std::pow (a, 48000.0 / (currentSampleRate * oversampling)));   // block-rate (DSP-05 exemption: updateBlockRate)
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
            return std::pow (phase, exponent);   // inherited O-Prism Bend warp, per-sample (documented DSP-05 exemption: Round A SUMMARY)
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

float TerrainOscillator::scan (double phase, int partial, bool shadow) noexcept
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

    // Trajectory feedback (ARCH Core 4, Decision 1): p += fb · d · û, û = the orbit's
    // rotation vector; d = a·d + (1 − a)·½ (y1 + y2) — the two-sample average is the
    // anti-hunting zero at Nyquist; d clamped ± 0.5 and NaN-scrubbed. With fb = 0 the
    // add is 0·finite = 0 and px + 0 == px bit-exactly (H3 memcmp row). The harness
    // switches select the single-sample form (Nyquist control) or skip the path.
    if (feedbackPathEnabled && feedbackActive)
    {
        const float avg = singleSampleFeedback ? fbY1[partial] : 0.5f * (fbY1[partial] + fbY2[partial]);
        float d = aEff * fbD[partial] + (1.0f - aEff) * avg;
        if (! std::isfinite (d)) d = 0.0f;
        d = juce::jlimit (-0.5f, 0.5f, d);
        if (! shadow) fbD[partial] = d;
        const float disp = feedback * d;
        px += disp * cosRot;
        py += disp * sinRot;
    }

    px = juce::jlimit (-1.0f, 1.0f, px);
    py = juce::jlimit (-1.0f, 1.0f, py);

    float y = terrain (terrainKind, px, py, terrainFreq * rTrack, terrainModX, terrainModY);   // clamped inside

    if (shadow)
        return y;   // crossfade's old path: no feedback / capture writes (saturation below is stateless)

    // The feedback tap reads the pre-saturation, pre-blocker scan value (Core 1)
    fbY2[partial] = fbY1[partial];
    fbY1[partial] = y;

    // Core 10: the display voice's partial 0 writes (θ, p.x, p.y, y) per base sample
    if (capture != nullptr && partial == 0)
    {
        const uint32_t w = capture->writeIndex.load (std::memory_order_relaxed);
        float* slot = capture->ring.data() + (w % CycleCapture::kPoints) * 4;
        slot[0] = theta; slot[1] = px; slot[2] = py; slot[3] = y;
        capture->writeIndex.store (w + 1, std::memory_order_release);
    }

    return saturate (y);
}

float TerrainOscillator::saturate (float y) const noexcept
{
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

float TerrainOscillator::subSample (int i, double& phase, double& master, double inc, bool isSyncMode, bool shadow) noexcept
{
    // The inherited Sync / Window and Bend / FM branches, one sub-sample (inc = phaseIncrement / OS)
    const double readPhase = phase;
    float sample;

    if (isSyncMode)
    {
        sample = scan (readPhase, i, shadow);

        if (warpType == WarpType::Window)
            sample *= static_cast<float> (std::sin (kPi * master));

        const double masterInc = inc * unisonDetuneFactors[i];
        master += masterInc;

        const double syncRatio = 1.0 + static_cast<double> (warpAmount) * 3.0;
        phase += masterInc * syncRatio;

        if (phase >= 1.0)
            phase -= std::floor (phase);

        if (master >= 1.0)
        {
            master -= std::floor (master);
            // Re-seed can land ≥ 1.0 (syncRatio up to 4) — wrap it too
            phase = master * syncRatio;
            phase -= std::floor (phase);
        }
    }
    else
    {
        const double warped = applyWarp (readPhase);
        sample = scan (warped, i, shadow);

        phase += inc * unisonDetuneFactors[i];
        if (phase >= 1.0)
            phase -= std::floor (phase);
    }

    return sample;
}

float TerrainOscillator::decimate (int i, int set, int os, const float* sub) noexcept
{
    if (os == 1)
        return sub[0];
    if (os == 2)
        return hb2[set][i].down (sub[0], sub[1], *hbCoeffs);
    const float a = hb4[set][i].down (sub[0], sub[1], *hbCoeffs);
    const float b = hb4[set][i].down (sub[2], sub[3], *hbCoeffs);
    return hb2[set][i].down (a, b, *hbCoeffs);
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

    const bool isSyncMode = (warpType == WarpType::Sync || warpType == WarpType::Window)
                            && warpAmount > 0.001f;
    fresh = false;

    // Oversampling loop (ARCH Core 1 / 5): per partial, OS sub-samples with phase
    // increment phaseIncrement / OS, every setter value held over the sub-samples,
    // feedback per sub-sample, then one halfband-decimated base sample. With
    // kernelBypass (H7 baseline) the scan returns 0 and the decimator is skipped.
    const int os = oversampling;
    const double inc = phaseIncrement / os;

    for (int i = 0; i < unisonCount; ++i)
    {
        float sample;

        if (kernelBypass)
        {
            for (int k = 0; k < os; ++k)
                subSample (i, phaseAccumulators[i], masterPhases[i], inc, isSyncMode, false);
            sample = 0.0f;
        }
        else
        {
            float sub[4];

            if (xfadeRemaining > 0)
            {
                // Old path as a shadow from the same θ (local phase copies, no state writes)
                const int oldOs = osFor (oldQuality);
                const double oldInc = phaseIncrement / oldOs;
                double p = phaseAccumulators[i], m = masterPhases[i];
                float oldSub[4];
                for (int k = 0; k < oldOs; ++k)
                    oldSub[k] = subSample (i, p, m, oldInc, isSyncMode, true);
                const float oldY = decimate (i, currentSet ^ 1, oldOs, oldSub);

                for (int k = 0; k < os; ++k)
                    sub[k] = subSample (i, phaseAccumulators[i], masterPhases[i], inc, isSyncMode, false);
                const float newY = decimate (i, currentSet, os, sub);

                const float t = 1.0f - static_cast<float> (xfadeRemaining) / 64.0f;   // 0 → 1 over the fade
                sample = oldY + t * (newY - oldY);
            }
            else
            {
                for (int k = 0; k < os; ++k)
                    sub[k] = subSample (i, phaseAccumulators[i], masterPhases[i], inc, isSyncMode, false);
                sample = decimate (i, currentSet, os, sub);
            }
        }

        const double s = static_cast<double> (sample) * unisonGain;
        outL += s * unisonPanL[i];
        outR += s * unisonPanR[i];
    }

    if (xfadeRemaining > 0)
        --xfadeRemaining;

    // Output conditioning (Core 1): 5 Hz one-pole DC blocker per channel
    if (dcBlockerBypass)
        return;   // harness-only: expose the raw scan sum (H3 boundedness row)
    const double bL = outL - dcX1L + dcR * dcY1L;
    dcX1L = outL; dcY1L = bL; outL = bL;
    const double bR = outR - dcX1R + dcR * dcY1R;
    dcX1R = outR; dcY1R = bR; outR = bR;
}
