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
#include "VBAPRenderer.h"

void VBAPRenderer::prepare (const SpeakerLayout& layout)
{
    currentNumSpeakers = layout.getChannelCount();
    speakerAzimuths.clear();
    lfeChannelIndices.clear();

    for (int i = 0; i < currentNumSpeakers; ++i)
    {
        speakerAzimuths.push_back (layout.speakers[(size_t) i].azimuth);

        if (layout.speakers[(size_t) i].isLFE)
            lfeChannelIndices.push_back (i);
    }

    useSAF = (currentNumSpeakers >= 4);

    // Reset external gain table
    externalGainTable = nullptr;
    externalTableSize = 0;
}

void VBAPRenderer::computeGains (float azimuth, float elevation, float* gains, int numSpeakers)
{
    if (numSpeakers <= 0)
        return;

    for (int i = 0; i < numSpeakers; ++i)
        gains[i] = 0.0f;

    if (numSpeakers == 2)
    {
        computeStereoGains (azimuth, gains);
    }
    else if (numSpeakers == 3 || ! useSAF)
    {
        computePairwiseGains (azimuth, gains, numSpeakers);
    }
    else
    {
        computeSAFGains (azimuth, elevation, gains, numSpeakers);
    }

    // Zero out LFE channels
    for (int lfeIdx : lfeChannelIndices)
    {
        if (lfeIdx < numSpeakers)
            gains[lfeIdx] = 0.0f;
    }

    if (centerDiverge > 0.0f)
        applyCenterDiverge (gains, numSpeakers);
}

void VBAPRenderer::computeStereoGains (float azimuth, float* gains)
{
    // Azimuth convention: 0=front, +counter-clockwise, -clockwise
    // Speakers at L=-30 (left), R=+30 (right)
    // Map azimuth to pan position [0, 1] where 0=L, 1=R
    float panAngle = (azimuth + 90.0f) / 180.0f;
    panAngle = std::clamp (panAngle, 0.0f, 1.0f);

    gains[0] = std::cos (panAngle * (float) M_PI * 0.5f);  // L
    gains[1] = std::sin (panAngle * (float) M_PI * 0.5f);  // R
}

void VBAPRenderer::computePairwiseGains (float azimuth, float* gains, int numSpeakers)
{
    // Find the two nearest speakers surrounding the source azimuth
    // and apply equal-power panning between them

    float bestDist1 = 999.0f, bestDist2 = 999.0f;
    int idx1 = 0, idx2 = 0;

    for (int i = 0; i < numSpeakers; ++i)
    {
        // Skip LFE speakers
        bool isLFE = false;
        for (int lfe : lfeChannelIndices)
            if (lfe == i) { isLFE = true; break; }
        if (isLFE) continue;

        float diff = std::fmod (speakerAzimuths[(size_t) i] - azimuth + 540.0f, 360.0f) - 180.0f;
        float absDiff = std::abs (diff);

        if (absDiff < bestDist1)
        {
            bestDist2 = bestDist1;
            idx2 = idx1;
            bestDist1 = absDiff;
            idx1 = i;
        }
        else if (absDiff < bestDist2)
        {
            bestDist2 = absDiff;
            idx2 = i;
        }
    }

    if (idx1 == idx2)
    {
        gains[idx1] = 1.0f;
        return;
    }

    // Compute angle between the two speakers
    float span = std::fmod (speakerAzimuths[(size_t) idx2] - speakerAzimuths[(size_t) idx1] + 540.0f, 360.0f) - 180.0f;
    float relAz = std::fmod (azimuth - speakerAzimuths[(size_t) idx1] + 540.0f, 360.0f) - 180.0f;

    float t = 0.5f;
    if (std::abs (span) > 0.001f)
        t = std::clamp (relAz / span, 0.0f, 1.0f);

    gains[idx1] = std::cos (t * (float) M_PI * 0.5f);
    gains[idx2] = std::sin (t * (float) M_PI * 0.5f);
}

void VBAPRenderer::computeSAFGains (float azimuth, float elevation, float* gains, int numSpeakers)
{
    if (externalGainTable == nullptr || externalNumVBAPSpeakers <= 0)
    {
        // Fallback: equal gain to all non-LFE speakers
        int nonLFE = numSpeakers - (int) lfeChannelIndices.size();
        float equalGain = (nonLFE > 0) ? 1.0f / std::sqrt ((float) nonLFE) : 0.0f;
        for (int i = 0; i < numSpeakers; ++i)
            gains[i] = equalGain;
        for (int lfe : lfeChannelIndices)
            if (lfe < numSpeakers) gains[lfe] = 0.0f;
        return;
    }

    if (externalIs3D)
    {
        int N_azi = (int) (360.0f / (float) externalAziRes + 0.5f) + 1;
        int aziIndex = (int) (std::fmod (azimuth + 180.0f, 360.0f) / (float) externalAziRes + 0.5f);
        int elevIndex = (int) ((elevation + 90.0f) / (float) externalElevRes + 0.5f);
        elevIndex = std::clamp (elevIndex, 0, 180 / externalElevRes);
        aziIndex = std::clamp (aziIndex, 0, N_azi - 1);
        int idx = elevIndex * N_azi + aziIndex;

        if (idx * externalNumVBAPSpeakers + externalNumVBAPSpeakers <= externalTableSize)
        {
            for (int s = 0; s < externalNumVBAPSpeakers; ++s)
            {
                int outCh = (externalSpeakerToChannel != nullptr) ? externalSpeakerToChannel[s] : s;
                if (outCh >= 0 && outCh < numSpeakers)
                    gains[outCh] = externalGainTable[idx * externalNumVBAPSpeakers + s];
            }
        }
    }
    else
    {
        int aziIndex = (int) (std::fmod (azimuth + 180.0f, 360.0f) / (float) externalAziRes + 0.5f);
        aziIndex = std::clamp (aziIndex, 0, externalTableSize / std::max (externalNumVBAPSpeakers, 1) - 1);

        if (aziIndex * externalNumVBAPSpeakers + externalNumVBAPSpeakers <= externalTableSize)
        {
            for (int s = 0; s < externalNumVBAPSpeakers; ++s)
            {
                int outCh = (externalSpeakerToChannel != nullptr) ? externalSpeakerToChannel[s] : s;
                if (outCh >= 0 && outCh < numSpeakers)
                    gains[outCh] = externalGainTable[aziIndex * externalNumVBAPSpeakers + s];
            }
        }
    }
}

void VBAPRenderer::applyCenterDiverge (float* gains, int numSpeakers)
{
    if (numSpeakers < 3 || speakerAzimuths.size() < (size_t) numSpeakers)
        return;

    float diverge = centerDiverge;
    std::vector<float> newGains ((size_t) numSpeakers, 0.0f);

    for (int i = 0; i < numSpeakers; ++i)
    {
        if (gains[i] <= 0.0f) continue;

        // Skip LFE
        bool isLFE = false;
        for (int lfe : lfeChannelIndices)
            if (lfe == i) { isLFE = true; break; }
        if (isLFE) continue;

        newGains[(size_t) i] += gains[i];

        for (int j = 0; j < numSpeakers; ++j)
        {
            if (j == i) continue;

            bool isJLFE = false;
            for (int lfe : lfeChannelIndices)
                if (lfe == j) { isJLFE = true; break; }
            if (isJLFE) continue;

            float angDiff = std::abs (std::fmod (speakerAzimuths[(size_t) j] - speakerAzimuths[(size_t) i] + 540.0f, 360.0f) - 180.0f);
            float proximity = std::max (0.0f, 1.0f - angDiff / 180.0f);
            newGains[(size_t) j] += diverge * proximity * gains[i];
        }
    }

    // Energy-preserving normalization
    float energy = 0.0f;
    for (int i = 0; i < numSpeakers; ++i)
        energy += newGains[(size_t) i] * newGains[(size_t) i];

    if (energy > 0.0f)
    {
        float originalEnergy = 0.0f;
        for (int i = 0; i < numSpeakers; ++i)
            originalEnergy += gains[i] * gains[i];

        float scale = (originalEnergy > 0.0f) ? std::sqrt (originalEnergy / energy) : 0.0f;
        for (int i = 0; i < numSpeakers; ++i)
            gains[i] = newGains[(size_t) i] * scale;
    }
}

void VBAPRenderer::setExternalGainTable (const float* table, int tableSize, int numSpkrs,
                                          int aziResolution, int elevResolution, bool is3DTable,
                                          const int* spkrToChannelMap, int numVBAPSpkrs)
{
    externalGainTable = table;
    externalTableSize = tableSize;
    externalNumVBAPSpeakers = numVBAPSpkrs;
    externalAziRes = aziResolution;
    externalElevRes = elevResolution;
    externalIs3D = is3DTable;
    externalSpeakerToChannel = spkrToChannelMap;
    externalNumMappedSpeakers = numSpkrs;
}
