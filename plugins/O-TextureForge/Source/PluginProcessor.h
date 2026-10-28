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
#include "dsp/SearchIndex.h"
#include "dsp/CorpusLoader.h"
#include "dsp/GrainScheduler.h"
#include <memory>
#include <atomic>
#include <vector>

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

class TextureForgeProcessor : public juce::AudioProcessor,
                              private juce::Timer
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

    // UI interaction. v1.6.2: a copy taken under corpusLock — safe from any
    // thread except the audio thread.
    std::shared_ptr<SharedCorpus> getCurrentCorpus() const
    {
        const juce::ScopedLock sl(corpusLock);
        return currentCorpus;
    }
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

    /** The codec. languageIndex() maps anything that is neither "fr" nor
        "zh-Hans" to 0, so a hand-edited session or an unexpected argument from
        the page degrades to English rather than being stored unvalidated.

        NO CHINESE CHARACTER APPEARS ANYWHERE UNDER Source/ — every Chinese
        string lives in the UI table (Source/ui/public/js/i18n.js), and the one
        Han string in the markup is written as numeric character references.
        What is persisted here is a language CODE, an ASCII identifier. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }

    // v1.6.0: grain-selection RNG seed. Drawn at random per instance, persisted
    // as an RNG child element, so a reopened session picks the same grains.
    std::atomic<uint64_t> rngSeed { static_cast<uint64_t>(juce::Random::getSystemRandom().nextInt64()) };

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
    // v1.8.0
    std::array<std::atomic<float>*, kNumWeightGroups> weightParams {};   // 0..400 %, 100 = shipped weight
    std::atomic<float>* pickSpreadParam = nullptr;
    std::atomic<float>* repeatAvoidParam = nullptr;

    // Visualization double-buffer (lock-free audio->GUI)
    std::array<VizSnapshot, 2> vizSnapshots {};
    std::atomic<int> vizWriteIndex { 0 };

    // v1.6.2: atomic — written by prepareToPlay (host thread), read on the
    // message thread when a load starts or completes.
    std::atomic<double> currentSampleRate { 44100.0 };

    // DSP components
    GrainScheduler grainScheduler;
    CorpusLoader corpusLoader;

    // Thread-safe corpus sharing. currentCorpus and retiredCorpora are written
    // on the message thread only (the loader's callAsync completion) and guarded
    // by corpusLock, which other threads take to read them (v1.6.2: prepareToPlay
    // and getStateInformation run on host threads). The audio thread never locks;
    // it reads one raw pointer per block.
    //
    // v1.6.1: a publish parks the outgoing corpus instead of overwriting a single
    // retired slot. With one slot, two completions back to back (a queued callAsync
    // the next load's cancel can't purge, or a prepareToPlay reload) dropped the
    // last reference to a corpus an in-flight block was still reading. An entry is
    // freed once >= 2 blocks have completed since parking, or when no block is in
    // flight (blocksEntered == blocksRendered) — the +2 rule alone never fires
    // while the host has stopped calling processBlock.
    //
    // v1.8.0: the steered SearchIndex is parked in the same list and reaped by the
    // same rule (an entry holds a corpus, an index, or both).
    struct RetiredCorpus
    {
        std::shared_ptr<SharedCorpus> corpus;
        uint64_t parkedAtBlock = 0;
        std::shared_ptr<SearchIndex> index;
    };

    void publishCorpus(std::shared_ptr<SharedCorpus> newCorpus);
    void reapRetiredCorpora();

    // v1.8.0: steered descriptor weights. A 20 Hz message-thread timer compares the
    // four weight parameters (and the current corpus) with the published index; on
    // a mismatch it queues one build on indexPool (a worker thread: building the
    // tree allocates), and the completion publishes it on the message thread. All
    // four at 100 % publishes nothing, so the default search is v1.7.0's.
    void timerCallback() override;
    void publishSearchIndex(std::shared_ptr<SearchIndex> newIndex);
    WeightMults readWeightMults() const;

    std::shared_ptr<SearchIndex> currentIndex;               // message thread (owns lifetime)
    std::atomic<SearchIndex*> indexForAudio { nullptr };     // audio thread (read-only)
    bool indexBuildInFlight = false;                          // message thread
    uint64_t indexGenerationCounter = 0;                      // message thread
    uint64_t corpusGenerationCounter = 0;                     // message thread
    juce::ThreadPool indexPool { juce::ThreadPoolOptions{}.withThreadName("TF search index").withNumberOfThreads(1) };

    // v1.6.2: message thread. Reloads the current corpus when it was analysed at
    // a different rate than the host now runs at, unless that load is in flight.
    void reloadCorpusIfRateChanged();

    juce::CriticalSection corpusLock;

    std::shared_ptr<SharedCorpus> currentCorpus;  // Message thread (owns lifetime)
    std::vector<RetiredCorpus> retiredCorpora;    // Message thread; outgoing corpora until unreferenced
    std::atomic<SharedCorpus*> corpusForAudio { nullptr };  // Audio thread (read-only)
    std::atomic<uint64_t> blocksEntered { 0 };    // first statement of processBlock
    std::atomic<uint64_t> blocksRendered { 0 };   // every exit from processBlock

    // v1.6.2: the load the loader is working on (message thread; rate 0 = none),
    // so repeated prepareToPlay calls queue one reload, not one each.
    juce::String inFlightLoadPath;
    double inFlightLoadRate = 0.0;

    // Weak guard for safe async callback delivery
    std::shared_ptr<int> lifetimeGuard = std::make_shared<int>(0);

    // UI-selected grain for triggering
    std::atomic<int> pendingGrainSelect { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureForgeProcessor)
};
