/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

    O-Wind - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FluteSynthSound.h"
#include "FluteSynthVoice.h"
#include "DSP/StereoWidth.h"
#include "DSP/InstrumentPresets.h"
#include "DSP/DelayProcessor.h"
#include "DSP/EQProcessor.h"
#include "DSP/ReverbProcessor.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "OuariconPresetManager.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OWindAudioProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    OWindAudioProcessor();
    ~OWindAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Wind"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public access to APVTS for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Public access to tuning engine
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // Public access to preset manager for editor
    OuariconPresetManager& getPresetManager() { return presetManager; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback.
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // ------------------------------------------------------------------------
    // v1.17.0 — UI LANGUAGE
    //
    // 0 = en, 1 = fr. Atomic because the editor's native functions read and
    // write it from the message thread while getStateInformation() may be
    // called from another; the RUNTIME form is an index behind the two-function
    // codec below while the PERSISTED form stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead — which on this plugin means it rides
    // through OuariconPresetManager::getStateAsXml()'s copyState(), because that
    // is the idiom this processor already uses for its own state.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synthesiser;

    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

    // Tuning engine (processor-level, shared by all voices)
    TuningEngine tuningEngine;

    // Post-voice stereo width processor
    StereoWidthProcessor stereoWidth;

    // Post-width headjoint formant resonance filter (stereo)
    juce::dsp::IIR::Filter<float> formantFilterL, formantFilterR;
    float lastFormantGainDb = 0.0f;
    float lastFormantCenterHz = 2500.0f;
    int lastFormantPresetIndex = -1;

    // v1.14.0: Effects chain (Chorus -> Delay -> Reverb -> EQ)
    juce::dsp::Chorus<float> chorus;
    DelayProcessor delay;
    EQProcessor eq;
    ReverbProcessor reverbProcessor;

    // FX tail-out: when a mix knob hits 0 the effect keeps processing for a
    // bounded window so delay/reverb content decays naturally instead of
    // freezing in the buffers (and replaying stale audio when mix rises again)
    int chorusTailRemaining = 0;
    int delayTailRemaining = 0;
    int reverbTailRemaining = 0;

    // v1.14.0: Effects parameter cache for real-time access
    struct EffectsParamCache {
        std::atomic<float>* chorusBypass = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* delayBypass = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMode = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* eqBypass = nullptr;
        std::atomic<float>* eqLowGain = nullptr;
        std::atomic<float>* eqMidGain = nullptr;
        std::atomic<float>* eqMidFreq = nullptr;
        std::atomic<float>* eqHighGain = nullptr;
        std::atomic<float>* reverbBypass = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamp = nullptr;
        std::atomic<float>* reverbPredelay = nullptr;
        std::atomic<float>* reverbMix = nullptr;
        std::atomic<float>* reverbMod = nullptr;
        std::atomic<float>* reverbShimmer = nullptr;
        // Post-voice per-block reads (IN-01: no string-keyed lookups in processBlock)
        std::atomic<float>* width = nullptr;
        std::atomic<float>* formant = nullptr;
        std::atomic<float>* instrumentPreset = nullptr;
    } fxCache;

    // Preset manager (OuariconPresetManager)
    OuariconPresetManager presetManager;

    // APVTS parameter change callback
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Factory preset initialization
    void initializeFactoryPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OWindAudioProcessor)
};
