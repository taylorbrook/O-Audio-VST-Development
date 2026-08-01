/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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

    EnsembleChorus.h
    3-voice ensemble chorus with staggered delay lines and stereo spread

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EnsembleChorus
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::dsp::AudioBlock<float>& block);

    void setRate (float hz);
    void setDepth (float d);       // 0..1
    void setMix (float m);         // 0..1

private:
    static constexpr int kNumVoices = 3;
    static constexpr float kCenterDelaysMs[kNumVoices] = { 5.0f, 7.0f, 9.0f };
    static constexpr float kRateMultipliers[kNumVoices] = { 1.0f, 0.93f, 1.07f };
    static constexpr float kInitialPhases[kNumVoices] = { 0.0f, 2.0943951f, 4.1887902f }; // 0, 2π/3, 4π/3
    static constexpr float kPanPositions[kNumVoices] = { -0.6f, 0.0f, 0.6f };
    static constexpr float kMaxModDepthMs = 2.0f; // max LFO modulation depth in ms

    struct Voice
    {
        std::vector<float> delayBufferL, delayBufferR;
        int writePos = 0;
        int bufferSize = 0;
        float centerDelaySamples = 0.0f;
        float lfoPhase = 0.0f;
        float lfoPhaseInc = 0.0f;
        float panL = 0.0f, panR = 0.0f;
    };

    Voice voices[kNumVoices];
    double sampleRate = 44100.0;
    float rate = 1.0f;
    float depth = 0.5f;
    float mix = 0.0f;

    void updateLfoRates();
};
