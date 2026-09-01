/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    O-Freeze - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class OFreezeAudioProcessor : public juce::AudioProcessor
{
public:
    OFreezeAudioProcessor();
    ~OFreezeAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Freeze"; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // ------------------------------------------------------------------------
    // v2.1.0 — the UI language. 0 = en, 1 = fr.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile (juce::String is not trivially copyable), so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

private:
    // Gate state machine
    enum class GateState { Idle, Frozen };

    // Grain structure for granular synthesis
    struct Grain
    {
        int startSample = 0;      // Position within grain (0 to grainSize)
        int position = 0;         // Base position in freeze buffer (set at activation, fixed)
        int jitterOffset = 0;     // Per-grain random position offset (decorrelates overlapping grains)
        float playbackRate = 1.0f;        // Pitch micro-detuning rate multiplier
        // Fractional read offset from base position. DOUBLE, not float (v2.3.0):
        // a float accumulator at 44100 samples (1000 ms grain) has an ulp of
        // 2^-8, so adding a rate of 1.004 per sample rounds the rate itself to a
        // multiple of 1/512 -- every grain's pitch snapped to a 3.38 ct grid,
        // measured. The knob claims 0.1 ct.
        double fractionalPosition = 0.0;
        bool active = false;      // Grain active flag
    };

    // DSP Components (declared BEFORE parameters for initialization order)
    juce::AudioBuffer<float> freezeBuffer;
    int writePosition = 0;
    bool bufferFrozen = false;
    double currentSampleRate = 44100.0;
    juce::LinearSmoothedValue<float> freezeGain;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Threshold gate components
    GateState gateState = GateState::Idle;
    float rmsLevel = 0.0f;
    std::vector<float> rmsBuffer;
    int rmsWriteIndex = 0;
    int rmsSamplesPerWindow = 0;

    // Granular synthesis components
    static constexpr int MAX_GRAINS = 32;
    int numGrains = 8;         // Active grain count (from GRAIN_COUNT parameter)
    int lastGrainCount = 8;    // Change detection
    std::array<Grain, MAX_GRAINS> grains;
    std::vector<float> hannWindow;
    int grainSize = 0;
    int maxGrainSize = 0;
    float lastGrainSizeMs = 400.0f;
    int grainTriggerInterval = 0;
    int nextTriggerInterval = 0;  // Jittered interval for next grain trigger
    int grainTriggerCounter = 0;
    int nextGrainIndex = 0;
    juce::Random random;
    bool stopTriggeringNewGrains = false;  // Soft release: let active grains complete

    // Per-sample grain scratch buffers (class members to avoid stack zero-init each sample)
    float windowValues[MAX_GRAINS] {};
    int grainPos0[MAX_GRAINS] {};
    int grainPos1[MAX_GRAINS] {};
    float grainFrac[MAX_GRAINS] {};

    // WSOLA overlap matching (best-overlap grain positioning)
    static constexpr int WSOLA_TAIL_SIZE = 64;
    std::array<float, WSOLA_TAIL_SIZE> lastGrainTail{};
    bool hasLastGrainTail = false;

    // Drift offset (locked at freeze engage, shared by all grains)
    float frozenDriftOffset = 0.0f;    // Offset locked at freeze moment (0 to 1)
    juce::LinearSmoothedValue<float> smoothedDriftOffset; // Smoothed drift in samples (prevents clicks)

    // LFO state (computed per-block for efficiency)
    float lfoPhase = 0.0f;            // 0 to 1 phase accumulator
    float lfoCurrentValue = 0.0f;     // Current LFO output (-1 to +1)
    float randomHoldValue = 0.0f;     // Sample-and-hold value for Random shape

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreezeAudioProcessor)
};
