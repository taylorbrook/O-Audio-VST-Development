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
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_freq", 1 }, "LF Frequency",
        juce::NormalisableRange<float>(30.0f, 500.0f, 0.1f, 0.3f), 100.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lf_gain", 1 }, "LF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lf_on", 1 }, "LF On", true));

    // LMF Band (Low-Mid Frequency Bell)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_freq", 1 }, "LMF Frequency",
        juce::NormalisableRange<float>(100.0f, 2000.0f, 0.1f, 0.3f), 500.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lmf_gain", 1 }, "LMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "lmf_q", 1 }, "LMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" }, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lmf_on", 1 }, "LMF On", true));

    // HMF Band (High-Mid Frequency Bell)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_freq", 1 }, "HMF Frequency",
        juce::NormalisableRange<float>(500.0f, 8000.0f, 0.1f, 0.3f), 2000.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hmf_gain", 1 }, "HMF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "hmf_q", 1 }, "HMF Q",
        juce::StringArray { "WIDE", "MED", "TIGHT" }, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hmf_on", 1 }, "HMF On", true));

    // HF Band (High Frequency Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_freq", 1 }, "HF Frequency",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.1f, 0.3f), 8000.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "hf_gain", 1 }, "HF Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "hf_on", 1 }, "HF On", true));

    // Global Controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output_gain", 1 }, "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "analog", 1 }, "Analog", true));

    return layout;
}

OuariconAnalogEQAudioProcessor::OuariconAnalogEQAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

void OuariconAnalogEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    lfFilter.prepare(spec);
    lmfFilter.prepare(spec);
    hmfFilter.prepare(spec);
    hfFilter.prepare(spec);
    saturation.prepare(spec);
    outputGain.prepare(spec);

    lfFilter.reset();
    lmfFilter.reset();
    hmfFilter.reset();
    hfFilter.reset();
    saturation.reset();
    outputGain.reset();

    // Gentle warmth: low drive (0.5x) preserves dynamics,
    // 2.0x post-gain compensates for tanh compression
    saturation.functionToUse = [](float x) { return std::tanh(x * 0.5f) * 2.0f; };
}

void OuariconAnalogEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters
    const float lfFreq  = parameters.getRawParameterValue("lf_freq")->load();
    const float lfGain  = parameters.getRawParameterValue("lf_gain")->load();
    const bool  lfOn    = parameters.getRawParameterValue("lf_on")->load() > 0.5f;

    const float lmfFreq = parameters.getRawParameterValue("lmf_freq")->load();
    const float lmfGain = parameters.getRawParameterValue("lmf_gain")->load();
    const int   lmfQ    = static_cast<int>(parameters.getRawParameterValue("lmf_q")->load());
    const bool  lmfOn   = parameters.getRawParameterValue("lmf_on")->load() > 0.5f;

    const float hmfFreq = parameters.getRawParameterValue("hmf_freq")->load();
    const float hmfGain = parameters.getRawParameterValue("hmf_gain")->load();
    const int   hmfQ    = static_cast<int>(parameters.getRawParameterValue("hmf_q")->load());
    const bool  hmfOn   = parameters.getRawParameterValue("hmf_on")->load() > 0.5f;

    const float hfFreq  = parameters.getRawParameterValue("hf_freq")->load();
    const float hfGain  = parameters.getRawParameterValue("hf_gain")->load();
    const bool  hfOn    = parameters.getRawParameterValue("hf_on")->load() > 0.5f;

    const float outputGainDB = parameters.getRawParameterValue("output_gain")->load();
    const bool  analogOn     = parameters.getRawParameterValue("analog")->load() > 0.5f;

    // Update filter coefficients
    auto dBtoGain = [](float dB) { return std::pow(10.0f, dB / 20.0f); };

    *lfFilter.state  = *IIRCoefficients::makeLowShelf(currentSampleRate, lfFreq, 0.707f, dBtoGain(lfGain));
    *lmfFilter.state = *IIRCoefficients::makePeakFilter(currentSampleRate, lmfFreq, qValues[lmfQ], dBtoGain(lmfGain));
    *hmfFilter.state = *IIRCoefficients::makePeakFilter(currentSampleRate, hmfFreq, qValues[hmfQ], dBtoGain(hmfGain));
    *hfFilter.state  = *IIRCoefficients::makeHighShelf(currentSampleRate, hfFreq, 0.707f, dBtoGain(hfGain));

    outputGain.setGainDecibels(outputGainDB);

    // Process audio: LF -> LMF -> HMF -> HF -> Saturation -> Output Gain
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    if (lfOn)  lfFilter.process(context);
    if (lmfOn) lmfFilter.process(context);
    if (hmfOn) hmfFilter.process(context);
    if (hfOn)  hfFilter.process(context);

    if (analogOn) saturation.process(context);
    outputGain.process(context);

    // VU Meter - peak level after all processing
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peakLevel = std::max(peakLevel, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));

    outputLevelDB.store(peakLevel > 0.00001f
        ? juce::Decibels::gainToDecibels(peakLevel)
        : -100.0f, std::memory_order_relaxed);
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconAnalogEQAudioProcessor();
}
