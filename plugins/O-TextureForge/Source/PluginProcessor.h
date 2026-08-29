/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    O-TextureForge - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "dsp/SharedCorpus.h"
#include "dsp/CorpusLoader.h"
#include "dsp/GrainScheduler.h"
#include <memory>
#include <atomic>

struct VizSnapshot
{
    struct ActiveGrain
    {
        uint32_t grainIndex;
        float envelope;
        float readPositionNorm;
    };

    int activeCount = 0;
    std::array<ActiveGrain, 64> activeGrains;
    float cursorX = 0.5f;
    float cursorY = 0.5f;
};

class TextureForgeProcessor : public juce::AudioProcessor
{
public:
    TextureForgeProcessor();
    ~TextureForgeProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-TextureForge"; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    const VizSnapshot& getVizSnapshot() const
    {
        return vizSnapshots[static_cast<size_t>(1 - vizWriteIndex.load(std::memory_order_acquire))];
    }

    // Corpus loading
    void loadCorpusFile(const juce::File& file);
    void cancelUmap() { corpusLoader.cancelUmap(); }

    // UI interaction
    std::shared_ptr<SharedCorpus> getCurrentCorpus() const { return currentCorpus; }
    void selectGrainFromUI(int grainIndex);

    juce::AudioProcessorValueTreeState parameters;

    // =========================================================================
    // v1.1.0 — the UI language. 0 = en, 1 = fr.
    //
    // An INDEX rather than a string because std::atomic<juce::String> does not
    // compile (juce::String is not trivially copyable), so the audio-safe form
    // is an index behind the two-function codec below while the PERSISTED form
    // stays a readable language code.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a preset must not be able to change which language
    // somebody reads their plugin in. It is saved beside the corpus path as a
    // plain XML child element instead — this file's own idiom for the one piece
    // of non-parameter state it already had.
    // =========================================================================
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached parameter pointers (real-time safe)
    std::atomic<float>* energyParam = nullptr;
    std::atomic<float>* brightnessParam = nullptr;
    std::atomic<float>* textureParam = nullptr;
    std::atomic<float>* positionParam = nullptr;
    std::atomic<float>* grainDensityParam = nullptr;
    std::atomic<float>* grainSizeParam = nullptr;
    std::atomic<float>* scatterXParam = nullptr;
    std::atomic<float>* scatterYParam = nullptr;
    std::atomic<float>* variationParam = nullptr;
    std::atomic<float>* crossfadeParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* midiModeParam = nullptr;

    // Visualization double-buffer (lock-free audio->GUI)
    std::array<VizSnapshot, 2> vizSnapshots {};
    std::atomic<int> vizWriteIndex { 0 };

    double currentSampleRate = 44100.0;

    // DSP components
    GrainScheduler grainScheduler;
    CorpusLoader corpusLoader;

    // Thread-safe corpus sharing
    std::shared_ptr<SharedCorpus> currentCorpus;  // Message thread (owns lifetime)
    std::shared_ptr<SharedCorpus> retiredCorpus;  // Keeps previous corpus alive while audio thread drains
    std::atomic<SharedCorpus*> corpusForAudio { nullptr };  // Audio thread (read-only)

    // Weak guard for safe async callback delivery
    std::shared_ptr<int> lifetimeGuard = std::make_shared<int>(0);

    // UI-selected grain for triggering
    std::atomic<int> pendingGrainSelect { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureForgeProcessor)
};
