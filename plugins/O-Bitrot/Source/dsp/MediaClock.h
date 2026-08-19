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

    O-Bitrot - MediaClock (Stage 2, Phase 2.1)

    Emits the "re-roll" tick that quantizes all transport-failure decisions
    (FUNC-01/03), as SAMPLE-ACCURATE offsets within the block — block-granular
    triggering is host-block-size dependent and fails QUAL-02 outright.

    Sync mode ports the EDGES of O-Polystutter's updateBeatSync (BPM clamp
    20-999, no-playhead => free-run fallback, wasPlaying guard, unconditional
    state commit at block end including early-outs) but replaces its
    block-granular boundary test with per-block interval math:

        ppqPerSample = bpm / (60 * fs)
        for each k*subdivPPQ in (ppqStart, ppqEnd]:
            offset = clamp(floor((k*subdivPPQ - ppqStart) / ppqPerSample), 0, N-1)

    std::floor throughout, NEVER static_cast<int> — int-cast truncates toward
    zero and breaks the boundary test around 0 during negative-PPQ pre-roll.

    Free mode: sample-counting phase accumulator at CLOCK_FREE_RATE Hz —
    a pure function of samples elapsed, so it is block-size invariant by
    construction. Every sync fallback reuses the same accumulator: missing
    playhead / position / PPQ, AND a host transport that is simply STOPPED
    (v1.2.1 — Sync is the default mode, so emitting nothing while parked made
    the plugin look broken during live audition).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MediaClock
{
public:
    // The only allocation. maxBlockSize bounds the tick-offset store.
    void prepare (double sampleRate, int maxBlockSize)
    {
        fs = sampleRate;
        offsets.assign (static_cast<size_t> (juce::jmax (64, maxBlockSize)), 0);
        reset();
    }

    void reset() noexcept
    {
        count      = 0;
        freePhase  = 0.0;
        wasPlaying = false;
    }

    // CLOCK_SYNC_DIV table: 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 bar.
    // "1 bar" derives from the time signature (4/4 fallback).
    static double subdivisionPPQ (int divIndex, double quartersPerBar) noexcept
    {
        switch (divIndex)
        {
            case 0: return 0.25;            // 1/16
            case 1: return 1.0 / 3.0;       // 1/8T
            case 2: return 0.5;             // 1/8
            case 3: return 2.0 / 3.0;       // 1/4T
            case 4: return 1.0;             // 1/4
            case 5: return 2.0;             // 1/2
            case 6: return quartersPerBar;  // 1 bar
            default: return 0.5;
        }
    }

    // Computes this block's tick offsets. mode: 0 = Sync, 1 = Free.
    // Allocation-free (offset store preallocated in prepare()).
    void processBlock (juce::AudioPlayHead* playHead, int numSamples,
                       int mode, int divIndex, double freeRateHz) noexcept
    {
        count = 0;

        if (numSamples <= 0)
            return;

        if (mode != 0)                              // Free mode
        {
            runFree (numSamples, freeRateHz);
            return;
        }

        // Sync mode. Missing playhead / position / PPQ => free-run fallback
        // (offline render, standalone, harness). Transport state is committed
        // on EVERY exit path.
        if (playHead == nullptr)
        {
            runFree (numSamples, freeRateHz);
            wasPlaying = false;
            return;
        }

        const auto pos = playHead->getPosition();

        if (! pos.hasValue())
        {
            runFree (numSamples, freeRateHz);
            wasPlaying = false;
            return;
        }

        const double bpm = pos->getBpm().hasValue()
                               ? juce::jlimit (20.0, 999.0, *pos->getBpm())
                               : 120.0;

        if (! pos->getPpqPosition().hasValue())
        {
            runFree (numSamples, freeRateHz);
            wasPlaying = pos->getIsPlaying();
            return;
        }

        const double ppqStart  = *pos->getPpqPosition();
        const bool   isPlaying = pos->getIsPlaying();

        // wasPlaying guard: the first block after transport start emits no
        // ticks (stale edge protection, O-Polystutter pattern).
        if (isPlaying && wasPlaying)
        {
            double quartersPerBar = 4.0;
            if (const auto ts = pos->getTimeSignature(); ts.hasValue() && ts->denominator > 0)
                quartersPerBar = 4.0 * static_cast<double> (ts->numerator)
                                     / static_cast<double> (ts->denominator);

            const double subdiv = subdivisionPPQ (divIndex, quartersPerBar);
            const double pps    = bpm / (60.0 * fs);
            const double ppqEnd = ppqStart + numSamples * pps;

            // First boundary strictly AFTER ppqStart (open interval start —
            // a boundary landing exactly on ppqStart belonged to the
            // previous block). std::floor handles negative pre-roll PPQ.
            double k = std::floor (ppqStart / subdiv) + 1.0;

            for (double t = k * subdiv; t <= ppqEnd; k += 1.0, t = k * subdiv)
            {
                if (t <= ppqStart)
                    continue;

                const int off = juce::jlimit (0, numSamples - 1,
                                              static_cast<int> (std::floor ((t - ppqStart) / pps)));
                push (off);
            }
        }
        else if (! isPlaying)
        {
            // Transport STOPPED: fall back to the free-run accumulator rather
            // than emitting nothing. Sync is the default mode, so without this
            // the plugin is pure passthrough whenever the host is parked —
            // auditioning live input reads as "the plugin is broken".
            // Playing behaviour above is untouched.
            runFree (numSamples, freeRateHz);
        }

        // Unconditional commit at block end. On the stopped->playing edge the
        // free accumulator is rewound so the NEXT stop starts from phase 0
        // instead of inheriting a partial period from the previous one.
        if (isPlaying && ! wasPlaying)
            freePhase = 0.0;

        wasPlaying = isPlaying;
    }

    int getNumTicks() const noexcept        { return count; }
    int getTickOffset (int i) const noexcept { return offsets[static_cast<size_t> (i)]; }

private:
    void runFree (int numSamples, double freeRateHz) noexcept
    {
        const double inc = juce::jlimit (0.05, 40.0, freeRateHz) / fs;

        for (int i = 0; i < numSamples; ++i)
        {
            freePhase += inc;
            if (freePhase >= 1.0)
            {
                freePhase -= 1.0;
                push (i);
            }
        }
    }

    void push (int offset) noexcept
    {
        if (count < static_cast<int> (offsets.size()))
            offsets[static_cast<size_t> (count++)] = offset;
    }

    std::vector<int> offsets;   // preallocated in prepare()
    int    count      = 0;
    double fs         = 48000.0;
    double freePhase  = 0.0;
    bool   wasPlaying = false;
};
