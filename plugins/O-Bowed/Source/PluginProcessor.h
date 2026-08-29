/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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

    O-Bowed - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BowedMPESynthesiser.h"
#include "BowedStringVoice.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "DSP/BodyResonator.h"
#include "DSP/StereoWidthProcessor.h"
#include "DSP/SympatheticStringEngine.h"
#include "DSP/HumanizeEngine.h"
#include "OuariconPresetManager.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (via ouaricon_add_module)

class OBowedAudioProcessor : public juce::AudioProcessor
{
public:
    OBowedAudioProcessor();
    ~OBowedAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Bowed"; }
    bool acceptsMidi() const override { return true; }
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

    // Public access to APVTS for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Public access to tuning engine
    TuningEngine* getTuningEngine() { return &tuningEngine; }

    // Public access to humanize engine (voices read per-block offsets)
    const HumanizeEngine* getHumanizeEngine() const noexcept { return &humanizeEngine; }

    // Public access to preset manager for editor
    OuariconPresetManager& getPresetManager() { return presetManager; }

    // VST3 Note Expression (kTuningTypeID) — Dorico microtonal playback (Phase 24).
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    // ── v1.5.0: the UI LANGUAGE. 0 = en, 1 = fr ─────────────────────────────
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile — juce::String is not trivially copyable — so the audio-safe form
    // is an index behind the two-function codec below, while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It rides the APVTS state tree as a
    // non-parameter property instead — which on this plugin means it rides
    // through OuariconPresetManager::getStateAsXml()'s copyState(), because
    // that is the idiom this processor already uses for its own state.
    // ------------------------------------------------------------------------
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

    // Check if any synthesiser voice is active (for visualization)
    bool isAnyVoiceActive() const
    {
        for (int i = 0; i < synthesiser.getNumVoices(); ++i)
            if (synthesiser.getVoice(i)->isActive())
                return true;
        return false;
    }

private:
    juce::AudioProcessorValueTreeState parameters;
    BowedMPESynthesiser synthesiser;

    // VST3 Note Expression support (module-owned table + raw-event scratch)
    Ouaricon::NoteExpression::VST3Extensions vst3Extensions;

    // Tuning engine (processor-level, shared by all voices)
    TuningEngine tuningEngine;

    // Body resonator and stereo width (processor-level, post-voice processing)
    BodyResonator bodyResonator;
    StereoWidthProcessor stereoWidthProcessor;

    // Sympathetic string engine (processor-level)
    SympatheticStringEngine sympatheticEngine;

    // Random-walk humanization, shared across voices (processor-level)
    HumanizeEngine humanizeEngine;

    // Preset manager
    OuariconPresetManager presetManager;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Factory preset initialization
    void initializeFactoryPresets();

    // DC blocker state (per-channel first-order highpass)
    float dcBlockX[2] = { 0.0f, 0.0f };
    float dcBlockY[2] = { 0.0f, 0.0f };

    // WR-06: APVTS atomic pointers resolved once in prepareToPlay so processBlock reads
    // parameters without per-callback string-keyed map lookups on the audio thread.
    std::atomic<float>* pSympatheticAmount = nullptr;
    std::atomic<float>* pSympatheticCount  = nullptr;
    std::atomic<float>* pBodyMaterial      = nullptr;
    std::atomic<float>* pBodySize          = nullptr;
    std::atomic<float>* pWidth             = nullptr;
    std::atomic<float>* pReferencePitch    = nullptr;
    std::atomic<float>* pSympatheticDecay  = nullptr;
    std::atomic<float>* pBodyAmount        = nullptr;
    std::atomic<float>* pTuningSystem      = nullptr;
    std::atomic<float>* pOutputLevel       = nullptr;
    std::atomic<float>* pHumanizeRange[4]  = { nullptr, nullptr, nullptr, nullptr };
    std::atomic<float>* pHumanizeRate[4]   = { nullptr, nullptr, nullptr, nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBowedAudioProcessor)
};
