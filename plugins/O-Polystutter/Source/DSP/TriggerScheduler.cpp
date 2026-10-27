/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
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

    TriggerScheduler.cpp
    Ouaricon Polystutter - Sample-accurate trigger scheduling

  ==============================================================================
*/

#include "TriggerScheduler.h"
#include "RepeatLane.h"

namespace
{
    // A boundary within this many samples of a sample instant lands ON it, so
    // block segmentations whose block-start ppq values differ by rounding agree
    constexpr double kSampleEpsilon = 1.0e-6;
}

void TriggerScheduler::reset()
{
    wasPlaying = false;
    lastPPQPosition = 0.0;
    for (int lane = 0; lane < kNumLanes; ++lane)
    {
        numEvents[lane] = 0;
        lastFiredBoundary[lane] = LLONG_MIN;
    }
}

double TriggerScheduler::getSubdivisionPPQ(int subdivIndex)
{
    switch (subdivIndex)
    {
        case 0: return 1.0;       // 1/4 note
        case 1: return 0.5;       // 1/8 note
        case 2: return 0.25;      // 1/16 note
        case 3: return 0.125;     // 1/32 note
        case 4: return 1.0 / 3.0; // 1/8T (triplet)
        case 5: return 1.0 / 6.0; // 1/16T (triplet)
        default: return 0.25;     // Default 1/16
    }
}

void TriggerScheduler::add(int lane, const Event& e)
{
    int& n = numEvents[lane];
    if (n >= kMaxEventsPerLane)
        return;

    // Stable insertion by (offset, kind) — lists are a handful of events
    int i = n++;
    while (i > 0 && (events[lane][i - 1].offset > e.offset
                     || (events[lane][i - 1].offset == e.offset && events[lane][i - 1].kind > e.kind)))
    {
        events[lane][i] = events[lane][i - 1];
        --i;
    }
    events[lane][i] = e;
}

void TriggerScheduler::schedule(const BlockInfo& info, const juce::MidiBuffer& midi)
{
    for (int lane = 0; lane < kNumLanes; ++lane)
        numEvents[lane] = 0;

    const int numSamples = info.numSamples;
    if (numSamples <= 0)
        return;

    // Quarter notes per sample while the transport runs
    const double ppqPerSample = info.bpm / (60.0 * juce::jmax(1.0, info.sampleRate));
    const double ppqAdvance = info.isPlaying ? ppqPerSample : 0.0;

    // ========== MIDI note-ons at their own sample ==========
    // Notes 60-63 (C3-D#3, middle C = C3) → lanes 1-4, 67 (G3) → all lanes.
    // Every note-on is its own event, so a chord fires every lane it names.
    // The `midi` tooltip in ui/public/js/i18n.js names these same notes.
    if (info.midiEnabled)
    {
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (!message.isNoteOn())
                continue;

            const int note = message.getNoteNumber();
            const bool allLanes = note == 67;
            if (!allLanes && (note < 60 || note > 63))
                continue;

            Event e;
            e.offset = juce::jlimit(0, numSamples - 1, metadata.samplePosition);
            e.kind = Kind::Midi;
            e.requiresIdle = true;
            if (info.hasPpq)
            {
                e.step = Step::FromPpq;
                e.ppq = info.ppq + e.offset * ppqAdvance;
            }

            for (int lane = 0; lane < kNumLanes; ++lane)
                if ((allLanes || lane == note - 60) && info.laneEnabled[lane])
                    add(lane, e);
        }
    }

    // ========== Manual button: block start ==========
    if (info.manualEdge)
    {
        Event e;
        e.kind = Kind::Manual;
        if (info.hasPpq)
        {
            e.step = Step::FromPpq;
            e.ppq = info.ppq;
        }

        for (int lane = 0; lane < kNumLanes; ++lane)
            if (info.laneEnabled[lane])
                add(lane, e);
    }

    // ========== Beat-sync boundaries at their own sample ==========
    if (!info.hasPpq)
    {
        // No musical position: nothing to sync to, and no reference for the next block
        wasPlaying = false;
        return;
    }

    const double ppq = info.ppq;

    // A transport start or a backward jump (DAW loop wrap, relocate) restarts
    // the boundary count, so the boundary the playhead lands on fires (WR-04)
    const bool fresh = !wasPlaying || ppq < lastPPQPosition;

    if (info.isPlaying && !info.midiEnabled)
    {
        for (int lane = 0; lane < kNumLanes; ++lane)
        {
            if (fresh)
                lastFiredBoundary[lane] = LLONG_MIN;

            if (!info.laneEnabled[lane])
                continue;

            const double subdivPPQ = getSubdivisionPPQ(info.laneSubdiv[lane]);

            // Boundary k sits at ppq k * subdivPPQ and fires on the first
            // sample at or after it: n = ceil((k * subdivPPQ - ppq) / ppqPerSample).
            // Start one boundary early — a boundary less than a sample before
            // the block start rounds to sample 0 (the previous block saw it at
            // n == its length and left it). floor() keeps negative ppq right.
            long long k = static_cast<long long>(std::floor((ppq - ppqPerSample) / subdivPPQ));
            for (int guard = 0; guard < kMaxEventsPerLane + 2; ++guard, ++k)
            {
                const double samplesToBoundary = (static_cast<double>(k) * subdivPPQ - ppq) / ppqPerSample;
                const double n = std::ceil(samplesToBoundary - kSampleEpsilon);

                if (n >= numSamples)
                    break;
                if (n < 0.0 || k <= lastFiredBoundary[lane])
                    continue;

                Event e;
                e.offset = static_cast<int>(n);
                e.kind = Kind::Beat;
                e.step = Step::FromIndex;
                e.boundary = k;
                add(lane, e);
                lastFiredBoundary[lane] = k;
            }
        }
    }

    lastPPQPosition = ppq;
    wasPlaying = info.isPlaying;
}

void TriggerScheduler::renderLane(RepeatLane& lane, int laneIndex, int subdivIndex,
                                  juce::AudioBuffer<float>& laneBuffer, int numSamples) const
{
    // v1.17.0: Render up to each event, apply it, carry on. The capture write
    // runs per segment, so a trigger's slice ends exactly at its own sample.
    int position = 0;
    const int count = numEvents[laneIndex];

    for (int i = 0; i < count; ++i)
    {
        const auto& e = events[laneIndex][i];

        if (e.offset > position)
        {
            lane.processBlock(laneBuffer, position, e.offset - position);
            position = e.offset;
        }

        // v1.1.13: Per-lane lockout - MIDI ignores triggers while the lane is repeating
        if (e.requiresIdle && lane.isRepeating())
            continue;

        if (e.step == Step::FromIndex)
            lane.setPatternStepFromIndex(e.boundary);
        else if (e.step == Step::FromPpq)
            lane.updatePatternPosition(e.ppq, subdivIndex);

        lane.trigger();
    }

    if (numSamples > position)
        lane.processBlock(laneBuffer, position, numSamples - position);
}
