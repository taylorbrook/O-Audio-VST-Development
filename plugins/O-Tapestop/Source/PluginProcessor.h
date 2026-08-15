/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>

#include <atomic>

// O-Tapestop — varispeed tapestop/tapestart + drawable-envelope scratch mode.
//
// Stage 1 (foundation): build system + full 14-parameter APVTS + stereo
// bitwise pass-through shell. NO DSP — varispeed/transport/scratch land in
// Stage 2. MIX and OUTPUT_GAIN exist as parameters but stay unwired; the
// disengaged path must never touch samples (Stage-0 decision #6 — Stage 2's
// null probes depend on bit-transparency here).
//
// NOTE: this file (and PluginProcessor.cpp) must stay free of editor-only
// includes — the Stage-2 render harness compiles the processor with
// JUCE_WEB_BROWSER=0 and no editor sources.
class TapestopProcessor : public juce::AudioProcessor
{
public:
    TapestopProcessor();
    ~TapestopProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached raw parameter atomics — resolved once in the constructor
    // (getRawParameterValue), read per block on the audio thread
    // (research/ARCHITECTURE.md line 348, suite convention). Unused in the
    // Stage-1 pass-through shell; Stage 2 wires them.
    std::atomic<float>* pEngage       = nullptr;
    std::atomic<float>* pMode         = nullptr;
    std::atomic<float>* pSyncMode     = nullptr;
    std::atomic<float>* pStopSyncDiv  = nullptr;
    std::atomic<float>* pStopFreeMs   = nullptr;
    std::atomic<float>* pStopCurve    = nullptr;
    std::atomic<float>* pStartSyncDiv = nullptr;
    std::atomic<float>* pStartFreeMs  = nullptr;
    std::atomic<float>* pStartCurve   = nullptr;
    std::atomic<float>* pEnvSyncDiv   = nullptr;
    std::atomic<float>* pEnvFreeMs    = nullptr;
    std::atomic<float>* pToneTrack    = nullptr;
    std::atomic<float>* pMix          = nullptr;
    std::atomic<float>* pOutputGain   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapestopProcessor)
};
