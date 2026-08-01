/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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

    PluginProcessor.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "dsp/GlottalWavetable.h"
#include "dsp/LyricsEngine.h"
#include "dsp/DelayProcessor.h"
#include "dsp/EQProcessor.h"
#include "dsp/ReverbProcessor.h"
#include "OuariconPresetManager.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "TuningExporter.h"
#include "EmbeddedTunings.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OFormantAudioProcessor : public juce::AudioProcessor
{
public:
    OFormantAudioProcessor();
    ~OFormantAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Formant"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }
    OuariconPresetManager& getPresetManager() { return presetManager; }

    TuningEngine tuningEngine;
    ScaleGenerator scaleGenerator;
    TuningExporter tuningExporter;
    LyricsEngine lyricsEngine;

    LyricsEngine& getLyricsEngine() { return lyricsEngine; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

private:
    // DSP: Shared wavetable (generated once, read-only across all voices)
    GlottalWavetable glottalWavetable;
    double lastSampleRate = 0.0;

    // Post-synth output gain (dB -> linear, smoothed)
    juce::SmoothedValue<float> outputGainSmoothed { 1.0f };

    // Effects chain (Chorus -> Delay -> Reverb -> EQ)
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delayProcessor;
    ReverbProcessor reverbProcessor;
    EQProcessor eqProcessor;

    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;
    juce::MPESynthesiser synthesiser;

    // Cached raw-parameter pointers for the effects/output chain (IN-02).
    // Fetched once in prepareToPlay so processBlock avoids ~22 string-keyed
    // APVTS hash lookups per block. Pointers are stable for the processor's
    // lifetime. Grouped by effect to mirror the processBlock stages.
    struct EffectParamPtrs
    {
        std::atomic<float>* chorusBypass  = nullptr;
        std::atomic<float>* chorusMix     = nullptr;
        std::atomic<float>* chorusRate    = nullptr;
        std::atomic<float>* chorusDepth   = nullptr;

        std::atomic<float>* delayBypass   = nullptr;
        std::atomic<float>* delayMix      = nullptr;
        std::atomic<float>* delayTime     = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMode     = nullptr;

        std::atomic<float>* reverbBypass   = nullptr;
        std::atomic<float>* reverbMix      = nullptr;
        std::atomic<float>* reverbSize     = nullptr;
        std::atomic<float>* reverbDamp     = nullptr;
        std::atomic<float>* reverbPredelay = nullptr;
        std::atomic<float>* reverbMod      = nullptr;
        std::atomic<float>* reverbShimmer  = nullptr;

        std::atomic<float>* eqBypass   = nullptr;
        std::atomic<float>* eqLowGain  = nullptr;
        std::atomic<float>* eqMidGain  = nullptr;
        std::atomic<float>* eqMidFreq  = nullptr;
        std::atomic<float>* eqHighGain = nullptr;

        std::atomic<float>* outputGain = nullptr;
    };
    EffectParamPtrs fxParams;

    void cacheParamPointers();

    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OFormantAudioProcessor)
};
