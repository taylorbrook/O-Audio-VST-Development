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
{
    // Initialize curves to neutral (no shaping)
    std::fill(attackCurve.begin(), attackCurve.end(), 0.0f);
    std::fill(sustainCurve.begin(), sustainCurve.end(), 0.0f);
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

    // Report fixed 512-sample latency (FFT size)
    setLatencySamples(512);

    // Prepare STFT processors (one per channel)
    for (int ch = 0; ch < 2; ++ch)
    {
        stftProcessor[ch].prepare(sampleRate);
        stftProcessor[ch].reset();
    }

    // Preallocate dry delay buffer (512 samples for latency matching)
    dryDelayBuffer.setSize(2, 512);
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

    // Read parameters (atomic, real-time safe)
    auto* mixParam = parameters.getRawParameterValue("MIX");
    auto* sensitivityParam = parameters.getRawParameterValue("SENSITIVITY");
    auto* attackTimeParam = parameters.getRawParameterValue("ATTACK_TIME");
    auto* sustainTimeParam = parameters.getRawParameterValue("SUSTAIN_TIME");
    auto* lookaheadEnabledParam = parameters.getRawParameterValue("LOOKAHEAD_ENABLED");
    auto* lookaheadTimeParam = parameters.getRawParameterValue("LOOKAHEAD_TIME");
    auto* outputGainParam = parameters.getRawParameterValue("OUTPUT_GAIN");

    float mixValue = mixParam->load();
    float sensitivity = sensitivityParam->load();
    float attackTime = attackTimeParam->load();
    float sustainTime = sustainTimeParam->load();
    lookaheadEnabled = lookaheadEnabledParam->load() > 0.5f;
    float lookaheadTimeMs = lookaheadTimeParam->load();
    float outputGainDB = outputGainParam->load();
    float outputGain = juce::Decibels::decibelsToGain(outputGainDB);

    // Calculate lookahead delay length
    if (lookaheadEnabled)
    {
        lookaheadDelayLength = static_cast<int>(currentSampleRate * lookaheadTimeMs / 1000.0);
        lookaheadDelayLength = juce::jmin(lookaheadDelayLength, lookaheadBuffer.getNumSamples());
    }
    else
    {
        lookaheadDelayLength = 0;
    }

    // Update STFT parameters
    for (int ch = 0; ch < numChannels; ++ch)
    {
        stftProcessor[ch].setSensitivity(sensitivity);
        stftProcessor[ch].setAttackTime(attackTime);
        stftProcessor[ch].setSustainTime(sustainTime);
        stftProcessor[ch].setAttackCurve(attackCurve);
        stftProcessor[ch].setSustainCurve(sustainCurve);
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
    auto state = parameters.copyState();

    // Add curve data as child nodes
    auto curvesXml = state.getOrCreateChildWithName("Curves", nullptr);

    // Encode attack curve as hex string
    juce::String attackHex = juce::String::toHexString(
        reinterpret_cast<const juce::uint8*>(attackCurve.data()),
        static_cast<int>(attackCurve.size() * sizeof(float))
    );
    curvesXml.setProperty("attackCurve", attackHex, nullptr);

    // Encode sustain curve as hex string
    juce::String sustainHex = juce::String::toHexString(
        reinterpret_cast<const juce::uint8*>(sustainCurve.data()),
        static_cast<int>(sustainCurve.size() * sizeof(float))
    );
    curvesXml.setProperty("sustainCurve", sustainHex, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OSpectralShaperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        parameters.replaceState(state);

        // Restore curve data
        auto curves = state.getChildWithName("Curves");
        if (curves.isValid())
        {
            // Decode attack curve from hex string
            juce::String attackHex = curves.getProperty("attackCurve", "");
            if (attackHex.isNotEmpty())
            {
                juce::MemoryBlock attackBlock;
                attackBlock.loadFromHexString(attackHex);
                if (attackBlock.getSize() == attackCurve.size() * sizeof(float))
                {
                    std::memcpy(attackCurve.data(), attackBlock.getData(), attackBlock.getSize());
                    setAttackCurve(attackCurve);  // Update STFT processors
                }
            }

            // Decode sustain curve from hex string
            juce::String sustainHex = curves.getProperty("sustainCurve", "");
            if (sustainHex.isNotEmpty())
            {
                juce::MemoryBlock sustainBlock;
                sustainBlock.loadFromHexString(sustainHex);
                if (sustainBlock.getSize() == sustainCurve.size() * sizeof(float))
                {
                    std::memcpy(sustainCurve.data(), sustainBlock.getData(), sustainBlock.getSize());
                    setSustainCurve(sustainCurve);  // Update STFT processors
                }
            }
        }
    }
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

    // Read delayed sample (512 samples ago for latency matching)
    int readPosition = (dryDelayWritePosition + 1) % 512;
    return dryDelayBuffer.getSample(channel, readPosition);
}

void OSpectralShaperAudioProcessor::advanceDryDelay()
{
    dryDelayWritePosition = (dryDelayWritePosition + 1) % 512;
}

float OSpectralShaperAudioProcessor::getLookaheadDelayedSample(int channel, float input)
{
    if (lookaheadDelayLength == 0)
        return input;  // No lookahead, pass through

    // Write current input to lookahead buffer
    lookaheadBuffer.setSample(channel, lookaheadWritePosition, input);

    // Read delayed sample (lookaheadDelayLength samples ago)
    int readPosition = (lookaheadWritePosition - lookaheadDelayLength + lookaheadBuffer.getNumSamples())
                       % lookaheadBuffer.getNumSamples();
    float delayed = lookaheadBuffer.getSample(channel, readPosition);

    // Advance write position
    lookaheadWritePosition = (lookaheadWritePosition + 1) % lookaheadBuffer.getNumSamples();

    return delayed;
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
