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

    TriggerScheduler.h
    Ouaricon Polystutter - Sample-accurate trigger scheduling
    v1.17.0: Replaces TriggerRouter. Builds, per block, each lane's trigger
    events at their exact sample offsets (beat-sync boundaries, MIDI note-ons,
    the manual button); the processor renders each lane in segments split at
    them. Was: one trigger per block, fired at sample 0 — beat-sync up to a
    block LATE, MIDI up to a block EARLY (note offset ignored), chords
    collapsed to their last note (WR-07), loop-wrap downbeats dropped (WR-04).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <climits>

class TriggerScheduler
{
public:
    static constexpr int kNumLanes = 4;
    static constexpr int kMaxEventsPerLane = 64;

    // Same-offset events keep v1.16.0's block-start order: MIDI, manual, beat
    enum class Kind : uint8_t { Midi = 0, Manual = 1, Beat = 2 };

    // How the event sets the lane's pattern step before triggering
    enum class Step : uint8_t { Keep, FromPpq, FromIndex };

    struct Event
    {
        int offset = 0;
        Kind kind = Kind::Beat;
        Step step = Step::Keep;
        double ppq = 0.0;            // Step::FromPpq
        long long boundary = 0;      // Step::FromIndex (absolute subdivision index)
        bool requiresIdle = false;   // MIDI lockout: skip while the lane is repeating
    };

    struct BlockInfo
    {
        int numSamples = 0;
        double sampleRate = 48000.0;
        double bpm = 120.0;          // already clamped by the processor
        bool hasPosition = false;    // playhead returned a position
        bool hasPpq = false;
        double ppq = 0.0;            // block-start ppq
        bool isPlaying = false;
        bool midiEnabled = false;
        bool manualEdge = false;     // manual-trigger rising edge this block
        bool laneEnabled[kNumLanes] = { false, false, false, false };
        int laneSubdiv[kNumLanes] = { 2, 2, 2, 2 };
    };

    void reset();

    // Build every lane's event list for this block (sorted by offset, then kind)
    void schedule(const BlockInfo& info, const juce::MidiBuffer& midi);

    int getNumEvents(int lane) const { return numEvents[lane]; }
    const Event& getEvent(int lane, int index) const { return events[lane][index]; }

    // Render one lane's block in segments split at its events: render up to
    // each event, apply it (lockout, pattern step, trigger), carry on
    void renderLane(class RepeatLane& lane, int laneIndex, int subdivIndex,
                    juce::AudioBuffer<float>& laneBuffer, int numSamples) const;

    // Subdivision length in quarter notes (index as the lane_subdiv parameter)
    static double getSubdivisionPPQ(int subdivIndex);

private:
    void add(int lane, const Event& e);

    Event events[kNumLanes][kMaxEventsPerLane];
    int numEvents[kNumLanes] = { 0, 0, 0, 0 };

    // Beat-sync continuity
    bool wasPlaying = false;
    double lastPPQPosition = 0.0;
    long long lastFiredBoundary[kNumLanes] = { LLONG_MIN, LLONG_MIN, LLONG_MIN, LLONG_MIN };
};
