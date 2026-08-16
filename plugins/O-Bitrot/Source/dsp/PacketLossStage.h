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

    GE mapping (per block, from PACKET_LOSS / PACKET_BURST):
        piB  = loss01 * 0.6                      (stationary Bad occupancy)
        E[B] = 1 + burst01 * 7                   (expected burst, packets)
        pBG  = 1 / E[B]
        pGB  = clamp(piB * pBG / (1 - piB), 0, 1)
    Advanced ONCE per packet (packet RNG stream): transition roll, then a
    loss roll — lose with 0.5 in Bad, 0.01 in Good. EXACTLY 2 draws per
    packet, unconditionally.

    Concealment (PACKET_CONCEAL, latched per burst):
        0 Silence     — hard dropout.
        1 Repeat      — previous good packet, verbatim (robotic).
        2 Decay       — repeat at -3 dB per repetition (fades over ~60 ms).
        3 Substitute  — AMDF period estimate (2-15 ms lags, decimated) on the
                        last good packet; replay the last T samples cyclically
                        at -1 dB per cycle. Auto-degrades to Decay when the
                        AMDF minimum >= 0.5x its mean (no periodicity).

    Crossfades: 1-5 ms at every good<->lost transition unless HARD_EDGES; the
    outgoing source keeps rendering during the fade (live input, or the
    conceal machinery continuing). Repetition restarts WITHIN a burst are
    intentionally hard — the machine-gun edge is the packet-repeat aesthetic
    (same spirit as the CD loop's hard repeats); Substitute is
    period-continuous by construction.

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
        concealMode = 2;
        decayGain   = 1.0f;
        subPeriod   = 0;
        subIdx      = 0;
        subGain     = 1.0f;
    }

    // Per-block parameter snapshot (cached atomics read by the processor).
    void setParams (float loss01, float burst01, int concealChoice,
                    bool enabled, bool hardEdgesIn) noexcept
    {
        const double piB = juce::jlimit (0.0, 0.6, (double) loss01 * 0.6);
        const double eB  = 1.0 + juce::jlimit (0.0, 1.0, (double) burst01) * 7.0;

        pBG = 1.0 / eB;
        pGB = juce::jlimit (0.0, 1.0, piB * pBG / (1.0 - piB));

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

        const bool lostNext = rLoss < (stateBad ? 0.5f : 0.01f);

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
                    decayGain *= kMinus3dB;        // -3 dB per repetition
            }
        }
        else
        {
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
        concealMode = concealChoiceParam;
        decayGain   = kMinus3dB;                   // first repetition at -3 dB
        subIdx      = 0;
        subGain     = 1.0f;

        if (concealMode == 3)
        {
            // AMDF on the last good packet (mono sum), decimated: lag step 2,
            // up to 240 terms per lag — bounded, once per burst.
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
                concealMode = 2;                   // no periodicity: degrade to Decay
            else
                subPeriod = bestLag;
        }
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
                const int idx = packetSamples - subPeriod + subIdx;
                outL = gL[idx] * subGain;
                outR = gR[idx] * subGain;

                if (++subIdx >= subPeriod)
                {
                    subIdx = 0;
                    subGain *= kMinus1dB;          // -1 dB per cycle
                }
                break;
            }

            case 2:                                // Decay
            default:
                outL = gL[counter] * decayGain;
                outR = gR[counter] * decayGain;
                break;
        }
    }

    static constexpr float kMinus3dB = 0.70794578f;   // 10^(-3/20)
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
    double pBG      = 1.0;
    double pGB      = 0.0;
    bool   stateBad = false;
    bool   lostCur  = false;
    int    burstLen = 0;

    // Concealment state (latched per burst)
    int   concealChoiceParam = 2;
    int   concealMode        = 2;
    float decayGain          = 1.0f;
    int   subPeriod          = 0;
    int   subIdx             = 0;
    float subGain            = 1.0f;

    // Boundary crossfade
    int  fadeRemain      = 0;
    bool fadeFromConceal = false;
    bool hardEdges       = false;

    EnableFade enableFade;
};
