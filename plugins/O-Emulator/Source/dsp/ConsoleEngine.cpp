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

#include <cstring>

namespace oemu
{

//==============================================================================
// Pipeline

void ConsoleEngine::Pipeline::prepare (const ConsoleSpec& spec, int consoleIdx,
                                       double hostRate, int reportedLatencySamples,
                                       float crushPct)
{
    consoleIndex = consoleIdx;

    const double hostPerConsole = hostRate / spec.rate;
    const double hostPerReverb  = hostRate / SpuReverb::kRate;

    // ── Latency alignment (plan decision #2 + Task 7) ────────────────────────
    // The REPORTED figure is the worst-case formula; this console's wet path
    // is structurally shorter, so the difference is made up by priming the
    // upsample ring with zeros (console-domain):
    //
    //   est = feeder + aaGD + downHoldback + (codecBlock + upsampleHist)·host/console
    //   prime = round((reported − est) · console/host)
    const float aaCutoff = (float) (0.45 * spec.rate);

    const double aaGdHost = kButterworthGdSum * hostRate
                          / (juce::MathConstants<double>::twoPi * (double) aaCutoff);

    const double upsampleHist =
        spec.upsample == ConsoleResampler::UpsampleMode::gaussian
            ? kGaussHistoryEstConsole
            : kZohHistoryEstConsole;

    const double estHost = (double) FixedChunkFeeder::getLatencySamples()
                         + aaGdHost
                         + kDownHoldbackEstHost
                         + ((double) spec.codecBlockLen + upsampleHist) * hostPerConsole;

    const int prime = juce::jlimit (0, ConsoleResampler::kUpCap - 16,
                                    (int) std::lround (((double) reportedLatencySamples
                                                        - estHost)
                                                       / hostPerConsole));
    jassert (prime > 0);   // the worst-case formula exceeds every mode's structural delay

    resampler.prepare (hostRate, spec.rate, aaCutoff, prime, spec.upsample);

    brr[0].reset();
    brr[1].reset();
    spu[0].reset();
    spu[1].reset();
    dpcm[0].prepare (spec.rate);
    dpcm[1].prepare (spec.rate);
    gbq[0].setLevels (16);
    gbq[1].setLevels (16);
    gen[0].prepare (spec.rate);
    gen[1].prepare (spec.rate);

    const juce::dsp::ProcessSpec ps { hostRate,
                                      (juce::uint32) FixedChunkFeeder::kChunk, 2u };
    outputStage.prepare (ps, spec.outputLpHz, spec.clip);

    // ── Reverb send/return alignment (L119) ──────────────────────────────────
    // The RETURN must join the direct path time-aligned at the sum point.
    // Direct-path delay from a decoded console sample to the sum:
    //   directHost = (prime + upsampleHist) · host/console
    // Return-path structural terms: the send lerp history (~1 console sample)
    // and the return lerp history (~1 reverb sample); the remainder is the
    // return ring's priming (floored at 4 for production-jitter headroom).
    send.prepare (spec.rate);

    const double directHost  = ((double) prime + upsampleHist) * hostPerConsole;
    const double sendHost    = 1.0 * hostPerConsole;
    const double retHistHost = 1.0 * hostPerReverb;

    const int retPrime = juce::jlimit (4, ReverbReturn::kCap - 16,
                                       (int) std::lround ((directHost - sendHost - retHistHost)
                                                          / hostPerReverb));
    ret.prepare (hostRate, retPrime);

    // Chunk-rate smoother: one step per fixed chunk keeps the trajectory a
    // pure function of the absolute chunk index (block-size invariance).
    driveGain.reset (hostRate / (double) FixedChunkFeeder::kChunk, 0.02);
    driveGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::driveDbFor (consoleIndex, crushPct)));
}

void ConsoleEngine::Pipeline::reset (float crushPct) noexcept
{
    resampler.reset();
    brr[0].reset();
    brr[1].reset();
    spu[0].reset();
    spu[1].reset();
    dpcm[0].reset();
    dpcm[1].reset();
    gen[0].reset();
    gen[1].reset();
    outputStage.reset();
    send.reset();
    ret.reset();
    driveGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::driveDbFor (consoleIndex, crushPct)));
}

void ConsoleEngine::Pipeline::processChunk (float* inL, float* inR,
                                            float* outL, float* outR,
                                            float crushPct, float sendGain,
                                            bool reverbActive,
                                            SpuReverb& reverb) noexcept
{
    // ── Control-chunk update (once per 32 host samples, never per host block:
    //    pattern_block_rate_envelope_breaks_blocksize_invariance) ───────────
    driveGain.setTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::driveDbFor (consoleIndex, crushPct)));
    const float drive = driveGain.getNextValue();

    // Per-console crush mapping (CrushCurve rows). Integer steps by design;
    // the 5 ms micro-fades land in Phase 2.4.
    switch (consoleIndex)
    {
        case 0:
        {
            const int shiftFloor = CrushCurve::snesShiftFloor (crushPct);
            brr[0].setShiftFloor (shiftFloor);
            brr[1].setShiftFloor (shiftFloor);
            break;
        }
        case 1:
        {
            const int shiftFloor = CrushCurve::ps1ShiftFloor (crushPct);
            spu[0].setShiftFloor (shiftFloor);
            spu[1].setShiftFloor (shiftFloor);
            break;
        }
        case 2:
        {
            const int idx = CrushCurve::nesRateIndex (crushPct);
            dpcm[0].setRateIndex (idx);
            dpcm[1].setRateIndex (idx);
            break;
        }
        case 3:
        {
            const int lv = CrushCurve::gbLevels (crushPct);
            gbq[0].setLevels (lv);
            gbq[1].setLevels (lv);
            break;
        }
        case 4:
        default:
        {
            const double hz = CrushCurve::genesisUpdateRateHz (crushPct);
            gen[0].setUpdateRateHz (hz);
            gen[1].setUpdateRateHz (hz);
            break;
        }
    }

    // ── AA + decimation (in place on the caller's chunk buffers) ────────────
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
        switch (consoleIndex)
        {
            case 0:
                yl = brr[0].processSample (xl);
                yr = brr[1].processSample (xr);
                break;
            case 1:
                yl = spu[0].processSample (xl);
                yr = spu[1].processSample (xr);
                break;
            case 2:
                yl = dpcm[0].processSample (xl);
                yr = dpcm[1].processSample (xr);
                break;
            case 3:
                yl = gbq[0].processSample (xl);
                yr = gbq[1].processSample (xr);
                break;
            case 4:
            default:
                yl = gen[0].processSample (xl);
                yr = gen[1].processSample (xr);
                break;
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

    // ── Upsample back to host rate (Gaussian or ZOH per console) ────────────
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

    // ── Output stage: DAC LP + per-console clip, then the 10 Hz DC blocker
    //    (structural NES-DPCM DC removal at the mixer boundary).
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

    // All five pipelines pre-allocated and prepared — console switch never
    // allocates (ARCHITECTURE, PERF-01).
    for (int c = 0; c < 5; ++c)
        pipelines[c].prepare (kConsoleSpecs[c], c, hostRate, reportedLatencySamples,
                              crushTargetPct);

    crossfader.prepare (hostRate);
    anyChunkProcessed = false;

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

    for (auto& p : pipelines)
        p.reset (crushTargetPct);

    crossfader.reset();
    anyChunkProcessed = false;
    reverb.reset();
    reverbEverActive = false;
    sendGainSmoothed.setCurrentAndTargetValue (
        juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));
    activePipeline = pendingPipeline;
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
    // ── Console switch, applied at the chunk boundary ───────────────────────
    if (pendingPipeline != activePipeline)
    {
        if (! anyChunkProcessed)
        {
            // First-chunk INSTANT switch: no audio has flowed, nothing to
            // crossfade — and this reproduces the Phase 2.2 chunk-0 hard
            // switch bit-exactly (reverb state is still all-zero, so the
            // 2.2-era reverb.reset() is a structural no-op), which is what
            // keeps the recorded 2.2 digest anchor valid.
            activePipeline = pendingPipeline;
            pipelines[activePipeline].reset (crushTargetPct);
        }
        else if (! crossfader.isFading())
        {
            // Mid-stream: 30 ms equal-power fade. Old renders through it,
            // new starts from reset; a request during a fade stays QUEUED
            // (pendingPipeline holds it) until the fade completes — only two
            // pipelines ever render concurrently. The reverb persists: its
            // tail carries across, fed/returned through the NEW pipeline.
            crossfader.begin (activePipeline);
            activePipeline = pendingPipeline;
            pipelines[activePipeline].reset (crushTargetPct);
        }
    }

    sendGainSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));
    const float sendGain = sendGainSmoothed.getNextValue();

    // Sticky activation gate: latched on the raw target so the smoother's
    // ramp starts from an armed reverb; never un-latches (the tail must keep
    // rendering after the send closes).
    if (reverbTargetPct > 0.0f)
        reverbEverActive = true;

    if (crossfader.isFading())
    {
        // Pristine input copy BEFORE the active pipeline AA-filters the
        // feeder buffers in place.
        std::memcpy (fadeIn[0], inL, sizeof (fadeIn[0]));
        std::memcpy (fadeIn[1], inR, sizeof (fadeIn[1]));

        pipelines[activePipeline].processChunk (inL, inR, outL, outR,
                                                crushTargetPct, sendGain,
                                                reverbEverActive, reverb);

        // The fading-out pipeline neither feeds nor pulls the reverb — the
        // single reverb instance belongs to the active pipeline.
        pipelines[crossfader.fadingIndex()].processChunk (fadeIn[0], fadeIn[1],
                                                          fadeOut[0], fadeOut[1],
                                                          crushTargetPct, 0.0f,
                                                          false, reverb);

        crossfader.mixChunk (outL, outR, fadeOut[0], fadeOut[1],
                             FixedChunkFeeder::kChunk);
    }
    else
    {
        pipelines[activePipeline].processChunk (inL, inR, outL, outR,
                                                crushTargetPct, sendGain,
                                                reverbEverActive, reverb);
    }

    anyChunkProcessed = true;
}

} // namespace oemu
