/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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
#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cstdint>
#include "GiParamIds.h"
#include "NoteExpression.h"   // modules/tuning/note-expression 1.1.2 (ouaricon_add_module)

// Sidechain default-active flag. The shipping build leaves it at 1; the
// gate-only CMake option OGI_GATE_SC_DEFAULT_OFF compiles 0 into the PLUGIN
// target. Consoles never inherit the plugin's compile definitions, so the
// render harness always tests the shipping default (PLAN P7 / P8).
#ifndef OGI_SC_DEFAULT_ACTIVE
 #define OGI_SC_DEFAULT_ACTIVE 1
#endif

// ============================================================================
// D9: the only input bus is the Record-in "Sidechain". On VST3 the first input
// bus is typed kAux exactly when getPluginHasMainInput() returns false
// (juce_VST3ClientExtensions.h; juce_audio_plugin_client_VST3.cpp getBusInfo).
// The note-expression module class does not override it, so this plugin-local
// subclass does.
// ============================================================================
struct GiVst3Extensions final : Ouaricon::NoteExpression::VST3Extensions
{
    bool getPluginHasMainInput() const override { return false; }
};

// ============================================================================
// O-GranulatorInstrument, Stage 1 shell.
//
// 261 APVTS parameters (GiParamIds.h), a stereo instrument output plus ONE
// optional stereo "Sidechain" input that never reaches the output, latency 0,
// and the state scaffold: root uiLanguage attribute, <Engine seed stateVersion
// modDestTableVersion/> and an empty <Slots/>. The output is silent: every
// channel is cleared, MIDI is consumed and not acted on.
//
// Threading: no listener and no audio-thread code writes a parameter. Actions
// (slot load / clear / reset, ...) are Stage 3 native functions.
// ============================================================================
class GranulatorInstrumentProcessor : public juce::AudioProcessor
{
public:
    GranulatorInstrumentProcessor();
    ~GranulatorInstrumentProcessor() override = default;

    /** D9 seam: the one place the bus set is declared. Sidechain input (active
        per OGI_SC_DEFAULT_ACTIVE) + stereo output. Any D9 fallback lands here. */
    static BusesProperties makeBuses();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;

    // ── Seed (deterministic per-grain randomness; persisted as an int64 string)
    uint64_t getSeed() const noexcept    { return seed.load (std::memory_order_relaxed); }
    void setSeed (uint64_t s) noexcept   { seed.store (s, std::memory_order_relaxed); }

    // ── UI language: a state-tree ROOT attribute, not a parameter (0 en, 1 fr, 2 zh-Hans)
    std::atomic<int> uiLanguage { 0 };
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    /** Sidechain channels from the NEGOTIATED bus (0 = no bus / disabled),
        decided in prepareToPlay. Never derived from the buffer width. */
    int getSidechainChannelCount() const noexcept { return sidechainChannels.load (std::memory_order_relaxed); }

   #if OGI_TEST_HOOKS
    /** Render harness only. Decode / resample + build the tape for slot 0...3
        (= the rec_target index) on the CALLING thread and publish it as if the
        loader worker had finished. Stage 1 stubs: return false with an error. */
    bool loadSlotSync (int slot, const juce::File& file, juce::String& error);
    bool loadSlotSync (int slot, const juce::AudioBuffer<float>& pcm, double sourceRate,
                       const juce::String& name, juce::String& error);
   #endif

private:
    void updateSidechainChannelCount() noexcept;

    GiVst3Extensions vst3Extensions;
    std::atomic<uint64_t> seed { 0 };
    std::atomic<int> sidechainChannels { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorInstrumentProcessor)
};
