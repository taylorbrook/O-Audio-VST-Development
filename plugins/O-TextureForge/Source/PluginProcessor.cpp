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

    O-TextureForge - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include <algorithm>
#include <iterator>

// PluginEditor.h is included behind #if JUCE_WEB_BROWSER, and every use of
// TextureForgeEditor below carries the same guard. The param-dump console
// target (scripts/param-dump) compiles this TU with JUCE_WEB_BROWSER=0 and no
// editor sources, so the editor is not a complete type there. Under a normal
// build JUCE_WEB_BROWSER=1 and every guarded arm is taken — behaviour is
// byte-identical to v1.1.0.
#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

TextureForgeProcessor::TextureForgeProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Force Eigen's cache-size static to initialize on the main thread.
    // Avoids PAC trap when __cxa_guard_acquire runs on background threads
    // in arm64e AU hosting processes (Logic Pro).
    initEigenOnMainThread();
    energyParam       = parameters.getRawParameterValue("ENERGY");
    brightnessParam   = parameters.getRawParameterValue("BRIGHTNESS");
    textureParam      = parameters.getRawParameterValue("TEXTURE");
    positionParam     = parameters.getRawParameterValue("POSITION");
    grainDensityParam = parameters.getRawParameterValue("GRAIN_DENSITY");
    grainSizeParam    = parameters.getRawParameterValue("GRAIN_SIZE");
    scatterXParam     = parameters.getRawParameterValue("SCATTER_X");
    scatterYParam     = parameters.getRawParameterValue("SCATTER_Y");
    variationParam    = parameters.getRawParameterValue("VARIATION");
    crossfadeParam    = parameters.getRawParameterValue("CROSSFADE");
    outputGainParam   = parameters.getRawParameterValue("OUTPUT_GAIN");
    midiModeParam     = parameters.getRawParameterValue("MIDI_MODE");

    // v1.8.0
    weightParams[kWeightTimbre]     = parameters.getRawParameterValue("WEIGHT_TIMBRE");
    weightParams[kWeightBrightness] = parameters.getRawParameterValue("WEIGHT_BRIGHTNESS");
    weightParams[kWeightTexture]    = parameters.getRawParameterValue("WEIGHT_TEXTURE");
    weightParams[kWeightEnergy]     = parameters.getRawParameterValue("WEIGHT_ENERGY");
    pickSpreadParam   = parameters.getRawParameterValue("PICK_SPREAD");
    repeatAvoidParam  = parameters.getRawParameterValue("REPEAT_AVOID");

    startTimerHz(20);
}

TextureForgeProcessor::~TextureForgeProcessor()
{
    // Invalidate the lifetime guard so pending callAsync callbacks become no-ops
    lifetimeGuard.reset();

    // v1.8.0: no index build may outlive the processor.
    stopTimer();
    indexPool.removeAllJobs(true, 10000);
}

juce::AudioProcessorValueTreeState::ParameterLayout TextureForgeProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ENERGY", 1 },
        "Energy",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BRIGHTNESS", 1 },
        "Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TEXTURE", 1 },
        "Texture",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "POSITION", 1 },
        "Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "GRAIN_DENSITY", 1 },
        "Grain Density",
        1, 64, 8));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "GRAIN_SIZE", 1 },
        "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 0.1f, 0.5f),
        50.0f,
        "ms"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SCATTER_X", 1 },
        "Scatter X",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SCATTER_Y", 1 },
        "Scatter Y",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VARIATION", 1 },
        "Variation",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CROSSFADE", 1 },
        "Crossfade",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        -6.0f,   // v1.5.1: suite -6 dB gain staging (was 0.0f); ID/range unchanged
        "dB"));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MIDI_MODE", 1 },
        "MIDI Mode",
        juce::StringArray { "Pitch-Mapped", "Trigger + Modulate", "Generative Drone" },
        2));

    // v1.8.0: steerable descriptor weights (research §3.5). A multiplier on each
    // group's shipped weight: 100 % = v1.7.0, 0 % ignores the group, 400 % = four
    // times its pull. Skew 0.5 puts 100 % at the centre of the dial; the 1 %
    // interval snaps the default back to exactly 100.
    const auto addWeight = [&layout](const char* id, const char* name)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 2 },
            name,
            juce::NormalisableRange<float>(0.0f, 400.0f, 1.0f, 0.5f),
            100.0f,
            "%"));
    };
    addWeight("WEIGHT_TIMBRE",     "Timbre Weight");
    addWeight("WEIGHT_BRIGHTNESS", "Brightness Weight");
    addWeight("WEIGHT_TEXTURE",    "Texture Weight");
    addWeight("WEIGHT_ENERGY",     "Energy Weight");

    // v1.8.0: 0 = always the nearest candidate (v1.7.0); above 0 the pick is
    // softmax(-D^2/tau) over a cached 16 nearest.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "PICK_SPREAD", 2 },
        "Pick Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"));

    // v1.8.0: penalty on the last 8 picked grains; 0 = off (v1.7.0).
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "REPEAT_AVOID", 2 },
        "Repeat Avoid",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        "%"));

    return layout;
}

void TextureForgeProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate.store(sampleRate);
    // v1.5.1: the scheduler's output-gain smoother is seeded at the live level
    // so the first block ramps from neither 0 nor 1.
    grainScheduler.prepare(sampleRate, outputGainParam->load());
    grainScheduler.reset();

    // v1.6.2: the rate check and reload run on the message thread. prepareToPlay
    // is called on a host thread, where reading currentCorpus raced its
    // reassignment and loadCorpusFile drove the loader concurrently with the
    // message thread's own loads (and could block here for up to 5 s).
    std::weak_ptr<int> guard(lifetimeGuard);
    juce::MessageManager::callAsync([this, guard]()
    {
        if (!guard.expired())
            reloadCorpusIfRateChanged();
    });
}

void TextureForgeProcessor::reloadCorpusIfRateChanged()
{
    const auto corpus = getCurrentCorpus();
    const double sampleRate = currentSampleRate.load();
    if (corpus == nullptr || std::abs(corpus->sampleRate - sampleRate) <= 0.1)
        return;
    if (inFlightLoadRate > 0.0 && std::abs(inFlightLoadRate - sampleRate) <= 0.1
        && inFlightLoadPath == corpus->filePath)
        return;

    juce::File corpusFile(corpus->filePath);
    if (corpusFile.existsAsFile())
        loadCorpusFile(corpusFile);
}

void TextureForgeProcessor::releaseResources() {}

void TextureForgeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // v1.6.1: entry/exit counters for the retired-corpus reap. The exit is a
    // scope guard so every return path counts it.
    blocksEntered.fetch_add(1, std::memory_order_seq_cst);
    struct BlockExit
    {
        std::atomic<uint64_t>& counter;
        ~BlockExit() { counter.fetch_add(1, std::memory_order_seq_cst); }
    } blockExit { blocksRendered };

    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumSamples() == 0)
        return;

    // Read parameters (atomic, real-time safe)
    SchedulerParams params;
    params.energy = energyParam->load();
    params.brightness = brightnessParam->load();
    params.texture = textureParam->load();
    params.position = positionParam->load();
    params.grainDensity = static_cast<int>(grainDensityParam->load());
    params.grainSizeMs = grainSizeParam->load();
    params.scatterX = scatterXParam->load();
    params.scatterY = scatterYParam->load();
    params.variation = variationParam->load();
    params.crossfadePercent = crossfadeParam->load();
    params.outputGainDb = outputGainParam->load();
    params.midiMode = static_cast<int>(midiModeParam->load());
    params.rngSeed = rngSeed.load(std::memory_order_relaxed);
    params.pickSpread = pickSpreadParam->load() / 100.0f;
    params.repeatAvoid = repeatAvoidParam->load() / 100.0f;

    // Load corpus pointer (atomic read)
    const SharedCorpus* corpus = corpusForAudio.load(std::memory_order_seq_cst);

    // v1.8.0: the steered index, only while it was built for this corpus. Loaded
    // after the corpus and after blocksEntered, so the reap rule covers it too.
    const SearchIndex* index = indexForAudio.load(std::memory_order_seq_cst);
    params.searchIndex = (index != nullptr && index->corpus.get() == corpus) ? index : nullptr;

    // Check for UI grain selection
    int grainSel = pendingGrainSelect.exchange(-1, std::memory_order_relaxed);
    if (grainSel >= 0 && corpus != nullptr && grainSel < static_cast<int>(corpus->grains.size()))
    {
        grainScheduler.triggerSpecificGrain(corpus, params, static_cast<uint32_t>(grainSel));
    }

    // Process grains
    grainScheduler.processBlock(buffer, midiMessages, corpus, params);

    // NaN/Inf output protection
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::clip(buffer.getWritePointer(ch), buffer.getReadPointer(ch), -1.0f, 1.0f, numSamples);

    // Update visualization snapshot
    int writeIdx = vizWriteIndex.load(std::memory_order_relaxed);
    int nextIdx = 1 - writeIdx;

    auto& viz = vizSnapshots[static_cast<size_t>(nextIdx)];
    viz.cursorX = params.scatterX;
    viz.cursorY = params.scatterY;

    GrainScheduler::ActiveGrainInfo activeGrains[64];
    grainScheduler.getActiveGrains(activeGrains, viz.activeCount);

    for (int i = 0; i < viz.activeCount; ++i)
    {
        viz.activeGrains[i].grainIndex = activeGrains[i].grainIndex;
        viz.activeGrains[i].envelope = activeGrains[i].envelope;
        viz.activeGrains[i].readPositionNorm = activeGrains[i].readPositionNorm;
    }

    vizWriteIndex.store(nextIdx, std::memory_order_release);
}

juce::AudioProcessorEditor* TextureForgeProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new TextureForgeEditor(*this);
#else
    // The param-dump console target never opens an editor; this keeps the TU
    // linkable without the editor sources.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void TextureForgeProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Save corpus file path for session restore
    const auto corpus = getCurrentCorpus();
    if (corpus != nullptr && corpus->filePath.isNotEmpty())
    {
        auto* corpusXml = xml->createNewChildElement("CORPUS");
        corpusXml->setAttribute("filePath", corpus->filePath);
    }

    // v1.1.0: the UI language, saved as one more plain XML child beside CORPUS
    // above — this processor's own idiom for non-parameter state. Written as a
    // language CODE ("en"/"fr") rather than the atomic's index, so a
    // hand-inspected session file says what it means.
    //
    // A CHILD ELEMENT rather than a property on the APVTS state tree, and that
    // choice sidesteps a trap the tree form carries: NamedValueSet's XML
    // round-trip rebuilds every property as a var over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type), so an isBool()/isInt()
    // guard on the way back would be false for every saved session. An XML
    // attribute is a string by construction and getStringAttribute is the only
    // read it has.
    auto* langXml = xml->createNewChildElement("UILANG");
    langXml->setAttribute("code", languageCode(uiLanguage.load(std::memory_order_acquire)));

    // v1.6.0: the grain-selection seed, as 16 hex digits (an XML int attribute
    // is 32-bit).
    auto* rngXml = xml->createNewChildElement("RNG");
    rngXml->setAttribute("seed", juce::String::toHexString(static_cast<juce::int64>(rngSeed.load(std::memory_order_relaxed))));

    copyXmlToBinary(*xml, destData);
}

void TextureForgeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        // v1.8.0: a session saved before 1.8.0 has no entry for the new
        // parameters, and replaceState() would leave them where this instance
        // had them. They are added at their defaults, so an old session plays
        // the v1.7.0 search.
        auto tree = juce::ValueTree::fromXml(*xml);
        for (const char* id : { "WEIGHT_TIMBRE", "WEIGHT_BRIGHTNESS", "WEIGHT_TEXTURE", "WEIGHT_ENERGY",
                                "PICK_SPREAD", "REPEAT_AVOID" })
        {
            if (tree.getChildWithProperty("id", id).isValid())
                continue;
            if (auto* p = parameters.getParameter(id))
            {
                juce::ValueTree child("PARAM");
                child.setProperty("id", id, nullptr);
                child.setProperty("value", p->convertFrom0to1(p->getDefaultValue()), nullptr);
                tree.appendChild(child, nullptr);
            }
        }
        parameters.replaceState(tree);

        // v1.1.0: the UI language. A pre-1.1.0 session has no UILANG child at
        // all and the default (English) stands. languageIndex() clamps anything
        // that is not "fr" to 0, so a hand-edited value degrades to English
        // rather than to a bad index.
        //
        // The editor PULLS this through the getUiLanguage native function at
        // page init rather than being pushed from here — a push would race the
        // WebView's load.
        if (auto* langXml = xml->getChildByName("UILANG"))
            uiLanguage.store(languageIndex(langXml->getStringAttribute("code")),
                             std::memory_order_release);

        // v1.6.0: the seed. A pre-1.6.0 session has none and keeps this
        // instance's random seed.
        if (auto* rngXml = xml->getChildByName("RNG"))
        {
            const auto hex = rngXml->getStringAttribute("seed");
            if (hex.containsOnly("0123456789abcdefABCDEF") && hex.isNotEmpty() && hex.length() <= 16)
                rngSeed.store(static_cast<uint64_t>(hex.getHexValue64()), std::memory_order_relaxed);
        }

        // Restore corpus from saved path — defer to message thread to avoid
        // blocking the audio thread (setStateInformation can be called from any thread)
        auto* corpusXml = xml->getChildByName("CORPUS");
        if (corpusXml != nullptr)
        {
            juce::String savedPath = corpusXml->getStringAttribute("filePath");
            std::weak_ptr<int> guard(lifetimeGuard);

            juce::MessageManager::callAsync([this, guard, savedPath]()
            {
                if (guard.expired())
                    return;

                juce::File file(savedPath);
                if (file.existsAsFile())
                {
                    loadCorpusFile(file);
                }
                else if (savedPath.isNotEmpty())
                {
#if JUCE_WEB_BROWSER
                    if (auto* editor = dynamic_cast<TextureForgeEditor*>(getActiveEditor()))
                        editor->onCorpusMissing(savedPath);
#endif
                }
            });
        }
    }
}

void TextureForgeProcessor::loadCorpusFile(const juce::File& file)
{
    std::weak_ptr<int> guard(lifetimeGuard);

    const double sampleRate = currentSampleRate.load();
    inFlightLoadPath = file.getFullPathName();
    inFlightLoadRate = sampleRate;

    corpusLoader.loadFile(file, sampleRate,
        // Completion callback (PCA done, deliver corpus)
        [this, guard](std::shared_ptr<SharedCorpus> newCorpus)
        {
            if (guard.expired())
                return;

            inFlightLoadRate = 0.0;

            if (newCorpus != nullptr && !newCorpus->grains.empty())
            {
                publishCorpus(newCorpus);

                juce::Logger::writeToLog("Corpus loaded: " + juce::String(newCorpus->grains.size()) + " grains");

                // Notify editor
#if JUCE_WEB_BROWSER
                if (auto* editor = dynamic_cast<TextureForgeEditor*>(getActiveEditor()))
                {
                    editor->onCorpusLoaded(static_cast<int>(newCorpus->grains.size()));
                }
#endif

                // v1.6.2: the host changed rate while this load was analysing at the
                // old one — it plays now (wrong pitch) and is reloaded at the new rate.
                reloadCorpusIfRateChanged();
            }
        },
        // UMAP progress callback
        [this, guard](float progress)
        {
            if (guard.expired())
                return;

            reapRetiredCorpora();

#if JUCE_WEB_BROWSER
            if (auto* editor = dynamic_cast<TextureForgeEditor*>(getActiveEditor()))
            {
                const auto corpus = getCurrentCorpus();
                if (progress < 0.0f)
                {
                    // UMAP was cancelled
                    editor->onUmapCancelled();
                }
                else if (progress >= 1.0f && corpus != nullptr && corpus->umapReady.load())
                {
                    // Build UMAP points JSON
                    juce::String json = "[";
                    for (size_t i = 0; i < corpus->umapX.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        float pitchNorm = (i < corpus->grains.size()) ? corpus->grains[i].descriptors[13] : 0.0f;
                        float energyNorm = (i < corpus->grains.size()) ? corpus->grains[i].descriptors[17] : 0.0f;
                        json += "[" + juce::String(corpus->umapX[i], 4)
                              + "," + juce::String(corpus->umapY[i], 4)
                              + "," + juce::String(pitchNorm, 3)
                              + "," + juce::String(energyNorm, 3) + "]";
                    }
                    json += "]";
                    editor->onUmapComplete(json);
                }
                else
                {
                    editor->onUmapProgress(progress);
                }
            }
#else
            juce::ignoreUnused(progress);
#endif
        },
        // Failure callback
        [this, guard](const juce::String& reason)
        {
            if (guard.expired())
                return;

            inFlightLoadRate = 0.0;

#if JUCE_WEB_BROWSER
            if (auto* editor = dynamic_cast<TextureForgeEditor*>(getActiveEditor()))
            {
                editor->onCorpusLoadFailed(reason);
            }
#else
            juce::ignoreUnused(reason);
#endif
        }
    );
}

// Message thread only. The outgoing corpus is parked, not freed: an in-flight
// audio block may still be reading its raw pointer.
void TextureForgeProcessor::publishCorpus(std::shared_ptr<SharedCorpus> newCorpus)
{
    reapRetiredCorpora();

    const juce::ScopedLock sl(corpusLock);

    if (currentCorpus != nullptr)
        retiredCorpora.push_back({ currentCorpus, blocksRendered.load(std::memory_order_seq_cst), nullptr });

    // v1.8.0: before the audio thread can see it (keys the scheduler's cache).
    newCorpus->generation = ++corpusGenerationCounter;

    currentCorpus = std::move(newCorpus);
    // seq_cst pairs with processBlock's blocksEntered increment + pointer load for
    // the quiescence test below.
    corpusForAudio.store(currentCorpus.get(), std::memory_order_seq_cst);
}

// Frees parked corpora no block can still reference: >= 2 blocks completed since
// parking, or none in flight now. Exits are read FIRST: entries >= exits always,
// so equality means nothing was in flight when entries was read, and a block
// entering later loads the already-published pointer. The scheduler's voices
// hold only sample offsets between blocks, never the corpus pointer.
void TextureForgeProcessor::reapRetiredCorpora()
{
    std::vector<RetiredCorpus> freed;   // destroyed after the lock is released
    {
        const juce::ScopedLock sl(corpusLock);
        const uint64_t rendered = blocksRendered.load(std::memory_order_seq_cst);
        const uint64_t entered  = blocksEntered.load(std::memory_order_seq_cst);
        const bool quiescent = (entered == rendered);
        const auto keep = std::stable_partition(retiredCorpora.begin(), retiredCorpora.end(),
                                                [rendered, quiescent](const RetiredCorpus& r)
                                                { return !(quiescent || rendered >= r.parkedAtBlock + 2); });
        std::move(keep, retiredCorpora.end(), std::back_inserter(freed));
        retiredCorpora.erase(keep, retiredCorpora.end());
    }
}

WeightMults TextureForgeProcessor::readWeightMults() const
{
    WeightMults m {};
    for (size_t g = 0; g < m.size(); ++g)
        m[g] = weightParams[g]->load() / 100.0f;
    return m;
}

// Message thread. Also the periodic reap, which until v1.8.0 ran only on a
// publish or a UMAP progress callback.
void TextureForgeProcessor::timerCallback()
{
    reapRetiredCorpora();

    const auto corpus = getCurrentCorpus();
    const auto mults = readWeightMults();

    if (corpus == nullptr || SearchIndex::isDefault(mults))
    {
        if (currentIndex != nullptr)
            publishSearchIndex(nullptr);
        return;
    }

    if (currentIndex != nullptr && currentIndex->corpus == corpus && currentIndex->mults == mults)
        return;
    if (indexBuildInFlight)
        return;   // the next tick rebuilds if this one is stale when it lands

    indexBuildInFlight = true;
    const uint64_t generation = ++indexGenerationCounter;
    std::weak_ptr<int> guard(lifetimeGuard);

    indexPool.addJob([this, guard, corpus, mults, generation]()
    {
        auto built = SearchIndex::build(corpus, mults, generation);
        juce::MessageManager::callAsync([this, guard, built]() mutable
        {
            if (guard.expired())
                return;
            indexBuildInFlight = false;
            // Dropped if the corpus changed or the weights went back to default
            // meanwhile; a stale-weight build is published and replaced next tick.
            if (built->corpus == getCurrentCorpus() && !SearchIndex::isDefault(readWeightMults()))
                publishSearchIndex(std::move(built));
        });
    });
}

// Message thread. The outgoing index is parked like a corpus: an in-flight block
// may still be searching its tree.
void TextureForgeProcessor::publishSearchIndex(std::shared_ptr<SearchIndex> newIndex)
{
    reapRetiredCorpora();

    const juce::ScopedLock sl(corpusLock);

    if (currentIndex != nullptr)
        retiredCorpora.push_back({ nullptr, blocksRendered.load(std::memory_order_seq_cst), currentIndex });

    currentIndex = std::move(newIndex);
    indexForAudio.store(currentIndex.get(), std::memory_order_seq_cst);
}

void TextureForgeProcessor::selectGrainFromUI(int grainIndex)
{
    pendingGrainSelect.store(grainIndex, std::memory_order_relaxed);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TextureForgeProcessor();
}
