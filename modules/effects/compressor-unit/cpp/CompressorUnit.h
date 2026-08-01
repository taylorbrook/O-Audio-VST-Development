/*
   This file is part of the Ouaricon Audio compressor-unit module.
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

    CompressorUnit - Embeddable Dynamics Compressor Module
    Ouaricon Audio Module System v1.3.0

    Compact compressor with 4 essential parameters:
    - Threshold, Ratio, Attack, Release
    Fixed 6dB soft knee for musical response.
    3ms look-ahead for click-free transient handling.

    See module CHANGELOG for version history.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <vector>

class CompressorUnit
{
public:
    static constexpr const char* PARAM_PREFIX = "comp_";
    static constexpr float FIXED_KNEE_DB = 6.0f;
    static constexpr float GAIN_SMOOTH_TIME_MS = 10.0f;
    static constexpr float MIN_DB = -60.0f;
    static constexpr float LOOKAHEAD_MS = 3.0f;

    CompressorUnit() = default;
    ~CompressorUnit() = default;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& prefix = PARAM_PREFIX)
    {
        // Enabled (bypass toggle)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "enabled", 1 },
            "Compressor Enabled",
            true  // ON by default
        ));

        // Threshold
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "threshold", 1 },
            "Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -20.0f,
            "dB"
        ));

        // Ratio
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "ratio", 1 },
            "Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            2.0f,
            ":1"
        ));

        // Attack
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "attack", 1 },
            "Attack",
            juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
            10.0f,
            "ms"
        ));

        // Release
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "release", 1 },
            "Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f),
            100.0f,
            "ms"
        ));

        // Autogain (makeup gain = gain reduction)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "autogain", 1 },
            "Auto Gain",
            false  // OFF by default
        ));
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;
        envelopeDB = MIN_DB;
        updateCoefficients(10.0f, 100.0f);
        gainSmoothCoeff = 1.0f - std::exp(-1000.0f / (GAIN_SMOOTH_TIME_MS * static_cast<float>(sampleRate)));

        // Initialize look-ahead delay buffer
        lookaheadSamples = static_cast<int>(LOOKAHEAD_MS * sampleRate / 1000.0);
        delayBufferSize = lookaheadSamples + maxBlockSize;
        delayBufferL.resize(static_cast<size_t>(delayBufferSize), 0.0f);
        delayBufferR.resize(static_cast<size_t>(delayBufferSize), 0.0f);
        delayWritePos = 0;
    }

    void reset()
    {
        envelopeDB = MIN_DB;
        smoothedGainDB = 0.0f;
        gainReductionDB.store(0.0f);

        // Clear delay buffers
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        delayWritePos = 0;
    }

    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& prefix = PARAM_PREFIX)
    {
        auto* enabledParam = apvts.getRawParameterValue(prefix + "enabled");
        if (enabledParam->load() < 0.5f)
        {
            // Bypassed: ramp gain toward unity and pass audio through delay line
            const int numSamples = buffer.getNumSamples();
            const int numChannels = buffer.getNumChannels();

            for (int sample = 0; sample < numSamples; ++sample)
            {
                smoothedGainDB += (0.0f - smoothedGainDB) * gainSmoothCoeff;

                // Write to delay buffer
                int writeIdx = (delayWritePos + sample) % delayBufferSize;
                if (numChannels > 0) delayBufferL[static_cast<size_t>(writeIdx)] = buffer.getSample(0, sample);
                if (numChannels > 1) delayBufferR[static_cast<size_t>(writeIdx)] = buffer.getSample(1, sample);

                // Read from delay buffer (lookahead samples behind)
                int readIdx = (delayWritePos + sample - lookaheadSamples + delayBufferSize) % delayBufferSize;
                if (numChannels > 0) buffer.getWritePointer(0)[sample] = delayBufferL[static_cast<size_t>(readIdx)];
                if (numChannels > 1) buffer.getWritePointer(1)[sample] = delayBufferR[static_cast<size_t>(readIdx)];
            }

            delayWritePos = (delayWritePos + numSamples) % delayBufferSize;
            envelopeDB = MIN_DB;
            gainReductionDB.store(0.0f);
            return;
        }

        float thresholdDB = apvts.getRawParameterValue(prefix + "threshold")->load();
        float ratio = apvts.getRawParameterValue(prefix + "ratio")->load();
        float attackTimeMs = apvts.getRawParameterValue(prefix + "attack")->load();
        float releaseTimeMs = apvts.getRawParameterValue(prefix + "release")->load();
        bool autogainEnabled = apvts.getRawParameterValue(prefix + "autogain")->load() > 0.5f;

        updateCoefficients(attackTimeMs, releaseTimeMs);

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        float peakGainReduction = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Write current sample to delay buffer
            int writeIdx = (delayWritePos + sample) % delayBufferSize;
            float inL = (numChannels > 0) ? buffer.getSample(0, sample) : 0.0f;
            float inR = (numChannels > 1) ? buffer.getSample(1, sample) : inL;
            delayBufferL[static_cast<size_t>(writeIdx)] = inL;
            delayBufferR[static_cast<size_t>(writeIdx)] = inR;

            // Stereo-linked detection from CURRENT input (look-ahead)
            float maxInputLevel = std::max(std::abs(inL), std::abs(inR));
            float inputLevelDB = juce::Decibels::gainToDecibels(maxInputLevel, MIN_DB);

            // Envelope follower
            float envCoeff = (inputLevelDB > envelopeDB) ? attackCoeff : releaseCoeff;
            envelopeDB += (inputLevelDB - envelopeDB) * envCoeff;

            float gr = calculateGainReduction(envelopeDB, thresholdDB, ratio, FIXED_KNEE_DB);
            peakGainReduction = std::max(peakGainReduction, gr);

            // Smooth gain in dB domain
            float targetGainDB = -gr;
            float smoothCoeff = (targetGainDB < smoothedGainDB)
                ? std::min(attackCoeff, gainSmoothCoeff)
                : std::min(releaseCoeff, gainSmoothCoeff);
            smoothedGainDB += (targetGainDB - smoothedGainDB) * smoothCoeff;

            // Read DELAYED audio and apply gain
            int readIdx = (delayWritePos + sample - lookaheadSamples + delayBufferSize) % delayBufferSize;
            float delayedL = delayBufferL[static_cast<size_t>(readIdx)];
            float delayedR = delayBufferR[static_cast<size_t>(readIdx)];

            float smoothedGainLinear = juce::Decibels::decibelsToGain(smoothedGainDB);
            if (numChannels > 0) buffer.getWritePointer(0)[sample] = delayedL * smoothedGainLinear;
            if (numChannels > 1) buffer.getWritePointer(1)[sample] = delayedR * smoothedGainLinear;
        }

        delayWritePos = (delayWritePos + numSamples) % delayBufferSize;
        gainReductionDB.store(peakGainReduction);

        // Autogain: theoretical formula matching OuariconComp standalone
        // autoGainDB = -threshold × (1 - 1/ratio)
        if (autogainEnabled)
        {
            float autoGainDB = -thresholdDB * (1.0f - 1.0f / ratio);
            buffer.applyGain(juce::Decibels::decibelsToGain(autoGainDB));
        }
    }

    float getGainReductionDB() const { return gainReductionDB.load(); }

private:
    double currentSampleRate = 44100.0;
    float envelopeDB = MIN_DB;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float smoothedGainDB = 0.0f;
    float gainSmoothCoeff = 0.005f;
    std::atomic<float> gainReductionDB { 0.0f };

    // Look-ahead delay buffer
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int delayBufferSize = 0;
    int delayWritePos = 0;
    int lookaheadSamples = 0;

    float calculateGainReduction(float inputLevel, float thresholdDB, float ratio, float kneeDB)
    {
        float x = inputLevel - thresholdDB;

        if (x < -kneeDB / 2.0f)
        {
            // Below knee - no compression
            return 0.0f;
        }
        else if (x > kneeDB / 2.0f)
        {
            // Above knee - full compression
            return x - (x / ratio);
        }
        else
        {
            // Inside knee - smooth transition
            float kneeFactor = (x + kneeDB / 2.0f) / kneeDB;
            float fullReduction = x - (x / ratio);
            return kneeFactor * kneeFactor * fullReduction;
        }
    }

    void updateCoefficients(float attackTimeMs, float releaseTimeMs)
    {
        attackCoeff = 1.0f - std::exp(-1.0f / (attackTimeMs * static_cast<float>(currentSampleRate) / 1000.0f));
        releaseCoeff = 1.0f - std::exp(-1.0f / (releaseTimeMs * static_cast<float>(currentSampleRate) / 1000.0f));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorUnit)
};
