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

    Reverb engines live in Source/dsp/; this class hosts them in two ring-out
    slots and keeps everything after the reverb (type EQ, CHARACTER, LOW CUT,
    wet/dry, meter).

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"
#include "dsp/FdnEngine.h"
#include "dsp/PlateEngine.h"
#include "dsp/EarlyReflections.h"

// Test hooks: defined on the render-check target only (CMakeLists.txt). A
// mutant is a deliberately broken build of one behaviour, which render-check
// --mutants uses to show that each gate fails when the thing it gates is gone.
#if OSIMPLEREVERB_TEST_HOOKS
 #define OSR_MUTANT(m) (testMutant == Mutant::m)
#else
 #define OSR_MUTANT(m) false
#endif

class OSimpleReverbAudioProcessor : public juce::AudioProcessor
{
public:
    // ==================== DSP Constants ====================

    // VU meter floor level
    static constexpr float kVuMeterFloorDB = -100.0f;

    // TYPE change: the playing slot's input falls to 0 and the slot rings out;
    // the other slot's input rises. The duck is 90 % there after kTypeDuckMs.
    // A slot that has to be taken back while it still sounds fades its OUTPUT
    // over kStealFadeMs first.
    static constexpr float kTypeDuckMs = 10.0f;
    static constexpr float kStealFadeMs = 10.0f;

    // A ringing slot is retired once its output has stayed under kRetireLevel
    // (-100 dB) for kRetireHoldMs - longer than the longest delay line, so a
    // gap between echoes is not mistaken for the end of the tail.
    static constexpr float kRetireLevel = 1.0e-5f;
    static constexpr float kRetireHoldMs = 300.0f;

    // Per-type voicing. DECAY multiplies baseT60; SIZE scales the engine's
    // lengths geometrically from sizeLo (0 %) to sizeHi (100 %), and moves
    // nothing else - the tail time holds.
    struct TypePreset {
        enum class Engine { Fdn, Plate, Spring } engine;
        float baseT60;              // mid-band RT60 in seconds at DECAY 1.0x
        float sizeLo, sizeHi;       // length scale at SIZE 0 / SIZE 100
        osr::FdnConfig fdn;         // the delay set and absorption (unused by the plate: all zero)
        float earlySpanMs;          // last early-reflection tap at size scale 1 (0 = no taps)
        float earlyLevel;           // early reflections at the slot output, re the tank
        float preDelayMs;           // ahead of the taps and the tank; not scaled by SIZE
        float eqFreq;               // Type-specific EQ frequency
        float eqGain;               // Type-specific EQ gain (dB)
        float eqQ;                  // Type-specific EQ Q
        enum class EqType { None, LowShelf, HighShelf, Peak, HighPass } eqType;
        float wetTrimDb;            // level match across types at defaults
    };

    static const TypePreset typePresets[6];

    // Which engine a type runs. Spring stays on the FDN until its own engine lands.
    static bool runsPlate(int typeIndex) { return typePresets[typeIndex].engine == TypePreset::Engine::Plate; }

    // Engine length scale for a SIZE in percent.
    static float sizeScale(const TypePreset& preset, float sizePercent)
    {
        return preset.sizeLo * std::pow(preset.sizeHi / preset.sizeLo, juce::jlimit(0.0f, 1.0f, sizePercent / 100.0f));
    }

#if OSIMPLEREVERB_TEST_HOOKS
    enum class Mutant {
        none,
        decayDead,      // DECAY does not reach the engine
        sizeIsDecay,    // SIZE scales the decay time as well as the lengths
        sizeDead,       // SIZE does not reach the engine
        sameDelays,     // every FDN type runs Hall's delay set
        earlyMuted,     // early reflections at level 0
        monoTail,       // right output = left output
        noRingOut,      // a TYPE change clears the old slot instead of letting it ring
        noNanGuard,     // a non-finite slot output is passed on
        noChunking,     // an oversized host block is processed in one piece
        noGlide,        // SIZE and DECAY land at once instead of gliding
        hardSteal,      // a sounding slot is taken back without its output fade
        noShimmer,      // the plate's cross-feeds bypass the octave shifter
        decayStuck      // the engine is asked for an endless tail (the plate sits on its decay ceiling)
    };
    Mutant testMutant = Mutant::none;
    bool testInjectNaN = false;     // poisons the playing slot's input once, then clears itself
#endif

    OSimpleReverbAudioProcessor();
    ~OSimpleReverbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // WR-05: constrain layouts to mono/stereo. The default returns true for
    // anything, letting surround hosts negotiate >2-channel layouts that the
    // reverb (stereo-max) would silently pass through dry.
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
    double getTailLengthSeconds() const override { return 15.0; }   // Ambient at DECAY 2.0x: 14 s, plus pre-delay

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

    // === Reverb slots ===
    // Two of them. One plays the current TYPE. On a TYPE change its input
    // ducks to zero and it keeps running - the old tail rings out under its
    // own type, size, decay and EQ - while the other slot starts the new type
    // from silence. Clearing the tank at the switch would cut the tail on
    // every type and preset change. A third change while both still sound
    // takes the older slot back behind a fast output fade.
    struct Slot {
        enum class State { idle, playing, ringing, stolen };

        // One engine of each kind; only the one the slot's type runs is processed
        osr::FdnEngine fdn;
        osr::PlateEngine plate;
        osr::EarlyReflections early;
        osr::RingDelay preDelayL, preDelayR;
        // Type EQ: one per slot, so a ringing tail keeps its own type's EQ
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> eq;
        juce::AudioBuffer<float> out;   // the slot's output for one chunk, bus-wide

        State state = State::idle;
        int type = -1;
        int preDelaySamples = 1;
        float inputGain = 0.0f;         // the duck: rises to 1 while playing, falls to 0 otherwise
        float inputLead = 0.0f;         //   its first pole (see renderSlot)
        float outputGain = 1.0f;        // the steal fade's position: 1, except while a stolen slot fades
        float trim = 1.0f;              // the type's wetTrimDb, at the INPUT (see renderSlot)
        float earlyLevel = 0.0f;
        int quietSamples = 0;           // how long the output has been under kRetireLevel
    };
    std::array<Slot, 2> slots;
    int currentSlot = 0;                // the slot that plays, or is about to play, the current TYPE

    void startSlot(Slot& slot, int typeIndex, float decayValue, float sizeValue, bool inputOpen);
    void clearSlot(Slot& slot);
    void driveSlot(Slot& slot, float decayValue, float sizeValue);
    void renderSlot(Slot& slot, const float* inL, const float* inR, int numSamples);
    void setTypeEq(Slot& slot, int typeIndex);

    // User LOW CUT (a high-pass, 20-400 Hz; the LPFREQ/LPON IDs are historical).
    // v1.12.0: always runs, into lowCutBuffer, so ON/OFF is a crossfade rather
    // than a switch and the filter state is never stale. A TPT SVF because its
    // cutoff is modulated (smoothed) every kCharacterSubBlock samples.
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;
    juce::AudioBuffer<float> lowCutBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> lowCutFreqSmoothed;
    juce::SmoothedValue<float> lowCutMixSmoothed;

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
    double currentSampleRate = 44100.0;
    int maxChunkSamples = 512;             // the prepared block size: nothing below is sized for more
    float duckCoeff = 1.0f;                // of each of the duck's two poles
    float stealStep = 0.0f;                // per sample, for kStealFadeMs
    int retireHoldSamples = 0;
    float previousCharacterValue = 0.0f;   // seeded out of range in prepareToPlay

    // Helper methods
    void processChunk(juce::AudioBuffer<float>& buffer, int start, int numSamples);
    static float wetTrimGain(int typeIndex) { return juce::Decibels::decibelsToGain(typePresets[typeIndex].wetTrimDb); }
    void initializeFactoryPresets();

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
