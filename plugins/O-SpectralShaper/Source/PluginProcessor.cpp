/*
  ==============================================================================

    O-SpectralShaper - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================================
// Parameter Layout Creation (JUCE 8 format)
// ============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
OSpectralShaperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MIX (0-100%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // ATTACK_TIME (0.1-50ms, logarithmic skew)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ATTACK_TIME", 1 },
        "Attack Time",
        juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.3f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // SUSTAIN_TIME (10-500ms, logarithmic skew)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SUSTAIN_TIME", 1 },
        "Sustain Time",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.3f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // SENSITIVITY (0-100%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SENSITIVITY", 1 },
        "Sensitivity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // LOOKAHEAD_ENABLED (toggle, default OFF)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "LOOKAHEAD_ENABLED", 1 },
        "Lookahead Enabled",
        false
    ));

    // LOOKAHEAD_TIME (0.1-10ms, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LOOKAHEAD_TIME", 1 },
        "Lookahead Time",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // OUTPUT_GAIN (-12 to +12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    return layout;
}

// ============================================================================
// Constructor/Destructor
// ============================================================================

OSpectralShaperAudioProcessor::OSpectralShaperAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-SpectralShaper")
{
    // Initialize curves to neutral (no shaping)
    std::fill(attackCurve.begin(), attackCurve.end(), 0.0f);
    std::fill(sustainCurve.begin(), sustainCurve.end(), 0.0f);

    // Custom state callbacks for curve data (not in APVTS)
    presetManager.setCustomStateCallbacks(
        // Save callback
        [this]() -> juce::var {
            auto* obj = new juce::DynamicObject();
            // Encode curves as arrays
            juce::Array<juce::var> attackArr, sustainArr;
            for (int i = 0; i < 32; ++i)
            {
                attackArr.add(static_cast<double>(attackCurve[i]));
                sustainArr.add(static_cast<double>(sustainCurve[i]));
            }
            obj->setProperty("attackCurve", juce::var(attackArr));
            obj->setProperty("sustainCurve", juce::var(sustainArr));
            return juce::var(obj);
        },
        // Load callback
        [this](const juce::var& data) {
            if (auto* obj = data.getDynamicObject())
            {
                if (obj->hasProperty("attackCurve"))
                {
                    auto* arr = obj->getProperty("attackCurve").getArray();
                    if (arr != nullptr && arr->size() == 32)
                    {
                        std::array<float, 32> curve;
                        for (int i = 0; i < 32; ++i)
                            curve[i] = static_cast<float>((*arr)[i]);
                        setAttackCurve(curve);
                    }
                }
                if (obj->hasProperty("sustainCurve"))
                {
                    auto* arr = obj->getProperty("sustainCurve").getArray();
                    if (arr != nullptr && arr->size() == 32)
                    {
                        std::array<float, 32> curve;
                        for (int i = 0; i < 32; ++i)
                            curve[i] = static_cast<float>((*arr)[i]);
                        setSustainCurve(curve);
                    }
                }
            }
        }
    );

    // Initialize factory presets
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        {
            "Default",
            {{"MIX", 1.0f}, {"ATTACK_TIME", 0.198f}, {"SUSTAIN_TIME", 0.184f},
             {"SENSITIVITY", 0.5f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 0.192f}, {"OUTPUT_GAIN", 0.5f}},
            juce::var()
        },
        {
            "Transient Tamer",
            {{"MIX", 0.75f}, {"ATTACK_TIME", 0.05f}, {"SUSTAIN_TIME", 0.10f},
             {"SENSITIVITY", 0.7f}, {"LOOKAHEAD_ENABLED", 1.0f},
             {"LOOKAHEAD_TIME", 0.30f}, {"OUTPUT_GAIN", 0.5f}},
            juce::var()
        },
        {
            "Punch Enhancer",
            {{"MIX", 0.85f}, {"ATTACK_TIME", 0.30f}, {"SUSTAIN_TIME", 0.05f},
             {"SENSITIVITY", 0.6f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 0.192f}, {"OUTPUT_GAIN", 0.54f}},
            juce::var()
        },
        {
            "Gentle Shaping",
            {{"MIX", 0.50f}, {"ATTACK_TIME", 0.25f}, {"SUSTAIN_TIME", 0.30f},
             {"SENSITIVITY", 0.35f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 0.192f}, {"OUTPUT_GAIN", 0.5f}},
            juce::var()
        },
        {
            "Aggressive Bite",
            {{"MIX", 1.0f}, {"ATTACK_TIME", 0.10f}, {"SUSTAIN_TIME", 0.02f},
             {"SENSITIVITY", 0.85f}, {"LOOKAHEAD_ENABLED", 1.0f},
             {"LOOKAHEAD_TIME", 0.50f}, {"OUTPUT_GAIN", 0.46f}},
            juce::var()
        },
        {
            "Sustain Lift",
            {{"MIX", 0.70f}, {"ATTACK_TIME", 0.40f}, {"SUSTAIN_TIME", 0.60f},
             {"SENSITIVITY", 0.45f}, {"LOOKAHEAD_ENABLED", 0.0f},
             {"LOOKAHEAD_TIME", 0.192f}, {"OUTPUT_GAIN", 0.52f}},
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(factoryPresets);

    // Cache parameter pointers (avoids string hash lookup 7x per processBlock)
    cachedMix = parameters.getRawParameterValue("MIX");
    cachedSensitivity = parameters.getRawParameterValue("SENSITIVITY");
    cachedAttackTime = parameters.getRawParameterValue("ATTACK_TIME");
    cachedSustainTime = parameters.getRawParameterValue("SUSTAIN_TIME");
    cachedLookaheadEnabled = parameters.getRawParameterValue("LOOKAHEAD_ENABLED");
    cachedLookaheadTime = parameters.getRawParameterValue("LOOKAHEAD_TIME");
    cachedOutputGain = parameters.getRawParameterValue("OUTPUT_GAIN");
}

OSpectralShaperAudioProcessor::~OSpectralShaperAudioProcessor()
{
}

// ============================================================================
// Audio Processing
// ============================================================================

void OSpectralShaperAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Report base latency (FFT size); updated dynamically when lookahead changes
    setLatencySamples(STFTProcessor::FFT_SIZE);

    // Prepare STFT processors (one per channel)
    for (int ch = 0; ch < 2; ++ch)
    {
        stftProcessor[ch].prepare(sampleRate);
        stftProcessor[ch].reset();
    }

    // Preallocate dry delay buffer (FFT_SIZE samples for latency matching)
    dryDelayBuffer.setSize(2, STFTProcessor::FFT_SIZE);
    dryDelayBuffer.clear();
    dryDelayWritePosition = 0;

    // Preallocate lookahead buffer (max 10ms @ highest sample rate)
    int maxLookaheadSamples = static_cast<int>(sampleRate * 0.010);  // 10ms
    lookaheadBuffer.setSize(2, maxLookaheadSamples);
    lookaheadBuffer.clear();
    lookaheadWritePosition = 0;
    lookaheadDelayLength = 0;

    juce::ignoreUnused(samplesPerBlock);
}

void OSpectralShaperAudioProcessor::releaseResources()
{
    // Optional: Release buffers to save memory
    dryDelayBuffer.setSize(0, 0);
    lookaheadBuffer.setSize(0, 0);
}

void OSpectralShaperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Edge case: zero-length buffer
    if (buffer.getNumSamples() == 0)
        return;

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);  // Stereo only
    const int numSamples = buffer.getNumSamples();

    // Read parameters (atomic, real-time safe — pointers cached in constructor)
    float mixValue = cachedMix->load();
    float sensitivity = cachedSensitivity->load();
    float attackTime = cachedAttackTime->load();
    float sustainTime = cachedSustainTime->load();
    lookaheadEnabled = cachedLookaheadEnabled->load() > 0.5f;
    float lookaheadTimeMs = cachedLookaheadTime->load();
    float outputGainDB = cachedOutputGain->load();
    float outputGain = juce::Decibels::decibelsToGain(outputGainDB);

    // Calculate lookahead delay length and update reported latency
    if (lookaheadEnabled)
    {
        lookaheadDelayLength = static_cast<int>(currentSampleRate * lookaheadTimeMs / 1000.0);
        lookaheadDelayLength = juce::jmin(lookaheadDelayLength, lookaheadBuffer.getNumSamples());
    }
    else
    {
        lookaheadDelayLength = 0;
    }
    setLatencySamples(STFTProcessor::FFT_SIZE + lookaheadDelayLength);

    // Update STFT parameters
    for (int ch = 0; ch < numChannels; ++ch)
    {
        stftProcessor[ch].setSensitivity(sensitivity);
        stftProcessor[ch].setAttackTime(attackTime);
        stftProcessor[ch].setSustainTime(sustainTime);
    }

    // Process sample-by-sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, sample);

            // Optional lookahead delay (reduces pre-ringing on sharp transients)
            float lookaheadInput = getLookaheadDelayedSample(ch, input);

            // Dry path (latency-matched to STFT processing)
            float dry = getDryDelayedSample(ch, lookaheadInput);

            // Wet path (STFT processing)
            float wet = stftProcessor[ch].processSample(lookaheadInput);

            // Mix and output gain
            float output = (dry * (1.0f - mixValue) + wet * mixValue) * outputGain;
            buffer.setSample(ch, sample, output);
        }

        advanceDryDelay();
        advanceLookahead();

        // Phase 3.3: Push visualization data once per FFT hop
        if (++hopCounter >= STFTProcessor::HOP_SIZE)
        {
            hopCounter = 0;

            // Build visualization frame from left channel (mono visualization)
            VisualizationFrame frame;
            frame.fftMagnitudes = stftProcessor[0].getLastMagnitudes();
            frame.transientActivity = stftProcessor[0].getTransientActivity();

            writeVisualizationFrame(frame);
        }
    }

    // Clear unused channels
    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);
}

// ============================================================================
// Editor
// ============================================================================

juce::AudioProcessorEditor* OSpectralShaperAudioProcessor::createEditor()
{
    return new OSpectralShaperAudioProcessorEditor(*this);
}

// ============================================================================
// State Management
// ============================================================================

void OSpectralShaperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OSpectralShaperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());
}

// ============================================================================
// Curve Access Methods
// ============================================================================

void OSpectralShaperAudioProcessor::setAttackCurve(const std::array<float, 32>& curve)
{
    attackCurve = curve;
    // Update STFT processors
    for (int ch = 0; ch < 2; ++ch)
        stftProcessor[ch].setAttackCurve(curve);
}

void OSpectralShaperAudioProcessor::setSustainCurve(const std::array<float, 32>& curve)
{
    sustainCurve = curve;
    // Update STFT processors
    for (int ch = 0; ch < 2; ++ch)
        stftProcessor[ch].setSustainCurve(curve);
}

// ============================================================================
// Dry Delay Buffer Helpers
// ============================================================================

float OSpectralShaperAudioProcessor::getDryDelayedSample(int channel, float input)
{
    // Write current input to delay buffer
    dryDelayBuffer.setSample(channel, dryDelayWritePosition, input);

    // Read delayed sample (FFT_SIZE samples ago for latency matching)
    int readPosition = (dryDelayWritePosition + 1) % STFTProcessor::FFT_SIZE;
    return dryDelayBuffer.getSample(channel, readPosition);
}

void OSpectralShaperAudioProcessor::advanceDryDelay()
{
    dryDelayWritePosition = (dryDelayWritePosition + 1) % STFTProcessor::FFT_SIZE;
}

float OSpectralShaperAudioProcessor::getLookaheadDelayedSample(int channel, float input)
{
    if (lookaheadDelayLength == 0)
        return input;

    lookaheadBuffer.setSample(channel, lookaheadWritePosition, input);

    int readPosition = (lookaheadWritePosition - lookaheadDelayLength + lookaheadBuffer.getNumSamples())
                       % lookaheadBuffer.getNumSamples();
    return lookaheadBuffer.getSample(channel, readPosition);
}

void OSpectralShaperAudioProcessor::advanceLookahead()
{
    if (lookaheadDelayLength > 0)
        lookaheadWritePosition = (lookaheadWritePosition + 1) % lookaheadBuffer.getNumSamples();
}

void OSpectralShaperAudioProcessor::writeVisualizationFrame(const VisualizationFrame& frame)
{
    // Write to FIFO if space available (audio thread, lock-free)
    if (visualizationFifo.getFreeSpace() > 0)
    {
        int start1, size1, start2, size2;
        visualizationFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            visualizationBuffer[static_cast<size_t>(start1)] = frame;
        }

        visualizationFifo.finishedWrite(size1);
    }
}

// ============================================================================
// Factory Function
// ============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSpectralShaperAudioProcessor();
}
