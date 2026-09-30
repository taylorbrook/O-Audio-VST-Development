/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
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

    O-SimpleReverb - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    v1.13.0 - Real flutter + octave shimmer, DECAY headroom map (see CHANGELOG)

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"
#include "ModulationFx.h"

class OSimpleReverbAudioProcessor : public juce::AudioProcessor
{
public:
    // ==================== DSP Constants ====================
    // These named constants replace magic numbers for clarity and tuneability

    // Delay line sizing (ms; converted at the running rate in prepareToPlay).
    // Each must cover the longest delay any TypePreset asks for:
    static constexpr float kMaxPreDelayMs = 50.0f;          // Hall pre-delay
    static constexpr float kMaxEarlyReflectionMs = 62.0f;   // 23 ms x 2.5 (Ambient) x 1.07 (R offset) = 61.5 ms
    static constexpr float kMaxAllPassMs = 4.0f;            // 3.7 ms x 1.05 (R offset) = 3.9 ms

    // Stereo offsets for spatial separation
    static constexpr float kEarlyReflectionStereoOffset = 1.07f;  // 7% L/R offset
    static constexpr float kAllPassStereoOffset = 1.05f;          // 5% L/R offset

    // Plate shimmer: the octave-up voice's level against the dry chain (v1.13.0)
    static constexpr float kShimmerMixAmount = 0.3f;         // about -10 dB

    // Early reflection base times (ms) - prime numbers for natural sound
    static constexpr std::array<float, 4> kBaseEarlyDelaysMs = { 7.0f, 11.0f, 17.0f, 23.0f };

    // VU meter floor level
    static constexpr float kVuMeterFloorDB = -100.0f;

    // TYPE change (v1.12.0): the pre-reverb chain ducks to silence on the OLD
    // type's delays, swaps, and comes back; the post-reverb type EQ crossfades.
    static constexpr float kTypeDuckMs = 10.0f;
    static constexpr float kTypeEqFadeMs = 30.0f;


    OSimpleReverbAudioProcessor();
    ~OSimpleReverbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // WR-05: constrain layouts to mono/stereo. The default returns true for
    // anything, letting surround hosts negotiate >2-channel layouts that the
    // reverb (juce::dsp::Reverb is stereo-max) would silently pass through dry.
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        auto out = layouts.getMainOutputChannelSet();
        return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
            && layouts.getMainInputChannelSet() == out;
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-SimpleReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;

    // VU Meter - linear output peak HELD until the editor reads it. The audio
    // thread raises it (max), the editor timer exchange()s it back to 0, so a
    // peak in any block between two 30 Hz reads reaches the meter.
    std::atomic<float> outputPeak { 0.0f };

    // ------------------------------------------------------------------------
    // v1.6.0 - the UI language. 0 = en, 1 = fr, 2 = zh-Hans.
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

    /** The codec. languageIndex() maps anything that is neither "fr" nor
        "zh-Hans" to 0, so a hand-edited session or an unexpected argument from
        the page degrades to English rather than being stored unvalidated.

        PURE ASCII, AND THAT IS THE CONTRACT. Not one Han character exists
        anywhere under Source/ — every Chinese string lives in the UI table
        (Source/ui/public/js/i18n.js), and the one Han string in the markup is
        written as numeric character references. What is persisted here is a
        language CODE, which is an ASCII identifier in every language. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components
    juce::dsp::Reverb reverb;
    // CHARACTER = two filters that ALWAYS run and never change family: a Warm
    // low-pass (parked near Nyquist unless warm) into a Bright high shelf (0 dB
    // unless bright). Switching one biquad between low-pass and shelf, or
    // resetting it, clicked at every crossing of centre. The low-pass is a TPT
    // state-variable filter: a biquad parked near Nyquist has poles by z = -1
    // and its state explodes on the next coefficient change.
    juce::dsp::StateVariableTPTFilter<float> warmFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> brightFilter;
    juce::SmoothedValue<float> characterSmoothed;
    static constexpr int kCharacterSubBlock = 32;  // LOW CUT cutoff update interval (samples)
    void updateCharacterCoefficients(float characterValue);

    // === Type-Specific DSP Components (v1.1.0) ===

    // Pre-delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR;

    // Early reflection comb filters (4 per channel for density)
    static constexpr int numEarlyReflections = 4;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numEarlyReflections> earlyReflectionsL;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numEarlyReflections> earlyReflectionsR;
    std::array<float, numEarlyReflections> earlyReflectionGains = { 0.7f, 0.5f, 0.35f, 0.25f };

    // Schroeder all-pass diffusers for Spring dispersion (flat magnitude, chirpy phase)
    static constexpr int numAllPassFilters = 3;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numAllPassFilters> allPassL;
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, numAllPassFilters> allPassR;
    std::array<float, numAllPassFilters> allPassDelayMs = { 1.5f, 2.3f, 3.7f };  // Prime-ish ratios
    float allPassCoeff = 0.6f;  // Feedback coefficient

    // Pitch flutter (Spring) and slow tail movement (Hall/Ambient): a swept
    // short delay per channel, driven by one shared sine (v1.13.0)
    float lfoPhase = 0.0f;
    float lfoIncrement = 0.0f;
    FlutterDelay flutterL, flutterR;

    // Type-specific EQ: two instances, crossfaded over kTypeEqFadeMs on a type
    // change (a coefficient jump on the running tail clicked). eqMix ramps the
    // idle instance in; at 1 it becomes the active one.
    std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>, 2> typeEq;
    int eqActive = 0;
    int eqType = -1;            // type whose EQ typeEq[eqActive] holds
    float eqMix = 0.0f;         // 0 = no crossfade running
    float eqMixStep = 0.0f;
    juce::AudioBuffer<float> eqFadeBuffer;
    void setTypeEq(int slot, int typeIndex);

    // User LOW CUT (a high-pass, 20-400 Hz; the LPFREQ/LPON IDs are historical).
    // v1.12.0: always runs, into lowCutBuffer, so ON/OFF is a crossfade rather
    // than a switch and the filter state is never stale. A TPT SVF because its
    // cutoff is modulated (smoothed) every kCharacterSubBlock samples.
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;
    juce::AudioBuffer<float> lowCutBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> lowCutFreqSmoothed;
    juce::SmoothedValue<float> lowCutMixSmoothed;

    // Plate shimmer: a real octave-up voice (v1.13.0; was a 1.5 kHz ring modulator)
    OctaveUpShifter shimmerL, shimmerR;

    // Pre-allocated buffers (sized in prepareToPlay — CR-04)
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> wetBuffer;

    // Wet/dry gain smoothing (WR-03, 20ms linear ramp)
    juce::SmoothedValue<float> wetGainSmoothed;
    juce::SmoothedValue<float> dryGainSmoothed;

    // Cached parameter pointers (never change after construction)
    std::atomic<float>* typeParam = nullptr;
    std::atomic<float>* characterParam = nullptr;
    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* wetParam = nullptr;
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* lpFreqParam = nullptr;
    std::atomic<float>* lpOnParam = nullptr;

    // State tracking
    // TYPE duck-and-swap: the chain runs activeType; when the parameter moves,
    // chainGain falls to 0 on the old settings, the chain switches, then rises.
    int activeType = -1;
    float chainGain = 1.0f;
    float chainGainStep = 0.0f;
    double currentSampleRate = 44100.0;
    float previousCharacterValue = 0.0f;   // seeded out of range in prepareToPlay

    // Type preset structure (expanded)
    struct TypePreset {
        float baseRoomSize;
        float baseDamping;
        float width;
        float preDelayMs;           // Pre-delay in milliseconds
        float earlyReflectionScale; // Scale factor for early reflection times
        float earlyReflectionMix;   // How much early reflections to mix in
        float modRate;              // flutter rate in Hz (0 = no modulation)
        float modCents;             // flutter depth: peak pitch deviation (v1.13.0)
        bool useAllPass;            // Enable all-pass dispersion (Spring)
        bool useShimmer;            // Enable shimmer effect (Plate)
        float eqFreq;               // Type-specific EQ frequency
        float eqGain;               // Type-specific EQ gain (dB)
        float eqQ;                  // Type-specific EQ Q
        enum class EqType { None, LowShelf, HighShelf, Peak, HighPass } eqType;
        float wetTrimDb;            // v1.12.0: level match to Room at defaults
    };

    static const TypePreset typePresets[6];

    // Helper methods
    void switchChainTo(int typeIndex);
    static float wetTrimGain(int typeIndex) { return juce::Decibels::decibelsToGain(typePresets[typeIndex].wetTrimDb); }
    float processAllPassChain(float input, bool isLeft);
    void initializeFactoryPresets();

    // Template helpers to reduce initialization code duplication
    template<typename DelayContainer>
    void prepareDelayContainer(DelayContainer& delays, const juce::dsp::ProcessSpec& spec, int maxDelaySamples) {
        for (auto& delay : delays) {
            delay.setMaximumDelayInSamples(maxDelaySamples);
            delay.prepare(spec);
            delay.reset();
        }
    }

    template<typename FilterType>
    void prepareFilterAsAllPass(FilterType& filter, const juce::dsp::ProcessSpec& spec, double sampleRate) {
        filter.prepare(spec);
        filter.reset();
        // ArrayCoefficients assignment primes the state's coefficient storage so
        // later audio-thread coefficient updates never reallocate (CR-03)
        *filter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(sampleRate, 1000.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSimpleReverbAudioProcessor)
};
