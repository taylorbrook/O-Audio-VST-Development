/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): a silent 16-voice synth shell with the complete
    21-parameter APVTS, a MidiMessageCollector for the Stage 3 on-screen
    keyboard, and APVTS-XML state carrying the uiLanguage property and a
    strip-on-load IMPORTED_BANK stub.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiMessageCollector (juce_audio_devices)
#include <array>
#include <atomic>
#include "WtVoice.h"

//==============================================================================
// Parameter IDs - the exact snake_case strings of parameter-spec.md (LOCKED).
// C++ identifiers are camelCase so none can shadow a juce:: free function.
namespace OSimpleWavetable::ParamIDs
{
    inline constexpr auto bank        = "bank";
    inline constexpr auto position    = "position";
    inline constexpr auto interp      = "interp";
    inline constexpr auto bandlimit   = "bandlimit";
    inline constexpr auto bitDepth    = "bit_depth";
    inline constexpr auto lfoRate     = "lfo_rate";
    inline constexpr auto lfoSync     = "lfo_sync";
    inline constexpr auto lfoDiv      = "lfo_div";
    inline constexpr auto lfoShape    = "lfo_shape";
    inline constexpr auto lfoDepth    = "lfo_depth";
    inline constexpr auto menvAttack  = "menv_attack";
    inline constexpr auto menvDecay   = "menv_decay";
    inline constexpr auto menvSustain = "menv_sustain";
    inline constexpr auto menvRelease = "menv_release";
    inline constexpr auto envAmount   = "env_amount";
    inline constexpr auto ampAttack   = "amp_attack";
    inline constexpr auto ampDecay    = "amp_decay";
    inline constexpr auto ampSustain  = "amp_sustain";
    inline constexpr auto ampRelease  = "amp_release";
    inline constexpr auto voiceMode   = "voice_mode";
    inline constexpr auto outputLevel = "output_level";

    // Spec order - single source of truth for the count/order gate (and Stage 3 loops).
    inline constexpr std::array<const char*, 21> all {
        bank, position, interp, bandlimit, bitDepth, lfoRate, lfoSync, lfoDiv, lfoShape,
        lfoDepth, menvAttack, menvDecay, menvSustain, menvRelease, envAmount,
        ampAttack, ampDecay, ampSustain, ampRelease, voiceMode, outputLevel };
}

//==============================================================================
class OSimpleWavetableAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleWavetableAudioProcessor();
    ~OSimpleWavetableAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleWavetable"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }   // amp_release max

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Mockup contract (mockups/v1-PluginEditor.cpp calls exactly these names).
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // On-screen keyboard: the editor injects note on/off from the WebView (any
    // thread). Queued via the MidiMessageCollector and merged into processBlock.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    // Interface language of the WebView UI. NOT a parameter (no automation
    // lane, no preset can change it). Runtime form = index, persisted form =
    // the ASCII language code (XML round trip rebuilds properties as strings).
    std::atomic<int> uiLanguage { 0 };

    // Codec. languageIndex() maps anything that is neither "fr" nor "zh-Hans"
    // to 0, so a hand-edited session degrades to English (allow-list).
    static juce::String languageCode  (int i)                 { return i == 2 ? "zh-Hans" : i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "zh-Hans" ? 2 : s == "fr" ? 1 : 0; }

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;   // declared BEFORE synth/collector (ctor init order)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int kNumVoices = 16;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;

    //==========================================================================
    // State helpers. The IMPORTED_BANK child is written only from
    // processor-owned data (Stage 2.4) and is never kept in the live APVTS
    // tree: it is stripped before replaceState and from every copyState().
    static constexpr const char* kImportedBankTag = "IMPORTED_BANK";
    static constexpr const char* kUiLanguageProp  = "uiLanguage";

    void restoreImportedBank (const juce::ValueTree& childOrInvalid);   // Stage 1: no-op stub
    void writeImportedBank   (juce::ValueTree& state) const;            // Stage 1: writes nothing

    static void stripImportedBank (juce::ValueTree& tree)
    {
        // ALL copies: a hand-edited or legacy blob may hold two.
        for (auto c = tree.getChildWithName (kImportedBankTag); c.isValid();
                  c = tree.getChildWithName (kImportedBankTag))
            tree.removeChild (c, nullptr);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleWavetableAudioProcessor)
};
