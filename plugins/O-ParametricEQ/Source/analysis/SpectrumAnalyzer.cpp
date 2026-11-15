/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - spectrum analyzer, message side. See SpectrumAnalyzer.h.

  ==============================================================================
*/

#include "SpectrumAnalyzer.h"

#include "../PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kFallbackRate = 48000.0;
}

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer (OuariconParametricEQAudioProcessor& p)
    : processor (p)
{
    taps[0].tap = &processor.getPreTap();
    taps[1].tap = &processor.getPostTap();

    for (auto& t : taps)
        t.points.fill (floorDb);

    drainBuffer.assign (static_cast<std::size_t> (eq::SpectrumTap::capacity), 0.0f);

    processor.setAnalyzerConsumer (true);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    processor.setAnalyzerConsumer (false);
}

//==============================================================================
int SpectrumAnalyzer::fftSizeForRate (double sampleRateHz) noexcept
{
    if (sampleRateHz <= 48000.0)
        return 4096;

    if (sampleRateHz <= 96000.0)
        return 8192;

    return 16384;
}

double SpectrumAnalyzer::pointFrequency (int p) noexcept
{
    return minDisplayHz * std::pow (maxDisplayHz / minDisplayHz, static_cast<double> (p) / static_cast<double> (numPoints));
}

//==============================================================================
void SpectrumAnalyzer::prepare (double sampleRateHz, bool known)
{
    const int n = fftSizeForRate (sampleRateHz);
    const bool layoutChanged = (n != fftSize) || ! juce::exactlyEqual (sampleRateHz, preparedRate) || ! prepared;

    if (n != fftSize || fft == nullptr)
    {
        const int order = static_cast<int> (std::lround (std::log2 (static_cast<double> (n * zeroPadFactor))));
        fft = std::make_unique<juce::dsp::FFT> (order);
        jassert (fft->getSize() == n * zeroPadFactor);

        window.assign (static_cast<std::size_t> (n), 0.0);
        windowSum = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2.0 * kPi * static_cast<double> (i) / static_cast<double> (n));
            window[static_cast<std::size_t> (i)] = w;
            windowSum += w;
        }

        scratch.assign (static_cast<std::size_t> (2 * n * zeroPadFactor), 0.0f);
        frameDb.assign (static_cast<std::size_t> (2 * n + 1), floorDb);
    }

    fftSize = n;
    numBins = 2 * n + 1;
    preparedRate = sampleRateHz;
    rateKnown = known;
    prepared = true;
    ++prepareCount;

    for (auto& t : taps)
    {
        t.history.assign (static_cast<std::size_t> (n), 0.0f);
        t.valid = 0;
        t.skipNext = false;
        t.generation = t.tap->getGeneration();
        t.drops = t.tap->getDropCount();

        if (layoutChanged)
        {
            t.held.assign (static_cast<std::size_t> (numBins), floorDb);
            t.points.fill (floorDb);
        }
    }
}

//==============================================================================
int SpectrumAnalyzer::drainTap (TapState& t)
{
    std::uint32_t gen = 0;
    std::uint32_t drops = 0;

    // Everything, every time (maxN = capacity).
    const int count = t.tap->drain (drainBuffer.data(), eq::SpectrumTap::capacity, gen, drops);

    t.drained += static_cast<std::uint64_t> (count);

    if (gen != t.generation)
    {
        // A reset since the prepare: nothing of this drain is used, and the
        // next process call re-prepares.
        t.valid = 0;
        t.discarded += static_cast<std::uint64_t> (count);
        return 0;
    }

    if (drops != t.drops)
    {
        // A gap lies in or after this run: this run and the next are discarded.
        t.drops = drops;
        t.valid = 0;
        t.skipNext = true;
        t.discarded += static_cast<std::uint64_t> (count);
        return 0;
    }

    if (count == 0)
        return 0;

    if (t.skipNext)
    {
        t.skipNext = false;
        t.valid = 0;
        t.discarded += static_cast<std::uint64_t> (count);
        return 0;
    }

    // Shift in, newest last.
    const int n = fftSize;
    float* const h = t.history.data();

    if (count >= n)
    {
        std::memcpy (h, drainBuffer.data() + (count - n), sizeof (float) * static_cast<std::size_t> (n));
    }
    else
    {
        std::memmove (h, h + count, sizeof (float) * static_cast<std::size_t> (n - count));
        std::memcpy (h + (n - count), drainBuffer.data(), sizeof (float) * static_cast<std::size_t> (count));
    }

    t.valid = std::min (n, t.valid + count);
    return count;
}

void SpectrumAnalyzer::computeFrame (const TapState& t)
{
    const int n = fftSize;
    float* const s = scratch.data();

    for (int i = 0; i < n; ++i)
        s[i] = static_cast<float> (static_cast<double> (t.history[static_cast<std::size_t> (i)])
                                   * window[static_cast<std::size_t> (i)]);

    std::fill (s + n, s + scratch.size(), 0.0f);

    fft->performFrequencyOnlyForwardTransform (s);

    const double scale = 2.0 / windowSum;

    for (int k = 0; k < numBins; ++k)
    {
        const double magnitude = static_cast<double> (s[k]) * scale;
        double db = static_cast<double> (floorDb);

        if (magnitude > 0.0 && std::isfinite (magnitude))
            db = std::max (db, 20.0 * std::log10 (magnitude));

        frameDb[static_cast<std::size_t> (k)] = static_cast<float> (db);
    }
}

void SpectrumAnalyzer::reducePoints (TapState& t) const
{
    const double nyquist = 0.5 * preparedRate;
    const double binHz = preparedRate / static_cast<double> (fftSize * zeroPadFactor);
    const float* const held = t.held.data();

    for (int p = 0; p < numPoints; ++p)
    {
        const double f0 = pointFrequency (p);
        const double f1 = pointFrequency (p + 1);
        float value = floorDb;

        if (f0 < nyquist)
        {
            // The bins with f0 <= k binHz < f1.
            const int k0 = static_cast<int> (std::ceil (f0 / binHz));
            const int k1 = std::min (static_cast<int> (std::ceil (f1 / binHz)) - 1, numBins - 1);

            if (k0 <= k1)
            {
                value = held[k0];

                for (int k = k0 + 1; k <= k1; ++k)
                    value = std::max (value, held[k]);
            }
            else
            {
                // No bin in the span: the dB interpolation of the two bins
                // around its centre, linear in frequency.
                const double x = std::sqrt (f0 * f1) / binHz;
                const int k = static_cast<int> (std::floor (x));

                if (k + 1 < numBins)
                {
                    const double frac = x - static_cast<double> (k);
                    value = static_cast<float> ((1.0 - frac) * static_cast<double> (held[k])
                                                + frac * static_cast<double> (held[k + 1]));
                }
                else
                {
                    value = held[numBins - 1];
                }
            }
        }

        t.points[static_cast<std::size_t> (p)] = value;
    }
}

//==============================================================================
void SpectrumAnalyzer::process (double dtSeconds)
{
    const double dt = (std::isfinite (dtSeconds) && dtSeconds > 0.0) ? dtSeconds : 0.0;

    // 0. The rate first, then the generations (the processor publishes them
    // in the opposite order).
    const double rate = processor.getPreparedRate();
    const bool known = rate > 0.0;
    const double fs = known ? rate : kFallbackRate;

    bool needPrepare = ! prepared || ! juce::exactlyEqual (fs, preparedRate) || known != rateKnown;

    for (const auto& t : taps)
        needPrepare = needPrepare || t.tap->getGeneration() != t.generation;

    if (needPrepare)
        prepare (fs, known);

    const double fall = releaseDbPerSecond * dt;

    for (auto& t : taps)
    {
        // 1.
        const int appended = drainTap (t);

        // 2. and 3.
        float* const held = t.held.data();

        if (appended > 0 && t.valid >= fftSize)
        {
            computeFrame (t);
            ++t.frames;

            for (int k = 0; k < numBins; ++k)
            {
                const double released = std::max (static_cast<double> (held[k]) - fall, static_cast<double> (floorDb));
                held[k] = static_cast<float> (std::max (static_cast<double> (frameDb[static_cast<std::size_t> (k)]), released));
            }
        }
        else
        {
            for (int k = 0; k < numBins; ++k)
                held[k] = static_cast<float> (std::max (static_cast<double> (held[k]) - fall, static_cast<double> (floorDb)));
        }

        // 4.
        reducePoints (t);
    }
}
