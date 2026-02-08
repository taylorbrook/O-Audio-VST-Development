/*
  ==============================================================================

    O-Chorus - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rate", 1}, "Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.35f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1}, "Depth", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voices", 1}, "Voices", 1, 8, 4));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"width", 1}, "Width", 0.0f, 1.0f, 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tone", 1}, "Tone",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1}, "Drive", 0.0f, 1.0f, 0.3f));

    return {params.begin(), params.end()};
}

OChorusAudioProcessor::OChorusAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
#if OUARICON_LICENSING_ENABLED
    licenseManager = std::make_unique<OuariconLicense>(
        "ouaricon-chorus", OUARICON_SUPABASE_URL, OUARICON_SUPABASE_ANON_KEY);
#endif
}

void OChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    chorusEngine.prepare(sampleRate, samplesPerBlock);
}

void OChorusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read all parameters via atomic loads (real-time safe)
    float rate   = parameters.getRawParameterValue("rate")->load();
    float depth  = parameters.getRawParameterValue("depth")->load();
    int   voices = static_cast<int>(parameters.getRawParameterValue("voices")->load());
    float width  = parameters.getRawParameterValue("width")->load();
    float tone   = parameters.getRawParameterValue("tone")->load();
    float mix    = parameters.getRawParameterValue("mix")->load();
    float drive  = parameters.getRawParameterValue("drive")->load();

    chorusEngine.process(buffer, rate, depth, voices, width, tone, mix, drive);
}

juce::AudioProcessorEditor* OChorusAudioProcessor::createEditor()
{
    return new OChorusAudioProcessorEditor(*this);
}

void OChorusAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OChorusAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OChorusAudioProcessor();
}
