#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (BEFORE constructor)
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AutoClipAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // clipThreshold - Float (0-100%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "clipThreshold", 1 },
        "Clip Threshold",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f, 1.0f),  // min, max, step, skew
        0.0f,
        "%"
    ));

    // soloClipped - Bool (default: false)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "soloClipped", 1 },
        "Clip Solo",
        false
    ));

    return layout;
}

//==============================================================================
// Constructor
//==============================================================================
AutoClipAudioProcessor::AutoClipAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

AutoClipAudioProcessor::~AutoClipAudioProcessor()
{
}

//==============================================================================
// Audio Processing
//==============================================================================
void AutoClipAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Phase 4.1: Prepare lookahead delay lines (5ms fixed delay)
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lookaheadSamples = static_cast<int>(0.005 * sampleRate);  // 5ms in samples

    lookaheadDelayL.prepare(spec);
    lookaheadDelayR.prepare(spec);
    lookaheadDelayL.setMaximumDelayInSamples(lookaheadSamples);
    lookaheadDelayR.setMaximumDelayInSamples(lookaheadSamples);
    lookaheadDelayL.reset();
    lookaheadDelayR.reset();

    // Phase 4.2: Prepare gain smoothing (50ms time constant)
    smoothedGain.reset(sampleRate, 0.05);  // 50ms smoothing
    smoothedGain.setCurrentAndTargetValue(1.0f);  // Default gain = 1.0

    // Phase 4.3: Preallocate original buffer for clip solo
    originalBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    originalBuffer.clear();
}

void AutoClipAudioProcessor::releaseResources()
{
    // Release large buffers to save memory when plugin not in use
    originalBuffer.setSize(0, 0);
}

void AutoClipAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Read parameters (atomic, real-time safe)
    auto* clipThresholdParam = parameters.getRawParameterValue("clipThreshold");
    float clipThresholdPercent = clipThresholdParam->load();
    float clipThreshold = clipThresholdPercent * 0.01f;  // Convert 0-100% to 0.0-1.0

    auto* soloClippedParam = parameters.getRawParameterValue("soloClipped");
    bool soloClipped = soloClippedParam->load() > 0.5f;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Phase 4.3: Store original signal before processing
    originalBuffer.setSize(numChannels, numSamples, false, false, true);
    for (int channel = 0; channel < numChannels; ++channel)
    {
        originalBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
    }

    // Phase 4.1 & 4.2: Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& delayLine = (channel == 0) ? lookaheadDelayL : lookaheadDelayR;

        // Reset peak detectors for this block
        inputPeak = 0.0f;
        outputPeak = 0.0f;

        // Process sample by sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Phase 4.1: Push input to lookahead buffer
            delayLine.pushSample(channel, channelData[sample]);

            // Phase 4.2: Analyze input peak from lookahead buffer (before clipping)
            // Get delayed sample from lookahead
            float delayedSample = delayLine.popSample(channel, lookaheadSamples);
            inputPeak = juce::jmax(inputPeak, std::abs(delayedSample));

            // Phase 4.1: Apply hard clipping
            float clippedSample = juce::jlimit(-clipThreshold, clipThreshold, delayedSample);

            // Phase 4.2: Analyze output peak from clipped signal
            outputPeak = juce::jmax(outputPeak, std::abs(clippedSample));

            // Store clipped sample (will apply gain in second pass)
            channelData[sample] = clippedSample;
        }
    }

    // Phase 4.2: Calculate gain compensation (after analyzing all channels)
    float targetGain = 1.0f;
    if (outputPeak > 0.001f && inputPeak > 0.001f)
    {
        targetGain = inputPeak / outputPeak;  // Restore to input peak level
    }
    smoothedGain.setTargetValue(targetGain);

    // Apply smoothed gain to all channels
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float currentGain = smoothedGain.getNextValue();
            channelData[sample] *= currentGain;
        }
    }

    // Phase 4.3: Clip solo routing (output difference signal if enabled)
    if (soloClipped)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* outputData = buffer.getWritePointer(channel);
            auto* originalData = originalBuffer.getReadPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Difference signal = original - clipped_with_gain
                outputData[sample] = originalData[sample] - outputData[sample];
            }
        }
    }
}

//==============================================================================
// Editor Creation
//==============================================================================
juce::AudioProcessorEditor* AutoClipAudioProcessor::createEditor()
{
    return new AutoClipAudioProcessorEditor(*this);
}

//==============================================================================
// State Management
//==============================================================================
void AutoClipAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AutoClipAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Factory function
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoClipAudioProcessor();
}
