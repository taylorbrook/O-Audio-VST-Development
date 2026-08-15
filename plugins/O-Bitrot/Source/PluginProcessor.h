/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

    O-Bitrot - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class OBitrotAudioProcessor : public juce::AudioProcessor
{
public:
    OBitrotAudioProcessor();
    ~OBitrotAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Bitrot"; }
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

    // Public APVTS member for direct UI access (standard pattern for JUCE plugins)
    juce::AudioProcessorValueTreeState apvts;

private:
    // ------------------------------------------------------------------------
    // Cached parameter pointers (atomic, real-time safe).
    // Unused in the Stage 1 passthrough processBlock; wired now so Stage 2
    // only adds DSP.
    // ------------------------------------------------------------------------

    // Global (6)
    std::atomic<float>* clockModeParam = nullptr;
    std::atomic<float>* clockSyncDivParam = nullptr;
    std::atomic<float>* clockFreeRateParam = nullptr;
    std::atomic<float>* seedParam = nullptr;
    std::atomic<float>* hardEdgesParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    // Tape (4)
    std::atomic<float>* tapeEnableParam = nullptr;
    std::atomic<float>* tapeProbParam = nullptr;
    std::atomic<float>* tapeStopProbParam = nullptr;
    std::atomic<float>* tapeRampParam = nullptr;

    // CD Skip (4)
    std::atomic<float>* cdEnableParam = nullptr;
    std::atomic<float>* cdProbParam = nullptr;
    std::atomic<float>* cdSeverityParam = nullptr;
    std::atomic<float>* cdSegmentParam = nullptr;

    // Vinyl (4)
    std::atomic<float>* vinylEnableParam = nullptr;
    std::atomic<float>* vinylProbParam = nullptr;
    std::atomic<float>* vinylRpmParam = nullptr;
    std::atomic<float>* vinylPopParam = nullptr;

    // Packet Loss (4)
    std::atomic<float>* packetEnableParam = nullptr;
    std::atomic<float>* packetLossParam = nullptr;
    std::atomic<float>* packetBurstParam = nullptr;
    std::atomic<float>* packetConcealParam = nullptr;

    // Codec (3)
    std::atomic<float>* codecEnableParam = nullptr;
    std::atomic<float>* codecModeParam = nullptr;
    std::atomic<float>* codecMixParam = nullptr;

    // Crush (6)
    std::atomic<float>* crushEnableParam = nullptr;
    std::atomic<float>* crushBitsParam = nullptr;
    std::atomic<float>* crushRateParam = nullptr;
    std::atomic<float>* crushJitterParam = nullptr;
    std::atomic<float>* crushEnvAmtParam = nullptr;
    std::atomic<float>* crushDitherParam = nullptr;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBitrotAudioProcessor)
};
