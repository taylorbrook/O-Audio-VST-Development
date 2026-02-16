/*
  ==============================================================================

    O-Texture - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
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
    decoderOutputBuffer.resize(static_cast<size_t>(kBlockSize), 0.0f);

    loadDimMap();
    initDecoderSession();

    evolveNoise.setSeed(static_cast<uint32_t>(juce::Time::currentTimeMillis() & 0xFFFFFFFF));
}

TextureProcessor::~TextureProcessor()
{
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
        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        // Input: latent vector [1, 32]
        std::array<int64_t, 2> inputShape = { 1, kLatentDim };
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            const_cast<float*>(latent),
            static_cast<size_t>(kLatentDim),
            inputShape.data(),
            inputShape.size()
        );

        // Output: audio [1, 1, 4096] — uses pre-allocated member buffer
        std::array<int64_t, 3> outputShape = { 1, 1, kBlockSize };
        Ort::Value outputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            decoderOutputBuffer.data(),
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

        // Copy output (the decoder output is [1, 1, 4096], we want the inner 4096 samples)
        std::memcpy(outputAudio, decoderOutputBuffer.data(), sizeof(float) * static_cast<size_t>(kBlockSize));
        return true;
    }
    catch (const Ort::Exception& e)
    {
        juce::Logger::writeToLog("O-Texture: Decoder inference error: " + juce::String(e.what()));
        return false;
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
            latentL[static_cast<size_t>(dim)] = inactiveRng.nextFloat() * 0.6f - 0.3f;
    }

    for (int i = 0; i < kLatentDim; ++i)
        latentR[static_cast<size_t>(i)] = latentL[static_cast<size_t>(i)];

    latentL[static_cast<size_t>(dimMap.xDim)] += kStereoOffset;
    latentR[static_cast<size_t>(dimMap.xDim)] -= kStereoOffset;
    latentL[static_cast<size_t>(dimMap.yDim)] += kStereoOffset * 0.5f;
    latentR[static_cast<size_t>(dimMap.yDim)] -= kStereoOffset * 0.5f;
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

void TextureProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    olaProcessor.prepare(2);
    needsNewBlock = true;

    tiltFilter.prepare(sampleRate, 2);

    setLatencySamples(OverlapAddProcessor::ACCUM_SIZE);

    evolveNoise.reset();
    inactiveRng.setSeed(static_cast<juce::int64>(juce::Time::currentTimeMillis()));

    std::fill(decodedBufferL.begin(), decodedBufferL.end(), 0.0f);
    std::fill(decodedBufferR.begin(), decodedBufferR.end(), 0.0f);
    decoderReady = false;

    prepared = true;

    juce::Logger::writeToLog("O-Texture prepareToPlay: sampleRate=" + juce::String(sampleRate)
                             + " latency=" + juce::String(OverlapAddProcessor::ACCUM_SIZE));
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

    const float x          = xParam->load();
    const float y          = yParam->load();
    const float charA      = characterAParam->load();
    const float charB      = characterBParam->load();
    const float evolveRate = evolveParam->load();
    const bool  freeze     = freezeParam->load() > 0.5f;
    const float brightness = brightnessParam->load();
    const float mix        = mixParam->load();

    tiltFilter.setBrightness(brightness);

    int samplesProcessed = 0;

    while (samplesProcessed < numSamples)
    {
        if (needsNewBlock)
        {
            constructLatentVectors(x, y, charA, charB, evolveRate, freeze);

            // Run decoder inference synchronously for both channels
            bool okL = runDecoder(latentL.data(), decodedBufferL.data());
            bool okR = runDecoder(latentR.data(), decodedBufferR.data());

            if (okL && okR)
            {
                olaProcessor.addDecodedBlock(0, decodedBufferL.data());
                olaProcessor.addDecodedBlock(1, decodedBufferR.data());
                decoderReady = true;
            }

            needsNewBlock = false;
        }

        int samplesToRead = std::min(numSamples - samplesProcessed,
                                     olaProcessor.getSamplesUntilNextHop());

        if (samplesToRead <= 0)
            samplesToRead = numSamples - samplesProcessed;

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

juce::AudioProcessorEditor* TextureProcessor::createEditor()
{
    return new TextureEditor(*this);
}

// =============================================================================
// State Serialization
// =============================================================================

void TextureProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    auto noiseSeed = static_cast<int>(evolveNoise.getSeed());
    state.setProperty("evolve_seed", noiseSeed, nullptr);

    const auto& cursors = evolveNoise.getCursors();
    juce::String cursorStr;
    for (int i = 0; i < PerlinNoise1D<28>::getNumChannels(); ++i)
    {
        if (i > 0) cursorStr += ",";
        cursorStr += juce::String(cursors[static_cast<size_t>(i)], 6);
    }
    state.setProperty("evolve_cursors", cursorStr, nullptr);

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

        if (state.hasProperty("evolve_seed"))
        {
            auto seed = static_cast<uint32_t>(static_cast<int>(state.getProperty("evolve_seed")));
            evolveNoise.setSeed(seed);
        }

        if (state.hasProperty("evolve_cursors"))
        {
            juce::String cursorStr = state.getProperty("evolve_cursors").toString();
            juce::StringArray tokens;
            tokens.addTokens(cursorStr, ",", "");

            if (tokens.size() == PerlinNoise1D<28>::getNumChannels())
            {
                std::array<float, 28> restoredCursors{};
                for (int i = 0; i < tokens.size(); ++i)
                    restoredCursors[static_cast<size_t>(i)] = tokens[i].getFloatValue();
                evolveNoise.setCursors(restoredCursors);
            }
        }
    }
}

// =============================================================================
// Plugin Instance Creator
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TextureProcessor();
}
