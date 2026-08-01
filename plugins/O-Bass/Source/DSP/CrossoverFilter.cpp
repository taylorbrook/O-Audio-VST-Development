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

    CrossoverFilter.cpp
    O-Bass - Dual-Mode Crossover Filter Implementation

    Approach 3: Deferred Update - Pre-computes FIR coefficient bank at prepare()
    time. Parameter changes in FIR mode only update pendingFirIndex; filter
    reload occurs at next prepareToPlay() or mode switch.

  ==============================================================================
*/

#include "CrossoverFilter.h"
#include <numeric>

//==============================================================================
CrossoverFilter::CrossoverFilter()
{
    // Initialize smoothed cutoff with default 80Hz
    smoothedCutoff.setCurrentAndTargetValue(80.0f);

    // Set IIR filter types
    iirLowpass.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    iirHighpass.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    // Initialize FIR index tracking (80Hz = index 8)
    currentFirIndex = 8;
    pendingFirIndex = 8;
}

//==============================================================================
void CrossoverFilter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    blockSize = static_cast<int>(spec.maximumBlockSize);

    // Prepare IIR filters
    iirLowpass.prepare(spec);
    iirHighpass.prepare(spec);
    updateIIRFilters();

    // Calculate FIR tap count
    // For 40Hz crossover at 44.1kHz, need ~4096 taps minimum
    // Scale up for higher sample rates to maintain same frequency resolution
    firTapCount = static_cast<int>(4096.0 * sampleRate / 44100.0);

    // Ensure tap count is odd for symmetric linear-phase filter
    if (firTapCount % 2 == 0)
        firTapCount += 1;

    // Clamp to reasonable range (4096 to 16384)
    firTapCount = juce::jlimit(4096, 16384, firTapCount);

    // Prepare FIR convolution
    firLowpass.prepare(spec);

    // Pre-compute FIR coefficient bank for all frequencies (40-200Hz at 5Hz steps)
    precomputeFIRBank();

    // Load filter at pending index (applies any pending changes from previous session)
    // Both IIR and FIR are always prepared so mode switching is RT-safe
    loadFilterAtIndex(pendingFirIndex);
    currentFirIndex = pendingFirIndex;  // Sync indices so FIR is at desired frequency

    // Reset smoothed cutoff with ~10ms smoothing time
    smoothedCutoff.reset(sampleRate, 0.010);
    smoothedCutoff.setCurrentAndTargetValue(targetCutoffHz);
}

//==============================================================================
void CrossoverFilter::reset()
{
    iirLowpass.reset();
    iirHighpass.reset();
    firLowpass.reset();
    smoothedCutoff.setCurrentAndTargetValue(targetCutoffHz);
}

//==============================================================================
void CrossoverFilter::setMode(Mode newMode)
{
    // RT-SAFE: Just flip the atomic flag
    // Both IIR and FIR filters are always prepared and ready
    activeMode.store(newMode, std::memory_order_release);
}

//==============================================================================
void CrossoverFilter::setCutoffFrequency(float freqHz)
{
    // Clamp to valid range (40-200Hz per context decision)
    freqHz = juce::jlimit(40.0f, 200.0f, freqHz);

    if (std::abs(targetCutoffHz - freqHz) > 0.01f)
    {
        targetCutoffHz = freqHz;

        // IIR mode: use smoothed value (immediate response)
        smoothedCutoff.setTargetValue(freqHz);

        // FIR mode: store desired index for next prepare()
        // Do NOT reload filter in real-time - fully deferred
        if (activeMode.load(std::memory_order_acquire) == Mode::HighFidelity)
        {
            pendingFirIndex = frequencyToIndex(freqHz);
            // Note: Filter will update on next prepareToPlay()
        }
    }
}

//==============================================================================
void CrossoverFilter::process(const juce::AudioBuffer<float>& input,
                               juce::AudioBuffer<float>& lowBand,
                               juce::AudioBuffer<float>& highBand)
{
    // REAL-TIME SAFE: No allocations in this function.
    // FIR coefficients are pre-computed in prepare(), filter changes
    // are deferred until next prepareToPlay() or mode switch.

    const int numSamples = input.getNumSamples();
    const int numChannels = juce::jmin(input.getNumChannels(), 2);

    if (activeMode.load(std::memory_order_acquire) == Mode::LowLatency)
    {
        // IIR mode: sample-by-sample processing with smoothed cutoff
        // Optimization: Update filter coefficients every 16 samples instead of per-sample
        // setCutoffFrequency() recalculates coefficients which is expensive
        constexpr int kCoefficientUpdateInterval = 16;
        int samplesSinceUpdate = kCoefficientUpdateInterval;  // Force update on first sample

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Update cutoff every N samples if smoothing is active
            if (smoothedCutoff.isSmoothing())
            {
                if (++samplesSinceUpdate >= kCoefficientUpdateInterval)
                {
                    float newCutoff = smoothedCutoff.getNextValue();
                    iirLowpass.setCutoffFrequency(newCutoff);
                    iirHighpass.setCutoffFrequency(newCutoff);
                    samplesSinceUpdate = 0;
                }
                else
                {
                    // Advance smoother without applying (keeps timing consistent)
                    smoothedCutoff.getNextValue();
                }
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float in = input.getSample(ch, sample);
                float low, high;

                // LR4 processSample gives both outputs with correct phase
                iirLowpass.processSample(ch, in, low, high);

                lowBand.setSample(ch, sample, low);
                highBand.setSample(ch, sample, high);
            }
        }
    }
    else
    {
        // FIR mode: block-based convolution with pre-loaded filter
        // No filter updates here - completely RT-safe

        // Copy input to lowBand for in-place convolution processing
        lowBand.makeCopyOf(input);

        // Process lowpass via convolution
        juce::dsp::AudioBlock<float> lowBlock(lowBand);
        juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
        firLowpass.process(lowContext);

        // Compute highpass as complementary filter: high = input - low
        // This maintains perfect reconstruction (linear phase)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* inputData = input.getReadPointer(ch);
            const float* lowData = lowBand.getReadPointer(ch);
            float* highData = highBand.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                highData[i] = inputData[i] - lowData[i];
            }
        }
    }
}

//==============================================================================
int CrossoverFilter::getLatencyInSamples() const
{
    if (activeMode.load(std::memory_order_acquire) == Mode::LowLatency)
    {
        // IIR mode: effectively zero latency
        return 0;
    }
    else
    {
        // FIR mode: symmetric linear-phase filter has (N-1)/2 latency
        return (firTapCount - 1) / 2;
    }
}

//==============================================================================
void CrossoverFilter::updateIIRFilters()
{
    iirLowpass.setCutoffFrequency(targetCutoffHz);
    iirHighpass.setCutoffFrequency(targetCutoffHz);
}

//==============================================================================
void CrossoverFilter::precomputeFIRBank()
{
    // Resize bank to hold all pre-computed filters
    firCoefficientBank.resize(static_cast<size_t>(kNumPrecomputedFilters));

    // Pre-compute coefficients for each frequency (40, 45, 50, ..., 200 Hz)
    for (int i = 0; i < kNumPrecomputedFilters; ++i)
    {
        float freq = kMinFreq + static_cast<float>(i) * kFreqStep;
        firCoefficientBank[static_cast<size_t>(i)] = generateWindowedSincLowpass(freq, firTapCount);
    }
}

//==============================================================================
int CrossoverFilter::frequencyToIndex(float freqHz) const
{
    // Clamp and quantize to nearest pre-computed index
    freqHz = juce::jlimit(kMinFreq, kMaxFreq, freqHz);
    int index = static_cast<int>(std::round((freqHz - kMinFreq) / kFreqStep));
    return juce::jlimit(0, kNumPrecomputedFilters - 1, index);
}

//==============================================================================
void CrossoverFilter::loadFilterAtIndex(int index)
{
    // This function is ONLY called from prepare() or setMode() - never from RT thread
    // Allocations here are acceptable (non-RT safe points)

    if (firCoefficientBank.empty() || index < 0 || index >= kNumPrecomputedFilters)
        return;

    const auto& coeffs = firCoefficientBank[static_cast<size_t>(index)];

    // Create AudioBuffer from pre-computed coefficients
    juce::AudioBuffer<float> irBuffer(1, firTapCount);
    irBuffer.copyFrom(0, 0, coeffs.data(), firTapCount);

    firLowpass.loadImpulseResponse(
        std::move(irBuffer),
        sampleRate,
        juce::dsp::Convolution::Stereo::no,
        juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::no
    );

    currentFirIndex = index;
}

//==============================================================================
std::vector<float> CrossoverFilter::generateWindowedSincLowpass(float cutoffHz, int numTaps)
{
    std::vector<float> coeffs(static_cast<size_t>(numTaps));

    // Normalized cutoff frequency (0 to 0.5)
    float fc = static_cast<float>(cutoffHz / sampleRate);
    int M = numTaps - 1;

    for (int n = 0; n <= M; ++n)
    {
        // Sinc function
        if (n == M / 2)
        {
            coeffs[static_cast<size_t>(n)] = 2.0f * fc;
        }
        else
        {
            float x = static_cast<float>(n) - static_cast<float>(M) / 2.0f;
            coeffs[static_cast<size_t>(n)] = std::sin(2.0f * juce::MathConstants<float>::pi * fc * x)
                                             / (juce::MathConstants<float>::pi * x);
        }

        // Apply Blackman window for good stopband attenuation (~74dB)
        float window = 0.42f
                     - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(n) / static_cast<float>(M))
                     + 0.08f * std::cos(4.0f * juce::MathConstants<float>::pi * static_cast<float>(n) / static_cast<float>(M));

        coeffs[static_cast<size_t>(n)] *= window;
    }

    // Normalize sum to 1.0 for unity gain at DC
    float sum = std::accumulate(coeffs.begin(), coeffs.end(), 0.0f);
    if (sum > 0.0f)
    {
        for (auto& c : coeffs)
            c /= sum;
    }

    return coeffs;
}
