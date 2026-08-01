/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

    Compressor.h
    Single-band feed-forward compressor with soft knee
    O-MultiBandCompressor
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "EnvelopeDetector.h"
#include "GainComputer.h"

class Compressor
{
public:
    Compressor() = default;
    ~Compressor() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;

        // Prepare envelope detector
        envelopeDetector.prepare(sampleRate, maxBlockSize);

        // Initialize smoothed gain reduction
        smoothedGainReductionDB = 0.0f;

        // Update coefficients
        updateAttackReleaseCoefficients();

        // Prepare sidechain filters
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1; // Mono sidechain

        scHPF.prepare(spec);
        scLPF.prepare(spec);
        scHPF.reset();
        scLPF.reset();

        // Seed both filters with a real biquad here, on the host thread. This is what
        // makes the later assignments RT-safe: Coefficients::operator=(std::array)
        // calls clearQuick() + ensureStorageAllocated(8) + add(), which only touches
        // the heap if the storage is not already big enough. Doing it once here means
        // every subsequent update on the audio thread reuses that allocation.
        *scHPF.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, 1000.0f, 0.707f);
        *scLPF.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, 1000.0f, 0.707f);

        // Force the first processed block to reconfigure from the live parameters
        // rather than trusting a cached frequency from a previous sample rate.
        currentSCHPFFreq = 0.0f;
        currentSCLPFFreq = 0.0f;
        scHPFEnabled = false;
        scLPFEnabled = false;

        // Initialize auto-makeup
        averageGainReduction = 0.0f;
        autoMakeupSmoothingCoeff = 1.0f - std::exp(-1.0f / (0.5f * static_cast<float>(sampleRate))); // 500ms smoothing
    }

    void reset()
    {
        envelopeDetector.reset();
        smoothedGainReductionDB = 0.0f;
        scHPF.reset();
        scLPF.reset();
        averageGainReduction = 0.0f;
    }

    void setAttackTime(float attackMs)
    {
        // IN-02: recompute the (exp-heavy) coefficient only when the value actually changes.
        if (attackMs == currentAttackMs)
            return;

        currentAttackMs = attackMs;
        attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(currentSampleRate)));
    }

    void setReleaseTime(float releaseMs)
    {
        // IN-02: recompute the (exp-heavy) coefficient only when the value actually changes.
        if (releaseMs == currentReleaseMs)
            return;

        currentReleaseMs = releaseMs;
        releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(currentSampleRate)));
    }

    // Process stereo buffer with sidechain filtering and auto-makeup.
    // numActiveChannels: how many channels of `buffer` actually carry signal (WR-02) —
    // the band buffers are preallocated stereo, but in mono M/S modes only channel 0 is
    // filled; averaging the silent channel into the detector halves the level (−6 dB).
    void processStereo(juce::AudioBuffer<float>& buffer,
                      int numActiveChannels,
                      float thresholdDB,
                      float ratio,
                      float kneeDB,
                      float makeupDB,
                      float peakRmsBlend,
                      bool isBypassed,
                      float scHPFFreq,
                      float scLPFFreq,
                      bool scListen,
                      bool autoMakeupEnabled,
                      std::atomic<float>& gainReductionMeter)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = juce::jmin(numActiveChannels, buffer.getNumChannels());

        if (numSamples == 0 || numChannels == 0)
            return;

        // Update sidechain filter coefficients
        updateSidechainFilters(scHPFFreq, scLPFFreq);

        if (isBypassed)
        {
            gainReductionMeter.store(0.0f, std::memory_order_relaxed);
            return;
        }

        // Cache channel data pointers once (IN-01) — the band buffers are at most stereo.
        jassert(numChannels <= 2);
        float* chanData[2] = { nullptr, nullptr };
        for (int channel = 0; channel < numChannels; ++channel)
            chanData[channel] = buffer.getWritePointer(channel);

        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Use stereo link: average both channels for detection
            float detectorInput = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
            {
                detectorInput += chanData[channel][sample];
            }
            detectorInput /= static_cast<float>(numChannels);

            // Apply sidechain filtering to detector signal (not audio path)
            float filteredSidechain = applySidechainFilters(detectorInput);

            // Sidechain listen mode: replace audio output with filtered sidechain
            if (scListen)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    chanData[channel][sample] = filteredSidechain;
                }
                continue; // Skip compression processing in listen mode
            }

            // Envelope detection (peak/RMS blend) on filtered sidechain
            float envelopeLevel = envelopeDetector.processSample(filteredSidechain, peakRmsBlend);

            // Gain computer (soft knee)
            float targetGainReductionDB = gainComputer.calculateGainReduction(
                envelopeLevel, thresholdDB, ratio, kneeDB);

            // Envelope smoothing (attack/release ballistics)
            if (targetGainReductionDB < smoothedGainReductionDB)
            {
                // Rising gain reduction (more compression) - use attack
                smoothedGainReductionDB += (targetGainReductionDB - smoothedGainReductionDB) * (1.0f - attackCoeff);
            }
            else
            {
                // Falling gain reduction (less compression) - use release
                smoothedGainReductionDB += (targetGainReductionDB - smoothedGainReductionDB) * (1.0f - releaseCoeff);
            }

            // Update auto-makeup calculation (running average of GR)
            averageGainReduction += (smoothedGainReductionDB - averageGainReduction) * autoMakeupSmoothingCoeff;

            // Calculate total makeup gain
            float autoMakeupDB = autoMakeupEnabled ? (-averageGainReduction * 0.8f) : 0.0f;
            float totalMakeupDB = makeupDB + autoMakeupDB;

            // Convert to linear gain
            float gainReductionLinear = std::pow(10.0f, smoothedGainReductionDB / 20.0f);
            float makeupLinear = std::pow(10.0f, totalMakeupDB / 20.0f);
            float totalGain = gainReductionLinear * makeupLinear;

            // Apply gain to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                chanData[channel][sample] *= totalGain;
            }
        }

        // Update gain reduction meter (atomic for UI thread)
        gainReductionMeter.store(smoothedGainReductionDB, std::memory_order_relaxed);
    }

private:
    void updateAttackReleaseCoefficients()
    {
        // Convert ms to seconds
        float attackSec = currentAttackMs * 0.001f;
        float releaseSec = currentReleaseMs * 0.001f;

        // Calculate coefficients: coeff = exp(-1.0 / (time * sampleRate))
        attackCoeff = std::exp(-1.0f / (attackSec * static_cast<float>(currentSampleRate)));
        releaseCoeff = std::exp(-1.0f / (releaseSec * static_cast<float>(currentSampleRate)));
    }

    // v1.6.0 fix: the enabled flags are now derived from the requested frequency on
    // every call, and only the *coefficients* are recomputed conditionally.
    //
    // The previous form set scHPFEnabled = true only inside the "frequency changed"
    // branch, so the case (freq > 0 && freq == currentFreq) fell through both branches
    // and left the flag at whatever it was. Any path that had disabled the filter left
    // it disabled even though the parameter asked for it — reachable with one knob
    // (set SC HPF to 100 Hz, down to Off, back to 100 Hz: silently stayed off) and hit
    // constantly when switching presets, since currentSCHPFFreq survives the switch.
    //
    // RT-safety: ArrayCoefficients returns a stack std::array with identical maths,
    // where Coefficients::makeHighPass heap-allocates a ref-counted object on the
    // audio thread.
    void updateSidechainFilters(float hpfFreq, float lpfFreq)
    {
        // HPF: 0 means off, otherwise 20-2000 Hz
        const bool wantHPF = hpfFreq > 0.0f;

        if (wantHPF && hpfFreq != currentSCHPFFreq)
        {
            // operator=(std::array) normalises the 6 raw values by a0 and stores the
            // resulting 5. Do NOT memcpy the array over getRawCoefficients() — the
            // stored form is 5 normalised values, not the 6 raw ones.
            *scHPF.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                currentSampleRate, hpfFreq, 0.707f); // Q = 0.707 (Butterworth)
            currentSCHPFFreq = hpfFreq;
        }

        scHPFEnabled = wantHPF;

        // LPF: 0 means off, and 20 kHz or above is treated as off too
        const bool wantLPF = lpfFreq > 0.0f && lpfFreq < 20000.0f;

        if (wantLPF && lpfFreq != currentSCLPFFreq)
        {
            *scLPF.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
                currentSampleRate, lpfFreq, 0.707f); // Q = 0.707 (Butterworth)
            currentSCLPFFreq = lpfFreq;
        }

        scLPFEnabled = wantLPF;
    }

    float applySidechainFilters(float input)
    {
        float filtered = input;

        // Apply HPF if enabled
        if (scHPFEnabled)
        {
            filtered = scHPF.processSample(filtered);
        }

        // Apply LPF if enabled
        if (scLPFEnabled)
        {
            filtered = scLPF.processSample(filtered);
        }

        return filtered;
    }

    double currentSampleRate = 44100.0;

    // DSP components
    EnvelopeDetector envelopeDetector;
    GainComputer gainComputer;

    // Sidechain filters (2nd order Butterworth)
    juce::dsp::IIR::Filter<float> scHPF;
    juce::dsp::IIR::Filter<float> scLPF;
    bool scHPFEnabled = false;
    bool scLPFEnabled = false;
    float currentSCHPFFreq = 0.0f;
    float currentSCLPFFreq = 0.0f;

    // Auto-makeup gain
    float averageGainReduction = 0.0f;
    float autoMakeupSmoothingCoeff = 0.0f;

    // Attack/release parameters
    float currentAttackMs = 10.0f;
    float currentReleaseMs = 100.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Smoothed gain reduction state
    float smoothedGainReductionDB = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compressor)
};
