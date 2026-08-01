/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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

    O-SpectralShaper - STFT Processor
    Ouaricon Development
    Developer: Taylor Brook

    Overlap-add STFT processing with 32-band spectral transient detection
    and per-band envelope shaping.

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

class STFTProcessor
{
public:
    // FFT configuration (fixed at 512 samples for all sample rates)
    static constexpr int FFT_ORDER = 9;
    static constexpr int FFT_SIZE = 512;
    static constexpr int HOP_SIZE = 256;  // 50% overlap
    static constexpr int NUM_BINS = FFT_SIZE / 2 + 1;  // 257 bins (0Hz to Nyquist)
    static constexpr int NUM_BANDS = 32;  // Logarithmic frequency bands

    STFTProcessor();

    // Lifecycle
    void prepare(double sampleRate);
    void reset();

    // Sample-by-sample processing interface
    float processSample(float input);

    // Transient detection output (read by processor for UI updates)
    const std::array<float, NUM_BANDS>& getTransientActivity() const { return transientActivity; }

    // Visualization data access (Phase 3.3)
    const std::array<float, NUM_BINS>& getLastMagnitudes() const { return lastMagnitudes; }

    // Parameter setters (called from processBlock)
    void setAttackCurve(const std::array<float, NUM_BANDS>& curve);
    void setSustainCurve(const std::array<float, NUM_BANDS>& curve);
    void setAttackTime(float ms);
    void setSustainTime(float ms);
    void setSensitivity(float value);

    // Bypass mode for null-test verification
    void setBypass(bool shouldBypass) { bypass = shouldBypass; }
    bool isBypassed() const { return bypass; }

private:
    // FFT engine
    juce::dsp::FFT forwardFFT { FFT_ORDER };
    juce::dsp::FFT inverseFFT { FFT_ORDER };

    // Window tables
    std::array<float, FFT_SIZE> windowTable {};           // Analysis (Hann)
    std::array<float, FFT_SIZE> synthesisWindowTable {};   // Synthesis (Hann with WOLA normalization)

    // Input/Output FIFOs for sample-by-sample interface
    std::array<float, FFT_SIZE> inputFIFO {};
    std::array<float, FFT_SIZE> outputFIFO {};
    std::array<float, FFT_SIZE * 2> fftData {};  // Complex data (real/imag interleaved)
    int fifoIndex = 0;

    // Band structure for transient detection
    struct BandBoundary {
        int startBin;
        int endBin;
    };
    std::array<BandBoundary, NUM_BANDS> bandBoundaries;

    struct Band {
        float prevMagnitude = 0.0f;
        float fastEnvelope = 0.0f;
        float slowEnvelope = 0.0f;
        float transientActivity = 0.0f;
        juce::SmoothedValue<float> gainSmoothed;
    };
    std::array<Band, NUM_BANDS> bands;

    // Transient activity output (for UI visualization)
    std::array<float, NUM_BANDS> transientActivity {};

    // FFT magnitude storage for visualization (Phase 3.3)
    std::array<float, NUM_BINS> lastMagnitudes {};

    // Parameters
    float sensitivity = 0.5f;
    float attackTimeMs = 10.0f;
    float sustainTimeMs = 100.0f;
    float sampleRate = 44100.0f;
    float fastCoeff = 0.0f;
    float slowCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Curves (double-buffered for thread-safe updates — separate indices)
    std::array<float, NUM_BANDS> attackCurve[2] {};
    std::array<float, NUM_BANDS> sustainCurve[2] {};
    std::atomic<int> activeAttackBuffer { 0 };
    std::atomic<int> activeSustainBuffer { 0 };

    // Maximum gain range for curve shaping (±12dB at curve extremes, matches UI grid labels)
    static constexpr float MAX_SHAPE_DB = 12.0f;

    // Bypass mode
    bool bypass = false;

    // Processing methods
    void processFrame();
    void setupBandBoundaries(double sampleRate);
    void detectTransients();
    void applyEnvelopeShaping();

    // Helper methods
    static float calculateEnvelopeCoefficient(float timeMs, float sr, float hopSize);
};
