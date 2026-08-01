/*
   This file is part of O-Detune, an Ouaricon Audio plugin.
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

    O-Detune - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"

class ODetuneAudioProcessor : public juce::AudioProcessor
{
public:
    ODetuneAudioProcessor();
    ~ODetuneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Detune"; }
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

    // Note: no processing latency is reported. The wobble/unison ~50 ms centre
    // delay is a wet-path effect, not PDC latency — the dry path is undelayed,
    // so reporting it would misalign the dry signal. (Previous getLatencySamples()
    // override was a non-virtual no-op in JUCE 8 and never took effect.)

    // APVTS (public for editor access)
    juce::AudioProcessorValueTreeState parameters;

    // Preset management
    OuariconPresetManager presetManager;

private:
    //==============================================================================
    // DSP Components (Phase 4.1: Core Processing)

    // Processing spec
    juce::dsp::ProcessSpec spec;

    // Wobble Engine (delay-based pitch modulation)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wobbleDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wobbleDelayR;

    // Unison Engine (3 voices for Phase 4.1)
    static constexpr int maxUnisonVoices = 7;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> unisonDelaysL[maxUnisonVoices];
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> unisonDelaysR[maxUnisonVoices];

    // Focus Filter (frequency-selective processing)
    juce::dsp::IIR::Filter<float> focusHighPassL;
    juce::dsp::IIR::Filter<float> focusHighPassR;
    juce::dsp::IIR::Filter<float> focusLowPassL;
    juce::dsp::IIR::Filter<float> focusLowPassR;

    // Cached Focus cutoffs — recompute IIR coefficients only when they change
    // (avoids heap-allocating coefficient objects in processBlock every block)
    float lastFocusLow = -1.0f;
    float lastFocusHigh = -1.0f;

    // Dry/Wet Mixer
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Processing buffers (pre-allocated for real-time safety)
    juce::AudioBuffer<float> wobbleBuffer;
    juce::AudioBuffer<float> unisonBuffer;

    // State variables
    double currentSampleRate = 48000.0;
    static constexpr float centerDelayMs = 50.0f;

    // LFO state (for multi-waveform support)
    float lfoPhase = 0.0f;
    float noiseHeldValue = 0.0f;
    juce::Random random;

    // Per-voice random offsets (seeded once in prepareToPlay; scaled by random_amt)
    float voiceRandomOffsets[maxUnisonVoices] = {0};

    // Per-voice LFO phases for chorus-style unison modulation
    float voiceLfoPhases[maxUnisonVoices] = {0};

    // Pre-delay lines
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayR;

    // Parameter smoothing (50ms ramp time) — only the values actually consumed
    // per-sample in processBlock are kept here.
    juce::SmoothedValue<float> smoothedBlend;
    juce::SmoothedValue<float> smoothedWidth;
    juce::SmoothedValue<float> smoothedDelay;
    juce::SmoothedValue<float> smoothedFeedback;

    //==============================================================================
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP helper functions
    float generateLFO(float phase, int shapeType, float& noiseHeld, juce::Random& rng);
    void processWidth(float& left, float& right, float widthPercent);
    void processMonoSafe(float& left, float& right);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ODetuneAudioProcessor)
};
