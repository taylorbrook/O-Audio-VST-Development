/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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

    O-simpleAmbisonics - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include <atomic>

#include "dsp/FixedDelay.h"
#include "dsp/SpatialEngine.h"

namespace OSimpleAmbisonics::ParamIDs
{
    // The 12-parameter v1.0 contract (research/ARCHITECTURE.md §Parameter Mapping).
    // IDs are frozen: a range change after 1.0.0 needs a preset-migration gate.
    inline constexpr auto azimuth     = "azimuth";
    inline constexpr auto elevation   = "elevation";
    inline constexpr auto distance    = "distance";
    inline constexpr auto order       = "order";
    inline constexpr auto yaw         = "yaw";
    inline constexpr auto pitch       = "pitch";
    inline constexpr auto roll        = "roll";
    inline constexpr auto decoder     = "decoder";
    inline constexpr auto orbit       = "orbit";
    inline constexpr auto orbitRate   = "orbitRate";
    inline constexpr auto inputMode   = "inputMode";
    inline constexpr auto outputLevel = "outputLevel";
}

class OSimpleAmbisonicsAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleAmbisonicsAudioProcessor();
    ~OSimpleAmbisonicsAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Public APVTS member for direct UI / harness access (house pattern).
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached raw-value atomics (the only two parameters the Stage-1 pass-through reads).
    std::atomic<float>* inputModeParam   = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;

    // Pass-through chain: conditioner → 64-sample delay → seeded gain.
    // Stage 2 replaces `delay` with a SpatialEngine call inside the slice loop.
    OSimpleAmbisonics::dsp::DecoderDelay delay;
    juce::LinearSmoothedValue<float> outputGain;

    int numInputLegs = 2;   // negotiated input bus width (1 or 2), NOT the buffer width
    int maxBlock     = 0;   // samplesPerBlock from prepareToPlay; slice bound

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAmbisonicsAudioProcessor)
};
