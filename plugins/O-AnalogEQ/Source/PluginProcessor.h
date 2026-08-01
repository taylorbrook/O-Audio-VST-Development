/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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

    Ouaricon Analog EQ - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "OuariconPresetManager.h"

class OuariconAnalogEQAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconAnalogEQAudioProcessor();
    ~OuariconAnalogEQAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Analog EQ"; }
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
    OuariconPresetManager presetManager;

    // VU Meter - output level for WebView (thread-safe)
    std::atomic<float> outputLevelDB { -100.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefficients = juce::dsp::IIR::Coefficients<float>;
    using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients>;
    using SmoothedFloat = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    // EQ Band Filters (4 bands: LF shelf, LMF bell, HMF bell, HF shelf)
    StereoFilter lfFilter, lmfFilter, hmfFilter, hfFilter;

    juce::dsp::WaveShaper<float> saturation;
    juce::dsp::Gain<float> outputGain;

    double currentSampleRate = 44100.0;

    // WR-02: smooth each band's frequency/gain so automation and preset changes ramp
    // instead of stepping per block (zipper noise). Coefficients are rebuilt per
    // kSmoothingBlock-sample chunk while a band is moving, using the allocation-free
    // ArrayCoefficients factory (identical math to Coefficients::make*, which just
    // wraps it in a heap allocation). This keeps CR-01's steady-state path — when no
    // band is moving, coefficients are left untouched and the block runs in one pass.
    SmoothedFloat lfFreqSm,  lfGainSm;
    SmoothedFloat lmfFreqSm, lmfGainSm;
    SmoothedFloat hmfFreqSm, hmfGainSm;
    SmoothedFloat hfFreqSm,  hfGainSm;

    int lastLmfQ = -1, lastHmfQ = -1;   // discrete Q — recompute only on change
    bool coeffsInitialised = false;     // force first coefficient build after prepareToPlay

    static constexpr int   kSmoothingBlock   = 32;    // coefficient update granularity (samples)
    static constexpr float kSmoothingSeconds = 0.03f; // 30 ms parameter ramp
    static constexpr float qValues[] = { 0.5f, 1.0f, 2.0f }; // WIDE, MED, TIGHT

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconAnalogEQAudioProcessor)
};
