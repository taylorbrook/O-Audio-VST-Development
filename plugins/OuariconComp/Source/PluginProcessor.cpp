/*
  ==============================================================================

    OuariconComp - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconCompAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // threshold - Compression threshold level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "threshold", 1 },
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        "dB"
    ));

    // ratio - Compression ratio
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ratio", 1 },
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
        2.0f,
        ":1"
    ));

    // attack_time - Attack time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attack_time", 1 },
        "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
        10.0f,
        "ms"
    ));

    // release_time - Release time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "release_time", 1 },
        "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f),
        100.0f,
        "ms"
    ));

    // knee - Knee width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "knee", 1 },
        "Knee",
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f),
        6.0f,
        "dB"
    ));

    // output_gain - Output makeup gain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // auto_gain - Automatic makeup gain
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "auto_gain", 1 },
        "Auto Gain",
        false
    ));

    return layout;
}

OuariconCompAudioProcessor::OuariconCompAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "OuariconComp")
{
}

OuariconCompAudioProcessor::~OuariconCompAudioProcessor()
{
}

void OuariconCompAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Configure DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Initialize envelope state
    envelopeDB = -60.0f;

    // Calculate initial coefficients
    auto* attackParam = parameters.getRawParameterValue("attack_time");
    auto* releaseParam = parameters.getRawParameterValue("release_time");
    updateCoefficients(attackParam->load(), releaseParam->load(), sampleRate);
}

void OuariconCompAudioProcessor::releaseResources()
{
    // Reset envelope state
    envelopeDB = -60.0f;
}

void OuariconCompAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    auto* thresholdParam = parameters.getRawParameterValue("threshold");
    auto* ratioParam = parameters.getRawParameterValue("ratio");
    auto* attackParam = parameters.getRawParameterValue("attack_time");
    auto* releaseParam = parameters.getRawParameterValue("release_time");
    auto* kneeParam = parameters.getRawParameterValue("knee");
    auto* outputGainParam = parameters.getRawParameterValue("output_gain");
    auto* autoGainParam = parameters.getRawParameterValue("auto_gain");

    float thresholdDB = thresholdParam->load();
    float ratio = ratioParam->load();
    float attackTimeMs = attackParam->load();
    float releaseTimeMs = releaseParam->load();
    float kneeDB = kneeParam->load();
    float outputGainDB = outputGainParam->load();
    bool autoGainEnabled = autoGainParam->load() > 0.5f;

    // Update attack/release coefficients (lightweight calculation, real-time safe)
    updateCoefficients(attackTimeMs, releaseTimeMs, spec.sampleRate);

    // Calculate makeup gain
    float autoGainDB = 0.0f;
    if (autoGainEnabled)
    {
        autoGainDB = -thresholdDB * (1.0f - 1.0f / ratio);
    }
    float totalGainDB = autoGainDB + outputGainDB;
    float makeupGainLinear = juce::Decibels::decibelsToGain(totalGainDB);

    // Process audio (per-sample loop for accurate envelope following)
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Track peak levels for metering
    float peakInputLevel = 0.0f;
    float peakOutputLevel = 0.0f;
    float peakGainReduction = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Stereo-linked detection: use max of all channels
        float maxInputLevel = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            float inputLevel = std::abs(channelData[sample]);
            maxInputLevel = std::max(maxInputLevel, inputLevel);
        }

        // Track peak input for metering
        peakInputLevel = std::max(peakInputLevel, maxInputLevel);

        // Convert to dB
        float inputLevelDBLocal = juce::Decibels::gainToDecibels(maxInputLevel, -60.0f);

        // Envelope follower (attack/release ballistics)
        if (inputLevelDBLocal > envelopeDB)
        {
            // Attack (signal increasing)
            envelopeDB += (inputLevelDBLocal - envelopeDB) * attackCoeff;
        }
        else
        {
            // Release (signal decreasing)
            envelopeDB += (inputLevelDBLocal - envelopeDB) * releaseCoeff;
        }

        // Calculate gain reduction
        float gainReductionDBLocal = calculateGainReduction(envelopeDB, thresholdDB, ratio, kneeDB);
        peakGainReduction = std::max(peakGainReduction, gainReductionDBLocal);

        // Convert to linear gain
        float gainLinear = juce::Decibels::decibelsToGain(-gainReductionDBLocal) * makeupGainLinear;

        // Apply same gain to all channels (stereo-linked)
        float maxOutputLevel = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            channelData[sample] *= gainLinear;
            maxOutputLevel = std::max(maxOutputLevel, std::abs(channelData[sample]));
        }
        peakOutputLevel = std::max(peakOutputLevel, maxOutputLevel);
    }

    // Update atomic meter values (thread-safe for UI access)
    inputLevelDB.store(juce::Decibels::gainToDecibels(peakInputLevel, -60.0f));
    outputLevelDB.store(juce::Decibels::gainToDecibels(peakOutputLevel, -60.0f));
    currentGainReductionDB.store(peakGainReduction);
    currentEnvelopeDB.store(envelopeDB);
}

juce::AudioProcessorEditor* OuariconCompAudioProcessor::createEditor()
{
    return new OuariconCompAudioProcessorEditor(*this);
}

void OuariconCompAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Use preset manager for complete state (includes current preset name)
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OuariconCompAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

// DSP Helper Methods
float OuariconCompAudioProcessor::calculateGainReduction(float inputLevel, float thresholdDB,
                                                          float ratio, float kneeDB)
{
    float x = inputLevel - thresholdDB;

    if (x < -kneeDB / 2.0f)
    {
        // Below knee - no compression
        return 0.0f;
    }
    else if (x > kneeDB / 2.0f)
    {
        // Above knee - full compression
        return x - (x / ratio);
    }
    else
    {
        // Inside knee - smooth transition (quadratic curve)
        float kneeFactor = (x + kneeDB / 2.0f) / kneeDB;
        float fullReduction = x - (x / ratio);
        return kneeFactor * kneeFactor * fullReduction;
    }
}

void OuariconCompAudioProcessor::updateCoefficients(float attackTimeMs, float releaseTimeMs, double sampleRate)
{
    // Calculate attack/release coefficients using exponential formula
    // coeff = 1 - exp(-1 / (timeMs * sampleRate / 1000))
    attackCoeff = 1.0f - std::exp(-1.0f / (attackTimeMs * static_cast<float>(sampleRate) / 1000.0f));
    releaseCoeff = 1.0f - std::exp(-1.0f / (releaseTimeMs * static_cast<float>(sampleRate) / 1000.0f));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconCompAudioProcessor();
}
