/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain - Visualization Tap (real-time-safe spectrum + oscilloscope)

    PERF-01: the audio thread is COPY-ONLY into a pre-allocated lock-free ring;
    NO allocation, NO FFT, NO locks on the audio thread. The FFT runs on the
    message thread (editor Timer, 30 Hz).

    Two pieces:
      - VizRing          : audio-thread writer / message-thread reader ring.
      - GrainVizAnalyzer : message-thread FFT (4096 / Blackman-Harris) + scope
                           downsampler. Owned by the Stage-3 editor; Stage 3
                           emits its output to the WebView (UI-04), Stage 2.3
                           just provides the audio-thread ring write at the
                           processBlock tail + the public accessors.

    Phase 2.3: copied VERBATIM from O-simpleFM (Source/FmVizAnalyzer.h). Ring +
    FFT logic UNCHANGED — only the analyzer type was renamed FmVizAnalyzer ->
    GrainVizAnalyzer (all behavior identical). The audio thread only WRITES
    samples to the ring; the FFT is the Stage-3 editor Timer's job.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include <cmath>

//==============================================================================
// Lock-free overwrite ring. Power-of-two size + bitmask wrap. Fully allocated
// at construction — nothing to prepare.
class VizRing
{
public:
    static constexpr int kSize = 8192;           // >= FFT 4096 + scope window
    static_assert ((kSize & (kSize - 1)) == 0, "VizRing size must be power of two");

    void write (const float* data, int n) noexcept            // AUDIO THREAD
    {
        int w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            buffer[(size_t) w].store (data[i], std::memory_order_relaxed);
            w = (w + 1) & (kSize - 1);
        }
        writePos.store (w, std::memory_order_release);
    }

    void readLatest (float* dest, int n) const noexcept        // MESSAGE THREAD
    {
        const int w = writePos.load (std::memory_order_acquire);
        int start = (w - n) & (kSize - 1);
        for (int i = 0; i < n; ++i)
            dest[i] = buffer[(size_t) ((start + i) & (kSize - 1))].load (std::memory_order_relaxed);
    }

private:
    std::array<std::atomic<float>, (size_t) kSize> buffer {};
    std::atomic<int> writePos { 0 };
};

//==============================================================================
class GrainVizAnalyzer
{
public:
    static constexpr int kFftOrder    = 12;          // 4096 — separates discrete sidebands
    static constexpr int kFftSize     = 1 << kFftOrder;
    static constexpr int kScopeWindow = 1024;        // raw samples pulled for the scope
    static constexpr int kScopePoints = 128;         // downsampled display points
    static constexpr int kSpectrumBins = 256;        // log-frequency output bins

    GrainVizAnalyzer()
        : fft (kFftOrder),
          window ((size_t) kFftSize, juce::dsp::WindowingFunction<float>::blackmanHarris)
    {
        spectrum.assign (kSpectrumBins, -100.0f);
        scope.assign    (kScopePoints, 0.0f);
    }

    // Message thread (editor Timer). Reads the ring, fills scope + spectrum.
    void process (const VizRing& ring, double sampleRate) noexcept
    {
        // Scope FIRST — performFrequencyOnlyForwardTransform clobbers its buffer
        // in place, so the scope window must be copied before the FFT runs.
        ring.readLatest (scopeRaw.data(), kScopeWindow);
        const int per = kScopeWindow / kScopePoints;
        for (int i = 0; i < kScopePoints; ++i)
        {
            float m = 0.0f;
            for (int j = 0; j < per; ++j)
            {
                const float s = scopeRaw[(size_t) (i * per + j)];
                if (std::abs (s) > std::abs (m)) m = s;       // max-abs, keep sign
            }
            scope[(size_t) i] = juce::jlimit (-1.0f, 1.0f, m);
        }

        // Spectrum.
        ring.readLatest (work.data(), kFftSize);
        window.multiplyWithWindowingTable (work.data(), (size_t) kFftSize);
        fft.performFrequencyOnlyForwardTransform (work.data());   // magnitudes in [0 .. size/2]

        const float minHz = 20.0f;
        const float maxHz = (float) (sampleRate * 0.5);
        for (int b = 0; b < kSpectrumBins; ++b)
        {
            const float frac = (float) b / (float) (kSpectrumBins - 1);
            const float hz   = minHz * std::pow (maxHz / minHz, frac);   // log-frequency axis
            const int   bin  = juce::jlimit (0, kFftSize / 2 - 1,
                                             (int) std::round (hz * (float) kFftSize / (float) sampleRate));
            const float mag  = work[(size_t) bin] / (float) kFftSize;
            const float db   = juce::Decibels::gainToDecibels (mag + 1.0e-9f, -100.0f);

            const float prev  = spectrum[(size_t) b];
            const float coeff = (db > prev) ? 0.5f : 0.1f;             // rise fast, fall slow
            spectrum[(size_t) b] = prev + coeff * (db - prev);
        }

        frameCount.fetch_add (1, std::memory_order_relaxed);
    }

    const std::vector<float>& getSpectrum() const noexcept { return spectrum; }
    const std::vector<float>& getScope()    const noexcept { return scope; }
    int  getFrameCount() const noexcept { return frameCount.load (std::memory_order_relaxed); }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, (size_t) kFftSize * 2> work {};
    std::array<float, (size_t) kScopeWindow> scopeRaw {};
    std::vector<float> spectrum, scope;
    std::atomic<int> frameCount { 0 };
};
