/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation) shell: the frozen 196-parameter layout in 25 parameter
    groups, the host bypass parameter, symmetric mono / stereo buses, bit-exact
    pass-through audio and the v1 state format. No DSP (Stage 2), no WebView
    editor (Stage 3), no presets (Stage 4).

    Contract: .planning/parameter-spec.md. IDs, order, ranges and skews are
    frozen from this stage on.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>

// vu-meter 1.0.0 compile proof (Stage 1 plan, P6): the header is included so
// that its first build inside a plugin happens now. No member is declared;
// the meter itself is mounted in Stage 3.
#include "VUMeterBridge.h"

class OuariconParametricEQAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconParametricEQAudioProcessor();
    ~OuariconParametricEQAudioProcessor() override = default;

    static constexpr int kNumBands = 24;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    /** Mono -> mono and stereo -> stereo only. The base class accepts every
        layout a wrapper enumerates, asymmetric ones included. */
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** JUCE's own VST3 and AU wrappers never call this while a bypass
        parameter exists; it is routed to the same path for hosts that do. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    /** The `bypass` parameter. */
    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    /** 196 parameters. State tree type "OParametricEQ". */
    juce::AudioProcessorValueTreeState parameters;

    //==========================================================================
    // Session UI state. NOT host parameters, not in APVTS, not part of presets.
    // Atomics because a host may restore state off the message thread.
    //
    // uiLanguage is persisted as a ROOT attribute of the state (a language
    // code); the other five live in the state's `UI` child.

    /** 0 = en, 1 = fr, 2 = zh-Hans. */
    std::atomic<int> uiLanguage { 0 };

    /** 0 Off, 1 Pre, 2 Post, 3 Pre+Post. */
    std::atomic<int> analyzerMode { 2 };

    /** Plus / minus dB shown by the response display: 6, 12 or 30. */
    std::atomic<int> uiDisplayRange { 12 };

    /** 0 = none, 1 ... 24. */
    std::atomic<int> uiSelectedBand { 0 };

    /** Editor size, 800 x 500 ... 1600 x 1000. */
    std::atomic<int> uiEditorWidth { 960 };
    std::atomic<int> uiEditorHeight { 600 };

    /** The language codec. Pure ASCII by contract: what is persisted is a
        language code. Anything that is neither "fr" nor "zh-Hans" maps to 0,
        so a hand-edited session degrades to English and is never stored raw. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Stage 1 audio path, shared by processBlock and processBlockBypassed. */
    void passThrough (juce::AudioBuffer<float>&);

    juce::AudioProcessorParameter* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuariconParametricEQAudioProcessor)
};
