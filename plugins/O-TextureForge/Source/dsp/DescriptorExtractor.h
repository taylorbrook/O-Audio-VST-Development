/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    DescriptorExtractor.h
    19-dimensional descriptor extraction (MFCCs + spectral features)

  ==============================================================================
*/

#pragma once

#include "MFCCExtractor.h"
#include "GrainMetadata.h"
#include <array>
#include <vector>

class DescriptorExtractor
{
public:
    DescriptorExtractor();

    // Extract 19D descriptor from audio frame
    // 13 MFCCs + spectral centroid + flatness + flux + rolloff + RMS + ZCR
    std::array<float, 19> extract(const float* frame, int frameSize, double sampleRate);

    // Compute z-score normalization statistics from all grains
    // Two-pass: compute means, then stddevs
    static void computeNormalizationStats(
        std::vector<GrainMetadata>& grains,
        std::array<float, 19>& outMeans,
        std::array<float, 19>& outStddevs);

    // Apply z-score normalization to descriptors
    static void normalizeDescriptors(
        std::vector<GrainMetadata>& grains,
        const std::array<float, 19>& means,
        const std::array<float, 19>& stddevs);

private:
    MFCCExtractor mfccExtractor;
    std::vector<float> previousMagnitudes;  // For spectral flux

    float computeSpectralCentroid(const std::vector<float>& magnitudes, double sampleRate) const;
    float computeSpectralFlatness(const std::vector<float>& magnitudes) const;
    float computeSpectralFlux(const std::vector<float>& magnitudes);
    float computeSpectralRolloff(const std::vector<float>& magnitudes, double sampleRate) const;
    float computeRMS(const float* frame, int frameSize) const;
    float computeZCR(const float* frame, int frameSize) const;
};
