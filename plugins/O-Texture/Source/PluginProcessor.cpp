/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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

    O-Texture - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.
#include "ModelData.h"

// =============================================================================
// Dim Map JSON parsing helpers
// =============================================================================

static int getIntFromVar(const juce::var& v, int defaultVal)
{
    if (v.isInt() || v.isDouble())
        return static_cast<int>(v);
    return defaultVal;
}

static std::vector<int> getIntArrayFromVar(const juce::var& v)
{
    std::vector<int> result;
    if (auto* arr = v.getArray())
    {
        for (const auto& item : *arr)
            result.push_back(static_cast<int>(item));
    }
    return result;
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

TextureProcessor::TextureProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread("O-Texture Inference"),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    sourceParam       = parameters.getRawParameterValue("SOURCE");
    modeParam         = parameters.getRawParameterValue("MODE");
    xParam            = parameters.getRawParameterValue("X");
    yParam            = parameters.getRawParameterValue("Y");
    characterAParam   = parameters.getRawParameterValue("CHARACTER_A");
    characterBParam   = parameters.getRawParameterValue("CHARACTER_B");
    evolveParam       = parameters.getRawParameterValue("EVOLVE");
    freezeParam       = parameters.getRawParameterValue("FREEZE");
    brightnessParam   = parameters.getRawParameterValue("BRIGHTNESS");
    mixParam          = parameters.getRawParameterValue("MIX");

    decodedBufferL.resize(static_cast<size_t>(kBlockSize), 0.0f);
    decodedBufferR.resize(static_cast<size_t>(kBlockSize), 0.0f);
    asyncDecodedL.resize(static_cast<size_t>(kBlockSize), 0.0f);
    asyncDecodedR.resize(static_cast<size_t>(kBlockSize), 0.0f);

    loadDimMap();
    initDecoderSession();

    const auto seed = static_cast<uint32_t>(juce::Time::currentTimeMillis() & 0xFFFFFFFF);
    evolveNoise.setSeed(seed);
    regenerateInactiveValues(seed);
    snapshotSeed = seed;
    snapshotCursors = evolveNoise.getCursors();

    if (decoderSession)
        startThread(Priority::high);
}

TextureProcessor::~TextureProcessor()
{
    stopThread(2000);
    decoderSession.reset();
}

// =============================================================================
// Dim Map Loading
// =============================================================================

void TextureProcessor::loadDimMap()
{
    int dataSize = 0;
    const char* data = ModelData::getNamedResource("dim_map_rain_json", dataSize);

    if (data == nullptr || dataSize == 0)
    {
        juce::Logger::writeToLog("O-Texture: dim_map not found, using defaults");
        dimMap.evolveDims = { 4, 5, 6, 7, 8, 9, 10, 11 };
        dimMap.inactiveDims = { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
        return;
    }

    auto json = juce::JSON::parse(juce::String::fromUTF8(data, dataSize));

    if (auto* obj = json.getDynamicObject())
    {
        dimMap.latentDim = getIntFromVar(obj->getProperty("latent_dim"), 32);

        if (auto* mapping = obj->getProperty("mapping").getDynamicObject())
        {
            if (auto* xMap = mapping->getProperty("X").getDynamicObject())
                dimMap.xDim = getIntFromVar(xMap->getProperty("dim"), 0);
            if (auto* yMap = mapping->getProperty("Y").getDynamicObject())
                dimMap.yDim = getIntFromVar(yMap->getProperty("dim"), 1);
            if (auto* caMap = mapping->getProperty("CHARACTER_A").getDynamicObject())
                dimMap.charADim = getIntFromVar(caMap->getProperty("dim"), 2);
            if (auto* cbMap = mapping->getProperty("CHARACTER_B").getDynamicObject())
                dimMap.charBDim = getIntFromVar(cbMap->getProperty("dim"), 3);
        }

        dimMap.evolveDims = getIntArrayFromVar(obj->getProperty("evolve_dims"));
        dimMap.inactiveDims = getIntArrayFromVar(obj->getProperty("inactive_dims"));
    }

    juce::Logger::writeToLog("O-Texture: dim_map loaded - X=" + juce::String(dimMap.xDim)
                             + " Y=" + juce::String(dimMap.yDim)
                             + " evolve=" + juce::String(static_cast<int>(dimMap.evolveDims.size()))
                             + " inactive=" + juce::String(static_cast<int>(dimMap.inactiveDims.size())));
}

// =============================================================================
// ONNX Runtime Decoder Session
// =============================================================================

void TextureProcessor::initDecoderSession()
{
    int decoderSize = 0;
    const char* decoderData = ModelData::getNamedResource("decoder_onnx", decoderSize);

    if (decoderData == nullptr || decoderSize == 0)
    {
        juce::Logger::writeToLog("O-Texture: decoder.onnx not found!");
        return;
    }

    try
    {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetInterOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        decoderSession = std::make_unique<Ort::Session>(
            ortEnv,
            decoderData,
            static_cast<size_t>(decoderSize),
            sessionOptions
        );

        juce::Logger::writeToLog("O-Texture: ONNX decoder session created");
    }
    catch (const Ort::Exception& e)
    {
        juce::Logger::writeToLog("O-Texture: ONNX error: " + juce::String(e.what()));
    }
}

bool TextureProcessor::runDecoder(const float* latent, float* outputAudio)
{
    if (!decoderSession)
        return false;

    try
    {
        // Input: latent vector [1, 32]
        std::array<int64_t, 2> inputShape = { 1, kLatentDim };
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            const_cast<float*>(latent),
            static_cast<size_t>(kLatentDim),
            inputShape.data(),
            inputShape.size()
        );

        // Output: audio [1, 1, 4096] bound directly to the caller's buffer —
        // runDecoder holds no shared mutable state, so the inference thread and
        // the offline-render path may both call it concurrently.
        std::array<int64_t, 3> outputShape = { 1, 1, kBlockSize };
        Ort::Value outputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            outputAudio,
            static_cast<size_t>(kBlockSize),
            outputShape.data(),
            outputShape.size()
        );

        // Run inference
        const char* inputNames[] = { "z" };
        const char* outputNames[] = { "audio" };

        decoderSession->Run(
            Ort::RunOptions{ nullptr },
            inputNames, &inputTensor, 1,
            outputNames, &outputTensor, 1
        );

        return true;
    }
    catch (const Ort::Exception&)
    {
        decodeError.store(true, std::memory_order_relaxed);
        return false;
    }
}

void TextureProcessor::run()
{
    while (!threadShouldExit())
    {
        const uint32_t target = reqSeq.load(std::memory_order_acquire);

        if (doneSeq.load(std::memory_order_relaxed) != target)
        {
            const bool okL = runDecoder(reqLatentL.data(), asyncDecodedL.data());
            const bool okR = runDecoder(reqLatentR.data(), asyncDecodedR.data());
            asyncDecodeOk = okL && okR;
            doneSeq.store(target, std::memory_order_release);
        }
        else
        {
            wait(1);
        }
    }
}

// =============================================================================
// Latent Vector Construction
// =============================================================================

void TextureProcessor::constructLatentVectors(float x, float y, float charA, float charB,
                                               float evolveRate, bool freeze)
{
    evolveNoise.setSpeed(evolveRate);
    evolveNoise.advance(freeze);

    auto paramToLatent = [](float val) -> float {
        return val * 6.0f - 3.0f;
    };

    latentL.fill(0.0f);
    latentR.fill(0.0f);

    float xLatent = paramToLatent(x);
    float yLatent = paramToLatent(y);
    float charALatent = paramToLatent(charA);
    float charBLatent = paramToLatent(charB);

    latentL[static_cast<size_t>(dimMap.xDim)] = xLatent;
    latentL[static_cast<size_t>(dimMap.yDim)] = yLatent;
    latentL[static_cast<size_t>(dimMap.charADim)] = charALatent;
    latentL[static_cast<size_t>(dimMap.charBDim)] = charBLatent;

    const auto& noiseValues = evolveNoise.getAllValues();
    for (size_t i = 0; i < dimMap.evolveDims.size() && i < 28; ++i)
    {
        int dim = dimMap.evolveDims[i];
        if (dim >= 0 && dim < kLatentDim)
        {
            float noiseVal = noiseValues[i] * evolveRate * 2.0f;
            latentL[static_cast<size_t>(dim)] = juce::jlimit(-3.0f, 3.0f, noiseVal);
        }
    }

    for (int dim : dimMap.inactiveDims)
    {
        if (dim >= 0 && dim < kLatentDim)
            latentL[static_cast<size_t>(dim)] = inactiveValues[static_cast<size_t>(dim)];
    }

    for (int i = 0; i < kLatentDim; ++i)
        latentR[static_cast<size_t>(i)] = latentL[static_cast<size_t>(i)];

    latentL[static_cast<size_t>(dimMap.xDim)] += kStereoOffset;
    latentR[static_cast<size_t>(dimMap.xDim)] -= kStereoOffset;
    latentL[static_cast<size_t>(dimMap.yDim)] += kStereoOffset * 0.5f;
    latentR[static_cast<size_t>(dimMap.yDim)] -= kStereoOffset * 0.5f;

    // Publish noise state for getStateInformation; if the host is mid-read,
    // skip — the next hop retries.
    const juce::SpinLock::ScopedTryLockType snapshotLock(noiseSnapshotLock);
    if (snapshotLock.isLocked())
    {
        snapshotSeed = evolveNoise.getSeed();
        snapshotCursors = evolveNoise.getCursors();
    }
}

void TextureProcessor::regenerateInactiveValues(uint32_t seed)
{
    // Deterministic per seed: the same project restores the same texture, and
    // FREEZE genuinely freezes (values never change between hops).
    juce::Random rng(static_cast<juce::int64>(seed) ^ 0x6b43a9b5);
    for (auto& v : inactiveValues)
        v = rng.nextFloat() * 0.6f - 0.3f;
}

// =============================================================================
// Parameter Layout
// =============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout TextureProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "SOURCE", 1 }, "Source",
        juce::StringArray { "Rain", "Metal", "Wind", "Crowd", "Synth", "Organic" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 }, "Mode",
        juce::StringArray { "Generate", "Transform" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "X", 1 }, "X",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "Y", 1 }, "Y",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CHARACTER_A", 1 }, "Character A",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CHARACTER_B", 1 }, "Character B",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "EVOLVE", 1 }, "Evolve",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "FREEZE", 1 }, "Freeze", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BRIGHTNESS", 1 }, "Brightness",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 }, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    return layout;
}

// =============================================================================
// Prepare / Release
// =============================================================================

bool TextureProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && (layouts.getChannelSet(true, 0).isDisabled()
            || layouts.getChannelSet(true, 0) == juce::AudioChannelSet::stereo());
}

void TextureProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    olaProcessor.prepare(2);
    needsNewBlock = true;

    tiltFilter.prepare(sampleRate, juce::jmax(2, getTotalNumOutputChannels()));

    // Generate mode is a source: audio appears at sample 0 of the first hop,
    // so there is no input-to-output delay to report.
    setLatencySamples(0);

    // Note: evolveNoise is deliberately NOT reset here — its state is
    // hop-domain (sample-rate independent) and a reset would wipe the
    // evolution position restored by setStateInformation.

    std::fill(decodedBufferL.begin(), decodedBufferL.end(), 0.0f);
    std::fill(decodedBufferR.begin(), decodedBufferR.end(), 0.0f);

    prepared = true;
}

void TextureProcessor::releaseResources()
{
    olaProcessor.reset();
    tiltFilter.reset();
    prepared = false;
}

// =============================================================================
// Process Block
// =============================================================================

void TextureProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = 2; ch < totalNumOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    if (!decoderSession || !prepared)
    {
        buffer.clear();
        return;
    }

    // Apply noise state staged by setStateInformation. Try-lock: if the host
    // is mid-write we skip and pick it up next block.
    if (noiseStagingPending.load(std::memory_order_acquire))
    {
        const juce::SpinLock::ScopedTryLockType stagingLock(noiseStagingLock);
        if (stagingLock.isLocked())
        {
            if (stagedHasSeed)
            {
                evolveNoise.setSeed(stagedSeed);
                regenerateInactiveValues(stagedSeed);
                stagedHasSeed = false;
            }
            if (stagedHasCursors)
            {
                evolveNoise.setCursors(stagedCursors);
                stagedHasCursors = false;
            }
            noiseStagingPending.store(false, std::memory_order_release);
        }
    }

    const float x          = xParam->load();
    const float y          = yParam->load();
    const float charA      = characterAParam->load();
    const float charB      = characterBParam->load();
    const float evolveRate = evolveParam->load();
    const bool  freeze     = freezeParam->load() > 0.5f;
    const float brightness = brightnessParam->load();
    const float mix        = mixParam->load();

    tiltFilter.setBrightness(brightness);

    const bool offline = isNonRealtime();
    int samplesProcessed = 0;

    while (samplesProcessed < numSamples)
    {
        if (needsNewBlock)
        {
            if (offline)
            {
                // Offline render has no deadline: decode synchronously so
                // bounces are deterministic and never depend on inference-
                // thread timing. Discard any async result left in flight
                // from realtime playback.
                const uint32_t rq = reqSeq.load(std::memory_order_relaxed);
                if (doneSeq.load(std::memory_order_acquire) == rq)
                    consumedSeq = rq;

                constructLatentVectors(x, y, charA, charB, evolveRate, freeze);

                if (runDecoder(latentL.data(), decodedBufferL.data())
                    && runDecoder(latentR.data(), decodedBufferR.data()))
                {
                    olaProcessor.addDecodedBlock(0, decodedBufferL.data());
                    olaProcessor.addDecodedBlock(1, decodedBufferR.data());
                }
            }
            else
            {
                // Realtime: consume the block the inference thread finished,
                // then immediately request the next hop's decode. The decoder
                // gets a full hop (~43 ms @ 48 kHz) per block.
                const uint32_t rq = reqSeq.load(std::memory_order_relaxed);

                if (doneSeq.load(std::memory_order_acquire) == rq && consumedSeq != rq)
                {
                    if (asyncDecodeOk)
                    {
                        olaProcessor.addDecodedBlock(0, asyncDecodedL.data());
                        olaProcessor.addDecodedBlock(1, asyncDecodedR.data());
                    }
                    consumedSeq = rq;
                }

                if (consumedSeq == rq)
                {
                    // Idle: publish the next request (also primes at startup)
                    constructLatentVectors(x, y, charA, charB, evolveRate, freeze);
                    reqLatentL = latentL;
                    reqLatentR = latentR;
                    reqSeq.store(rq + 1, std::memory_order_release);
                }
                // else: decode still in flight — this hop plays the OLA tail
                // (graceful Hann fade) and we retry at the next hop boundary.
            }

            needsNewBlock = false;
        }

        const int samplesToRead = std::min(numSamples - samplesProcessed,
                                           olaProcessor.getSamplesUntilNextHop());

        // getSamplesUntilNextHop() ∈ [1, HOP_SIZE], so this can't fire; the
        // break guards against an infinite loop if that invariant ever breaks.
        jassert(samplesToRead > 0);
        if (samplesToRead <= 0)
            break;

        int hops = olaProcessor.readSamples(buffer, samplesProcessed, samplesToRead);

        if (hops > 0)
            needsNewBlock = true;

        samplesProcessed += samplesToRead;
    }

    tiltFilter.processBlock(buffer);

    if (mix < 1.0f)
        buffer.applyGain(mix);
}

// =============================================================================
// Editor
// =============================================================================

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* TextureProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new TextureEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

// =============================================================================
// State Serialization
// =============================================================================

void TextureProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Never read evolveNoise directly here — the audio thread owns it. Read
    // the per-hop snapshot instead; if a restore is still staged (audio not
    // yet running), pass it through verbatim so save-after-load round-trips.
    uint32_t seedOut;
    std::array<float, 28> cursorsOut{};
    {
        const juce::SpinLock::ScopedLockType lock(noiseSnapshotLock);
        seedOut = snapshotSeed;
        cursorsOut = snapshotCursors;
    }

    if (noiseStagingPending.load(std::memory_order_acquire))
    {
        const juce::SpinLock::ScopedLockType lock(noiseStagingLock);
        if (stagedHasSeed)    seedOut = stagedSeed;
        if (stagedHasCursors) cursorsOut = stagedCursors;
    }

    state.setProperty("evolve_seed", static_cast<int>(seedOut), nullptr);

    juce::String cursorStr;
    for (int i = 0; i < PerlinNoise1D<28>::getNumChannels(); ++i)
    {
        if (i > 0) cursorStr += ",";
        cursorStr += juce::String(cursorsOut[static_cast<size_t>(i)], 6);
    }
    state.setProperty("evolve_cursors", cursorStr, nullptr);

    // v0.2.0: the UI language rides the same tree as one more plain property,
    // beside the two above. Not a parameter (see PluginProcessor.h), so it is
    // saved and restored here rather than by the APVTS parameter round-trip.
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    state.setProperty("uiLanguage",
                      languageCode(uiLanguage.load(std::memory_order_acquire)), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TextureProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));

        auto state = parameters.state;

        // Stage the restored noise state; the audio thread applies it at the
        // top of the next processBlock. Mutating evolveNoise from this (host)
        // thread would race the audio thread's advance()/getAllValues().
        if (state.hasProperty("evolve_seed") || state.hasProperty("evolve_cursors"))
        {
            const juce::SpinLock::ScopedLockType lock(noiseStagingLock);

            if (state.hasProperty("evolve_seed"))
            {
                stagedSeed = static_cast<uint32_t>(static_cast<int>(state.getProperty("evolve_seed")));
                stagedHasSeed = true;
            }

            if (state.hasProperty("evolve_cursors"))
            {
                juce::String cursorStr = state.getProperty("evolve_cursors").toString();
                juce::StringArray tokens;
                tokens.addTokens(cursorStr, ",", "");

                if (tokens.size() == PerlinNoise1D<28>::getNumChannels())
                {
                    for (int i = 0; i < tokens.size(); ++i)
                        stagedCursors[static_cast<size_t>(i)] = tokens[i].getFloatValue();
                    stagedHasCursors = true;
                }
            }

            noiseStagingPending.store(true, std::memory_order_release);
        }

        // v0.2.0: the UI language. hasProperty() + toString() is the ONLY
        // correct read, and it is this file's own idiom for the two noise
        // properties above. getStateInformation writes a STRING var, but even a
        // bool or an int written there would not survive: the XML round-trip
        // does not preserve the type, because NamedValueSet::setFromXmlAttributes
        // rebuilds every property as `var (value)` over the attribute STRING
        // (critical_valuetree_xml_roundtrip_loses_type), so an isBool() or
        // isInt() guard would be false for every saved session. A pre-0.2.0
        // session has no such property at all and the default (English) stands.
        // languageIndex() clamps anything that is not "fr" to 0, so a
        // hand-edited value degrades to English rather than to a bad index.
        //
        // The editor PULLS this through the getUiLanguage native fn at page
        // init rather than being pushed from here — a push would race the
        // WebView's load.
        if (state.hasProperty("uiLanguage"))
            uiLanguage.store(languageIndex(state.getProperty("uiLanguage").toString()),
                             std::memory_order_release);
    }
}

// =============================================================================
// Plugin Instance Creator
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TextureProcessor();
}
