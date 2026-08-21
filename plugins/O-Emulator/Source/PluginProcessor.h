/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "dsp/ConsoleEngine.h"

class OEmulatorAudioProcessor : public juce::AudioProcessor
{
public:
    OEmulatorAudioProcessor();
    ~OEmulatorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Emulator"; }
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

#if OUARICON_RENDER_HARNESS
    // Harness-only instrumentation (O-Tapestop PluginProcessor.h:160-175
    // idiom): FIFO fill / chunk phase / computed latency exposed to probes
    // without shipping it.
    int getComputedLatencyForTest() const noexcept { return totalLatencySamples; }
    int getFeederChunkPhaseForTest() const noexcept { return engine.getChunkPhaseForTest(); }
    std::uint64_t getFeederAbsoluteSamplesForTest() const noexcept { return engine.getAbsoluteSamplesForTest(); }
    int getUpsampleFillForTest() const noexcept { return engine.getUpsampleFillForTest(); }
#endif

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── Latency + mix (plan decision #2, BINDING) ────────────────────────────
    // DryWetMixer MUST be constructed with the max wet latency — a default-
    // constructed mixer sizes its Thiran delay to 0 and setWetLatency()
    // becomes a silent no-op (RESEARCH §1.2 ctor gotcha; O-Bitrot does it
    // right, O-Prism carries the latent bug). 1024 covers the 192 kHz
    // worst-case figure (~319) with ~3x margin.
    static constexpr int kMaxWetLatencySamples = 1024;

    oemu::ConsoleEngine engine;
    juce::dsp::DryWetMixer<float> dryWetMixer { kMaxWetLatencySamples };

    /** Computed ONCE in prepareToPlay (setLatencySamples triggers
        updateHostDisplay — never call it from the audio callback). Mirrored
        exactly into setWetLatency. */
    int totalLatencySamples = 0;

    // Cached parameter atomics (audio-thread reads). age/reverb exist in the
    // APVTS but drive nothing until Phases 2.2/2.4; console until 2.3.
    std::atomic<float>* crushParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OEmulatorAudioProcessor)
};
