/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
#include "VBAPDataExchange.h"

extern "C" {
#include "saf.h"
}

void VBAPComputeThread::run()
{
    while (! threadShouldExit())
    {
        wait (-1); // Wait for notification

        if (threadShouldExit())
            break;

        SpeakerLayout layout;
        bool shouldCompute = false;

        {
            const juce::ScopedLock lock (requestLock);
            if (hasRequest)
            {
                layout = pendingLayout;
                hasRequest = false;
                shouldCompute = true;
            }
        }

        if (shouldCompute)
            computeGainTable (layout);
    }
}

void VBAPComputeThread::computeGainTable (const SpeakerLayout& layout)
{
    int totalSpeakers = layout.getChannelCount();
    if (totalSpeakers < 4)
        return; // 2-3 speakers handled by pair-wise panning, no SAF needed

    // Build speaker direction array, excluding LFE
    std::vector<float> lsDirsDeg;
    std::vector<int> speakerToChannel;
    int numVBAPSpeakers = 0;

    for (int i = 0; i < totalSpeakers; ++i)
    {
        if (layout.speakers[(size_t) i].isLFE)
            continue;

        lsDirsDeg.push_back (layout.speakers[(size_t) i].azimuth);
        lsDirsDeg.push_back (layout.speakers[(size_t) i].elevation);
        speakerToChannel.push_back (i);
        ++numVBAPSpeakers;
    }

    if (numVBAPSpeakers < 4)
        return;

    float* gtable = nullptr;
    int nGtable = 0;
    int nTriangles = 0;

    int aziRes = 1;
    int elevRes = 1;

    if (layout.is3D)
    {
        generateVBAPgainTable3D (
            lsDirsDeg.data(),
            numVBAPSpeakers,
            aziRes,    // azimuth resolution
            elevRes,   // elevation resolution
            1,         // omit large triangles
            1,         // enable dummy speakers
            0.0f,      // spread
            &gtable,
            &nGtable,
            &nTriangles
        );
    }
    else
    {
        // 2D: all elevation = 0, use same function with enableDummies
        generateVBAPgainTable3D (
            lsDirsDeg.data(),
            numVBAPSpeakers,
            aziRes,
            elevRes,
            1,
            1,         // enableDummies adds polar speakers for convex hull
            0.0f,
            &gtable,
            &nGtable,
            &nTriangles
        );
    }

    if (gtable == nullptr || nGtable <= 0)
        return;

    auto data = std::make_unique<VBAPData>();
    data->gainTable.assign (gtable, gtable + (size_t) (nGtable * numVBAPSpeakers));
    data->speakerToChannelMap = speakerToChannel;
    data->numVBAPSpeakers = numVBAPSpeakers;
    data->numOutputChannels = totalSpeakers;
    data->aziRes = aziRes;
    data->elevRes = elevRes;
    data->is3D = layout.is3D;

    free (gtable);

    dataExchange.setNewData (std::move (data));
}
