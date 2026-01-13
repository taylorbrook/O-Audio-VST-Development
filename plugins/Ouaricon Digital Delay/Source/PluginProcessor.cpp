/*
  ==============================================================================

    Ouaricon Digital Delay - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconDigitalDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // time - Float (1.0-2000.0 ms, default: 500.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "time", 1 },
        "Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 1.0f),
        500.0f,
        "ms"
    ));

    // sync - Bool (default: false)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "sync", 1 },
        "Sync",
        false
    ));

    // division - Choice (12 options, default: 1 "1/8")
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "division", 1 },
        "Division",
        juce::StringArray { "1/4", "1/8", "1/16", "1/4D", "1/8D", "1/16D",
                           "1/4T", "1/8T", "1/16T", "1/4(5)", "1/8(5)", "1/16(5)" },
        1  // Default: 1/8
    ));

    // feedback - Float (0.0-100.0%, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // spread - Float (0.0-100.0%, default: 0.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // mod - Float (0.0-100.0%, default: 0.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mod", 1 },
        "Mod",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // wet - Float (0.0-100.0%, default: 30.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wet", 1 },
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // dry - Float (0.0-100.0%, default: 100.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dry", 1 },
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        100.0f,
        "%"
    ));

    return layout;
}

OuariconDigitalDelayAudioProcessor::OuariconDigitalDelayAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "Ouaricon Digital Delay")
{
}

OuariconDigitalDelayAudioProcessor::~OuariconDigitalDelayAudioProcessor()
{
}

void OuariconDigitalDelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Calculate maximum delay buffer size (2000ms at up to 192kHz)
    // Add extra headroom for modulation and spread (25ms total)
    const double maxDelaySeconds = (2000.0 + 25.0) / 1000.0;
    const int maxDelaySamples = static_cast<int>(std::ceil(maxDelaySeconds * sampleRate));

    // Prepare delay lines with maximum size
    delayLineLeft.setMaximumDelayInSamples(maxDelaySamples);
    delayLineRight.setMaximumDelayInSamples(maxDelaySamples);
    delayLineLeft.prepare(spec);
    delayLineRight.prepare(spec);
    delayLineLeft.reset();
    delayLineRight.reset();

    // Prepare LFO (0.3Hz sine wave)
    lfo.prepare(spec);
    lfo.setFrequency(0.3f);
    lfo.initialise([](float x) { return std::sin(x); }, 128);

    // Reset smoothed values with 20ms ramp time
    const double rampTimeSamples = 0.02 * sampleRate;
    smoothedTimeMs.reset(rampTimeSamples);
    smoothedFeedback.reset(rampTimeSamples);
    smoothedSpread.reset(rampTimeSamples);
    smoothedMod.reset(rampTimeSamples);
    smoothedWet.reset(rampTimeSamples);
    smoothedDry.reset(rampTimeSamples);

    // Reset RMS meters with faster response (10ms)
    rmsLevelLeft.reset(sampleRate, 0.01);
    rmsLevelRight.reset(sampleRate, 0.01);

    // Clear feedback state
    feedbackLeft = 0.0f;
    feedbackRight = 0.0f;
}

void OuariconDigitalDelayAudioProcessor::releaseResources()
{
    // Optional: Release resources when plugin not in use
    // Delay lines will be cleaned up automatically
}

void OuariconDigitalDelayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Early return for empty buffers
    if (numSamples == 0 || numChannels == 0)
        return;

    // Read parameters (atomic, real-time safe)
    auto* timeParam = parameters.getRawParameterValue("time");
    auto* syncParam = parameters.getRawParameterValue("sync");
    auto* divisionParam = parameters.getRawParameterValue("division");
    auto* feedbackParam = parameters.getRawParameterValue("feedback");
    auto* spreadParam = parameters.getRawParameterValue("spread");
    auto* modParam = parameters.getRawParameterValue("mod");
    auto* wetParam = parameters.getRawParameterValue("wet");
    auto* dryParam = parameters.getRawParameterValue("dry");

    // Calculate delay time (free mode or synced mode)
    float delayTimeMs = timeParam->load();
    bool isSync = syncParam->load() > 0.5f;

    if (isSync)
    {
        // Tempo sync mode: Calculate delay from BPM + subdivision
        auto* playHead = getPlayHead();
        if (playHead != nullptr)
        {
            auto positionInfo = playHead->getPosition();
            if (positionInfo.hasValue() && positionInfo->getBpm().hasValue())
            {
                double bpm = *positionInfo->getBpm();
                if (bpm > 0.0)
                {
                    int divisionIndex = static_cast<int>(divisionParam->load());
                    divisionIndex = juce::jlimit(0, 11, divisionIndex);

                    // Calculate delay time from BPM: (60000ms / bpm) * subdivisionFactor
                    float subdivisionFactor = subdivisionFactors[divisionIndex];
                    delayTimeMs = static_cast<float>((60000.0 / bpm) * subdivisionFactor);

                    // Clamp to valid range
                    delayTimeMs = juce::jlimit(1.0f, 2000.0f, delayTimeMs);
                }
            }
        }
    }

    // Set smoothed parameter targets
    smoothedTimeMs.setTargetValue(delayTimeMs);
    smoothedFeedback.setTargetValue(feedbackParam->load() / 100.0f);
    smoothedSpread.setTargetValue(spreadParam->load() / 100.0f);
    smoothedMod.setTargetValue(modParam->load() / 100.0f);
    smoothedWet.setTargetValue(wetParam->load() / 100.0f);
    smoothedDry.setTargetValue(dryParam->load() / 100.0f);

    // Process audio sample-by-sample for feedback loop
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get smoothed parameter values for this sample
        float currentDelayMs = smoothedTimeMs.getNextValue();
        float currentFeedback = juce::jlimit(0.0f, 0.95f, smoothedFeedback.getNextValue());
        float currentSpread = smoothedSpread.getNextValue();
        float currentMod = smoothedMod.getNextValue();
        float currentWet = smoothedWet.getNextValue();
        float currentDry = smoothedDry.getNextValue();

        // Calculate base delay time in samples
        float baseDelaySamples = (currentDelayMs / 1000.0f) * static_cast<float>(spec.sampleRate);

        // Calculate stereo spread offset for right channel (0-15ms)
        float spreadMs = currentSpread * 15.0f;
        float spreadSamples = (spreadMs / 1000.0f) * static_cast<float>(spec.sampleRate);

        // Get LFO modulation (0-10ms depth)
        float lfoValue = lfo.processSample(0.0f);  // Returns -1 to +1
        float modDepthMs = currentMod * 10.0f;
        float modSamples = (modDepthMs / 1000.0f) * static_cast<float>(spec.sampleRate) * lfoValue;

        // Calculate final delay times for each channel
        float leftDelaySamples = baseDelaySamples + modSamples;
        float rightDelaySamples = baseDelaySamples + spreadSamples + modSamples;

        // Clamp to valid range (minimum 1 sample)
        leftDelaySamples = juce::jmax(1.0f, leftDelaySamples);
        rightDelaySamples = juce::jmax(1.0f, rightDelaySamples);

        // Process left channel
        if (numChannels >= 1)
        {
            float* leftChannel = buffer.getWritePointer(0);
            float inputSample = leftChannel[sample];
            float drySample = inputSample;

            // Add feedback to input
            inputSample += feedbackLeft;

            // Push to delay line
            delayLineLeft.pushSample(0, inputSample);

            // Read delayed sample with interpolation
            float delayedSample = delayLineLeft.popSample(0, leftDelaySamples);

            // Store feedback for next iteration
            feedbackLeft = delayedSample * currentFeedback;

            // Mix wet and dry
            float wetSample = delayedSample * currentWet;
            float dryOutput = drySample * currentDry;
            leftChannel[sample] = wetSample + dryOutput;
        }

        // Process right channel
        if (numChannels >= 2)
        {
            float* rightChannel = buffer.getWritePointer(1);
            float inputSample = rightChannel[sample];
            float drySample = inputSample;

            // Add feedback to input
            inputSample += feedbackRight;

            // Push to delay line
            delayLineRight.pushSample(0, inputSample);

            // Read delayed sample with interpolation
            float delayedSample = delayLineRight.popSample(0, rightDelaySamples);

            // Store feedback for next iteration
            feedbackRight = delayedSample * currentFeedback;

            // Mix wet and dry
            float wetSample = delayedSample * currentWet;
            float dryOutput = drySample * currentDry;
            rightChannel[sample] = wetSample + dryOutput;
        }
    }

    // Calculate RMS levels for output meter
    if (numChannels >= 1)
    {
        float sumSquares = 0.0f;
        auto* leftChannel = buffer.getReadPointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = leftChannel[i];
            sumSquares += sample * sample;
        }
        float rms = std::sqrt(sumSquares / static_cast<float>(numSamples));
        rmsLevelLeft.setTargetValue(rms);
        rmsLevelLeft.skip(numSamples);
    }

    if (numChannels >= 2)
    {
        float sumSquares = 0.0f;
        auto* rightChannel = buffer.getReadPointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = rightChannel[i];
            sumSquares += sample * sample;
        }
        float rms = std::sqrt(sumSquares / static_cast<float>(numSamples));
        rmsLevelRight.setTargetValue(rms);
        rmsLevelRight.skip(numSamples);
    }
}

double OuariconDigitalDelayAudioProcessor::getTailLengthSeconds() const
{
    // Calculate tail length based on delay time and feedback
    auto* feedbackParam = parameters.getRawParameterValue("feedback");
    float feedbackValue = feedbackParam->load() / 100.0f;
    feedbackValue = juce::jlimit(0.0f, 0.95f, feedbackValue);

    // If no feedback, no tail
    if (feedbackValue < 0.01f)
        return 0.0;

    // Calculate maximum possible delay time (2000ms)
    const double maxDelaySeconds = 2.0;

    // Tail length formula: maxDelay * (1 / (1 - feedback))
    // This calculates how long it takes for the delay to decay to silence
    double tailLength = maxDelaySeconds * (1.0 / (1.0 - static_cast<double>(feedbackValue)));

    // Cap at reasonable maximum (30 seconds)
    return juce::jmin(tailLength, 30.0);
}

juce::AudioProcessorEditor* OuariconDigitalDelayAudioProcessor::createEditor()
{
    return new OuariconDigitalDelayAudioProcessorEditor(*this);
}

void OuariconDigitalDelayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Use preset manager for state serialization (includes APVTS + current preset name)
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OuariconDigitalDelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    // Use preset manager for state restoration
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconDigitalDelayAudioProcessor();
}
