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

    MFCCExtractor.h
    13-coefficient MFCC extraction from audio frames

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class MFCCExtractor
{
public:
    MFCCExtractor();

    // Extract 13 MFCCs from a single frame
    // Returns MFCCs and exposes magnitude spectrum for reuse
    std::array<float, 13> extractMFCCs(const float* frame, int frameSize);

    // Get magnitude spectrum from last extraction (for spectral features)
    const std::vector<float>& getMagnitudeSpectrum() const { return magnitudes; }

private:
    static constexpr int FFT_ORDER = 11;
    static constexpr int FFT_SIZE = 2048;
    static constexpr int NUM_MEL_FILTERS = 40;
    static constexpr int NUM_MFCCS = 13;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, FFT_SIZE * 2> fftData;  // Complex format
    std::vector<float> magnitudes;
    std::array<std::array<float, FFT_SIZE / 2 + 1>, NUM_MEL_FILTERS> melFilterbank;
    std::array<std::array<float, NUM_MEL_FILTERS>, NUM_MFCCS> dctMatrix;

    void buildMelFilterbank(double sampleRate);
    void buildDCTMatrix();
    float hzToMel(float hz) const;
    float melToHz(float mel) const;
};
