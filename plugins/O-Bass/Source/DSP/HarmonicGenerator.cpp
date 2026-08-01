/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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

    HarmonicGenerator.cpp
    O-Bass - Chebyshev Waveshaper Implementation

    Generates 2nd and 3rd harmonics using Chebyshev polynomials for controlled
    harmonic generation. Output is bandpassed to 30-500Hz for psychoacoustic
    bass enhancement.

    Note: Oversampling is disabled due to JUCE compatibility issues with
    Logic Pro (causes "Sample Rate XXXXX" crash). Processing occurs at native
    sample rate with gentler waveshaping to minimize aliasing.

  ==============================================================================
*/

#include "HarmonicGenerator.h"
#include <cmath>

//==============================================================================
// Chebyshev Polynomials T2, T3
// For input x in [-1, 1], Tn(cos(theta)) = cos(n*theta)
// This generates the nth harmonic when applied to a sinusoidal input

inline float T2(float x) { return 2.0f * x * x - 1.0f; }
inline float T3(float x) { return 4.0f * x * x * x - 3.0f * x; }

//==============================================================================
HarmonicGenerator::HarmonicGenerator()
{
    // No oversampling - processing at native sample rate
}

//==============================================================================
void HarmonicGenerator::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    blockSize = static_cast<int>(spec.maximumBlockSize);

    // Prepare bandpass filters
    juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32>(blockSize), 1 };
    outputBandpassLow.prepare(monoSpec);
    outputBandpassHigh.prepare(monoSpec);

    // Set filter coefficients - bandpass 30-500Hz for bass harmonics
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 30.0f, 0.707f);
    outputBandpassLow.coefficients = hpCoeffs;

    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 500.0f, 0.707f);
    outputBandpassHigh.coefficients = lpCoeffs;

    // Reset to clear any garbage state
    outputBandpassLow.reset();
    outputBandpassHigh.reset();
}

//==============================================================================
void HarmonicGenerator::reset()
{
    outputBandpassLow.reset();
    outputBandpassHigh.reset();
}

//==============================================================================
void HarmonicGenerator::setMode(Mode newMode)
{
    // Mode stored for potential future oversampling implementation
    activeMode.store(newMode, std::memory_order_release);
}

//==============================================================================
void HarmonicGenerator::process(juce::AudioBuffer<float>& monoBuffer)
{
    const int numSamples = monoBuffer.getNumSamples();
    if (numSamples == 0 || monoBuffer.getNumChannels() < 1)
        return;

    // OVERSAMPLING BYPASSED: JUCE Oversampling causes Logic Pro crash
    // ("Sample Rate XXXXX" error - memory corruption from oversampler internals)
    // Process directly at native sample rate with gentler waveshaping to minimize aliasing

    float* data = monoBuffer.getWritePointer(0);

    // Process waveshaping at native rate (no oversampling)
    processOversampled(data, numSamples);

    // Apply output bandpass filter (40-300Hz) at original sample rate
    for (int i = 0; i < numSamples; ++i)
    {
        data[i] = outputBandpassLow.processSample(data[i]);
        data[i] = outputBandpassHigh.processSample(data[i]);
    }
}

//==============================================================================
void HarmonicGenerator::processOversampled(float* data, int numSamples)
{
    // Generate harmonics using Chebyshev polynomials
    // Output is ONLY the harmonic content (added to dry signal in CleanModeProcessor)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Safety: skip processing if input is invalid
        if (std::isnan(x) || std::isinf(x))
        {
            data[i] = 0.0f;
            continue;
        }

        // Soft clip input to [-1, 1] for Chebyshev polynomials
        float clipped = std::tanh(x * kInputDrive);

        // Generate 2nd and 3rd harmonics (most important for psychoacoustic bass)
        float h2 = T2(clipped) * kH2Weight;  // 2nd harmonic - adds warmth
        float h3 = T3(clipped) * kH3Weight;  // 3rd harmonic - adds presence

        // Output is the harmonic content only (will be mixed with dry)
        float harmonics = (h2 + h3) * kHarmonicMix;

        // Soft limit
        data[i] = std::tanh(harmonics);
    }
}

//==============================================================================
int HarmonicGenerator::getLatencyInSamples() const
{
    // Oversampling disabled - no latency
    return 0;
}
