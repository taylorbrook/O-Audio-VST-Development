/*
  ==============================================================================

    Ouaricon Analog EQ - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OuariconAnalogEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // LF Band (Low Frequency Shelf)
    // lf_freq - Corner frequency (30-500 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_freq", 1 },
        "LF Frequency",
        juce::NormalisableRange<float>(30.0f, 500.0f, 0.1f, 0.3f),  // Logarithmic skew
        100.0f,
        "Hz"
    ));

    // lf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_gain", 1 },
        "LF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // lf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lf_on", 1 },
        "LF On",
        true
    ));

    // LMF Band (Low-Mid Frequency Bell)
    // lmf_freq - Center frequency (100-2000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_freq", 1 },
        "LMF Frequency",
        juce::NormalisableRange<float>(100.0f, 2000.0f, 0.1f, 0.3f),
        500.0f,
        "Hz"
    ));

    // lmf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_gain", 1 },
        "LMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // lmf_q - Q factor selection (WIDE/MED/TIGHT)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lmf_q", 1 },
        "LMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" },
        1  // Default: MED
    ));

    // lmf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lmf_on", 1 },
        "LMF On",
        true
    ));

    // HMF Band (High-Mid Frequency Bell)
    // hmf_freq - Center frequency (500-8000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_freq", 1 },
        "HMF Frequency",
        juce::NormalisableRange<float>(500.0f, 8000.0f, 0.1f, 0.3f),
        2000.0f,
        "Hz"
    ));

    // hmf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_gain", 1 },
        "HMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // hmf_q - Q factor selection (WIDE/MED/TIGHT)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "hmf_q", 1 },
        "HMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" },
        1  // Default: MED
    ));

    // hmf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hmf_on", 1 },
        "HMF On",
        true
    ));

    // HF Band (High Frequency Shelf)
    // hf_freq - Corner frequency (2000-20000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_freq", 1 },
        "HF Frequency",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.1f, 0.3f),
        8000.0f,
        "Hz"
    ));

    // hf_gain - Boost/cut amount (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_gain", 1 },
        "HF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // hf_on - Band enable/bypass
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hf_on", 1 },
        "HF On",
        true
    ));

    // Global Controls
    // output_gain - Master output level (±12 dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // analog - Analog warmth/saturation enable
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "analog", 1 },
        "Analog",
        true
    ));

    return layout;
}

OuariconAnalogEQAudioProcessor::OuariconAnalogEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconAnalogEQAudioProcessor::~OuariconAnalogEQAudioProcessor()
{
}

void OuariconAnalogEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare all DSP components
    lfFilter.prepare(spec);
    lmfFilter.prepare(spec);
    hmfFilter.prepare(spec);
    hfFilter.prepare(spec);
    saturation.prepare(spec);
    outputGain.prepare(spec);

    // Reset components to initial state
    lfFilter.reset();
    lmfFilter.reset();
    hmfFilter.reset();
    hfFilter.reset();
    saturation.reset();
    outputGain.reset();

    // Initialize saturation transfer function: tanh(x * 1.5) * 1.1
    saturation.functionToUse = [](float x) { return std::tanh(x * 1.5f) * 1.1f; };

    // Update all filter coefficients for current sample rate
    updateFilterCoefficients();
}

void OuariconAnalogEQAudioProcessor::releaseResources()
{
    // Optional: Release resources when plugin not in use
    // DSP components handle their own cleanup automatically
}

void OuariconAnalogEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    float lfFreq = parameters.getRawParameterValue("lf_freq")->load();
    float lfGain = parameters.getRawParameterValue("lf_gain")->load();
    bool lfOn = parameters.getRawParameterValue("lf_on")->load() > 0.5f;

    float lmfFreq = parameters.getRawParameterValue("lmf_freq")->load();
    float lmfGain = parameters.getRawParameterValue("lmf_gain")->load();
    int lmfQChoice = static_cast<int>(parameters.getRawParameterValue("lmf_q")->load());
    bool lmfOn = parameters.getRawParameterValue("lmf_on")->load() > 0.5f;

    float hmfFreq = parameters.getRawParameterValue("hmf_freq")->load();
    float hmfGain = parameters.getRawParameterValue("hmf_gain")->load();
    int hmfQChoice = static_cast<int>(parameters.getRawParameterValue("hmf_q")->load());
    bool hmfOn = parameters.getRawParameterValue("hmf_on")->load() > 0.5f;

    float hfFreq = parameters.getRawParameterValue("hf_freq")->load();
    float hfGain = parameters.getRawParameterValue("hf_gain")->load();
    bool hfOn = parameters.getRawParameterValue("hf_on")->load() > 0.5f;

    float outputGainDB = parameters.getRawParameterValue("output_gain")->load();
    bool analogOn = parameters.getRawParameterValue("analog")->load() > 0.5f;

    // Update filter coefficients if parameters changed
    bool needsUpdate = false;

    if (lfFreq != previousLfFreq || lfGain != previousLfGain)
    {
        float gainFactor = std::pow(10.0f, lfGain / 20.0f);
        *lfFilter.state = *IIRCoefficients::makeLowShelf(spec.sampleRate, lfFreq, 0.707f, gainFactor);
        previousLfFreq = lfFreq;
        previousLfGain = lfGain;
    }

    if (lmfFreq != previousLmfFreq || lmfGain != previousLmfGain || lmfQChoice != previousLmfQ)
    {
        float gainFactor = std::pow(10.0f, lmfGain / 20.0f);
        float qValue = getQValueFromChoice(lmfQChoice);
        *lmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, lmfFreq, qValue, gainFactor);
        previousLmfFreq = lmfFreq;
        previousLmfGain = lmfGain;
        previousLmfQ = lmfQChoice;
    }

    if (hmfFreq != previousHmfFreq || hmfGain != previousHmfGain || hmfQChoice != previousHmfQ)
    {
        float gainFactor = std::pow(10.0f, hmfGain / 20.0f);
        float qValue = getQValueFromChoice(hmfQChoice);
        *hmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, hmfFreq, qValue, gainFactor);
        previousHmfFreq = hmfFreq;
        previousHmfGain = hmfGain;
        previousHmfQ = hmfQChoice;
    }

    if (hfFreq != previousHfFreq || hfGain != previousHfGain)
    {
        float gainFactor = std::pow(10.0f, hfGain / 20.0f);
        *hfFilter.state = *IIRCoefficients::makeHighShelf(spec.sampleRate, hfFreq, 0.707f, gainFactor);
        previousHfFreq = hfFreq;
        previousHfGain = hfGain;
    }

    // Update output gain
    outputGain.setGainDecibels(outputGainDB);

    // Process audio through DSP chain
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Sequential processing: LF → LMF → HMF → HF → Saturation → Output Gain
    if (lfOn)
        lfFilter.process(context);

    if (lmfOn)
        lmfFilter.process(context);

    if (hmfOn)
        hmfFilter.process(context);

    if (hfOn)
        hfFilter.process(context);

    if (analogOn)
        saturation.process(context);

    outputGain.process(context);

    // VU Meter - Calculate peak level after all processing
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        peakLevel = std::max(peakLevel, channelPeak);
    }
    float levelDB = peakLevel > 0.00001f
        ? juce::Decibels::gainToDecibels(peakLevel)
        : -100.0f;
    outputLevelDB.store(levelDB, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* OuariconAnalogEQAudioProcessor::createEditor()
{
    return new OuariconAnalogEQAudioProcessorEditor(*this);
}

void OuariconAnalogEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconAnalogEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Helper function to map Q choice parameter to Q value
float OuariconAnalogEQAudioProcessor::getQValueFromChoice(int choiceIndex)
{
    switch (choiceIndex)
    {
        case 0: return 0.5f;  // WIDE - broad, gentle curves
        case 1: return 1.0f;  // MED - balanced, musical curves
        case 2: return 2.0f;  // TIGHT - focused, surgical curves
        default: return 1.0f; // Default to MED
    }
}

// Helper function to update all filter coefficients
void OuariconAnalogEQAudioProcessor::updateFilterCoefficients()
{
    // Read all parameters
    float lfFreq = parameters.getRawParameterValue("lf_freq")->load();
    float lfGain = parameters.getRawParameterValue("lf_gain")->load();

    float lmfFreq = parameters.getRawParameterValue("lmf_freq")->load();
    float lmfGain = parameters.getRawParameterValue("lmf_gain")->load();
    int lmfQChoice = static_cast<int>(parameters.getRawParameterValue("lmf_q")->load());

    float hmfFreq = parameters.getRawParameterValue("hmf_freq")->load();
    float hmfGain = parameters.getRawParameterValue("hmf_gain")->load();
    int hmfQChoice = static_cast<int>(parameters.getRawParameterValue("hmf_q")->load());

    float hfFreq = parameters.getRawParameterValue("hf_freq")->load();
    float hfGain = parameters.getRawParameterValue("hf_gain")->load();

    // Update all filter coefficients
    float lfGainFactor = std::pow(10.0f, lfGain / 20.0f);
    *lfFilter.state = *IIRCoefficients::makeLowShelf(spec.sampleRate, lfFreq, 0.707f, lfGainFactor);

    float lmfGainFactor = std::pow(10.0f, lmfGain / 20.0f);
    float lmfQ = getQValueFromChoice(lmfQChoice);
    *lmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, lmfFreq, lmfQ, lmfGainFactor);

    float hmfGainFactor = std::pow(10.0f, hmfGain / 20.0f);
    float hmfQ = getQValueFromChoice(hmfQChoice);
    *hmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, hmfFreq, hmfQ, hmfGainFactor);

    float hfGainFactor = std::pow(10.0f, hfGain / 20.0f);
    *hfFilter.state = *IIRCoefficients::makeHighShelf(spec.sampleRate, hfFreq, 0.707f, hfGainFactor);

    // Update previous values
    previousLfFreq = lfFreq;
    previousLfGain = lfGain;
    previousLmfFreq = lmfFreq;
    previousLmfGain = lmfGain;
    previousLmfQ = lmfQChoice;
    previousHmfFreq = hmfFreq;
    previousHmfGain = hmfGain;
    previousHmfQ = hmfQChoice;
    previousHfFreq = hfFreq;
    previousHfGain = hfGain;
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconAnalogEQAudioProcessor();
}
