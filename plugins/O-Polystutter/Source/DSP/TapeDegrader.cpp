/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
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

    TapeDegrader.cpp
    Ouaricon Polystutter - Tape Degradation Implementation
    Phase 2.4: Analog tape simulation effects

  ==============================================================================
*/

#include "TapeDegrader.h"

TapeDegrader::TapeDegrader()
{
    // Initialize saturation waveshaper with tanh function
    // Drive is applied before the waveshaper, so use simple tanh here
    saturationShaper.functionToUse = [](float x) {
        return std::tanh(x);
    };

    // Initialize LFOs with sine waveforms
    wowLFO.initialise([](float x) { return std::sin(x); }, 128);
    flutterLFO.initialise([](float x) { return std::sin(x); }, 128);
}

void TapeDegrader::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Prepare saturation waveshaper
    saturationShaper.prepare(spec);
    saturationShaper.reset();

    // Prepare LFOs
    wowLFO.prepare(spec);
    flutterLFO.prepare(spec);
    wowLFO.setFrequency(0.1f);  // Default slow wow
    flutterLFO.setFrequency(5.0f);  // Default fast flutter

    // Prepare wow/flutter delay lines (max 100ms delay for modulation)
    int maxDelaySamples = static_cast<int>(spec.sampleRate * 0.1);  // 100ms
    wowFlutterDelayLeft.prepare(spec);
    wowFlutterDelayRight.prepare(spec);
    wowFlutterDelayLeft.setMaximumDelayInSamples(maxDelaySamples);
    wowFlutterDelayRight.setMaximumDelayInSamples(maxDelaySamples);
    wowFlutterDelayLeft.reset();
    wowFlutterDelayRight.reset();

    // Prepare hiss bandpass filters (5kHz-15kHz)
    updateHissBandpass();
    hissBandpassLeft.prepare(spec);
    hissBandpassRight.prepare(spec);
    hissBandpassLeft.reset();
    hissBandpassRight.reset();

    // Prepare rolloff lowpass filter
    updateRolloffFilter();
    rolloffFilterLeft.prepare(spec);
    rolloffFilterRight.prepare(spec);
    rolloffFilterLeft.reset();
    rolloffFilterRight.reset();

    // Reset dropout state
    isDroppedOut = false;
    dropoutSamplesRemaining = 0;
    dropoutFadeSamples = static_cast<int>(spec.sampleRate * 0.005);  // 5ms crossfade
}

void TapeDegrader::reset()
{
    saturationShaper.reset();
    wowLFO.reset();
    flutterLFO.reset();
    wowFlutterDelayLeft.reset();
    wowFlutterDelayRight.reset();
    hissBandpassLeft.reset();
    hissBandpassRight.reset();
    rolloffFilterLeft.reset();
    rolloffFilterRight.reset();

    isDroppedOut = false;
    dropoutSamplesRemaining = 0;
    lastRolloffAmount = -1.0f;  // Force filter coefficient update on next process
}

void TapeDegrader::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return;

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // ========== 1. Saturation ==========
    if (saturationAmount > 0.01f)
    {
        // Apply drive gain before waveshaper (1.0 to 5.0)
        float drive = 1.0f + (saturationAmount * 4.0f);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);
            juce::FloatVectorOperations::multiply(channelData, drive, numSamples);
        }

        // Apply tanh waveshaper
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        saturationShaper.process(context);

        // Output compensation to maintain perceived loudness
        float compensation = 1.0f / drive;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);
            juce::FloatVectorOperations::multiply(channelData, compensation, numSamples);
        }
    }

    // ========== 2. Wow/Flutter (Pitch Modulation) ==========
    if (wowAmount > 0.01f || flutterAmount > 0.01f)
    {
        updateWowFlutterLFOs();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Generate modulation signal (combined wow + flutter)
            float wowMod = wowLFO.processSample(0.0f) * wowAmount * 0.02f;  // 2% max depth
            float flutterMod = flutterLFO.processSample(0.0f) * flutterAmount * 0.01f;  // 1% max depth
            float totalMod = wowMod + flutterMod;

            // Convert pitch modulation to delay time variation
            // Positive modulation = pitch up = shorter delay
            // Negative modulation = pitch down = longer delay
            float baseDelay = 5.0f;  // 5 samples base delay
            float modulatedDelay = baseDelay * (1.0f + totalMod);
            modulatedDelay = juce::jlimit(1.0f, 100.0f, modulatedDelay);

            // Process left channel
            if (numChannels > 0)
            {
                float inputSample = buffer.getSample(0, sample);
                wowFlutterDelayLeft.pushSample(0, inputSample);
                float delayedSample = wowFlutterDelayLeft.popSample(0, modulatedDelay);
                buffer.setSample(0, sample, delayedSample);
            }

            // Process right channel
            if (numChannels > 1)
            {
                float inputSample = buffer.getSample(1, sample);
                wowFlutterDelayRight.pushSample(0, inputSample);
                float delayedSample = wowFlutterDelayRight.popSample(0, modulatedDelay);
                buffer.setSample(1, sample, delayedSample);
            }
        }
    }

    // ========== 3. Hiss (High-Frequency Noise) ==========
    if (hissAmount > 0.01f)
    {
        // Calculate hiss level: -60dB to -56dB (scaled down 10x - previous range was too loud)
        float hissLevelDB = -60.0f + (hissAmount * 4.0f);
        float hissLevel = juce::Decibels::decibelsToGain(hissLevelDB);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Generate white noise
            float noiseSample = (noiseGenerator.nextFloat() * 2.0f - 1.0f) * hissLevel;

            // Filter to 5kHz-15kHz bandpass
            float filteredNoiseLeft = hissBandpassLeft.processSample(noiseSample);
            float filteredNoiseRight = hissBandpassRight.processSample(noiseSample);

            // Add to audio
            if (numChannels > 0)
                buffer.setSample(0, sample, buffer.getSample(0, sample) + filteredNoiseLeft);
            if (numChannels > 1)
                buffer.setSample(1, sample, buffer.getSample(1, sample) + filteredNoiseRight);
        }
    }

    // ========== 4. Rolloff (High-Frequency Attenuation) ==========
    if (rolloffAmount > 0.01f)
    {
        // Only update filter coefficients when rolloff amount changes (avoid allocation in audio thread)
        if (std::abs(rolloffAmount - lastRolloffAmount) > 0.001f)
        {
            updateRolloffFilter();
            lastRolloffAmount = rolloffAmount;
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (numChannels > 0)
            {
                float inputSample = buffer.getSample(0, sample);
                float filtered = rolloffFilterLeft.processSample(inputSample);
                buffer.setSample(0, sample, filtered);
            }

            if (numChannels > 1)
            {
                float inputSample = buffer.getSample(1, sample);
                float filtered = rolloffFilterRight.processSample(inputSample);
                buffer.setSample(1, sample, filtered);
            }
        }
    }

    // ========== 5. Dropout (Random Signal Loss) ==========
    if (dropoutAmount > 0.01f)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Check if we should start a new dropout
            if (!isDroppedOut && dropoutSamplesRemaining == 0)
            {
                // Per-sample probability: dropout * 0.001 (0.1% chance at 100%)
                float dropoutChance = dropoutAmount * 0.001f;
                if (noiseGenerator.nextFloat() < dropoutChance)
                {
                    // Start dropout with random duration 10-100ms
                    int minDropout = static_cast<int>(sampleRate * 0.01);   // 10ms
                    int maxDropout = static_cast<int>(sampleRate * 0.1);    // 100ms
                    dropoutSamplesRemaining = minDropout + noiseGenerator.nextInt(maxDropout - minDropout);
                    isDroppedOut = true;
                    dropoutFadePosition = 0;
                }
            }

            // Process dropout
            if (isDroppedOut)
            {
                float dropoutGain = 1.0f;

                // Fade out (first 5ms)
                if (dropoutFadePosition < dropoutFadeSamples)
                {
                    dropoutGain = 1.0f - (static_cast<float>(dropoutFadePosition) / dropoutFadeSamples);
                }
                // Silence (middle section)
                else if (dropoutSamplesRemaining > dropoutFadeSamples)
                {
                    dropoutGain = 0.0f;
                }
                // Fade in (last 5ms)
                else
                {
                    int fadeInPos = juce::jmax(0, dropoutFadeSamples - dropoutSamplesRemaining);
                    dropoutGain = static_cast<float>(fadeInPos) / dropoutFadeSamples;
                }

                // Apply dropout gain
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float currentSample = buffer.getSample(ch, sample);
                    buffer.setSample(ch, sample, currentSample * dropoutGain);
                }

                dropoutFadePosition++;
                dropoutSamplesRemaining--;

                // End dropout when duration complete
                if (dropoutSamplesRemaining <= 0)
                {
                    isDroppedOut = false;
                    dropoutFadePosition = 0;
                }
            }
        }
    }
}

void TapeDegrader::setSaturation(float saturationPercent)
{
    saturationAmount = juce::jlimit(0.0f, 1.0f, saturationPercent / 100.0f);
}

void TapeDegrader::setWow(float wowPercent)
{
    wowAmount = juce::jlimit(0.0f, 1.0f, wowPercent / 100.0f);
}

void TapeDegrader::setFlutter(float flutterPercent)
{
    flutterAmount = juce::jlimit(0.0f, 1.0f, flutterPercent / 100.0f);
}

void TapeDegrader::setHiss(float hissPercent)
{
    hissAmount = juce::jlimit(0.0f, 1.0f, hissPercent / 100.0f);
}

void TapeDegrader::setRolloff(float rolloffPercent)
{
    rolloffAmount = juce::jlimit(0.0f, 1.0f, rolloffPercent / 100.0f);
}

void TapeDegrader::setDropout(float dropoutPercent)
{
    dropoutAmount = juce::jlimit(0.0f, 1.0f, dropoutPercent / 100.0f);
}

// ========== Helper Functions ==========

float TapeDegrader::tanhSaturation(float input, float drive)
{
    // Apply input gain, then tanh, then compensate
    float driven = input * drive;
    return std::tanh(driven) / drive;
}

void TapeDegrader::updateWowFlutterLFOs()
{
    // Wow: 0.1Hz to 3Hz (slow pitch modulation)
    float wowFreq = 0.1f + (wowAmount * 2.9f);
    wowLFO.setFrequency(wowFreq);

    // Flutter: 5Hz to 20Hz (fast pitch modulation)
    float flutterFreq = 5.0f + (flutterAmount * 15.0f);
    flutterLFO.setFrequency(flutterFreq);
}

void TapeDegrader::updateRolloffFilter()
{
    // Lowpass cutoff: 20kHz to 2kHz
    float cutoffFreq = 20000.0f - (rolloffAmount * 18000.0f);
    cutoffFreq = juce::jlimit(2000.0f, 20000.0f, cutoffFreq);

    // Runs on the audio thread during rolloff automation: ArrayCoefficients
    // returns a stack std::array (no heap allocation), and the in-place
    // assignment reuses the arrays pre-sized by the prepare()-time call.
    const auto coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
        sampleRate,
        cutoffFreq,
        0.707f  // Q = 1/sqrt(2) (Butterworth response)
    );

    *rolloffFilterLeft.coefficients = coefficients;
    *rolloffFilterRight.coefficients = coefficients;
}

void TapeDegrader::updateHissBandpass()
{
    // Hiss noise coloring: lowpass at 15kHz only (highpass stage intentionally
    // omitted — less critical for hiss simulation)
    const auto coefficientsLow = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
        sampleRate,
        15000.0f,
        0.707f
    );

    *hissBandpassLeft.coefficients = coefficientsLow;
    *hissBandpassRight.coefficients = coefficientsLow;
}
