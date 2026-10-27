/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>

struct SyncInfo
{
    double bpm = 120.0;
    double ppqPosition = 0.0;
    double ppqPerSample = 0.0;
    bool isPlaying = false;

    // v3.1.0: ppq at block sample i is ppqOrigin + (sampleOffset + i) * ppqPerSample.
    // Host: origin = the host's block-start ppq, offset 0. Standalone/no playhead:
    // origin 0, offset = samples since reset, so the ppq of a given sample is one
    // product, the same however the stream is split into blocks.
    double ppqOrigin = 0.0;
    juce::int64 sampleOffset = 0;

    double ppqAt (juce::int64 i) const
    {
        return ppqOrigin + static_cast<double> (sampleOffset + i) * ppqPerSample;
    }
};

class TempoTracker
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        manualSamples = 0;
    }

    SyncInfo update (juce::AudioPlayHead* playHead, int numSamples)
    {
        SyncInfo info;

        bool gotPosition = false;

        if (playHead != nullptr)
        {
            auto pos = playHead->getPosition();
            if (pos.hasValue())
            {
                auto bpmOpt = pos->getBpm();
                auto ppqOpt = pos->getPpqPosition();
                auto playing = pos->getIsPlaying();

                // WR-09: a host reporting bpm <= 0 (stopped/scanning) must NOT be taken
                // verbatim — ppqPerSample would be 0 and Sync mode would silently stop
                // scheduling. Treat it as "no position" and use the 120 BPM fallback below.
                if (bpmOpt.hasValue() && ppqOpt.hasValue() && *bpmOpt > 0.0)
                {
                    info.bpm = *bpmOpt;
                    info.ppqPosition = *ppqOpt;
                    info.isPlaying = playing;
                    info.ppqPerSample = info.bpm / (60.0 * sampleRate);
                    info.ppqOrigin = info.ppqPosition;

                    gotPosition = true;
                }
            }
        }

        if (!gotPosition)
        {
            // Standalone fallback: 120 BPM manual counter
            info.bpm = 120.0;
            info.ppqPerSample = 120.0 / (60.0 * sampleRate);
            info.ppqOrigin = 0.0;
            info.sampleOffset = manualSamples;
            info.ppqPosition = info.ppqAt (0);
            info.isPlaying = true;

            // v3.1.0: an integer sample count, not an accumulated ppq — the sum of
            // per-block increments rounds differently for different block sizes.
            manualSamples += numSamples;
        }

        return info;
    }

private:
    double sampleRate = 44100.0;
    juce::int64 manualSamples = 0;
};
