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

    Two detector topologies (v1.6.1, link corrected v1.9.0):
      - linked (all L/R paths, Mid-only, Side-only): each channel sidechain-filtered
        on its own state, rectified, then linked on the MAX into ONE detector
        (element [0]), one gain applied to every channel. (Pre-1.9.0 this averaged
        the SIGNED samples — a mono fold-down that under-read panned and
        decorrelated content and could not see anti-phase content at all.)
      - unlinked dual-mono (M/S "Both" mode): each channel gets its own detector,
        sidechain filters, ballistics and makeup state. Without this, averaging
        mid and side collapses the detector to (M+S)/2 = L/sqrt(2) — the left
        channel only — and a shared gain on M and S is algebraically identical
        to a shared gain on L and R, i.e. not M/S processing at all.

    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <algorithm>
#include <cmath>
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

        // Sidechain filter spec (mono per detector channel)
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1;

        for (int det = 0; det < 2; ++det)
        {
            envelopeDetector[det].prepare(sampleRate, maxBlockSize);
            smoothedGainReductionDB[det] = 0.0f;
            averageGainReduction[det] = 0.0f;
            smoothedMakeupDB[det] = 0.0f;
            makeupSnapPending[det] = true;

            scHPF[det].prepare(spec);
            scLPF[det].prepare(spec);
            scHPF[det].reset();
            scLPF[det].reset();

            // Seed both filters with a real biquad here, on the host thread. This is what
            // makes the later assignments RT-safe: Coefficients::operator=(std::array)
            // calls clearQuick() + ensureStorageAllocated(8) + add(), which only touches
            // the heap if the storage is not already big enough. Doing it once here means
            // every subsequent update on the audio thread reuses that allocation.
            *scHPF[det].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 1000.0f, 0.707f);
            *scLPF[det].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, 1000.0f, 0.707f);
        }

        // Update coefficients
        updateAttackReleaseCoefficients();

        // Force the first processed block to reconfigure from the live parameters
        // rather than trusting a cached frequency from a previous sample rate.
        currentSCHPFFreq = 0.0f;
        currentSCLPFFreq = 0.0f;
        scHPFEnabled = false;
        scLPFEnabled = false;

        autoMakeupSmoothingCoeff = 1.0f - std::exp(-1.0f / (0.5f * static_cast<float>(sampleRate))); // 500ms smoothing

        // v1.6.1 (WR-04): makeup gain is smoothed per sample (~15 ms one-pole) so a
        // Makeup knob turn no longer steps at block boundaries and an AUTO_MAKEUP
        // toggle no longer jumps several dB in one sample.
        makeupSmoothingCoeff = 1.0f - std::exp(-1.0f / (0.015f * static_cast<float>(sampleRate)));
    }

    void reset()
    {
        for (int det = 0; det < 2; ++det)
        {
            envelopeDetector[det].reset();
            smoothedGainReductionDB[det] = 0.0f;
            averageGainReduction[det] = 0.0f;
            smoothedMakeupDB[det] = 0.0f;
            makeupSnapPending[det] = true;
            scHPF[det].reset();
            scLPF[det].reset();
        }
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
    // filled; the silent channel must stay out of the sidechain filters and gain loop.
    // linkedDetector: true = stereo-linked (max of per-channel rectified sidechains
    // into one detector, one shared gain — v1.9.0); false = unlinked dual-mono, one
    // detector chain per channel (M/S "Both" mode — v1.6.1).
    void processStereo(juce::AudioBuffer<float>& buffer,
                      int numActiveChannels,
                      bool linkedDetector,
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
            if (linkedDetector)
            {
                // Stereo link (v1.9.0): filter each channel on its own sidechain state,
                // rectify, then link on the MAX of the rectified channels — one detector,
                // one shared gain. The previous signed average (L+R)/2 was a mono
                // fold-down of the sidechain: hard-panned content read −6 dB, decorrelated
                // stereo ~−3 dB, and anti-phase content was invisible to the detector.
                float linkedLevel = 0.0f;
                float filteredCh[2] = { 0.0f, 0.0f };
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    filteredCh[channel] = applySidechainFilters(channel, chanData[channel][sample]);
                    linkedLevel = std::max(linkedLevel, std::abs(filteredCh[channel]));
                }

                // Sidechain listen mode: each channel outputs its own filtered signal
                if (scListen)
                {
                    for (int channel = 0; channel < numChannels; ++channel)
                    {
                        chanData[channel][sample] = filteredCh[channel];
                    }
                    continue; // Skip compression processing in listen mode
                }

                const float totalGain = computeChannelGain(
                    0, linkedLevel, thresholdDB, ratio, kneeDB,
                    makeupDB, peakRmsBlend, autoMakeupEnabled);

                // Apply gain to all channels
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    chanData[channel][sample] *= totalGain;
                }
            }
            else
            {
                // Unlinked dual-mono: each channel has its own detector, sidechain
                // filter state, ballistics, and makeup state (v1.6.1, M/S "Both").
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    float filteredSidechain = applySidechainFilters(channel, chanData[channel][sample]);

                    if (scListen)
                    {
                        chanData[channel][sample] = filteredSidechain;
                        continue;
                    }

                    chanData[channel][sample] *= computeChannelGain(
                        channel, filteredSidechain, thresholdDB, ratio, kneeDB,
                        makeupDB, peakRmsBlend, autoMakeupEnabled);
                }
            }
        }

        // Update gain reduction meter (atomic for UI thread). Linked mode reports the
        // single shared detector; unlinked mode reports the deeper of the two channels.
        const float meterGR = linkedDetector
            ? smoothedGainReductionDB[0]
            : std::min(smoothedGainReductionDB[0], smoothedGainReductionDB[1]);
        gainReductionMeter.store(meterGR, std::memory_order_relaxed);
    }

private:
    // One detector channel's full gain path: envelope → gain computer → attack/release
    // ballistics → auto-makeup average → smoothed makeup → linear gain.
    float computeChannelGain(int det,
                             float filteredSidechain,
                             float thresholdDB,
                             float ratio,
                             float kneeDB,
                             float makeupDB,
                             float peakRmsBlend,
                             bool autoMakeupEnabled)
    {
        // Envelope detection (peak/RMS blend) on filtered sidechain
        float envelopeLevel = envelopeDetector[det].processSample(filteredSidechain, peakRmsBlend);

        // Gain computer (soft knee)
        float targetGainReductionDB = gainComputer.calculateGainReduction(
            envelopeLevel, thresholdDB, ratio, kneeDB);

        // Envelope smoothing (attack/release ballistics)
        float& grDB = smoothedGainReductionDB[det];
        if (targetGainReductionDB < grDB)
        {
            // Rising gain reduction (more compression) - use attack
            grDB += (targetGainReductionDB - grDB) * (1.0f - attackCoeff);
        }
        else
        {
            // Falling gain reduction (less compression) - use release
            grDB += (targetGainReductionDB - grDB) * (1.0f - releaseCoeff);
        }

        // Update auto-makeup calculation (running average of GR)
        averageGainReduction[det] += (grDB - averageGainReduction[det]) * autoMakeupSmoothingCoeff;

        // Calculate total makeup gain
        float autoMakeupDB = autoMakeupEnabled ? (-averageGainReduction[det] * 0.8f) : 0.0f;
        float totalMakeupDB = makeupDB + autoMakeupDB;

        // v1.6.1 (WR-04): one-pole smooth the makeup so knob steps and the AUTO_MAKEUP
        // toggle glide (~15 ms) instead of stepping. Snap on the first sample after
        // prepare/reset so a session never opens with a fade-in from 0 dB.
        float& smoothedDB = smoothedMakeupDB[det];
        if (makeupSnapPending[det])
        {
            smoothedDB = totalMakeupDB;
            makeupSnapPending[det] = false;
        }
        else
        {
            smoothedDB += (totalMakeupDB - smoothedDB) * makeupSmoothingCoeff;
        }

        // Convert to linear gain
        float gainReductionLinear = std::pow(10.0f, grDB / 20.0f);
        float makeupLinear = std::pow(10.0f, smoothedDB / 20.0f);
        return gainReductionLinear * makeupLinear;
    }

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
            const auto taps = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                currentSampleRate, hpfFreq, 0.707f); // Q = 0.707 (Butterworth)
            *scHPF[0].coefficients = taps;
            *scHPF[1].coefficients = taps;
            currentSCHPFFreq = hpfFreq;
        }

        scHPFEnabled = wantHPF;

        // LPF: 0 means off, and 20 kHz or above is treated as off too
        const bool wantLPF = lpfFreq > 0.0f && lpfFreq < 20000.0f;

        if (wantLPF && lpfFreq != currentSCLPFFreq)
        {
            const auto taps = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
                currentSampleRate, lpfFreq, 0.707f); // Q = 0.707 (Butterworth)
            *scLPF[0].coefficients = taps;
            *scLPF[1].coefficients = taps;
            currentSCLPFFreq = lpfFreq;
        }

        scLPFEnabled = wantLPF;
    }

    float applySidechainFilters(int det, float input)
    {
        float filtered = input;

        // Apply HPF if enabled
        if (scHPFEnabled)
        {
            filtered = scHPF[det].processSample(filtered);
        }

        // Apply LPF if enabled
        if (scLPFEnabled)
        {
            filtered = scLPF[det].processSample(filtered);
        }

        return filtered;
    }

    double currentSampleRate = 44100.0;

    // DSP components — element [0] is the linked/shared detector; element [1] is only
    // touched by the unlinked dual-mono path (M/S "Both").
    EnvelopeDetector envelopeDetector[2];
    GainComputer gainComputer;

    // Sidechain filters (2nd order Butterworth), one pair per detector channel.
    // Both pairs share the same coefficients; only the state differs.
    juce::dsp::IIR::Filter<float> scHPF[2];
    juce::dsp::IIR::Filter<float> scLPF[2];
    bool scHPFEnabled = false;
    bool scLPFEnabled = false;
    float currentSCHPFFreq = 0.0f;
    float currentSCLPFFreq = 0.0f;

    // Auto-makeup gain (per detector channel)
    float averageGainReduction[2] = { 0.0f, 0.0f };
    float autoMakeupSmoothingCoeff = 0.0f;

    // v1.6.1 (WR-04): smoothed makeup state (per detector channel)
    float smoothedMakeupDB[2] = { 0.0f, 0.0f };
    bool makeupSnapPending[2] = { true, true };
    float makeupSmoothingCoeff = 0.0f;

    // Attack/release parameters
    float currentAttackMs = 10.0f;
    float currentReleaseMs = 100.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Smoothed gain reduction state (per detector channel)
    float smoothedGainReductionDB[2] = { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compressor)
};
