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

ConsoleEngine::StepSet ConsoleEngine::Pipeline::computeSteps (float crushPct) const noexcept
{
    StepSet s;
    s.aaIndex = CrushCurve::aaOpenIndex (crushPct);

    switch (consoleIndex)
    {
        case 0:  s.codecStep = CrushCurve::snesShiftFloor (crushPct); break;
        case 1:  s.codecStep = CrushCurve::ps1ShiftFloor (crushPct); break;
        case 2:  s.codecStep = CrushCurve::nesRateIndex (crushPct); break;
        case 3:  s.codecStep = CrushCurve::gbLevels (crushPct); break;
        default: s.codecStep = 0; break;   // Genesis: rate is continuous, not a step
    }

    return s;
}

void ConsoleEngine::Pipeline::applySteps (const StepSet& s) noexcept
{
    switch (consoleIndex)
    {
        case 0:
            brr[0].setShiftFloor (s.codecStep);
            brr[1].setShiftFloor (s.codecStep);
            break;
        case 1:
            spu[0].setShiftFloor (s.codecStep);
            spu[1].setShiftFloor (s.codecStep);
            break;
        case 2:
            dpcm[0].setRateIndex (s.codecStep);
            dpcm[1].setRateIndex (s.codecStep);
            break;
        case 3:
            gbq[0].setLevels (s.codecStep);
            gbq[1].setLevels (s.codecStep);
            break;
        default:
            break;
    }

    resampler.setAaOpenIndex (s.aaIndex);
    applied = s;
}

void ConsoleEngine::Pipeline::prepare (const ConsoleSpec& spec, int consoleIdx,
                                       double hostRate, int reportedLatencySamples,
                                       float crushPct)
{
    consoleIndex = consoleIdx;
    baseLpHz = spec.outputLpHz;

    const double hostPerConsole = hostRate / spec.rate;
    const double hostPerReverb  = hostRate / SpuReverb::kRate;

    // ── Latency alignment: prime the upsample ring onto the reported figure.
    // Floored so at least ±kMinDriftOffsetHost host samples of drift offset
    // (plus 4 console samples of jitter) can never drain the ring — this
    // floor moved PS1's prime 4 -> 8 in Phase 2.4 (~+9 host samples of
    // structural delay at 48 kHz, still inside the xcorr probes' ±15). The
    // pipeline's ACTUAL drift rail (maxDriftOffsetHost below) is whatever
    // its final priming absorbs. ───────────────────────────────────────────
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

    const int primeAligned = (int) std::lround (((double) reportedLatencySamples - estHost)
                                                / hostPerConsole);

    const int primeFloor = (int) std::ceil (kMinDriftOffsetHost / hostPerConsole) + 4;

    const int prime = juce::jlimit (primeFloor, ConsoleResampler::kUpCap - 16,
                                    primeAligned);

    // What the priming actually absorbs = this pipeline's drift rail.
    maxDriftOffsetHost = juce::jlimit (2.0, 64.0,
                                       (double) (prime - 4) * hostPerConsole);

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
    send.prepare (spec.rate);

    const double directHost  = ((double) prime + upsampleHist) * hostPerConsole;
    const double sendHost    = 1.0 * hostPerConsole;
    const double retHistHost = 1.0 * hostPerReverb;

    const int retPrime = juce::jlimit (4, ReverbReturn::kCap - 16,
                                       (int) std::lround ((directHost - sendHost - retHistHost)
                                                          / hostPerReverb));
    ret.prepare (hostRate, retPrime);

    // ── Control smoothing + micro-fade constants ─────────────────────────────
    driveGain.reset (hostRate / (double) FixedChunkFeeder::kChunk, 0.02);
    driveGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::driveDbFor (consoleIndex, crushPct)));

    fadeChunks = (int) std::lround (0.005 * hostRate / (double) FixedChunkFeeder::kChunk);
    if ((fadeChunks & 1) != 0)
        ++fadeChunks;
    fadeChunks = juce::jmax (2, fadeChunks);
    fadeHalfChunks = fadeChunks / 2;
    fadePos = -1;

    applySteps (computeSteps (crushPct));
    pending = applied;
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
    outputStage.setLpCutoffHz (baseLpHz);
    send.reset();
    ret.reset();
    driveGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::driveDbFor (consoleIndex, crushPct)));

    fadePos = -1;
    applySteps (computeSteps (crushPct));   // immediate — nothing rendered yet
    pending = applied;
}

void ConsoleEngine::Pipeline::processChunk (float* inL, float* inR,
                                            float* outL, float* outR,
                                            float crushPct, float agePct,
                                            double driftFactor,
                                            float sendGain, bool reverbActive,
                                            SpuReverb& reverb) noexcept
{
    // ── Continuous controls (once per fixed chunk) ──────────────────────────
    driveGain.setTargetValue (
        juce::Decibels::decibelsToGain (CrushCurve::driveDbFor (consoleIndex, crushPct)));
    const float drive = driveGain.getNextValue();

    // Age dulling: corner × (1.0 -> 0.45); value-gated re-derive inside.
    outputStage.setLpCutoffHz (baseLpHz * (1.0f - 0.55f * agePct * 0.01f));

    // Genesis's DAC update rate is continuous (no integer step, no fade).
    if (consoleIndex == 4)
    {
        const double hz = CrushCurve::genesisUpdateRateHz (crushPct);
        gen[0].setUpdateRateHz (hz);
        gen[1].setUpdateRateHz (hz);
    }

    resampler.setDriftRatioFactor (driftFactor);

    // ── Integer steps: 5 ms equal-gain micro-fade state machine (Task 18).
    //    Fade down, apply EVERYTHING pending at the trough, fade up. ─────────
    const StepSet target = computeSteps (crushPct);

    if (fadePos < 0)
    {
        if (target != applied)
        {
            fadePos = 0;
            pending = target;
        }
    }
    else if (fadePos < fadeHalfChunks)
    {
        pending = target;                    // latch the latest until the trough
    }

    if (fadePos == fadeHalfChunks)
        applySteps (pending);                // gain ~0 here — the step is inaudible

    // ── AA + decimation (in place on the caller's chunk buffers) ────────────
    const int nConsole = resampler.downsample (inL, inR, FixedChunkFeeder::kChunk,
                                               consoleBuf[0], consoleBuf[1],
                                               kConsoleCap);

    // ── Drive -> int16-rail clip -> codec round trip -> upsample queue ──────
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

    // ── Reverb return joins pre-output-stage (exact-inactive gate) ──────────
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

    // ── Output stage: DAC LP (age-dulled) + per-console clip + DC blocker ───
    for (int i = 0; i < FixedChunkFeeder::kChunk; ++i)
    {
        outL[i] = outputStage.processDcBlock (0, outputStage.processColor (0, outL[i]));
        outR[i] = outputStage.processDcBlock (1, outputStage.processColor (1, outR[i]));
    }

    outputStage.snapToZero();

    // ── Micro-fade dip applied to the finished chunk (the engine's age bed
    //    is added AFTER this, so hiss/hum ride through the dip — authentic).
    //    Linear (equal-gain) V: 1 -> 0 over the first half, 0 -> 1 over the
    //    second; the trough coincides with applySteps above. ────────────────
    if (fadePos >= 0)
    {
        const int halfSamples = fadeHalfChunks * FixedChunkFeeder::kChunk;
        const int base = fadePos * FixedChunkFeeder::kChunk;

        for (int i = 0; i < FixedChunkFeeder::kChunk; ++i)
        {
            const int p = base + i;
            const float g = p < halfSamples
                                ? (float) (halfSamples - p) / (float) halfSamples
                                : (float) (p + 1 - halfSamples) / (float) halfSamples;
            outL[i] *= g;
            outR[i] *= g;
        }

        if (++fadePos >= fadeChunks)
        {
            fadePos = -1;

            // A change that arrived during the ramp-up starts the next fade
            // at the following chunk (computeSteps re-evaluates then).
        }
    }
}

//==============================================================================
// ConsoleEngine

void ConsoleEngine::prepare (double hostRate, int maxBlockSize, int reportedLatencySamples)
{
    juce::ignoreUnused (maxBlockSize);

    feeder.prepare();

    for (int c = 0; c < 5; ++c)
        pipelines[c].prepare (kConsoleSpecs[c], c, hostRate, reportedLatencySamples,
                              crushTargetPct);

    crossfader.prepare (hostRate);
    anyChunkProcessed = false;

    reverb.reset();
    reverbEverActive = false;

    ageModel.prepare (hostRate);

    const double chunkRate = hostRate / (double) FixedChunkFeeder::kChunk;
    sendGainSmoothed.reset (chunkRate, 0.02);
    sendGainSmoothed.setCurrentAndTargetValue (
        juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));

    ageSmoothed.reset (chunkRate, 0.02);
    ageSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 100.0f, ageTargetPct));

    // Drift: own stream, fixed seed, ~0.3 Hz smoothing at the chunk rate.
    driftRng = 0x1234ABCDu;
    driftWalk = 0.0;
    driftSlow = 0.0;
    driftSmoothCoeff = juce::jmin (1.0, juce::MathConstants<double>::twoPi * 0.3 / chunkRate);
    driftOffsetHost = 0.0;

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
    ageModel.reset();
    sendGainSmoothed.setCurrentAndTargetValue (
        juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));
    ageSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 100.0f, ageTargetPct));
    driftRng = 0x1234ABCDu;
    driftWalk = 0.0;
    driftSlow = 0.0;
    driftOffsetHost = 0.0;
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
    // ── Console switch (Task 14; first-chunk instant path) ──────────────────
    if (pendingPipeline != activePipeline)
    {
        if (! anyChunkProcessed)
        {
            activePipeline = pendingPipeline;
            pipelines[activePipeline].reset (crushTargetPct);
            driftOffsetHost = 0.0;
        }
        else if (! crossfader.isFading())
        {
            crossfader.begin (activePipeline);
            activePipeline = pendingPipeline;
            pipelines[activePipeline].reset (crushTargetPct);
            driftOffsetHost = 0.0;   // the new pipeline's rings start at prime
        }
    }

    // ── Chunk-rate control smoothing ────────────────────────────────────────
    sendGainSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, reverbTargetPct * 0.01f));
    const float sendGain = sendGainSmoothed.getNextValue();

    if (reverbTargetPct > 0.0f)
        reverbEverActive = true;

    ageSmoothed.setTargetValue (juce::jlimit (0.0f, 100.0f, ageTargetPct));
    const float ageVal = ageSmoothed.getNextValue();

    // ── Drift walk (Task 17): own stream, consumed UNCONDITIONALLY every
    //    chunk. Rails: the walk is dimensionless in [−1, 1] (exact at every
    //    rate); the accumulated time offset is clamped in host samples (a
    //    buffer bound, covered by the priming floor). Factor is EXACTLY 1.0
    //    at age 0, so drift-free renders take the bit-nominal path. ─────────
    {
        driftRng ^= driftRng << 13;
        driftRng ^= driftRng >> 17;
        driftRng ^= driftRng << 5;
        const double rnd = (double) driftRng / 2147483648.0 - 1.0;   // [−1, 1)

        driftWalk = juce::jlimit (-1.0, 1.0, driftWalk * 0.999 + rnd * 0.02);
        driftSlow += driftSmoothCoeff * (driftWalk - driftSlow);
    }

    const double cents = 15.0 * driftSlow * ((double) ageVal * 0.01);
    double driftFactor = cents != 0.0 ? std::exp2 (cents / 1200.0) : 1.0;

    double offsetDelta = (driftFactor - 1.0) * (double) FixedChunkFeeder::kChunk;
    if (std::abs (driftOffsetHost + offsetDelta)
        > pipelines[activePipeline].maxDriftOffsetHost)
    {
        // At the rail: hold the offset and push the walk back toward centre.
        driftFactor = 1.0;
        offsetDelta = 0.0;
        driftWalk *= -0.5;
        driftSlow *= -0.5;
    }
    driftOffsetHost += offsetDelta;

    // ── Render (single pipeline, or two during a crossfade) ─────────────────
    if (crossfader.isFading())
    {
        std::memcpy (fadeIn[0], inL, sizeof (fadeIn[0]));
        std::memcpy (fadeIn[1], inR, sizeof (fadeIn[1]));

        pipelines[activePipeline].processChunk (inL, inR, outL, outR,
                                                crushTargetPct, ageVal, driftFactor,
                                                sendGain, reverbEverActive, reverb);

        pipelines[crossfader.fadingIndex()].processChunk (fadeIn[0], fadeIn[1],
                                                          fadeOut[0], fadeOut[1],
                                                          crushTargetPct, ageVal, 1.0,
                                                          0.0f, false, reverb);

        crossfader.mixChunk (outL, outR, fadeOut[0], fadeOut[1],
                             FixedChunkFeeder::kChunk);
    }
    else
    {
        pipelines[activePipeline].processChunk (inL, inR, outL, outR,
                                                crushTargetPct, ageVal, driftFactor,
                                                sendGain, reverbEverActive, reverb);
    }

    // ── Age bed (Task 16): ONE bed, engine level, wet path only — hiss/hum
    //    ride on top of whatever the pipeline(s) produced. RNG streams
    //    advance unconditionally inside. ──────────────────────────────────────
    ageModel.processChunk (outL, outR, FixedChunkFeeder::kChunk, ageVal);

    anyChunkProcessed = true;
}

} // namespace oemu
