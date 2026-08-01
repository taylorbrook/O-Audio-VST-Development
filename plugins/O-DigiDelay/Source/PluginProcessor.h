/*
   This file is part of O-DigiDelay, an Ouaricon Audio plugin.
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

    Ouaricon Digital Delay - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"

class OuariconDigitalDelayAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconDigitalDelayAudioProcessor();
    ~OuariconDigitalDelayAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-DigiDelay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

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

    // RMS level getters for output meter (returns 0.0-1.0)
    // Read the published atomic snapshot, not the audio-thread smoother state (WR-06).
    float getRmsLevelLeft() const { return rmsMeterLeft.load(std::memory_order_relaxed); }
    float getRmsLevelRight() const { return rmsMeterRight.load(std::memory_order_relaxed); }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (declare BEFORE APVTS for initialization order)
    juce::dsp::ProcessSpec spec;

    // Dual mono delay lines with Lagrange3rd interpolation
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineLeft;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineRight;

    // LFO for delay time modulation (0.3Hz fixed rate)
    juce::dsp::Oscillator<float> lfo;

    // Smoothed parameter values (20ms ramp to prevent clicks)
    juce::SmoothedValue<float> smoothedTimeMs { 500.0f };
    juce::SmoothedValue<float> smoothedFeedback { 0.3f };
    juce::SmoothedValue<float> smoothedSpread { 0.0f };
    juce::SmoothedValue<float> smoothedMod { 0.0f };
    juce::SmoothedValue<float> smoothedWet { 0.3f };
    juce::SmoothedValue<float> smoothedDry { 1.0f };

    // Feedback state (per-channel)
    float feedbackLeft = 0.0f;
    float feedbackRight = 0.0f;

    // Cached parameter pointers (stable after construction)
    std::atomic<float>* timeParam = nullptr;
    std::atomic<float>* syncParam = nullptr;
    std::atomic<float>* divisionParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* spreadParam = nullptr;
    std::atomic<float>* modParam = nullptr;
    std::atomic<float>* wetParam = nullptr;
    std::atomic<float>* dryParam = nullptr;

    // RMS level calculation for output meter (audio-thread only)
    juce::LinearSmoothedValue<float> rmsLevelLeft { 0.0f };
    juce::LinearSmoothedValue<float> rmsLevelRight { 0.0f };

    // Thread-safe meter snapshots published from processBlock, read by the editor timer (WR-06)
    std::atomic<float> rmsMeterLeft { 0.0f };
    std::atomic<float> rmsMeterRight { 0.0f };

    // Subdivision lookup table (12 values)
    static constexpr float subdivisionFactors[12] = {
        1.0f,     // 1/4
        0.5f,     // 1/8
        0.25f,    // 1/16
        1.5f,     // 1/4D (dotted)
        0.75f,    // 1/8D
        0.375f,   // 1/16D
        0.667f,   // 1/4T (triplet)
        0.333f,   // 1/8T
        0.167f,   // 1/16T
        0.8f,     // 1/4(5) (quintuplet)
        0.4f,     // 1/8(5)
        0.2f      // 1/16(5)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconDigitalDelayAudioProcessor)
};
