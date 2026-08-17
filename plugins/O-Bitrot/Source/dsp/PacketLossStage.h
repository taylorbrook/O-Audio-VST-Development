/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - PacketLossStage (Stage 2, Phase 2.3)

    Gilbert-Elliott bursty loss over a FIXED 20 ms packet grid (DSP-04),
    operating on the MediaPlayer output. The grid is sample-counted and
    deliberately independent of the MediaClock — burst statistics only hold
    on the packet grid (ARCHITECTURE decision).

    GE mapping (per block, from PACKET_LOSS / PACKET_BURST), v1.2 — the knob
    now spans clean -> true total failure:
        piB   = loss01 * 0.95                    (stationary Bad occupancy)
        E[B]  = 1 + burst01 * 7                  (expected burst, packets)
        pBG   = 1 / E[B]
        pGB   = clamp(piB * pBG / (1 - piB), 0, 1)
        pBad  = 0.5 + 0.5 * loss01               (loss prob in Bad)
        pGood = 0.01 * loss01 + t^2 * (0.90 - 0.01 * loss01),
                t = max(0, (loss01 - 0.75) / 0.25)
    The Good-state floor scales with loss01, so loss01 = 0 is loss-free
    (no uninvited dropouts at knob zero). The top-quartile pGood ramp is
    what makes full knob a TRUE >= ~95% loss: the Markov clamp caps Bad
    occupancy at 1/(1 + pBG) (~0.89 even at burst 100%), so near total
    failure the Good state must drop packets too.
    Advanced ONCE per packet (packet RNG stream): transition roll, then a
    loss roll. EXACTLY 2 draws per packet, unconditionally.

    Concealment (PACKET_CONCEAL, latched per burst):
        0 Silence     — hard dropout.
        1 Repeat      — previous good packet, verbatim (robotic).
        2 Decay       — pitch-aligned cyclic replay (same AMDF estimate as
                        Substitute) when the packet is periodic, else
                        packet-aligned replay; gain ramps -6 dB per
                        repetition per sample and hard-floors to SILENCE by
                        the end of the 3rd repetition (~60 ms), matching
                        real PLC mute-out.
        3 Substitute  — AMDF period estimate (2-15 ms lags, decimated) on the
                        last good packet; replay the last T samples cyclically
                        at -1 dB per cycle. Auto-degrades to Decay when the
                        AMDF minimum >= 0.5x its mean (no periodicity).
    Cyclic replay (Substitute, pitch-aligned Decay) crossfades each cycle
    joint with a ~1 ms raised-cosine OLA (tail -> head, gain step inside
    the fade); the resume index skips the pre-blended head samples so the
    period is preserved.

    Crossfades: 1-5 ms at every good<->lost transition unless HARD_EDGES; the
    outgoing source keeps rendering during the fade (live input, or the
    conceal machinery continuing). Repeat-mode restarts WITHIN a burst are
    intentionally hard — the machine-gun edge is the packet-repeat aesthetic
    (same spirit as the CD loop's hard repeats).

    DETERMINISM CONVENTION (documented invariant): the packet grid, the GE
    chain and the history capture run UNCONDITIONALLY — PACKET_ENABLE only
    gates whether the concealed signal is audible, through a ~10 ms
    EnableFade. Consequences:
      * PACKET_ENABLE off => the stage is bit-transparent (output untouched),
        so all pre-2.3 renders are unchanged (regression gate);
      * the loss schedule is a pure function of (seed, params, sample count) —
        toggling enable mid-render does not shift it;
      * same seed => bit-identical renders at any block size (QUAL-02).

    prepare() is the only allocation (2 x 20 ms per channel of history).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RngBank.h"
#include "Arbitration.h"   // EnableFade

class PacketLossStage
{
public:
    // UI telemetry only: current packet is lost or the GE chain is in a burst.
    bool isConcealing() const noexcept { return lostCur || stateBad; }

    // The only allocation.
    void prepare (double sampleRate, bool initiallyEnabled)
    {
        fs            = sampleRate;
        packetSamples = juce::jmax (16, (int) std::ceil (0.020 * fs));
        fadeLen       = juce::jmax (1, juce::jmin ((int) (0.003 * fs), packetSamples / 4));
        lagMin        = juce::jmax (2, (int) (0.002 * fs));
        lagMax        = juce::jmin (packetSamples - 2, (int) (0.015 * fs));

        for (auto& b : pkt)
        {
            b.setSize (2, packetSamples);
            b.clear();
        }

        enableFade.prepare (fs, initiallyEnabled);
        reset();
    }

    void reset() noexcept
    {
        for (auto& b : pkt)
            b.clear();

        counter    = 0;
        curIdx     = 0;
        goodIdx    = 1;
        stateBad   = false;
        lostCur    = false;
        burstLen   = 0;
        fadeRemain = 0;
        fadeFromConceal = false;
        concealMode   = 2;
        decayGain     = 1.0f;
        prevDecayGain = 1.0f;
        decayAligned  = false;
        subPeriod     = 0;
        subIdx        = 0;
        subGain       = 1.0f;
        olaLen        = 0;
    }

    // Per-block parameter snapshot (cached atomics read by the processor).
    void setParams (float loss01, float burst01, int concealChoice,
                    bool enabled, bool hardEdgesIn) noexcept
    {
        const double l   = juce::jlimit (0.0, 1.0, (double) loss01);
        const double piB = 0.95 * l;
        const double eB  = 1.0 + juce::jlimit (0.0, 1.0, (double) burst01) * 7.0;

        pBG = 1.0 / eB;
        pGB = juce::jlimit (0.0, 1.0, piB * pBG / (1.0 - piB));

        pLossBad = 0.5 + 0.5 * l;

        const double t = juce::jmax (0.0, (l - 0.75) / 0.25);
        pLossGood = 0.01 * l + t * t * (0.90 - 0.01 * l);

        concealChoiceParam = concealChoice;
        hardEdges          = hardEdgesIn;
        enableFade.setEnabled (enabled);
    }

    // Per-sample, stereo in place. RNG (packet stream) consumed ONLY at
    // packet boundaries.
    void processSample (RngBank& rng, float& left, float& right) noexcept
    {
        // 1. History capture — unconditional, so enabling mid-flight has
        //    fresh material.
        auto* cL = pkt[curIdx].getWritePointer (0);
        auto* cR = pkt[curIdx].getWritePointer (1);
        cL[counter] = left;
        cR[counter] = right;

        // 2. Concealment render (needed while lost, or while fading back out
        //    of a burst — the outgoing source keeps rendering).
        float concealL = 0.0f, concealR = 0.0f;
        if (lostCur || (fadeRemain > 0 && fadeFromConceal))
            renderConceal (concealL, concealR);

        // 3. Packet-stage output (before the enable fade).
        float pL, pR;
        if (lostCur)
        {
            pL = concealL;
            pR = concealR;

            if (fadeRemain > 0 && ! fadeFromConceal)
            {
                // good -> lost: fade live out, conceal in.
                const float t = 1.0f - (float) fadeRemain / (float) fadeLen;
                pL = concealL * t + left  * (1.0f - t);
                pR = concealR * t + right * (1.0f - t);
            }
        }
        else
        {
            pL = left;
            pR = right;

            if (fadeRemain > 0 && fadeFromConceal)
            {
                // lost -> good: fade conceal out, live in.
                const float t = 1.0f - (float) fadeRemain / (float) fadeLen;
                pL = left  * t + concealL * (1.0f - t);
                pR = right * t + concealR * (1.0f - t);
            }
        }

        if (fadeRemain > 0)
            --fadeRemain;

        // 4. Enable fade wrapper. Exact rails keep the bypass bit-exact.
        const float g = enableFade.next();
        if (g >= 1.0f)
        {
            left  = pL;
            right = pR;
        }
        else if (g > 0.0f)
        {
            left  = left  * (1.0f - g) + pL * g;
            right = right * (1.0f - g) + pR * g;
        }
        // g == 0: output untouched — bit-transparent.

        // 5. Grid advance + boundary (unconditional).
        if (++counter >= packetSamples)
        {
            counter = 0;
            onPacketBoundary (rng);
        }
    }

private:
    //==========================================================================
    void onPacketBoundary (RngBank& rng) noexcept
    {
        const bool prevLost = lostCur;

        // The just-finished packet becomes the reference when it was good.
        if (! prevLost)
        {
            goodIdx = curIdx;
            curIdx ^= 1;
        }

        // GE chain: EXACTLY 2 packet-stream draws per packet, always.
        const float rTrans = rng.get (RngBank::packet).nextFloat();
        const float rLoss  = rng.get (RngBank::packet).nextFloat();

        if (stateBad)
        {
            if (rTrans < (float) pBG)
                stateBad = false;
        }
        else
        {
            if (rTrans < (float) pGB)
                stateBad = true;
        }

        const bool lostNext = rLoss < (float) (stateBad ? pLossBad : pLossGood);

        if (lostNext)
        {
            if (! prevLost)
            {
                burstLen = 1;
                startBurst();
            }
            else
            {
                ++burstLen;
                if (concealMode == 2)
                {
                    // -6 dB per repetition (per-sample ramp toward the
                    // target), hard-floored to silence by the end of the
                    // 3rd repetition (~60 ms) — real PLC mutes out.
                    prevDecayGain = decayGain;
                    decayGain     = burstLen >= 3 ? 0.0f
                                                  : decayGain * kMinus6dB;
                }
            }
        }
        else
        {
            // Burst over: hold the decay ramp flat for the fade-out tail.
            if (prevLost && concealMode == 2)
                prevDecayGain = decayGain;

            burstLen = 0;
        }

        if (lostNext != prevLost && ! hardEdges)
        {
            fadeRemain      = fadeLen;
            fadeFromConceal = prevLost;
        }

        lostCur = lostNext;
    }

    void startBurst() noexcept
    {
        concealMode   = concealChoiceParam;
        prevDecayGain = 1.0f;                      // rep 1 ramps 0 -> -6 dB
        decayGain     = kMinus6dB;
        decayAligned  = false;
        subIdx        = 0;
        subGain       = 1.0f;
        olaLen        = 0;

        if (concealMode == 2 || concealMode == 3)
        {
            const int period = estimatePeriod();   // 0 = no periodicity

            if (concealMode == 3)
            {
                if (period > 0)
                    subPeriod = period;
                else
                    concealMode = 2;               // degrade: packet-aligned Decay
            }
            else if (period > 0)
            {
                subPeriod    = period;             // Decay, pitch-aligned
                decayAligned = true;
            }

            if (period > 0)
            {
                // ~1 ms raised-cosine OLA at cycle joints; capped at T/3 so
                // the resume index (olaLen) stays clear of the fade region.
                olaLen = juce::jmin ((int) (0.001 * fs), period / 3);
                if (olaLen < 2)
                    olaLen = 0;
            }
        }
    }

    // AMDF on the last good packet (mono sum), decimated: lag step 2,
    // up to 240 terms per lag — bounded, once per burst. Returns the best
    // lag, or 0 when the AMDF minimum >= 0.5x its mean (no periodicity).
    int estimatePeriod() const noexcept
    {
        const auto* gL = pkt[goodIdx].getReadPointer (0);
        const auto* gR = pkt[goodIdx].getReadPointer (1);

        double best = 1.0e30, sum = 0.0;
        int bestLag = 0, cnt = 0;

        for (int lag = lagMin; lag <= lagMax; lag += 2)
        {
            const int terms = juce::jmin (240, packetSamples - lag);
            double acc = 0.0;

            for (int i = 0; i < terms; ++i)
            {
                const int   na = packetSamples - 1 - i;
                const int   nb = na - lag;
                const float a  = 0.5f * (gL[na] + gR[na]);
                const float b  = 0.5f * (gL[nb] + gR[nb]);
                acc += std::abs ((double) a - (double) b);
            }

            acc /= (double) juce::jmax (1, terms);
            sum += acc;
            ++cnt;

            if (acc < best)
            {
                best    = acc;
                bestLag = lag;
            }
        }

        const double mean = sum / (double) juce::jmax (1, cnt);

        if (bestLag < lagMin || best >= 0.5 * mean)
            return 0;

        return bestLag;
    }

    void renderConceal (float& outL, float& outR) noexcept
    {
        const auto* gL = pkt[goodIdx].getReadPointer (0);
        const auto* gR = pkt[goodIdx].getReadPointer (1);

        switch (concealMode)
        {
            case 0:                                // Silence
                outL = 0.0f;
                outR = 0.0f;
                break;

            case 1:                                // Repeat (counter restarts per packet)
                outL = gL[counter];
                outR = gR[counter];
                break;

            case 3:                                // Substitute: cyclic pitch-period replay
            {
                // -1 dB per cycle; the step lands inside the joint OLA.
                cyclicSample (gL, gR, subGain, subGain * kMinus1dB, outL, outR);

                if (++subIdx >= subPeriod)
                {
                    subIdx = olaLen;               // head already blended in
                    subGain *= kMinus1dB;
                }
                break;
            }

            case 2:                                // Decay
            default:
            {
                // Per-sample ramp prev -> target across the packet; reaches
                // exact silence by the end of the 3rd repetition.
                const float g = prevDecayGain
                              + (decayGain - prevDecayGain)
                                    * (float) counter / (float) packetSamples;

                if (decayAligned)                  // pitch-aligned replay
                {
                    cyclicSample (gL, gR, g, g, outL, outR);

                    if (++subIdx >= subPeriod)
                        subIdx = olaLen;
                }
                else                               // aperiodic: packet-aligned
                {
                    outL = gL[counter] * g;
                    outR = gR[counter] * g;
                }
                break;
            }
        }
    }

    // One sample of cyclic replay of the last subPeriod samples of the good
    // packet, with a raised-cosine OLA at the cycle joint: the outgoing tail
    // (gain gTail) fades into the incoming head (gain gHead). Callers resume
    // the next cycle at subIdx = olaLen, so the period is preserved.
    void cyclicSample (const float* gL, const float* gR,
                       float gTail, float gHead,
                       float& outL, float& outR) const noexcept
    {
        const int s         = packetSamples - subPeriod;
        const int fadeStart = subPeriod - olaLen;

        if (olaLen > 0 && subIdx >= fadeStart)
        {
            const int   j = subIdx - fadeStart;
            const float w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::pi
                                                     * ((float) j + 0.5f) / (float) olaLen));

            outL = gL[s + subIdx] * gTail * (1.0f - w) + gL[s + j] * gHead * w;
            outR = gR[s + subIdx] * gTail * (1.0f - w) + gR[s + j] * gHead * w;
        }
        else
        {
            outL = gL[s + subIdx] * gTail;
            outR = gR[s + subIdx] * gTail;
        }
    }

    static constexpr float kMinus6dB = 0.50118723f;   // 10^(-6/20)
    static constexpr float kMinus1dB = 0.89125094f;   // 10^(-1/20)

    double fs            = 48000.0;
    int    packetSamples = 960;
    int    fadeLen       = 144;
    int    lagMin        = 96;
    int    lagMax        = 720;

    juce::AudioBuffer<float> pkt[2];   // [curIdx] capture, [goodIdx] last good
    int  counter = 0;
    int  curIdx  = 0;
    int  goodIdx = 1;

    // GE chain
    double pBG       = 1.0;
    double pGB       = 0.0;
    double pLossBad  = 0.5;
    double pLossGood = 0.0;
    bool   stateBad  = false;
    bool   lostCur   = false;
    int    burstLen  = 0;

    // Concealment state (latched per burst)
    int   concealChoiceParam = 2;
    int   concealMode        = 2;
    float decayGain          = 1.0f;   // ramp target for the current packet
    float prevDecayGain      = 1.0f;   // ramp start for the current packet
    bool  decayAligned       = false;  // Decay found an AMDF period
    int   subPeriod          = 0;
    int   subIdx             = 0;
    float subGain            = 1.0f;
    int   olaLen             = 0;      // cycle-joint OLA length, 0 = off

    // Boundary crossfade
    int  fadeRemain      = 0;
    bool fadeFromConceal = false;
    bool hardEdges       = false;

    EnableFade enableFade;
};
