/*
  ==============================================================================

    O-Freeze - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OFreezeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // FREEZE - Manual trigger button
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "FREEZE", 1 },
        "Freeze",
        false));

    // THRESHOLD - Auto-freeze level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "THRESHOLD", 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -40.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // MODE - Trigger mode selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MODE", 1 },
        "Mode",
        juce::StringArray { "Manual", "Threshold" },
        0));

    // DRIFT - Grain position randomization
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRIFT", 1 },
        "Drift",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // MIX - Dry/Wet blend
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return layout;
}

OFreezeAudioProcessor::OFreezeAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OFreezeAudioProcessor::~OFreezeAudioProcessor()
{
}

void OFreezeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Pre-allocate freeze buffer for 2 seconds at max sample rate (192kHz = 384,000 samples)
    const int maxSampleRate = 192000;
    const int bufferLengthSeconds = 2;
    const int bufferLength = maxSampleRate * bufferLengthSeconds;
    freezeBuffer.setSize(getTotalNumOutputChannels(), bufferLength);
    freezeBuffer.clear();

    // Reset positions
    writePosition = 0;
    readPosition = 0;
    bufferFrozen = false;

    // Initialize crossfade smoother (50ms fade-in, 100ms fade-out)
    freezeGain.reset(sampleRate, 0.050); // 50ms ramp time
    freezeGain.setCurrentAndTargetValue(0.0f);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();

    // Initialize threshold gate RMS detection (20ms rolling window)
    rmsSamplesPerWindow = static_cast<int>(sampleRate * 0.020);
    rmsBuffer.resize(rmsSamplesPerWindow);
    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
    rmsWriteIndex = 0;
    rmsLevel = 0.0f;
    gateState = GateState::Idle;

    // Initialize granular synthesis components
    grainSize = static_cast<int>(sampleRate * 0.200); // 200ms grains
    grainTriggerInterval = grainSize / 8; // 8 grains with 87.5% overlap

    // Pre-compute asymmetric Blackman-Harris window (extended attack, softer onsets)
    // Attack: 60% of grain, Release: 40% of grain
    hannWindow.resize(grainSize);
    const double PI = juce::MathConstants<double>::pi;
    const int attackLen = static_cast<int>(grainSize * 0.6);
    const int releaseLen = grainSize - attackLen;

    // Blackman-Harris coefficients
    const double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;

    // Attack phase (stretched Blackman-Harris rise)
    for (int i = 0; i < attackLen; ++i)
    {
        double phase = 0.5 * static_cast<double>(i) / (attackLen - 1); // 0 to 0.5
        double w = a0 - a1 * std::cos(2.0 * PI * phase)
                      + a2 * std::cos(4.0 * PI * phase)
                      - a3 * std::cos(6.0 * PI * phase);
        hannWindow[i] = static_cast<float>(w);
    }

    // Release phase (compressed Blackman-Harris fall)
    for (int i = 0; i < releaseLen; ++i)
    {
        double phase = 0.5 + 0.5 * static_cast<double>(i) / (releaseLen - 1); // 0.5 to 1.0
        double w = a0 - a1 * std::cos(2.0 * PI * phase)
                      + a2 * std::cos(4.0 * PI * phase)
                      - a3 * std::cos(6.0 * PI * phase);
        hannWindow[attackLen + i] = static_cast<float>(w);
    }

    // Reset all grains to inactive
    for (auto& grain : grains)
    {
        grain.active = false;
        grain.startSample = 0;
        grain.position = 0;
    }

    grainTriggerCounter = 0;
    nextGrainIndex = 0;
}

void OFreezeAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2 (DSP)
}

void OFreezeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), freezeBuffer.getNumChannels());

    if (numSamples == 0)
        return;

    // Read parameters (atomic, real-time safe)
    auto* freezeParam = parameters.getRawParameterValue("FREEZE");
    auto* modeParam = parameters.getRawParameterValue("MODE");
    auto* thresholdParam = parameters.getRawParameterValue("THRESHOLD");
    auto* mixParam = parameters.getRawParameterValue("MIX");

    int modeValue = static_cast<int>(modeParam->load());
    float thresholdDB = thresholdParam->load();
    float mixValue = mixParam->load() / 100.0f; // Convert 0-100% to 0-1

    // Determine freeze state based on mode
    bool freezeActive = false;

    if (modeValue == 0) // Manual mode
    {
        freezeActive = freezeParam->load() > 0.5f;
    }
    else // Threshold mode
    {
        // Calculate RMS level from input buffer
        float sumSquares = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getReadPointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float inputSample = channelData[sample];

                // Update RMS buffer (circular)
                rmsBuffer[rmsWriteIndex] = inputSample * inputSample;
                rmsWriteIndex = (rmsWriteIndex + 1) % rmsSamplesPerWindow;
            }
        }

        // Calculate RMS from rolling window
        for (float squaredSample : rmsBuffer)
            sumSquares += squaredSample;

        float rms = std::sqrt(sumSquares / static_cast<float>(rmsSamplesPerWindow * numChannels));
        rmsLevel = 20.0f * std::log10(rms + 1e-6f); // Convert to dB

        // Threshold gate with hysteresis (3dB)
        const float hysteresisDB = 3.0f;

        if (gateState == GateState::Idle && rmsLevel < thresholdDB)
        {
            gateState = GateState::Frozen;
        }
        else if (gateState == GateState::Frozen && rmsLevel > (thresholdDB + hysteresisDB))
        {
            gateState = GateState::Idle;
        }

        freezeActive = (gateState == GateState::Frozen);
    }

    // Read DRIFT parameter
    auto* driftParam = parameters.getRawParameterValue("DRIFT");
    float driftValue = driftParam->load() / 100.0f; // Convert 0-100% to 0-1

    // Push dry signal to mixer
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    // Update freeze state and crossfade envelope
    if (freezeActive != bufferFrozen)
    {
        bufferFrozen = freezeActive;

        if (bufferFrozen)
        {
            // Freeze engaged: fade in frozen signal
            freezeGain.reset(currentSampleRate, 0.050); // 50ms fade-in
            freezeGain.setTargetValue(1.0f);

            // Immediately trigger all 8 grains with staggered positions for instant full overlap
            // Start reading from BEHIND writePosition (where audio actually exists)
            const int freezeBufLen = freezeBuffer.getNumSamples();
            int startPos = (writePosition - grainSize + freezeBufLen) % freezeBufLen;
            for (int i = 0; i < 8; ++i)
            {
                grains[i].active = true;
                grains[i].startSample = i * (grainSize / 8); // Stagger within grain window
                grains[i].position = (startPos + i * (grainSize / 8)) % freezeBufLen;
            }
            nextGrainIndex = 0;
            grainTriggerCounter = 0;
        }
        else
        {
            // Freeze released: fade out frozen signal
            freezeGain.reset(currentSampleRate, 0.100); // 100ms fade-out
            freezeGain.setTargetValue(0.0f);

            // Deactivate all grains
            for (auto& grain : grains)
                grain.active = false;
        }
    }

    const int freezeBufferLength = freezeBuffer.getNumSamples();

    // Process sample-by-sample (all channels together) so grain state stays in sync
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Trigger new grain (once per trigger interval)
        if (bufferFrozen)
        {
            if (grainTriggerCounter >= grainTriggerInterval)
            {
                // Activate new grain
                Grain& newGrain = grains[nextGrainIndex];
                newGrain.active = true;
                newGrain.startSample = 0;

                // Calculate grain position with drift
                // Start from behind writePosition (where audio exists), drift adds variation
                int basePos = (writePosition - grainSize + freezeBufferLength) % freezeBufferLength;
                int driftRange = static_cast<int>(grainSize * driftValue); // Drift within captured grain
                int driftOffset = random.nextInt(driftRange + 1);
                newGrain.position = (basePos + driftOffset) % freezeBufferLength;

                // Advance grain index (round-robin)
                nextGrainIndex = (nextGrainIndex + 1) % 8;

                // Reset trigger counter
                grainTriggerCounter = 0;
            }
            else
            {
                grainTriggerCounter++;
            }
        }

        // Get current window values and positions for all active grains (before advancing)
        float windowValues[8] = {0};
        int grainPositions[8] = {0};
        int activeGrainCount = 0;

        for (int g = 0; g < 8; ++g)
        {
            if (grains[g].active)
            {
                windowValues[g] = hannWindow[grains[g].startSample];
                grainPositions[g] = grains[g].position;
                activeGrainCount++;
            }
        }

        // Process each channel using the same grain state
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* freezeData = freezeBuffer.getWritePointer(channel);

            float inputSample = channelData[sample];
            float outputSample = inputSample;

            // Write to freeze buffer (if not frozen)
            if (!bufferFrozen)
            {
                freezeData[writePosition] = inputSample;
            }

            // Granular synthesis (if frozen or fading out)
            if (bufferFrozen || freezeGain.getCurrentValue() > 0.001f)
            {
                // Sum all active grains (overlap-add synthesis)
                float granularSum = 0.0f;

                for (int g = 0; g < 8; ++g)
                {
                    if (grains[g].active)
                    {
                        // Read from freeze buffer at grain position
                        float grainSample = freezeData[grainPositions[g]];

                        // Apply Hann window
                        granularSum += grainSample * windowValues[g];
                    }
                }

                // Normalize output to prevent clipping
                float frozenSample = (activeGrainCount > 0) ? (granularSum / activeGrainCount) : 0.0f;

                // Apply crossfade envelope
                float currentGain = freezeGain.getNextValue();
                outputSample = inputSample * (1.0f - currentGain) + frozenSample * currentGain;
            }

            channelData[sample] = outputSample;
        }

        // Advance grain state AFTER processing all channels
        for (auto& grain : grains)
        {
            if (grain.active)
            {
                grain.startSample++;
                grain.position = (grain.position + 1) % freezeBufferLength;

                // Deactivate grain when complete
                if (grain.startSample >= grainSize)
                {
                    grain.active = false;
                }
            }
        }

        // Advance write position AFTER processing all channels
        if (!bufferFrozen)
        {
            writePosition = (writePosition + 1) % freezeBufferLength;
        }
    }

    // Apply dry/wet mix
    dryWetMixer.setWetMixProportion(mixValue);
    dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}

juce::AudioProcessorEditor* OFreezeAudioProcessor::createEditor()
{
    return new OFreezeAudioProcessorEditor(*this);
}

void OFreezeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OFreezeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OFreezeAudioProcessor();
}
