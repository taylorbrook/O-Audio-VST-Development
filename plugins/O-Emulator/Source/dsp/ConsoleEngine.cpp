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

#include "ConsoleEngine.h"

namespace oemu
{

void ConsoleEngine::prepare (double hostRate, int maxBlockSize, int reportedLatencySamples)
{
    juce::ignoreUnused (maxBlockSize);   // all buffers are chunk-sized members

    const ConsoleSpec& spec = kConsoleSpecs[0];   // SNES — only Phase 2.1 console

    feeder.prepare();

    // ── Latency alignment (plan decision #2 + Task 7) ────────────────────────
    // The REPORTED figure is the worst-case formula; the SNES wet path's
    // structural delay is shorter, so the difference is made up by priming
    // the upsample ring with zeros (console-domain). Estimate the structural
    // terms, convert the shortfall to console samples:
    //
    //   est = feeder + aaGD + downHoldback + (codecBlock + gaussHist)·host/console
    //   prime = round((reported − est) · console/host)
    //
    // Estimate error is a few samples; the harness latency probe budgets
    // ~15 samples of xcorr tolerance on top (L120).
    const double hostPerConsole = hostRate / spec.rate;

    const double estHost = (double) FixedChunkFeeder::getLatencySamples()
                         + kAaGroupDelayEstHost
                         + kDownHoldbackEstHost
                         + ((double) BrrCodec::kBlockLen + kGaussHistoryEstConsole)
                               * hostPerConsole;

    const int prime = juce::jlimit (0, ConsoleResampler::kUpCap - 16,
                                    (int) std::lround (((double) reportedLatencySamples
                                                        - estHost)
                                                       / hostPerConsole));
    jassert (prime > 0);   // the worst-case formula always exceeds SNES's structural delay

    const float aaCutoff = (float) (0.45 * spec.rate);
    resampler.prepare (hostRate, spec.rate, aaCutoff, prime);

    brr[0].reset();
    brr[1].reset();

    const juce::dsp::ProcessSpec ps { hostRate,
                                      (juce::uint32) FixedChunkFeeder::kChunk, 2u };
    outputStage.prepare (ps, spec.outputLpHz);

    // Chunk-rate smoother: one step per fixed chunk keeps the trajectory a
    // pure function of the absolute chunk index (block-size invariance).
    const double chunkRate = hostRate / (double) FixedChunkFeeder::kChunk;
    driveGain.reset (chunkRate, 0.02);
    driveGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::snesDriveDb (crushTargetPct)));
}

void ConsoleEngine::reset()
{
    feeder.prepare();
    resampler.reset();
    brr[0].reset();
    brr[1].reset();
    outputStage.reset();
    driveGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::snesDriveDb (crushTargetPct)));
}

void ConsoleEngine::process (juce::AudioBuffer<float>& buffer) noexcept
{
    jassert (buffer.getNumChannels() >= 2);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    feeder.process (l, r, buffer.getNumSamples(),
                    [this] (float* inL, float* inR, float* outL, float* outR)
                    {
                        processChunk (inL, inR, outL, outR);
                    });
}

void ConsoleEngine::processChunk (float* inL, float* inR,
                                  float* outL, float* outR) noexcept
{
    // ── Control-chunk update (once per 32 host samples, never per host block:
    //    pattern_block_rate_envelope_breaks_blocksize_invariance) ───────────
    driveGain.setTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::snesDriveDb (crushTargetPct)));
    const float drive = driveGain.getNextValue();

    const int shiftFloor = CrushCurve::snesShiftFloor (crushTargetPct);
    brr[0].setShiftFloor (shiftFloor);
    brr[1].setShiftFloor (shiftFloor);

    // ── AA + decimation (in place on the feeder's chunk buffers) ────────────
    const int nConsole = resampler.downsample (inL, inR, FixedChunkFeeder::kChunk,
                                               consoleBuf[0], consoleBuf[1],
                                               kConsoleCap);

    // ── Drive -> int16-rail clip -> BRR round trip -> upsample queue ────────
    // Codecs assume int16-domain input (ARCHITECTURE Processing Order 3);
    // the hard clip at the rails is the structural QUAL-01 blow-up guard.
    for (int i = 0; i < nConsole; ++i)
    {
        const float xl = juce::jlimit (-1.0f, 1.0f, consoleBuf[0][i] * drive);
        const float xr = juce::jlimit (-1.0f, 1.0f, consoleBuf[1][i] * drive);

        resampler.pushConsoleSample (brr[0].processSample (xl),
                                     brr[1].processSample (xr));
    }

    // ── Gaussian upsample back to host rate ─────────────────────────────────
    resampler.upsample (outL, outR, FixedChunkFeeder::kChunk);

    // ── Output stage: DAC LP + soft clip, then the 10 Hz DC blocker.
    //    Phase 2.4's Age bed injects BETWEEN color and dcBlock. ─────────────
    for (int i = 0; i < FixedChunkFeeder::kChunk; ++i)
    {
        outL[i] = outputStage.processDcBlock (0, outputStage.processColor (0, outL[i]));
        outR[i] = outputStage.processDcBlock (1, outputStage.processColor (1, outR[i]));
    }

    outputStage.snapToZero();
}

} // namespace oemu
