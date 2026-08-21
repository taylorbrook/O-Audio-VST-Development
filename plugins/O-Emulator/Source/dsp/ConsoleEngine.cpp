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

//==============================================================================
// Pipeline

void ConsoleEngine::Pipeline::prepare (const ConsoleSpec& spec, int consoleIdx,
                                       double hostRate, int reportedLatencySamples,
                                       float initialDriveGain)
{
    consoleIndex = consoleIdx;

    const double hostPerConsole = hostRate / spec.rate;
    const double hostPerReverb  = hostRate / SpuReverb::kRate;

    // ── Latency alignment (plan decision #2 + Task 7) ────────────────────────
    // The REPORTED figure is the worst-case formula; this console's wet path
    // is structurally shorter, so the difference is made up by priming the
    // upsample ring with zeros (console-domain):
    //
    //   est = feeder + aaGD + downHoldback + (codecBlock + gaussHist)·host/console
    //   prime = round((reported − est) · console/host)
    const float aaCutoff = (float) (0.45 * spec.rate);

    const double aaGdHost = kButterworthGdSum * hostRate
                          / (juce::MathConstants<double>::twoPi * (double) aaCutoff);

    const double estHost = (double) FixedChunkFeeder::getLatencySamples()
                         + aaGdHost
                         + kDownHoldbackEstHost
                         + ((double) spec.codecBlockLen + kGaussHistoryEstConsole)
                               * hostPerConsole;

    const int prime = juce::jlimit (0, ConsoleResampler::kUpCap - 16,
                                    (int) std::lround (((double) reportedLatencySamples
                                                        - estHost)
                                                       / hostPerConsole));
    jassert (prime > 0);   // the worst-case formula exceeds every mode's structural delay

    resampler.prepare (hostRate, spec.rate, aaCutoff, prime);

    brr[0].reset();
    brr[1].reset();
    spu[0].reset();
    spu[1].reset();

    const juce::dsp::ProcessSpec ps { hostRate,
                                      (juce::uint32) FixedChunkFeeder::kChunk, 2u };
    outputStage.prepare (ps, spec.outputLpHz, spec.clip);

    // ── Reverb send/return alignment (L119) ──────────────────────────────────
    // The RETURN must join the direct path time-aligned at the sum point.
    // Direct-path delay from a decoded console sample to the sum:
    //   directHost = (gaussPrime + gaussHist) · host/console
    // Return-path structural terms: the send lerp history (~1 console sample)
    // and the return lerp history (~1 reverb sample); the remainder is the
    // return ring's priming (floored at 4 for production-jitter headroom).
    send.prepare (spec.rate);

    const double directHost  = ((double) prime + kGaussHistoryEstConsole) * hostPerConsole;
    const double sendHost    = 1.0 * hostPerConsole;
    const double retHistHost = 1.0 * hostPerReverb;

    const int retPrime = juce::jlimit (4, ReverbReturn::kCap - 16,
                                       (int) std::lround ((directHost - sendHost - retHistHost)
                                                          / hostPerReverb));
    ret.prepare (hostRate, retPrime);

    // Chunk-rate smoother: one step per fixed chunk keeps the trajectory a
    // pure function of the absolute chunk index (block-size invariance).
    driveGain.reset (hostRate / (double) FixedChunkFeeder::kChunk, 0.02);
    driveGain.setCurrentAndTargetValue (initialDriveGain);
}

void ConsoleEngine::Pipeline::reset (float initialDriveGain) noexcept
{
    resampler.reset();
    brr[0].reset();
    brr[1].reset();
    spu[0].reset();
    spu[1].reset();
    outputStage.reset();
    send.reset();
    ret.reset();
    driveGain.setCurrentAndTargetValue (initialDriveGain);
}

void ConsoleEngine::Pipeline::processChunk (float* inL, float* inR,
                                            float* outL, float* outR,
                                            float driveTargetGain, int shiftFloor,
                                            float sendGain, bool reverbActive,
                                            SpuReverb& reverb) noexcept
{
    // ── Control-chunk update (once per 32 host samples, never per host block:
    //    pattern_block_rate_envelope_breaks_blocksize_invariance) ───────────
    driveGain.setTargetValue (driveTargetGain);
    const float drive = driveGain.getNextValue();

    const bool ps1 = (consoleIndex == 1);

    if (ps1)
    {
        spu[0].setShiftFloor (shiftFloor);
        spu[1].setShiftFloor (shiftFloor);
    }
    else
    {
        brr[0].setShiftFloor (shiftFloor);
        brr[1].setShiftFloor (shiftFloor);
    }

    // ── AA + decimation (in place on the feeder's chunk buffers) ────────────
    const int nConsole = resampler.downsample (inL, inR, FixedChunkFeeder::kChunk,
                                               consoleBuf[0], consoleBuf[1],
                                               kConsoleCap);

    // ── Drive -> int16-rail clip -> codec round trip -> upsample queue.
    //    The reverb send tap is POST-CODEC in the console domain: the reverb
    //    hears the degraded signal, exactly as the SPU processed already-
    //    ADPCM'd voices (ARCHITECTURE Processing Order 4). ──────────────────
    for (int i = 0; i < nConsole; ++i)
    {
        const float xl = juce::jlimit (-1.0f, 1.0f, consoleBuf[0][i] * drive);
        const float xr = juce::jlimit (-1.0f, 1.0f, consoleBuf[1][i] * drive);

        float yl, yr;
        if (ps1)
        {
            yl = spu[0].processSample (xl);
            yr = spu[1].processSample (xr);
        }
        else
        {
            yl = brr[0].processSample (xl);
            yr = brr[1].processSample (xr);
        }

        resampler.pushConsoleSample (yl, yr);

        if (reverbActive)
            send.push (yl * sendGain, yr * sendGain,
                       [this, &reverb] (float sl, float sr)
                       {
                           float rl, rr;
                           reverb.processTick (sl, sr, rl, rr);
                           ret.push (rl, rr);
                       });
    }

    // ── Gaussian upsample back to host rate ─────────────────────────────────
    resampler.upsample (outL, outR, FixedChunkFeeder::kChunk);

    // ── Reverb return joins pre-output-stage, time-aligned via its primed
    //    ring (L119). Skipped entirely behind the inactive gate — an
    //    unconditional `+= 0.0f` would flip -0.0 samples and silently move
    //    the recorded digest anchors. ─────────────────────────────────────────
    if (reverbActive)
    {
        for (int i = 0; i < FixedChunkFeeder::kChunk; ++i)
        {
            float rl, rr;
            ret.pull (rl, rr);
            outL[i] += rl;
            outR[i] += rr;
        }
    }

    // ── Output stage: DAC LP + per-console clip, then the 10 Hz DC blocker.
    //    Phase 2.4's Age bed injects BETWEEN color and dcBlock. ─────────────
    for (int i = 0; i < FixedChunkFeeder::kChunk; ++i)
    {
        outL[i] = outputStage.processDcBlock (0, outputStage.processColor (0, outL[i]));
        outR[i] = outputStage.processDcBlock (1, outputStage.processColor (1, outR[i]));
    }

    outputStage.snapToZero();
}

//==============================================================================
// ConsoleEngine

void ConsoleEngine::prepare (double hostRate, int maxBlockSize, int reportedLatencySamples)
{
    juce::ignoreUnused (maxBlockSize);   // all buffers are chunk-sized members

    feeder.prepare();

    // Both pipelines pre-allocated and prepared — console switch never
    // allocates (ARCHITECTURE, PERF-01).
    pipelines[0].prepare (kConsoleSpecs[0], 0, hostRate, reportedLatencySamples,
                          currentDriveTargetGain (0));
    pipelines[1].prepare (kConsoleSpecs[1], 1, hostRate, reportedLatencySamples,
                          currentDriveTargetGain (1));

    reverb.reset();
    reverbEverActive = false;

    const double chunkRate = hostRate / (double) FixedChunkFeeder::kChunk;
    sendGainSmoothed.reset (chunkRate, 0.02);
    sendGainSmoothed.setCurrentAndTargetValue (
        juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));

    activePipeline = 0;
    pendingPipeline = 0;
}

void ConsoleEngine::reset()
{
    feeder.prepare();
    pipelines[0].reset (currentDriveTargetGain (0));
    pipelines[1].reset (currentDriveTargetGain (1));
    reverb.reset();
    reverbEverActive = false;
    sendGainSmoothed.setCurrentAndTargetValue (
        juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));
    activePipeline = pendingPipeline;
}

float ConsoleEngine::currentDriveTargetGain (int pipelineIndex) const noexcept
{
    const float db = (pipelineIndex == 1) ? CrushCurve::ps1DriveDb (crushTargetPct)
                                          : CrushCurve::snesDriveDb (crushTargetPct);
    return juce::Decibels::decibelsToGain (db);
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
    // ── Interim hard switch at the chunk boundary (Phase 2.3 replaces this
    //    with the 30 ms equal-power ConsoleCrossfader): incoming pipeline
    //    starts from reset state, reverb cleared. All reset paths are
    //    allocation-free. ─────────────────────────────────────────────────────
    if (pendingPipeline != activePipeline)
    {
        activePipeline = pendingPipeline;
        pipelines[activePipeline].reset (currentDriveTargetGain (activePipeline));
        reverb.reset();
    }

    Pipeline& p = pipelines[activePipeline];
    const bool ps1 = (p.consoleIndex == 1);

    // ── Per-chunk control latch (CrushCurve per-console rows) ───────────────
    const float driveDb = ps1 ? CrushCurve::ps1DriveDb (crushTargetPct)
                              : CrushCurve::snesDriveDb (crushTargetPct);
    const int shiftFloor = ps1 ? CrushCurve::ps1ShiftFloor (crushTargetPct)
                               : CrushCurve::snesShiftFloor (crushTargetPct);

    sendGainSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));
    const float sendGain = sendGainSmoothed.getNextValue();

    // Sticky activation gate: latched on the raw target so the smoother's
    // ramp starts from an armed reverb; never un-latches (the tail must keep
    // rendering after the send closes).
    if (reverbTargetPct > 0.0f)
        reverbEverActive = true;

    p.processChunk (inL, inR, outL, outR,
                    juce::Decibels::decibelsToGain (driveDb), shiftFloor,
                    sendGain, reverbEverActive, reverb);
}

} // namespace oemu
