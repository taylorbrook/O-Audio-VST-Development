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

    // DRIFT - Grain position randomization range
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
    grainSize = static_cast<int>(sampleRate * 0.400); // 400ms grains
    grainTriggerInterval = grainSize / NUM_GRAINS; // 8 grains with 87.5% overlap

    // True Hann window scaled for COLA (Constant Overlap-Add)
    // With 8 grains at 87.5% overlap (hop = N/8), Hann windows sum to 4.0
    // Scale by 0.25 so overlapping grains sum to 1.0 (no normalization needed)
    hannWindow.resize(grainSize);
    const double PI = juce::MathConstants<double>::pi;
    const float colaScale = 0.25f;  // 1/4 scaling for 8-grain COLA sum = 1.0

    for (int i = 0; i < grainSize; ++i)
    {
        // Standard Hann window: 0.5 * (1 - cos(2πn/N))
        double phase = static_cast<double>(i) / static_cast<double>(grainSize);
        float hannValue = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * PI * phase)));
        hannWindow[i] = hannValue * colaScale;
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
    stopTriggeringNewGrains = false;

    // Initialize drift state
    frozenDriftOffset = 0.0f;
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

            // LOCK drift offset at freeze moment - all grains share this for COLA
            // Pick a random offset (adds variation between freeze engages)
            frozenDriftOffset = random.nextFloat();

            // Staggered activation: only activate FIRST grain immediately
            // Subsequent grains will be activated by normal trigger mechanism
            const int freezeBufLen = freezeBuffer.getNumSamples();
            int startPos = (writePosition - grainSize + freezeBufLen) % freezeBufLen;

            // Activate only the first grain
            grains[0].active = true;
            grains[0].startSample = 0;
            grains[0].position = startPos;

            // Deactivate all other grains - they'll be triggered naturally
            for (int i = 1; i < NUM_GRAINS; ++i)
            {
                grains[i].active = false;
            }

            nextGrainIndex = 1;  // Next grain to trigger
            grainTriggerCounter = 0;
            stopTriggeringNewGrains = false;  // Ensure we're triggering grains
        }
        else
        {
            // Freeze released: soft deactivation
            // Stop triggering NEW grains, but let active grains complete their cycle
            stopTriggeringNewGrains = true;

            // Extended fade-out to cover grain completion time (grainSize samples ≈ 350ms)
            // Add extra 50ms safety margin
            freezeGain.reset(currentSampleRate, 0.400); // 400ms fade-out
            freezeGain.setTargetValue(0.0f);

            // DON'T deactivate grains here - they'll naturally complete
            // when startSample >= grainSize (already handled in grain advance loop)
        }
    }

    const int freezeBufferLength = freezeBuffer.getNumSamples();

    // Process sample-by-sample (all channels together) so grain state stays in sync
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Only trigger new grains if frozen AND not in soft release mode
        if (bufferFrozen && !stopTriggeringNewGrains)
        {
            // IMPORTANT: Drift offset is LOCKED when freeze engages (set in freeze activation)
            // Do NOT update drift while frozen - all grains must share the same offset
            // for COLA phase alignment to work correctly

            if (grainTriggerCounter >= grainTriggerInterval)
            {
                // Activate new grain
                Grain& newGrain = grains[nextGrainIndex];
                newGrain.active = true;
                newGrain.startSample = 0;

                // Store BASE position only - drift applied at read time for COLA
                int basePos = (writePosition - grainSize + freezeBufferLength) % freezeBufferLength;
                newGrain.position = basePos;

                // Advance grain index (round-robin)
                nextGrainIndex = (nextGrainIndex + 1) % NUM_GRAINS;

                // Reset trigger counter
                grainTriggerCounter = 0;
            }
            else
            {
                grainTriggerCounter++;
            }
        }

        // Calculate shared drift offset (all grains use same offset for COLA)
        int driftRange = static_cast<int>(grainSize * driftValue);
        int sharedDriftOffset = static_cast<int>(frozenDriftOffset * driftRange);

        // Get current window values and positions for all active grains (before advancing)
        float windowValues[NUM_GRAINS] = {0};
        int grainPositions[NUM_GRAINS] = {0};

        for (int g = 0; g < NUM_GRAINS; ++g)
        {
            if (grains[g].active)
            {
                windowValues[g] = hannWindow[grains[g].startSample];
                // Apply shared drift offset at READ time (maintains COLA phase alignment)
                grainPositions[g] = (grains[g].position + sharedDriftOffset + freezeBufferLength) % freezeBufferLength;
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
                // COLA property: 8 Hann windows at 87.5% overlap sum to constant
                // Window is pre-scaled by 0.25 so sum = 1.0 (no normalization needed)
                float granularSum = 0.0f;

                for (int g = 0; g < NUM_GRAINS; ++g)
                {
                    if (grains[g].active)
                    {
                        // Read from freeze buffer at grain position
                        float grainSample = freezeData[grainPositions[g]];

                        // Apply pre-scaled Hann window
                        granularSum += grainSample * windowValues[g];
                    }
                }

                // Direct sum - COLA guarantees constant amplitude
                float frozenSample = granularSum;

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
