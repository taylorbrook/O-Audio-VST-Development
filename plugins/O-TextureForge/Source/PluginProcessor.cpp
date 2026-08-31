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
}

TextureForgeProcessor::~TextureForgeProcessor()
{
    // Invalidate the lifetime guard so pending callAsync callbacks become no-ops
    lifetimeGuard.reset();
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
        0.0f,
        "dB"));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MIDI_MODE", 1 },
        "MIDI Mode",
        juce::StringArray { "Pitch-Mapped", "Trigger + Modulate", "Generative Drone" },
        2));

    return layout;
}

void TextureForgeProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    grainScheduler.prepare(sampleRate);
    grainScheduler.reset();

    if (currentCorpus != nullptr && std::abs(currentCorpus->sampleRate - sampleRate) > 0.1)
    {
        juce::File corpusFile(currentCorpus->filePath);
        if (corpusFile.existsAsFile())
        {
            loadCorpusFile(corpusFile);
        }
    }
}

void TextureForgeProcessor::releaseResources() {}

void TextureForgeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
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

    // Load corpus pointer (atomic read)
    const SharedCorpus* corpus = corpusForAudio.load(std::memory_order_acquire);

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
    if (currentCorpus != nullptr && currentCorpus->filePath.isNotEmpty())
    {
        auto* corpusXml = xml->createNewChildElement("CORPUS");
        corpusXml->setAttribute("filePath", currentCorpus->filePath);
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

    copyXmlToBinary(*xml, destData);
}

void TextureForgeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));

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

    corpusLoader.loadFile(file, currentSampleRate,
        // Completion callback (PCA done, deliver corpus)
        [this, guard](std::shared_ptr<SharedCorpus> newCorpus)
        {
            if (guard.expired())
                return;

            if (newCorpus != nullptr && !newCorpus->grains.empty())
            {
                // Keep old corpus alive while audio thread drains its current processBlock
                retiredCorpus = currentCorpus;
                currentCorpus = newCorpus;
                corpusForAudio.store(currentCorpus.get(), std::memory_order_release);

                juce::Logger::writeToLog("Corpus loaded: " + juce::String(newCorpus->grains.size()) + " grains");

                // Notify editor
#if JUCE_WEB_BROWSER
                if (auto* editor = dynamic_cast<TextureForgeEditor*>(getActiveEditor()))
                {
                    editor->onCorpusLoaded(static_cast<int>(newCorpus->grains.size()));
                }
#endif
            }
        },
        // UMAP progress callback
        [this, guard](float progress)
        {
            if (guard.expired())
                return;

#if JUCE_WEB_BROWSER
            if (auto* editor = dynamic_cast<TextureForgeEditor*>(getActiveEditor()))
            {
                if (progress < 0.0f)
                {
                    // UMAP was cancelled
                    editor->onUmapCancelled();
                }
                else if (progress >= 1.0f && currentCorpus != nullptr && currentCorpus->umapReady.load())
                {
                    // Build UMAP points JSON
                    juce::String json = "[";
                    for (size_t i = 0; i < currentCorpus->umapX.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        float pitchNorm = (i < currentCorpus->grains.size()) ? currentCorpus->grains[i].descriptors[13] : 0.0f;
                        float energyNorm = (i < currentCorpus->grains.size()) ? currentCorpus->grains[i].descriptors[17] : 0.0f;
                        json += "[" + juce::String(currentCorpus->umapX[i], 4)
                              + "," + juce::String(currentCorpus->umapY[i], 4)
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

void TextureForgeProcessor::selectGrainFromUI(int grainIndex)
{
    pendingGrainSelect.store(grainIndex, std::memory_order_relaxed);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TextureForgeProcessor();
}
