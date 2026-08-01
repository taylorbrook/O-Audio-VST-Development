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

    EnsembleChorus.cpp
    3-voice ensemble chorus with staggered delay lines and stereo spread

  ==============================================================================
*/

#include "EnsembleChorus.h"
#include "MathConstants.h"

void EnsembleChorus::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Max delay = largest center + max mod depth + margin
    float maxDelayMs = kCenterDelaysMs[kNumVoices - 1] + kMaxModDepthMs + 1.0f;
    int maxDelaySamples = static_cast<int> (maxDelayMs * 0.001f * static_cast<float> (sampleRate)) + 2;

    for (int v = 0; v < kNumVoices; ++v)
    {
        auto& voice = voices[v];
        voice.bufferSize = maxDelaySamples;
        voice.delayBufferL.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        voice.delayBufferR.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        voice.writePos = 0;
        voice.centerDelaySamples = kCenterDelaysMs[v] * 0.001f * static_cast<float> (sampleRate);
        voice.lfoPhase = kInitialPhases[v];

        // Equal-power pan
        float t = (kPanPositions[v] + 1.0f) * 0.5f; // map [-1,1] -> [0,1]
        voice.panL = std::cos (t * static_cast<float> (kHalfPi));
        voice.panR = std::sin (t * static_cast<float> (kHalfPi));
    }

    updateLfoRates();
}

void EnsembleChorus::reset()
{
    for (int v = 0; v < kNumVoices; ++v)
    {
        auto& voice = voices[v];
        std::fill (voice.delayBufferL.begin(), voice.delayBufferL.end(), 0.0f);
        std::fill (voice.delayBufferR.begin(), voice.delayBufferR.end(), 0.0f);
        voice.writePos = 0;
        voice.lfoPhase = kInitialPhases[v];
    }
}

void EnsembleChorus::setRate (float hz)
{
    rate = hz;
    updateLfoRates();
}

void EnsembleChorus::setDepth (float d)
{
    depth = d;
}

void EnsembleChorus::setMix (float m)
{
    mix = m;
}

void EnsembleChorus::updateLfoRates()
{
    for (int v = 0; v < kNumVoices; ++v)
        voices[v].lfoPhaseInc = kRateMultipliers[v] * rate * static_cast<float> (kTwoPi) / static_cast<float> (sampleRate);
}

void EnsembleChorus::process (juce::dsp::AudioBlock<float>& block)
{
    auto numSamples = static_cast<int> (block.getNumSamples());
    auto* dataL = block.getChannelPointer (0);
    auto* dataR = block.getNumChannels() > 1 ? block.getChannelPointer (1) : nullptr;

    float maxModSamples = kMaxModDepthMs * 0.001f * static_cast<float> (sampleRate);
    float dryGain = 1.0f - mix;
    // Scale wet per voice so total wet energy ~ 1.0
    float wetGain = mix * (1.0f / std::sqrt (static_cast<float> (kNumVoices)));

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = dataL[i];
        float inR = dataR != nullptr ? dataR[i] : inL;

        float outL = inL * dryGain;
        float outR = (dataR != nullptr ? inR : inL) * dryGain;

        for (int v = 0; v < kNumVoices; ++v)
        {
            auto& voice = voices[v];

            // Write input into delay buffer
            voice.delayBufferL[static_cast<size_t> (voice.writePos)] = inL;
            voice.delayBufferR[static_cast<size_t> (voice.writePos)] = inR;

            // LFO modulates delay time
            float lfoVal = std::sin (voice.lfoPhase);
            float delaySamples = voice.centerDelaySamples + lfoVal * depth * maxModSamples;
            delaySamples = juce::jlimit (1.0f, static_cast<float> (voice.bufferSize - 2), delaySamples);

            // Fractional delay with linear interpolation
            int delayInt = static_cast<int> (delaySamples);
            float frac = delaySamples - static_cast<float> (delayInt);

            int readPos0 = voice.writePos - delayInt;
            if (readPos0 < 0) readPos0 += voice.bufferSize;
            int readPos1 = readPos0 - 1;
            if (readPos1 < 0) readPos1 += voice.bufferSize;

            float delL = voice.delayBufferL[static_cast<size_t> (readPos0)] * (1.0f - frac)
                       + voice.delayBufferL[static_cast<size_t> (readPos1)] * frac;
            float delR = voice.delayBufferR[static_cast<size_t> (readPos0)] * (1.0f - frac)
                       + voice.delayBufferR[static_cast<size_t> (readPos1)] * frac;

            // Apply equal-power panning and accumulate
            outL += delL * voice.panL * wetGain;
            outR += delR * voice.panR * wetGain;

            // Advance LFO
            voice.lfoPhase += voice.lfoPhaseInc;
            if (voice.lfoPhase >= static_cast<float> (kTwoPi))
                voice.lfoPhase -= static_cast<float> (kTwoPi);

            // Advance write position
            voice.writePos = (voice.writePos + 1) % voice.bufferSize;
        }

        dataL[i] = outL;
        if (dataR != nullptr)
            dataR[i] = outR;
    }
}
