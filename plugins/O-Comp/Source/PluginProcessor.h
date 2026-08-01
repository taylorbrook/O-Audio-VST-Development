/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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

    O-Comp - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "OuariconPresetManager.h"

class OCompAudioProcessor : public juce::AudioProcessor
{
public:
    OCompAudioProcessor();
    ~OCompAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // Preset management
    OuariconPresetManager presetManager;

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP state
    double currentSampleRate = 0.0;

    // Cached parameter pointers (set once in constructor, never change)
    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* kneeParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* autoGainParam = nullptr;

    // Compressor state
    float envelopeDB = -60.0f;  // Current envelope level in dB
    float attackCoeff = 0.0f;   // Attack time coefficient
    float releaseCoeff = 0.0f;  // Release time coefficient

    // Makeup/output gain smoothing (de-zippers automation and auto-gain toggles)
    juce::SmoothedValue<float> smoothedMakeup { 1.0f };

    // Metering (atomic for thread-safe access from UI)
    std::atomic<float> inputLevelDB { -60.0f };
    std::atomic<float> outputLevelDB { -60.0f };
    std::atomic<float> currentGainReductionDB { 0.0f };
    std::atomic<float> currentEnvelopeDB { -60.0f };

public:
    // Meter getters for UI
    float getInputLevelDB() const { return inputLevelDB.load(); }
    float getOutputLevelDB() const { return outputLevelDB.load(); }
    float getGainReductionDB() const { return currentGainReductionDB.load(); }
    float getEnvelopeDB() const { return currentEnvelopeDB.load(); }

private:

    // DSP Helper Methods
    float calculateGainReduction(float inputLevel, float thresholdDB,
                                  float ratio, float kneeDB);
    void updateCoefficients(float attackTimeMs, float releaseTimeMs, double sampleRate);

    // Linear makeup gain (auto-gain + output_gain) from current param values.
    // Shared by prepareToPlay (initial snap) and processBlock (per-block target).
    float computeMakeupGainLinear() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OCompAudioProcessor)
};
