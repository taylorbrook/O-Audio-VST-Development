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

    Stage 1 (Foundation): the complete 21-parameter APVTS, a
    MidiMessageCollector for the Stage 3 on-screen keyboard, and APVTS-XML
    state carrying the uiLanguage property and a strip-on-load
    IMPORTED_BANK stub.

    Stage 2.2 (Voice + Oscillator): 16-voice Poly / true-legato Mono
    wavetable engine over the shared BuiltInBanks, per-block BlockContext
    (D-H), oversized-block chunk loop with sliced MIDI, seeded output stage,
    lead-voice display atomics. Imported (bank 5) renders silence until 2.4
    publishes into importedForAudio.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiMessageCollector (juce_audio_devices)
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
#include "BuiltInBanks.h"
#include "MonoStack.h"
#include "WavetableBank.h"
#include "WtSynthesiser.h"
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

    //==========================================================================
    // Lead-voice display snapshot (Stage 3 viz seam). Written by the audio
    // thread once per block (newest note among sounding voices), relaxed.
    float getDisplayPosition() const noexcept { return dispPos.load (std::memory_order_relaxed); }
    int   getDisplayLevel() const noexcept    { return dispLevel.load (std::memory_order_relaxed); }
    int   getDisplayFrame() const noexcept    { return dispFrame.load (std::memory_order_relaxed); }
    bool  isDisplaySounding() const noexcept  { return dispSounding.load (std::memory_order_relaxed); }

    // Built-in banks (immutable; shared by all instances). Message thread
    // readers (Stage 3 cycle view / thumbnails) may use this freely.
    const BuiltInBanks& getBuiltInBanks() const noexcept { return *builtIns; }

    static constexpr int kNumVoices   = 16;
    static constexpr int kNumBanks    = 6;    // 0..4 built-in, 5 Imported
    static constexpr int kImportedIdx = 5;

   #if OSIW_TEST_HOOKS
    // Test-only accessors (console targets). Call from the thread that runs
    // processBlock (the harness's main thread).
    int getSoundingVoiceCountForTesting() const noexcept
    {
        int n = 0;
        for (const auto* v : wtVoices)
            if (v != nullptr && v->isSounding())
                ++n;
        return n;
    }

    const WtVoice* getVoiceForTesting (int i) const noexcept
    {
        return (i >= 0 && i < kNumVoices) ? wtVoices[(size_t) i] : nullptr;
    }
   #endif

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;   // declared BEFORE synth/collector (ctor init order)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Declared before anything that reads it. Built on the message thread
    // (first instance) inside JUCE's SpinLock; never on the audio thread.
    juce::SharedResourcePointer<BuiltInBanks> builtIns;

    WtSynthesiser synth;   // alloc-free findVoiceToSteal (see WtSynthesiser.h)
    juce::MidiMessageCollector midiCollector;

    //==========================================================================
    // Stage 2.2 engine state (audio thread unless noted).

    // 2.4 publishes the imported bank here (seq_cst store); nullptr = the
    // Imported bank is empty and renders exact silence.
    std::atomic<const WavetableBank*> importedForAudio { nullptr };

    // Raw parameter pointers (cached in the ctor; atomic loads only).
    std::atomic<float>* pBank       = nullptr;
    std::atomic<float>* pPosition   = nullptr;
    std::atomic<float>* pInterp     = nullptr;
    std::atomic<float>* pBandlimit  = nullptr;
    std::atomic<float>* pBitDepth   = nullptr;
    std::atomic<float>* pAmpAttack  = nullptr;
    std::atomic<float>* pAmpDecay   = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;
    std::atomic<float>* pVoiceMode  = nullptr;
    std::atomic<float>* pOutput     = nullptr;

    std::array<WtVoice*, (size_t) kNumVoices> wtVoices {};   // owned by synth

    BlockContext blockCtx;                  // pointer handed to every voice (stable)
    std::vector<float> knobBuf, zeroBuf;    // [preparedBlock], sized in prepareToPlay
    juce::MidiBuffer chunkMidi, wheelMidi;  // ensureSize()d in prepareToPlay

    juce::SmoothedValue<float> outputGain { 1.0f };
    juce::SmoothedValue<float> knobSmooth { 0.0f };   // 20 ms position knob (D-D)

    MonoStack monoStack;
    int lastVoiceMode = 0;
    int preparedBlock = 512;

    std::atomic<float> dispPos      { 0.0f };
    std::atomic<int>   dispLevel    { 0 };
    std::atomic<int>   dispFrame    { 0 };
    std::atomic<bool>  dispSounding { false };

    //==========================================================================
    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples);
    void renderMono (juce::AudioBuffer<float>& view, const juce::MidiBuffer& chunk, int numSamples);
    void updateDisplayFromLeadVoice() noexcept;
    const WavetableBank* resolveBank (int bankIndex) const noexcept;
    juce::ADSR::Parameters currentAmpParams() const noexcept;
    float currentKnob() const noexcept;
    float currentOutputGain() const noexcept;

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
