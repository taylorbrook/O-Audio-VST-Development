/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

#include "ConsoleResampler.h"

#include <cstring>

namespace oemu
{

void ConsoleResampler::prepare (double hostRate, double consoleRate,
                                float aaCutoffHz, int primeConsoleSamples)
{
    jassert (hostRate > 0.0 && consoleRate > 0.0);

    ratio = hostRate / consoleRate;
    primeCount = juce::jlimit (0, kUpCap - 16, primeConsoleSamples);

    // Keep the AA corner strictly below the HOST Nyquist too (it already is
    // at every supported rate; this is belt-and-braces for exotic hosts).
    const float cutoff = juce::jmin (aaCutoffHz, (float) (0.45 * hostRate));

    // ArrayCoefficients: raw 6-element arrays, no heap churn beyond the one
    // prepare-time Coefficients object per stage (never on the audio thread —
    // Phase 2.4's AA-open steps between sets PRECOMPUTED here).
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int s = 0; s < 4; ++s)
        {
            const auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (
                               hostRate, cutoff, kButterworthQ8[s]);

            aa[ch][s].coefficients
                = new juce::dsp::IIR::Coefficients<float> (c[0], c[1], c[2],
                                                           c[3], c[4], c[5]);
            aa[ch][s].reset();
        }
    }

    GaussianInterpolator::warmTable();   // magic-static init OFF the audio thread

    reset();
}

void ConsoleResampler::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        for (auto& stage : aa[ch])
            stage.reset();

        lagrange[ch].reset();
        gauss[ch].reset();
    }

    std::memset (downFifo, 0, sizeof (downFifo));
    std::memset (upRing, 0, sizeof (upRing));
    downFill = 0;
    upWrite = 0;
    upRead = 0;
    upFill = 0;
    upPhase = 0.0;

    // Latency alignment: pre-queue zeros so the console stream reaches the
    // Gaussian exactly when the reported latency says it should.
    for (int i = 0; i < primeCount; ++i)
        pushConsoleSample (0.0f, 0.0f);
}

int ConsoleResampler::downsample (float* l, float* r, int numHost,
                                  float* outL, float* outR, int maxOut) noexcept
{
    jassert (numHost > 0 && numHost <= FixedChunkFeeder::kChunk);
    jassert (downFill + numHost <= kDownCap);

    float* io[2] = { l, r };

    for (int ch = 0; ch < 2; ++ch)
    {
        // Cascade the 4 AA biquads in place, stage by stage; per-sample
        // recursive paths get their denormal purge per chunk.
        for (auto& stage : aa[ch])
        {
            for (int i = 0; i < numHost; ++i)
                io[ch][i] = stage.processSample (io[ch][i]);

            stage.snapToZero();
        }

        std::memcpy (downFifo[ch] + downFill, io[ch],
                     (size_t) numHost * sizeof (float));
    }

    downFill += numHost;

    // Deterministic production count: a pure function of the fill, which is
    // itself a pure function of the absolute chunk index (fixed 32-sample
    // feed) — block-size invariance by construction. The −3 margin
    // guarantees the interpolator's ">= ratio · numOut samples available"
    // contract even with its ~1-sample over-read.
    int numOut = (int) (((double) downFill - 3.0) / ratio);
    numOut = juce::jlimit (0, maxOut, numOut);

    if (numOut == 0)
        return 0;

    float* outs[2] = { outL, outR };
    int consumed0 = 0;

    for (int ch = 0; ch < 2; ++ch)
    {
        downFifo[ch][downFill] = 0.0f;   // +1 cleared guard sample

        const int consumed = lagrange[ch].process (ratio, downFifo[ch],
                                                   outs[ch], numOut);

        if (ch == 0)
            consumed0 = consumed;
        else
            jassert (consumed == consumed0);   // identical streams, identical walk
    }

    // Advance the FIFO by the RETURNED consumed count (RESEARCH §1.1 — the
    // wrap-around overload returns a modulo position; this one returns a
    // genuine consumed count).
    jassert (consumed0 >= 0 && consumed0 <= downFill);
    consumed0 = juce::jlimit (0, downFill, consumed0);

    downFill -= consumed0;

    for (int ch = 0; ch < 2; ++ch)
        std::memmove (downFifo[ch], downFifo[ch] + consumed0,
                      (size_t) downFill * sizeof (float));

    return numOut;
}

void ConsoleResampler::pushConsoleSample (float l, float r) noexcept
{
    jassert (upFill < kUpCap);
    if (upFill >= kUpCap)
        return;   // defensive: drop rather than overwrite unread output

    upRing[0][upWrite] = l;
    upRing[1][upWrite] = r;
    upWrite = (upWrite + 1) & (kUpCap - 1);
    ++upFill;
}

void ConsoleResampler::upsample (float* outL, float* outR, int numHost) noexcept
{
    const double inc = 1.0 / ratio;   // console samples per host sample

    for (int i = 0; i < numHost; ++i)
    {
        upPhase += inc;

        while (upPhase >= 1.0)
        {
            upPhase -= 1.0;

            float sl = 0.0f, sr = 0.0f;

            if (upFill > 0)
            {
                sl = upRing[0][upRead];
                sr = upRing[1][upRead];
                upRead = (upRead + 1) & (kUpCap - 1);
                --upFill;
            }
            else
            {
                // Priming makes this unreachable; a zero substitute keeps the
                // stream bounded if the accounting is ever wrong.
                jassertfalse;
            }

            gauss[0].push (sl);
            gauss[1].push (sr);
        }

        outL[i] = gauss[0].interpolate (upPhase);
        outR[i] = gauss[1].interpolate (upPhase);
    }
}

} // namespace oemu
