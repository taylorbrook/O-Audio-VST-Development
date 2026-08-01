/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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

    O-simplePhysicalModelSynth - Visualization Analyzer (spectrum + oscilloscope)

    Stage 3 (GUI) — Phase 3.2. MESSAGE-THREAD ONLY: the FFT runs in the editor's
    30 Hz Timer, never on the audio thread (PERF-01 — the audio thread is copy-only
    into the VizTap ring). Ported from O-simpleFM's FmVizAnalyzer (the analyzer half
    only): this file holds NO ring — it reads `VizTap::waveform` (the global VizRing
    declared in VizTap.h) which Stage 2 already feeds.

    Output:
      - spectrum : 256 log-spaced dB bins (20 Hz → Nyquist). The log axis makes the
                   String harmonic comb read as an even ladder and the Modal modes
                   (f_k = f0·k·√(1+B·k²)) read as uneven spacing — for free.
      - scope    : 128 max-abs (sign-kept) points from a 1024-sample window.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "VizTap.h"          // reuse the global VizRing (do NOT redefine it here)
#include <array>
#include <vector>
#include <atomic>
#include <cmath>

//==============================================================================
class PmVizAnalyzer
{
public:
    static constexpr int kFftOrder     = 12;          // 4096 — separates discrete modes
    static constexpr int kFftSize      = 1 << kFftOrder;
    static constexpr int kScopeWindow  = 1024;        // raw samples pulled for the scope
    static constexpr int kScopePoints  = 128;         // downsampled display points
    static constexpr int kSpectrumBins = 256;         // log-frequency output bins

    PmVizAnalyzer()
        : fft (kFftOrder),
          window ((size_t) kFftSize, juce::dsp::WindowingFunction<float>::blackmanHarris)
    {
        spectrum.assign (kSpectrumBins, -100.0f);
        scope.assign    (kScopePoints, 0.0f);
    }

    // Message thread (editor Timer). Reads the ring, fills scope + spectrum.
    void process (const VizRing& ring, double sampleRate) noexcept
    {
        const double sr = (sampleRate > 0.0 ? sampleRate : 44100.0);   // host may not have prepared yet

        // Scope FIRST — performFrequencyOnlyForwardTransform clobbers its work
        // buffer in place, so the scope window must be copied before the FFT runs.
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
        const float maxHz = (float) (sr * 0.5);
        for (int b = 0; b < kSpectrumBins; ++b)
        {
            const float frac = (float) b / (float) (kSpectrumBins - 1);
            const float hz   = minHz * std::pow (maxHz / minHz, frac);   // log-frequency axis
            const int   bin  = juce::jlimit (0, kFftSize / 2 - 1,
                                             (int) std::round (hz * (float) kFftSize / (float) sr));
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
